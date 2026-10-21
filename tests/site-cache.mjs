// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {createHash} from 'node:crypto';

const info=JSON.parse(fs.readFileSync('build/site/build-info.json','utf8'));
const manifest=JSON.parse(fs.readFileSync('build/site/auditions.json','utf8'));
assert.equal(manifest.schema_version,4);
assert.equal(manifest.profile_count,64);
assert.equal(manifest.source_mask_semantics,'slot-bits');
assert.equal(manifest.files.length,285);
assert.equal(manifest.files.filter(item=>item.diagnostic).length,208);
const full=manifest.files.filter(item=>item.kind==='preset'&&item.render_mode==='full')
    .sort((a,b)=>a.preset_id-b.preset_id);
assert.deepEqual(full.map(item=>item.preset_id),Array.from({length:64},(_,id)=>id));
const dry=manifest.files.filter(item=>item.kind==='preset'&&item.render_mode==='dry-diagnostic')
    .sort((a,b)=>a.preset_id-b.preset_id);
assert.deepEqual(dry.map(item=>item.preset_id),Array.from({length:64},(_,id)=>id));
const sources=manifest.files.filter(item=>item.kind==='source');
assert.equal(sources.length,144);
assert.equal(manifest.files.filter(item=>item.preset_id<32).length,141);
const expectedSources=full.flatMap(item=>[1,2,4]
    .filter(bit=>(item.available_source_flags&(item.available_source_flags-1))&&(item.available_source_flags&bit))
    .map(bit=>[item.preset_id,bit]));
assert.deepEqual(sources.map(item=>[item.preset_id,item.source_flags]),expectedSources);
assert.deepEqual(full.slice(0,32).map(item=>item.available_source_flags),
    [6,4,6,6,7,7,6,6,7,5,7,7,5,7,6,6,6,6,4,7,6,4,6,6,6,5,4,7,4,7,6,7]);
assert.deepEqual(full.slice(32).map(item=>item.role),
    [...Array(12).fill('80S SYNTH'),...Array(10).fill('90S SYNTH'),...Array(10).fill('00S SYNTH')]);
assert.equal(full.slice(32).filter(item=>item.source_flags===6&&item.available_source_flags===6).length,16);
assert.equal(full.slice(32).filter(item=>item.source_flags===7&&item.available_source_flags===7).length,16);
for(const kind of ['FM2','VA2'])assert.equal(full.slice(32).filter(item=>item.source_names[0]===kind).length,8);
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
    for(const profile of [...(item.preset_id>=0?[item]:[]),...item.patches]) {
        const id=profile===item?item.preset_id:profile.profile;
        const mask=profile===item?item.source_flags:profile.mask;
        assert.equal(profile.source_names.length,3);
        assert.deepEqual(profile.source_names.slice(1),['FM','VA']);
        assert.equal(profile.source_names[0],id<32?'PCM':profile.aux_profile?.kind??'---');
        assert.equal(profile.aux_profile===null,id<32||!(profile.available_source_flags&1));
        if(profile.aux_profile) {
            assert(['FM2','VA2'].includes(profile.aux_profile.kind));
            assert(Number.isInteger(profile.aux_profile.tuning_q16)&&profile.aux_profile.tuning_q16>0);
        }
        const order=id<32?[0,1,2]:[1,2,0];
        assert.equal(profile.source_configuration,order.filter(slot=>mask&profile.available_source_flags&(1<<slot))
            .map(slot=>profile.source_names[slot]).join(' + '));
    }
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
    assert.equal(item.normalization_gain,1);
    assert.equal(item.adaptive_normalization,false);
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
