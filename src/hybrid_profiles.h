/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef MC_HYBRID_PROFILES_H
#define MC_HYBRID_PROFILES_H
#include "hybrid.h"
/* hybrid.c 한 TU에서 포함하는 불변 recipe. 기본 gain/EG는 factory.c 소유다.
 * family: 0 finger, 1 piano(3존), 2 muted, 3 nylon, 4 hit, 5 strings, 6 choir.
 * 어택 전용 FM은 유한 duration, 나머지는 변조 감쇠 뒤 carrier가 몸통을 잇는다. */
static const McHybridProfile mc_hybrid_profiles[MC_PRESETS] = {
    {4,HY_PCM_NONE,1,0,0,0,0,0,0,0,0}, /* Deep Sub */
    {4,HY_PCM_NONE,1,0,0,1,0,20,0,0,15000}, /* Slide Bass */
    {6,HY_PCM_NONE,2,42,8,0,26,12,28,48,6500}, /* Pulse Drive */
    {6,HY_PCM_NONE,1,64,12,0,30,8,105,0,3500}, /* Solid FM */
    {7,0,2,52,10,0,46,14,175,0,6500}, /* Rubber FM */
    {5,0,1,0,0,0,0,8,0,0,1800}, /* Round Finger */
    {4,HY_PCM_NONE,1,0,0,0,0,10,0,0,8000}, /* Reed Pulse */
    {6,HY_PCM_NONE,7,68,18,0,32,12,110,220,4500}, /* Metal Seq */
    {7,1,3,44,3,0,28,10,420,0,2200}, /* Stage Piano */
    {5,1,1,0,0,0,0,12,0,0,-2500}, /* Soft Piano */
    {7,1,2,30,2,0,26,8,560,0,1800}, /* Warm Tines */
    {3,1,4,54,6,0,34,10,310,0,1800}, /* Clear EP */
    {1,2,1,0,0,0,0,10,0,0,2500}, /* Muted Gtr */
    {3,3,1,18,2,0,16,8,220,0,1400}, /* Nylon Pick */
    {6,HY_PCM_NONE,3,62,5,0,24,8,190,0,3000}, /* Glass Pluck */
    {2,HY_PCM_NONE,2,26,2,0,18,8,82,0,1200}, /* Soft Mallet */
    {6,HY_PCM_NONE,4,35,4,0,20,8,32,65,2200}, /* Perc Organ */
    {6,HY_PCM_NONE,2,45,6,0,24,14,48,85,14000}, /* Brass Stab */
    {4,HY_PCM_NONE,1,0,0,0,0,14,0,0,13000}, /* Chord Pluck */
    {7,4,6,64,12,0,28,12,65,120,6000}, /* Orch Hit */
    {6,HY_PCM_NONE,2,38,4,0,22,8,42,75,3000}, /* Euro Saw */
    {4,HY_PCM_NONE,1,0,0,0,0,10,0,0,4500}, /* Pulse Lead */
    {6,HY_PCM_NONE,1,22,2,0,20,8,260,0,2800}, /* Porta Lead */
    {6,HY_PCM_NONE,3,68,8,0,30,8,620,0,1800}, /* Bright FM */
    {5,5,1,0,0,0,0,8,0,0,3600}, /* Warm Pad */
    {5,5,1,0,0,0,0,6,0,0,2000}, /* Slow Strings */
    {5,6,1,0,0,0,0,6,0,0,1400}, /* Air Choir */
    {7,6,5,46,3,0,18,8,1800,0,-4200}, /* Glass Pad */
    {4,HY_PCM_NONE,1,0,0,0,0,14,0,0,11000}, /* Pulse Arp */
    {3,2,5,54,4,0,26,10,96,0,2500}, /* Digital Arp */
    {5,5,1,0,0,0,0,8,0,0,18000}, /* Soft Sweep */
    {7,6,7,58,3,0,22,8,2200,0,-3200}, /* Bell Bloom */
};
#endif
