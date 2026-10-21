/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 dspaudio */
#include "mcfm.h"
/* 순서는 docs/SOUND_BANK_32.md의 ID와 같다. */
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
    "Slow Strings",
    "Air Choir",
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
    {0,2,0,0,112,0,1,68,110,20,0,112,0,127,4}, /* Deep Sub */
    {1,0,1,0,52,82,1,48,46,12,58,98,0,127,4}, /* Slide Bass */
    {0,1,2,0,92,18,1,46,72,14,0,100,42,116,6}, /* Pulse Drive */
    {0,2,3,0,112,6,1,38,68,18,0,110,112,40,6}, /* Solid FM */
    {0,2,4,22,102,9,2,58,76,22,0,108,102,48,6}, /* Rubber FM */
    {0,2,5,120,114,4,1,70,66,24,0,112,0,40,5}, /* Round Finger */
    {0,1,6,0,80,12,2,34,56,12,0,104,0,127,4}, /* Reed Pulse */
    {0,0,7,0,112,12,0,32,20,12,0,100,106,38,6}, /* Metal Seq */
    {0,0,8,112,122,4,1,88,48,44,0,108,58,34,7}, /* Stage Piano */
    {0,0,9,92,101,2,3,104,76,60,0,106,0,72,5}, /* Soft Piano */
    {0,2,10,20,112,2,2,92,56,46,0,112,110,42,6}, /* Warm Tines */
    {0,2,11,64,125,3,0,74,34,34,0,108,118,0,3}, /* Clear EP */
    {0,2,12,127,121,3,0,26,0,10,0,120,0,0,1}, /* Muted Gtr */
    {0,2,13,112,119,2,1,68,20,30,0,112,38,0,3}, /* Nylon Pick */
    {0,1,14,0,122,4,0,44,0,20,0,108,116,30,6}, /* Glass Pluck */
    {0,2,15,0,115,0,1,56,0,24,0,120,127,0,2}, /* Soft Mallet */
    {0,1,16,0,113,5,0,24,102,12,0,104,36,114,6}, /* Perc Organ */
    {0,0,17,0,82,22,3,42,24,16,0,100,36,118,6}, /* Brass Stab */
    {0,0,18,0,88,14,1,38,0,18,0,108,0,127,4}, /* Chord Pluck */
    {0,0,19,115,120,6,0,45,0,22,0,96,40,46,7}, /* Orch Hit */
    {0,0,20,0,119,12,2,68,96,28,0,98,32,120,6}, /* Euro Saw */
    {0,1,21,0,113,10,1,60,100,24,0,104,0,127,4}, /* Pulse Lead */
    {1,0,22,0,96,7,4,76,106,30,72,102,30,114,6}, /* Porta Lead */
    {0,0,23,0,124,5,1,66,86,26,0,104,116,30,6}, /* Bright FM */
    {0,0,24,36,89,5,55,100,101,65,0,100,0,108,5}, /* Warm Pad */
    {0,0,25,112,106,3,74,112,110,74,0,102,0,42,5}, /* Slow Strings */
    {0,2,26,112,118,2,48,106,104,66,0,110,0,38,5}, /* Air Choir */
    {0,2,27,38,120,3,42,112,88,68,0,106,104,30,7}, /* Glass Pad */
    {0,1,28,0,94,16,0,28,0,10,0,108,0,127,4}, /* Pulse Arp */
    {0,2,29,32,126,2,0,36,0,14,0,114,120,0,3}, /* Digital Arp */
    {0,0,30,32,62,18,88,118,100,68,0,98,0,112,5}, /* Soft Sweep */
    {0,2,31,24,122,2,5,108,62,62,0,110,114,34,6}, /* Bell Bloom */
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
    { /* subtle shared ROOM2 reverb on new projects only; stored projects keep their own sends */
        static const uint8_t reverb_send[MC_PARTS]={12,72,24,8};
        p->fx_bus_type[0]=1;
        for(unsigned t=0;t<MC_PARTS;t++) { p->fx_send[t][0]=reverb_send[t]; p->fx_send[t][1]=0; }
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
