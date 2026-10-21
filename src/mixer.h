/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef MC_MOD_MIXER_H
#define MC_MOD_MIXER_H
#include "hybrid.h"
#include "mcfm.h"

typedef struct {
    McEngine base;
    HySynth synth;
    uint64_t drum_frames, bank_frames[MC_FX_BANKS];
    uint32_t peak_drums;
} PocMixer;

void poc_init(PocMixer *m);
void poc_drum(PocMixer *m,unsigned sound,unsigned velocity);
/* 기존 MC-1 드럼·route·FX·limiter와 새 혼합 멜로디를 연결하는 호스트 경로다. */
void poc_render(PocMixer *m,int16_t *stereo,unsigned frames);
#endif
