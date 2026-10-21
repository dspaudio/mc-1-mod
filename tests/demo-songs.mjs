// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {createHash} from 'node:crypto';
import {execFileSync} from 'node:child_process';

const manifest=JSON.parse(fs.readFileSync('build/site/auditions.json','utf8'));
const native=JSON.parse(fs.readFileSync('build/canonical-audio/auditions.json','utf8'));
assert.equal(manifest.schema_version,4);
assert.equal(native.schema_version,3);
assert.equal(manifest.native_schema_version,native.schema_version);
assert.equal(manifest.measurement_basis,'canonical_wav');
assert.equal(manifest.delivery.lossy,true);
assert.equal(manifest.delivery.sample_exact,false);
const module=new WebAssembly.Module(fs.readFileSync('build/mod1.wasm'));
const songs=manifest.files.filter(item=>item.kind==='song');
assert.deepEqual(songs.map(item=>[item.demo_id,item.bpm,item.song_bars]),[[2,128,24],[3,144,32]]);
const references=manifest.files.filter(item=>item.kind==='demo');
assert.deepEqual(references.map(item=>[item.demo_id,item.bpm,item.song_bars]),[[0,118,8],[1,140,32]]);
assert(references.every(item=>item.mc1_original_arrangement));
const sha256=data=>createHash('sha256').update(data).digest('hex');
const tap=(e,button)=>{e.emu_input(0,1<<button);e.emu_input(0,0);};
for(const song of [...references,...songs]) {
    const reference=song.kind==='demo';
    const original=native.files.find(item=>item.kind===song.kind&&item.demo_id===song.demo_id);
    assert(original);
    const canonical=Object.fromEntries(['file','sha256','bytes','frames','sample_rate','channels','bits_per_sample']
        .map(key=>[key,original[key]]));
    const file=original.file.replace(/\.wav$/,'.mp3');
    const publicPath='build/site/audio/'+file;
    const mp3=fs.readFileSync(publicPath);
    assert.deepEqual(song,{...original,file,path:'audio/'+file,sha256:sha256(mp3),bytes:mp3.length,
        paired_file:original.paired_file?.replace(/\.wav$/,'.mp3')??null,canonical_wav:canonical});
    const probe=JSON.parse(execFileSync('ffprobe',['-v','error','-show_streams','-of','json',publicPath],
        {encoding:'utf8',timeout:15000}));
    assert.equal(probe.streams.length,1);
    assert.equal(probe.streams[0].codec_name,'mp3');
    assert.equal(Number(probe.streams[0].sample_rate),canonical.sample_rate);
    assert.equal(probe.streams[0].channels,canonical.channels);
    const decoded=execFileSync('ffmpeg',['-v','error','-nostdin','-i',publicPath,'-f','s16le','-'],
        {timeout:15000,maxBuffer:canonical.frames*canonical.channels*2+65536});
    assert.equal(decoded.length,canonical.frames*canonical.channels*2,'gapless MP3 decode retains duration, not sample equality');
    const wav=fs.readFileSync('build/canonical-audio/'+canonical.file);
    assert.equal(sha256(wav),canonical.sha256,'verify native WAV provenance before parsing PCM');
    assert.equal(wav.length,canonical.bytes);
    assert.equal(wav.toString('ascii',0,4),'RIFF');
    assert.equal(wav.toString('ascii',8,16),'WAVEfmt ');
    assert.equal(wav.readUInt16LE(20),1);
    assert.equal(wav.readUInt16LE(22),canonical.channels);
    assert.equal(wav.readUInt32LE(24),canonical.sample_rate);
    assert.equal(wav.readUInt16LE(34),canonical.bits_per_sample);
    assert.equal(wav.toString('ascii',36,40),'data');
    assert.equal(wav.readUInt32LE(40),canonical.frames*4);
    assert.equal(wav.length,44+canonical.frames*4);
    const e=new WebAssembly.Instance(module,{}).exports;
    new Uint8Array(e.memory.buffer,e.emu_flash_ptr(),e.emu_flash_size()).fill(255);
    e.emu_init();
    for(let i=0;i<3;i++)tap(e,7);
    e.emu_enc(2,song.demo_id-1);
    e.emu_enc(5,1);
    assert.equal(e.emu_state(0),song.bpm);
    assert.equal(e.emu_state(4),0);
    tap(e,12);
    assert.equal(e.emu_state(4),1);
    assert.equal(wav.readUInt32LE(24),44100);
    for(let frame=0;frame<song.frames;) {
        if(reference&&frame===song.performance_frames&&e.emu_state(4))tap(e,12);
        let count=Math.min(128,song.frames-frame);
        if(reference&&frame<song.performance_frames)count=Math.min(count,song.performance_frames-frame);
        e.emu_render(count);
        const output=new Float32Array(e.memory.buffer,e.emu_audio_ptr(),count*2);
        for(let sample=0;sample<output.length;sample++)
            assert.equal(Math.round(output[sample]*32768),wav.readInt16LE(44+frame*4+sample*2),
                `${song.file} frame ${frame+Math.floor(sample/2)} channel ${sample%2}`);
        frame+=count;
    }
    assert.equal(e.emu_state(4),0);
    console.log(`${reference?'MC1_DEMO':'SONG'}_WASM_PASS id=${song.demo_id} bpm=${song.bpm} bars=${song.song_bars} frames=${song.frames} PCM=exact`);
}
