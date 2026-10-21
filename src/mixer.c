/* SPDX-License-Identifier: GPL-3.0-only */
#include "mixer.h"
/* 원본 테스트와 같은 TU 포함 방식으로 실제 drum·route·FX 코드를 재사용한다. */
#include "engine.c"
#include <string.h>

void poc_init(PocMixer *m) {
    memset(m,0,sizeof *m);
    mc_init(&m->base);hy_init(&m->synth);
    m->base.project.demo=0;
    m->base.project.bpm=120;m->base.project.drum_kit=6;
    for(unsigned part=0;part<MC_PARTS;part++) {
        m->base.project.level[part]=110;m->base.project.pan[part]=64;
        for(unsigned bank=0;bank<MC_FX_BANKS;bank++)
            m->base.project.fx_send[part][bank]=0;
    }
    m->base.project.fx_bus_type[0]=5;m->base.project.fx_bus_type[1]=5;
    m->base.project.fx_bus_type[2]=4;m->base.project.fx_bus_type[3]=4;
}
void poc_drum(PocMixer *m,unsigned sound,unsigned velocity) {
    mc_drum(&m->base,sound,velocity);
    unsigned count=0;
    for(unsigned i=0;i<MC_DRUM_VOICES;i++)count+=m->base.drum[i].env!=0;
    if(count>m->peak_drums)m->peak_drums=count;
}
void poc_render(PocMixer *m,int16_t *stereo,unsigned frames) {
    for(unsigned offset=0;offset<frames;) {
        unsigned n=frames-offset;if(n>16)n=16;
        McRenderSlice slice;memset(&slice,0,sizeof slice);
        int32_t parts[16*HY_PARTS];
        hy_render(&m->synth,parts,n);
        slice.fx.frames=n;
        for(unsigned f=0;f<n;f++) {
            for(unsigned p=0;p<HY_PARTS;p++)slice.part[p][f]=parts[f*HY_PARTS+p];
            for(unsigned d=0;d<MC_DRUM_VOICES;d++) {
                if(m->base.drum[d].env)m->drum_frames++;
                slice.part[3][f]+=percussion(&m->base.random,m->base.project.drum_level,&m->base.drum[d]);
            }
            slice.mute[f]=m->base.mute;
        }
        render_route(&m->base,&slice,0,0);
        uint32_t ticks[MC_FX_BANKS]={0};
        unsigned processed=mc_fx_job_run(&m->base.shared_fx,&m->base.extra_fx,&slice.fx,ticks,0);
        for(unsigned bank=0;bank<MC_FX_BANKS;bank++)
            if(processed&(1u<<bank))m->bank_frames[bank]+=n;
        for(unsigned f=0;f<n;f++) {
            int32_t left=slice.dry[f][0],right=slice.dry[f][1];
            for(unsigned bank=0;bank<MC_FX_BANKS;bank++) {
                left+=mc_div_pow2(slice.fx.bus[bank][f][0],1);
                right+=mc_div_pow2(slice.fx.bus[bank][f][1],1);
            }
            left=mc_div3(left);right=mc_div3(right);
            mc_output(&m->base,&left,&right);
            stereo[(offset+f)*2]=(int16_t)left;
            stereo[(offset+f)*2+1]=(int16_t)right;
        }
        offset+=n;
    }
}
