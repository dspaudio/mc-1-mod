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
    /* PCM-free expansion. Mask value 1 selects AUX for IDs 32..63. */
    {6,HY_PCM_NONE,1,18,0,0,8,12,180,0,7200,16384,24576,8192,300,256}, /* Poly Brass */
    {7,HY_PCM_NONE,2,16,0,0,10,8,45,90,2800,0,4096,16384,900,256}, /* Chorus Poly */
    {6,HY_PCM_NONE,1,58,2,0,16,16,85,0,9200,8192,0,0,0,256}, /* Punch Bass */
    {6,HY_PCM_NONE,4,34,1,0,18,18,75,130,12000,0,28672,4096,100,256}, /* Saw Clav */
    {6,HY_PCM_NONE,2,68,3,0,22,20,260,0,10500,4096,4096,24576,240,256}, /* Digi Rubber */
    {7,HY_PCM_NONE,1,52,2,0,20,14,90,160,6500,0,0,0,0,256}, /* FM Slap */
    {6,HY_PCM_NONE,3,42,0,0,14,10,1100,0,1800,2048,0,0,0,256}, /* Tine Spark */
    {7,HY_PCM_NONE,3,36,1,0,12,8,520,0,2600,4096,0,0,0,256}, /* Crystal Key */
    {7,HY_PCM_NONE,1,10,0,0,4,6,1800,0,1400,8192,4096,12288,2400,256}, /* Velvet Poly */
    {7,HY_PCM_NONE,3,12,0,0,4,8,2400,0,2000,4096,16384,4096,3200,256}, /* Silk Poly */
    {6,HY_PCM_NONE,1,28,1,0,12,12,360,0,4200,20480,24576,8192,650,256}, /* Square Solo */
    {7,HY_PCM_NONE,5,44,1,0,14,8,2200,0,1200,4096,0,0,0,256}, /* Bell Stack */
    {7,HY_PCM_NONE,3,38,2,0,16,18,50,110,14500,0,24576,4096,140,256}, /* Rave Stab */
    {7,HY_PCM_NONE,1,64,5,0,12,12,480,0,5800,24576,0,24576,700,256}, /* Hoover Bite */
    {6,HY_PCM_NONE,1,22,1,HY_FILTER_ACID4,12,0,120,0,0,16384,8192,24576,180,256}, /* Acid Pulse */
    {7,HY_PCM_NONE,1,8,0,0,0,0,100,0,0,32768,0,0,0,256}, /* House Organ */
    {6,HY_PCM_NONE,6,58,3,0,24,18,95,0,9800,0,28672,4096,120,256}, /* Wire Pluck */
    {7,HY_PCM_NONE,2,40,4,0,12,10,700,0,3600,16384,4096,28672,1800,256}, /* Digital Vox */
    {6,HY_PCM_NONE,7,26,0,0,10,6,2600,0,-1800,2048,0,0,0,256}, /* Glass Orbit */
    {7,HY_PCM_NONE,1,14,0,0,6,6,1500,0,2400,16384,16384,4096,2200,256}, /* Warm Fifth */
    {6,HY_PCM_NONE,2,32,2,0,18,16,160,0,6400,12288,8192,24576,400,256}, /* Euro Lead */
    {6,HY_PCM_NONE,8,62,6,0,16,14,3400,0,-9000,8192,28672,4096,2800,256}, /* Metal Sweep */
    {7,HY_PCM_NONE,2,20,1,0,14,12,40,75,3800,0,8192,20480,1100,256}, /* Trance Saw */
    {6,HY_PCM_NONE,1,38,2,0,16,14,280,0,5200,20480,28672,12288,850,256}, /* Anthem Lead */
    {7,HY_PCM_NONE,2,48,1,0,20,16,125,0,11500,0,24576,4096,180,256}, /* Dance Pluck */
    {6,HY_PCM_NONE,1,76,5,0,20,20,140,0,8800,16384,28672,8192,260,256}, /* Electro Bass */
    {7,HY_PCM_NONE,1,26,2,0,8,10,1800,0,-3200,24576,4096,24576,2600,256}, /* Reese Motion */
    {7,HY_PCM_NONE,3,82,8,0,24,18,420,0,7600,16384,24576,8192,600,256}, /* FM Growl */
    {6,HY_PCM_NONE,4,30,0,0,16,20,70,0,13200,0,24576,4096,90,256}, /* Neon Arp */
    {7,HY_PCM_NONE,2,16,0,0,6,6,4200,0,-1200,4096,0,0,0,256}, /* Shimmer Pad */
    {6,HY_PCM_NONE,1,20,0,0,6,8,3000,0,-4200,8192,0,28672,4800,256}, /* Pulse Dream */
    {6,HY_PCM_NONE,6,50,2,0,18,10,1650,0,3200,1024,0,0,0,256}, /* Future Bell */
};
#endif
