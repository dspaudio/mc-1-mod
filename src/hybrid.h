/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef MC_MOD_HYBRID_H
#define MC_MOD_HYBRID_H
#include "mcfm.h"
#include "pcm_types.h"

enum { HY_SLOT1=1, HY_PCM=HY_SLOT1, HY_FM=2, HY_VA=4, HY_ALL=7 };
enum { HY_FILTER_2POLE, HY_FILTER_ACID4 };
enum { HY_PCM_NONE=255 };

/* 불변 recipe다. 기본 mixㆍADSRㆍwave는 mc_presets의 McPatch가 소유한다.
 * PCM family는 0..6 또는 HY_PCM_NONE, ratio는 정수 1..8이다.
 * index/feedback/velocity는 0..127, 시간은 0..10000ms다.
 * duration=0은 무제한이며 filter_depth는 signed Q15다. */
typedef struct {
    uint8_t available_mask, pcm_family, fm_ratio, fm_index, fm_feedback;
    uint8_t filter_kind, velocity_index, velocity_cutoff;
    uint16_t fm_decay_ms, fm_duration_ms;
    int16_t filter_depth;
    uint16_t fm_body_q15, va_colour_start_q15, va_colour_end_q15, va_morph_ms;
    uint16_t gain_q8;
} McHybridProfile;

/* Slot 1 is PCM for the original profiles, or one typed auxiliary oscillator.
 * Tuning is a Q16 multiplier of the shared pitch; colour and FM body are Q15. */
typedef struct {
    uint32_t tuning_q16;
    uint16_t colour_q15, fm_decay_ms, fm_body_q15;
    uint8_t kind, wave, fm_ratio, fm_index, fm_feedback;
} McAuxProfile;

/* slice 안에서만 사용하는 준비 상태다. 직렬화하거나 worker에 포인터를 전송하지 않는다.
 * increment는 glide/bend/Analog가 한 번 적용된 공통 pitch다.
 * pcm_step은 같은 pitch와 zone->tuning_hz_q16에서 산출한 선택 mip의 Q16 step이다.
 * pcm은 zone->mip의 선택 레벨이며 table은 실제 pitch에 맞는 VA 테이블이다. */
typedef struct {
    const McHybridProfile *profile;
    const McAuxProfile *aux;
    const McPcmZone *zone;
    const McPcmMip *pcm;
    const int16_t *table;
    const int16_t *table_colour;
    const int16_t *aux_table, *aux_table_colour;
    McPcmCache *cache;
    McPitchCache *pitch;
    McAnalogVoice *analog;
    uint32_t ratio, increment, pcm_step;
    uint32_t aux_increment;
    uint32_t analog_increment, analog_ratio;
    uint32_t fm_decay_frames, va_morph_frames;
    int32_t cutoff, sustain, feedback, analog_adjustment;
    uint8_t fixed_pitch, refresh, analog_ready, analog_amount, amount;
} McVoiceRender;

/* 범위 밖 ID는 NULL이다. descriptor/data는 hybrid.c 한 TU에서만 포함한다. */
const McHybridProfile *hy_profile(unsigned profile);
const McAuxProfile *hy_aux_profile(unsigned profile);
/* Source is a slot index (0..2), not a source-mask bit. */
const char *hy_source_name(unsigned profile,unsigned source);
const McPcmZone *hy_pcm_zone(unsigned zone);
/* 새 note의 소스 상태만 초기화한다. mono slide에서는 호출하지 않는다. */
void hy_source_init(McVoice *voice);
/* raw mono 합 /381을 반환한다. 공통 gateㆍpitch 진행ㆍ최종 EGㆍ필터는 호출자 소유다. */
int32_t hy_source_sample(McVoice *voice,McVoiceRender *render);
#endif
