/* SPDX-License-Identifier: GPL-3.0-only */
#include "ui.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static McEngine e;
static McProject decoded;
static uint8_t record[MC_RECORD_BYTES],again[MC_RECORD_BYTES];
static McEngine dry;
static McUi ui;
static uint16_t pixels[240*8];
static void drain(void) { McMidiMessage m;while(mc_midi_pop(&e,&m)) {} }
static void selection(void) {
    mc_init(&e);mc_ui_init(&ui);assert(MC_DEMOS==4&&ui.demo_choice==1);
    ui.page=4;
    for(unsigned i=0;i<MC_DEMOS;i++) {
        mc_ui_encoder(&e,&ui,3,1);unsigned choice=(i+2)%MC_DEMOS;
        assert(ui.demo_choice==choice&&mc_demo_names[choice]&&*mc_demo_names[choice]);
        mc_ui_encoder(&e,&ui,6,1);assert(ui.load_request==choice+1);
        for(unsigned y=0;y<240;y+=8)mc_ui_strip(&e,&ui,y,8,pixels);
    }
    mc_ui_encoder(&e,&ui,3,-2);assert(ui.demo_choice==3);
    mc_ui_encoder_at(&e,&ui,3,1,100);assert(ui.demo_choice==0);
    mc_ui_encoder_at(&e,&ui,3,1,101);assert(ui.demo_choice==1);
    ui.load_request=0;mc_start(&e);mc_ui_encoder(&e,&ui,6,1);assert(!ui.load_request);
    uint32_t hash=mc_crc(&e,sizeof e);
    mc_demo_load(&e,MC_DEMOS);assert(hash==mc_crc(&e,sizeof e));
    mc_demo_load(&e,UINT_MAX);assert(hash==mc_crc(&e,sizeof e));
    puts("PASS demo UI: default 1, four choices, signed wrap, load guard, invalid IDs");
}
static void midi(void) {
    static const unsigned controls[3]={64,120,123};
    for(unsigned demo=0;demo<MC_DEMOS;demo++)for(unsigned mode=0;mode<4;mode++) {
        McMidiMessage m;
        mc_init(&e);e.project.midi_channel[0]=14;
        if(mode==1) { e.project.arp[0].enabled=1;e.project.arp[0].latch=1; }
        mc_key(&e,0,60,100,1);int16_t frame[2];mc_render(&e,frame,1);drain();
        if(mode==2) {
            mc_midi_byte(&e,1,0xbe);mc_midi_byte(&e,1,64);mc_midi_byte(&e,1,127);
            mc_key(&e,0,60,0,0);
        }
        mc_midi_send(&e,0xfa,0,0,1);drain();
        for(unsigned i=0;i<(mode==3?MC_MIDI_QUEUE+1u:8u);i++)mc_midi_send(&e,0x9e,72,100,3);
        mc_demo_load(&e,demo);
        assert(!e.playing&&!e.midi.sustain[0]&&!e.arp[0].count);
        for(unsigned v=0;v<MC_VOICES;v++)assert(!e.voice[v].active);
        assert(mc_midi_pop(&e,&m)&&m.length==1&&m.data[0]==0xfc);
        for(unsigned i=0;i<3;i++) {
            assert(mc_midi_pop(&e,&m)&&m.length==3&&m.data[0]==0xbe);
            assert(m.data[1]==controls[i]&&!m.data[2]);
        }
        for(unsigned p=0;p<3;p++) {
            assert(mc_midi_pop(&e,&m)&&m.length==2&&m.data[0]==0xc0+e.project.midi_channel[p]);
            assert(m.data[1]==e.project.preset[p]);
        }
        assert(!mc_midi_pop(&e,&m)&&!e.midi.panic&&!e.midi.cleanup&&!e.midi.output_running);
    }
    puts("PASS all four demo MIDI loads: programs, pending notes, ARP, sustain, overflow, Stop and channel cleanup");
}
/* SONG sends are explicit mix edits; the four effect processors remain shared. */
static void fx_contract(unsigned demo) {
    static const uint8_t original_types[2][MC_FX_BANKS]={{3,0,7,6},{5,1,6,4}};
    static const uint8_t original_sends[2][MC_PARTS][MC_FX_BANKS]={
        {{8,0,10,0},{52,28,38,0},{24,0,28,18},{10,0,0,0}},
        {{8,0,12,0},{40,18,44,0},{20,0,26,16},{8,0,0,0}}};
    static const uint8_t song_sends[2][MC_PARTS][MC_FX_BANKS]={
        {{2,0,0,0},{8,3,4,6},{8,0,4,4},{2,0,0,0}},
        {{4,0,8,4},{8,4,0,4},{10,0,4,4},{2,0,0,0}}};
    mc_init(&e);mc_demo_load(&e,demo);
    assert(!memcmp(e.project.fx_bus_type,demo<MC_DEMO_HIPHOP?
        original_types[demo]:mc_factory_fx_types,sizeof e.project.fx_bus_type));
    for(unsigned part=0;part<MC_PARTS;part++) {
        unsigned live=0;
        const uint8_t *expected=demo>=MC_DEMO_HIPHOP? song_sends[demo-MC_DEMO_HIPHOP][part]:
            original_sends[demo][part];
        assert(!memcmp(e.project.fx_send[part],expected,MC_FX_BANKS));
        for(unsigned bank=0;bank<MC_FX_BANKS;bank++)live+=e.project.fx_send[part][bank];
        assert(live);
    }
}
static void original_score(unsigned demo) {
    static const uint8_t roots[2][4]={{45,41,48,43},{40,40,43,41}};
    static const uint8_t melody[16]={0,0,7,0,12,10,0,7,3,0,7,10,12,0,15,10};
    static const uint8_t motif[16]={0,12,7,15,12,7,19,12,0,7,12,15,19,15,7,12};
    mc_init(&e);mc_demo_load(&e,demo);
    assert(e.project.bpm==(demo?140:118)&&e.project.swing==(demo?0:10));
    assert(e.project.demo==demo&&!e.project.song_mode&&!e.project.automation_count);
    for(unsigned p=0;p<MC_PATTERNS;p++) {
        McStep expected[MC_PARTS][MC_STEPS]={0};
        if(p<4)for(unsigned s=0;s<MC_STEPS;s++) {
            unsigned root=roots[demo][p];
            McStep *bass=&expected[0][s],*middle=&expected[1][s];
            McStep *hook=&expected[2][s],*drum=&expected[3][s];
            if(!demo) {
                *bass=(McStep){{(uint8_t)(root+((s==6||s==14)?12:0)),0,0},
                    (uint8_t)(s==3||s==11?0:s%4==0?115:85),65,
                    (uint8_t)((s%4==0?MC_ACCENT:0)|(s==5||s==13?MC_SLIDE:0)),0};
                if(s%8==0)*middle=(McStep){{(uint8_t)(root+12),(uint8_t)(root+15),(uint8_t)(root+19)},76,100,0,0};
                if(s%2==0||s==7||s==15)*hook=(McStep){{(uint8_t)(root+24+melody[s]),0,0},
                    (uint8_t)(80+(s%3)*10),55,0,0};
                drum->drums=(uint16_t)((s%4==0?1:0)|(s%8==4?2:0)|(s%2==0?4:0)|
                    (s==14?8:0)|(s==12?16:0)|(p==3&&s==15?32:0));
                drum->velocity=(uint8_t)(s%4==0?112:85);
            } else {
                *bass=(McStep){{(uint8_t)(root+(s==15?12:0)),0,0},(uint8_t)(s%4?100:0),42,0,0};
                *middle=(McStep){{(uint8_t)(root+12+motif[(s+p*2)%16]),0,0},
                    (uint8_t)(s%4==0?115:90),50,
                    (uint8_t)((s%4==0?MC_ACCENT:0)|(s%8==5?MC_SLIDE:0)),0};
                if(s%2==0||s==15)*hook=(McStep){{(uint8_t)(root+24+motif[(s+4)%16]),0,0},
                    (uint8_t)(s%4==0?104:92),45,0,0};
                drum->drums=(uint16_t)((s%4==0?1:0)|(s%2==1?4:0)|(s%4==2?8:0)|
                    (s%8==4?16:0)|(s%4==3?(1u<<12):0)|(p==3&&s>=14?(1u<<9):0));
                drum->velocity=(uint8_t)(s%4==0?120:85);
            }
            drum->gate=50;
        }
        assert(!memcmp(e.project.step[p],expected,sizeof expected));
    }
}
static void original_roles(void) {
    static const uint8_t presets[2][3]={{1,25,8},{1,1,15}};
    /* Original editable controls, with the new Strings source's sine wave. */
    static const uint8_t controls[2][2][10]={
        {{1,0,48,90,1,55,45,10,55,100},{0,2,58,0,94,120,127,68,0,88}},
        {{1,1,65,35,1,30,45,15,70,100},{1,0,80,35,1,40,35,8,55,90}}};
    static const uint8_t levels[2][4]={{100,75,104,100},{127,100,104,75}};
    static const uint8_t analog[2][3]={{18,52,8},{24,60,6}};
    static const uint8_t pans[4]={64,42,88,64};
    static const uint8_t fx[2][4]={{0,0,0,0},{48,35,20,30}};
    for(unsigned demo=0;demo<2;demo++) {
        original_score(demo);
        assert(mc_project_valid(&e.project));
        assert(!memcmp(e.project.preset,presets[demo],3));
        assert(!memcmp(e.project.level,levels[demo],4)&&!memcmp(e.project.pan,pans,4));
        assert(!memcmp(e.project.analog,analog[demo],3)&&!memcmp(e.project.fx,fx[demo],4));
        for(unsigned part=0;part<2;part++) {
            const McPatch *p=&e.project.patch[part],*base=&mc_presets[presets[demo][part]];
            uint8_t actual[10]={p->mode,p->wave,p->cutoff,p->resonance,p->attack,p->decay,
                p->sustain,p->release,p->glide,p->level};
            assert(!memcmp(actual,controls[demo][part],sizeof actual));
            assert(p->profile==base->profile&&p->source_mask==base->source_mask);
            assert(p->pcm_mix==base->pcm_mix&&p->fm_mix==base->fm_mix&&p->va_mix==base->va_mix);
        }
        McPatch hook=mc_presets[presets[demo][2]];
        hook.resonance=8;
        if(!demo) { hook.pcm_mix=108;hook.cutoff=100; }
        assert(!memcmp(&e.project.patch[2],&hook,sizeof hook));
        for(unsigned d=0;d<MC_DRUMS;d++) {
            unsigned gain=demo?(d==0?115:d==2?70:d==3?64:d==4?85:100):100;
            assert(e.project.drum_level[d]==gain);
        }
        assert(!e.project.drum_kit);
    }
    mc_preset(&e,1,MC_PRESET_SLOW_STRINGS);
    assert(e.project.patch[1].attack==4);
    mc_demo_load(&e,1);drain();e.playing=1;
    for(unsigned bar=0;bar<32;bar++)for(unsigned frame=0;frame<3;frame++) {
        e.events=bar*16+1;e.queued=255;e.pattern=(uint8_t)(bar%4);
        unsigned span=MC_RATE*240u*4u/e.project.bpm;
        e.demo_frame=((span-1u)*frame/2u/64u)*64u;
        int rise=(int)((e.demo_frame%span)*90u/span);
        int cutoff=bar<8?30+rise/2:bar<16?35+rise:bar<20?20+rise:bar<24?95-rise/2:40+rise;
        int resonance=bar<16?45:bar<20?35+rise:100;
        mc_demo_tick(&e);drain();
        assert(e.mute==(bar<4?2:bar<16?0:bar<20?13:bar<24?4:0));
        assert(e.queued==(bar+1)%4);
        assert(e.project.patch[1].cutoff==(cutoff>127?127:cutoff));
        assert(e.project.patch[1].resonance==resonance&&e.project.patch[2].cutoff==88+rise/5);
    }
    McPatch before=e.project.patch[1];
    e.demo_frame=1;mc_demo_tick(&e);
    assert(!memcmp(&before,&e.project.patch[1],sizeof before));
    e.events=32*16+1;e.playing=1;mc_demo_tick(&e);assert(!e.playing);
    puts("PASS original demos: exact scores, roles, controls, mix and 32-bar scene automation");
}
/* Full-tone and zero-send renders of the same loaded song must differ; sends are cleared after load. */
static void wet_dry(unsigned demo) {
    mc_init(&e);mc_demo_load(&e,demo);
    mc_init(&dry);mc_demo_load(&dry,demo);
    memset(dry.project.fx_send,0,sizeof dry.project.fx_send);
    mc_start(&e);mc_start(&dry);
    uint64_t diff=0,energy=0;
    for(unsigned n=0;n<4*MC_RATE;n++) {
        int16_t a[2],b[2];mc_render(&e,a,1);mc_render(&dry,b,1);
        for(unsigned c=0;c<2;c++) {
            int d=a[c]-b[c];diff+=(uint64_t)(d<0?-d:d);energy+=(uint64_t)(b[c]<0?-b[c]:b[c]);
        }
        if(e.midi.count)drain();
        if(dry.midi.count) { McMidiMessage m;while(mc_midi_pop(&dry,&m)) {} }
    }
    assert(energy&&diff);
    printf("PASS %s wet-vs-dry: dry_abs=%llu wet_minus_dry_abs=%llu\n",mc_demo_names[demo],
        (unsigned long long)energy,(unsigned long long)diff);
}
static uint64_t mix_energy(unsigned demo,unsigned mode) {
    mc_init(&e);mc_demo_load(&e,demo);
    e.mute=(uint8_t)(mode==0?8:mode==1?14:mode==2?13:7);
    if(mode==3)for(unsigned p=0;p<MC_PATTERNS;p++)for(unsigned s=0;s<MC_STEPS;s++)
        e.project.step[p][3][s].drums&=1u;
    unsigned bar_frames=(MC_RATE*240u+e.project.bpm-1u)/e.project.bpm;
    unsigned frames=(MC_RATE*240u*8u+e.project.bpm-1u)/e.project.bpm;
    uint64_t energy=0,samples=0;
    unsigned transient=demo==MC_DEMO_HIPHOP&&mode==2;
    mc_start(&e);
    for(unsigned offset=0;offset<frames;) {
        int16_t audio[256];
        unsigned block=transient?1u:128u;
        unsigned count=frames-offset>block?block:frames-offset;
        mc_render(&e,audio,count);
        unsigned sounding=!transient;
        if(transient)for(unsigned v=0;v<MC_VOICES;v++)
            sounding|=e.voice[v].active&&e.voice[v].part==1&&e.voice[v].envelope>0;
        for(unsigned i=0;i<count;i++)if(offset+i>=bar_frames&&sounding)
            for(unsigned c=0;c<2;c++) {
                int64_t value=audio[2*i+c];energy+=(uint64_t)(value*value);
                samples++;
            }
        drain();offset+=count;
    }
    assert(energy&&samples&&!e.clips&&!e.limiter_hits&&!e.steals);
    return energy/samples;
}
/* Compare seven drum-bearing bars at the shipped master and effect sends.
 * The compact foreground pop is measured while its actual EG sounds, not
 * penalized for the newly intentional silence between short notes. */
static void mix_balance(unsigned demo) {
    uint64_t melody=mix_energy(demo,0),bass=mix_energy(demo,1);
    uint64_t lead=mix_energy(demo,2),kick=mix_energy(demo,3);
    assert(melody>=kick&&bass*25>=kick*16);
    if(demo==MC_DEMO_HIPHOP)assert(lead*25>=kick*16&&lead*64>=bass*9);
    printf("PASS %s mix mean-square: melody=%llu bass=%llu lead=%llu kick=%llu\n",
        mc_demo_names[demo],(unsigned long long)melody,(unsigned long long)bass,
        (unsigned long long)lead,(unsigned long long)kick);
}
static void kick_attack(unsigned demo) {
    mc_init(&e);mc_demo_load(&e,demo);e.mute=7;
    for(unsigned p=0;p<MC_PATTERNS;p++)for(unsigned s=0;s<MC_STEPS;s++)
        e.project.step[p][3][s].drums&=1u;
    mc_start(&e);
    unsigned found=0,age=0,peak=0;
    uint64_t energy=0;
    unsigned bound=MC_RATE*480u/e.project.bpm;
    for(unsigned n=0;n<bound;n++) {
        int16_t audio[2];mc_render(&e,audio,1);drain();
        if(!found)for(unsigned v=0;v<MC_DRUM_VOICES;v++)
            if(e.drum[v].env&&e.drum[v].sound==0&&e.drum[v].age==1u) {
                assert(e.pattern==1u&&e.step==0u);found=1;
            }
        if(found) {
            for(unsigned c=0;c<2;c++) {
                unsigned magnitude=(unsigned)(audio[c]<0?-(int)audio[c]:audio[c]);
                if(magnitude>peak)peak=magnitude;
                energy+=(uint64_t)((int64_t)audio[c]*audio[c]);
            }
            if(++age==MC_RATE/100u)break;
        }
    }
    assert(found&&age==MC_RATE/100u&&energy);
    assert(peak>(demo==MC_DEMO_HIPHOP?5000u:2500u));
    assert(!e.clips&&!e.limiter_hits&&e.master==MC_MASTER_DEFAULT);
    printf("PASS %s first kick: actual SONG trigger, 10ms peak=%u, unchanged master\n",
        mc_demo_names[demo],peak);
}
static void song(unsigned demo) {
    static const uint8_t presets[2][3]={{2,28,14},{1,18,17}};
    unsigned techno=demo==MC_DEMO_TECHNO,bars=techno?32u:24u;
    mc_init(&e);mc_demo_load(&e,demo);
    assert(!e.playing&&!e.project.demo&&e.project.song_mode&&!e.project.song_loop);
    assert(e.project.bpm==(techno?144:128)&&e.project.swing==(techno?0:12));
    assert(e.project.drum_kit==1&&mc_project_valid(&e.project));
    assert(!memcmp(e.project.preset,presets[techno],3));
    unsigned total=0,parts[4]={0},used=0;
    for(unsigned i=0;i<e.project.song_length;i++) {
        total+=e.project.song[i].repeats;used|=1u<<e.project.song[i].pattern;
    }
    assert(total==bars&&used==65535u);
    unsigned accents=0,slides=0;
    for(unsigned p=0;p<16;p++) {
        for(unsigned q=p+1;q<16;q++)assert(memcmp(e.project.step[p],e.project.step[q],sizeof e.project.step[p]));
        for(unsigned t=0;t<4;t++)for(unsigned s=0;s<16;s++) {
            const McStep *step=&e.project.step[p][t][s];
            if(t==3)parts[t]+=step->drums!=0;
            else {
                parts[t]+=step->velocity!=0;
                accents+=(step->flags&MC_ACCENT)!=0;
                slides+=(step->flags&MC_SLIDE)!=0;
            }
        }
    }
    for(unsigned t=0;t<4;t++)assert(parts[t]);
    assert(accents&&slides&&e.project.patch[0].mode==MC_MONO);
    for(unsigned step=0;step<MC_STEPS;step++)assert(!e.project.step[0][3][step].drums);
    assert(e.project.step[1][3][0].drums&1);
    assert(e.project.step[1][3][0].velocity>=110&&e.project.drum_level[0]==(techno?88:96));
    if(!techno) {
        assert(e.project.step[0][0][0].note[0]==43&&e.project.step[0][0][0].velocity==118);
        assert(e.project.step[0][1][1].note[0]==67&&e.project.step[0][1][1].velocity==108);
        assert(!e.project.automation_count);
    } else {
        assert(e.project.step[0][0][0].note[0]==60&&e.project.step[0][0][0].velocity==112);
        assert(e.project.step[0][0][1].note[0]==55&&(e.project.step[0][0][1].flags&MC_SLIDE));
        assert(!e.project.step[1][0][0].velocity);
        assert(e.project.step[1][0][1].note[0]==60);
        assert(e.project.step[1][0][2].note[0]==55&&(e.project.step[1][0][2].flags&MC_SLIDE));
        assert(e.project.automation_count==112);
        unsigned cutoff_changes=0,resonance_changes=0;
        for(unsigned i=0;i<e.project.automation_count;i++) {
            const McAutomation *a=&e.project.automation[i];
            assert(a->address/5<16&&a->address%5==0&&a->step<16&&a->value<=127);
            assert(a->parameter==4||a->parameter==5);
            cutoff_changes+=a->parameter==4;resonance_changes+=a->parameter==5;
        }
        assert(cutoff_changes&&resonance_changes);
    }
    uint32_t sequence=0;
    assert(mc_encode(&e.project,23,record,sizeof record)==MC_RECORD_BYTES);
    assert(!mc_decode(&decoded,&sequence,record,sizeof record)&&sequence==23);
    assert(!memcmp(&decoded,&e.project,sizeof decoded));
    assert(mc_encode(&decoded,23,again,sizeof again)==MC_RECORD_BYTES&&!memcmp(record,again,sizeof record));
    /* 복원된 프로젝트 자체를 내부 시계와 실제 C 오디오 경로로 끝까지 재생한다. */
    e.project=decoded;drain();mc_start(&e);
    unsigned frames=0,max_voice=0,max_drum=0,seen=0,peak=0;
    uint64_t energy=0;
    unsigned bound=MC_RATE*240u*bars/e.project.bpm+MC_RATE;
    while(e.playing&&frames<bound) {
        int16_t audio[2];mc_render(&e,audio,1);frames++;
        seen|=1u<<e.pattern;
        unsigned voices=0,drums=0;
        for(unsigned v=0;v<MC_VOICES;v++)voices+=e.voice[v].active!=0;
        for(unsigned d=0;d<MC_DRUM_VOICES;d++)drums+=e.drum[d].env!=0;
        if(voices>max_voice)max_voice=voices;
        if(drums>max_drum)max_drum=drums;
        for(unsigned c=0;c<2;c++) {
            unsigned value=(unsigned)(audio[c]<0?-(int)audio[c]:audio[c]);
            energy+=value;if(value>peak)peak=value;
        }
        if(e.midi.count)drain();
    }
    assert(!e.playing&&e.song_finished&&seen==65535u&&e.events==bars*16);
    /* 내부 시계는 각 스텝의 나머지를 다음 스텝에 누적한다. */
    assert(frames==(MC_RATE*240u*bars+e.project.bpm-1u)/e.project.bpm);
    assert(energy&&peak<MC_OUTPUT_CEILING&&!e.clips&&!e.limiter_hits&&!e.steals);
    assert(max_voice<=8&&max_drum<=6&&e.master==MC_MASTER_DEFAULT);
    /* 종결의 빈 후반부와 짧은 EG 뒤에는 FX 꼬리도 충분히 작아진다. */
    unsigned tail_peak=0;
    /* Render 12 s after the scheduled performance; an unfinished tail fails, history is never cleared. */
    for(unsigned n=0;n<12u*MC_RATE;n++) {
        int16_t audio[2];mc_render(&e,audio,1);
        if(n>=12u*MC_RATE-1024)for(unsigned c=0;c<2;c++) {
            unsigned value=(unsigned)(audio[c]<0?-(int)audio[c]:audio[c]);
            if(value>tail_peak)tail_peak=value;
        }
        if(e.midi.count)drain();
    }
    assert(tail_peak<32);
    printf("PASS %s: bpm=%u swing=%u bars=%u frames=%u events=%u peak=%u clips=%u limiter=%u steals=%u slots=%u/%u tail=%u roundtrip=exact\n",
        mc_demo_names[demo],e.project.bpm,e.project.swing,bars,frames,e.events,peak,e.clips,e.limiter_hits,e.steals,max_voice,max_drum,tail_peak);
}
int main(void) {
    selection();midi();original_roles();
    for(unsigned demo=0;demo<MC_DEMOS;demo++)fx_contract(demo);
    puts("PASS demo FX: original shared settings for demos 0/1, candidate palette for demos 2/3");
    wet_dry(MC_DEMO_HIPHOP);wet_dry(MC_DEMO_TECHNO);
    mix_balance(MC_DEMO_HIPHOP);mix_balance(MC_DEMO_TECHNO);
    kick_attack(MC_DEMO_HIPHOP);kick_attack(MC_DEMO_TECHNO);
    song(MC_DEMO_HIPHOP);song(MC_DEMO_TECHNO);
    puts("demo-songs: PASS");
}
