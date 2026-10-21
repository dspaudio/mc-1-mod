/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 dspaudio */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "pcm_bank.h"

#define TWO_PI 6.2831853071795864769
#define HALF_PI 1.5707963267948966192
#define TOP_HZ ((double)MC_PCM_RATE * 0.44)
#define SEMITONE 1.0594630943592952646
#define QUARTER 1.1892071150027210667
#define PROBE_HZ 20.0
#define BRIGHT_WINDOW 512u
#define TONAL_WINDOW 2048u
#define MAX_BANDS 48u
#define MAX_WINDOW 3200u

/* This primitive test links without -lm, so the spectral probes carry their own
   cosine: range reduction into 0..pi/2 then the Taylor series through x^10/10!,
   whose truncation error below pi/2 stays under 1.3e-7. */
static double kcos(double x) {
    x -= TWO_PI * (double)(long long)(x / TWO_PI);
    if (x < 0.0) x += TWO_PI;
    if (x > TWO_PI / 2.0) x = TWO_PI - x;
    int flip = 0;
    if (x > HALF_PI) { x = TWO_PI / 2.0 - x; flip = 1; }
    double s = x * x;
    double r = 1.0 - s / 2.0 + s * s / 24.0 - s * s * s / 720.0 +
               s * s * s * s / 40320.0 - s * s * s * s * s / 3628800.0;
    return flip ? -r : r;
}

static double taper[MAX_WINDOW];
static unsigned taper_count = 0;
static int taper_hann = -1;

/* A Hann taper drops probe leakage from -13 dB to -31 dB, which the broadband
   measures need. A loop region is exactly periodic, so there the rectangular
   taper is the accurate one: neighbouring loop bins stay orthogonal and a
   neighbour probe reads real content instead of the fundamental's skirt. */
static void set_window(unsigned count, int hann) {
    assert(count <= MAX_WINDOW);
    if (count == taper_count && hann == taper_hann) return;
    for (unsigned n = 0; n < count; n++)
        taper[n] = hann ? 0.5 - 0.5 * kcos(TWO_PI * (double)n / (double)count) : 1.0;
    taper_count = count;
    taper_hann = hann;
}

/* Goertzel power at hz. Squared magnitude keeps the test free of sqrt and log. */
static double tone(const int16_t *data, unsigned start, unsigned count, double hz) {
    double w = TWO_PI * hz / (double)MC_PCM_RATE;
    double cs = kcos(w), sn = kcos(w - HALF_PI), c = 2.0 * cs;
    double s1 = 0.0, s2 = 0.0;
    for (unsigned n = 0; n < count; n++) {
        double v = (double)data[start + n] * taper[n] + c * s1 - s2;
        s2 = s1; s1 = v;
    }
    double re = s1 - s2 * cs, im = s2 * sn;
    return (re * re + im * im) / ((double)count * (double)count);
}

static double brightness(const int16_t *data, unsigned start, double f0) {
    double num = 0.0, den = 0.0, hz = f0;
    set_window(BRIGHT_WINDOW, 1);
    while (hz < TOP_HZ) {
        double e = tone(data, start, BRIGHT_WINDOW, hz);
        num += hz * e; den += e; hz *= SEMITONE;
    }
    return den > 0.0 ? num / den : 0.0;
}

/* Quarter-octave bands from the fundamental up, each swept on a 20 Hz grid so a
   partial between band centres is still found. Counts bands within 30 dB of the
   strongest, which is the partial-richness floor the material must hold. */
static unsigned live_bands(const int16_t *data, unsigned start, unsigned count,
                           double f0, unsigned *available) {
    double best[MAX_BANDS], peak = 0.0;
    unsigned total = 0, live = 0;
    set_window(count, 1);
    for (double low = f0; low * QUARTER < TOP_HZ && total < MAX_BANDS; low *= QUARTER) {
        double high = low * QUARTER, top = 0.0;
        unsigned steps = (unsigned)((high - low) / PROBE_HZ) + 2u;
        for (unsigned j = 0; j < steps; j++) {
            double e = tone(data, start, count,
                            low + (high - low) * (double)j / (double)(steps - 1u));
            if (e > top) top = e;
        }
        best[total++] = top;
    }
    for (unsigned b = 0; b < total; b++) if (best[b] > peak) peak = best[b];
    for (unsigned b = 0; b < total; b++) if (best[b] > peak / 1000.0) live++;
    *available = total;
    return live;
}

int main(void) {
    unsigned bytes = 0, payload = 0, families[MC_PCM_FAMILIES] = {0};
    for (unsigned z = 0; z < MC_PCM_ZONES; z++) {
        const McPcmZone *zone = &mc_pcm_zones[z];
        assert(zone->family < MC_PCM_FAMILIES);
        assert(zone->root >= 12 && zone->root <= 108);
        assert(zone->levels == 7);
        assert(zone->tuning_hz_q16 > 0);
        families[zone->family]++;
        /* Deepest mip a playable note can select: one halving per octave above root. */
        unsigned reach = (108u - zone->root + 11u) / 12u;
        if (reach > zone->levels - 1u) reach = zone->levels - 1u;
        uint64_t loop_cycles = 0;
        int base_peak = 0;
        for (unsigned m = 0; m < zone->levels; m++) {
            const McPcmMip *level = &zone->mip[m];
            assert(level->length > 0);
            if (m) assert(level->length == (zone->mip[m - 1].length + 1u) / 2u);
            assert(level->loop_end <= level->length);
            assert((level->loop_start == 0 && level->loop_end == 0) ||
                   (level->loop_start < level->loop_end));
            int peak = 0, adjacent = 0, curve = 0;
            int64_t offset = 0;
            uint64_t energy = 0;
            for (unsigned n = 0; n < level->length; n++) {
                int value = level->data[n], magnitude = value < 0 ? -value : value;
                if (magnitude > peak) peak = magnitude;
                energy += (uint64_t)((int64_t)value * value);
                offset += value;
                if (n) {
                    int difference = value - level->data[n - 1];
                    if (difference < 0) difference = -difference;
                    if (difference > adjacent) adjacent = difference;
                }
                if (n && n + 1u < level->length) {
                    int second = level->data[n + 1] - 2 * value + level->data[n - 1];
                    if (second < 0) second = -second;
                    if (second > curve) curve = second;
                }
            }
            assert(peak > 0 && peak <= MC_PCM_PEAK && energy > 0);
            if (!m) base_peak = peak;
            /* Every mip a note can reach must carry real signal, not filter residue. */
            if (m <= reach) assert(peak >= MC_PCM_PEAK / 10);
            if (offset < 0) offset = -offset;
            assert(offset * 32 <= (int64_t)peak * level->length);
            if (level->loop_end) {
                unsigned local = level->loop_end - level->loop_start;
                uint64_t span = (uint64_t)local << m;
                uint64_t period = 16000u * 65536u;
                uint64_t phase = (uint64_t)zone->tuning_hz_q16 * span;
                uint64_t cycles = (phase + period / 2u) / period;
                uint64_t error = phase > cycles * period ? phase - cycles * period
                                                         : cycles * period - phase;
                assert(cycles > 0 && error <= span / 2u + 1u);
                if (m) assert(cycles == loop_cycles);
                loop_cycles = cycles;
                int wrap = level->data[level->loop_end - 1] - level->data[level->loop_start];
                if (wrap < 0) wrap = -wrap;
                assert(wrap <= adjacent);
                /* Exact periodic closure: the seam curves no harder than the material. */
                int seam = level->data[level->loop_start + 1] -
                           2 * level->data[level->loop_start] +
                           level->data[level->loop_end - 1];
                if (seam < 0) seam = -seam;
                assert(seam <= curve);
                /* The loop region plays forever, so its own offset must vanish. */
                int64_t sustained = 0;
                for (unsigned n = level->loop_start; n < level->loop_end; n++)
                    sustained += level->data[n];
                if (sustained < 0) sustained = -sustained;
                assert(sustained * 1024 <= (int64_t)peak * local);
            }
            payload += (unsigned)level->length * 2u;
            bytes += (unsigned)level->length * 2u;
        }
        bytes += (unsigned)zone->levels * (unsigned)sizeof(McPcmMip);

        const McPcmMip *base = &zone->mip[0];
        double f0 = (double)zone->tuning_hz_q16 / 65536.0;
        unsigned start = base->loop_end ? base->loop_start : base->length / 4u;
        unsigned count = base->loop_end ? (unsigned)(base->loop_end - base->loop_start)
                                        : TONAL_WINDOW;
        /* A loop resolves its orthogonal neighbour bins; a one-shot window cannot
           resolve a semitone at the lowest root, so it probes three semitones. */
        double low, high;
        if (base->loop_end) {
            double cycles = (double)loop_cycles;
            low = f0 * (cycles - 1.0) / cycles;
            high = f0 * (cycles + 1.0) / cycles;
        } else {
            low = f0 / QUARTER;
            high = f0 * QUARTER;
        }
        set_window(count, base->loop_end ? 0 : 1);
        double fundamental = tone(base->data, start, count, f0);
        double rival = tone(base->data, start, count, low);
        double other = tone(base->data, start, count, high);
        double octave = tone(base->data, start, count, f0 * 0.5);
        if (other > rival) rival = other;
        if (octave > rival) rival = octave;
        assert(fundamental > 0.0 && fundamental > rival * 512.0);

        unsigned available = 0;
        unsigned live = live_bands(base->data, start, count, f0, &available);
        assert(available >= 10u && live * 20u >= available * 9u);

        double attack = brightness(base->data, 0, f0);
        double body = brightness(base->data, base->length - BRIGHT_WINDOW, f0);
        assert(attack > 0.0 && body > 0.0);
        /* Material must evolve: the attack and the body cannot share one spectrum. */
        assert(attack >= body * 1.12 || attack * 1.12 <= body);

        printf("zone %u family %u root %3u tuning %9.4f Hz peak %5d bands %2u/%-2u "
               "pitch %9.3g attack %7.1f Hz body %7.1f Hz ratio %.3f reach m%u\n",
               z, zone->family, zone->root, f0, base_peak, live, available,
               fundamental / rival, attack, body, attack / body, reach);
    }
    for (unsigned family = 0; family < MC_PCM_FAMILIES; family++) assert(families[family] > 0);
    bytes += (unsigned)sizeof(mc_pcm_zones);
    assert(payload == MC_PCM_PAYLOAD);
    assert(bytes <= 224u * 1024u);
    printf("PCM_BANK_PASS zones=%u families=%u payload=%u bytes=%u budget=%u\n",
           MC_PCM_ZONES, MC_PCM_FAMILIES, payload, bytes, 224u * 1024u);
    return 0;
}
