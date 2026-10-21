/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef MC_TABLES_H
#define MC_TABLES_H
#include <stdint.h>
extern const int16_t mc_sine[1024];
extern const int16_t mc_saw[7][256];
extern const int16_t mc_square[7][256];
extern const uint32_t mc_frequency[109];
#endif
