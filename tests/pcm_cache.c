/* SPDX-License-Identifier: GPL-3.0-only */
/* 실제 엔진 준비 경로와 공개 소스 kernel을 독립 PCM 오라클로 검증한다. */
#include "../src/engine.c"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(((McEngine *)0)->pcm_cache)==MC_VOICES*72u,"32 sample cache");
_Static_assert(offsetof(McEngine,live_keys)==offsetof(McEngine,pcm_cache)+sizeof(((McEngine *)0)->pcm_cache),"cache layout");
static McEngine eng;
static uint32_t rng=0x1234abcdu;
static unsigned long calls,misses,words;
static const struct { uint32_t step,ratio; } motion[5]={
    {65536,65536},{24317,65536},{131849,70001},{5003,58000},{196609,65536}};
static uint32_t rnd(void) { rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng; }
static const McPcmMip *level_of(unsigned k,unsigned mip) { return &hy_pcm_zone(k)->mip[mip]; }
static int32_t oracle(McVoice *v,uint32_t step,const McPcmMip *level) {
    if(v->hybrid.ended_mask&HY_PCM)return 0;
    unsigned pos=v->pcm_pos>>16;
    if(pos>=level->length) { v->hybrid.ended_mask|=HY_PCM;return 0; }
    unsigned next=pos+1;
    if(level->loop_end&&next>=level->loop_end)next=level->loop_start;
    else if(next>=level->length)next=level->length-1u;
    int32_t sample=level->data[pos];
    sample+=(int32_t)((int64_t)(level->data[next]-sample)*(v->pcm_pos&65535u)/65536);
    if(!level->loop_end&&pos+32u>=level->length)sample=sample*(int32_t)(level->length-1u-pos)/32;
    uint64_t advance=(uint64_t)v->pcm_pos+step;
    if(level->loop_end) {
        uint64_t end=(uint64_t)level->loop_end<<16,span=(uint64_t)(level->loop_end-level->loop_start)<<16;
        while(advance>=end)advance-=span;
    } else if(advance>=((uint64_t)level->length<<16)) {
        advance=(uint32_t)level->length<<16;v->hybrid.ended_mask|=HY_PCM;
    }
    v->pcm_pos=(uint32_t)advance;
    return sample*127/381;
}
static void both(McPcmCache *c,McVoice *v,McVoice *o,uint32_t ratio) {
    McPcmCache before=*c;
    const McPcmMip *level=level_of(v->hybrid.pcm_zone,v->sample_level);
    uint32_t step=(uint32_t)(((uint64_t)v->sample_step*ratio)>>16);
    McVoiceRender r={.profile=hy_profile(v->patch.profile),.pcm=level,.cache=c,.pcm_step=step,
        .va_morph_frames=(uint32_t)hy_profile(v->patch.profile)->va_morph_ms*MC_RATE/1000u};
    int32_t got=hy_source_sample(v,&r),want=oracle(o,step,level);
    assert(got==want&&v->pcm_pos==o->pcm_pos&&v->hybrid.ended_mask==o->hybrid.ended_mask);
    calls++;
    if(c->window!=before.window||c->count!=before.count||c->zone!=before.zone||c->mip!=before.mip) {
        misses++;words+=c->count;
    }
    if(c->count)assert(c->count<=MC_PCM_WINDOW&&(unsigned)c->window+c->count<=level_of(c->zone,c->mip)->length);
}
static uint32_t random_pos(unsigned len) {
    unsigned s;
    if(rnd()&1)s=rnd()%(len+8u);
    else { unsigned back=rnd()%200u;s=len>back?len-back:0; }
    return ((uint32_t)s<<16)|(rnd()&65535u);
}
static void retarget(McVoice *v,McVoice *o,unsigned k,unsigned mip,uint32_t pos,uint32_t step) {
    memset(v,0,sizeof *v);v->patch=mc_presets[MC_PRESET_STAGE_PIANO];
    v->patch.source_mask=HY_PCM;v->patch.pcm_mix=127;v->velocity=127;hy_source_init(v);
    v->hybrid.pcm_zone=(uint8_t)k;v->sample_level=(uint8_t)mip;
    v->pcm_pos=pos;v->sample_step=step;*o=*v;
}
static void random_retarget(McVoice *v,McVoice *o,uint32_t *ratio) {
    unsigned k=rnd()%MC_PCM_ZONES,mip=rnd()%7u,m=rnd()%5u;
    retarget(v,o,k,mip,random_pos(level_of(k,mip)->length),motion[m].step);
    *ratio=motion[rnd()%5u].ratio;
}
static void sweep_all_levels(void) {
    unsigned scenario=0,levels=0;
    for(unsigned k=0;k<MC_PCM_ZONES;k++)for(unsigned mip=0;mip<hy_pcm_zone(k)->levels;mip++) {
        const McPcmMip *lv=level_of(k,mip);unsigned len=lv->length;
        uint32_t starts[7]={0,125,len>140?len-140u:0,len>130?len-130u:0,len>3?len-3u:0,
            lv->loop_end>3?lv->loop_end-3u:0,lv->loop_start};levels++;
        for(unsigned s=0;s<7;s++)for(unsigned m=0;m<5;m++) {
            McVoice v,o;McPcmCache *c=&eng.pcm_cache[scenario&7u];
            if(scenario&1u)memset(c,0,sizeof *c);
            scenario++;retarget(&v,&o,k,mip,(starts[s]<<16)|((s*0x2493u)&65535u),motion[m].step);
            for(unsigned n=0;n<300;n++)both(c,&v,&o,motion[m].ratio);
        }
    }
    assert(levels==MC_PCM_ZONES*7u);
}
static void swap_sources(void) {
    McVoice v,o;uint32_t ratio;memset(eng.pcm_cache,0,sizeof eng.pcm_cache);
    random_retarget(&v,&o,&ratio);
    for(unsigned it=0;it<30000;it++) {
        uint32_t r=rnd();
        if(r%53u==0)random_retarget(&v,&o,&ratio);
        else if(r%211u==1) { v.sample_level=(uint8_t)(rnd()%7u);v.hybrid.ended_mask=0;o=v; }
        else if(r%307u==2) { v.hybrid.pcm_zone=(uint8_t)(rnd()%MC_PCM_ZONES);v.hybrid.ended_mask=0;o=v; }
        both(&eng.pcm_cache[0],&v,&o,ratio);
    }
}
static void interleave_voices(void) {
    McVoice v[MC_VOICES],o[MC_VOICES];uint32_t ratio[MC_VOICES];
    memset(eng.pcm_cache,0,sizeof eng.pcm_cache);
    for(unsigned i=0;i<MC_VOICES;i++)random_retarget(&v[i],&o[i],&ratio[i]);
    for(unsigned it=0;it<80000;it++) {
        unsigned i=it%MC_VOICES;
        if(it%997u==i)random_retarget(&v[i],&o[i],&ratio[i]);
        both(&eng.pcm_cache[i],&v[i],&o[i],ratio[i]);
    }
}
static void sequential_bound(void) {
    const McPcmMip *lv=level_of(2,0);assert(!lv->loop_end);
    McVoice v,o;unsigned long m0=misses;
    memset(&eng.pcm_cache[0],0,sizeof eng.pcm_cache[0]);retarget(&v,&o,2,0,0,65536);
    for(unsigned n=0;n<(unsigned)lv->length+2u;n++)both(&eng.pcm_cache[0],&v,&o,65536);
    assert(misses-m0<=lv->length/(MC_PCM_WINDOW-1u)+2u);
}
static void multi_loop(void) {
    const int16_t data[5]={12000,-8000,5000,-13000,11000};
    const McPcmMip level={data,5,2,5};
    McVoice v,o;McPcmCache c={0};retarget(&v,&o,2,0,4u<<16,UINT32_MAX);
    McVoiceRender r={.profile=hy_profile(v.patch.profile),.pcm=&level,.cache=&c,.pcm_step=UINT32_MAX,
        .va_morph_frames=(uint32_t)hy_profile(v.patch.profile)->va_morph_ms*MC_RATE/1000u};
    for(unsigned i=0;i<128;i++) {
        assert(hy_source_sample(&v,&r)==oracle(&o,UINT32_MAX,&level));
        assert(v.pcm_pos==o.pcm_pos&&v.pcm_pos>=(2u<<16)&&v.pcm_pos<(5u<<16));
    }
    puts("PASS: 64-bit cursor addition and one-step multiple-loop modulo");
}
static void zone_selection(void) {
    const unsigned notes[]={12,48,54,55,60,66,67,72,108};
    const unsigned zones[]={1,1,1,2,2,2,3,3,3};
    for(unsigned n=0;n<sizeof notes/sizeof *notes;n++) {
        mc_init(&eng);mc_preset(&eng,0,MC_PRESET_STAGE_PIANO);
        mc_note(&eng,0,notes[n],127,0,0);
        assert(eng.voice[0].hybrid.pcm_zone==zones[n]);
        int16_t out[2];mc_render(&eng,out,1);
        const McVoice *v=&eng.voice[0];const McPcmZone *z=hy_pcm_zone(v->hybrid.pcm_zone);
        assert(v->sample_level<z->levels);
        assert(v->sample_step<=65536u||v->sample_level+1u==z->levels);
        uint32_t step=(uint32_t)((uint64_t)eng.pitch_cache[0].increment*MC_PCM_RATE/z->tuning_hz_q16);
        for(unsigned mip=0;mip<v->sample_level;mip++)step=(step+1u)/2u;
        assert(step==v->sample_step);
    }
    mc_init(&eng);mc_preset(&eng,0,MC_PRESET_GLASS_PAD);mc_note(&eng,0,60,127,0,0);
    int16_t out[2];mc_render(&eng,out,1);
    assert(eng.pitch_cache[0].pcm_step!=(uint32_t)((uint64_t)MC_PCM_RATE*65536u/MC_RATE));
    puts("PASS: nearest root/ties, actual tuning correction and safe pitch-derived mip");
}
static void muted_pitch(void) {
    mc_init(&eng);mc_preset(&eng,0,MC_PRESET_STAGE_PIANO);mc_note(&eng,0,89,127,0,0);
    int16_t out[256];mc_render(&eng,out,1);
    assert(!eng.voice[0].sample_level);
    assert(!mc_parameter_set(&eng,0,MC_PARAM_SOURCE_MASK,0));mc_render(&eng,out,128);
    McVoice before=eng.voice[0];McPcmCache cache=eng.pcm_cache[0];
    eng.midi.bend[0]=8191;mc_render(&eng,out,1);
    assert(eng.voice[0].pcm_pos==before.pcm_pos&&eng.voice[0].sample_level==before.sample_level);
    assert(eng.voice[0].phase==before.phase&&eng.voice[0].phase2==before.phase2);
    assert(!memcmp(&cache,&eng.pcm_cache[0],sizeof cache));
    assert(!mc_parameter_set(&eng,0,MC_PARAM_SOURCE_MASK,HY_ALL));mc_render(&eng,out,1);
    assert(eng.voice[0].sample_level==1);
    assert(eng.voice[0].pcm_pos==(before.pcm_pos>>1)+eng.voice[0].sample_step);
    puts("PASS: muted sources preserve cursor/phase/cache across a bend-driven mip boundary");
}
static void phase_frame(void) {
    uint32_t phase[MC_VOICES],second[MC_VOICES],base[MC_VOICES],ratio[MC_VOICES];
    unsigned moving[MC_VOICES];
    int16_t stereo[2];
    for(unsigned i=0;i<MC_VOICES;i++) {
        const McVoice *v=&eng.voice[i];
        phase[i]=v->phase;second[i]=v->phase2;base[i]=v->increment;
        moving[i]=v->increment!=v->target;
        int bend=eng.midi.bend[v->part];
        ratio[i]=(uint32_t)(65536+bend*(bend<0?7150:8026)/8192);
        if(!v->active)continue;
        assert(v->stage!=2&&!v->gate_left);
        if(moving[i]) {
            int32_t diff=(int32_t)v->target-(int32_t)base[i];
            int32_t step=diff/(20+v->patch.glide*16);
            if(!step)step=diff>0?1:-1;
            base[i]=(uint32_t)((int32_t)base[i]+step);
        }
        uint32_t advance=(uint32_t)(((uint64_t)base[i]*ratio[i])>>16);
        if(v->hybrid.gain[1])phase[i]+=advance;
        if(v->hybrid.gain[2])second[i]+=advance;
    }
    mc_render(&eng,stereo,1);
    for(unsigned i=0;i<MC_VOICES;i++) {
        const McVoice *v=&eng.voice[i];
        assert(v->phase==phase[i]&&v->phase2==second[i]&&v->increment==base[i]);
        if(v->active&&!moving[i]) {
            assert(eng.pitch_cache[i].base==base[i]&&eng.pitch_cache[i].ratio==ratio[i]);
            uint32_t advance=(uint32_t)(((uint64_t)base[i]*ratio[i])>>16);
            assert(eng.pitch_cache[i].increment==advance);
            const McPcmZone *zone=hy_pcm_zone(v->hybrid.pcm_zone);
            assert(eng.pitch_cache[i].pcm_step==(zone?(uint32_t)((uint64_t)advance*MC_PCM_RATE/zone->tuning_hz_q16):0));
        }
    }
}
static void phase_cache(void) {
    static const int16_t bends[5]={-8192,-1,0,1,8191};
    mc_init(&eng);
    for(unsigned part=0;part<3;part++)mc_preset(&eng,part,MC_PRESET_STAGE_PIANO);
    for(unsigned i=0;i<MC_VOICES;i++)mc_note(&eng,i%3,12+i*12,100,0,0);
    for(unsigned bend=0;bend<5;bend++) {
        for(unsigned part=0;part<3;part++)eng.midi.bend[part]=bends[(bend+part)%5];
        for(unsigned sample=0;sample<3;sample++)phase_frame();
    }
    mc_note(&eng,0,108,100,0,0);phase_frame();
    mc_init(&eng);mc_preset(&eng,0,MC_PRESET_PORTA_LEAD);
    eng.project.patch[0].glide=64;mc_note(&eng,0,36,100,0,0);phase_frame();
    mc_note(&eng,0,60,100,MC_SLIDE,0);
    for(unsigned sample=0;sample<1024;sample++) {
        if(sample%128==0)eng.midi.bend[0]=bends[(sample/128)%5];
        if(sample==512)assert(!mc_parameter_set(&eng,0,10,0));
        phase_frame();
    }
    puts("PASS: exact phase cache, all8 slots, bend extremes/reversal, reuse, glide and live speed changes");
}
static void slice_preparation(void) {
    static McEngine other;
    int16_t block[257*2],frame[2];
    mc_init(&eng);
    eng.project.bpm=240;
    for(unsigned p=0;p<3;p++) {
        mc_preset(&eng,p,p==2?MC_PRESET_PORTA_LEAD:MC_PRESET_STAGE_PIANO);
        eng.project.arp[p].enabled=1;eng.project.arp[p].rate=2;
        eng.project.patch[p].glide=127;
        mc_key(&eng,p,48+p*12,100,1);mc_key(&eng,p,55+p*12,127,1);
        eng.arp[p].clock=MC_RATE*60u-eng.project.bpm*(1u<<2)*(7u+p);
    }
    other=eng;
    for(unsigned block_index=0;block_index<32;block_index++) {
        for(unsigned p=0;p<3;p++) {
            int bend=(int)((block_index*997u+p*2311u)%16384u)-8192;
            eng.midi.bend[p]=other.midi.bend[p]=(int16_t)bend;
            unsigned cutoff=(block_index*17u+p*37u)%128u;
            assert(!mc_parameter_set(&eng,p,offsetof(McPatch,cutoff),cutoff));
            assert(!mc_parameter_set(&other,p,offsetof(McPatch,cutoff),cutoff));
        }
        mc_render(&eng,block,257);
        for(unsigned n=0;n<257;n++) {
            mc_render(&other,frame,1);
            assert(block[n*2]==frame[0]&&block[n*2+1]==frame[1]);
        }
        assert(!memcmp(&eng,&other,sizeof eng));
    }
    puts("PASS: stopped three-part ARP source preparation, pitch/filter changes and complete state across 257/1-frame blocks");
    mc_demo_load(&eng,1);mc_start(&eng);
    eng.events=65;eng.demo_frame=63;
    other=eng;
    mc_render(&eng,block,257);
    for(unsigned n=0;n<257;n++) {
        mc_render(&other,frame,1);
        assert(block[n*2]==frame[0]&&block[n*2+1]==frame[1]);
    }
    assert(!memcmp(&eng,&other,sizeof eng));
    puts("PASS: demo filter automation inside a slice preserves PCM and complete state");
}
static void safe_va_bands(void) {
    const unsigned harmonics[]={32,24,16,8,4,2,1};
    const int bends[]={-8192,0,8191};
    for(unsigned note=12;note<=108;note++)for(unsigned bend=0;bend<3;bend++)
        for(unsigned amount=0;amount<=127;amount+=127)for(unsigned wave=0;wave<2;wave++)
            for(unsigned polarity=0;polarity<2;polarity++) {
            mc_init(&eng);mc_preset(&eng,0,MC_PRESET_STAGE_PIANO);
            eng.project.patch[0].source_mask|=HY_VA;
            eng.project.patch[0].wave=(uint8_t)wave;mc_note(&eng,0,note,127,0,0);
            McVoice *v=&eng.voice[0];McVoiceRender r;
            McAnalogVoice *analog=&eng.analog_voice[0];
            analog->random=123;analog->tuning=polarity?306:-307;
            analog->drift=polarity?32512:-32768;analog->target=analog->drift;analog->clock=1;
            uint32_t ratio=(uint32_t)(65536+bends[bend]*(bends[bend]<0?7150:8026)/8192);
            prepare_voice(v,&eng.pcm_cache[0],&eng.pitch_cache[0],analog,amount,ratio,&r);
            McHybridState initial=v->hybrid;
            prepare_voice(v,&eng.pcm_cache[0],&eng.pitch_cache[0],analog,amount,ratio,&r);
            assert(!memcmp(&initial,&v->hybrid,sizeof initial));
            /* Cross bands during actual glide and inspect post-Analog increments. */
            v->target=mc_frequency[note==108?12:108];v->patch.glide=127;
            for(unsigned sample=0;sample<128;sample++) {
                uint32_t phase=v->phase2;
                (void)melodic_sample(v,0,&r,32767);
                unsigned band=0;
                while(band<7&&(uint64_t)harmonics[band]*r.increment>=UINT32_C(0x73333333))band++;
                assert(r.table==(band<7?(wave?mc_square[band]:mc_saw[band]):0));
                assert(r.table_colour==(band<7?(wave?mc_square_colour[band]:mc_saw_colour[band]):0));
                assert(v->phase2-phase==r.increment);
            }
        }
    for(unsigned band=0;band<7;band++) {
        assert((uint64_t)mc_va_max_increment[band]*harmonics[band]<UINT32_C(0x73333333));
        assert((uint64_t)(mc_va_max_increment[band]+1u)*harmonics[band]>=UINT32_C(0x73333333));
    }
    puts("PASS: VA richest safe shared-phase band, MIDI 12..108/bend/Analog extremes and post-glide increments");
}
int main(void) {
    safe_va_bands();
    sweep_all_levels();
    multi_loop();
    zone_selection();
    muted_pitch();
    swap_sources();
    interleave_voices();
    sequential_bound();
    phase_cache();
    slice_preparation();
    memset(eng.pcm_cache,0xff,sizeof eng.pcm_cache);
    mc_init(&eng); /* 콜드 리셋은 캐시를 비운다 */
    for(unsigned i=0;i<sizeof eng.pcm_cache;i++) assert(((const uint8_t *)eng.pcm_cache)[i]==0);
    printf("pcm_cache offset=%zu extra=%zu bytes calls=%lu window misses=%lu copied=%lu int16 (uncached ~%lu reads)\n",
        offsetof(McEngine,pcm_cache),sizeof eng.pcm_cache,calls,misses,words,calls*2ul);
    puts("host 계수일 뿐 장치 타이밍 주장 아님");
    return 0;
}
