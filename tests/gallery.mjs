// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';

const manifest=JSON.parse(fs.readFileSync('build/site/auditions.json','utf8'));
assert.equal(manifest.schema_version,4);
assert.equal(manifest.profile_count,64);
assert.equal(manifest.files.length,285);
assert.equal(manifest.files.filter(item=>item.diagnostic).length,208);
assert.equal(manifest.files.filter(item=>item.kind==='source').length,144);
assert(manifest.files.every(item=>/^[\w.-]+\.mp3$/.test(item.file)));
class Element {
    constructor(tag) { this.tagName=tag;this.children=[];this.paused=true; }
    append(...nodes) { this.children.push(...nodes); }
    replaceChildren(...nodes) { this.children=nodes; }
    setAttribute(name,value) { this[name]=value; }
    pause() { this.paused=true; }
}
class Audio extends Element { constructor() { super('audio'); } }
function walk(node) { return [node,...node.children.flatMap(walk)]; }
const gallery=new Element('div'),status=new Element('p'),error=new Element('div');
let finished;
const done=new Promise(resolve=>finished=resolve);
Object.defineProperty(status,'textContent',{set(value){this.value=value;finished();}});
const handlers={};
let actualRequest;
let servedManifest=manifest;
const context=vm.createContext({
    document:{
        currentScript:{src:'https://example.invalid/mc-1-mod/gallery.js?v=audio-build-A'},
        querySelector:selector=>({'#gallery':gallery,'#gallery-status':status,'#gallery-error':error})[selector],
        createElement:tag=>tag==='audio'?new Audio():new Element(tag),
        querySelectorAll:tag=>walk(gallery).filter(node=>node.tagName===tag),
        addEventListener:(event,handler)=>{handlers[event]=handler;},
    },
    HTMLAudioElement:Audio,
    URL,
    fetch:async(url,options)=>{
        actualRequest={url:String(url),cache:options?.cache};
        return {ok:true,json:async()=>servedManifest};
    },
});
vm.runInContext(fs.readFileSync('site/gallery.js','utf8'),context);
let timer;
try { await Promise.race([done,new Promise((_,reject)=>{timer=setTimeout(()=>reject(new Error('gallery completion deadline')),5000);})]); }
finally { clearTimeout(timer); }
assert.deepEqual(actualRequest,{url:'./auditions.json?v=audio-build-A',cache:'no-store'});
assert.equal(error.hidden,undefined,'valid manifest must not expose the error state');
const audios=walk(gallery).filter(node=>node instanceof Audio);
assert.equal(audios.length,manifest.files.length,'every rendered audition has a control');
const primaryPresets=gallery.children[2];
assert(primaryPresets);
assert.equal(walk(primaryPresets).filter(node=>node instanceof Audio).length,64);
assert.deepEqual(walk(primaryPresets).filter(node=>node instanceof Audio).map(audio=>audio.src).sort(),
    manifest.files.filter(item=>item.kind==='preset'&&item.render_mode==='full').map(item=>'./audio/'+encodeURIComponent(item.file)+'?v='+item.sha256).sort());
const diagnostics=gallery.children.find(node=>node.tagName==='details');
assert(diagnostics);
assert.notEqual(diagnostics.open,true);
assert.equal(walk(diagnostics).filter(node=>node instanceof Audio).length,manifest.files.filter(item=>item.diagnostic).length);
assert.deepEqual(audios.map(audio=>audio.src).sort(),manifest.files.map(item=>'./audio/'+encodeURIComponent(item.file)+'?v='+item.sha256).sort());
assert(audios.every(audio=>audio.controls&&audio.preload==='none'));
assert.equal(new Set(audios.map(audio=>audio.src)).size,manifest.files.length);
assert.deepEqual(manifest.files.filter(item=>item.kind==='song').map(item=>item.demo_id),[2,3]);
assert.deepEqual(manifest.files.filter(item=>item.kind==='demo').map(item=>item.demo_id),[0,1]);
assert(manifest.files.filter(item=>item.kind==='demo').every(item=>item.mc1_original_arrangement));
assert.equal(gallery.children[0].id,'mc1-demos');
assert.equal(gallery.children[1].id,'beat-songs');
assert.deepEqual(audios.slice(0,2).map(audio=>audio.src),manifest.files.filter(item=>item.kind==='demo').map(item=>'./audio/'+item.file+'?v='+item.sha256));
assert.deepEqual(walk(gallery).filter(node=>node.tagName==='a').map(link=>link.href).sort(),audios.map(audio=>audio.src).sort());
audios[0].paused=false;audios[1].paused=false;
handlers.play({target:audios[1]});
assert.equal(audios[0].paused,true);
assert.equal(audios[1].paused,false,'the newly playing audition remains playing');
// Given actual source-only entries, when rendered, then slot 0 exposes its typed source.
for(const item of manifest.files.filter(item=>item.kind==='source'&&item.preset_id>=32&&item.source_flags===1)) {
    const row=walk(gallery).find(node=>node.tagName==='article'&&walk(node).some(child=>
        child instanceof Audio&&child.src.includes('/'+item.file+'?')));
    assert(row);
    assert(row.children[0].textContent.includes(item.source_names[0]));
    assert(row.children[1].textContent.includes(item.source_names[0]));
    assert(!row.children[0].textContent.includes('PCM'));
    assert(!row.children[1].textContent.includes('PCM'));
}
// Given malformed profile sets, when loaded, then the exact ID/count contract rejects them.
const full=manifest.files.filter(item=>item.kind==='preset'&&item.render_mode==='full');
for(const files of [
    manifest.files.filter(item=>item!==full[63]),
    [...manifest.files,full[0]],
    manifest.files.map(item=>item===full[63]?{...item,preset_id:0}:item),
    manifest.files.map(item=>item===full[63]?{...item,preset_id:64}:item),
]) {
    servedManifest={...manifest,files};
    error.hidden=true;
    const completed=new Promise(resolve=>{finished=resolve;});
    vm.runInContext(fs.readFileSync('site/gallery.js','utf8'),context);
    let deadline;
    try { await Promise.race([completed,new Promise((_,reject)=>{
        deadline=setTimeout(()=>reject(new Error('gallery rejection deadline')),5000);
    })]); }
    finally { clearTimeout(deadline); }
    assert.equal(error.hidden,false);
}
console.log(`GALLERY_PASS all ${audios.length} unique controls/downloads, 64 full presets, typed source labels, exact ID rejection, collapsed diagnostics, two MC-1 demos first, no eager loading, exclusive playback`);
