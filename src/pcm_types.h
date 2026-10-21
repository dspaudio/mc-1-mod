/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef MC_PCM_TYPES_H
#define MC_PCM_TYPES_H
#include <stdint.h>
#define MC_PCM_RATE 16000u
#define MC_PCM_FAMILIES 7u
#define MC_PCM_ZONES 9u
typedef struct { const int16_t *data; uint16_t length,loop_start,loop_end; } McPcmMip;
typedef struct { uint8_t family,root,levels; uint32_t tuning_hz_q16; const McPcmMip *mip; } McPcmZone;
#endif
