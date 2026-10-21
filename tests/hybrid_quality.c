/* SPDX-License-Identifier: GPL-3.0-only */
#include "hybrid.h"
#include "tables.h"
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { Q_FRAMES=4*MC_RATE, Q_WINDOW=16384 };
static McEngine q;
static int16_t reference[Q_FRAMES],mixed[Q_FRAMES],isolated[Q_FRAMES];
static unsigned failures;
#define CHECK(c,id,n,label) do { if(!(c)) { \
    fprintf(stderr,"FAIL preset=%u note=%u check=%s line=%d\n",id,n,label,__LINE__);failures++; \
} } while(0)

static void setup(unsigned id,unsigned mask,int held) {
    mc_init(&q);
    memset(q.project.analog,0,sizeof q.project.analog);
    memset(q.project.arp,0,sizeof q.project.arp);
    memset(q.project.track_filter,0,sizeof q.project.track_filter);
    q.master=32768;
    mc_preset(&q,0,id);
    memset(q.project.fx_send,0,sizeof q.project.fx_send);
    q.project.patch[0].source_mask=(uint8_t)mask;
    if(held) {
        /* 피치/루프 관찰용 hold다. 음색 기본값 검사는 별도로 수행한다. */
        q.project.patch[0].attack=0;
        q.project.patch[0].sustain=127;
    }
}
static double energy(const int16_t *a,unsigned start,unsigned count) {
    double sum=0;
    for(unsigned i=start;i<start+count;i++)sum+=(double)a[i]*a[i];
    return sum/count;
}
static double harmonic(const int16_t *a,unsigned start,unsigned count,double hz) {
    double angle=6.2831853071795864769*hz/MC_RATE;
    double cs=cos(angle),sn=sin(angle),c=1,s=0,re=0,im=0;
    for(unsigned i=0;i<count;i++) {
        /* Hann은 분석 창에만 적용한다. 오디오/배음을 변경하지 않는다. */
        double w=0.5-0.5*cos(6.2831853071795864769*i/(count-1u));
        double x=a[start+i]*w;
        re+=x*c;im-=x*s;
        double next=c*cs-s*sn;s=s*cs+c*sn;c=next;
    }
    return 4.0*hypot(re,im)/count;
}
static double brightness(const int16_t *a,unsigned start,double hz) {
    double low=harmonic(a,start,Q_WINDOW,hz),high=0;
    for(unsigned h=2;h<=16&&h*hz<MC_RATE/2.0;h++) {
        double x=harmonic(a,start,Q_WINDOW,h*hz);high+=x*x;
    }
    return high/(low*low+high+1e-30);
}
static double modulation(const int16_t *a) {
    double lo=1e100,hi=0;
    for(unsigned i=2*MC_RATE;i+MC_RATE/10u<=Q_FRAMES;i+=MC_RATE/10u) {
        double rms=sqrt(energy(a,i,MC_RATE/10u));
        if(rms<lo)lo=rms;
        if(rms>hi)hi=rms;
    }
    return hi>0?(hi-lo)/(hi+lo):0;
}
static void capture(unsigned id,unsigned mask,unsigned note,int16_t *out,int held,int phase) {
    setup(id,mask,held);
    mc_note(&q,0,note,110,0,0);
    for(unsigned i=0;i<Q_FRAMES;) {
        unsigned n=phase?1u:128u;
        if(n>Q_FRAMES-i)n=Q_FRAMES-i;
        McVoice old=q.voice[0];
        int16_t block[256];
        mc_render(&q,block,n);
        for(unsigned j=0;j<n;j++)out[i+j]=block[j*2];
        if(phase) {
            const McVoice *v=&q.voice[0];
            uint32_t inc=mc_frequency[note];
            if(v->hybrid.gain[1]&&!(v->hybrid.ended_mask&HY_FM))
                CHECK(v->phase-old.phase==inc,id,note,"FM_shared_phase");
            if(v->hybrid.gain[2])CHECK(v->phase2-old.phase2==inc,id,note,"VA_shared_phase");
            CHECK(q.pitch_cache[0].increment==inc,id,note,"Analog0_fixed_pitch");
            if(v->hybrid.gain[0]&&!(old.hybrid.ended_mask&HY_PCM)) {
                const McPcmZone *z=hy_pcm_zone(v->hybrid.pcm_zone);
                uint32_t step=(uint32_t)((uint64_t)inc*MC_PCM_RATE/z->tuning_hz_q16);
                unsigned mip=0;
                while(mip+1u<z->levels&&step>65536u) { step=(step+1u)/2u;mip++; }
                CHECK(v->sample_step==step,id,note,"PCM_actual_tuning_step");
                if(old.sample_level==v->sample_level) {
                    const McPcmMip *m=&z->mip[mip];
                    uint64_t expected=(uint64_t)old.pcm_pos+step;
                    if(m->loop_end&&expected>=((uint64_t)m->loop_end<<16)) {
                        uint32_t start=(uint32_t)m->loop_start<<16;
                        uint32_t span=(uint32_t)(m->loop_end-m->loop_start)<<16;
                        expected=start+(expected-start)%span;
                    } else if(!m->loop_end&&expected>=((uint64_t)m->length<<16))
                        expected=(uint32_t)m->length<<16;
                    CHECK(v->pcm_pos==expected,id,note,"PCM_loop_phase_or_end");
                }
            }
        }
        i+=n;
    }
}
static void contribution(void) {
    for(unsigned id=0;id<32;id++) {
        unsigned mask=mc_presets[id].source_mask;
        capture(id,mask,60,mixed,0,0);
        CHECK(energy(mixed,0,MC_RATE)>0,id,60,"default_audible");
        for(unsigned bit=1;bit<=4;bit*=2)if(hy_profile(id)->available_mask&bit) {
            capture(id,bit,60,isolated,0,0);
            CHECK(energy(isolated,0,MC_RATE)>0,id,60,"source_contribution");
            if(mask!=bit) {
                unsigned comparison=mask&bit?mask^bit:mask|bit;
                capture(id,comparison,60,reference,0,0);
                uint64_t diff=0;
                for(unsigned i=0;i<MC_RATE;i++) {
                    int32_t d=(int32_t)mixed[i]-reference[i];
                    diff+=(uint64_t)((int64_t)d*d);
                }
                CHECK(diff>0,id,60,"source_mute_changes_output");
                printf("CONTRIBUTION preset=%u name=\"%s\" source=%u optional=%u delta_rms=%.6f\n",
                    id,mc_preset_names[id],bit,!(mask&bit),sqrt((double)diff/MC_RATE));
            }
        }
        if(id==8||id==9) {
            capture(id,HY_PCM,60,reference,0,0);
            capture(id,mask,60,mixed,0,0);
            CHECK(energy(reference,MC_RATE,MC_RATE)==0,id,60,"piano_PCM_ended");
            CHECK(energy(mixed,MC_RATE,MC_RATE)>0,id,60,"synth_strings_body_remains");
            CHECK(mc_presets[id].wave==MC_WAVE_SAW,id,60,"modern_saw_body");
        }
    }
}
static void loops(void) {
    for(unsigned zi=0;zi<MC_PCM_ZONES;zi++) {
        const McPcmZone *z=hy_pcm_zone(zi);
        for(unsigned mi=0;mi<z->levels;mi++) {
            const McPcmMip *m=&z->mip[mi];
            if(!m->loop_end)continue;
            uint64_t span=(uint64_t)(m->loop_end-m->loop_start)<<mi;
            uint64_t scale=MC_PCM_RATE*65536u,phase=(uint64_t)z->tuning_hz_q16*span;
            uint64_t cycles=(phase+scale/2u)/scale;
            uint64_t error=phase>cycles*scale?phase-cycles*scale:cycles*scale-phase;
            int jump=m->data[m->loop_start]-m->data[m->loop_end-1],max_delta=0;
            for(unsigned i=m->loop_start+1u;i<m->loop_end;i++) {
                int d=m->data[i]-m->data[i-1];if(d<0)d=-d;
                if(d>max_delta)max_delta=d;
            }
            /* 실제 Q16 tuning 반올림 오차와 기존 데이터의 인접 기울기 한계다. */
            printf("LOOP zone=%u mip=%u root=%u tuning_hz=%.9f cycles=%" PRIu64
                " phase_error_cycles=%.12f seam=%d adjacent_max=%d\n",
                zi,mi,z->root,z->tuning_hz_q16/65536.0,cycles,(double)error/(double)scale,jump,max_delta);
            for(unsigned id=0;id<MC_PRESETS;id++)if(hy_profile(id)->pcm_family==z->family) {
                if(!cycles||error>span/2u+1u||abs(jump)>max_delta)
                    fprintf(stderr,"LOOP_EVIDENCE preset=%u name=\"%s\" optional=%u zone=%u mip=%u "
                        "seam=%d local_max=%d phase_error_cycles=%.12f\n",id,mc_preset_names[id],
                        !(mc_presets[id].source_mask&HY_PCM),zi,mi,jump,max_delta,(double)error/(double)scale);
                CHECK(cycles&&error<=span/2u+1u,id,z->root,"loop_integer_cycles");
                CHECK(abs(jump)<=max_delta,id,z->root,"seam_within_source_slope");
            }
        }
    }
}
static void sustained(void) {
    const unsigned notes[]={48,60,84};
    for(unsigned id=0;id<MC_PRESETS;id++)for(unsigned n=0;n<3;n++) {
        unsigned note=notes[n],mask=mc_presets[id].source_mask;
        capture(id,mask,note,mixed,1,1);
        double hz=(double)mc_frequency[note]*MC_RATE/4294967296.0;
        double full_mod=modulation(mixed);
        for(unsigned bit=1;bit<=4;bit*=2)if(mask&bit) {
            capture(id,bit,note,isolated,1,0);
            printf("SUSTAIN preset=%u name=\"%s\" note=%u source=%u expected_hz=%.9f "
                "mix_AM_depth_100ms=%.9f source_AM_depth_100ms=%.9f "
                "mix_attack_higher_fraction=%.9f source_attack_higher_fraction=%.9f "
                "mix_sustain_higher_fraction=%.9f source_sustain_higher_fraction=%.9f\n",
                id,mc_preset_names[id],note,bit,hz,full_mod,modulation(isolated),
                brightness(mixed,MC_RATE/16u,hz),brightness(isolated,MC_RATE/16u,hz),
                brightness(mixed,2*MC_RATE,hz),brightness(isolated,2*MC_RATE,hz));
        }
    }
}
static void harmonic_reference(void) {
    /* raw 단독 VA와 같은 단독 VA의 제품 경로를 비교한다. 파형 정규화는 하지 않는다.
     * 완전 소실만 회귀 실패다. 감쇠량/음악적 허용치는 수치 출력 후 미승인으로 남긴다. */
    for(unsigned id=8;id<=9;id++) {
        capture(id,HY_VA,60,isolated,1,0);
        McVoice v={0};McPcmCache cache={0};
        v.patch=mc_presets[id];v.patch.source_mask=HY_VA;v.note=60;v.velocity=110;
        hy_source_init(&v);
        unsigned band=0;uint32_t inc=mc_frequency[60];
        while(band<6&&inc>mc_va_max_increment[band])band++;
        McVoiceRender r={.profile=hy_profile(id),.cache=&cache,.increment=inc,
            .table=mc_saw[band],.table_colour=mc_saw_colour[band],
            .va_morph_frames=(uint32_t)hy_profile(id)->va_morph_ms*MC_RATE/1000u};
        for(unsigned i=0;i<Q_FRAMES;i++) { reference[i]=(int16_t)hy_source_sample(&v,&r);v.age++; }
        double hz=(double)inc*MC_RATE/4294967296.0;
        double raw_fund=harmonic(reference,2*MC_RATE,Q_WINDOW,hz);
        double wet_fund=harmonic(isolated,2*MC_RATE,Q_WINDOW,hz);
        for(unsigned h=1;h<=12;h++) {
            double raw=harmonic(reference,2*MC_RATE,Q_WINDOW,hz*h);
            double wet=harmonic(isolated,2*MC_RATE,Q_WINDOW,hz*h);
            printf("HARMONIC preset=%u note=60 h=%u raw_source_amplitude=%.9f "
                "product_source_only_amplitude=%.9f ratio=%.9f raw_relative=%.9f product_relative=%.9f\n",
                id,h,raw,wet,raw>0?wet/raw:0,raw/raw_fund,wet/wet_fund);
            if(h>=2&&raw>1)CHECK(wet>1,id,60,"harmonic_above_PCM16_quantization_floor");
        }
    }
}
static void transitions(void) {
    setup(27,HY_ALL,1);
    q.project.patch[0].mode=MC_MONO;q.project.patch[0].glide=64;
    mc_note(&q,0,48,110,0,0);
    int16_t out[2];
    mc_render(&q,out,1);
    McVoice initial=q.voice[0];
    mc_note(&q,0,72,110,MC_SLIDE,0);
    CHECK(q.voice[0].phase==initial.phase&&q.voice[0].phase2==initial.phase2&&
        q.voice[0].pcm_pos==initial.pcm_pos,27,72,"legato_phase_continuity");
    const int16_t bends[3]={-8192,0,8191};
    for(unsigned b=0;b<3;b++) {
        q.midi.bend[0]=bends[b];
        for(unsigned i=0;i<4096;i++) {
            McVoice old=q.voice[0];mc_render(&q,out,1);
            uint32_t advance=q.pitch_cache[0].increment;
            CHECK(q.voice[0].phase-old.phase==advance&&q.voice[0].phase2-old.phase2==advance,
                27,72,"bend_glide_shared_phase");
            const McPcmZone *z=hy_pcm_zone(q.voice[0].hybrid.pcm_zone);
            CHECK(q.pitch_cache[0].pcm_step==(uint32_t)((uint64_t)advance*MC_PCM_RATE/z->tuning_hz_q16),
                27,72,"bend_glide_actual_PCM_tuning");
        }
    }
    puts("TRANSITIONS: bend -8192/0/8191, mono glide, FM/VA shared phase and PCM actual tuning");
}
static void reviewed_roles(void) {
    const unsigned notes[]={36,43,48};
    const double floor_ratio[8]={0.0008,0.18,0.025,0.004,0.025,0.025,0.07,0.08};
    const double floor_rms[8]={400,60,150,175,275,240,115,160};
    for(unsigned id=0;id<8;id++)for(unsigned ni=0;ni<3;ni++) {
        unsigned note=notes[ni];
        capture(id,mc_presets[id].source_mask,note,mixed,0,0);
        double hz=(double)mc_frequency[note]*MC_RATE/4294967296.0;
        for(unsigned window=0;window<2;window++) {
            unsigned start=MC_RATE*(window?600u:150u)/1000u;
            double low=0,mid=0;
            for(unsigned h=1;h*hz<=1600;h++) {
                double a=harmonic(mixed,start,Q_WINDOW,h*hz);
                if(h*hz<200)low+=a*a;else mid+=a*a;
            }
            double rms=sqrt(energy(mixed,start,Q_WINDOW));
            double fundamental=harmonic(mixed,start,Q_WINDOW,hz);
            printf("ROLE_BASS id=%u note=%u start_ms=%u rms=%.6f mid_low=%.9f mid=%.6f fundamental=%.6f\n",
                id,note,window?600:150,rms,mid/(low+1e-30),sqrt(mid),fundamental);
            CHECK(mid/(low+1e-30)>floor_ratio[id],id,note,"sustained_midrange_not_empty");
            CHECK(rms>floor_rms[id]&&fundamental>60,id,note,"bass_body_and_fundamental");
            if(id==5)CHECK(brightness(mixed,start,hz)>0.15,id,note,"finger_sustained_harmonic_body");
        }
    }
    for(unsigned id=16;id<=17;id++) {
        capture(id,mc_presets[id].source_mask,60,mixed,0,0);
        double early=sqrt(energy(mixed,0,MC_RATE/200));
        double body=sqrt(energy(mixed,MC_RATE/5,MC_RATE/10));
        printf("ROLE_START id=%u early5=%.6f body200=%.6f late1000=%.6f\n",
            id,early,body,sqrt(energy(mixed,MC_RATE,MC_RATE/10)));
        CHECK(early<body*0.2&&body>120,id,60,"smooth_start_strong_body");
    }
    const unsigned pads[]={24,26,27,30};
    for(unsigned p=0;p<4;p++) {
        unsigned id=pads[p];
        setup(id,mc_presets[id].source_mask,0);
        mc_note(&q,0,48,100,0,0);mc_note(&q,0,52,100,0,0);mc_note(&q,0,55,100,0,0);
        for(unsigned i=0;i<Q_FRAMES;i++) {
            int16_t out[2];mc_render(&q,out,1);mixed[i]=out[0];
        }
        double early=sqrt(energy(mixed,0,MC_RATE/200));
        double body=sqrt(energy(mixed,MC_RATE,MC_RATE/5));
        double late=sqrt(energy(mixed,3*MC_RATE,MC_RATE/5));
        double colour1=brightness(mixed,MC_RATE,(double)mc_frequency[48]*MC_RATE/4294967296.0);
        double colour3=brightness(mixed,3*MC_RATE,(double)mc_frequency[48]*MC_RATE/4294967296.0);
        mc_off(&q,0,48);mc_off(&q,0,52);mc_off(&q,0,55);
        for(unsigned i=0;i<MC_RATE;i++) {
            int16_t out[2];mc_render(&q,out,1);isolated[i]=out[0];
        }
        double release=sqrt(energy(isolated,0,MC_RATE/20));
        double end=sqrt(energy(isolated,MC_RATE*9/10,MC_RATE/10));
        printf("ROLE_DRY_CHORD id=%u early5=%.6f body1000=%.6f late3000=%.6f release50=%.6f end900=%.6f colour1=%.6f colour3=%.6f\n",
            id,early,body,late,release,end,colour1,colour3);
        CHECK(early<body*0.1&&body>150&&late>150&&late>body*0.75,
            id,48,"dry_pad_smooth_onset_full_held_chord");
        CHECK(release>body*0.4&&end<1,id,48,"dry_pad_release_not_cut_or_reverb");
        CHECK(!(mc_presets[id].source_mask&HY_PCM),id,48,"no_default_bowed_or_vowel_PCM");
        if(id==30)CHECK(fabs(colour1-colour3)>0.005,id,48,"intrinsic_sweep_colour_motion");
    }
    capture(27,mc_presets[27].source_mask,60,mixed,0,0);
    double glass_hz=(double)mc_frequency[60]*MC_RATE/4294967296.0;
    CHECK(brightness(mixed,MC_RATE,glass_hz)<0.03,27,60,"clear_glass_not_metallic");
    capture(28,mc_presets[28].source_mask,60,mixed,0,0);
    double early=sqrt(energy(mixed,0,MC_RATE/1000));
    double pop=sqrt(energy(mixed,MC_RATE/100,MC_RATE/25));
    double tail=sqrt(energy(mixed,MC_RATE*3/10,MC_RATE/10));
    double hz=(double)mc_frequency[60]*MC_RATE/4294967296.0;
    double upper=0,lower=0;
    for(unsigned h=1;h<=16;h++) {
        double a=harmonic(mixed,MC_RATE/100,4096,h*hz);
        if(h>=6)upper+=a*a;else lower+=a*a;
    }
    printf("ROLE_POP id=28 early1=%.6f pop10=%.6f tail300=%.6f upper_lower=%.6f\n",
        early,pop,tail,upper/(lower+1e-30));
    CHECK(early<pop*0.2&&pop>75&&tail<1,28,60,"rounded_compact_pop");
    CHECK(upper/(lower+1e-30)<0.06,28,60,"pop_not_metal_ring");
    capture(28,mc_presets[28].source_mask,74,mixed,0,0);
    hz=(double)mc_frequency[74]*MC_RATE/4294967296.0;
    double fundamental=harmonic(mixed,MC_RATE/20,2048,hz);
    double third=harmonic(mixed,MC_RATE/20,2048,3*hz);
    double fifth=harmonic(mixed,MC_RATE/20,2048,5*hz);
    CHECK(fundamental>50&&third<fundamental*0.22&&fifth<fundamental*0.10,
        28,74,"foreground_pop_has_round_filtered_body");
}
int main(void) {
    setvbuf(stdout,NULL,_IONBF,0);
    contribution();loops();sustained();harmonic_reference();transitions();reviewed_roles();
    printf("MECHANICAL_%s failures=%u\n",failures?"FAIL":"PASS",failures);
    puts("QUALITY_UNRUN: 수치는 승인 기준이 아니다. AM/시간별 배음 비교는 추가 A/B 검토가 필요하다.");
    puts("UNRUN: 실제 청취, alias 전대역 판정, WASM 실행, 타깃, 장치");
    return failures?1:0;
}
