/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef MC_HYBRID_PROFILES_H
#define MC_HYBRID_PROFILES_H
#include "hybrid.h"
/* Immutable source recipes; factory.c owns editable mix, wave and common EG.
 * Family: 0 finger, 1 piano (3 zones), 2 muted, 3 nylon, 4 hit, 5 strings, 6 choir.
 * Columns: mask,family,ratio,index,feedback,filter,velocity-index,velocity-cutoff,
 * decay-ms,duration-ms,filter-depth,FM-body,VA-start,VA-end,VA-morph-ms.
 * Q15 fields use 32768 for unity. Finite FM is an onset, not a sustained layer. */
static const McHybridProfile mc_hybrid_profiles[MC_PRESETS] = {
    {6,HY_PCM_NONE,2,18,0,0,8,4,90,0,1800,24576,0,0,0}, /* Deep Sub */
    {4,HY_PCM_NONE,1,0,0,1,0,0,0,0,0,0,0,0,0}, /* Slide Bass */
    {6,HY_PCM_NONE,3,14,0,0,8,12,160,0,4500,24576,0,0,0}, /* Pulse Drive */
    {6,HY_PCM_NONE,1,32,1,0,12,10,160,0,4800,24576,0,0,0}, /* Solid FM */
    {7,0,2,24,1,0,20,16,240,0,7800,24576,0,0,0}, /* Rubber FM */
    {7,0,1,24,0,0,12,10,310,0,3200,24576,0,0,0}, /* Round Finger */
    {4,HY_PCM_NONE,1,0,0,0,0,18,0,0,6000,0,16384,0,420}, /* Reed Pulse */
    {6,HY_PCM_NONE,3,18,0,0,10,12,105,0,6500,16384,28672,4096,220}, /* Metal Seq */
    {7,1,2,26,1,0,16,12,220,320,2800,0,2048,24576,1100}, /* Stage Piano */
    {5,1,1,0,0,0,0,10,0,0,-1800,0,12288,28672,2100}, /* Soft Piano */
    {7,1,2,24,1,0,18,8,720,0,2200,10240,8192,20480,1400}, /* Warm Tines */
    {7,1,3,28,2,0,14,12,480,0,3200,8192,24576,8192,650}, /* Clear EP */
    {5,2,1,0,0,0,0,18,0,0,11500,0,28672,4096,90}, /* Muted Gtr */
    {7,3,1,20,1,0,18,12,280,0,4800,8192,4096,20480,360}, /* Nylon Pick */
    {6,HY_PCM_NONE,3,30,1,0,16,10,210,0,5500,0,24576,8192,180}, /* Glass Pluck */
    {6,HY_PCM_NONE,2,24,1,0,12,8,110,0,2200,0,0,0,0}, /* Soft Mallet */
    {6,HY_PCM_NONE,2,12,0,0,8,10,100,0,3500,8192,12288,12288,0}, /* Perc Organ */
    {6,HY_PCM_NONE,1,12,0,0,8,16,140,0,15500,8192,4096,20480,210}, /* Brass Stab */
    {4,HY_PCM_NONE,1,0,0,0,0,18,0,0,14500,0,28672,4096,160}, /* Chord Pluck */
    {7,4,1,8,0,0,0,0,100,150,0,0,0,0,0}, /* Orch Hit */
    {6,HY_PCM_NONE,2,22,1,0,16,12,40,75,4800,0,2048,20480,850}, /* Euro Saw */
    {4,HY_PCM_NONE,1,0,0,0,0,14,0,0,6500,0,24576,8192,1200}, /* Pulse Lead */
    {6,HY_PCM_NONE,1,18,1,0,16,10,420,0,4200,12288,8192,24576,1100}, /* Porta Lead */
    {6,HY_PCM_NONE,2,30,2,0,18,10,760,0,3200,12288,20480,4096,1600}, /* Bright FM */
    {5,5,1,0,0,0,0,8,0,0,3200,0,0,8192,3000}, /* Warm Pad */
    {5,5,1,0,0,0,0,8,0,0,3800,0,8192,32768,4400}, /* Slow Strings */
    {4,HY_PCM_NONE,1,0,0,0,0,10,0,0,2400,0,8192,8192,0}, /* Hollow Pad */
    {7,6,1,8,0,0,4,8,1200,0,-2400,24576,0,0,0}, /* Glass Pad */
    {6,HY_PCM_NONE,1,6,0,0,4,0,28,40,2400,0,0,0,0}, /* Pulse Arp */
    {7,2,3,24,1,0,20,16,90,180,8500,0,4096,16384,180}, /* Digital Arp */
    {5,5,1,0,0,0,0,10,0,0,21000,0,0,16384,2200}, /* Soft Sweep */
    {7,6,4,26,1,0,14,10,2400,0,-2400,10240,0,0,0}, /* Bell Bloom */
};
#endif
