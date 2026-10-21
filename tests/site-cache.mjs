// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import fs from 'node:fs';

const info=JSON.parse(fs.readFileSync('build/site/build-info.json','utf8'));
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
console.log('SITE_CACHE_PASS English language attributes and source-fingerprint asset URLs');
