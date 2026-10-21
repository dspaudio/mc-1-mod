/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 dspaudio; 자체 수학적 PCM, 녹음/LA 뱅크 아님. */
#ifndef MC_PCM_BANK_H
#define MC_PCM_BANK_H
#include "pcm_types.h"
#include "pcm/zone_0.h"
#include "pcm/zone_1.h"
#include "pcm/zone_2.h"
#include "pcm/zone_3.h"
#include "pcm/zone_4.h"
#include "pcm/zone_5.h"
#include "pcm/zone_6.h"
#include "pcm/zone_7.h"
#include "pcm/zone_8.h"
static const McPcmZone mc_pcm_zones[MC_PCM_ZONES] = {
{0,36,7,4228129u,mc_pcm_z0_mips},
{1,48,7,8572947u,mc_pcm_z1_mips},
{1,60,7,17145893u,mc_pcm_z2_mips},
{1,72,7,34291786u,mc_pcm_z3_mips},
{2,60,7,17145893u,mc_pcm_z4_mips},
{3,60,7,17145893u,mc_pcm_z5_mips},
{4,60,7,17145893u,mc_pcm_z6_mips},
{5,60,7,17039360u,mc_pcm_z7_mips},
{6,60,7,17039360u,mc_pcm_z8_mips},
};
#endif
