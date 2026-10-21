/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#include "hybrid.h"
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/* WAV 자체의 SHA256이다. 외부 프로그램과 플랫폼 전용 라이브러리에 의존하지 않는다. */
typedef struct { uint32_t h[8]; uint64_t bytes; unsigned used; uint8_t block[64]; } Sha;
static uint32_t rotr(uint32_t x,unsigned n) { return (x>>n)|(x<<(32u-n)); }
static void sha_block(Sha *s) {
    static const uint32_t k[64]={
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    uint32_t w[64],v[8];
    for(unsigned i=0;i<16;i++) {
        unsigned j=4*i;
        w[i]=((uint32_t)s->block[j]<<24)|((uint32_t)s->block[j+1]<<16)|
             ((uint32_t)s->block[j+2]<<8)|s->block[j+3];
    }
    for(unsigned i=16;i<64;i++) {
        uint32_t a=w[i-15],b=w[i-2];
        w[i]=w[i-16]+(rotr(a,7)^rotr(a,18)^(a>>3))+w[i-7]+(rotr(b,17)^rotr(b,19)^(b>>10));
    }
    memcpy(v,s->h,sizeof v);
    for(unsigned i=0;i<64;i++) {
        uint32_t t=v[7]+(rotr(v[4],6)^rotr(v[4],11)^rotr(v[4],25))+
            ((v[4]&v[5])^(~v[4]&v[6]))+k[i]+w[i];
        uint32_t u=(rotr(v[0],2)^rotr(v[0],13)^rotr(v[0],22))+
            ((v[0]&v[1])^(v[0]&v[2])^(v[1]&v[2]));
        for(unsigned j=7;j>0;j--)v[j]=v[j-1];
        v[4]+=t;v[0]=t+u;
    }
    for(unsigned i=0;i<8;i++)s->h[i]+=v[i];
}
static void sha_add(Sha *s,const uint8_t *p,size_t n) {
    s->bytes+=n;
    while(n--) {
        s->block[s->used++]=*p++;
        if(s->used==64) { sha_block(s);s->used=0; }
    }
}
static int sha_file(const char *path,char out[65]) {
    Sha s={{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19},0,0,{0}};
    FILE *f=fopen(path,"rb");
    if(!f)return -1;
    uint8_t buf[4096];size_t n;
    while((n=fread(buf,1,sizeof buf,f)))sha_add(&s,buf,n);
    int bad=ferror(f);
    if(fclose(f))bad=1;
    if(bad)return -1;
    uint64_t bits=s.bytes*8;
    uint8_t one=128,zero=0;
    sha_add(&s,&one,1);
    while(s.used!=56)sha_add(&s,&zero,1);
    for(unsigned i=0;i<8;i++)buf[i]=(uint8_t)(bits>>(56-8*i));
    sha_add(&s,buf,8);
    for(unsigned i=0;i<8;i++)snprintf(out+8*i,9,"%08" PRIx32,s.h[i]);
    return 0;
}
static void json_string(FILE *f,const char *s) {
    fputc('"',f);
    for(;*s;s++) {
        unsigned c=(unsigned char)*s;
        if(c=='"'||c=='\\')fprintf(f,"\\%c",(int)c);
        else if(c<32)fprintf(f,"\\u%04x",c);
        else fputc((int)c,f);
    }
    fputc('"',f);
}
static void le16(uint8_t *p,unsigned v) { p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8); }
static void le32(uint8_t *p,uint32_t v) {
    for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));
}
typedef struct {
    uint32_t frame,gate;
    unsigned part,note,velocity,flags,drum;
} AudEvent;
typedef struct {
    FILE *wav;
    uint32_t frame,frames;
    uint64_t squares,nonzero,rails,triple_frames,drum_six_frames,all_fx_frames,fx_active_frames,full_stress_frames;
    unsigned peak,peak_voices,peak_drums,end_voices,end_drums,tail_peak,drum_steals;
    AudEvent events[512];unsigned count;
} AudResult;
static McEngine aud_engine;
static unsigned active_voices(void) {
    unsigned n=0;
    for(unsigned i=0;i<MC_VOICES;i++)n+=aud_engine.voice[i].active!=0;
    return n;
}
static unsigned active_drums(void) {
    unsigned n=0;
    for(unsigned i=0;i<MC_DRUM_VOICES;i++)n+=aud_engine.drum[i].env!=0;
    return n;
}
static void aud_setup(unsigned preset,unsigned mask,int effects) {
    mc_init(&aud_engine);
    memset(aud_engine.project.analog,0,sizeof aud_engine.project.analog);
    memset(aud_engine.project.fx_send,0,sizeof aud_engine.project.fx_send);
    memset(aud_engine.project.arp,0,sizeof aud_engine.project.arp);
    memset(aud_engine.project.track_filter,0,sizeof aud_engine.project.track_filter);
    for(unsigned p=0;p<4;p++) {
        aud_engine.project.level[p]=96;aud_engine.project.pan[p]=64;
        if(p<3) {
            mc_preset(&aud_engine,p,preset);
            aud_engine.project.patch[p].source_mask=(uint8_t)mask;
        }
        if(effects)for(unsigned b=0;b<4;b++)aud_engine.project.fx_send[p][b]=24;
    }
    if(effects) {
        const uint8_t types[4]={3,1,6,4};
        memcpy(aud_engine.project.fx_bus_type,types,sizeof types);
    }
}
static int aud_until(AudResult *r,uint32_t end) {
    while(r->frame<end) {
        /* 실제 동시 소스/드럼 점유를 frame 단위로 관찰한다. 성능 측정용이 아니다. */
        int16_t out[2];mc_render(&aud_engine,out,1);
        unsigned voices=active_voices(),drums=active_drums(),triples=0,fx=0;
        for(unsigned i=0;i<MC_VOICES;i++) {
            const McVoice *v=&aud_engine.voice[i];
            if(v->active&&v->hybrid.gain[0]&&v->hybrid.gain[1]&&v->hybrid.gain[2]&&
               !(v->hybrid.ended_mask&(HY_PCM|HY_FM)))triples++;
        }
        for(unsigned b=0;b<4;b++) {
            unsigned sum=0;
            for(unsigned p=0;p<4;p++)sum+=aud_engine.project.fx_send[p][b];
            fx+=sum!=0;
        }
        r->triple_frames+=triples==8;r->drum_six_frames+=drums==6;r->all_fx_frames+=fx==4;
        unsigned active_fx=(unsigned)(aud_engine.shared_fx.rd_active!=0)+
            (unsigned)(aud_engine.shared_fx.cf_active!=0)+(unsigned)(aud_engine.extra_fx.rd_active!=0)+
            (unsigned)(aud_engine.extra_fx.cf_active!=0);
        r->fx_active_frames+=active_fx==4;
        r->full_stress_frames+=triples==8&&drums==6&&active_fx==4;
        if(voices>r->peak_voices)r->peak_voices=voices;
        if(drums>r->peak_drums)r->peak_drums=drums;
        uint8_t bytes[4];
        for(unsigned i=0;i<2;i++) {
            int32_t s=out[i];unsigned a=(unsigned)(s<0?-s:s);
            r->squares+=(uint64_t)((int64_t)s*s);r->nonzero+=s!=0;r->rails+=a>=32767;
            if(a>r->peak)r->peak=a;
            if(r->frame+MC_RATE/10u>=r->frames&&a>r->tail_peak)r->tail_peak=a;
            le16(bytes+2*i,(uint16_t)out[i]);
        }
        if(fwrite(bytes,1,4,r->wav)!=4)return -1;
        r->frame++;
    }
    return 0;
}
static int aud_event(AudResult *r,unsigned part,unsigned note,unsigned velocity,unsigned flags,uint32_t gate,int drum) {
    if(r->count>=512)return -1;
    r->events[r->count++]=(AudEvent){r->frame,gate,part,note,velocity,flags,(unsigned)drum};
    if(drum) {
        unsigned empty=0,matching=0;
        for(unsigned i=0;i<6;i++) {
            const McDrum *d=&aud_engine.drum[i];
            empty+=!d->env||(note==2&&d->sound==3);
            matching+=d->env&&d->sound==note;
        }
        r->drum_steals+=!empty&&!matching;
        mc_drum(&aud_engine,note,velocity);
    } else mc_note(&aud_engine,part,note,velocity,flags,gate);
    return 0;
}
static int aud_phrase(AudResult *r,unsigned id) {
    unsigned base=id<8?36:60;
    const unsigned velocities[3]={40,85,120};
    /* 세 velocity와 세 음역을 함께 노출하고, 뒤에 별도 역할 프레이즈를 둔다. */
    for(unsigned i=0;i<3;i++) {
        if(aud_event(r,0,base+12*i,velocities[i],0,MC_RATE,0)||
           aud_until(r,(i+1u)*2u*MC_RATE))return -1;
    }
    uint32_t start=r->frame;
    int chord=(id>=8&&id<12)||(id>=16&&id<20)||(id>=24&&id<28)||id==30;
    static const unsigned offsets[8]={0,0,7,10,12,7,3,7};
    for(unsigned step=0;step<8;step++) {
        if(aud_until(r,start+step*MC_RATE/2u))return -1;
        if(chord) {
            if(step%4)continue;
            unsigned root=60+(step?5u:0u);
            for(unsigned k=0;k<3;k++)
                if(aud_event(r,0,root+(k==1?3u:k==2?7u:0u),90,0,MC_RATE*3u/2u,0))return -1;
        } else {
            unsigned flags=aud_engine.project.patch[0].mode==MC_MONO&&step%4?MC_SLIDE:0;
            if(aud_event(r,0,base+offsets[step],step%2?86:108,flags,
                         flags?MC_RATE*3u/5u:MC_RATE*2u/5u,0))return -1;
        }
    }
    return aud_until(r,r->frames);
}
static int aud_kit(AudResult *r,unsigned kit) {
    aud_engine.project.drum_kit=(uint8_t)kit;
    for(unsigned lane=0;lane<16;lane++) {
        if(aud_until(r,lane*MC_RATE/2u)||aud_event(r,3,lane,110,0,0,1))return -1;
    }
    for(unsigned step=0;step<16;step++) {
        if(aud_until(r,8u*MC_RATE+step*MC_RATE/8u))return -1;
        if(!(step%4)&&aud_event(r,3,0,115,0,0,1))return -1;
        if(step%4==2&&aud_event(r,3,1,105,0,0,1))return -1;
        if(aud_event(r,3,step==15?3:2,step%2?75:96,0,0,1))return -1;
    }
    return aud_until(r,r->frames);
}
static int aud_demo(AudResult *r,int stress) {
    static const unsigned pitches[8]={48,52,55,59,60,64,67,71};
    static const unsigned sounds[6]={0,1,3,4,5,13};
    if(stress) {
        for(unsigned p=0;p<3;p++) {
            mc_preset(&aud_engine,p,MC_PRESET_GLASS_PAD);
            McPatch *v=&aud_engine.project.patch[p];
            v->source_mask=HY_ALL;v->sustain=127;v->attack=0;
        }
        for(unsigned i=0;i<8;i++)
            if(aud_event(r,i%3,pitches[i],100,0,4u*MC_RATE,0))return -1;
        for(unsigned step=0;step<16;step++) {
            if(aud_until(r,step*MC_RATE/4u))return -1;
            for(unsigned i=0;i<6;i++)if(aud_event(r,3,sounds[i],110,0,0,1))return -1;
        }
    } else {
        mc_preset(&aud_engine,0,MC_PRESET_DEEP_SUB);
        mc_preset(&aud_engine,1,MC_PRESET_STAGE_PIANO);
        mc_preset(&aud_engine,2,MC_PRESET_PULSE_LEAD);
        for(unsigned step=0;step<32;step++) {
            if(aud_until(r,step*MC_RATE/4u))return -1;
            unsigned root=step<16?36:41;
            if(!(step%2)&&aud_event(r,0,root+(step%8==6?12u:0u),108,0,MC_RATE/5u,0))return -1;
            if(!(step%8))for(unsigned k=0;k<3;k++)
                if(aud_event(r,1,root+24+(k==1?3u:k==2?7u:0u),90,0,MC_RATE,0))return -1;
            if(step%2&&aud_event(r,2,root+36+(step%8==7?10u:7u),96,0,MC_RATE/4u,0))return -1;
            if(!(step%4)&&aud_event(r,3,0,116,0,0,1))return -1;
            if(step%4==2&&aud_event(r,3,1,106,0,0,1))return -1;
            if(aud_event(r,3,step%8==7?3:2,80,0,0,1))return -1;
        }
    }
    return aud_until(r,r->frames);
}
static unsigned aud_loop_issues(unsigned preset,unsigned mask) {
    if(!(mask&HY_PCM))return 0;
    unsigned count=0;
    for(unsigned zi=0;zi<MC_PCM_ZONES;zi++) {
        const McPcmZone *z=hy_pcm_zone(zi);
        if(z->family!=hy_profile(preset)->pcm_family)continue;
        for(unsigned mi=0;mi<z->levels;mi++) {
            const McPcmMip *m=&z->mip[mi];
            if(!m->loop_end)continue;
            int jump=m->data[m->loop_start]-m->data[m->loop_end-1],largest=0;
            if(jump<0)jump=-jump;
            for(unsigned n=m->loop_start+1u;n<m->loop_end;n++) {
                int d=m->data[n]-m->data[n-1];if(d<0)d=-d;
                if(d>largest)largest=d;
            }
            count+=jump>largest;
        }
    }
    return count;
}
static int aud_file(FILE *report,const char *dir,const char *name,const char *kind,
                    int preset,int kit,unsigned mask,int song,int *first) {
    char path[4096],hash[65];
    if(snprintf(path,sizeof path,"%s/%s",dir,name)>=(int)sizeof path)return -1;
    AudResult r={0};
    int isdemo=!strcmp(kind,"demo"),stress=!strcmp(kind,"stress"),issong=song>=0;
    aud_setup(preset<0?0u:(unsigned)preset,mask,isdemo||stress);
    unsigned bars=0;
    if(issong) {
        mc_demo_load(&aud_engine,(unsigned)song);
        for(unsigned i=0;i<aud_engine.project.song_length;i++)bars+=aud_engine.project.song[i].repeats;
    }
    r.frames=(stress?7u:isdemo?11u:13u)*MC_RATE;
    if(issong)r.frames=(uint32_t)(((uint64_t)MC_RATE*240u*bars+aud_engine.project.bpm-1u)/
                               aud_engine.project.bpm)+10u*MC_RATE;
    /* 기존 POC/이전 실행 산출물을 덮어쓰지 않는다. */
    r.wav=fopen(path,"wbx");
    if(!r.wav) { perror(path);return -1; }
    uint8_t header[44]={0};
    memcpy(header,"RIFF",4);le32(header+4,36+r.frames*4);memcpy(header+8,"WAVEfmt ",8);
    le32(header+16,16);le16(header+20,1);le16(header+22,2);le32(header+24,MC_RATE);
    le32(header+28,MC_RATE*4);le16(header+32,4);le16(header+34,16);
    memcpy(header+36,"data",4);le32(header+40,r.frames*4);
    if(fwrite(header,1,44,r.wav)!=44) { fclose(r.wav);return -1; }
    int failed;
    if(issong) {
        mc_start(&aud_engine);
        failed=aud_until(&r,r.frames)||aud_engine.playing;
    } else failed=kit>=0?aud_kit(&r,(unsigned)kit):
        (isdemo||stress)?aud_demo(&r,stress):aud_phrase(&r,(unsigned)preset);
    r.end_voices=active_voices();r.end_drums=active_drums();
    if(ferror(r.wav))failed=1;
    if(fclose(r.wav))failed=1;
    if(failed||sha_file(path,hash))return -1;
    if(!*first)fputs(",\n",report);
    *first=0;
    fputs("{\"file\":",report);json_string(report,name);
    fputs(",\"path\":",report);json_string(report,path);
    fputs(",\"kind\":",report);json_string(report,kind);
    fprintf(report,",\"preset_id\":%d,\"presetID\":%d,\"drum_kit\":%d,\"name\":",
            preset,preset,issong?(int)aud_engine.project.drum_kit:kit);
    json_string(report,issong?mc_demo_names[song]:preset>=0?mc_preset_names[preset]:kit>=0?mc_kit_names[kit]:kind);
    fputs(",\"role\":",report);json_string(report,preset>=0?mc_preset_categories[preset]:kind);
    static const char *const genres[32]={
        "T H P E","T H E","T E","T H P E","H P E","H P","T E","T E",
        "P E H","H P E","H P","P E","H P E","H P","T H P E","H P E",
        "H P E","T P E","T P E","H P E","E P T","E P H","H P E","T P E",
        "H P E","P E H","H P E","T H P E","T P E","T P E","T H P E","T H P E"};
    fputs(",\"genre_tags\":",report);json_string(report,issong?(song==MC_DEMO_HIPHOP?"H":"T"):preset>=0?genres[preset]:"T H P E");
    unsigned loop_issues=preset>=0?aud_loop_issues((unsigned)preset,mask):stress?aud_loop_issues(27,7):0;
    unsigned actual_mask=mask,available=preset>=0?hy_profile((unsigned)preset)->available_mask:0;
    if(isdemo||stress||issong)for(unsigned p=0;p<3;p++) {
        actual_mask|=aud_engine.project.patch[p].source_mask;
        available|=hy_profile(aud_engine.project.patch[p].profile)->available_mask;
        if(issong)loop_issues+=aud_loop_issues(aud_engine.project.patch[p].profile,
                                            aud_engine.project.patch[p].source_mask);
    }
    int mechanical_ok=r.nonzero&&!r.rails&&!r.end_voices&&!r.end_drums&&!r.tail_peak&&
        (!stress||r.full_stress_frames>0)&&(!issong||!aud_engine.clips);
    char drum_steals[24];
    snprintf(drum_steals,sizeof drum_steals,"%u",r.drum_steals);
    fprintf(report,",\"source_flags\":%u,\"available_source_flags\":%u,\"sha256\":\"%s\","
        "\"frames\":%u,\"bytes\":%u,\"seconds\":%.6f,\"sample_rate\":%u,\"channels\":2,"
        "\"bits_per_sample\":16,\"peak_pcm16\":%u,\"rms_pcm16\":%.6f,"
        "\"nonzero_samples\":%" PRIu64 ",\"clipped_samples\":%" PRIu64 ","
        "\"engine_clip_frames\":%u,\"limiter_frames\":%u,\"voice_steals\":%u,"
        "\"drum_slot_steals\":%s,\"peak_voices\":%u,\"peak_drums\":%u,"
        "\"end_active_voices\":%u,\"end_active_drums\":%u,\"tail_last_100ms_peak\":%u,"
        "\"ended_tails\":%s,\"eight_triple_frames\":%" PRIu64 ","
        "\"six_drum_frames\":%" PRIu64 ",\"four_fx_send_frames\":%" PRIu64 ","
        "\"four_fx_active_frames\":%" PRIu64 ","
        "\"eight_triple_six_drum_four_fx_frames\":%" PRIu64 ",\"loop_seam_review_count\":%u,"
        "\"quality_status\":\"%s\",\"mechanical_status\":\"%s\","
        "\"analog\":[%u,%u,%u],\"normalization_gain\":1.0,\"adaptive_normalization\":false,",
        actual_mask,available,hash,
        r.frame,44+r.frame*4,(double)r.frame/MC_RATE,MC_RATE,r.peak,
        sqrt((double)r.squares/(r.frame*2.0)),r.nonzero,r.rails,aud_engine.clips,
        aud_engine.limiter_hits,aud_engine.steals,issong?"null":drum_steals,r.peak_voices,r.peak_drums,
        r.end_voices,r.end_drums,r.tail_peak,
        !r.end_voices&&!r.end_drums&&!r.tail_peak?"true":"false",
        r.triple_frames,r.drum_six_frames,r.all_fx_frames,r.fx_active_frames,r.full_stress_frames,loop_issues,
        loop_issues?"FAIL_LOOP_SEAM_REVIEW":"UNAPPROVED_CANDIDATE",
        mechanical_ok?"PASS":"FAIL",
        aud_engine.project.analog[0],aud_engine.project.analog[1],aud_engine.project.analog[2]);
    if(issong) {
        fprintf(report,"\"demo_id\":%d,\"bpm\":%u,\"swing\":%u,\"song_bars\":%u,"
            "\"song_mode\":%s,\"song_loop\":%s,\"song_entries\":[",
            song,aud_engine.project.bpm,aud_engine.project.swing,bars,
            aud_engine.project.song_mode?"true":"false",aud_engine.project.song_loop?"true":"false");
        uint8_t used[MC_PATTERNS]={0};
        for(unsigned i=0;i<aud_engine.project.song_length;i++) {
            const McSongEntry *entry=&aud_engine.project.song[i];used[entry->pattern]=1;
            fprintf(report,"%s{\"pattern\":%u,\"repeats\":%u}",i?",":"",entry->pattern,entry->repeats);
        }
        fputs("],\"score\":[",report);
        unsigned count=0;
        for(unsigned p=0;p<MC_PATTERNS;p++)if(used[p])
            for(unsigned part=0;part<MC_PARTS;part++)for(unsigned step=0;step<MC_STEPS;step++) {
                const McStep *s=&aud_engine.project.step[p][part][step];
                if(!s->velocity||(!s->note[0]&&!s->drums))continue;
                fprintf(report,"%s{\"pattern\":%u,\"part\":%u,\"step\":%u,\"notes\":[%u,%u,%u],"
                    "\"velocity\":%u,\"gate\":%u,\"flags\":%u,\"drums\":%u}",count++?",":"",
                    p,part,step,s->note[0],s->note[1],s->note[2],s->velocity,s->gate,s->flags,s->drums);
            }
        fputs("],",report);
    }
    fputs("\"patches\":[",report);
    for(unsigned p=0;p<3;p++) {
        const McPatch *v=&aud_engine.project.patch[p];
        fprintf(report,"%s{\"profile\":%u,\"mask\":%u,\"gains\":[%u,%u,%u],"
            "\"cutoff\":%u,\"resonance\":%u,\"adsr\":[%u,%u,%u,%u],\"wave\":%u,\"mode\":%u,\"glide\":%u}",
            p?",":"",v->profile,v->source_mask,v->pcm_mix,v->fm_mix,v->va_mix,
            v->cutoff,v->resonance,v->attack,v->decay,v->sustain,v->release,v->wave,v->mode,v->glide);
    }
    fputs("],\"fx_types\":[",report);
    for(unsigned b=0;b<4;b++)fprintf(report,"%s%u",b?",":"",aud_engine.project.fx_bus_type[b]);
    fputs("],\"fx_sends\":[",report);
    for(unsigned p=0;p<4;p++) {
        fprintf(report,"%s[",p?",":"");
        for(unsigned b=0;b<4;b++)fprintf(report,"%s%u",b?",":"",aud_engine.project.fx_send[p][b]);
        fputs("]",report);
    }
    fputs("],\"events\":[",report);
    for(unsigned i=0;i<r.count;i++) {
        const AudEvent *e=&r.events[i];
        fprintf(report,"%s{\"frame\":%u,\"part\":%u,\"note\":%u,\"velocity\":%u,"
            "\"flags\":%u,\"gate_frames\":%u,\"drum\":%s}",i?",":"",
            e->frame,e->part,e->note,e->velocity,e->flags,e->gate,e->drum?"true":"false");
    }
    fputs("]}",report);
    return ferror(report)||!mechanical_ok?-1:0;
}
int auditions(const char *dir,const char *json) {
    if(mkdir(dir,0755)&&errno!=EEXIST) { perror(dir);return 1; }
    FILE *f=fopen(json,"wbx");
    if(!f) { perror(json);return 1; }
    fputs("{\"schema_version\":2,\"host_only\":true,\"renderer\":\"McEngine shared C\","
        "\"subjective_listening_approved\":false,\"target_test_performed\":false,"
        "\"quality_status\":\"UNAPPROVED_CANDIDATE\",\"wasm_test_performed\":false,"
        "\"device_test_performed\":false,\"comparison_policy\":\"same events and gain; mask only, no loudness matching\","
        "\"files\":[\n",f);
    int first=1,failed=0;char name[80];
    for(unsigned id=0;id<32&&!failed;id++) {
        snprintf(name,sizeof name,"preset-%02u.wav",id);
        failed=aud_file(f,dir,name,"preset",(int)id,-1,mc_presets[id].source_mask,-1,&first);
        unsigned available=hy_profile(id)->available_mask;
        if((available&(available-1u))!=0)for(unsigned bit=1;bit<=4&&!failed;bit*=2)if(available&bit) {
            snprintf(name,sizeof name,"preset-%02u-source-%u.wav",id,bit);
            failed=aud_file(f,dir,name,"source",(int)id,-1,bit,-1,&first);
        }
    }
    for(unsigned kit=0;kit<8&&!failed;kit++) {
        snprintf(name,sizeof name,"kit-%02u.wav",kit);
        failed=aud_file(f,dir,name,"kit",-1,(int)kit,0,-1,&first);
    }
    if(!failed)failed=aud_file(f,dir,"hiphop-pocket.wav","song",-1,-1,0,MC_DEMO_HIPHOP,&first);
    if(!failed)failed=aud_file(f,dir,"techno-drive.wav","song",-1,-1,0,MC_DEMO_TECHNO,&first);
    if(!failed)failed=aud_file(f,dir,"modern-demo.wav","demo",-1,-1,0,-1,&first);
    if(!failed)failed=aud_file(f,dir,"modern-stress.wav","stress",-1,-1,7,-1,&first);
    fprintf(f,"\n],\"render_complete\":%s}\n",failed?"false":"true");
    if(ferror(f))failed=1;
    if(fclose(f))failed=1;
    if(failed) { fprintf(stderr,"Failed to write auditions: %s\n",json);return 1; }
    printf("PASS: actual C auditions saved: %s; no listening or hardware approval implied\n",json);
    return 0;
}
