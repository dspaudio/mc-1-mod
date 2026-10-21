/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef MC_STORE_H
#define MC_STORE_H
#include "mcfm.h"
#define MC_SECTOR 4096u
#define MC_STORE_A 0x99000u
#define MC_STORE_B 0xa2000u
#define MC_STORE_BYTES (9u * MC_SECTOR)
#define MC_STORE_END (MC_STORE_B + MC_STORE_BYTES)
typedef struct {
    int (*read)(uint32_t,void *,unsigned);
    int (*erase)(uint32_t);
    int (*program)(uint32_t,const void *,unsigned);
} McStorage;
enum {
    MC_STORE_CURRENT=0, MC_STORE_EMPTY=1, MC_STORE_CORRUPT_OWN=2,
    MC_STORE_UNSUPPORTED=3, MC_STORE_IO_ERROR=4
};
extern const uint8_t mc_pack_id[8];
int mc_store_status(const McStorage *io);
int mc_store_load(const McStorage *io,McProject *p,uint32_t *seq);
int mc_store_save(const McStorage *io,const McProject *p);
#endif
