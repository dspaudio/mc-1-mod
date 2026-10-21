/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 dspaudio */
#include "mcfm.h"
/* Stable order matches the zero-based IDs in docs/SOUND_BANK_32.md. */
const char *const mc_preset_names[MC_PRESETS] = {
    "Deep Sub",
    "Slide Bass",
    "Pulse Drive",
    "Solid FM",
    "Rubber FM",
    "Round Finger",
    "Reed Pulse",
    "Metal Seq",
    "Stage Piano",
    "Soft Piano",
    "Warm Tines",
    "Clear EP",
    "Muted Gtr",
    "Nylon Pick",
    "Glass Pluck",
    "Soft Mallet",
    "Perc Organ",
    "Brass Stab",
    "Chord Pluck",
    "Orch Hit",
    "Euro Saw",
    "Pulse Lead",
    "Porta Lead",
    "Bright FM",
    "Warm Pad",
    "Strings",
    "Hollow Pad",
    "Glass Pad",
    "Pulse Arp",
    "Digital Arp",
    "Soft Sweep",
    "Bell Bloom",
};
const char *const mc_preset_categories[MC_PRESETS] = {
    "BASS CORE",
    "BASS CORE",
    "BASS CORE",
    "BASS CORE",
    "BASS SEQ",
    "BASS SEQ",
    "BASS SEQ",
    "BASS SEQ",
    "PIANO EP",
    "PIANO EP",
    "PIANO EP",
    "PIANO EP",
    "PLUCK MALLET",
    "PLUCK MALLET",
    "PLUCK MALLET",
    "PLUCK MALLET",
    "COMP STAB",
    "COMP STAB",
    "COMP STAB",
    "COMP STAB",
    "LEAD",
    "LEAD",
    "LEAD",
    "LEAD",
    "PAD TEXTURE",
    "PAD TEXTURE",
    "PAD TEXTURE",
    "PAD TEXTURE",
    "MOTION",
    "MOTION",
    "MOTION",
    "MOTION",
};
/* mode,wave,profile,PCM,CUT,RES,A,D,S,R,glide,level,FM,VA,mask */
const McPatch mc_presets[MC_PRESETS] = {
    {0,2,0,0,108,3,1,56,112,18,0,108,127,127,6}, /* Deep Sub */
    {1,0,1,0,104,42,1,56,116,10,46,112,0,127,4}, /* Slide Bass */
    {0,1,2,0,104,8,1,60,112,12,0,108,112,127,6}, /* Pulse Drive */
    {0,2,3,0,104,8,1,60,112,18,0,112,127,96,6}, /* Solid FM */
    {0,2,4,28,98,12,1,76,112,20,0,108,127,96,6}, /* Rubber FM */
    {0,2,5,90,98,8,1,80,112,20,0,112,112,92,7}, /* Round Finger */
    {0,1,6,0,98,12,1,60,112,12,0,112,90,116,6}, /* Reed Pulse */
    {0,0,7,0,96,12,1,56,112,12,0,108,112,116,6}, /* Metal Seq */
    {0,2,8,127,112,6,1,88,96,42,0,100,24,24,3}, /* Stage Piano */
    {0,2,9,127,96,4,4,106,96,58,0,100,0,24,1}, /* Soft Piano */
    {0,2,10,104,104,5,2,100,0,40,0,104,64,20,3}, /* Warm Tines */
    {0,2,11,104,116,6,1,96,0,34,0,98,72,20,3}, /* Clear EP */
    {0,1,12,112,96,18,0,30,0,12,0,108,0,64,5}, /* Muted Gtr */
    {0,0,13,100,101,8,1,70,18,28,0,100,48,58,7}, /* Nylon Pick */
    {0,1,14,0,108,10,0,48,0,22,0,104,90,78,6}, /* Glass Pluck */
    {0,2,15,0,102,3,1,62,0,24,0,112,112,40,6}, /* Soft Mallet */
    {0,2,16,0,88,2,40,64,124,24,0,104,96,127,6}, /* Perc Organ */
    {0,2,17,0,64,4,44,80,120,30,0,104,96,127,6}, /* Brass Stab */
    {0,0,18,0,80,22,0,44,0,20,0,108,0,127,4}, /* Chord Pluck */
    {0,0,19,127,120,0,0,112,0,36,0,108,32,32,1}, /* Orch Hit */
    {0,0,20,0,108,18,1,74,102,28,0,102,32,124,6}, /* Euro Saw */
    {0,1,21,0,102,16,1,64,104,26,0,106,0,127,4}, /* Pulse Lead */
    {1,0,22,0,94,10,3,82,110,32,64,104,52,108,6}, /* Porta Lead */
    {0,0,23,0,112,8,1,78,94,28,0,102,100,54,6}, /* Bright FM */
    {0,2,24,0,90,2,48,112,112,72,0,98,32,50,6}, /* Warm Pad */
    {0,2,25,96,96,5,4,116,114,36,0,96,0,24,1}, /* Strings */
    {0,1,26,0,90,4,52,112,114,72,0,104,0,127,4}, /* Hollow Pad */
    {0,2,27,48,104,0,52,116,114,76,0,100,96,80,6}, /* Glass Pad */
    {0,1,28,0,48,24,10,18,0,12,0,127,0,127,4}, /* Pulse Arp */
    {0,0,29,72,108,12,0,42,0,16,0,100,88,80,7}, /* Digital Arp */
    {0,2,30,0,90,8,64,120,110,70,0,96,50,40,6}, /* Soft Sweep */
    {0,2,31,40,114,5,3,112,78,60,0,102,72,96,6}, /* Bell Bloom */
};
/* One shared factory palette: HALL1, CHORUS2, PANNING DLY, FLANGER.
 * Program selection consumes only the send row, never these global types. */
const uint8_t mc_factory_fx_types[MC_FX_BANKS]={3,1,7,5};
const uint8_t mc_preset_fx_send[MC_PRESETS][MC_FX_BANKS] = {
    {4,0,0,0}, /* Deep Sub */
    {6,0,14,4}, /* Slide Bass */
    {6,4,8,0}, /* Pulse Drive */
    {8,4,0,0}, /* Solid FM */
    {10,6,8,0}, /* Rubber FM */
    {8,6,4,0}, /* Round Finger */
    {10,8,14,4}, /* Reed Pulse */
    {12,4,18,6}, /* Metal Seq */
    {30,22,12,0}, /* Stage Piano */
    {38,30,10,4}, /* Soft Piano */
    {26,22,14,0}, /* Warm Tines */
    {24,16,20,4}, /* Clear EP */
    {12,6,16,0}, /* Muted Gtr */
    {22,12,18,0}, /* Nylon Pick */
    {28,12,26,4}, /* Glass Pluck */
    {20,8,14,0}, /* Soft Mallet */
    {18,28,8,4}, /* Perc Organ */
    {20,16,14,4}, /* Brass Stab */
    {22,18,22,4}, /* Chord Pluck */
    {30,12,16,0}, /* Orch Hit */
    {24,28,24,6}, /* Euro Saw */
    {22,20,28,8}, /* Pulse Lead */
    {28,26,22,4}, /* Porta Lead */
    {24,18,26,8}, /* Bright FM */
    {40,34,18,10}, /* Warm Pad */
    {44,38,16,8}, /* Slow Strings */
    {44,32,22,12}, /* Hollow Pad */
    {40,28,28,12}, /* Glass Pad */
    {12,12,24,4}, /* Pulse Arp */
    {16,10,28,6}, /* Digital Arp */
    {44,30,30,16}, /* Soft Sweep */
    {38,24,32,8}, /* Bell Bloom */
};
static void clear_bytes(void *p, unsigned n) { uint8_t *b=p; while(n--) *b++=0; }
void mc_factory(McProject *p)
{
    static const uint8_t roots[4]={45,41,48,43};
    static const uint8_t melody[16]={0,0,7,0,12,10,0,7,3,0,7,10,12,0,15,10};
    clear_bytes(p,sizeof *p);
    for(unsigned d=0;d<MC_DRUMS;d++) p->drum_level[d]=100;
    p->bpm=118; p->swing=10;
    p->patch[0]=mc_presets[MC_PRESET_DEEP_SUB]; p->patch[1]=mc_presets[MC_PRESET_WARM_TINES]; p->patch[2]=mc_presets[MC_PRESET_EURO_SAW];
    mc_project_defaults(p);
    { /* Defaults above clear sends/types; explicit factory loads set them here. */
        static const uint8_t drum_send[MC_FX_BANKS]={12,0,6,0};
        for(unsigned bank=0;bank<MC_FX_BANKS;bank++) {
            p->fx_bus_type[bank]=mc_factory_fx_types[bank];
            for(unsigned part=0;part<3;part++)
                p->fx_send[part][bank]=mc_preset_fx_send[p->preset[part]][bank];
            p->fx_send[3][bank]=drum_send[bank];
        }
    }
    for(unsigned t=0;t<4;t++) { p->level[t]=t==1?75:100; p->pan[t]=t==1?42:t==2?88:64; }
    for(unsigned b=0;b<4;b++) for(unsigned s=0;s<16;s++) {
        McStep *bass=&p->step[b][0][s], *pad=&p->step[b][1][s];
        McStep *lead=&p->step[b][2][s], *drum=&p->step[b][3][s];
        bass->note[0]=(uint8_t)(roots[b]+((s==6||s==14)?12:0));
        bass->velocity=(s%4==0)?115:85; bass->gate=65;
        bass->flags=(s%4==0?MC_ACCENT:0)|(s==5||s==13?MC_SLIDE:0);
        if(s==3||s==11) bass->velocity=0;
        if(s%8==0) {
            pad->note[0]=roots[b]+12; pad->note[1]=roots[b]+15; pad->note[2]=roots[b]+19;
            pad->velocity=76; pad->gate=100; /* long pad release bridges rests */
        }
        if(s%2==0||s==7||s==15) {
            lead->note[0]=(uint8_t)(roots[b]+24+melody[s]); lead->velocity=(uint8_t)(80+(s%3)*10); lead->gate=55;
        }
        drum->drums=(s%4==0?1:0)|(s%8==4?2:0)|(s%2==0?4:0)|(s==14?8:0)|(s==12?16:0)|(b==3&&s==15?32:0);
        drum->velocity=s%4==0?112:85; drum->gate=50;
    }
}
