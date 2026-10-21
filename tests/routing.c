/* SPDX-License-Identifier: GPL-3.0-only */
#include "../src/engine.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static McEngine engine;
static McRenderSlice slice;
static McSynthJob synth;
static uint32_t random_state=0x197a203du;
static int32_t sample(void) {
    random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;
    return (int32_t)(random_state%120001u)-60000;
}
static int16_t prefix[4096*2],reference[2048*2],dry_reference[2048*2],changed[2048*2];
static McProject expected_project;
static McSharedFx history_before[2];

static void preset_fx_setup(unsigned part,unsigned palette,unsigned dry) {
    static const uint8_t custom[MC_FX_BANKS]={2,4,6,3};
    mc_init(&engine);
    mc_preset(&engine,part,20);
    mc_preset(&engine,(part+1u)%3u,23);
    engine.project.patch[part].release=0;
    for(unsigned p=0;p<MC_PARTS;p++) {
        engine.project.level[p]=(uint8_t)(64+p*13);
        engine.project.pan[p]=(uint8_t)(17+p*29);
        for(unsigned bank=0;bank<MC_FX_BANKS;bank++) {
            engine.project.fx_send[p][bank]=(uint8_t)(dry?0:1+(p*23+bank*11+31)%127);
            engine.project.track_fx[p][bank]=(uint8_t)(17+p*19+bank*7);
            engine.project.track_fx_type[p][bank]=(uint8_t)((p+bank+3)%8);
        }
        engine.project.track_filter[p][0]=(uint8_t)(31+p*9);
        engine.project.track_filter[p][1]=(uint8_t)(76-p*7);
    }
    for(unsigned bank=0;bank<MC_FX_BANKS;bank++) {
        engine.project.fx_bus_type[bank]=palette?custom[bank]:0;
        engine.project.fx[bank]=(uint8_t)(29+bank*13);
        engine.project.fx_type[bank]=(uint8_t)((bank+5)%8);
    }
    engine.project.filter[0]=55;engine.project.filter[1]=71;
    mc_note(&engine,part,72,105,0,MC_RATE/100);
    mc_note(&engine,(part+1u)%3u,60,108,0,MC_RATE/8);
    mc_drum(&engine,0,116);
    mc_render(&engine,prefix,4096);
    unsigned nonzero=0;
    for(unsigned i=0;i<4096*2;i++)nonzero|=prefix[i]!=0;
    assert(nonzero);
    for(unsigned i=0;i<MC_VOICES;i++)
        assert(!engine.voice[i].active||engine.voice[i].part!=part);
    if(!dry)assert(engine.shared_fx.rd_active&&engine.shared_fx.cf_active&&
        engine.extra_fx.rd_active&&engine.extra_fx.cf_active);
}

static void preset_fx_preservation(void) {
    for(unsigned part=0;part<3;part++)for(unsigned palette=0;palette<2;palette++) {
        preset_fx_setup(part,palette,0);
        mc_render(&engine,reference,2048);
        preset_fx_setup(part,palette,1);
        mc_render(&engine,dry_reference,2048);
        assert(memcmp(reference,dry_reference,sizeof reference));
        for(unsigned preset=0;preset<MC_PRESETS;preset++) {
            preset_fx_setup(part,palette,0);
            memcpy(&expected_project,&engine.project,sizeof expected_project);
            expected_project.patch[part]=mc_presets[preset];
            expected_project.preset[part]=(uint8_t)preset;
            memcpy(expected_project.fx_send[part],mc_preset_fx_send[preset],MC_FX_BANKS);
            memcpy(&history_before[0],&engine.shared_fx,sizeof engine.shared_fx);
            memcpy(&history_before[1],&engine.extra_fx,sizeof engine.extra_fx);
            mc_preset(&engine,part,preset);
            assert(!memcmp(&engine.project,&expected_project,sizeof expected_project));
            assert(!memcmp(&engine.shared_fx,&history_before[0],sizeof engine.shared_fx));
            assert(!memcmp(&engine.extra_fx,&history_before[1],sizeof engine.extra_fx));
            mc_render(&engine,changed,2048);
            assert(!memcmp(reference,changed,sizeof reference));
        }
    }
    puts("PASS preset FX: all32 x three parts x two custom palettes preserve project fields, four histories and live tails");
}
int main(void) {
    mc_init(&engine);
    for(unsigned trial=0;trial<256;trial++) {
        unsigned frames=trial%16u+1u;
        memset(&slice,0,sizeof slice);memset(&synth,0,sizeof synth);
        slice.fx.frames=frames;synth.frames=frames;
        for(unsigned p=0;p<MC_PARTS;p++) {
            engine.project.level[p]=(uint8_t)((trial+p*31u)%128u);
            engine.project.pan[p]=(uint8_t)((trial*3u+p*43u)%128u);
            for(unsigned bank=0;bank<MC_FX_BANKS;bank++) {
                unsigned mode=(trial+p+bank)%3u;
                engine.project.fx_send[p][bank]=(uint8_t)(mode==0?0:mode==1?127:(trial*7u+p*19u+bank)%128u);
            }
            for(unsigned f=0;f<frames;f++) {
                slice.part[p][f]=sample();
                if(p<3)synth.part[p][f]=sample();else synth.sample[f]=sample();
            }
        }
        for(unsigned f=0;f<frames;f++)slice.mute[f]=(uint8_t)((trial+f)%16u);
        for(unsigned bank=0;bank<MC_FX_BANKS;bank++)
            engine.project.fx_bus_type[bank]=(uint8_t)((trial+bank)%8u);
        int32_t dry[16][2]={0},wet[MC_FX_BANKS][16][2]={0};
        unsigned sends=0;
        for(unsigned p=0;p<MC_PARTS;p++)for(unsigned bank=0;bank<MC_FX_BANKS;bank++)
            if(engine.project.fx_send[p][bank])sends|=1u<<bank;
        for(unsigned f=0;f<frames;f++)for(unsigned p=0;p<MC_PARTS;p++) {
            if((slice.mute[f]>>p)&1u)continue;
            int32_t mono=slice.part[p][f];
            if(trial&1u)mono=p<3?mono+synth.part[p][f]:synth.sample[f];
            int32_t v=mono*engine.project.level[p]/127;
            int32_t stereo[2]={v*(127-engine.project.pan[p])/127,v*engine.project.pan[p]/127};
            for(unsigned c=0;c<2;c++) {
                dry[f][c]+=stereo[c];
                for(unsigned bank=0;bank<MC_FX_BANKS;bank++)
                    wet[bank][f][c]+=stereo[c]*engine.project.fx_send[p][bank]/127;
            }
        }
        render_route(&engine,&slice,trial&1u?&synth:0,0);
        assert(slice.fx.sends==sends&&slice.fx.bpm==engine.project.bpm&&!slice.fx.skip);
        assert(!memcmp(slice.fx.type,engine.project.fx_bus_type,MC_FX_BANKS));
        assert(!memcmp(slice.dry,dry,frames*sizeof dry[0]));
        for(unsigned bank=0;bank<MC_FX_BANKS;bank++)
            assert(!memcmp(slice.fx.bus[bank],wet[bank],frames*sizeof wet[bank][0]));
    }
    puts("PASS routing: independent signed-division oracle, all gains/pans, zero/unity/mixed sends, mute and worker sums");
    preset_fx_preservation();
    return 0;
}
