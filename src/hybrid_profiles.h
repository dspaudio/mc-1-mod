/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef MC_HYBRID_PROFILES_H
#define MC_HYBRID_PROFILES_H
#include "hybrid.h"
/* Immutable source recipes; factory.c owns editable mix, wave and common EG.
 * Family: 0 finger, 1 piano (3 zones), 2 muted, 3 nylon, 4 hit, 5 strings, 6 choir.
 * Columns: mask,family,ratio,index,feedback,filter,velocity-index,velocity-cutoff,
 * decay-ms,duration-ms,filter-depth,FM-body,VA-start,VA-end,VA-morph-ms,gain-Q8.
 * Q15 uses 32768 and Q8 uses 256 for unity. Gain is a fixed post-filter trim,
 * never per-file normalization. Finite FM is an onset, not a sustained layer. */
static const McHybridProfile mc_hybrid_profiles[MC_PRESETS] = {
    {6,HY_PCM_NONE,2,22,0,0,4,4,60,0,1800,32768,0,0,0,256}, /* Deep Sub */
    {4,HY_PCM_NONE,1,0,0,1,0,0,0,0,0,0,0,0,0,256}, /* Slide Bass */
    {6,HY_PCM_NONE,1,32,0,0,8,12,160,0,4500,32768,0,0,0,256}, /* Pulse Drive */
    {6,HY_PCM_NONE,1,46,0,0,8,10,160,0,4800,28672,0,0,0,256}, /* Solid FM */
    {7,0,2,32,0,0,12,16,240,0,7800,32768,0,0,0,256}, /* Rubber FM */
    {7,0,1,36,0,0,12,10,310,0,3200,32768,0,0,0,256}, /* Round Finger */
    {6,HY_PCM_NONE,1,36,0,0,8,18,120,0,6000,32768,16384,8192,420,256}, /* Reed Pulse */
    {6,HY_PCM_NONE,3,32,0,0,12,12,105,0,6500,28672,28672,16384,220,256}, /* Metal Seq */
    {7,1,1,10,0,0,8,12,120,280,2800,0,0,0,0,256}, /* Stage Piano */
    {5,1,1,0,0,0,0,10,0,0,-1800,0,0,0,0,256}, /* Soft Piano */
    {7,1,2,20,0,0,8,8,600,0,2200,4096,0,0,0,256}, /* Warm Tines */
    {7,1,1,18,0,0,8,12,420,0,3200,4096,0,0,0,256}, /* Clear EP */
    {5,2,1,0,0,0,0,18,0,0,11500,0,28672,4096,90,256}, /* Muted Gtr */
    {7,3,1,20,1,0,18,12,280,0,4800,8192,4096,20480,360,256}, /* Nylon Pick */
    {6,HY_PCM_NONE,3,30,1,0,16,10,210,0,5500,0,24576,8192,180,256}, /* Glass Pluck */
    {6,HY_PCM_NONE,2,24,1,0,12,8,110,0,2200,0,0,0,0,256}, /* Soft Mallet */
    {6,HY_PCM_NONE,1,12,0,0,0,0,100,0,0,32768,0,0,0,256}, /* Perc Organ */
    {6,HY_PCM_NONE,1,24,0,0,0,0,140,0,4000,32768,0,0,0,256}, /* Brass Stab */
    {4,HY_PCM_NONE,1,0,0,0,0,18,0,0,14500,0,28672,4096,160,256}, /* Chord Pluck */
    {7,4,1,8,0,0,0,0,100,150,0,0,0,0,0,256}, /* Orch Hit */
    {6,HY_PCM_NONE,2,22,1,0,16,12,40,75,4800,0,2048,20480,850,256}, /* Euro Saw */
    {4,HY_PCM_NONE,1,0,0,0,0,14,0,0,6500,0,24576,8192,1200,256}, /* Pulse Lead */
    {6,HY_PCM_NONE,1,18,1,0,16,10,420,0,4200,12288,8192,24576,1100,256}, /* Porta Lead */
    {6,HY_PCM_NONE,2,30,2,0,18,10,760,0,3200,12288,20480,4096,1600,256}, /* Bright FM */
    {6,HY_PCM_NONE,1,14,0,0,0,0,2400,0,800,16384,0,0,0,256}, /* Warm Pad */
    {5,5,1,0,0,0,0,8,0,0,3800,0,0,0,0,256}, /* Strings */
    {4,HY_PCM_NONE,1,0,0,0,0,0,0,0,1200,0,0,24576,3600,256}, /* Hollow Pad */
    {7,6,2,10,0,0,0,0,5000,0,0,8192,0,0,0,256}, /* Glass Pad */
    {4,HY_PCM_NONE,1,0,0,0,0,0,0,0,3000,0,0,0,0,448}, /* Pulse Arp */
    {7,2,3,24,1,0,20,16,90,180,8500,0,4096,16384,180,256}, /* Digital Arp */
    {6,HY_PCM_NONE,1,32,0,0,0,0,4500,0,-5000,4096,0,0,0,256}, /* Soft Sweep */
    {7,6,5,14,0,0,8,10,1800,0,-2400,4096,0,0,0,256}, /* Bell Bloom */
};
#endif
