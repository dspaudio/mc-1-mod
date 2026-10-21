/* SPDX-License-Identifier: GPL-3.0-only */
#include "mcfm.h"
#include "store.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define BASE (MC_STORE_A-MC_SECTOR)
#define TOTAL (MC_STORE_BYTES*2+MC_SECTOR*2)
static uint8_t flash[TOTAL],snapshot[TOTAL];
static McProject original,changed,loaded,before;
static uint8_t encoded[MC_RECORD_BYTES],other[MC_RECORD_BYTES];
enum { W_PATCH=20+8+4, W_STEP=W_PATCH+3*15+24+8,
       W_PRESET=W_STEP+MC_PATTERNS*4*16*8+16,
       W_AUTOMATION=W_PRESET+3+18+20+10+3 };
static int budget=-1,corrupt_write,fail_verify;
static uint32_t unreadable;
static unsigned erased,programmed,reads,max_read,erase_calls,program_calls;
static int read_flash(uint32_t off,void *out,unsigned n) {
    assert(off>=MC_STORE_A&&off<=MC_STORE_END&&n<=MC_STORE_END-off);
    reads++;if(n>max_read)max_read=n;
    if(unreadable==UINT32_MAX||(unreadable&&off>=unreadable&&off<unreadable+MC_STORE_BYTES)||
       (fail_verify&&programmed==MC_RECORD_BYTES))return -1;
    memcpy(out,flash+(off-BASE),n);return 0;
}
static int byte(uint32_t off,uint8_t value,int erase) {
    assert(off>=MC_STORE_A&&off<MC_STORE_END);
    if(budget==0)return -1;
    if(budget>0)budget--;
    uint8_t *p=flash+(off-BASE);
    if(!erase)assert((*p&value)==value);
    *p=erase?255:value;return 0;
}
static int erase_flash(uint32_t off) {
    erase_calls++;
    assert(off>=MC_STORE_A&&off<MC_STORE_END&&off%MC_SECTOR==0);
    for(unsigned i=0;i<MC_SECTOR;i++)if(byte(off+i,255,1))return -1;
    erased++;return 0;
}
static int program_flash(uint32_t off,const void *data,unsigned n) {
    program_calls++;
    const uint8_t *p=data;
    assert(off>=MC_STORE_A&&off<=MC_STORE_END&&n<=MC_STORE_END-off);
    assert(erased==9);
    if(off==MC_STORE_A||off==MC_STORE_B)assert(programmed==MC_PROJECT_BYTES&&n==20);
    else assert((off==MC_STORE_A+20||off==MC_STORE_B+20)&&programmed==0&&n==MC_PROJECT_BYTES);
    for(unsigned i=0;i<n;i++) {
        if(byte(off+i,p[i],0))return -1;
        programmed++;
    }
    if(corrupt_write&&n>20)flash[off-BASE+99]^=1;
    return 0;
}
static const McStorage io={read_flash,erase_flash,program_flash};
static void reset_io(void) { budget=-1;corrupt_write=fail_verify=0;unreadable=0;erased=programmed=reads=max_read=erase_calls=program_calls=0; }
static void guards(void) {
    for(unsigned i=0;i<MC_SECTOR;i++)assert(flash[i]==0xa5&&flash[TOTAL-MC_SECTOR+i]==0xa5);
}
static void cut_at(unsigned cutoff) {
    memcpy(flash,snapshot,sizeof flash);reset_io();budget=(int)cutoff;
    assert(mc_store_save(&io,&changed)<0);assert(budget==0);budget=-1;
    uint32_t seq=999;
    assert(mc_store_load(&io,&loaded,&seq)==0&&seq==1);
    assert(!memcmp(&loaded,&original,sizeof loaded));guards();
}
static void put32(uint8_t *p,uint32_t value) {
    for(unsigned i=0;i<4;i++) p[i]=(uint8_t)(value>>(i*8));
}
static void checksums(uint8_t *p) {
    put32(p+12,mc_crc(p+20,MC_PROJECT_BYTES));
    put32(p+16,mc_crc(p,16));
}
static void reject(unsigned n) {
    before=loaded;uint32_t seq=0x12345678u;
    assert(mc_record_valid(encoded,n,&seq)<0&&seq==0x12345678u);
    assert(mc_decode(&loaded,&seq,encoded,n)<0&&seq==0x12345678u);
    assert(!memcmp(&before,&loaded,sizeof loaded));
}
static void wire_tests(void) {
    assert(mc_encode(&original,71,encoded,sizeof encoded)==MC_RECORD_BYTES);
    assert(!memcmp(encoded,"MCHY",4)&&encoded[4]==9&&!encoded[5]);
    assert(!memcmp(encoded+20,"MOD1\1\0\1\0",8));
    assert(!memcmp(encoded+20,mc_pack_id,8));
    assert((encoded[6]|(unsigned)encoded[7]<<8)==35134);
    memcpy(other,encoded,sizeof other);
    for(unsigned n=0;n<MC_RECORD_BYTES;n++) reject(n);
    reject(MC_RECORD_BYTES+1);
    for(unsigned i=0;i<28;i++) {
        encoded[i]^=1;reject(sizeof encoded);
        memcpy(encoded,other,sizeof encoded);
    }
    /* CRC가 올바른 외래 identity도 현재 스키마로 해석하지 않는다. */
    for(unsigned i=20;i<28;i++) {
        encoded[i]^=1;checksums(encoded);reject(sizeof encoded);
        memcpy(encoded,other,sizeof encoded);
    }
    for(unsigned version=1;version<9;version++) {
        encoded[4]=(uint8_t)version;checksums(encoded);reject(sizeof encoded);
        memcpy(encoded,other,sizeof encoded);
    }
    put32(encoded,0x4d46434du);checksums(encoded);reject(sizeof encoded);
    memcpy(encoded,other,sizeof encoded);
    encoded[6]=encoded[7]=255;checksums(encoded);reject(sizeof encoded);
    memcpy(encoded,other,sizeof encoded);
    for(unsigned t=0;t<3;t++) for(unsigned i=0;i<15;i++) {
        encoded[32+t*15+i]=255;checksums(encoded);reject(sizeof encoded);
        memcpy(encoded,other,sizeof encoded);
    }
    /* preset origin, 마지막 step, song, MIDI, 마지막 Analog까지 검증한다. */
    const unsigned bad_offsets[]={85,105,W_PRESET-16-3,W_PRESET,
        W_AUTOMATION-3,W_AUTOMATION-1,W_AUTOMATION,
        W_AUTOMATION+MC_AUTOMATION*5+1,35123,35127,35130,35153};
    for(unsigned i=0;i<sizeof bad_offsets/sizeof bad_offsets[0];i++) {
        encoded[bad_offsets[i]]=254;checksums(encoded);reject(sizeof encoded);
        memcpy(encoded,other,sizeof encoded);
    }
    for(unsigned profile=0;profile<MC_PRESETS;profile++) {
        changed=original;changed.patch[0]=mc_presets[profile];changed.preset[0]=(uint8_t)profile;
        changed.patch[0].cutoff^=1;
        changed.midi_channel[0]=15;changed.midi_channel[1]=0;
        changed.midi_channel[2]=7;changed.midi_channel[3]=9;changed.clock_source=3;
        for(unsigned custom=0;custom<2;custom++) {
            if(custom) changed.preset[0]=255;
            assert(mc_encode(&changed,profile,encoded,sizeof encoded)>0);
            uint32_t seq=99;
            assert(!mc_decode(&loaded,&seq,encoded,sizeof encoded)&&seq==profile);
            assert(!memcmp(&loaded,&changed,sizeof loaded));
        }
    }
    changed=original;changed.automation_count=1;
    changed.automation[0]=(McAutomation){0,15,MC_PARAM_MODE,MC_MONO};
    assert(mc_encode(&changed,9,encoded,sizeof encoded)>0);
    memcpy(other,encoded,sizeof other);
    const unsigned bad_params[]={MC_PARAM_PROFILE,MC_PARAMETERS,255};
    for(unsigned i=0;i<sizeof bad_params/sizeof bad_params[0];i++) {
        encoded[W_AUTOMATION+3]=(uint8_t)bad_params[i];checksums(encoded);reject(sizeof encoded);
        memcpy(encoded,other,sizeof encoded);
    }
    for(unsigned param=MC_PARAM_FM_MIX;param<=MC_PARAM_SOURCE_MASK;param++) {
        changed.patch[0]=mc_presets[8];changed.preset[0]=8;
        changed.automation[0].parameter=(uint8_t)param;
        changed.automation[0].value=param==MC_PARAM_SOURCE_MASK?7:127;
        assert(mc_encode(&changed,9,encoded,sizeof encoded)>0);
        uint32_t seq=0;assert(!mc_decode(&loaded,&seq,encoded,sizeof encoded));
        assert(!memcmp(&loaded,&changed,sizeof loaded));
        encoded[W_AUTOMATION+4]=128;checksums(encoded);reject(sizeof encoded);
        changed.automation[0].address=3;assert(!mc_project_valid(&changed));
        changed.automation[0].address=0;
    }
    changed=original;changed.automation_count=1;
    changed.automation[0]=(McAutomation){0,0,MC_PARAM_FM_MIX,1};
    assert(!mc_project_valid(&changed));
    changed.automation[0].parameter=MC_PARAM_SOURCE_MASK;
    changed.automation[0].value=1;assert(!mc_project_valid(&changed));
    assert(mc_encode(&original,1,encoded,sizeof encoded)>0);
    memcpy(other,encoded,sizeof other);
    const unsigned unavailable[]={3,12,14};
    for(unsigned i=0;i<sizeof unavailable/sizeof unavailable[0];i++) {
        encoded[W_PATCH+unavailable[i]]=1;checksums(encoded);reject(sizeof encoded);
        memcpy(encoded,other,sizeof encoded);
    }
    changed=original;changed.automation_count=2;
    changed.automation[0]=(McAutomation){0,0,MC_PARAM_MODE,MC_MONO};
    changed.automation[1]=changed.automation[0];
    assert(mc_encode(&changed,1,encoded,sizeof encoded)<0);
    changed.automation_count=1;assert(!mc_project_valid(&changed));
    changed=original;changed.preset[0]=1;assert(!mc_project_valid(&changed));
    assert(mc_encode(&original,1,encoded,sizeof encoded)>0);
    memcpy(other,encoded,sizeof other);encoded[20]^=1;checksums(encoded);
    uint32_t seq=99;assert(mc_recover(&loaded,&seq,encoded,other)==1&&seq==1);
    before=loaded;assert(mc_recover(&loaded,&seq,encoded,encoded)==-3&&seq==1);
    assert(!memcmp(&loaded,&before,sizeof loaded));
    memset(encoded,255,sizeof encoded);
    assert(mc_recover(&loaded,&seq,encoded,encoded)==-1&&seq==1);
    assert(!memcmp(&loaded,&before,sizeof loaded));
    changed=original;changed.bpm=142;
    changed.step[63][2][15].note[0]=99;changed.step[63][2][15].velocity=101;changed.step[63][2][15].gate=75;
    puts("PASS: current wire, 32 profiles/custom origins/MIDI, schema/revision/CRC/length/parameter rejection is atomic");
}
static void bank_state(unsigned bank,int state) {
    uint8_t *p=flash+((bank?MC_STORE_B:MC_STORE_A)-BASE);
    memset(p,255,MC_STORE_BYTES);
    if(state==MC_STORE_EMPTY) return;
    assert(mc_encode(&original,bank+1,p,MC_RECORD_BYTES)>0);
    if(state==MC_STORE_CORRUPT_OWN) p[100]^=1;
    if(state==MC_STORE_UNSUPPORTED) {p[20]^=1;checksums(p);}
}
static void status_tests(void) {
    for(int a=0;a<5;a++) for(int b=0;b<5;b++) {
        reset_io();bank_state(0,a);bank_state(1,b);
        if(a==MC_STORE_IO_ERROR) unreadable=MC_STORE_A;
        if(b==MC_STORE_IO_ERROR) unreadable=MC_STORE_B;
        if(a==MC_STORE_IO_ERROR&&b==MC_STORE_IO_ERROR) unreadable=UINT32_MAX;
        int expected=a==4||b==4?4:a==3||b==3?3:a==0||b==0?0:a==2||b==2?2:1;
        before=loaded;memcpy(snapshot,flash,sizeof flash);
        assert(mc_store_status(&io)==expected&&reads&&max_read<=256);
        assert(!memcmp(&before,&loaded,sizeof loaded)&&!memcmp(snapshot,flash,sizeof flash));
        assert(!erase_calls&&!program_calls);
        uint32_t seq=777;
        int rc=mc_store_load(&io,&loaded,&seq);
        if(a==0||b==0) assert(rc==(b==0?1:0)&&seq==(b==0?2u:1u));
        else {
            assert(rc==(expected==3?-3:-1)&&seq==777);
            assert(!memcmp(&loaded,&before,sizeof loaded));
        }
        if(expected==3||expected==4) {
            assert(mc_store_save(&io,&original)<0&&!erase_calls&&!program_calls);
            assert(!memcmp(snapshot,flash,sizeof flash));
        }
    }
    reset_io();
    const unsigned foreign_fields[]={0,4,5,16,20,21,22,23,24,25,26,27};
    for(unsigned i=0;i<sizeof foreign_fields/sizeof foreign_fields[0];i++) {
        bank_state(0,MC_STORE_CURRENT);bank_state(1,MC_STORE_CURRENT);
        uint8_t *p=flash+(MC_STORE_B-BASE);p[foreign_fields[i]]^=1;
        if(foreign_fields[i]!=16) checksums(p);
        memcpy(snapshot,flash,sizeof flash);
        assert(mc_store_status(&io)==MC_STORE_UNSUPPORTED);
        uint32_t seq=999;
        assert(mc_store_load(&io,&loaded,&seq)==0&&seq==1);
        assert(mc_store_save(&io,&original)<0&&!erase_calls&&!program_calls);
        assert(!memcmp(snapshot,flash,sizeof flash));
    }
    /* record 뒤 잔여 영역의 IO도 외래 상태보다 우선하며 SAVE를 잠근다. */
    bank_state(0,MC_STORE_UNSUPPORTED);bank_state(1,MC_STORE_CURRENT);
    unreadable=MC_STORE_B+MC_STORE_BYTES-256;
    assert(mc_store_status(&io)==MC_STORE_IO_ERROR);
    assert(mc_store_save(&io,&original)<0&&!erase_calls&&!program_calls);
    reset_io();bank_state(0,MC_STORE_CURRENT);bank_state(1,MC_STORE_EMPTY);
    changed=original;changed.patch[0].profile=MC_PRESETS;
    memcpy(snapshot,flash,sizeof flash);
    assert(mc_store_save(&io,&changed)<0&&!erase_calls&&!program_calls);
    assert(!memcmp(snapshot,flash,sizeof flash));
    bank_state(0,MC_STORE_EMPTY);bank_state(1,MC_STORE_CURRENT);
    uint8_t *p=flash+(MC_STORE_B-BASE);
    p[6]=p[7]=255;checksums(p);
    assert(mc_store_status(&io)==MC_STORE_CORRUPT_OWN);
    before=loaded;uint32_t seq=999;
    assert(mc_store_load(&io,&loaded,&seq)==-1&&seq==999);
    assert(!memcmp(&loaded,&before,sizeof loaded));
    bank_state(1,MC_STORE_CURRENT);
    p[W_AUTOMATION]=1;checksums(p);
    assert(mc_store_status(&io)==MC_STORE_CORRUPT_OWN);
    assert(mc_store_load(&io,&loaded,&seq)==-1&&seq==999);
    assert(!memcmp(&loaded,&before,sizeof loaded));
    reset_io();bank_state(0,MC_STORE_EMPTY);bank_state(1,MC_STORE_EMPTY);
    flash[MC_STORE_END-BASE-1]=0;
    assert(mc_store_status(&io)==MC_STORE_UNSUPPORTED);
    assert(mc_store_save(&io,&original)<0&&!erase_calls&&!program_calls);
    /* header가 부분 기록되었거나 CRC가 깨지면 소유권을 주장하지 않는다. */
    for(unsigned cut=0;cut<20;cut++) {
        bank_state(0,MC_STORE_CURRENT);bank_state(1,MC_STORE_EMPTY);
        memset(flash+(MC_STORE_A-BASE)+cut,255,20-cut);
        assert(mc_store_status(&io)==MC_STORE_UNSUPPORTED);
        assert(mc_store_save(&io,&original)<0&&!erase_calls&&!program_calls);
    }
    bank_state(0,MC_STORE_CORRUPT_OWN);bank_state(1,MC_STORE_EMPTY);
    assert(mc_store_status(&io)==MC_STORE_CORRUPT_OWN);
    assert(!mc_store_save(&io,&original));
    assert(mc_store_status(&io)==MC_STORE_CURRENT);
    puts("PASS: 25 status pairs, full-bank EMPTY, partial header protection, zero writes for foreign/IO, own-corrupt recovery");
}
int main(void) {
    assert(MC_RECORD_BYTES==35154&&MC_STORE_BYTES==36864);
    assert(MC_STORE_CURRENT==0&&MC_STORE_EMPTY==1&&MC_STORE_CORRUPT_OWN==2&&MC_STORE_UNSUPPORTED==3&&MC_STORE_IO_ERROR==4);
    assert(MC_STORE_A==0x99000&&MC_STORE_B==0xa2000&&MC_STORE_END==0xab000);
    mc_factory(&original);
    original.fx_send[0][0]=127;original.fx_send[3][1]=93;
    original.fx_bus_type[0]=7;original.fx_bus_type[1]=5;
    original.fx_send[1][2]=127;original.fx_send[2][3]=84;
    original.fx_bus_type[2]=6;original.fx_bus_type[3]=7;
    original.analog[0]=127;original.analog[1]=73;original.analog[2]=31;
    changed=original;changed.bpm=142;
    changed.step[63][2][15].note[0]=99;changed.step[63][2][15].velocity=101;changed.step[63][2][15].gate=75;
    assert(mc_project_valid(&original)&&mc_project_valid(&changed));
    wire_tests();
    memset(flash,255,sizeof flash);memset(flash,0xa5,MC_SECTOR);memset(flash+TOTAL-MC_SECTOR,0xa5,MC_SECTOR);
    uint32_t seq=77;assert(mc_store_load(&io,&loaded,&seq)<0&&seq==77);
    assert(!mc_store_save(&io,&original)&&erased==9&&programmed==MC_RECORD_BYTES);
    assert(mc_store_load(&io,&loaded,&seq)==0&&seq==1&&!memcmp(&loaded,&original,sizeof loaded));
    memcpy(snapshot,flash,sizeof flash);
    unsigned cuts=0;
    for(unsigned sector=0;sector<=9;sector++)for(int d=-1;d<=1;d++) {
        int pos=(int)(sector*MC_SECTOR)+d;if(pos>=0){cut_at((unsigned)pos);cuts++;}
    }
    for(unsigned page=0;page<MC_PROJECT_BYTES;page+=256)for(int d=-1;d<=1;d++) {
        int pos=(int)(MC_STORE_BYTES+page)+d;cut_at((unsigned)pos);cuts++;
    }
    for(unsigned i=0;i<20;i++){cut_at(MC_STORE_BYTES+MC_PROJECT_BYTES+i);cuts++;}
    printf("PASS: %u interrupted erase/page/header positions preserve full prior64-pattern project; guard sectors unchanged\n",cuts);
    memcpy(flash,snapshot,sizeof flash);reset_io();assert(!mc_store_save(&io,&changed));
    assert(mc_store_load(&io,&loaded,&seq)==1&&seq==2&&!memcmp(&loaded,&changed,sizeof loaded));guards();
    reset_io();assert(!mc_store_save(&io,&original));assert(mc_store_load(&io,&loaded,&seq)==0&&seq==3);
    puts("PASS: alternating nine-sector banks commit payload before header; pattern64 round trip");
    memcpy(flash,snapshot,sizeof flash);reset_io();unreadable=MC_STORE_B;
    assert(mc_store_load(&io,&loaded,&seq)==0);assert(mc_store_save(&io,&changed)<0&&!erased);
    unreadable=MC_STORE_A;assert(mc_store_load(&io,&loaded,&seq)<0);assert(mc_store_save(&io,&changed)<0&&!erased);
    reset_io();corrupt_write=1;assert(mc_store_save(&io,&changed)<0);corrupt_write=0;
    assert(mc_store_load(&io,&loaded,&seq)==0&&seq==1);guards();
    memcpy(flash,snapshot,sizeof flash);reset_io();fail_verify=1;assert(mc_store_save(&io,&changed)<0);fail_verify=0;
    assert(mc_store_load(&io,&loaded,&seq)==1&&seq==2);
    puts("PASS: unreadable banks refuse save; corrupted readback rejected; committed data survives verification-read failure");
    reset_io();assert(mc_encode(&original,0xffffffffu,flash+(MC_STORE_A-BASE),MC_RECORD_BYTES)>0);
    assert(mc_encode(&changed,0,flash+(MC_STORE_B-BASE),MC_RECORD_BYTES)>0);
    assert(mc_store_load(&io,&loaded,&seq)==1&&seq==0);
    assert(!mc_store_save(&io,&original));assert(mc_store_load(&io,&loaded,&seq)==0&&seq==1);
    flash[MC_STORE_B-BASE+6]=255;flash[MC_STORE_B-BASE+7]=255;
    assert(mc_store_load(&io,&loaded,&seq)==0);guards();
    puts("PASS: sequence rollover and oversized corrupt header; no read/write outside new72KiB banks");
    status_tests();guards();
    return 0;
}
