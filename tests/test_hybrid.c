/* SPDX-License-Identifier: GPL-3.0-only */
#include "hybrid.h"
#include "tables.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static McEngine a,b,before;
static int16_t whole[4096*2],split[4096*2];
static void setup(McEngine *e,unsigned profile,unsigned mask) {
    mc_init(e);
    memset(e->project.fx_send,0,sizeof e->project.fx_send);
    for(unsigned p=0;p<3;p++) {
        mc_preset(e,p,profile);
        McPatch *v=&e->project.patch[p];
        v->source_mask=(uint8_t)mask;v->attack=0;v->sustain=127;v->release=0;
        const McHybridProfile *r=hy_profile(profile);
        v->pcm_mix=(uint8_t)(r->available_mask&HY_PCM?127:0);
        v->fm_mix=(uint8_t)(r->available_mask&HY_FM?127:0);
        v->va_mix=(uint8_t)(r->available_mask&HY_VA?127:0);
    }
}
static unsigned active(const McEngine *e) {
    unsigned n=0;
    for(unsigned i=0;i<MC_VOICES;i++)n+=e->voice[i].active!=0;
    return n;
}
static int audible(const int16_t *audio,unsigned frames) {
    for(unsigned i=0;i<frames*2;i++)if(audio[i])return 1;
    return 0;
}
static void render(McEngine *e,unsigned frames) {
    while(frames) {
        unsigned n=frames>4096?4096:frames;
        mc_render(e,whole,n);frames-=n;
    }
}
static void sources(void) {
    for(unsigned mask=1;mask<=HY_ALL;mask++) {
        setup(&a,MC_PRESET_STAGE_PIANO,mask);
        mc_note(&a,1,60,127,0,0);
        McVoice *v=&a.voice[0];
        render(&a,512);
        assert(active(&a)==1&&v->age==512&&v->envelope==127*258);
        assert(!!v->pcm_pos==!!(mask&HY_PCM));
        assert(!!v->phase==!!(mask&HY_FM));
        assert(!!v->phase2==!!(mask&HY_VA));
        assert(!!a.pcm_cache[0].count==!!(mask&HY_PCM));
        assert(audible(whole,512));
        for(unsigned i=0;i<3;i++)assert(v->hybrid.gain[i]==(mask&(1u<<i)?127:0));
    }
    for(unsigned source=0;source<3;source++) {
        setup(&a,MC_PRESET_STAGE_PIANO,HY_ALL);
        a.project.patch[0].pcm_mix=(uint8_t)(source==0?127:0);
        a.project.patch[0].fm_mix=(uint8_t)(source==1?127:0);
        a.project.patch[0].va_mix=(uint8_t)(source==2?127:0);
        mc_note(&a,0,60,127,0,0);render(&a,512);
        assert(!!a.voice[0].pcm_pos==(source==0));
        assert(!!a.voice[0].phase==(source==1));
        assert(!!a.voice[0].phase2==(source==2));
        assert(audible(whole,512));
    }
}
static void ramp_and_end(void) {
    setup(&a,MC_PRESET_STAGE_PIANO,HY_ALL);
    mc_note(&a,0,60,127,0,0);render(&a,64);
    assert(!mc_parameter_set(&a,0,MC_PARAM_SOURCE_MASK,0));
    for(unsigned n=0;n<127;n++) {
        render(&a,1);
        for(unsigned i=0;i<3;i++)assert(a.voice[0].hybrid.gain[i]==126u-n);
    }
    McVoice held=a.voice[0];McPcmCache cache=a.pcm_cache[0];
    render(&a,4096);
    assert(a.voice[0].pcm_pos==held.pcm_pos&&a.voice[0].phase==held.phase&&a.voice[0].phase2==held.phase2);
    assert(!memcmp(&cache,&a.pcm_cache[0],sizeof cache)&&active(&a)==1);
    assert(!mc_parameter_set(&a,0,MC_PARAM_SOURCE_MASK,HY_ALL));
    for(unsigned n=1;n<=127;n++) {
        render(&a,1);
        for(unsigned i=0;i<3;i++)assert(a.voice[0].hybrid.gain[i]==n);
    }
    render(&a,MC_RATE);
    assert(a.voice[0].hybrid.ended_mask&HY_PCM);
    uint32_t pos=a.voice[0].pcm_pos;
    assert(!mc_parameter_set(&a,0,MC_PARAM_SOURCE_MASK,0));render(&a,128);
    assert(!mc_parameter_set(&a,0,MC_PARAM_SOURCE_MASK,HY_ALL));render(&a,128);
    assert(a.voice[0].pcm_pos==pos&&a.voice[0].hybrid.ended_mask&HY_PCM);
    assert(audible(whole,128)&&active(&a)==1);
    setup(&a,MC_PRESET_PULSE_DRIVE,HY_VA);
    mc_note(&a,0,60,127,0,0);render(&a,4096);
    assert(a.voice[0].hybrid.ended_mask&HY_FM);
    assert(!a.voice[0].phase);
    assert(!mc_parameter_set(&a,0,MC_PARAM_SOURCE_MASK,HY_FM|HY_VA));render(&a,512);
    assert(!a.voice[0].phase&&a.voice[0].hybrid.ended_mask&HY_FM);
    setup(&a,MC_PRESET_SOFT_MALLET,HY_FM);
    mc_note(&a,0,60,127,0,0);render(&a,4096);
    assert(!a.voice[0].hybrid.fm_env&&!(a.voice[0].hybrid.ended_mask&HY_FM));
    uint32_t phase=a.voice[0].phase;render(&a,128);
    assert(a.voice[0].phase!=phase&&audible(whole,128));
}
static void gates_and_steals(void) {
    setup(&a,MC_PRESET_STAGE_PIANO,HY_ALL);
    for(unsigned i=0;i<MC_VOICES;i++) {
        mc_note(&a,i%3,48+i,127,0,0);render(&a,1);
        assert(active(&a)==i+1);
    }
    mc_note(&a,0,80,127,0,0);
    assert(a.steals==1&&active(&a)==8&&a.voice[0].note==80);
    assert(!a.voice[0].age&&!a.voice[0].pcm_pos&&!a.voice[0].hybrid.ended_mask);
    assert(!a.pcm_cache[0].count&&!a.pitch_cache[0].increment);
    before=a;mc_note(&a,0,80,0,0,0);assert(!memcmp(&a,&before,sizeof a));
    mc_off(&a,0,80);assert(a.voice[0].stage==2);render(&a,512);
    assert(!a.voice[0].active);
    const unsigned gates[]={1,16,17};
    for(unsigned g=0;g<3;g++) {
        setup(&a,MC_PRESET_STAGE_PIANO,HY_ALL);
        mc_note(&a,0,60,127,0,gates[g]);
        if(gates[g]>1)render(&a,gates[g]-1);
        assert(a.voice[0].gate_left==1&&a.voice[0].stage!=2);
        render(&a,1);assert(!a.voice[0].gate_left&&a.voice[0].stage==2);
        render(&a,512);assert(!active(&a));
    }
}
static void parameters_and_snapshot(void) {
    setup(&a,MC_PRESET_STAGE_PIANO,HY_ALL);
    mc_note(&a,0,60,127,0,0);render(&a,64);
    before=a;
    const unsigned bad[][2]={{MC_PARAM_PROFILE,9},{MC_PARAM_SOURCE_MASK,8},{MC_PARAM_FM_MIX,128},{MC_PARAM_VA_MIX,128},{3,128},{1,3},{0,2}};
    for(unsigned i=0;i<sizeof bad/sizeof *bad;i++) {
        assert(mc_parameter_set(&a,0,bad[i][0],bad[i][1])==-1);
        assert(!memcmp(&a,&before,sizeof a));
    }
    const unsigned notes[][3]={{3,60,127},{0,11,127},{0,109,127},{0,60,128},{UINT_MAX,60,127}};
    for(unsigned i=0;i<sizeof notes/sizeof *notes;i++) {
        mc_note(&a,notes[i][0],notes[i][1],notes[i][2],0,0);
        assert(!memcmp(&a,&before,sizeof a));
    }
    assert(mc_parameter_get(&a,0,MC_PARAM_PROFILE)==MC_PRESET_STAGE_PIANO);
    assert(!mc_parameter_set(&a,0,6,90)&&a.voice[0].patch.attack==0);
    assert(!mc_parameter_set(&a,0,4,100)&&a.voice[0].patch.cutoff==100);
    assert(!mc_parameter_set(&a,0,7,30)&&a.voice[0].decay_step==1+32767/(880+30*30*6));
    mc_preset(&a,0,MC_PRESET_GLASS_PAD);
    McVoice tail=a.voice[0];
    assert(!mc_parameter_set(&a,0,MC_PARAM_FM_MIX,12));
    assert(!mc_parameter_set(&a,0,4,19));
    assert(!mc_parameter_set(&a,0,5,99));
    assert(!mc_parameter_set(&a,0,7,11));
    assert(!memcmp(&tail,&a.voice[0],sizeof tail));
    mc_preset(&a,0,MC_PRESET_DEEP_SUB);
    assert(mc_parameter_set(&a,0,3,1)==-1);
    assert(mc_parameter_set(&a,0,MC_PARAM_FM_MIX,1)==-1);
    assert(mc_parameter_set(&a,0,MC_PARAM_SOURCE_MASK,HY_ALL)==-1);
    mc_key(&a,0,72,100,1);
    assert(!mc_parameter_set(&a,0,MC_PARAM_MODE,MC_MONO));
    for(unsigned i=0;i<MC_VOICES;i++)if(a.voice[i].part==0)assert(a.voice[i].stage==2);
    for(unsigned i=0;i<16;i++)assert(!a.live_keys[0][i]);
}
static void pitch(void) {
    setup(&a,MC_PRESET_GLASS_PAD,HY_ALL);
    a.project.patch[0].mode=MC_MONO;a.project.patch[0].glide=64;
    mc_note(&a,0,48,127,0,0);render(&a,128);
    McVoice initial=a.voice[0];
    mc_note(&a,0,72,100,MC_SLIDE,0);
    assert(a.voice[0].phase==initial.phase&&a.voice[0].phase2==initial.phase2);
    assert(a.voice[0].pcm_pos==initial.pcm_pos&&a.voice[0].envelope==initial.envelope);
    assert(!memcmp(&initial.hybrid,&a.voice[0].hybrid,sizeof initial.hybrid));
    const int bends[]={-8192,-1,0,1,8191};
    for(unsigned i=0;i<1024;i++) {
        a.midi.bend[0]=(int16_t)bends[i%5];
        McVoice old=a.voice[0];
        render(&a,1);
        uint32_t advance=a.pitch_cache[0].increment;
        assert(a.voice[0].phase-old.phase==advance&&a.voice[0].phase2-old.phase2==advance);
        assert(a.voice[0].hybrid.modulator-old.hybrid.modulator==advance*hy_profile(old.patch.profile)->fm_ratio);
        const McPcmZone *z=hy_pcm_zone(old.hybrid.pcm_zone);
        assert(a.pitch_cache[0].pcm_step==(uint32_t)((uint64_t)advance*MC_PCM_RATE/z->tuning_hz_q16));
        assert(a.voice[0].increment>=old.increment&&a.voice[0].increment<=a.voice[0].target);
    }
}
static void compare_partition(unsigned frames,unsigned fixed) {
    const unsigned sizes[]={1,16,128,17,3,127,255,2};
    mc_render(&a,whole,frames);
    for(unsigned offset=0,chunk=0;offset<frames;chunk++) {
        unsigned n=fixed?fixed:sizes[chunk%8];if(n>frames-offset)n=frames-offset;
        mc_render(&b,split+offset*2,n);offset+=n;
    }
    assert(!memcmp(whole,split,frames*2*sizeof *whole));
    assert(!memcmp(&a,&b,sizeof a));
}
static void partition(void) {
    const unsigned sizes[]={1,16,128,0};
    for(unsigned n=0;n<4;n++) {
        setup(&a,MC_PRESET_STAGE_PIANO,HY_ALL);
        for(unsigned i=0;i<8;i++)mc_note(&a,i%3,48+i*3,100,0,i<3?17:0);
        b=a;before=a;mc_render(&a,whole,0);assert(!memcmp(&a,&before,sizeof a));
        compare_partition(1024,sizes[n]);
        assert(!mc_parameter_set(&a,0,MC_PARAM_FM_MIX,31));
        assert(!mc_parameter_set(&b,0,MC_PARAM_FM_MIX,31));
        a.midi.bend[2]=b.midi.bend[2]=4096;compare_partition(1024,sizes[n]);
        mc_off(&a,2,63);mc_off(&b,2,63);compare_partition(1024,sizes[n]);
    }
}
static void source_math(void) {
    /* 최종 필터/EG 밖의 실제 kernel을 정수 오라클과 대조한다. */
    for(unsigned mask=1;mask<=HY_ALL;mask++) {
        McVoice v={0};McPcmCache cache={0};
        v.patch=mc_presets[MC_PRESET_STAGE_PIANO];
        v.patch.source_mask=(uint8_t)mask;v.patch.wave=MC_WAVE_SINE;
        v.patch.pcm_mix=17;v.patch.fm_mix=63;v.patch.va_mix=111;v.note=60;v.velocity=127;
        hy_source_init(&v);v.hybrid.fm_index=0;
        v.phase=v.phase2=0x40000000u;v.pcm_pos=32u<<16;
        const McPcmMip *pcm=&hy_pcm_zone(v.hybrid.pcm_zone)->mip[0];
        McVoiceRender r={.profile=hy_profile(v.patch.profile),.pcm=pcm,.cache=&cache,
            .increment=mc_frequency[60],.pcm_step=100};
        int32_t sum=(mask&HY_PCM?pcm->data[32]*17:0)+(mask&HY_FM?mc_sine[256]*63:0)+
            (mask&HY_VA?mc_sine[256]*111:0);
        assert(hy_source_sample(&v,&r)==sum/381);
    }
    for(unsigned ratio=1;ratio<=8;ratio++) {
        McVoice v={0};v.patch=mc_presets[MC_PRESET_STAGE_PIANO];
        v.patch.source_mask=HY_FM;v.patch.fm_mix=127;v.velocity=127;hy_source_init(&v);
        McHybridProfile profile=*hy_profile(v.patch.profile);
        profile.fm_ratio=(uint8_t)ratio;profile.fm_feedback=127;v.hybrid.fm_index=127;
        McVoiceRender r={.profile=&profile,.increment=mc_frequency[108]};
        uint32_t original=v.hybrid.fm_env,step=v.hybrid.fm_step;
        for(unsigned i=0;i<1024;i++) {
            uint32_t mod=v.hybrid.modulator,phase=v.phase;
            int32_t out=hy_source_sample(&v,&r);v.age++;
            assert(out>=-10923&&out<=10923);
            assert(v.phase-phase==r.increment&&v.hybrid.modulator-mod==r.increment*ratio);
            assert(v.hybrid.previous_mod>=-32767&&v.hybrid.previous_mod<=32767);
            uint32_t decay=(i+1u)*step;
            assert(v.hybrid.fm_env==(decay<original?original-decay:0));
        }
    }
}
static void modern_body(void) {
    for(unsigned profile=MC_PRESET_STAGE_PIANO;profile<=MC_PRESET_SOFT_PIANO;profile++) {
        mc_init(&a);memset(a.project.fx_send,0,sizeof a.project.fx_send);
        mc_preset(&a,0,profile);
        assert(a.project.patch[0].wave==MC_WAVE_SAW);
        assert(a.project.patch[0].source_mask==(profile==MC_PRESET_STAGE_PIANO?HY_ALL:HY_PCM|HY_VA));
        mc_note(&a,0,60,127,0,0);render(&a,MC_RATE);
        assert(a.voice[0].hybrid.ended_mask&HY_PCM);
        assert(a.voice[0].active&&a.voice[0].phase2&&audible(whole,MC_RATE%4096));
        uint32_t phase=a.voice[0].phase2,pos=a.voice[0].pcm_pos;
        render(&a,128);
        assert(a.voice[0].pcm_pos==pos&&a.voice[0].phase2!=phase&&audible(whole,128));
        if(profile==MC_PRESET_SOFT_PIANO)assert(!a.voice[0].phase);
    }
}
int main(void) {
    sources();ramp_and_end();gates_and_steals();parameters_and_snapshot();pitch();partition();source_math();modern_body();
    puts("PASS hybrid: actual McEngine, seven masks, gain ramps/end latch, 8/9 steals, gates, shared pitch, snapshot and partition");
    return 0;
}
