/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#define _DARWIN_C_SOURCE 1
#ifndef LOAD_PROBE_NEW
#define LOAD_PROBE_NEW 1
#endif
#ifndef LOAD_PROBE_FIXTURE
#define LOAD_PROBE_FIXTURE "../firmware/perf_fixture.h"
#endif
#include "mcfm.h"
#include "parallel.h"
#include LOAD_PROBE_FIXTURE
#if LOAD_PROBE_NEW
#include "hybrid.h"
#endif
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { BLOCK=128, WARMUP=1024, BLOCKS=8192 };
static McEngine engine, reference;
static McDual dual;
static int16_t audio[BLOCK*2], expected[BLOCK*2];
static uint64_t samples[BLOCKS], sorted[BLOCKS];
static uint64_t notify_ns, worker_ns, epilogue_ns;
static unsigned worker_jobs, fx_jobs;

static uint64_t now(void) {
    struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC_RAW,&t)) { perror("clock_gettime");exit(2); }
    return (uint64_t)t.tv_sec*UINT64_C(1000000000)+(uint64_t)t.tv_nsec;
}
static uint32_t ticks(void) { return (uint32_t)now(); }
static void halt_worker(McDual *d) {
    fprintf(stderr,"Unexpected worker failure: %u\n",d->status);exit(3);
}
/* Run the real worker kernels synchronously, not a simulated MCU clock.
 * The complete notification span is removed from host transfer timing.
 * Parallel overlap is reconstructed only in the separate historical model. */
static void notify_worker(McDual *d) {
    uint64_t begin=now();
    uint32_t job=mc_acquire(&d->request);
    mc_dual_work(d,job);
    if(d->result!=MC_DUAL_READY)halt_worker(d);
    uint64_t banks=0;
    for(unsigned i=0;i<MC_FX_BANKS;i++)banks+=d->bank_ticks[i];
    if(banks>d->job_ticks) { fputs("Invalid worker timing\n",stderr);exit(3); }
    if(d->drums_enabled) { worker_ns+=d->job_ticks-banks;worker_jobs++; }
    else { epilogue_ns+=d->job_ticks-banks;fx_jobs++; }
    mc_release(&d->done,job);
    notify_ns+=now()-begin;
}
static void notes(unsigned high) {
    memset(engine.voice,0,sizeof engine.voice);
    for(unsigned i=0;i<MC_VOICES;i++) {
        unsigned part=i%3u;
        unsigned note=(high?(part==2?100u:101u):(part==2?59u:60u))-i/3u;
        mc_note(&engine,part,note,127,MC_ACCENT,0);
    }
}
static unsigned effective(const McVoice *v) {
#if LOAD_PROBE_NEW
    unsigned mask=0;
    for(unsigned i=0;i<3;i++)
        if(v->hybrid.gain[i]&&!(v->hybrid.ended_mask&(1u<<i)))mask|=1u<<i;
    return mask;
#else
    (void)v;return 0;
#endif
}
static int compare_u64(const void *a,const void *b) {
    uint64_t x=*(const uint64_t *)a,y=*(const uint64_t *)b;
    return (x>y)-(x<y);
}
static unsigned parse_unsigned(const char *s,unsigned maximum) {
    char *end;
    unsigned long value=strtoul(s,&end,10);
    if(!*s||*end||value>maximum) { fputs("Invalid argument\n",stderr);exit(2); }
    return (unsigned)value;
}
int main(int argc,char **argv) {
    if(argc!=6) {
        fputs("Usage: load_bench PROFILE_OR_BASELINE ATTACK ANALOG HIGH AUX_OFF\n",stderr);
        return 2;
    }
    int profile=!strcmp(argv[1],"baseline")?-1:(int)parse_unsigned(argv[1],MC_PRESETS-1u);
    unsigned attack=parse_unsigned(argv[2],1),analog=parse_unsigned(argv[3],127);
    unsigned high=parse_unsigned(argv[4],1),aux_off=parse_unsigned(argv[5],1);
#if !LOAD_PROBE_NEW
    if(profile>=0||aux_off)return 2;
#endif
    mc_init(&engine);engine.master=818;
    mc_perf_fixture_four(&engine,0);
    for(unsigned p=0;p<3;p++) {
#if LOAD_PROBE_NEW
        if(profile>=0) {
            engine.project.patch[p]=mc_presets[profile];
            engine.project.preset[p]=(uint8_t)profile;
        }
        McPatch *patch=&engine.project.patch[p];
        const McHybridProfile *recipe=hy_profile(patch->profile);
        patch->source_mask=(uint8_t)(recipe->available_mask&~(aux_off?1u:0u));
        patch->pcm_mix=(uint8_t)(patch->source_mask&1u?127:0);
        patch->fm_mix=(uint8_t)(patch->source_mask&2u?127:0);
        patch->va_mix=(uint8_t)(patch->source_mask&4u?127:0);
        patch->mode=MC_POLY;
#else
        McPatch *patch=&engine.project.patch[p];
#endif
        patch->attack=0;patch->sustain=127;patch->release=0;
        patch->cutoff=patch->resonance=patch->level=127;
        engine.project.analog[p]=(uint8_t)analog;
    }
    if(!mc_project_valid(&engine.project)) {
        fputs("Invalid benchmark project\n",stderr);return 3;
    }
    notes(high);
    for(unsigned b=0;b<WARMUP;b++) {
        if(attack)notes(high);
        mc_render(&engine,audio,BLOCK);
    }
    dual.status=MC_DUAL_READY;dual.ticks=ticks;dual.notify=notify_worker;
    dual.halt=halt_worker;dual.budget_ticks=UINT32_C(1000000000);
    dual.spin_limit=UINT32_MAX;
    McRenderContext context={0};context.dual=&dual;context.ticks=ticks;
    uint64_t stage[MC_TIME_COUNT]={0},total_worker=0,total_epilogue=0,total_notes=0;
    uint64_t hash=UINT64_C(14695981039346656037);
    unsigned vmin=8,vmax=0,dmin=6,dmax=0,lo[3]={8,8,8},hi[3]={0};
    unsigned flo=8,fhi=0,alo=8,ahi=0,jobs_lo=UINT32_MAX,jobs_hi=0;
    uint64_t total_wall=0,total_notify=0;
    uint32_t clips=engine.clips,limiter=engine.limiter_hits;
    for(unsigned b=0;b<BLOCKS;b++) {
        if(attack) { uint64_t t=now();notes(high);total_notes+=now()-t; }
        if(b==0||b==BLOCKS-1)reference=engine;
        uint32_t phase[8],aux[8];
        for(unsigned i=0;i<8;i++) {
#if LOAD_PROBE_NEW
            phase[i]=engine.voice[i].hybrid.modulator;
            aux[i]=engine.voice[i].aux.phase;
#else
            phase[i]=0;
            aux[i]=0;
#endif
        }
        notify_ns=worker_ns=epilogue_ns=0;worker_jobs=fx_jobs=0;
        uint64_t begin=now();
        mc_render_ex(&engine,audio,BLOCK,&context);
        samples[b]=now()-begin;total_wall+=samples[b];
        if(dual.status!=MC_DUAL_READY||dual.failures)return 3;
        if(context.time[MC_TIME_TRANSFER]<notify_ns) {
            fputs("Notification span exceeds transfer interval\n",stderr);return 3;
        }
        context.time[MC_TIME_TRANSFER]-=(uint32_t)notify_ns;
        for(unsigned i=0;i<MC_TIME_COUNT;i++)stage[i]+=context.time[i];
        total_worker+=worker_ns;total_epilogue+=epilogue_ns;total_notify+=notify_ns;
        if(b==0||b==BLOCKS-1) {
            mc_render(&reference,expected,BLOCK);
            if(memcmp(audio,expected,sizeof audio)||memcmp(&engine,&reference,sizeof engine)) {
                fputs("Serial/worker PCM or state mismatch\n",stderr);return 4;
            }
        }
        unsigned voices=0,drums=0,counts[3]={0},fm=0,av=0;
        for(unsigned i=0;i<8;i++)if(engine.voice[i].active) {
            voices++;
            unsigned mask=effective(&engine.voice[i]);
            for(unsigned s=0;s<3;s++)counts[s]+=!!(mask&(1u<<s));
#if LOAD_PROBE_NEW
            fm+=phase[i]!=engine.voice[i].hybrid.modulator;
            av+=aux[i]!=engine.voice[i].aux.phase;
#else
            (void)phase;(void)aux;
#endif
        }
        for(unsigned i=0;i<6;i++)drums+=engine.drum[i].env!=0;
        if(voices<vmin)vmin=voices;if(voices>vmax)vmax=voices;
        if(drums<dmin)dmin=drums;if(drums>dmax)dmax=drums;
        for(unsigned s=0;s<3;s++) {
            if(counts[s]<lo[s])lo[s]=counts[s];if(counts[s]>hi[s])hi[s]=counts[s];
        }
        if(fm<flo)flo=fm;if(fm>fhi)fhi=fm;
        if(av<alo)alo=av;if(av>ahi)ahi=av;
        if(worker_jobs<jobs_lo)jobs_lo=worker_jobs;if(worker_jobs>jobs_hi)jobs_hi=worker_jobs;
        for(unsigned i=0;i<BLOCK*2;i++) { hash^=(uint16_t)audio[i];hash*=UINT64_C(1099511628211); }
    }
    memcpy(sorted,samples,sizeof sorted);qsort(sorted,BLOCKS,sizeof sorted[0],compare_u64);
    printf("{\"engine\":\"%s\",\"profile\":%d,\"phase\":\"%s\",\"analog\":%u,"
        "\"register\":\"%s\",\"clock\":\"CLOCK_MONOTONIC_RAW\","
        "\"aux_enabled\":%s,\"blocks\":%u,\"warmup_blocks\":%u,"
        "\"frames_per_block\":%u,\"source_mask\":%u,\"name\":\"%s\","
        "\"stage_ns\":{\"cpu0_synth\":%"PRIu64",\"worker_combined\":%"PRIu64","
        "\"transfer\":%"PRIu64",\"route\":%"PRIu64",\"output\":%"PRIu64","
        "\"worker_epilogue\":%"PRIu64",\"banks\":[",
        LOAD_PROBE_NEW?"current":"original",profile,attack?"attack":"held",analog,
        high?"high":"middle",aux_off?"false":"true",BLOCKS,WARMUP,BLOCK,
#if LOAD_PROBE_NEW
        engine.project.patch[0].source_mask,
#else
        0u,
#endif
        profile>=0?mc_preset_names[profile]:"baseline",
        stage[MC_TIME_SYNTH],total_worker,stage[MC_TIME_TRANSFER],stage[MC_TIME_ROUTE],
        stage[MC_TIME_OUTPUT],total_epilogue);
    for(unsigned i=0;i<4;i++)printf("%s%"PRIu64,i?",":"",stage[mc_fx_stage(i)]);
    printf("]},\"activity\":{\"voices\":[%u,%u],\"drums\":[%u,%u],"
        "\"sources\":[[%u,%u],[%u,%u],[%u,%u]],\"fm_phase_progress\":[%u,%u],"
        "\"aux_phase_progress\":[%u,%u],\"worker_jobs\":[%u,%u]},"
        "\"render_wall_ns\":%"PRIu64",\"note_setup_ns\":%"PRIu64","
        "\"removed_notify_ns\":%"PRIu64",\"host_p50_ns\":%"PRIu64","
        "\"host_p95_ns\":%"PRIu64",\"host_p99_ns\":%"PRIu64","
        "\"host_max_ns\":%"PRIu64",\"clips\":%u,\"limiter_hits\":%u,"
        "\"pcm_hash\":\"%016"PRIx64"\",\"serial_worker_parity\":true,"
        "\"sizeof_bytes\":{\"voice\":%zu,\"worker_voice\":%zu,\"synth_job\":%zu,"
        "\"fx_job\":%zu},\"block_ns\":[",
        vmin,vmax,dmin,dmax,lo[0],hi[0],lo[1],hi[1],lo[2],hi[2],flo,fhi,alo,ahi,
        jobs_lo,jobs_hi,total_wall,total_notes,total_notify,sorted[BLOCKS/2],
        sorted[BLOCKS*95/100],sorted[BLOCKS*99/100],sorted[BLOCKS-1],
        engine.clips-clips,engine.limiter_hits-limiter,hash,
        sizeof(McVoice),sizeof(McWorkerVoice),sizeof(McSynthJob),sizeof(McFxJob));
    for(unsigned i=0;i<BLOCKS;i++)printf("%s%"PRIu64,i?",":"",samples[i]);
    puts("]}");
    return vmin==8&&dmin==6&&vmax==8&&dmax==6?0:5;
}
