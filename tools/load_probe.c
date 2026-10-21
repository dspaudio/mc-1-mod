/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#ifndef LOAD_PROBE_NEW
#define LOAD_PROBE_NEW 1
#endif
#ifndef LOAD_PROBE_FIXTURE
#define LOAD_PROBE_FIXTURE "../firmware/perf_fixture.h"
#endif
#include "mcfm.h"
#include "parallel.h"
#include LOAD_PROBE_FIXTURE
#if LOAD_PROBE_NEW
#include "hybrid.h"
#include "tables.h"
#endif
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

enum { BLOCK = 128, WARMUP = 1024, BLOCKS = 8192 };
_Static_assert(MC_VOICES == 8 && MC_DRUM_VOICES == 6 && MC_FX_BANKS == 4,
    "probe workload");
static McEngine engine;
static int16_t audio[BLOCK * 2];

static uint64_t ns(clockid_t clock_id) {
    struct timespec t;
    if (clock_gettime(clock_id, &t)) {
        perror("clock_gettime");
        exit(2);
    }
    return (uint64_t)t.tv_sec * UINT64_C(1000000000) + (uint64_t)t.tv_nsec;
}

/* 마스크는 실제 gain과 종료 상태를 반영하며 음량이나 명령 수가 아니다. */
#if LOAD_PROBE_NEW
static unsigned sources(const McVoice *v) {
    unsigned mask = 0;
    for (unsigned i = 0; i < 3; i++)
        if (v->hybrid.gain[i] && !(v->hybrid.ended_mask & (1u << i)))
            mask |= 1u << i;
    return mask;
}
#endif

static int check(unsigned *voices, unsigned *drums) {
    static const unsigned expected[3] = {3, 3, 2};
    unsigned parts[3] = {0, 0, 0}, sounds = 0;
    *voices = *drums = 0;
    for (unsigned i = 0; i < MC_VOICES; i++) {
        const McVoice *v = &engine.voice[i];
        if (!v->active || v->part >= 3) return 0;
        (*voices)++;
        parts[v->part]++;
        unsigned part = i % 3u;
        unsigned note = (part == 2 ? 100u : 101u) - i / 3u;
        if (v->part != part || v->note != note || v->velocity != 127 ||
            v->gate_left || v->stage != 1 || !v->accent) return 0;
#if LOAD_PROBE_NEW
        if (v->patch.source_mask != 7 || sources(v) != 7) return 0;
#endif
    }
    for (unsigned p = 0; p < 3; p++)
        if (parts[p] != expected[p] || engine.project.analog[p] != 127 ||
            engine.midi.bend[p] != 8191) return 0;
    for (unsigned i = 0; i < MC_DRUM_VOICES; i++) {
        const McDrum *d = &engine.drum[i];
        if (d->env) {
            (*drums)++;
            sounds |= 1u << d->sound;
            if (d->kit != 6 || d->velocity != 127) return 0;
        }
    }
    if (sounds != ((1u << 0) | (1u << 1) | (1u << 3) |
                   (1u << 8) | (1u << 10) | (1u << 11))) return 0;
    for (unsigned b = 0; b < MC_FX_BANKS; b++) {
        const McSharedFx *fx = b < 2 ? &engine.shared_fx : &engine.extra_fx;
        if (engine.project.fx_bus_type[b] != (b ? 5 : 4) ||
            !(b & 1u ? fx->cf_active : fx->rd_active) ||
            (b & 1u ? fx->cf_type : fx->rd_type) != (b ? 5 : 4)) return 0;
        for (unsigned p = 0; p < MC_PARTS; p++)
            if (engine.project.fx_send[p][b] != 127) return 0;
    }
    return *voices == 8 && *drums == 6 && !engine.steals &&
        engine.playing && engine.master == 818 && engine.project.bpm == 240;
}

static void configuration(void) {
    printf("\"configuration\":{\"bpm\":%u,\"master\":%u,\"drum_kit\":%u,"
        "\"fx_types\":[", engine.project.bpm, engine.master, engine.project.drum_kit);
    for (unsigned b = 0; b < MC_FX_BANKS; b++)
        printf("%s%u", b ? "," : "", engine.project.fx_bus_type[b]);
    printf("],\"fx_names\":[\"%s\",\"%s\",\"%s\",\"%s\"],\"sends\":[",
        mc_fx_rd_names[engine.project.fx_bus_type[0]],
        mc_fx_cf_names[engine.project.fx_bus_type[1]],
        mc_fx_rd_names[engine.project.fx_bus_type[2]],
        mc_fx_cf_names[engine.project.fx_bus_type[3]]);
    for (unsigned p = 0; p < MC_PARTS; p++) {
        printf("%s[", p ? "," : "");
        for (unsigned b = 0; b < MC_FX_BANKS; b++)
            printf("%s%u", b ? "," : "", engine.project.fx_send[p][b]);
        printf("]");
    }
    printf("],\"parts\":[");
    for (unsigned p = 0; p < 3; p++) {
        const McPatch *patch = &engine.project.patch[p];
        printf("%s{\"preset\":%u,\"name\":\"%s\",\"analog\":%u,"
            "\"bend\":%d,\"pcm_mix\":%u", p ? "," : "",
            engine.project.preset[p], mc_preset_names[engine.project.preset[p]],
            engine.project.analog[p], engine.midi.bend[p], patch->pcm_mix);
#if LOAD_PROBE_NEW
        const McHybridProfile *recipe = hy_profile(patch->profile);
        printf(",\"profile\":%u,\"available_mask\":%u,\"source_mask\":%u,"
            "\"fm_mix\":%u,\"va_mix\":%u,\"fm_duration_ms\":%u",
            patch->profile, recipe->available_mask, patch->source_mask,
            patch->fm_mix, patch->va_mix, recipe->fm_duration_ms);
#else
        printf(",\"engine\":%u,\"pcm_source\":%u,\"source_mask\":null",
            patch->engine, patch->pcm);
#endif
        printf("}");
    }
    printf("],\"voices\":[");
    for (unsigned i = 0; i < MC_VOICES; i++) {
        const McVoice *v = &engine.voice[i];
        printf("%s{\"part\":%u,\"note\":%u,\"age\":%u,\"pcm_position_q16\":%u,"
            "\"sample_level\":%u", i ? "," : "", v->part, v->note,
            v->age, v->pcm_pos, v->sample_level);
#if LOAD_PROBE_NEW
        const McPcmZone *zone = hy_pcm_zone(v->hybrid.pcm_zone);
        const McPcmMip *mip = &zone->mip[v->sample_level];
        printf(",\"effective_mask\":%u,\"gain\":[%u,%u,%u],\"ended_mask\":%u,"
            "\"pcm_zone\":%u,\"family\":%u,\"mip_length\":%u,"
            "\"loop_start\":%u,\"loop_end\":%u,\"selected_pcm_bytes\":%zu",
            sources(v), v->hybrid.gain[0], v->hybrid.gain[1], v->hybrid.gain[2],
            v->hybrid.ended_mask, v->hybrid.pcm_zone, zone->family,
            mip->length, mip->loop_start, mip->loop_end,
            (size_t)mip->length * sizeof mip->data[0]);
#endif
        printf("}");
    }
    printf("],\"drums\":[");
    for (unsigned i = 0; i < MC_DRUM_VOICES; i++)
        printf("%s{\"sound\":%u,\"env\":%d,\"age\":%u}", i ? "," : "",
            engine.drum[i].sound, engine.drum[i].env, engine.drum[i].age);
    printf("]},");
}

static void storage(void) {
    printf("\"sizeof_bytes\":{\"engine\":%zu,\"project\":%zu,\"patch\":%zu,"
        "\"voice\":%zu,\"voices\":%zu,\"pcm_cache\":%zu,\"pcm_cache_data\":%zu,"
        "\"pcm_caches\":%zu,\"pitch_caches\":%zu,\"analog_voices\":%zu,"
        "\"shared_fx\":%zu,\"worker_voice\":%zu,\"synth_job\":%zu,"
        "\"fx_job\":%zu,\"output_block\":%zu",
        sizeof engine, sizeof engine.project, sizeof(McPatch), sizeof(McVoice),
        sizeof engine.voice, sizeof(McPcmCache), sizeof engine.pcm_cache[0].data,
        sizeof engine.pcm_cache, sizeof engine.pitch_cache, sizeof engine.analog_voice,
        sizeof(McSharedFx), sizeof(McWorkerVoice), sizeof(McSynthJob),
        sizeof(McFxJob), sizeof audio);
#if LOAD_PROBE_NEW
    size_t pcm_bytes = 0, descriptor_bytes = 0;
    for (unsigned z = 0; z < MC_PCM_ZONES; z++) {
        const McPcmZone *zone = hy_pcm_zone(z);
        descriptor_bytes += sizeof *zone + (size_t)zone->levels * sizeof *zone->mip;
        for (unsigned m = 0; m < zone->levels; m++)
            pcm_bytes += (size_t)zone->mip[m].length * sizeof zone->mip[m].data[0];
    }
    printf(",\"hybrid_state\":%zu,\"voice_render\":%zu,\"pcm_bank_data\":%zu,"
        "\"pcm_bank_descriptors\":%zu,\"synth_tables\":%zu",
        sizeof(McHybridState), sizeof(McVoiceRender), pcm_bytes, descriptor_bytes,
        sizeof mc_sine + sizeof mc_saw + sizeof mc_square + sizeof mc_frequency);
#endif
    printf("},\"copies\":{\"dual_core_enabled\":false,\"worker_transfers\":0,"
        "\"pcm_refill_max_data_bytes\":%zu,\"pcm_refill_count\":null,"
        "\"total_internal_copy_bytes\":null}", sizeof engine.pcm_cache[0].data);
}

int main(void) {
    mc_init(&engine);
    engine.master = 818;
    mc_perf_fixture_four(&engine, 0);
    /* 원본 fixture의 Analog=0만 신규와 동일한 127로 맞춘다. */
    for (unsigned p = 0; p < 3; p++) engine.project.analog[p] = 127;
    if (!mc_project_valid(&engine.project)) return 3;
    for (unsigned b = 0; b < WARMUP; b++) mc_render(&engine, audio, BLOCK);
    unsigned voices, drums;
    if (!check(&voices, &drums)) return 4;
    uint32_t clips = engine.clips, hits = engine.limiter_hits, events = engine.events;
    engine.peak = 0;
    uint64_t nonzero = 0, rails = 0, ceiling = 0, sum_squares = 0;
    uint64_t hash = UINT64_C(14695981039346656037);
    uint64_t render_wall = 0, render_cpu = 0;
    unsigned peak = 0, vmin = UINT_MAX, vmax = 0, dmin = UINT_MAX, dmax = 0;
    uint64_t whole_wall = ns(CLOCK_MONOTONIC), whole_cpu = ns(CLOCK_PROCESS_CPUTIME_ID);
    for (unsigned b = 0; b < BLOCKS; b++) {
        uint64_t wall = ns(CLOCK_MONOTONIC), cpu = ns(CLOCK_PROCESS_CPUTIME_ID);
        mc_render(&engine, audio, BLOCK);
        render_cpu += ns(CLOCK_PROCESS_CPUTIME_ID) - cpu;
        render_wall += ns(CLOCK_MONOTONIC) - wall;
        if (!check(&voices, &drums)) {
            fprintf(stderr, "configuration mismatch at measured block %u\n", b);
            return 5;
        }
        if (voices < vmin) vmin = voices;
        if (voices > vmax) vmax = voices;
        if (drums < dmin) dmin = drums;
        if (drums > dmax) dmax = drums;
        for (unsigned i = 0; i < BLOCK * 2; i++) {
            int32_t sample = audio[i];
            unsigned absolute = (unsigned)(sample < 0 ? -sample : sample);
            nonzero += sample != 0;
            rails += absolute >= 32767;
            ceiling += absolute >= MC_OUTPUT_CEILING;
            if (absolute > peak) peak = absolute;
            sum_squares += (uint64_t)((int64_t)sample * sample);
            hash ^= (uint16_t)sample;
            hash *= UINT64_C(1099511628211);
        }
    }
    whole_cpu = ns(CLOCK_PROCESS_CPUTIME_ID) - whole_cpu;
    whole_wall = ns(CLOCK_MONOTONIC) - whole_wall;
    printf("{\"engine\":\"%s\",\"version\":\"%s\",\"presets\":%u,"
        "\"block_frames\":%u,\"warmup_blocks\":%u,\"measured_blocks\":%u,"
        "\"sample_rate\":%u,\"frames\":%u,\"render_wall_ns\":%" PRIu64 ","
        "\"render_cpu_ns\":%" PRIu64 ",\"whole_wall_ns\":%" PRIu64 ","
        "\"whole_cpu_ns\":%" PRIu64 ",\"active_voices_min\":%u,"
        "\"active_voices_max\":%u,\"active_drums_min\":%u,\"active_drums_max\":%u,"
        "\"sequencer_events\":%u,\"steals\":%u,\"nonzero_samples\":%" PRIu64 ","
        "\"rail_samples\":%" PRIu64 ",\"ceiling_samples\":%" PRIu64 ","
        "\"sample_peak\":%u,\"engine_peak\":%u,\"engine_clips\":%u,"
        "\"limiter_hits\":%u,\"sum_squares\":%" PRIu64 ",\"pcm_hash\":\"%016" PRIx64 "\",",
        LOAD_PROBE_NEW ? "new" : "original", MC_VERSION, MC_PRESETS, BLOCK, WARMUP,
        BLOCKS, MC_RATE, BLOCKS * BLOCK, render_wall, render_cpu, whole_wall,
        whole_cpu, vmin, vmax, dmin, dmax, engine.events - events, engine.steals,
        nonzero, rails, ceiling, peak, engine.peak, engine.clips - clips,
        engine.limiter_hits - hits, sum_squares, hash);
    configuration();
    storage();
    puts("}");
    if (!nonzero || engine.clips != clips || rails) return 6;
    return 0;
}
