/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 dspaudio */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "pcm_bank.h"

int main(void) {
    unsigned bytes=0,families[MC_PCM_FAMILIES]={0};
    for(unsigned z=0;z<MC_PCM_ZONES;z++) {
        const McPcmZone *zone=&mc_pcm_zones[z];
        assert(zone->family<MC_PCM_FAMILIES);
        assert(zone->root>=12&&zone->root<=108);
        assert(zone->levels==7);
        assert(zone->tuning_hz_q16>0);
        families[zone->family]++;
        uint64_t loop_cycles=0;
        for(unsigned m=0;m<zone->levels;m++) {
            const McPcmMip *level=&zone->mip[m];
            assert(level->length>0);
            if(m) assert(level->length==(zone->mip[m-1].length+1u)/2u);
            assert(level->loop_end<=level->length);
            assert((level->loop_start==0&&level->loop_end==0)||
                   (level->loop_start<level->loop_end));
            int peak=0,adjacent=0;
            uint64_t energy=0;
            for(unsigned n=0;n<level->length;n++) {
                int value=level->data[n],magnitude=value<0?-value:value;
                if(magnitude>peak) peak=magnitude;
                energy+=(uint64_t)((int64_t)value*value);
                if(n) {
                    int difference=value-level->data[n-1];
                    if(difference<0) difference=-difference;
                    if(difference>adjacent) adjacent=difference;
                }
            }
            assert(peak>0&&peak<=26000&&energy>0);
            if(level->loop_end) {
                uint64_t span=(uint64_t)(level->loop_end-level->loop_start)<<m;
                uint64_t period=16000u*65536u;
                uint64_t phase=(uint64_t)zone->tuning_hz_q16*span;
                uint64_t cycles=(phase+period/2u)/period;
                uint64_t error=phase>cycles*period?phase-cycles*period:cycles*period-phase;
                assert(cycles>0&&error<=span/2u+1u);
                if(m) assert(cycles==loop_cycles);
                loop_cycles=cycles;
                int wrap=level->data[level->loop_end-1]-level->data[level->loop_start];
                if(wrap<0) wrap=-wrap;
                assert(wrap<=adjacent);
            }
            bytes+=(unsigned)level->length*2u;
        }
        bytes+=(unsigned)zone->levels*(unsigned)sizeof(McPcmMip);
    }
    for(unsigned family=0;family<MC_PCM_FAMILIES;family++) assert(families[family]>0);
    bytes+=(unsigned)sizeof(mc_pcm_zones);
    assert(bytes<=224u*1024u);
    printf("PCM_BANK_PASS zones=%u families=%u bytes=%u\n",
           MC_PCM_ZONES,MC_PCM_FAMILIES,bytes);
    return 0;
}
