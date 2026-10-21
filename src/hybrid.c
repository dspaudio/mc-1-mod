/* SPDX-License-Identifier: GPL-3.0-only */
#include "hybrid.h"
#include "hybrid_profiles.h"
#include "pcm_bank.h"
#include "tables.h"
#include "fixed.h"

const McHybridProfile *hy_profile(unsigned profile) {
    return profile<MC_PRESETS?&mc_hybrid_profiles[profile]:0;
}
const McPcmZone *hy_pcm_zone(unsigned zone) {
    return zone<MC_PCM_ZONES?&mc_pcm_zones[zone]:0;
}
static int32_t sine(uint32_t phase) {
    unsigned i=phase>>22;
    int32_t a=mc_sine[i],difference=mc_sine[(i+1u)&1023u]-a;
    return a+difference*(int32_t)((phase>>6)&65535u)/65536;
}
void hy_source_init(McVoice *v) {
    const McHybridProfile *p=hy_profile(v->patch.profile);
    McHybridState *h=&v->hybrid;
    for(unsigned i=0;i<sizeof *h;i++)((uint8_t *)h)[i]=0;
    h->pcm_zone=HY_PCM_NONE;
    unsigned distance=256;
    for(unsigned i=0;i<MC_PCM_ZONES;i++)if(mc_pcm_zones[i].family==p->pcm_family) {
        unsigned root=mc_pcm_zones[i].root;
        unsigned d=v->note>root?v->note-root:root-v->note;
        if(d<distance) { distance=d;h->pcm_zone=(uint8_t)i; }
    }
    unsigned index=p->fm_index+v->velocity*p->velocity_index/127u;
    h->fm_index=(uint8_t)(index>127?127:index);
    h->velocity_cutoff=(uint16_t)(v->velocity*p->velocity_cutoff/127u);
    uint32_t frames=(uint32_t)p->fm_decay_ms*MC_RATE/1000u;
    uint32_t body=(uint32_t)p->fm_body_q15<<16;
    uint32_t depth=UINT32_C(0x80000000)-body;
    h->fm_env=frames?UINT32_C(0x80000000):body;
    h->fm_step=frames?depth/frames:0;
    h->fm_remainder=frames?depth%frames:0;
    h->fm_left=frames;
    h->fm_frames=(uint32_t)p->fm_duration_ms*MC_RATE/1000u;
    uint32_t morph=(uint32_t)p->va_morph_ms*MC_RATE/1000u;
    h->va_colour=(uint32_t)(morph?p->va_colour_start_q15:p->va_colour_end_q15)<<16;
    int32_t colour_delta=(int32_t)p->va_colour_end_q15-p->va_colour_start_q15;
    uint32_t colour_depth=(uint32_t)(colour_delta<0?-colour_delta:colour_delta)*65536u;
    h->va_colour_step=morph?(int32_t)(colour_depth/morph):0;
    if(colour_delta<0)h->va_colour_step=-h->va_colour_step;
    unsigned mask=v->patch.source_mask&p->available_mask;
    h->gain[0]=h->target[0]=(uint8_t)(mask&HY_PCM?v->patch.pcm_mix:0);
    h->gain[1]=h->target[1]=(uint8_t)(mask&HY_FM?v->patch.fm_mix:0);
    h->gain[2]=h->target[2]=(uint8_t)(mask&HY_VA?v->patch.va_mix:0);
}
static int32_t pcm(McVoice *v,McVoiceRender *r) {
    const McPcmMip *level=r->pcm;
    McPcmCache *c=r->cache;
    unsigned pos=v->pcm_pos>>16;
    if(pos>=level->length) { v->hybrid.ended_mask|=HY_PCM;return 0; }
    unsigned next=pos+1u;
    if(level->loop_end&&next>=level->loop_end)next=level->loop_start;
    else if(next>=level->length)next=level->length-1u;
    unsigned offset=pos-c->window;
    if(c->zone!=v->hybrid.pcm_zone||c->mip!=v->sample_level||
       offset>=c->count||(next==pos+1u&&offset+1u>=c->count)) {
        unsigned n=level->length-pos;
        if(n>MC_PCM_WINDOW)n=MC_PCM_WINDOW;
        for(unsigned i=0;i<n;i++)c->data[i]=level->data[pos+i];
        c->window=(uint16_t)pos;c->count=(uint16_t)n;
        c->zone=v->hybrid.pcm_zone;c->mip=v->sample_level;offset=0;
    }
    int32_t sample=c->data[offset];
    unsigned after_offset=next-c->window;
    int32_t after=after_offset<c->count?c->data[after_offset]:level->data[next];
    sample+=(int32_t)((int64_t)(after-sample)*(v->pcm_pos&65535u)/65536);
    if(!level->loop_end&&pos+32u>=level->length)
        sample=sample*(int32_t)(level->length-1u-pos)/32;
    uint64_t advance=(uint64_t)v->pcm_pos+r->pcm_step;
    if(level->loop_end&&advance>=((uint64_t)level->loop_end<<16)) {
        uint32_t start=(uint32_t)level->loop_start<<16;
        uint32_t span=(uint32_t)(level->loop_end-level->loop_start)<<16;
        advance=start+(advance-start)%span;
    } else if(!level->loop_end&&advance>=((uint64_t)level->length<<16)) {
        advance=(uint32_t)level->length<<16;v->hybrid.ended_mask|=HY_PCM;
    }
    v->pcm_pos=(uint32_t)advance;
    return sample;
}
static int32_t fm(McVoice *v,const McVoiceRender *r) {
    McHybridState *h=&v->hybrid;
    uint32_t feedback=(uint32_t)((int64_t)h->previous_mod*r->profile->fm_feedback*1024);
    int32_t mod=sine(h->modulator+feedback);
    h->previous_mod=mod;
    mod=(int32_t)mc_div_pow2_64((int64_t)mod*h->fm_env,31);
    uint32_t offset=(uint32_t)((int64_t)mod*h->fm_index*2048);
    int32_t out=sine(v->phase+offset);
    if(h->fm_frames&&h->fm_frames-v->age<128u)
        out=out*(int32_t)(h->fm_frames-v->age)/128;
    v->phase+=r->increment;
    h->modulator+=r->increment*r->profile->fm_ratio;
    if(h->fm_left) {
        uint32_t drop=h->fm_step;
        h->fm_error+=h->fm_remainder;
        if(h->fm_error>=r->fm_decay_frames) {
            h->fm_error-=r->fm_decay_frames;drop++;
        }
        h->fm_env-=drop;
        if(!--h->fm_left)h->fm_env=(uint32_t)r->profile->fm_body_q15<<16;
    }
    return out;
}
static int32_t table_sample(const int16_t *table,uint32_t phase) {
    unsigned i=phase>>24;
    int32_t a=table[i],difference=table[(i+1u)&255u]-a;
    return a+difference*(int32_t)((phase>>16)&255u)/256;
}
static int32_t va(McVoice *v,const McVoiceRender *r) {
    int32_t out=0;
    if(r->increment<=mc_va_max_increment[6]) {
        if(v->patch.wave==MC_WAVE_SINE)out=sine(v->phase2);
        else if(r->table) {
            unsigned weight=v->hybrid.va_colour>>16;
            if(weight==32768u)out=table_sample(r->table_colour,v->phase2);
            else {
                out=table_sample(r->table,v->phase2);
                if(weight)out+=(int32_t)mc_div_pow2_64(
                    (int64_t)(table_sample(r->table_colour,v->phase2)-out)*weight,15);
            }
        }
    }
    v->phase2+=r->increment;
    return out;
}
int32_t hy_source_sample(McVoice *v,McVoiceRender *r) {
    McHybridState *h=&v->hybrid;
    unsigned mask=v->patch.source_mask&r->profile->available_mask;
    h->target[0]=(uint8_t)(mask&HY_PCM?v->patch.pcm_mix:0);
    h->target[1]=(uint8_t)(mask&HY_FM?v->patch.fm_mix:0);
    h->target[2]=(uint8_t)(mask&HY_VA?v->patch.va_mix:0);
    for(unsigned i=0;i<3;i++) {
        if(h->gain[i]<h->target[i])h->gain[i]++;
        else if(h->gain[i]>h->target[i])h->gain[i]--;
    }
    /* Finite FM lifetime follows common age even while muted. */
    if(h->fm_frames&&v->age>=h->fm_frames)h->ended_mask|=HY_FM;
    if(v->age>=r->va_morph_frames) {
        h->va_colour=(uint32_t)r->profile->va_colour_end_q15<<16;
        h->va_colour_step=0;
    }
    int32_t sample=0;
    if(h->gain[0]&&!(h->ended_mask&HY_PCM))sample+=pcm(v,r)*h->gain[0];
    if(h->gain[1]&&!(h->ended_mask&HY_FM))sample+=fm(v,r)*h->gain[1];
    if(h->gain[2])sample+=va(v,r)*h->gain[2];
    h->va_colour=(uint32_t)((int64_t)h->va_colour+h->va_colour_step);
    return sample/381;
}
