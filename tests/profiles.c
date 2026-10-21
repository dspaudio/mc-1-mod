/* SPDX-License-Identifier: GPL-3.0-only */
#include "hybrid_profiles.h"
#include "pcm_bank.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Validate immutable recipes and real factory data, not perceived sound quality. */
int main(void) {
    static const uint8_t masks[32]={
        6,4,6,6,6,7,6,6,3,1,3,3,5,7,6,6,
        6,6,4,1,6,4,6,6,6,1,4,6,4,7,6,6};
    static const uint8_t families[32]={
        255,255,255,255,0,0,255,255,1,1,1,1,2,3,255,255,
        255,255,255,4,255,255,255,255,255,5,255,6,255,2,255,6};
    static const char *const groups[8]={
        "BASS CORE","BASS SEQ","PIANO EP","PLUCK MALLET",
        "COMP STAB","LEAD","PAD TEXTURE","MOTION"};
    _Static_assert(MC_PRESETS==64&&sizeof(McPatch)==15,"append-only profile ABI");
    _Static_assert(sizeof(McAuxState)==20&&sizeof(McAuxProfile)==16,"auxiliary runtime and recipe ABI");
    _Static_assert(MC_KITS==8&&MC_DRUMS==16&&MC_VOICES==8&&MC_DRUM_VOICES==6,"resource ABI");
    _Static_assert(MC_FX_BANKS==4&&MC_PARTS==4,"shared FX ABI");
    _Static_assert(MC_PCM_FAMILIES==7&&MC_PCM_ZONES==9,"PCM family ABI");
    _Static_assert(sizeof(((McHybridProfile *)0)->fm_body_q15)==2&&
        sizeof(((McHybridProfile *)0)->va_colour_start_q15)==2&&
        sizeof(((McHybridProfile *)0)->va_colour_end_q15)==2&&
        sizeof(((McHybridProfile *)0)->va_morph_ms)==2&&
        sizeof(((McHybridProfile *)0)->gain_q8)==2,"immutable recipe units");
    unsigned singles=0,triples=0,used=0;
    for(unsigned id=0;id<32;id++) {
        const McPatch *p=&mc_presets[id];
        const McHybridProfile *r=&mc_hybrid_profiles[id];
        assert(!hy_aux_profile(id));
        unsigned available=masks[id]|(id==4?HY_PCM:0)|
            (id==19?(HY_FM|HY_VA):0)|
            (((id>=8&&id<=11)||id==25)?HY_VA:0)|
            ((id==27||id==31)?HY_PCM:0);
        assert(p->profile==id&&p->source_mask==masks[id]);
        assert(r->available_mask==available&&r->pcm_family==families[id]);
        assert(available&&!(available&~HY_ALL));
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
        assert(r->fm_body_q15<=32768);
        assert(r->gain_q8==(id==28?448u:256u));
        assert(r->va_colour_start_q15<=32768&&r->va_colour_end_q15<=32768);
        assert(!r->va_morph_ms||(r->va_morph_ms>=80&&r->va_morph_ms<=10000));
        if(!r->va_morph_ms)assert(r->va_colour_start_q15==r->va_colour_end_q15);
        if(!(available&HY_VA)||p->wave==MC_WAVE_SINE)
            assert(!r->va_colour_start_q15&&!r->va_colour_end_q15&&!r->va_morph_ms);
        assert(r->filter_kind==(id==1?HY_FILTER_ACID4:HY_FILTER_2POLE));
        if(available&HY_FM) {
            assert(r->fm_index>0&&r->fm_decay_ms>0);
            if(r->fm_duration_ms) {
                assert(!r->fm_body_q15&&r->fm_duration_ms>=r->fm_decay_ms);
            } else if(p->sustain)assert(r->fm_body_q15>0);
        } else {
            assert(!r->fm_body_q15&&!r->fm_index&&!r->fm_feedback);
            assert(!r->velocity_index&&!r->fm_decay_ms&&!r->fm_duration_ms);
        }
        for(unsigned bank=0;bank<MC_FX_BANKS;bank++)
            assert(mc_preset_fx_send[id][bank]<=(id<8?24:64));
        if(available&HY_PCM) {
            assert(r->pcm_family<MC_PCM_FAMILIES);
            unsigned zones=0;
            for(unsigned z=0;z<MC_PCM_ZONES;z++)zones+=mc_pcm_zones[z].family==r->pcm_family;
            assert(zones==(r->pcm_family==1?3u:1u));
            used|=1u<<r->pcm_family;
        } else assert(r->pcm_family==HY_PCM_NONE);
        assert(!strcmp(mc_preset_categories[id],groups[id/4]));
        assert(mc_preset_names[id][0]&&strlen(mc_preset_names[id])<=12);
        for(unsigned other=0;other<id;other++) {
            /* Distinction must survive removal of the profile ID itself. */
            McPatch lhs=*p,rhs=mc_presets[other];lhs.profile=rhs.profile=0;
            assert(memcmp(&lhs,&rhs,sizeof lhs)||memcmp(r,&mc_hybrid_profiles[other],sizeof *r));
        }
        singles+=(masks[id]&(masks[id]-1u))==0;
        triples+=masks[id]==HY_ALL;
    }
    assert(singles==8&&triples==3&&used==127);
    {
        static const uint8_t kinds[32]={
            0,4,0,0,0,2,0,2,4,4,0,2,4,4,0,2,
            0,2,0,4,0,0,4,0,2,0,4,2,0,2,0,0};
        unsigned dual=0,fm=0,va=0;
        for(unsigned id=32;id<MC_PRESETS;id++) {
            const McPatch *p=&mc_presets[id];
            const McHybridProfile *r=hy_profile(id);
            const McAuxProfile *a=hy_aux_profile(id);
            unsigned mask=kinds[id-32]?HY_ALL:HY_FM|HY_VA;
            assert(p->profile==id&&p->source_mask==mask&&r->available_mask==mask);
            assert(r->pcm_family==HY_PCM_NONE&&p->mode<=MC_MONO);
            assert(p->wave<=MC_WAVE_SINE&&p->level&&p->fm_mix>=20&&p->va_mix>=20);
            const uint8_t *bytes=(const uint8_t *)p;
            for(unsigned n=3;n<sizeof *p;n++)assert(bytes[n]<=127);
            assert(r->fm_ratio>=1&&r->fm_ratio<=8&&r->fm_index&&r->fm_index<=127);
            assert(r->fm_feedback<=24&&r->velocity_index<=127&&r->velocity_cutoff<=127);
            assert(r->fm_decay_ms<=10000&&r->fm_duration_ms<=10000&&r->fm_body_q15<=32768);
            assert(r->gain_q8&&r->gain_q8<=448&&r->filter_kind<=HY_FILTER_ACID4);
            assert(r->va_colour_start_q15<=32768&&r->va_colour_end_q15<=32768&&r->va_morph_ms<=10000);
            if(a) {
                assert(a->kind==kinds[id-32]&&p->pcm_mix>=20);
                /* The sub-octave may be detuned slightly below exactly 0.5. */
                assert(a->tuning_q16>=32000&&a->tuning_q16<=131072);
                assert(a->wave<=MC_WAVE_SINE&&a->colour_q15<=32768);
                assert(a->fm_decay_ms<=10000&&a->fm_body_q15<=32768);
                if(a->kind==HY_FM) {
                    assert(a->fm_ratio>=1&&a->fm_ratio<=8&&a->fm_index<=127&&a->fm_feedback<=24);
                    assert(!strcmp(hy_source_name(id,0),"FM2"));fm++;
                } else { assert(!strcmp(hy_source_name(id,0),"VA2"));va++; }
            } else { assert(!kinds[id-32]&&!p->pcm_mix);dual++; }
            assert(!strcmp(mc_preset_categories[id],id<44?"80S SYNTH":id<54?"90S SYNTH":"00S SYNTH"));
            assert(mc_preset_names[id][0]&&strlen(mc_preset_names[id])<=12);
            for(unsigned other=0;other<id;other++) {
                McPatch lhs=*p,rhs=mc_presets[other];lhs.profile=rhs.profile=0;
                lhs.level=rhs.level=0;
                const McAuxProfile *b=hy_aux_profile(other);
                assert(memcmp(&lhs,&rhs,sizeof lhs)||memcmp(r,hy_profile(other),sizeof *r)||
                    (!!a!=!!b)||(a&&b&&memcmp(a,b,sizeof *a)));
            }
            for(unsigned bank=0;bank<MC_FX_BANKS;bank++)assert(mc_preset_fx_send[id][bank]<=64);
        }
        assert(dual==16&&fm==8&&va==8&&!hy_aux_profile(MC_PRESETS));
    }
    for(unsigned z=0;z<MC_PCM_ZONES;z++) {
        const McPcmZone *zone=&mc_pcm_zones[z];
        static const uint8_t roots[MC_PCM_ZONES]={36,48,60,72,60,60,60,60,60};
        assert(zone->family<MC_PCM_FAMILIES&&zone->tuning_hz_q16&&zone->levels==7);
        assert(zone->root==roots[z]);
        for(unsigned mip=0;mip<zone->levels;mip++) {
            const McPcmMip *m=&zone->mip[mip];unsigned nonzero=0;
            assert(m->data&&m->length>1);
            assert((!m->loop_start&&!m->loop_end)||
                (m->loop_start<m->loop_end&&m->loop_end<=m->length));
            for(unsigned n=0;n<m->length;n++)nonzero+=m->data[n]!=0;
            assert(nonzero);
        }
    }
    /* Structural role intent; these assertions cannot approve listening. */
    assert(mc_presets[0].wave==MC_WAVE_SINE&&mc_presets[1].glide>0);
    assert(mc_presets[2].wave==MC_WAVE_SQUARE&&!mc_hybrid_profiles[2].fm_duration_ms);
    assert(mc_hybrid_profiles[2].fm_body_q15>0);
    assert(mc_presets[6].source_mask==(HY_FM|HY_VA));
    assert(mc_presets[0].fm_mix>0&&mc_hybrid_profiles[0].fm_body_q15>0);
    assert(mc_presets[5].source_mask==HY_ALL&&mc_hybrid_profiles[5].fm_body_q15>0);
    assert(mc_presets[8].pcm_mix>mc_presets[8].fm_mix&&!(mc_presets[8].source_mask&HY_VA));
    assert(mc_hybrid_profiles[8].fm_duration_ms>0&&mc_presets[8].sustain>=80);
    assert(mc_presets[9].wave==MC_WAVE_SINE&&mc_presets[9].source_mask==HY_PCM);
    assert(mc_presets[9].sustain>=mc_presets[8].sustain&&mc_presets[9].release>mc_presets[8].release);
    assert(mc_presets[10].fm_mix>mc_presets[10].va_mix&&mc_presets[11].cutoff>mc_presets[10].cutoff);
    for(unsigned id=MC_PRESET_STAGE_PIANO;id<=MC_PRESET_CLEAR_EP;id++) {
        assert((mc_presets[id].source_mask&HY_PCM)&&!(mc_presets[id].source_mask&HY_VA));
        assert(mc_presets[id].wave==MC_WAVE_SINE&&!mc_hybrid_profiles[id].fm_feedback);
    }
    assert(mc_presets[14].sustain==0&&mc_presets[15].sustain==0);
    assert(mc_hybrid_profiles[16].fm_body_q15==32768&&mc_hybrid_profiles[17].fm_body_q15==32768);
    assert(mc_presets[16].sustain>=120&&mc_presets[17].sustain>=120&&mc_presets[18].sustain==0);
    assert(mc_presets[16].wave==MC_WAVE_SINE&&mc_presets[17].wave==MC_WAVE_SINE);
    assert(mc_presets[20].wave==MC_WAVE_SAW&&mc_presets[21].wave==MC_WAVE_SQUARE);
    assert(mc_presets[22].glide>0&&mc_presets[23].fm_mix>mc_presets[23].va_mix);
    assert(mc_presets[24].va_mix>mc_presets[24].pcm_mix);
    assert(mc_presets[25].pcm_mix>mc_presets[25].va_mix);
    assert(MC_PRESET_HOLLOW_PAD==26&&mc_presets[26].wave==MC_WAVE_SQUARE);
    assert(mc_presets[27].source_mask==(HY_FM|HY_VA)&&mc_hybrid_profiles[27].pcm_family==6);
    assert(mc_presets[27].wave==MC_WAVE_SINE&&!mc_hybrid_profiles[27].fm_feedback);
    for(unsigned id=24;id<=30;id++)if(id!=25&&id!=27&&id!=28&&id!=29)
        assert(!(mc_presets[id].source_mask&HY_PCM));
    assert(mc_presets[28].sustain==0&&mc_presets[29].sustain==0);
    assert(mc_presets[28].source_mask==HY_VA&&mc_presets[29].source_mask==HY_ALL);
    assert(mc_hybrid_profiles[28].available_mask==HY_VA&&mc_hybrid_profiles[29].fm_duration_ms);
    assert(mc_presets[24].wave==MC_WAVE_SINE&&mc_presets[30].wave==MC_WAVE_SINE);
    assert(mc_presets[30].attack>mc_presets[24].attack&&mc_hybrid_profiles[30].fm_decay_ms>=4000);
    assert(mc_hybrid_profiles[31].fm_decay_ms>1000&&mc_presets[31].va_mix>mc_presets[31].fm_mix);
    assert(mc_presets[31].source_mask==(HY_FM|HY_VA)&&!mc_hybrid_profiles[31].fm_feedback);
    {
        static McProject project;
        static const uint8_t types[4]={3,1,7,5},drums[4]={12,0,6,0},ids[3]={0,10,20};
        memset(&project,0xa5,sizeof project);
        mc_factory(&project);
        assert(mc_project_valid(&project));
        assert(!memcmp(mc_factory_fx_types,types,sizeof types));
        assert(!memcmp(project.fx_bus_type,types,sizeof types));
        assert(!memcmp(project.fx_send[3],drums,sizeof drums));
        for(unsigned part=0;part<3;part++) {
            assert(project.preset[part]==ids[part]);
            assert(!memcmp(&project.patch[part],&mc_presets[ids[part]],sizeof(McPatch)));
            assert(!memcmp(project.fx_send[part],mc_preset_fx_send[ids[part]],MC_FX_BANKS));
        }
        mc_project_defaults(&project);
        for(unsigned bank=0;bank<MC_FX_BANKS;bank++) {
            assert(!project.fx_bus_type[bank]);
            for(unsigned part=0;part<MC_PARTS;part++)assert(!project.fx_send[part][bank]);
        }
        mc_factory(&project);
        assert(!memcmp(project.fx_bus_type,types,sizeof types));
        for(unsigned part=0;part<3;part++)
            assert(!memcmp(project.fx_send[part],mc_preset_fx_send[ids[part]],MC_FX_BANKS));
        assert(!memcmp(project.fx_send[3],drums,sizeof drums));
    }
    puts("PASS: 32 preserved and 32 synthetic profiles, typed 16 dual/16 triple recipes, 15-byte patches");
    puts("PASS: factory 00/10/20, shared palette, melodic/drum sends after defaults clear");
    puts("UNRUN: audio rendering/listening, WASM execution, target/device approval");
    return 0;
}
