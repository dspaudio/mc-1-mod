// SPDX-License-Identifier: GPL-3.0-only
// Run with: node tools/measure_load.mjs [--smoke]
import assert from 'node:assert/strict';
import {execFile} from 'node:child_process';
import {createHash} from 'node:crypto';
import fs from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import {promisify} from 'node:util';
import {fileURLToPath} from 'node:url';

const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const original=path.resolve(root,'../mc-1/mcfm-1');
const historicalCommit='10a2240d7f3c63e3ed551774a8e17fb24ed002b8';
const smoke=process.argv.includes('--smoke');
assert(process.argv.slice(2).every(arg=>arg==='--smoke'));
const out=path.join(root,'build/load64',smoke?'smoke':'');
const runDir=path.join(out,`run-${process.pid}-${Date.now()}`);
await fs.mkdir(runDir,{recursive:true});
const execute=promisify(execFile);
const env={...process.env,GIT_MASTER:'1',GIT_PAGER:'cat'};
const command=async(file,args)=>execute(file,args,{cwd:root,env,maxBuffer:4*1024*1024,timeout:120000});
const sha=bytes=>createHash('sha256').update(bytes).digest('hex');
assert.equal((await command('git',['-C',original,'rev-parse','HEAD'])).stdout.trim(),historicalCommit);
assert.equal((await command('git',['-C',original,'status','--porcelain'])).stdout,'');
const originalStatus=(await command('git',['-C',original,'status','--porcelain'])).stdout;
const sourceCommit=(await command('git',['rev-parse','HEAD'])).stdout.trim();

const sourcePaths=[];
for(const folder of ['src','firmware'])
    for(const file of await fs.readdir(path.join(root,folder),{recursive:true}))
        if(/\.(?:c|h|S|ld)$/.test(file))sourcePaths.push(folder+'/'+file);
sourcePaths.push('tools/build_target.py','tools/gen_pcm.py','tools/gen_va.py','tools/target_abi.c',
    'data/pcm-manifest.json','data/pcm-source/vsco-attacks.json','data/pcm-source/generaluser-strings.json');
const sourceHasher=createHash('sha256');
for(const file of sourcePaths.sort())
    sourceHasher.update(file+'\0').update(await fs.readFile(path.join(root,file))).update('\0');
const sourceInput=sourceHasher.digest('hex');
const target=JSON.parse(await fs.readFile(path.join(root,'artifacts/firmware/target-report.json'),'utf8'));
assert.equal(sourceInput,target.source_input_sha256,'production target must match measured source');

const common=['effects','demo','engine','midi','factory','project','store','ui'];
const flags=['-O2','-g','-std=c11','-Wall','-Wextra','-Werror','-Wconversion','-Wno-sign-conversion'];
const cc=process.env.CC??'/usr/bin/cc';
const buildCommands=[];
for(const [name,tree,extra] of [['original',original,[]],['current',root,['hybrid','tables']]]) {
    const args=['-I'+path.join(tree,'src'),...flags,
        '-DLOAD_PROBE_NEW='+(name==='current'?1:0),
        '-DLOAD_PROBE_FIXTURE='+JSON.stringify(path.join(tree,'firmware/perf_fixture.h')),
        ...[...common,...extra].map(file=>path.join(tree,'src',file+'.c')),
        path.join(root,'tools/load_bench.c'),'-lm','-o',path.join(runDir,name+'-bench')];
    buildCommands.push({compiler:cc,args});
    await command(cc,args);
}
const compiler=(await command(cc,['--version'])).stdout.trim();
const fields=['cpu0_synth','worker_combined','transfer','route','output'];
let sequence=0;
const run=async(engine,profile,phase,analog,register,auxOff)=>{
    const args=[String(profile),phase==='attack'?'1':'0',String(analog),register==='high'?'1':'0',auxOff?'1':'0'];
    const result=await command(path.join(runDir,engine+'-bench'),args);
    const data=JSON.parse(result.stdout);
    assert.equal(data.engine,engine);
    assert.equal(data.serial_worker_parity,true);
    assert.deepEqual(data.activity.voices,[8,8]);
    assert.deepEqual(data.activity.drums,[6,6]);
    assert.equal(data.warmup_blocks,1024);
    assert.equal(data.blocks,8192);
    const file=String(sequence++).padStart(4,'0')+'-'+engine+'.json';
    await fs.writeFile(path.join(runDir,file),result.stdout);
    return {data,file:path.relative(root,path.join(runDir,file)),
        means:Object.fromEntries(fields.map(field=>[field,data.stage_ns[field]/data.blocks]))};
};
const profiles=smoke?[33,37]:Array.from({length:64},(_,i)=>i);
const auxProfiles=[33,37,39,40,41,43,44,45,47,49,51,54,56,58,59,61];
const variants=[
    ...profiles.map(profile=>({profile,analog:127,off:false})),
    ...(smoke?[33,37]:auxProfiles).map(profile=>({profile,analog:127,off:true})),
    ...(smoke?[]:[33,37,54,59]).map(profile=>({profile,analog:0,off:false}))
];
const rows=new Map();
const baselineFiles=[];
for(let repeat=0;repeat<3;repeat++)for(const phase of ['attack','held'])for(const register of ['middle','high']) {
    let before=await run('original','baseline','held',0,'high',false);
    baselineFiles.push(before.file);
    for(let start=0;start<variants.length;start+=8) {
        const batch=[];
        for(const variant of variants.slice(start,start+8))
            batch.push({variant,result:await run('current',variant.profile,phase,variant.analog,register,variant.off)});
        const after=await run('original','baseline','held',0,'high',false);
        baselineFiles.push(after.file);
        for(const {variant,result} of batch) {
            const key=[variant.profile,phase,register,variant.analog,variant.off].join(':');
            let row=rows.get(key);
            if(!row) {
                row={profile:variant.profile,name:result.data.name,phase,register,analog:variant.analog,
                    source_mask:result.data.source_mask,aux_enabled:!variant.off&&auxProfiles.includes(variant.profile),
                    ratios:Object.fromEntries(fields.map(field=>[field,[]])),
                    host_ns:Object.fromEntries(fields.map(field=>[field,[]])),
                    baseline_ns:Object.fromEntries(fields.map(field=>[field,[]])),
                    activity:result.data.activity,runs:[],pcm_hash:result.data.pcm_hash,
                    host_p50_ns:[],host_p95_ns:[],host_p99_ns:[],host_max_ns:[]};
                rows.set(key,row);
            }
            assert.equal(result.data.pcm_hash,row.pcm_hash,'fixed fixture must be deterministic across repeats');
            assert.deepEqual(result.data.activity,row.activity,'activity must not depend on host timing');
            for(const field of fields) {
                const anchor=(before.means[field]+after.means[field])/2;
                assert(anchor>0&&result.means[field]>0);
                row.baseline_ns[field].push(anchor);
                row.host_ns[field].push(result.means[field]);
                row.ratios[field].push(result.means[field]/anchor);
            }
            for(const field of ['host_p50_ns','host_p95_ns','host_p99_ns','host_max_ns'])
                row[field].push(result.data[field]);
            row.runs.push({repeat,current:result.file,before:before.file,after:after.file});
        }
        before=after;
        console.log(`MEASURE repeat=${repeat+1}/3 ${phase}/${register} variants=${Math.min(start+8,variants.length)}/${variants.length}`);
    }
}
assert.equal((await command('git',['-C',original,'status','--porcelain'])).stdout,originalStatus);
const report={
    schema:2,source_input_sha256:sourceInput,source_commit:sourceCommit,
    original_commit:historicalCommit,original_source_proxy:false,compiler,
    benchmark_source_sha256:sha(await fs.readFile(path.join(root,'tools/load_bench.c'))),
    runner_source_sha256:sha(await fs.readFile(fileURLToPath(import.meta.url))),
    host:{platform:process.platform,arch:process.arch,cpu:os.cpus()[0].model},
    smoke,complete:true,build_commands:buildCommands,baseline_files:baselineFiles,
    measurement_policy:{
        execution:'real C kernels, synchronous worker notification; not MCU/physical dual timing',
        anchor:'original historical-commit engine, held/high register, Analog0, original acid0 four-bank fixture',
        candidate:'synthetic poly8, same slot/event/drum/FX counts; profile sources and pitches differ',
        phase:'attack retriggers before every timed render; note construction excluded and recorded separately',
        source_activity:'actual gain/ended masks and phase progress; held may lose primary FM/PCM',
        transfer:'context transfer minus complete synchronous notification span; wait already excluded by core',
        worker:'non-bank time of synth-enabled jobs, includes wrapper residue; FX-only residue separate',
        ratios:'current stage mean divided by average of bracketing original-stage means',
        scope:'host timing and cross-architecture assumptions, not MCU measurements or confidence intervals'
    },
    rows:[...rows.values()]
};
await fs.writeFile(path.join(out,'measurements.json'),JSON.stringify(report,null,2)+'\n');
console.log(`MEASURE_COMPLETE rows=${report.rows.length} executions=${sequence} output=${path.relative(root,path.join(out,'measurements.json'))}`);
