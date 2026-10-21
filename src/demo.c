/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 dspaudio. Original NEON SPIRAL composition. */
#include "mcfm.h"
const char *const mc_demo_names[MC_DEMOS]={
    "NIGHT CIRCUIT","NEON SPIRAL","HIPHOP POCKET","TECHNO DRIVE"};

static void arranged_song(McEngine *e,unsigned techno) {
    McProject *p=&e->project;
    static const uint8_t presets[2][3]={
        {MC_PRESET_ROUND_FINGER,MC_PRESET_WARM_TINES,MC_PRESET_MUTED_GTR},
        {MC_PRESET_PULSE_DRIVE,MC_PRESET_BRASS_STAB,MC_PRESET_DIGITAL_ARP}};
    static const McSongEntry hiphop[]={{0,2},{1,3},{2,2},{3,1},{4,2},{5,3},{6,1},{2,3},{3,1},{5,4},{7,2}};
    static const McSongEntry drive[]={{0,4},{1,3},{3,1},{2,4},{4,4},{5,3},{6,1},{2,3},{3,1},{5,6},{7,2}};
    static const uint8_t roots[2][8]={{41,41,38,38,46,41,38,41},{33,33,36,36,40,33,36,33}};
    static const uint8_t phrase[8]={0,7,10,7,12,10,7,3};
    static const uint8_t levels[2][4]={{78,65,58,82},{74,58,52,84}};
    static const uint8_t sends[2][4][MC_FX_BANKS]={
        {{0,0,0,0},{22,0,0,0},{10,0,8,0},{5,0,0,0}},
        {{0,0,0,0},{14,0,8,0},{10,0,14,0},{4,0,0,0}}};
    for(unsigned i=0;i<sizeof p->step;i++)((uint8_t *)p->step)[i]=0;
    p->bpm=(uint16_t)(techno?144:128);p->swing=(uint8_t)(techno?0:12);
    p->drum_kit=(uint8_t)(techno?1:4);p->demo=0;p->song_mode=1;p->song_loop=0;
    p->song_length=(uint8_t)(techno?sizeof drive/sizeof *drive:sizeof hiphop/sizeof *hiphop);
    for(unsigned i=0;i<p->song_length;i++)p->song[i]=techno?drive[i]:hiphop[i];
    e->midi.receiving=1;
    for(unsigned part=0;part<3;part++) {
        mc_preset(e,part,presets[techno][part]);
        /* 소스 조합과 음정은 유지하고 공통 EG의 꼬리만 짧게 한다. */
        p->patch[part].release=8;
        p->patch[part].decay=(uint8_t)(part==0?32:40);
        p->patch[part].sustain=(uint8_t)(part==0?48:0);
        p->analog[part]=0;
    }
    e->midi.receiving=0;
    for(unsigned part=0;part<3;part++)
        mc_midi_send(e,0xc0u+p->midi_channel[part],p->preset[part],0,2);
    p->fx_bus_type[0]=0;p->fx_bus_type[1]=0;p->fx_bus_type[2]=6;p->fx_bus_type[3]=0;
    for(unsigned part=0;part<4;part++) {
        p->level[part]=levels[techno][part];
        p->pan[part]=(uint8_t)(part==1?54:part==2?78:64);
        for(unsigned bank=0;bank<MC_FX_BANKS;bank++)p->fx_send[part][bank]=sends[techno][part][bank];
    }
    for(unsigned d=0;d<MC_DRUMS;d++)p->drum_level[d]=76;
    p->drum_level[0]=110;p->drum_level[1]=90;p->drum_level[2]=55;
    p->drum_level[3]=55;p->drum_level[4]=72;p->drum_level[12]=42;
    /* 0 도입, 1/2 본문, 3 필, 4 브레이크, 5/6 훅, 7 여백 있는 종결. */
    for(unsigned pat=0;pat<8;pat++)for(unsigned s=0;s<16;s++) {
        unsigned root=roots[techno][pat],intro=pat==0,brk=pat==4,outro=pat==7;
        unsigned hook=pat==5||pat==6,fill=pat==3||pat==6;
        if(outro&&s>=8)continue;
        McStep *bass=&p->step[pat][0][s],*keys=&p->step[pat][1][s];
        McStep *lead=&p->step[pat][2][s],*drum=&p->step[pat][3][s];
        unsigned bass_hit=techno?(s%4==2||s==7||s==15):
            (s==0||s==3||s==6||s==10||s==11||s==14||(hook&&(s==7||s==15)));
        if(!brk&&(!intro||s<4)&&bass_hit) {
            bass->note[0]=(uint8_t)(root+(s==7||s==11?7:s==14||s==15?10:0));
            bass->velocity=(uint8_t)(s%4==0?104:88);bass->gate=(uint8_t)(techno?38:42);
        }
        if(s==2||s==10||(!techno&&s==7)||(hook&&s==14)) {
            keys->note[0]=(uint8_t)(root+24);keys->note[1]=(uint8_t)(root+27);
            if(!techno)keys->note[2]=(uint8_t)(root+(s==7?36:34));
            keys->velocity=(uint8_t)(intro?62:brk?72:84);keys->gate=(uint8_t)(techno?32:38);
        }
        unsigned lead_hit=techno?s%2==0:(s==1||s==6||s==11||(hook&&s==15));
        if(!intro&&!outro&&lead_hit&&(!brk||s==6)) {
            lead->note[0]=(uint8_t)(root+36+phrase[(s/2+(pat==2||pat==6?2u:0u))%8]);
            lead->velocity=(uint8_t)(hook?88:72);lead->gate=(uint8_t)(techno?30:45);
        }
        unsigned mask=0;
        if(techno) {
            if(!brk&&s%4==0)mask|=1;
            if(!intro&&!brk&&s%8==4)mask|=16;
            if(s%2==0)mask|=hook&&s%4==2?8u:4u;
            if(hook&&s%4==3)mask|=1u<<12;
        } else {
            /* 4박 킥 위에 당김음 베이스와 스네어/클랩을 겹친 댄스 바운스다. */
            if(!brk&&(s%4==0||(hook&&s==7)))mask|=1;
            if(!intro&&!brk&&s%8==4)mask|=2|16;
            if(s%2==0&&(!brk||s%4==2))mask|=4;
            if(hook&&s%4==3)mask|=1u<<12;
            if(pat==2&&s==15)mask|=1u<<7;
        }
        if(fill&&s>=14)mask|=1u<<(s==14?5:9);
        drum->drums=(uint16_t)mask;drum->velocity=(uint8_t)(s%4==0?108:s%2?64:80);drum->gate=50;
    }
}
void mc_demo_load(McEngine *e,unsigned demo) {
    if(demo>=MC_DEMOS)return;
    /* 아직 전송되지 않은 정리도 채널 소유 상태와 함께 다음 출력까지 보존한다. */
    uint16_t channels=e->midi.output_channels;uint8_t running=e->midi.output_running;
    mc_init(e);
    e->midi.output_channels=channels;e->midi.output_running=running;
    e->midi.cleanup=(uint8_t)(channels?3:0);e->midi.stop_pending=running;
    if(demo>=MC_DEMO_HIPHOP) { arranged_song(e,demo==MC_DEMO_TECHNO);return; }
    /* Both built-in demos use the active shared banks, not legacy master FX. */
    static const uint8_t sends[2][4][MC_FX_BANKS]={
        {{8,0,10,0},{52,28,38,0},{24,0,28,18},{10,0,0,0}},
        {{8,0,12,0},{40,18,44,0},{20,0,26,16},{8,0,0,0}}};
    static const uint8_t types[2][MC_FX_BANKS]={{3,0,7,6},{5,1,6,4}};
    /* Favor part 1's gentle pitch movement; keep recorded PCM attacks stable. */
    static const uint8_t analog[2][3]={{18,52,8},{24,60,6}};
    unsigned version=demo?1:0;
    /* R/D1 is reverb and R/D2 is delay: part 1 hears both throughout. */
    for(unsigned part=0;part<4;part++)for(unsigned bank=0;bank<MC_FX_BANKS;bank++)
        e->project.fx_send[part][bank]=sends[version][part][bank];
    for(unsigned bank=0;bank<MC_FX_BANKS;bank++)
        e->project.fx_bus_type[bank]=types[version][bank];
    for(unsigned part=0;part<3;part++)
        e->project.analog[part]=analog[version][part];
    if(!demo) {
        e->midi.receiving=1;mc_preset(e,2,MC_PRESET_STAGE_PIANO);e->midi.receiving=0;
        mc_midi_send(e,0xc0u+e->project.midi_channel[2],e->project.preset[2],0,2);
        /* piano 어택과 FM 배음, VA 몸통을 같은 hook으로 연주한다. */
        e->project.patch[2].pcm_mix=108;
        e->project.patch[2].cutoff=100;e->project.patch[2].resonance=8;
        e->project.level[2]=104;
        return;
    }
    for(unsigned i=0;i<sizeof e->project.step;i++) ((uint8_t *)e->project.step)[i]=0;
    e->project.bpm=140;e->project.swing=0;
    e->midi.receiving=1;
    mc_preset(e,0,MC_PRESET_PULSE_DRIVE);mc_preset(e,1,MC_PRESET_SLIDE_BASS);mc_preset(e,2,MC_PRESET_SOFT_MALLET);
    e->midi.receiving=0;
    for(unsigned part=0;part<3;part++)mc_midi_send(e,0xc0u+e->project.midi_channel[part],e->project.preset[part],0,2);
    e->project.patch[0].cutoff=65;e->project.patch[0].resonance=35;e->project.patch[0].decay=30;e->project.patch[0].sustain=45;
    e->project.patch[1].resonance=35;e->project.patch[1].glide=55;
    /* FM 단독 mallet에는 없는 PCM gain을 지정하지 않는다. */
    e->project.patch[2].resonance=8;
    e->project.level[0]=127;e->project.level[1]=100;e->project.level[2]=104;e->project.level[3]=75;
    e->project.drum_level[0]=115;e->project.drum_level[2]=70;e->project.drum_level[3]=64;e->project.drum_level[4]=85;
    static const uint8_t roots[4]={40,40,43,41};
    static const uint8_t motif[16]={0,12,7,15,12,7,19,12,0,7,12,15,19,15,7,12};
    for(unsigned p=0;p<4;p++) for(unsigned s=0;s<16;s++) {
        McStep *bass=&e->project.step[p][0][s],*acid=&e->project.step[p][1][s],*arp=&e->project.step[p][2][s],*drum=&e->project.step[p][3][s];
        bass->note[0]=(uint8_t)(roots[p]+(s==15?12:0));bass->velocity=(uint8_t)(s%4?100:0);bass->gate=42;
        acid->note[0]=(uint8_t)(roots[p]+12+motif[(s+p*2)%16]);acid->velocity=(uint8_t)(s%4==0?115:90);acid->gate=50;
        acid->flags=(uint8_t)((s%4==0?MC_ACCENT:0)|(s%8==5?MC_SLIDE:0));
        if(s%2==0||s==15) { arp->note[0]=(uint8_t)(roots[p]+24+motif[(s+4)%16]);arp->velocity=(uint8_t)(s%4==0?104:92);arp->gate=45; }
        drum->drums=(uint16_t)((s%4==0?1:0)|(s%2==1?4:0)|(s%4==2?8:0)|(s%8==4?16:0)|(s%4==3?(1u<<12):0)|(p==3&&s>=14?(1u<<9):0));
        drum->velocity=(uint8_t)(s%4==0?120:85);drum->gate=50;
    }
    e->project.demo=1;
    e->project.fx[0]=48;e->project.fx[1]=35;e->project.fx[2]=20;e->project.fx[3]=30;
}
void mc_demo_tick(McEngine *e) {
    if(!e->project.demo)return;
    unsigned bar=(e->events?e->events-1:0)/16;
    if(bar>=32) { mc_stop(e);return; }
    /* Expose the PCM hook in the intro before the acid joins at bar 4. */
    uint8_t mute=(uint8_t)(bar<4?2:bar<16?0:bar<20?13:bar<24?4:0);
    if(mute!=e->mute)for(unsigned p=0;p<4;p++)if((mute^e->mute)&(1u<<p))mc_mute(e,p);
    if(e->queued==255)mc_queue(e,(e->pattern+1)%4);
    if(e->demo_frame%64)return;
    unsigned section=bar<8?0:bar<16?1:bar<20?2:bar<24?3:4;
    unsigned span=MC_RATE*240u*4u/e->project.bpm;
    int rise=(int)((e->demo_frame%span)*90u/span);
    int cutoff=section==0?30+rise/2:section==1?35+rise:section==2?20+rise:section==3?95-rise/2:40+rise;
    int resonance=section==2?35+rise:section>=3?100:45;
    mc_control(e,1,0,cutoff-e->project.patch[1].cutoff);
    mc_control(e,1,1,resonance-e->project.patch[1].resonance);
    mc_control(e,2,0,88+rise/5-e->project.patch[2].cutoff);
}
