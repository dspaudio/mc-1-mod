/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 dspaudio */
#include "mcfm.h"
/* Stable zero-based IDs: docs/SOUND_BANK_32.md and docs/SYNTH_BANK_64.md. */
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
    "Poly Brass",
    "Chorus Poly",
    "Punch Bass",
    "Saw Clav",
    "Digi Rubber",
    "FM Slap",
    "Tine Spark",
    "Crystal Key",
    "Velvet Poly",
    "Silk Poly",
    "Square Solo",
    "Bell Stack",
    "Rave Stab",
    "Hoover Bite",
    "Acid Pulse",
    "House Organ",
    "Wire Pluck",
    "Digital Vox",
    "Glass Orbit",
    "Warm Fifth",
    "Euro Lead",
    "Metal Sweep",
    "Trance Saw",
    "Anthem Lead",
    "Dance Pluck",
    "Electro Bass",
    "Reese Motion",
    "FM Growl",
    "Neon Arp",
    "Shimmer Pad",
    "Pulse Dream",
    "Future Bell",
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
    "80S SYNTH",
    "80S SYNTH",
    "80S SYNTH",
    "80S SYNTH",
    "80S SYNTH",
    "80S SYNTH",
    "80S SYNTH",
    "80S SYNTH",
    "80S SYNTH",
    "80S SYNTH",
    "80S SYNTH",
    "80S SYNTH",
    "90S SYNTH",
    "90S SYNTH",
    "90S SYNTH",
    "90S SYNTH",
    "90S SYNTH",
    "90S SYNTH",
    "90S SYNTH",
    "90S SYNTH",
    "90S SYNTH",
    "90S SYNTH",
    "00S SYNTH",
    "00S SYNTH",
    "00S SYNTH",
    "00S SYNTH",
    "00S SYNTH",
    "00S SYNTH",
    "00S SYNTH",
    "00S SYNTH",
    "00S SYNTH",
    "00S SYNTH",
};
/* mode,wave,profile,slot1,CUT,RES,A,D,S,R,glide,level,FM,VA,mask.
 * Slot 1 remains PCM for IDs 0..31; it is AUX gain for IDs 32..63. */
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
    {0,0,32,0,76,10,10,70,100,30,0,96,64,112,6}, /* Poly Brass */
    {0,0,33,76,94,6,8,86,108,42,0,88,36,100,7}, /* Chorus Poly */
    {0,2,34,0,88,12,0,46,64,12,0,104,112,86,6}, /* Punch Bass */
    {0,0,35,0,92,16,0,34,0,14,0,100,72,112,6}, /* Saw Clav */
    {0,1,36,0,70,26,1,64,56,18,0,98,100,82,6}, /* Digi Rubber */
    {0,2,37,88,96,8,0,52,20,16,0,98,100,64,7}, /* FM Slap */
    {0,2,38,0,112,4,1,96,0,42,0,100,110,42,6}, /* Tine Spark */
    {0,2,39,60,118,4,1,94,0,48,0,90,92,36,7}, /* Crystal Key */
    {0,1,40,64,78,4,36,108,104,66,0,88,42,96,7}, /* Velvet Poly */
    {0,0,41,70,88,5,48,112,110,72,0,86,38,90,7}, /* Silk Poly */
    {1,1,42,0,96,18,2,76,108,24,38,100,62,108,6}, /* Square Solo */
    {0,2,43,58,116,3,0,110,0,66,0,90,96,32,7}, /* Bell Stack */
    {0,0,44,66,90,18,0,44,0,20,0,90,70,110,7}, /* Rave Stab */
    {1,0,45,78,100,20,2,80,104,30,22,86,70,102,7}, /* Hoover Bite */
    {1,1,46,0,82,68,0,42,68,12,52,96,48,116,6}, /* Acid Pulse */
    {0,2,47,70,106,2,0,50,120,18,0,92,94,78,7}, /* House Organ */
    {0,1,48,0,102,20,0,36,0,18,0,98,106,72,6}, /* Wire Pluck */
    {0,1,49,62,88,14,18,92,96,42,0,86,88,72,7}, /* Digital Vox */
    {0,2,50,0,116,6,12,112,66,72,0,94,108,36,6}, /* Glass Orbit */
    {0,0,51,68,82,6,24,104,106,58,0,88,44,96,7}, /* Warm Fifth */
    {1,0,52,0,106,16,1,72,108,26,34,100,76,104,6}, /* Euro Lead */
    {0,0,53,0,108,28,58,118,96,68,0,88,100,68,6}, /* Metal Sweep */
    {0,0,54,88,110,12,2,82,112,40,0,88,32,104,7}, /* Trance Saw */
    {0,1,55,0,112,12,1,78,112,34,0,98,88,100,6}, /* Anthem Lead */
    {0,0,56,58,86,20,0,40,0,26,0,92,96,84,7}, /* Dance Pluck */
    {1,1,57,0,78,22,0,54,84,14,18,100,112,88,6}, /* Electro Bass */
    {1,0,58,92,86,14,12,108,112,36,28,86,46,104,7}, /* Reese Motion */
    {1,1,59,76,84,26,1,70,96,24,26,88,106,64,7}, /* FM Growl */
    {0,1,60,0,74,24,0,26,0,12,0,104,80,110,6}, /* Neon Arp */
    {0,2,61,62,108,3,62,118,112,86,0,88,76,48,7}, /* Shimmer Pad */
    {0,1,62,0,86,8,54,114,108,78,0,94,60,100,6}, /* Pulse Dream */
    {0,2,63,0,120,8,0,102,0,58,0,96,116,36,6}, /* Future Bell */
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
    {22,20,12,0}, /* Poly Brass */
    {26,30,18,0}, /* Chorus Poly */
    {6,0,4,0}, /* Punch Bass */
    {14,8,18,0}, /* Saw Clav */
    {8,4,10,4}, /* Digi Rubber */
    {8,6,4,0}, /* FM Slap */
    {28,16,22,0}, /* Tine Spark */
    {34,14,28,0}, /* Crystal Key */
    {38,28,16,4}, /* Velvet Poly */
    {40,30,20,4}, /* Silk Poly */
    {20,16,24,4}, /* Square Solo */
    {34,10,30,0}, /* Bell Stack */
    {18,14,24,4}, /* Rave Stab */
    {16,16,20,12}, /* Hoover Bite */
    {8,0,20,4}, /* Acid Pulse */
    {14,20,12,0}, /* House Organ */
    {18,6,26,4}, /* Wire Pluck */
    {28,20,24,8}, /* Digital Vox */
    {38,16,34,8}, /* Glass Orbit */
    {32,24,18,4}, /* Warm Fifth */
    {22,22,30,4}, /* Euro Lead */
    {34,12,28,18}, /* Metal Sweep */
    {28,30,30,6}, /* Trance Saw */
    {26,22,32,4}, /* Anthem Lead */
    {22,12,32,0}, /* Dance Pluck */
    {6,4,8,4}, /* Electro Bass */
    {12,14,8,10}, /* Reese Motion */
    {8,4,12,8}, /* FM Growl */
    {14,10,28,0}, /* Neon Arp */
    {44,24,34,8}, /* Shimmer Pad */
    {40,28,28,10}, /* Pulse Dream */
    {30,10,34,4}, /* Future Bell */
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
