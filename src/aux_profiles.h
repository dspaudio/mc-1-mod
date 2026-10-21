/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef MC_AUX_PROFILES_H
#define MC_AUX_PROFILES_H
#include "hybrid.h"
_Static_assert(sizeof(McAuxProfile)==16,"McAuxProfile ABI");
/* IDs 32..63, indexed by ID - 32. Zero kind means no auxiliary.
 * Columns: tuning-Q16,colour-Q15,FM-decay-ms,FM-body-Q15,
 * kind,wave,FM-ratio,FM-index,FM-feedback.
 * FM decay changes modulation, never carrier amplitude or lifetime.
 * VA colour is static; each VA2 has its own phase and tuning. */
static const McAuxProfile mc_aux_profiles[32] = {
    {0,0,0,0,0,0,0,0,0}, /* 32 Poly Brass */
    {65800,8192,0,0,HY_VA,MC_WAVE_SAW,0,0,0}, /* 33 Chorus Poly */
    {0,0,0,0,0,0,0,0,0}, /* 34 Punch Bass */
    {0,0,0,0,0,0,0,0,0}, /* 35 Saw Clav */
    {0,0,0,0,0,0,0,0,0}, /* 36 Digi Rubber */
    {32768,0,320,32768,HY_FM,0,2,12,0}, /* 37 FM Slap */
    {0,0,0,0,0,0,0,0,0}, /* 38 Tine Spark */
    {131072,0,1600,0,HY_FM,0,7,24,0}, /* 39 Crystal Key */
    {32768,0,0,0,HY_VA,MC_WAVE_SINE,0,0,0}, /* 40 Velvet Poly */
    {65272,12288,0,0,HY_VA,MC_WAVE_SQUARE,0,0,0}, /* 41 Silk Poly */
    {0,0,0,0,0,0,0,0,0}, /* 42 Square Solo */
    {98304,0,900,8192,HY_FM,0,2,20,1}, /* 43 Bell Stack */
    {131072,16384,0,0,HY_VA,MC_WAVE_SQUARE,0,0,0}, /* 44 Rave Stab */
    {32637,24576,0,0,HY_VA,MC_WAVE_SAW,0,0,0}, /* 45 Hoover Bite */
    {0,0,0,0,0,0,0,0,0}, /* 46 Acid Pulse */
    {131072,0,100,32768,HY_FM,0,2,12,0}, /* 47 House Organ */
    {0,0,0,0,0,0,0,0,0}, /* 48 Wire Pluck */
    {49152,0,1400,12288,HY_FM,0,3,28,2}, /* 49 Digital Vox */
    {0,0,0,0,0,0,0,0,0}, /* 50 Glass Orbit */
    {98304,0,0,0,HY_VA,MC_WAVE_SINE,0,0,0}, /* 51 Warm Fifth */
    {0,0,0,0,0,0,0,0,0}, /* 52 Euro Lead */
    {0,0,0,0,0,0,0,0,0}, /* 53 Metal Sweep */
    {66298,12288,0,0,HY_VA,MC_WAVE_SAW,0,0,0}, /* 54 Trance Saw */
    {0,0,0,0,0,0,0,0,0}, /* 55 Anthem Lead */
    {131072,0,360,4096,HY_FM,0,4,22,1}, /* 56 Dance Pluck */
    {0,0,0,0,0,0,0,0,0}, /* 57 Electro Bass */
    {65014,16384,0,0,HY_VA,MC_WAVE_SAW,0,0,0}, /* 58 Reese Motion */
    {32768,0,1200,24576,HY_FM,0,1,44,4}, /* 59 FM Growl */
    {0,0,0,0,0,0,0,0,0}, /* 60 Neon Arp */
    {131072,0,6000,2048,HY_FM,0,5,18,0}, /* 61 Shimmer Pad */
    {0,0,0,0,0,0,0,0,0}, /* 62 Pulse Dream */
    {0,0,0,0,0,0,0,0,0}, /* 63 Future Bell */
};
#endif
