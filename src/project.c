/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 dspaudio */
#include "mcfm.h"
#include "hybrid.h"
const uint8_t mc_pack_id[8]={'M','O','D','1',MC_PATCH_SCHEMA,0,MC_BANK_REVISION,0};
static void put32(uint8_t *p,uint32_t v) { for(unsigned i=0;i<4;i++) p[i]=(uint8_t)(v>>(8*i)); }
static uint32_t get32(const uint8_t *p) { return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
uint32_t mc_crc(const void *data,unsigned n) {
    const uint8_t *b=data; uint32_t c=0xffffffffu;
    while(n--) { c^=*b++; for(unsigned i=0;i<8;i++) c=(c>>1)^(0xedb88320u*(c&1)); }
    return ~c;
}
static int patch_valid(const uint8_t *s) {
    const McHybridProfile *profile=hy_profile(s[2]);
    if(s[0]>MC_MONO||s[1]>MC_WAVE_SINE||!profile) return 0;
    for(unsigned i=3;i<14;i++) if(s[i]>127) return 0;
    return !(s[14]&~profile->available_mask)&&
        ((profile->available_mask&HY_PCM)||!s[3])&&
        ((profile->available_mask&HY_FM)||!s[12])&&
        ((profile->available_mask&HY_VA)||!s[13]);
}
static int automation_valid(unsigned part,unsigned param,unsigned value,const uint8_t *patch) {
    unsigned type=param==MC_PARAM_RD_TYPE||param==MC_PARAM_CF_TYPE||
        param==MC_PARAM_RD2_TYPE||param==MC_PARAM_CF2_TYPE;
    unsigned send=param==MC_PARAM_RD_SEND||param==MC_PARAM_CF_SEND||
        param==MC_PARAM_RD2_SEND||param==MC_PARAM_CF2_SEND;
    unsigned maximum=param==0?1:param==1?2:param==MC_PARAM_SOURCE_MASK?7:
        type?7:
        param>=MC_PARAM_DELAY_TYPE&&param<MC_PARAM_HPF?(param==MC_PARAM_MOD_TYPE?2:3):127;
    if(part<3) {
        const McHybridProfile *profile=hy_profile(patch[2]);
        if(!profile) return 0;
        if(param==MC_PARAM_SOURCE_MASK&&(value&~profile->available_mask)) return 0;
        if(value&&((param==3&&!(profile->available_mask&HY_PCM))||
            (param==MC_PARAM_FM_MIX&&!(profile->available_mask&HY_FM))||
            (param==MC_PARAM_VA_MIX&&!(profile->available_mask&HY_VA)))) return 0;
    }
    return param<MC_PARAMETERS&&param!=MC_PARAM_PROFILE&&value<=maximum&&
        !(part>=3&&param<12)&&!(part==4&&param<MC_PARAM_DELAY)&&
        !(param>=MC_PARAM_DRUM_LEVEL&&param<MC_PARAM_RD_SEND&&part!=3)&&
        !(send&&part==4)&&!(type&&part!=4)&&
        !(param>=MC_PARAM_ANALOG&&part>=3);
}
int mc_project_valid(const McProject *p) {
    if(p->song_length>MC_SONG_ENTRIES||p->song_loop>1||p->song_mode>1||p->clock_source>3) return 0;
    for(unsigned i=0;i<4;i++) if(p->midi_channel[i]>15) return 0;
    for(unsigned i=0;i<MC_SONG_ENTRIES;i++) if(p->song[i].pattern>=MC_PATTERNS||!p->song[i].repeats||p->song[i].repeats>16) return 0;
    if(p->bpm<40||p->bpm>240||p->swing>45||p->drum_kit>=MC_KITS||p->demo>1||p->padding[0]||p->padding[1]||p->padding[2]) return 0;
    for(unsigned i=0;i<4;i++) if(p->fx[i]>127) return 0;
    for(unsigned t=0;t<4;t++) for(unsigned i=0;i<4;i++) if(p->track_fx[t][i]>127) return 0;
    for(unsigned t=0;t<MC_PARTS;t++) for(unsigned i=0;i<MC_FX_BANKS;i++) if(p->fx_send[t][i]>127) return 0;
    for(unsigned i=0;i<MC_FX_BANKS;i++) if(p->fx_bus_type[i]>7) return 0;
    for(unsigned i=0;i<3;i++) if(p->analog[i]>127) return 0;
    if(p->metronome>1||p->automation_count>MC_AUTOMATION) return 0;
    for(unsigned t=0;t<3;t++) {
        const McArp *a=&p->arp[t];
        if((p->preset[t]>=MC_PRESETS&&p->preset[t]!=255)||a->enabled>1||a->mode>MC_ARP_CHORD||a->rate>2||
           a->octaves<1||a->octaves>3||a->gate<1||a->gate>100||a->latch>1) return 0;
    }
    for(unsigned t=0;t<5;t++) {
        const uint8_t *type=t==4?p->fx_type:p->track_fx_type[t];
        const uint8_t *filter=t==4?p->filter:p->track_filter[t];
        for(unsigned i=0;i<4;i++) if(type[i]>(i==3?2:3)) return 0;
        if(filter[0]>127||filter[1]>127) return 0;
    }
    for(unsigned i=0;i<MC_AUTOMATION;i++) {
        const McAutomation *a=&p->automation[i];
        if(i>=p->automation_count) { if(a->address||a->step||a->parameter||a->value) return 0;continue; }
        unsigned part=a->address%5,param=a->parameter;
        if(a->address>=MC_PATTERNS*5||a->step>=16||
           !automation_valid(part,param,a->value,part<3?(const uint8_t *)&p->patch[part]:0)) return 0;
        for(unsigned j=0;j<i;j++) if(p->automation[j].address==a->address&&p->automation[j].step==a->step&&
            p->automation[j].parameter==param) return 0;
    }
    for(unsigned t=0;t<3;t++) {
        if(!patch_valid((const uint8_t *)&p->patch[t])||
           (p->preset[t]!=255&&p->preset[t]!=p->patch[t].profile)) return 0;
    }
    for(unsigned t=0;t<4;t++) if(p->level[t]>127||p->pan[t]>127) return 0;
    for(unsigned d=0;d<MC_DRUMS;d++) if(p->drum_level[d]>127) return 0;
    for(unsigned b=0;b<MC_PATTERNS;b++) for(unsigned t=0;t<4;t++) for(unsigned i=0;i<16;i++) {
        const McStep *s=&p->step[b][t][i];
        if(s->velocity>127||s->gate>100||s->flags>7) return 0;
        for(unsigned c=0;c<3;c++) if(s->note[c]&&(s->note[c]<12||s->note[c]>108)) return 0;
        if(t<3&&s->velocity&&s->note[0]&&!s->gate) return 0;
    }
    return 1;
}
static void pack(const McProject *p,uint8_t *out) {
    for(unsigned i=0;i<8;i++) *out++=mc_pack_id[i];
    *out++=(uint8_t)p->bpm; *out++=(uint8_t)(p->bpm>>8); *out++=p->swing; *out++=p->drum_kit;
    for(unsigned t=0;t<3;t++) for(unsigned i=0;i<15;i++) *out++=((const uint8_t *)&p->patch[t])[i];
    for(unsigned t=0;t<4;t++) *out++=p->level[t];
    for(unsigned t=0;t<4;t++) *out++=p->pan[t];
    for(unsigned d=0;d<16;d++) *out++=p->drum_level[d];
    for(unsigned i=0;i<4;i++) *out++=p->fx[i];
    *out++=p->demo;for(unsigned i=0;i<3;i++) *out++=0;
    for(unsigned b=0;b<MC_PATTERNS;b++) for(unsigned t=0;t<4;t++) for(unsigned i=0;i<16;i++) {
        const McStep *s=&p->step[b][t][i];
        for(unsigned c=0;c<3;c++) *out++=s->note[c];
        *out++=s->velocity; *out++=s->gate; *out++=s->flags;
        *out++=(uint8_t)s->drums; *out++=(uint8_t)(s->drums>>8);
    }
    for(unsigned t=0;t<4;t++) for(unsigned i=0;i<4;i++) *out++=p->track_fx[t][i];
    for(unsigned t=0;t<3;t++) *out++=p->preset[t];
    for(unsigned t=0;t<3;t++) for(unsigned i=0;i<6;i++) *out++=((const uint8_t *)&p->arp[t])[i];
    for(unsigned i=0;i<4;i++) *out++=p->fx_type[i];
    for(unsigned t=0;t<4;t++) for(unsigned i=0;i<4;i++) *out++=p->track_fx_type[t][i];
    for(unsigned i=0;i<2;i++) *out++=p->filter[i];
    for(unsigned t=0;t<4;t++) for(unsigned i=0;i<2;i++) *out++=p->track_filter[t][i];
    *out++=p->metronome;*out++=(uint8_t)p->automation_count;*out++=(uint8_t)(p->automation_count>>8);
    for(unsigned n=0;n<MC_AUTOMATION;n++) {
        const McAutomation *a=&p->automation[n];
        *out++=(uint8_t)a->address;*out++=(uint8_t)(a->address>>8);
        *out++=a->step;*out++=a->parameter;*out++=a->value;
    }
    for(unsigned i=0;i<MC_SONG_ENTRIES;i++) { *out++=p->song[i].pattern;*out++=p->song[i].repeats; }
    *out++=p->song_length;*out++=p->song_loop;*out++=p->song_mode;
    for(unsigned i=0;i<4;i++) *out++=p->midi_channel[i];
    *out++=p->clock_source;
    for(unsigned t=0;t<MC_PARTS;t++) for(unsigned i=0;i<2;i++) *out++=p->fx_send[t][i];
    for(unsigned i=0;i<2;i++) *out++=p->fx_bus_type[i];
    for(unsigned t=0;t<MC_PARTS;t++) for(unsigned i=2;i<MC_FX_BANKS;i++) *out++=p->fx_send[t][i];
    for(unsigned i=2;i<MC_FX_BANKS;i++) *out++=p->fx_bus_type[i];
    for(unsigned i=0;i<3;i++) *out++=p->analog[i];
}
static void unpack(McProject *p,const uint8_t *in) {
    for(unsigned i=0;i<sizeof *p;i++) ((uint8_t *)p)[i]=0;
    in+=8;
    p->bpm=(uint16_t)(in[0]|(unsigned)in[1]<<8); in+=2; p->swing=*in++; p->drum_kit=*in++;
    for(unsigned t=0;t<3;t++) for(unsigned i=0;i<15;i++) ((uint8_t *)&p->patch[t])[i]=*in++;
    for(unsigned t=0;t<4;t++) p->level[t]=*in++;
    for(unsigned t=0;t<4;t++) p->pan[t]=*in++;
    for(unsigned d=0;d<16;d++) p->drum_level[d]=*in++;
    for(unsigned i=0;i<4;i++) p->fx[i]=*in++;
    p->demo=*in++;
    for(unsigned i=0;i<3;i++) p->padding[i]=*in++;
    for(unsigned b=0;b<MC_PATTERNS;b++) for(unsigned t=0;t<4;t++) for(unsigned i=0;i<16;i++) {
        McStep *s=&p->step[b][t][i];
        for(unsigned c=0;c<3;c++) s->note[c]=*in++;
        s->velocity=*in++; s->gate=*in++; s->flags=*in++;
        s->drums=(uint16_t)(in[0]|(unsigned)in[1]<<8); in+=2;
    }
    for(unsigned t=0;t<4;t++) for(unsigned i=0;i<4;i++) p->track_fx[t][i]=*in++;
    {
        for(unsigned t=0;t<3;t++) p->preset[t]=*in++;
        for(unsigned t=0;t<3;t++) for(unsigned i=0;i<6;i++) ((uint8_t *)&p->arp[t])[i]=*in++;
        for(unsigned i=0;i<4;i++) p->fx_type[i]=*in++;
        for(unsigned t=0;t<4;t++) for(unsigned i=0;i<4;i++) p->track_fx_type[t][i]=*in++;
        for(unsigned i=0;i<2;i++) p->filter[i]=*in++;
        for(unsigned t=0;t<4;t++) for(unsigned i=0;i<2;i++) p->track_filter[t][i]=*in++;
        p->metronome=*in++;p->automation_count=(uint16_t)(in[0]|(unsigned)in[1]<<8);in+=2;
        for(unsigned n=0;n<MC_AUTOMATION;n++) {
            McAutomation *a=&p->automation[n];a->address=*in++;
            a->address|=(uint16_t)((unsigned)*in++<<8);
            a->step=*in++;a->parameter=*in++;a->value=*in++;
        }
    }
    {
        for(unsigned i=0;i<MC_SONG_ENTRIES;i++) { p->song[i].pattern=*in++;p->song[i].repeats=*in++; }
        p->song_length=*in++;p->song_loop=*in++;p->song_mode=*in++;
        for(unsigned i=0;i<4;i++) p->midi_channel[i]=*in++;
        p->clock_source=*in++;
    }
    {
        for(unsigned t=0;t<MC_PARTS;t++) for(unsigned i=0;i<2;i++) p->fx_send[t][i]=*in++;
        for(unsigned i=0;i<2;i++) p->fx_bus_type[i]=*in++;
    }
    {
        for(unsigned t=0;t<MC_PARTS;t++) for(unsigned i=2;i<MC_FX_BANKS;i++) p->fx_send[t][i]=*in++;
        for(unsigned i=2;i<MC_FX_BANKS;i++) p->fx_bus_type[i]=*in++;
        for(unsigned i=0;i<3;i++) p->analog[i]=*in++;
    }
}
int mc_encode(const McProject *p,uint32_t sequence,uint8_t *out,unsigned n) {
    if(n<MC_RECORD_BYTES||!mc_project_valid(p)) return -1;
    put32(out,MC_PROJECT_MAGIC); out[4]=MC_PROJECT_VERSION; out[5]=0;
    out[6]=MC_PROJECT_BYTES&255; out[7]=MC_PROJECT_BYTES>>8;
    put32(out+8,sequence); pack(p,out+20);
    put32(out+12,mc_crc(out+20,MC_PROJECT_BYTES)); put32(out+16,mc_crc(out,16));
    return MC_RECORD_BYTES;
}
static unsigned address_at(const uint8_t *in) {
    return in[0]+((unsigned)in[1]<<8);
}
int mc_record_valid(const uint8_t *in,unsigned n,uint32_t *sequence) {
    if(n<28) return -1;
    if(get32(in)!=MC_PROJECT_MAGIC||in[4]!=MC_PROJECT_VERSION||in[5]||
       get32(in+16)!=mc_crc(in,16)) return -3;
    for(unsigned i=0;i<8;i++) if(in[20+i]!=mc_pack_id[i]) return -3;
    if(n!=MC_RECORD_BYTES||(in[6]|(unsigned)in[7]<<8)!=MC_PROJECT_BYTES||
       get32(in+12)!=mc_crc(in+20,MC_PROJECT_BYTES)) return -1;
    const uint8_t *at=in+28;
    unsigned bpm=at[0]|(unsigned)at[1]<<8;
    if(bpm<40||bpm>240||at[2]>45||at[3]>=MC_KITS) return -1;
    at+=4;
    const uint8_t *patches=at;
    for(unsigned t=0;t<3;t++,at+=15) if(!patch_valid(at)) return -1;
    for(unsigned i=0;i<24;i++) if(*at++>127) return -1;
    {
        for(unsigned i=0;i<4;i++) if(*at++>127) return -1;
        if(*at++>1) return -1;
        for(unsigned i=0;i<3;i++) if(*at++) return -1;
    }
    for(unsigned b=0;b<MC_PATTERNS;b++) for(unsigned t=0;t<4;t++) for(unsigned i=0;i<16;i++,at+=8) {
        for(unsigned c=0;c<3;c++) if(at[c]&&(at[c]<12||at[c]>108)) return -1;
        if(at[3]>127||at[4]>100||at[5]>7||(t<3&&at[3]&&at[0]&&!at[4])) return -1;
    }
    for(unsigned i=0;i<16;i++) if(*at++>127) return -1;
    {
        for(unsigned i=0;i<3;i++,at++)
            if(*at!=255&&(*at>=MC_PRESETS||*at!=patches[i*15+2])) return -1;
        for(unsigned t=0;t<3;t++,at+=6)
            if(at[0]>1||at[1]>MC_ARP_CHORD||at[2]>2||at[3]<1||at[3]>3||at[4]<1||at[4]>100||at[5]>1) return -1;
        for(unsigned i=0;i<20;i++) if(*at++>(i%4==3?2:3)) return -1;
        for(unsigned i=0;i<10;i++) if(*at++>127) return -1;
        if(*at++>1) return -1;
        unsigned count=at[0]|(unsigned)at[1]<<8;at+=2;
        if(count>MC_AUTOMATION) return -1;
        const uint8_t *events=at;unsigned width=5;
        for(unsigned i=0;i<MC_AUTOMATION;i++,at+=width) {
            if(i>=count) { for(unsigned j=0;j<width;j++) if(at[j]) return -1;continue; }
            unsigned address=address_at(at),step=at[width-3],param=at[width-2],value=at[width-1],part=address%5;
            if(address>=MC_PATTERNS*5||step>=16||
               !automation_valid(part,param,value,part<3?patches+part*15:0)) return -1;
            for(unsigned j=0;j<i;j++) {
                const uint8_t *previous=events+j*width;
                if(address_at(previous)==address&&previous[width-3]==step&&previous[width-2]==param) return -1;
            }
        }
    }
    {
        for(unsigned i=0;i<MC_SONG_ENTRIES;i++,at+=2) if(at[0]>=MC_PATTERNS||!at[1]||at[1]>16) return -1;
        if(at[0]>MC_SONG_ENTRIES||at[1]>1||at[2]>1) return -1;
        at+=3;
        for(unsigned i=0;i<4;i++) if(*at++>15) return -1;
        if(*at++>3) return -1;
    }
    {
        for(unsigned i=0;i<MC_PARTS*2;i++) if(*at++>127) return -1;
        for(unsigned i=0;i<2;i++) if(*at++>7) return -1;
    }
    {
        for(unsigned i=0;i<MC_PARTS*2;i++) if(*at++>127) return -1;
        for(unsigned i=0;i<2;i++) if(*at++>7) return -1;
        for(unsigned i=0;i<3;i++) if(*at++>127) return -1;
    }
    if((unsigned)(at-in)!=MC_RECORD_BYTES) return -1;
    if(sequence) *sequence=get32(in+8);
    return 0;
}
int mc_decode(McProject *p,uint32_t *sequence,const uint8_t *in,unsigned n) {
    int rc=mc_record_valid(in,n,0);
    if(rc) return rc;
    unpack(p,in+20);
    if(sequence) *sequence=get32(in+8);
    return 0;
}
int mc_recover(McProject *p,uint32_t *sequence,const uint8_t *a,const uint8_t *b) {
    uint32_t sa=0,sb=0;
    int ra=mc_record_valid(a,MC_RECORD_BYTES,&sa),rb=mc_record_valid(b,MC_RECORD_BYTES,&sb);
    int va=ra==0,vb=rb==0;
    if(!va&&!vb) {
        unsigned erased_a=1,erased_b=1;
        for(unsigned i=0;i<MC_RECORD_BYTES;i++) {
            if(a[i]!=255) erased_a=0;
            if(b[i]!=255) erased_b=0;
        }
        return (ra==-3&&!erased_a)||(rb==-3&&!erased_b)?-3:-1;
    }
    unsigned chosen=vb&&(!va||(int32_t)(sb-sa)>0);
    if(mc_decode(p,sequence,chosen?b:a,MC_RECORD_BYTES)) return -1;
    return (int)chosen;
}
