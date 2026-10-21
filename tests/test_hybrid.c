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
    for(unsigned p=0;p<3;p++) {
        mc_preset(e,p,profile);
        McPatch *v=&e->project.patch[p];
        v->source_mask=(uint8_t)mask;v->attack=0;v->sustain=127;v->release=0;
        const McHybridProfile *r=hy_profile(profile);
        v->pcm_mix=(uint8_t)(r->available_mask&HY_PCM?127:0);
        v->fm_mix=(uint8_t)(r->available_mask&HY_FM?127:0);
        v->va_mix=(uint8_t)(r->available_mask&HY_VA?127:0);
    }
    memset(e->project.fx_send,0,sizeof e->project.fx_send);
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
    assert(a.voice[0].hybrid.fm_env==held.hybrid.fm_env);
    assert(a.voice[0].hybrid.fm_left==held.hybrid.fm_left);
    assert(a.voice[0].hybrid.fm_error==held.hybrid.fm_error);
    assert(a.voice[0].hybrid.previous_mod==held.hybrid.previous_mod);
    assert(!mc_parameter_set(&a,0,MC_PARAM_SOURCE_MASK,HY_ALL));
    for(unsigned n=1;n<=127;n++) {
        render(&a,1);
        for(unsigned i=0;i<3;i++)assert(a.voice[0].hybrid.gain[i]==n);
    }
    assert(a.voice[0].hybrid.fm_left==held.hybrid.fm_left-127u);
    render(&a,MC_RATE);
    assert(a.voice[0].hybrid.ended_mask&HY_PCM);
    uint32_t pos=a.voice[0].pcm_pos;
    assert(!mc_parameter_set(&a,0,MC_PARAM_SOURCE_MASK,0));render(&a,128);
    assert(!mc_parameter_set(&a,0,MC_PARAM_SOURCE_MASK,HY_ALL));render(&a,128);
    assert(a.voice[0].pcm_pos==pos&&a.voice[0].hybrid.ended_mask&HY_PCM);
    assert(audible(whole,128)&&active(&a)==1);
    setup(&a,MC_PRESET_PULSE_ARP,HY_VA);
    mc_note(&a,0,60,127,0,0);render(&a,4096);
    assert(a.voice[0].hybrid.ended_mask&HY_FM);
    assert(!a.voice[0].phase);
    assert(!mc_parameter_set(&a,0,MC_PARAM_SOURCE_MASK,HY_FM|HY_VA));render(&a,512);
    assert(!a.voice[0].phase&&a.voice[0].hybrid.ended_mask&HY_FM);
    setup(&a,MC_PRESET_SOFT_MALLET,HY_FM);
    mc_note(&a,0,60,127,0,0);
    render(&a,(uint32_t)hy_profile(MC_PRESET_SOFT_MALLET)->fm_decay_ms*MC_RATE/1000u);
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
    McVoice fresh=a.voice[0];hy_source_init(&fresh);
    assert(!memcmp(&fresh.hybrid,&a.voice[0].hybrid,sizeof fresh.hybrid));
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
    mc_preset(&a,0,MC_PRESET_REED_PULSE);
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
static void piano_attack_first(void) {
    const unsigned notes[]={48,60,72,84},velocities[]={32,80,127};
    for(unsigned id=MC_PRESET_STAGE_PIANO;id<=MC_PRESET_CLEAR_EP;id++)
        for(unsigned n=0;n<4;n++)for(unsigned velocity=0;velocity<3;velocity++)
            for(unsigned mask=3;mask<=HY_ALL;mask+=2) {
                if(mask&~hy_profile(id)->available_mask)continue;
                mc_init(&a);mc_preset(&a,0,id);
                memset(a.project.fx_send,0,sizeof a.project.fx_send);
                memset(a.project.analog,0,sizeof a.project.analog);
                a.project.patch[0].source_mask=(uint8_t)mask;
                /* Include Warm Tines' editable PCM mixture, muted by default. */
                b=a;b.project.patch[0].source_mask=HY_PCM;
                mc_note(&a,0,notes[n],velocities[velocity],0,0);
                mc_note(&b,0,notes[n],velocities[velocity],0,0);
                mc_render(&a,whole,512);mc_render(&b,split,512);
                assert(audible(split,128));
                assert(!memcmp(whole,split,128*2*sizeof *whole));
                assert(memcmp(whole+256,split+256,384*2*sizeof *whole));
                uint32_t phase=mc_frequency[notes[n]]*512u;
                if(mask&HY_FM)assert(a.voice[0].phase==phase);
                if(mask&HY_VA)assert(a.voice[0].phase2==phase);
                /* A zero PCM mix must remain identical to source-mask editing. */
                mc_init(&a);mc_preset(&a,0,id);
                memset(a.project.fx_send,0,sizeof a.project.fx_send);
                memset(a.project.analog,0,sizeof a.project.analog);
                a.project.patch[0].source_mask=(uint8_t)mask;
                a.project.patch[0].pcm_mix=0;
                b=a;b.project.patch[0].source_mask=(uint8_t)(mask&~HY_PCM);
                mc_note(&a,0,notes[n],velocities[velocity],0,0);
                mc_note(&b,0,notes[n],velocities[velocity],0,0);
                mc_render(&a,whole,512);mc_render(&b,split,512);
                assert(audible(whole,128));
                assert(!memcmp(whole,split,512*2*sizeof *whole));
            }
    puts("PASS hybrid piano: audible PCM leads FM/VA, body follows with continuous phase; PCM-muted edits unchanged");
}
static void source_math(void) {
    /* 최종 필터/EG 밖의 실제 kernel을 정수 오라클과 대조한다. */
    for(unsigned mask=1;mask<=HY_ALL;mask++) {
        McVoice v={0};McPcmCache cache={0};
        v.patch=mc_presets[MC_PRESET_STAGE_PIANO];
        v.patch.source_mask=(uint8_t)mask;v.patch.wave=MC_WAVE_SINE;
        v.patch.pcm_mix=17;v.patch.fm_mix=63;v.patch.va_mix=111;v.note=60;v.velocity=127;
        hy_source_init(&v);v.hybrid.fm_index=0;
        v.phase=v.phase2=0x40000000u;v.pcm_pos=32u<<16;v.age=256;
        const McPcmMip *pcm=&hy_pcm_zone(v.hybrid.pcm_zone)->mip[0];
        McVoiceRender r={.profile=hy_profile(v.patch.profile),.pcm=pcm,.cache=&cache,
            .increment=mc_frequency[60],.pcm_step=100,
            .fm_decay_frames=(uint32_t)hy_profile(v.patch.profile)->fm_decay_ms*MC_RATE/1000u};
        int32_t sum=(mask&HY_PCM?pcm->data[32]*17:0)+(mask&HY_FM?mc_sine[256]*63:0)+
            (mask&HY_VA?mc_sine[256]*111:0);
        assert(hy_source_sample(&v,&r)==sum/381);
    }
    for(unsigned ratio=1;ratio<=8;ratio++) {
        McVoice v={0};v.patch=mc_presets[MC_PRESET_STAGE_PIANO];
        v.patch.source_mask=HY_FM;v.patch.fm_mix=127;v.velocity=127;hy_source_init(&v);
        McHybridProfile profile=*hy_profile(v.patch.profile);
        profile.fm_ratio=(uint8_t)ratio;profile.fm_feedback=24;v.hybrid.fm_index=127;
        McVoiceRender r={.profile=&profile,.increment=mc_frequency[108],
            .fm_decay_frames=(uint32_t)profile.fm_decay_ms*MC_RATE/1000u};
        uint32_t depth=UINT32_C(0x80000000)-((uint32_t)profile.fm_body_q15<<16);
        for(unsigned i=0;i<1024;i++) {
            uint32_t mod=v.hybrid.modulator,phase=v.phase;
            int32_t out=hy_source_sample(&v,&r);v.age++;
            assert(out>=-10923&&out<=10923);
            assert(v.phase-phase==r.increment&&v.hybrid.modulator-mod==r.increment*ratio);
            assert(v.hybrid.previous_mod>=-32767&&v.hybrid.previous_mod<=32767);
            uint32_t decay=(uint32_t)((uint64_t)(i+1u)*depth/r.fm_decay_frames);
            assert(v.hybrid.fm_env==UINT32_C(0x80000000)-decay);
        }
    }
}
static void modern_body(void) {
    for(unsigned profile=MC_PRESET_STAGE_PIANO;profile<=MC_PRESET_SOFT_PIANO;profile++) {
        mc_init(&a);
        mc_preset(&a,0,profile);
        memset(a.project.fx_send,0,sizeof a.project.fx_send);
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
static int32_t sine_oracle(uint32_t phase) {
    unsigned n=phase>>22;
    return mc_sine[n]+(int32_t)((int64_t)(mc_sine[(n+1u)%1024u]-mc_sine[n])*
        ((phase>>6)&65535u)/65536);
}
static int32_t table_oracle(const int16_t *table,uint32_t phase) {
    unsigned n=phase>>24;
    return table[n]+(int32_t)((int64_t)(table[(n+1u)%256u]-table[n])*
        ((phase>>16)&255u)/256);
}
static void trigger_oracles(void) {
    for(unsigned id=0;id<MC_PRESETS;id++)for(unsigned velocity=1;velocity<=127;velocity++) {
        McVoice v={0};
        memset(&v.hybrid,0xa5,sizeof v.hybrid);
        v.patch=mc_presets[id];v.note=60;v.velocity=(uint8_t)velocity;
        hy_source_init(&v);
        const McHybridProfile *p=hy_profile(id);
        uint32_t n=(uint32_t)p->fm_decay_ms*MC_RATE/1000u;
        uint32_t body=(uint32_t)p->fm_body_q15*65536u,depth=UINT32_C(0x80000000)-body;
        assert(v.hybrid.fm_env==(n?UINT32_C(0x80000000):body));
        assert(v.hybrid.fm_step==(n?depth/n:0)&&v.hybrid.fm_remainder==(n?depth%n:0));
        assert(v.hybrid.fm_left==n&&!v.hybrid.fm_error);
        unsigned index=p->fm_index+velocity*p->velocity_index/127u;
        assert(v.hybrid.fm_index==(index>127?127:index));
        uint32_t m=(uint32_t)p->va_morph_ms*MC_RATE/1000u;
        assert(v.hybrid.va_colour==(uint32_t)(m?p->va_colour_start_q15:p->va_colour_end_q15)*65536u);
        assert(v.hybrid.va_colour_step==(m?
            ((int64_t)p->va_colour_end_q15-p->va_colour_start_q15)*65536/m:0));
        McHybridState first=v.hybrid;
        memset(&v.hybrid,0x5a,sizeof v.hybrid);hy_source_init(&v);
        assert(!memcmp(&first,&v.hybrid,sizeof first));
    }
}
static void fm_oracles(void) {
    const unsigned milliseconds[]={0,1,82,175,420,620,1800,2200,10000};
    const unsigned floors[]={0,1024,4096,8192,32768};
    for(unsigned t=0;t<9;t++)for(unsigned f=0;f<5;f++) {
        McHybridProfile p=*hy_profile(MC_PRESET_STAGE_PIANO);
        p.fm_decay_ms=(uint16_t)milliseconds[t];p.fm_body_q15=(uint16_t)floors[f];
        p.fm_ratio=(uint8_t)(1u+t%8u);p.fm_feedback=24;p.fm_duration_ms=0;
        uint32_t n=(uint32_t)p.fm_decay_ms*MC_RATE/1000u;
        uint32_t body=(uint32_t)p.fm_body_q15*65536u,depth=UINT32_C(0x80000000)-body;
        McVoice v={0};v.patch=mc_presets[MC_PRESET_STAGE_PIANO];
        v.patch.source_mask=HY_FM;v.patch.fm_mix=127;v.velocity=127;hy_source_init(&v);
        v.hybrid.fm_frames=0;v.hybrid.fm_env=n?UINT32_C(0x80000000):body;
        v.hybrid.fm_left=n;v.hybrid.fm_step=n?depth/n:0;
        v.hybrid.fm_remainder=n?depth%n:0;v.hybrid.fm_error=0;
        McVoiceRender r={.profile=&p,.increment=mc_frequency[60],.fm_decay_frames=n};
        uint32_t phase=0,modulator=0;int32_t previous=0;
        for(uint32_t k=0;k<n+2u;k++) {
            uint32_t env=n&&k<n?UINT32_C(0x80000000)-(uint32_t)((uint64_t)k*depth/n):body;
            assert(v.hybrid.fm_env==env&&v.hybrid.fm_left==(k<n?n-k:0));
            assert(!n||v.hybrid.fm_error==(uint32_t)((uint64_t)(k<n?k:n)*depth%n));
            uint32_t feedback=(uint32_t)((int64_t)previous*p.fm_feedback*1024);
            previous=sine_oracle(modulator+feedback);
            int32_t mod=(int32_t)((int64_t)previous*env/INT64_C(2147483648));
            int32_t out=sine_oracle(phase+(uint32_t)((int64_t)mod*v.hybrid.fm_index*2048));
            assert(hy_source_sample(&v,&r)==out*127/381);
            phase+=r.increment;modulator+=r.increment*p.fm_ratio;v.age++;
            assert(v.phase==phase&&v.hybrid.modulator==modulator&&v.hybrid.previous_mod==previous);
            assert(v.hybrid.fm_env>=body&&v.hybrid.fm_env<=UINT32_C(0x80000000));
        }
    }
    /* All velocities at representative integer ratios, including negative products. */
    for(unsigned ratio=1;ratio<=8;ratio+=3)for(unsigned velocity=1;velocity<=127;velocity++) {
        McVoice v={0};v.patch=mc_presets[MC_PRESET_STAGE_PIANO];v.patch.source_mask=HY_FM;
        v.patch.fm_mix=127;v.velocity=(uint8_t)velocity;hy_source_init(&v);
        McHybridProfile p=*hy_profile(v.patch.profile);p.fm_ratio=(uint8_t)ratio;
        McVoiceRender r={.profile=&p,.increment=mc_frequency[84],
            .fm_decay_frames=(uint32_t)p.fm_decay_ms*MC_RATE/1000u};
        for(unsigned k=0;k<1024;k++) {
            int32_t previous=sine_oracle(v.hybrid.modulator+
                (uint32_t)((int64_t)v.hybrid.previous_mod*p.fm_feedback*1024));
            int32_t mod=(int32_t)((int64_t)previous*v.hybrid.fm_env/INT64_C(2147483648));
            int32_t expected=sine_oracle(v.phase+(uint32_t)((int64_t)mod*v.hybrid.fm_index*2048));
            assert(hy_source_sample(&v,&r)==expected*127/381);v.age++;
        }
    }
    puts("PASS hybrid FM: independent signed sample/closed-form decay oracle, 9 times x 5 floors, all velocities");
}
static void finite_fm_oracle(void) {
    McHybridProfile p=*hy_profile(MC_PRESET_STAGE_PIANO);
    p.fm_feedback=0;
    McVoice v={0};v.patch=mc_presets[MC_PRESET_STAGE_PIANO];v.patch.source_mask=HY_FM;
    v.patch.fm_mix=127;hy_source_init(&v);v.hybrid.fm_index=0;v.hybrid.fm_frames=256;
    McVoiceRender r={.profile=&p,.increment=mc_frequency[60],
        .fm_decay_frames=(uint32_t)p.fm_decay_ms*MC_RATE/1000u};
    for(unsigned age=0;age<=256;age++) {
        v.age=age;uint32_t phase=v.phase;
        int32_t expected=sine_oracle(phase);
        if(age>128&&age<256)expected=expected*(int32_t)(256-age)/128;
        if(age==256)expected=0;
        assert(hy_source_sample(&v,&r)==expected*127/381);
        assert(v.phase==phase+(age<256?r.increment:0));
    }
    McHybridState ended=v.hybrid;uint32_t phase=v.phase;
    v.age=UINT32_MAX;hy_source_sample(&v,&r);v.age=0;hy_source_sample(&v,&r);
    assert(v.phase==phase&&v.hybrid.ended_mask&HY_FM);
    assert(v.hybrid.fm_env==ended.fm_env&&v.hybrid.fm_left==ended.fm_left);
    puts("PASS hybrid FM: finite final 128-sample ramp and irreversible latch across completed age wrap");
}
static void colour_oracles(void) {
    const unsigned endpoints[][3]={{0,32768,80},{32768,0,10000},{4096,20480,2600},
        {16384,16384,0},{0,1,10000},{32768,32768,100}};
    for(unsigned wave=0;wave<2;wave++)for(unsigned test=0;test<6;test++) {
        McHybridProfile p=*hy_profile(MC_PRESET_STAGE_PIANO);
        p.va_colour_start_q15=(uint16_t)endpoints[test][0];
        p.va_colour_end_q15=(uint16_t)endpoints[test][1];p.va_morph_ms=(uint16_t)endpoints[test][2];
        uint32_t m=(uint32_t)p.va_morph_ms*MC_RATE/1000u;
        int64_t start=(int64_t)p.va_colour_start_q15*65536,end=(int64_t)p.va_colour_end_q15*65536;
        int64_t step=m?(end-start)/m:0;
        McVoice v={0};v.patch=mc_presets[MC_PRESET_STAGE_PIANO];v.patch.wave=(uint8_t)wave;
        v.patch.source_mask=HY_VA;v.patch.va_mix=127;v.velocity=127;hy_source_init(&v);
        v.hybrid.va_colour=(uint32_t)(m?start:end);v.hybrid.va_colour_step=(int32_t)step;
        McVoiceRender r={.profile=&p,.increment=mc_frequency[60],.va_morph_frames=m,
            .table=wave?mc_square[2]:mc_saw[2],.table_colour=wave?mc_square_colour[2]:mc_saw_colour[2]};
        for(uint32_t age=0;age<=m+1u;age++) {
            int64_t colour=age<m?start+(int64_t)age*step:end;
            if(age<m) {
                int64_t error=(colour-start)*(int64_t)m-(end-start)*age;
                assert(error>-(int64_t)m*age-1&&error<(int64_t)m*age+1);
            }
            v.age=age;
            /* The trajectory runs during mute, release and live sine editing. */
            v.patch.source_mask=(uint8_t)(age>=128&&age<300?0:HY_VA);
            v.patch.wave=(uint8_t)(age>=320&&age<400?MC_WAVE_SINE:wave);
            v.stage=(uint8_t)(age>m/2u?2:1);
            unsigned gain=v.hybrid.gain[2],target=v.patch.source_mask?127u:0u;
            gain= gain<target?gain+1u:gain>target?gain-1u:gain;
            uint32_t phase=v.phase2;
            int32_t classic=table_oracle(r.table,phase),alternate=table_oracle(r.table_colour,phase);
            int32_t expected=v.patch.wave==MC_WAVE_SINE?sine_oracle(phase):
                classic+(int32_t)((int64_t)(alternate-classic)*(colour/65536)/32768);
            assert(hy_source_sample(&v,&r)==expected*(int32_t)gain/381);
            assert(v.phase2==phase+(gain?r.increment:0));
            assert(v.hybrid.va_colour==(uint32_t)(age<m?colour+step:end));
            assert(v.hybrid.va_colour_step==(age<m?step:0));
        }
        v.age=UINT32_MAX;hy_source_sample(&v,&r);v.age=0;hy_source_sample(&v,&r);
        assert(v.hybrid.va_colour==(uint32_t)end&&!v.hybrid.va_colour_step);
    }
    puts("PASS hybrid colour: independent every-sample blend/trajectory, endpoints, descending/zero-step, mute/sine/release/wrap");
    for(unsigned wave=0;wave<3;wave++) {
        McVoice v={0};v.patch=mc_presets[MC_PRESET_STAGE_PIANO];v.patch.source_mask=HY_VA;
        v.patch.wave=(uint8_t)wave;v.patch.va_mix=127;hy_source_init(&v);
        McVoiceRender r={.profile=hy_profile(v.patch.profile),.increment=mc_va_max_increment[6]+1u};
        assert(!hy_source_sample(&v,&r)&&v.phase2==r.increment);
    }
}
static void preset_sends(void) {
    for(unsigned id=0;id<MC_PRESETS;id++)for(unsigned part=0;part<3;part++) {
        mc_init(&a);
        for(unsigned p=0;p<4;p++)for(unsigned bank=0;bank<4;bank++)
            a.project.fx_send[p][bank]=(uint8_t)(30+p*13+bank);
        for(unsigned bank=0;bank<4;bank++)a.project.fx_bus_type[bank]=(uint8_t)(7-bank);
        mc_note(&a,2,60,127,0,0);render(&a,2048);before=a;
        mc_preset(&a,part,id);
        assert(!memcmp(a.project.fx_send[part],mc_preset_fx_send[id],MC_FX_BANKS));
        for(unsigned p=0;p<4;p++)if(p!=part)
            assert(!memcmp(a.project.fx_send[p],before.project.fx_send[p],MC_FX_BANKS));
        assert(!memcmp(a.project.fx_bus_type,before.project.fx_bus_type,MC_FX_BANKS));
        assert(!memcmp(&a.shared_fx,&before.shared_fx,sizeof a.shared_fx));
        assert(!memcmp(&a.extra_fx,&before.extra_fx,sizeof a.extra_fx));
        McProject expected=before.project;
        expected.patch[part]=mc_presets[id];expected.preset[part]=(uint8_t)id;
        memcpy(expected.fx_send[part],mc_preset_fx_send[id],MC_FX_BANKS);
        assert(!memcmp(&a.project,&expected,sizeof expected));
    }
    puts("PASS hybrid preset: 32 x 3 sends only; global types, other sends and active shared histories preserved");
}
static void long_partitions(void) {
    const unsigned chunks[]={1,3,16,17,127,128,255,0};
    const unsigned ids[]={MC_PRESET_STAGE_PIANO,MC_PRESET_GLASS_PAD,MC_PRESET_SOFT_SWEEP};
    uint32_t decay=(uint32_t)hy_profile(ids[0])->fm_decay_ms*MC_RATE/1000u;
    uint32_t duration=(uint32_t)hy_profile(ids[0])->fm_duration_ms*MC_RATE/1000u;
    uint32_t body=(uint32_t)hy_profile(ids[1])->fm_decay_ms*MC_RATE/1000u;
    uint32_t events[]={0,127,255,decay-1u,decay,decay+1u,duration-1u,duration,duration+1u,
        MC_RATE,body-1u,body,body+1u,2u*MC_RATE,5u*MC_RATE-1u,
        5u*MC_RATE,5u*MC_RATE+1u,9u*MC_RATE,10u*MC_RATE+1u};
    for(unsigned i=1;i<sizeof events/sizeof *events;i++)for(unsigned j=i;j&&events[j]<events[j-1];j--) {
        uint32_t value=events[j];events[j]=events[j-1];events[j-1]=value;
    }
    for(unsigned run=0;run<8;run++) {
        mc_init(&a);
        for(unsigned part=0;part<3;part++) {
            mc_preset(&a,part,ids[part]);
            a.project.patch[part].attack=0;a.project.patch[part].sustain=127;
            a.project.patch[part].mode=MC_POLY;a.project.analog[part]=127;
        }
        memset(a.project.fx_send,0,sizeof a.project.fx_send);
        for(unsigned i=0;i<8;i++)mc_note(&a,i%3,48+i*3,100,0,0);
        b=a;
        for(unsigned event=1;event<sizeof events/sizeof *events;event++) {
            for(uint32_t age=events[event-1];age<events[event];) {
                unsigned n=events[event]-age;
                if(n>4096)n=4096;
                compare_partition(n,chunks[run]);age+=n;
            }
            if(events[event]==127||events[event]==255) {
                unsigned mask=events[event]==127?0:hy_profile(ids[1])->available_mask;
                assert(!mc_parameter_set(&a,1,MC_PARAM_SOURCE_MASK,mask));
                assert(!mc_parameter_set(&b,1,MC_PARAM_SOURCE_MASK,mask));
            }
            if(events[event]==MC_RATE||events[event]==2u*MC_RATE) {
                a.midi.bend[2]=b.midi.bend[2]=(int16_t)(events[event]==MC_RATE?8191:-8192);
                assert(!mc_parameter_set(&a,2,MC_PARAM_WAVE,events[event]==MC_RATE?MC_WAVE_SINE:MC_WAVE_SAW));
                assert(!mc_parameter_set(&b,2,MC_PARAM_WAVE,events[event]==MC_RATE?MC_WAVE_SINE:MC_WAVE_SAW));
            }
            if(events[event]==9u*MC_RATE) { mc_off(&a,1,51);mc_off(&b,1,51); }
        }
    }
    puts("PASS hybrid: 10-second PCM/full-state partitions 1/3/16/17/127/128/255/irregular, morph boundaries/masks/bend/release");
}
int main(void) {
    piano_attack_first();
    trigger_oracles();fm_oracles();finite_fm_oracle();colour_oracles();preset_sends();
    long_partitions();
    sources();ramp_and_end();gates_and_steals();parameters_and_snapshot();pitch();partition();source_math();modern_body();
    puts("PASS hybrid: actual McEngine, seven masks, gain ramps/end latch, 8/9 steals, gates, shared pitch, snapshot and partition");
    return 0;
}
