// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import fs from 'node:fs';

const manifest=JSON.parse(fs.readFileSync('build/site/auditions.json','utf8'));
const module=new WebAssembly.Module(fs.readFileSync('build/mod1.wasm'));
const songs=manifest.files.filter(item=>item.kind==='song');
assert.deepEqual(songs.map(item=>[item.demo_id,item.bpm,item.song_bars]),[[2,128,24],[3,144,32]]);
const tap=(e,button)=>{e.emu_input(0,1<<button);e.emu_input(0,0);};
for(const song of songs) {
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
    const wav=fs.readFileSync('build/site/audio/'+song.file);
    assert.equal(wav.toString('ascii',0,4),'RIFF');
    assert.equal(wav.readUInt32LE(24),44100);
    assert.equal(wav.length,44+song.frames*4);
    for(let frame=0;frame<song.frames;) {
        const count=Math.min(128,song.frames-frame);
        e.emu_render(count);
        const output=new Float32Array(e.memory.buffer,e.emu_audio_ptr(),count*2);
        for(let sample=0;sample<output.length;sample++)
            assert.equal(Math.round(output[sample]*32768),wav.readInt16LE(44+frame*4+sample*2),
                `${song.file} frame ${frame+Math.floor(sample/2)} channel ${sample%2}`);
        frame+=count;
    }
    assert.equal(e.emu_state(4),0);
    console.log(`SONG_WASM_PASS id=${song.demo_id} bpm=${song.bpm} bars=${song.song_bars} frames=${song.frames} PCM=exact`);
}
