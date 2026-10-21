/* SPDX-License-Identifier: GPL-3.0-only */
#include "hybrid.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static McEngine a,b;
static int16_t whole[8192*2],split[8192*2];
static void setup(McEngine *e) {
    mc_init(e);
    for(unsigned part=0;part<MC_PARTS;part++) {
        e->project.level[part]=127;e->project.pan[part]=64;
        if(part<3) {
            mc_preset(e,part,MC_PRESET_GLASS_PAD);
            McPatch *p=&e->project.patch[part];
            p->pcm_mix=p->fm_mix=p->va_mix=127;p->source_mask=HY_ALL;
            p->attack=0;p->sustain=127;p->release=0;p->level=127;
        }
        for(unsigned bank=0;bank<MC_FX_BANKS;bank++)e->project.fx_send[part][bank]=0;
    }
    for(unsigned i=0;i<MC_DRUMS;i++)e->project.drum_level[i]=127;
}
static int check_output(const int16_t *out,unsigned frames) {
    int nonzero=0;
    for(unsigned i=0;i<frames*2;i++) {
        assert(out[i]>=-MC_OUTPUT_CEILING&&out[i]<=MC_OUTPUT_CEILING);
        nonzero|=out[i]!=0;
    }
    return nonzero;
}
static void melody(McEngine *e) {
    for(unsigned part=0;part<3;part++)mc_note(e,part,48+12*part,127,0,0);
}
static void dry_partition(void) {
    setup(&a);melody(&a);mc_drum(&a,0,127);b=a;
    mc_render(&a,whole,4096);
    const unsigned sizes[]={1,3,16,128,127,2,255,17};
    for(unsigned offset=0,index=0;offset<4096;index++) {
        unsigned n=sizes[index%8];if(n>4096-offset)n=4096-offset;
        mc_render(&b,split+2*offset,n);offset+=n;
    }
    assert(!memcmp(whole,split,4096*2*sizeof *whole)&&!memcmp(&a,&b,sizeof a));
    assert(check_output(whole,4096)&&a.drum[0].age);
    assert(!a.shared_fx.rd_active&&!a.shared_fx.cf_active&&!a.extra_fx.rd_active&&!a.extra_fx.cf_active);
    for(unsigned i=0;i<3;i++)assert(a.voice[i].phase&&a.voice[i].phase2&&a.voice[i].pcm_pos);
}
static void drum_slots(void) {
    const unsigned sounds[]={0,1,4,5,6,7};
    setup(&a);assert(MC_DRUM_VOICES==6);
    for(unsigned i=0;i<MC_DRUM_VOICES;i++) {
        mc_drum(&a,sounds[i],127);
        unsigned active=0;
        for(unsigned j=0;j<MC_DRUM_VOICES;j++)active+=a.drum[j].env!=0;
        assert(active==i+1);mc_render(&a,whole,1);
    }
    mc_drum(&a,8,127);
    unsigned active=0,found=0;
    for(unsigned j=0;j<MC_DRUM_VOICES;j++) {
        active+=a.drum[j].env!=0;found+=a.drum[j].sound==8&&a.drum[j].env!=0;
    }
    assert(active==6&&found==1);
    mc_render(&a,whole,2048);assert(check_output(whole,2048));
    for(unsigned i=0;i<MC_VOICES;i++)assert(!a.voice[i].active);
}
static void four_banks_and_tail(void) {
    setup(&a);melody(&a);mc_drum(&a,0,127);b=a;
    assert(MC_FX_BANKS==4);
    for(unsigned bank=0;bank<MC_FX_BANKS;bank++) {
        a.project.fx_bus_type[bank]=(uint8_t)((bank&1u)?0:6);
        for(unsigned part=0;part<MC_PARTS;part++)a.project.fx_send[part][bank]=127;
    }
    mc_render(&a,whole,1024);mc_render(&b,split,1024);
    assert(check_output(whole,1024));
    assert(a.shared_fx.rd_active&&a.shared_fx.cf_active&&a.extra_fx.rd_active&&a.extra_fx.cf_active);
    assert(!b.shared_fx.rd_active&&!b.shared_fx.cf_active&&!b.extra_fx.rd_active&&!b.extra_fx.cf_active);
    for(unsigned p=0;p<4;p++) { mc_mute(&a,p);mc_mute(&b,p); }
    mc_render(&a,whole,8192);mc_render(&b,split,8192);
    assert(check_output(whole+4096*2,4096)&&!check_output(split+4096*2,4096));
    assert(memcmp(whole+4096*2,split+4096*2,4096*2*sizeof *whole));
    for(unsigned i=0;i<MC_VOICES;i++)assert(!a.voice[i].active&&!b.voice[i].active);
}
static void limiter(void) {
    setup(&a);melody(&a);
    for(unsigned i=3;i<MC_VOICES;i++)mc_note(&a,i%3,60+i,127,MC_ACCENT,0);
    for(unsigned i=0;i<MC_DRUM_VOICES;i++)mc_drum(&a,i+4,127);
    a.master=MC_MASTER_MAX;
    for(unsigned part=0;part<MC_PARTS;part++) {
        a.project.pan[part]=0;
        for(unsigned bank=0;bank<MC_FX_BANKS;bank++)a.project.fx_send[part][bank]=127;
    }
    for(unsigned bank=0;bank<MC_FX_BANKS;bank++)a.project.fx_bus_type[bank]=(uint8_t)((bank&1u)?0:6);
    for(unsigned block=0;block<4;block++) {
        mc_render(&a,whole,8192);assert(check_output(whole,8192));
    }
    assert(a.limiter_hits&&!a.clips);
    for(unsigned i=0;i<MC_VOICES;i++)assert(a.voice[i].active&&a.voice[i].hybrid.gain[0]&&a.voice[i].hybrid.gain[1]&&a.voice[i].hybrid.gain[2]);
}
int main(void) {
    dry_partition();drum_slots();four_banks_and_tail();limiter();
    puts("PASS mixer: actual engine, three parts/eight voices, six drums, four FX tails, limiter and partition state");
    return 0;
}
