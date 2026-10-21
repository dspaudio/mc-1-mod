/* SPDX-License-Identifier: GPL-3.0-only */
#include "hybrid_profiles.h"
#include "pcm_bank.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* 실행 kernel과 독립적으로 실제 factory 데이터와 후보 PCM의 계약을 검사한다. */
int main(void) {
    static const uint8_t masks[32]={
        4,4,6,6,6,5,4,6,7,5,6,3,1,3,6,2,
        6,6,4,7,6,4,6,6,5,5,5,7,4,3,5,6};
    static const uint8_t families[32]={
        255,255,255,255,0,0,255,255,1,1,1,1,2,3,255,255,
        255,255,255,4,255,255,255,255,5,5,6,6,255,2,5,6};
    static const char *const groups[8]={
        "BASS CORE","BASS SEQ","PIANO EP","PLUCK MALLET",
        "COMP STAB","LEAD","PAD TEXTURE","MOTION"};
    _Static_assert(MC_PRESETS==32&&sizeof(McPatch)==15,"profile ABI");
    _Static_assert(MC_KITS==8&&MC_DRUMS==16&&MC_VOICES==8,"resource ABI");
    unsigned singles=0,triples=0,used=0;
    for(unsigned id=0;id<MC_PRESETS;id++) {
        const McPatch *p=&mc_presets[id];
        const McHybridProfile *r=&mc_hybrid_profiles[id];
        unsigned available=masks[id]|((id==4||id==10||id==31)?HY_PCM:0);
        assert(p->profile==id&&p->source_mask==masks[id]);
        assert(r->available_mask==available&&r->pcm_family==families[id]);
        assert(!(p->source_mask&~r->available_mask));
        assert(p->mode==((id==1||id==22)?MC_MONO:MC_POLY));
        assert(p->wave<=MC_WAVE_SINE&&p->level>0);
        const uint8_t *bytes=(const uint8_t *)p;
        for(unsigned n=3;n<sizeof *p;n++)assert(bytes[n]<=127);
        const unsigned gain[3]={p->pcm_mix,p->fm_mix,p->va_mix};
        for(unsigned s=0;s<3;s++) {
            if(available&(1u<<s))assert(gain[s]>=20);
            else assert(!gain[s]);
        }
        assert(r->fm_ratio>=1&&r->fm_ratio<=8);
        assert(r->fm_index<=127&&r->fm_feedback<=24);
        assert(r->velocity_index<=127&&r->velocity_cutoff<=127);
        assert(r->fm_decay_ms<=10000&&r->fm_duration_ms<=10000);
        assert(r->filter_kind==(id==1?HY_FILTER_ACID4:HY_FILTER_2POLE));
        if(available&HY_FM)assert(r->fm_index>0&&r->fm_decay_ms>0);
        if(available&HY_PCM) {
            unsigned zones=0;
            for(unsigned z=0;z<MC_PCM_ZONES;z++)zones+=mc_pcm_zones[z].family==r->pcm_family;
            assert(zones==(r->pcm_family==1?3u:1u));
            used|=1u<<r->pcm_family;
        } else assert(r->pcm_family==HY_PCM_NONE);
        assert(!strcmp(mc_preset_categories[id],groups[id/4]));
        assert(mc_preset_names[id][0]&&strlen(mc_preset_names[id])<=12);
        for(unsigned other=0;other<id;other++) {
            /* profile ID 자체를 빼도 실제 편집값/recipe가 서로 달라야 한다. */
            McPatch lhs=*p,rhs=mc_presets[other];lhs.profile=rhs.profile=0;
            assert(memcmp(&lhs,&rhs,sizeof lhs)||memcmp(r,&mc_hybrid_profiles[other],sizeof *r));
        }
        singles+=(masks[id]&(masks[id]-1u))==0;
        triples+=masks[id]==HY_ALL;
    }
    assert(singles==8&&triples==3&&used==127);
    for(unsigned z=0;z<MC_PCM_ZONES;z++) {
        const McPcmZone *zone=&mc_pcm_zones[z];
        assert(zone->family<MC_PCM_FAMILIES&&zone->tuning_hz_q16&&zone->levels==7);
        for(unsigned mip=0;mip<zone->levels;mip++) {
            const McPcmMip *m=&zone->mip[mip];unsigned nonzero=0;
            assert(m->data&&m->length>1);
            assert((!m->loop_start&&!m->loop_end)||
                (m->loop_start<m->loop_end&&m->loop_end<=m->length));
            for(unsigned n=0;n<m->length;n++)nonzero+=m->data[n]!=0;
            assert(nonzero);
        }
    }
    /* 수치로 입증하는 제작 의도다. 청취 승인을 대신하지 않는다. */
    assert(mc_presets[0].wave==MC_WAVE_SINE&&mc_presets[1].glide>0);
    assert(mc_presets[2].wave==MC_WAVE_SQUARE&&mc_hybrid_profiles[2].fm_duration_ms<=80);
    assert(mc_presets[6].source_mask==HY_VA);
    assert(mc_presets[8].pcm_mix>mc_presets[8].fm_mix&&mc_presets[8].fm_mix>mc_presets[8].va_mix);
    assert(mc_presets[9].wave==MC_WAVE_SAW&&mc_presets[9].va_mix>mc_presets[8].va_mix);
    assert(mc_presets[9].sustain>mc_presets[8].sustain&&mc_presets[9].release>mc_presets[8].release);
    assert(mc_presets[10].fm_mix>mc_presets[10].va_mix&&mc_presets[11].cutoff>mc_presets[10].cutoff);
    assert(mc_presets[14].sustain==0&&mc_presets[15].sustain==0);
    assert(mc_hybrid_profiles[17].filter_depth>10000&&mc_presets[18].sustain==0);
    assert(mc_presets[20].wave==MC_WAVE_SAW&&mc_presets[21].wave==MC_WAVE_SQUARE);
    assert(mc_presets[22].glide>0&&mc_presets[23].fm_mix>mc_presets[23].va_mix);
    assert(mc_presets[24].va_mix>mc_presets[24].pcm_mix);
    assert(mc_presets[25].pcm_mix>mc_presets[25].va_mix);
    assert(mc_presets[26].wave==MC_WAVE_SINE&&mc_presets[27].fm_mix>mc_presets[27].pcm_mix);
    assert(mc_presets[28].sustain==0&&mc_presets[29].sustain==0);
    assert(mc_presets[30].attack>mc_presets[24].attack&&mc_hybrid_profiles[30].filter_depth>10000);
    assert(mc_hybrid_profiles[31].fm_decay_ms>1000&&mc_presets[31].fm_mix>mc_presets[31].va_mix);
    puts("PASS: 32 profiles, masks/optional sources, 15-byte parameters, 8 roles, 7 families/9 zones/63 mips");
    puts("UNRUN: audio rendering/listening, WASM execution, target/device approval");
    return 0;
}
