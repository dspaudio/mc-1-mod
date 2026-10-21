/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 dspaudio */
#include "mcfm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static McEngine e;
static McProject p;
static uint8_t record[MC_RECORD_BYTES];
static uint8_t current[MC_RECORD_BYTES];
enum { W_STEP=20+8+4+3*15+24+8,
       W_AUTOMATION=W_STEP+MC_PATTERNS*512+16+3+18+20+10+3,
       W_SONG=W_AUTOMATION+MC_AUTOMATION*5, W_FX=20+MC_PROJECT_BYTES-23 };
static void clocks(unsigned n) { while(n--) mc_clock_tick(&e); }
static void crc(void) {
    uint32_t v=mc_crc(record+20,record[6]+((unsigned)record[7]<<8));
    for(unsigned i=0;i<4;i++) record[12+i]=(uint8_t)(v>>(i*8));
    v=mc_crc(record,16);for(unsigned i=0;i<4;i++) record[16+i]=(uint8_t)(v>>(i*8));
}
static unsigned legacy(unsigned version) {
    static const unsigned sizes[7]={2112,2120,2136,3726,34830,35094,35104};
    unsigned body=sizes[version-1];
    assert(mc_encode(&e.project,77,current,sizeof current)==MC_RECORD_BYTES);
    memset(record,0,sizeof record);
    if(version>=5) memcpy(record,current,body+20);
    else {
        memcpy(record,current,20+64);
        if(version==1) memcpy(record+20+64,current+20+72,2048);
        else memcpy(record+20+64,current+20+64,2056);
        if(version>=3) memcpy(record+20+2120,current+20+32840,16);
        if(version==4) {
            memcpy(record+20+2136,current+20+32856,54);
            for(unsigned i=0;i<MC_AUTOMATION;i++) {
                const uint8_t *in=current+20+32910+i*5;uint8_t *out=record+20+2190+i*4;
                out[0]=in[0];memcpy(out+1,in+2,3);
            }
        }
    }
    record[4]=(uint8_t)version;record[6]=(uint8_t)body;record[7]=(uint8_t)(body>>8);crc();
    return body+20;
}
static void shared_fx_validation(void) {
    static const unsigned sends[MC_FX_BANKS]={MC_PARAM_RD_SEND,MC_PARAM_CF_SEND,MC_PARAM_RD2_SEND,MC_PARAM_CF2_SEND};
    static const unsigned types[MC_FX_BANKS]={MC_PARAM_RD_TYPE,MC_PARAM_CF_TYPE,MC_PARAM_RD2_TYPE,MC_PARAM_CF2_TYPE};
    uint32_t seq=0;
    mc_init(&e);
    for(unsigned t=0;t<MC_PARTS;t++) for(unsigned bus=0;bus<MC_FX_BANKS;bus++) {
        e.project.fx_send[t][bus]=(uint8_t)(t*27+bus*13);
    }
    for(unsigned bus=0;bus<MC_FX_BANKS;bus++) e.project.fx_bus_type[bus]=(uint8_t)(7-bus);
    e.project.automation_count=20;
    for(unsigned t=0;t<4;t++) for(unsigned bus=0;bus<MC_FX_BANKS;bus++)
        e.project.automation[t*MC_FX_BANKS+bus]=(McAutomation){(uint16_t)(63*5+t),15,(uint8_t)sends[bus],127};
    for(unsigned bus=0;bus<MC_FX_BANKS;bus++)
        e.project.automation[16+bus]=(McAutomation){319,15,(uint8_t)types[bus],7};
    assert(mc_encode(&e.project,99,record,sizeof record)==MC_RECORD_BYTES&&record[4]==MC_PROJECT_VERSION);
    for(unsigned t=0;t<MC_PARTS;t++) {
        assert(!memcmp(record+W_FX+t*2,e.project.fx_send[t],2));
        assert(!memcmp(record+W_FX+10+t*2,e.project.fx_send[t]+2,2));
    }
    assert(!memcmp(record+W_FX+8,e.project.fx_bus_type,2));
    assert(!memcmp(record+W_FX+18,e.project.fx_bus_type+2,2));
    assert(!mc_decode(&p,&seq,record,sizeof record)&&seq==99&&!memcmp(&p,&e.project,sizeof p));
    McAutomation saved=e.project.automation[9];
    e.project.automation[9]=e.project.automation[8];
    assert(!mc_project_valid(&e.project));
    e.project.automation[9]=saved;
    memcpy(record+W_AUTOMATION+9*5,record+W_AUTOMATION+8*5,5);crc();
    assert(mc_record_valid(record,sizeof record,0)<0);
    assert(mc_encode(&e.project,99,record,sizeof record)>0);
    for(unsigned i=0;i<20;i++) {
        unsigned off=W_FX+i;uint8_t saved=record[off];
        record[off]=(uint8_t)(i%10<8?128:8);crc();
        assert(mc_record_valid(record,sizeof record,0)<0&&mc_decode(&p,&seq,record,sizeof record)<0);
        record[off]=saved;crc();
    }
    for(unsigned t=0;t<4;t++) for(unsigned bus=0;bus<MC_FX_BANKS;bus++) {
        uint8_t saved=e.project.fx_send[t][bus];e.project.fx_send[t][bus]=128;
        assert(!mc_project_valid(&e.project)&&mc_encode(&e.project,0,record,sizeof record)<0);
        e.project.fx_send[t][bus]=saved;
    }
    for(unsigned bus=0;bus<MC_FX_BANKS;bus++) {
        e.project.fx_bus_type[bus]=8;assert(!mc_project_valid(&e.project));e.project.fx_bus_type[bus]=7;
    }
    for(unsigned part=0;part<3;part++) mc_preset(&e,part,8);
    e.project.automation_count=1;memset(e.project.automation,0,sizeof e.project.automation);
    for(unsigned param=MC_PARAM_DRUM_LEVEL;param<MC_PARAMETERS;param++) for(unsigned part=0;part<5;part++) {
        unsigned type=param==MC_PARAM_RD_TYPE||param==MC_PARAM_CF_TYPE||param==MC_PARAM_RD2_TYPE||param==MC_PARAM_CF2_TYPE;
        int valid=param<MC_PARAM_RD_SEND?part==3:type?part==4:param>=MC_PARAM_ANALOG?part<3:part<4;
        e.project.automation[0]=(McAutomation){(uint16_t)part,0,(uint8_t)param,(uint8_t)(type||param==MC_PARAM_SOURCE_MASK?7:127)};
        assert(mc_project_valid(&e.project)==valid);
        if(valid) {
            assert(mc_encode(&e.project,0,record,sizeof record)>0);
            assert(!mc_record_valid(record,sizeof record,0));
            record[W_AUTOMATION]=(uint8_t)(param<MC_PARAM_RD_SEND?0:type?3:4);crc();
            assert(mc_record_valid(record,sizeof record,0)<0);
            assert(mc_encode(&e.project,0,record,sizeof record)>0);
            record[W_AUTOMATION+4]=(uint8_t)(type?8:128);crc();
            assert(mc_record_valid(record,sizeof record,0)<0);
            e.project.automation[0].value=(uint8_t)(type?8:128);
            assert(!mc_project_valid(&e.project));
        }
    }
    puts("PASS: v9 four-bank FX append, full roundtrip, send/type bounds and canonical automation scope");
}
static void fx_migration(void) {
    /* 구형 팩을 임의 이관하지 않는 계약: FX 조합과 관계없이 원자적으로 거부한다. */
    for(unsigned version=1;version<=7;version++) for(unsigned family=0;family<2;family++)
    for(unsigned type=0;type<4;type++) for(unsigned mod=0;mod<3;mod++) {
        mc_init(&e);e.project.track_fx[1][family]=60;e.project.track_fx[2][3]=50;
        e.project.track_fx_type[1][family]=(uint8_t)type;
        e.project.track_fx_type[2][3]=(uint8_t)mod;
        unsigned n=legacy(version);memcpy(record,"MCFM",4);crc();
        uint32_t seq=77,hash=mc_crc(&p,sizeof p);
        assert(mc_record_valid(record,n,&seq)<0&&seq==77);
        assert(mc_decode(&p,&seq,record,n)<0&&seq==77&&mc_crc(&p,sizeof p)==hash);
    }
    mc_init(&e);
    for(unsigned version=1;version<MC_PROJECT_VERSION;version++) {
        assert(mc_encode(&e.project,77,record,sizeof record)==MC_RECORD_BYTES);
        record[4]=(uint8_t)version;crc();
        uint32_t seq=77,hash=mc_crc(&p,sizeof p);
        assert(mc_decode(&p,&seq,record,sizeof record)<0&&seq==77&&mc_crc(&p,sizeof p)==hash);
    }
    puts("PASS: legacy FX families/types and unsupported versions reject without project/sequence mutation");
}
int main(void) {
    shared_fx_validation();fx_migration();
    mc_init(&e);assert(mc_project_valid(&e.project));
    assert(!mc_song_set(&e,0,4,2)&&!mc_song_set(&e,1,63,1));
    assert(mc_song_set(&e,128,0,1)<0&&mc_song_set(&e,0,64,1)<0&&mc_song_set(&e,0,0,0)<0);
    mc_song_mode(&e,1);mc_start(&e);assert(!e.playing);
    assert(!mc_song_length(&e,2));mc_clock_source(&e,1);mc_start(&e);
    assert(e.pattern==4&&e.song_position==0&&e.step==0);
    clocks(96);assert(e.pattern==4&&e.song_repeat==1&&e.playing);
    clocks(96);assert(e.pattern==63&&e.song_position==1&&e.song_repeat==0);
    clocks(95);assert(e.playing);clocks(1);assert(!e.playing);
    e.project.song_loop=1;mc_start(&e);clocks(288);assert(e.playing&&e.pattern==4&&e.song_position==0);
    clocks(18);mc_stop(&e);unsigned step=e.step;mc_continue(&e);assert(e.step==step&&e.playing);
    mc_queue(&e,10);assert(e.queued==255);mc_song_mode(&e,0);assert(!e.playing);
    mc_start(&e);mc_queue(&e,42);clocks((16-e.step)*6);assert(e.pattern==42);
    mc_stop(&e);assert(!mc_song_length(&e,128));
    for(unsigned i=0;i<128;i++) assert(!mc_song_set(&e,i,i%64,i%16+1));
    e.project.song_mode=1;e.project.midi_channel[0]=15;e.project.clock_source=2;
    assert(mc_encode(&e.project,99,record,sizeof record)==MC_RECORD_BYTES&&record[4]==MC_PROJECT_VERSION);
    uint32_t sequence=0;assert(!mc_decode(&p,&sequence,record,sizeof record)&&sequence==99);
    assert(!memcmp(&p,&e.project,sizeof p));
    record[W_SONG+1]=0;crc();assert(mc_decode(&p,&sequence,record,sizeof record)<0);
    assert(mc_encode(&e.project,99,record,sizeof record)>0);
    record[4]=5;record[6]=34830&255;record[7]=34830>>8;crc();
    uint32_t hash=mc_crc(&p,sizeof p);
    assert(mc_decode(&p,&sequence,record,34850)<0&&sequence==99&&mc_crc(&p,sizeof p)==hash);
    for(unsigned flag=MC_SLIDE;flag<=MC_TIE;flag*=2) for(unsigned scenario=0;scenario<5;scenario++) {
        mc_init(&e);memset(e.project.step,0,sizeof e.project.step);e.project.swing=0;
        /* 오디오를 진행하지 않는 clock 시험은 이전 노트를 재사용하는 mono로 고정한다. */
        mc_preset(&e,0,MC_PRESET_SLIDE_BASS);
        mc_song_set(&e,0,4,(scenario==1||scenario==4)?2:1);mc_song_set(&e,1,63,1);mc_song_length(&e,2);
        mc_song_mode(&e,1);mc_clock_source(&e,1);e.project.song_loop=(uint8_t)(scenario==3);
        unsigned source=(scenario==2||scenario==3)?63:4;
        unsigned target=scenario==4?63:scenario==1?4:scenario>=2?4:63;
        e.project.step[source][0][15]=(McStep){{60,0,0},100,25,0,0};
        e.project.step[target][0][0]=(McStep){{60,0,0},100,75,(uint8_t)flag,0};
        mc_start(&e);if(scenario==4) mc_song_set(&e,0,63,2);
        clocks((scenario==2||scenario==3)?186:90);assert(e.step==15&&e.pattern==source);
        unsigned found=0;uint32_t samples=MC_RATE*15u/e.project.bpm;
        for(unsigned v=0;v<MC_VOICES;v++) if(e.voice[v].active&&e.voice[v].part==0&&e.voice[v].note==60) {
            assert(e.voice[v].gate_left==(scenario==2?samples/4:samples+2));found=1;
        }
        assert(found);
    }
    printf("PASS song: repeats, bar boundaries, end, loop, continue, 128 entries, current roundtrip/legacy rejection; record=%u\n",MC_RECORD_BYTES);
}
