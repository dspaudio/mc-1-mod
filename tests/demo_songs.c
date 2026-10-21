/* SPDX-License-Identifier: GPL-3.0-only */
#include "ui.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static McEngine e;
static McProject decoded;
static uint8_t record[MC_RECORD_BYTES],again[MC_RECORD_BYTES];
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
        unsigned programs=demo?3u:1u;
        for(unsigned part=0;part<programs;part++) {
            unsigned p=demo?part:2u;
            assert(mc_midi_pop(&e,&m)&&m.length==2&&m.data[0]==0xc0+e.project.midi_channel[p]);
            assert(m.data[1]==e.project.preset[p]);
        }
        assert(!mc_midi_pop(&e,&m)&&!e.midi.panic&&!e.midi.cleanup&&!e.midi.output_running);
    }
    puts("PASS all four demo MIDI loads: programs, pending notes, ARP, sustain, overflow, Stop and channel cleanup");
}
static void song(unsigned demo) {
    static const uint8_t presets[2][3]={{5,10,12},{2,17,29}};
    unsigned techno=demo==MC_DEMO_TECHNO,bars=techno?32u:24u;
    mc_init(&e);mc_demo_load(&e,demo);
    assert(!e.playing&&!e.project.demo&&e.project.song_mode&&!e.project.song_loop);
    assert(e.project.bpm==(techno?144:128)&&e.project.swing==(techno?0:12));
    assert(e.project.drum_kit==(techno?1:4)&&mc_project_valid(&e.project));
    assert(!memcmp(e.project.preset,presets[techno],3));
    unsigned total=0,parts[4]={0},used=0;
    for(unsigned i=0;i<e.project.song_length;i++) {
        total+=e.project.song[i].repeats;used|=1u<<e.project.song[i].pattern;
    }
    assert(total==bars&&used==255);
    for(unsigned p=0;p<8;p++) {
        for(unsigned q=p+1;q<8;q++)assert(memcmp(e.project.step[p],e.project.step[q],sizeof e.project.step[p]));
        for(unsigned t=0;t<4;t++)for(unsigned s=0;s<16;s++) {
            const McStep *step=&e.project.step[p][t][s];
            if(t==3)parts[t]+=step->drums!=0;
            else parts[t]+=step->velocity!=0;
        }
    }
    for(unsigned t=0;t<4;t++)assert(parts[t]);
    for(unsigned p=1;p<7;p++)if(p!=4)for(unsigned s=0;s<16;s+=4)
        assert(e.project.step[p][3][s].drums&1);
    if(!techno) {
        assert(e.project.step[1][0][3].velocity&&e.project.step[1][0][6].velocity);
        assert(e.project.step[5][0][7].velocity&&e.project.step[5][0][15].velocity);
        assert(e.project.step[1][1][2].velocity&&e.project.step[1][1][7].velocity&&e.project.step[1][1][10].velocity);
    }
    for(unsigned s=0;s<16;s++)assert(!e.project.step[4][0][s].velocity&&!(e.project.step[4][3][s].drums&1));
    assert(e.project.step[3][3][14].drums&(1u<<5));
    assert(e.project.step[3][3][15].drums&(1u<<9));
    for(unsigned s=8;s<16;s++)for(unsigned t=0;t<4;t++)
        assert(!e.project.step[7][t][s].velocity&&!e.project.step[7][t][s].drums);
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
    assert(!e.playing&&e.song_finished&&seen==255&&e.events==bars*16);
    /* 내부 시계는 각 스텝의 나머지를 다음 스텝에 누적한다. */
    assert(frames==(MC_RATE*240u*bars+e.project.bpm-1u)/e.project.bpm);
    assert(energy&&peak<MC_OUTPUT_CEILING&&!e.clips&&!e.limiter_hits&&!e.steals);
    assert(max_voice<=8&&max_drum<=6&&e.master==MC_MASTER_DEFAULT);
    /* 종결의 빈 후반부와 짧은 EG 뒤에는 FX 꼬리도 충분히 작아진다. */
    unsigned tail_peak=0;
    for(unsigned n=0;n<MC_RATE;n++) {
        int16_t audio[2];mc_render(&e,audio,1);
        if(n>=MC_RATE-1024)for(unsigned c=0;c<2;c++) {
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
    selection();midi();song(MC_DEMO_HIPHOP);song(MC_DEMO_TECHNO);
    puts("demo-songs: PASS");
}
