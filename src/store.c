/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 dspaudio */
#include "store.h"
/* 공용 scratch: transport를 정지한 foreground에서만 호출한다. */
static uint8_t record[MC_RECORD_BYTES];
_Static_assert(MC_RECORD_BYTES<=MC_STORE_BYTES,"Project exceeds storage bank");
static int inspect(const McStorage *io,uint32_t addr,uint32_t *seq) {
    uint8_t chunk[256];unsigned erased=1;
    /* 길이를 신뢰하지 않고 고정 bank 범위만 읽는다. EMPTY는 전체 erased일 때뿐이다. */
    for(unsigned off=0;off<MC_STORE_BYTES;off+=sizeof chunk) {
        unsigned n=MC_STORE_BYTES-off;if(n>sizeof chunk)n=sizeof chunk;
        if(io->read(addr+off,chunk,n)) return MC_STORE_IO_ERROR;
        for(unsigned i=0;i<n;i++) {
            if(chunk[i]!=255) erased=0;
            if(off+i<sizeof record) record[off+i]=chunk[i];
        }
    }
    if(erased) return MC_STORE_EMPTY;
    uint32_t magic=0,header_crc=0;
    for(unsigned i=0;i<4;i++) {
        magic|=(uint32_t)record[i]<<(8*i);
        header_crc|=(uint32_t)record[16+i]<<(8*i);
    }
    if(magic!=MC_PROJECT_MAGIC||record[4]!=MC_PROJECT_VERSION||record[5]||
       header_crc!=mc_crc(record,16)) return MC_STORE_UNSUPPORTED;
    for(unsigned i=0;i<8;i++) if(record[20+i]!=mc_pack_id[i]) return MC_STORE_UNSUPPORTED;
    return mc_record_valid(record,sizeof record,seq)?MC_STORE_CORRUPT_OWN:MC_STORE_CURRENT;
}
static int newer(const int valid[2],const uint32_t seq[2]) {
    if(valid[0]!=MC_STORE_CURRENT&&valid[1]!=MC_STORE_CURRENT) return -1;
    return valid[1]==MC_STORE_CURRENT&&(valid[0]!=MC_STORE_CURRENT||(int32_t)(seq[1]-seq[0])>0)?1:0;
}
static int scan(const McStorage *io,int valid[2],uint32_t seq[2]) {
    valid[0]=inspect(io,MC_STORE_A,&seq[0]);
    valid[1]=inspect(io,MC_STORE_B,&seq[1]);
    return newer(valid,seq);
}
static int status(const int valid[2]) {
    if(valid[0]==MC_STORE_IO_ERROR||valid[1]==MC_STORE_IO_ERROR) return MC_STORE_IO_ERROR;
    if(valid[0]==MC_STORE_UNSUPPORTED||valid[1]==MC_STORE_UNSUPPORTED) return MC_STORE_UNSUPPORTED;
    if(valid[0]==MC_STORE_CURRENT||valid[1]==MC_STORE_CURRENT) return MC_STORE_CURRENT;
    if(valid[0]==MC_STORE_CORRUPT_OWN||valid[1]==MC_STORE_CORRUPT_OWN) return MC_STORE_CORRUPT_OWN;
    return MC_STORE_EMPTY;
}
int mc_store_status(const McStorage *io) {
    int valid[2];uint32_t sequence[2]={0,0};
    scan(io,valid,sequence);
    return status(valid);
}
int mc_store_load(const McStorage *io,McProject *p,uint32_t *seq) {
    int valid[2];uint32_t sequence[2]={0,0};
    int active=scan(io,valid,sequence);
    if(active<0) return status(valid)==MC_STORE_UNSUPPORTED?-3:-1;
    if(inspect(io,active?MC_STORE_B:MC_STORE_A,0)!=MC_STORE_CURRENT||
       mc_decode(p,seq,record,sizeof record)) return -1;
    return active;
}
int mc_store_save(const McStorage *io,const McProject *p) {
    int valid[2];uint32_t sequence[2]={0,0};
    int active=scan(io,valid,sequence);
    /* 외래 또는 읽기 실패 bank는 지우거나 쓰지 않는다. */
    int state=status(valid);
    if(state==MC_STORE_UNSUPPORTED||state==MC_STORE_IO_ERROR) return -1;
    uint32_t seq=active<0?0:sequence[active];
    unsigned dest=active==0?1u:0u;
    if(mc_encode(p,seq+1,record,sizeof record)<0) return -1;
    uint32_t addr=dest?MC_STORE_B:MC_STORE_A;
    for(unsigned off=0;off<MC_STORE_BYTES;off+=MC_SECTOR)
        if(io->erase(addr+off)) return -1;
    /* payload 완료 뒤 CRC를 포함한 header를 마지막에 commit한다. */
    if(io->program(addr+20,record+20,MC_PROJECT_BYTES)) return -1;
    if(io->program(addr,record,20)) return -1;
    uint8_t verify[256];
    for(unsigned off=0;off<MC_RECORD_BYTES;off+=sizeof verify) {
        unsigned n=MC_RECORD_BYTES-off;if(n>sizeof verify)n=sizeof verify;
        if(io->read(addr+off,verify,n)) return -1;
        for(unsigned i=0;i<n;i++) if(record[off+i]!=verify[i]) return -1;
    }
    return 0;
}
