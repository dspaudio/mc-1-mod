// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {createHash} from 'node:crypto';

const info=JSON.parse(fs.readFileSync('build/site/build-info.json','utf8'));
const manifest=JSON.parse(fs.readFileSync('build/site/auditions.json','utf8'));
assert.equal(manifest.schema_version,4);
assert.equal(manifest.wasm_source_input_sha256,info.source_input_sha256);
assert.equal(manifest.render_complete,true);
assert.equal(manifest.mechanical_failures,0);
assert.equal(manifest.subjective_listening_approved,false);
assert.equal(manifest.target_test_performed,false);
assert.equal(manifest.device_test_performed,false);
assert.deepEqual(manifest.delivery,{format:'mp3',encoder:'LAME',vbr_quality:2,lossy:true,sample_exact:false});
assert.equal(manifest.measurement_basis,'canonical_wav');
const names=new Set(manifest.files.map(item=>item.file));
assert.equal(names.size,manifest.files.length);
assert.deepEqual(fs.readdirSync('build/site/audio').sort(),[...names].sort());
assert(!fs.readdirSync('build/site',{recursive:true}).some(name=>/\.wav$/i.test(name)),'no canonical WAV enters the public site');
for(const item of manifest.files) {
    assert.match(item.file,/^[\w.-]+\.mp3$/);
    assert.equal(item.path,'audio/'+item.file);
    const bytes=fs.readFileSync('build/site/'+item.path);
    assert.equal(bytes.length,item.bytes);
    assert.equal(createHash('sha256').update(bytes).digest('hex'),item.sha256);
    assert.match(item.canonical_wav.file,/^[\w.-]+\.wav$/);
    assert.match(item.canonical_wav.sha256,/^[a-f0-9]{64}$/);
    for(const key of ['frames','sample_rate','channels','bits_per_sample'])
        assert.equal(item.canonical_wav[key],item[key]);
    assert.equal(item.mechanical_status,'PASS');
    assert.equal(item.clipped_samples,0);
    assert.equal(item.ended_tails,true);
    if(item.paired_file)assert(names.has(item.paired_file));
}
const key=info.source_input_sha256.slice(0,16);
assert.match(key,/^[a-f0-9]{16}$/);
for(const name of ['index.html','install.html','emulator.html']) {
    const html=fs.readFileSync('build/site/'+name,'utf8');
    assert.match(html,/<html\b[^>]*\blang=["']en["']/);
    const assets=[...html.matchAll(/<(?:script|link)\b[^>]*\b(?:src|href)=["']([^"']+)["']/g)]
        .map(match=>match[1]).filter(url=>!url.includes('://')&&/\.(?:js|css)(?:\?|$)/.test(url));
    assert(assets.length>0,name);
    for(const asset of assets)assert.equal(new URL(asset,'https://local.invalid/').searchParams.get('v'),key,asset);
}
const frontend=fs.readFileSync('build/site/emu.js','utf8');
assert(frontend.includes('worklet.js?v='+key));
assert(frontend.includes('.wasm?v='+key));
console.log(`SITE_CACHE_PASS ${names.size} hash-verified MP3 previews, no public WAV, source-fingerprint asset URLs`);
