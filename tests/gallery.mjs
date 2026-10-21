// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';

const manifest=JSON.parse(fs.readFileSync('build/site/auditions.json','utf8'));
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
const context=vm.createContext({
    document:{
        querySelector:selector=>({'#gallery':gallery,'#gallery-status':status,'#gallery-error':error})[selector],
        createElement:tag=>tag==='audio'?new Audio():new Element(tag),
        querySelectorAll:tag=>walk(gallery).filter(node=>node.tagName===tag),
        addEventListener:(event,handler)=>{handlers[event]=handler;},
    },
    HTMLAudioElement:Audio,
    fetch:async()=>({ok:true,json:async()=>manifest}),
});
vm.runInContext(fs.readFileSync('site/gallery.js','utf8'),context);
let timer;
try { await Promise.race([done,new Promise((_,reject)=>{timer=setTimeout(()=>reject(new Error('gallery completion deadline')),5000);})]); }
finally { clearTimeout(timer); }
assert.equal(error.hidden,undefined,'valid manifest must not expose the error state');
const audios=walk(gallery).filter(node=>node instanceof Audio);
assert.equal(audios.length,98,'all presets, source comparisons, kits, songs and phrases are present');
assert.deepEqual(audios.map(audio=>audio.src).sort(),manifest.files.map(item=>'./audio/'+encodeURIComponent(item.file)+'?v='+item.sha256).sort());
assert(audios.every(audio=>audio.controls&&audio.preload==='none'));
assert.equal(new Set(audios.map(audio=>audio.src)).size,98);
assert.deepEqual(manifest.files.filter(item=>item.kind==='song').map(item=>item.demo_id),[2,3]);
assert.equal(gallery.children[0].id,'beat-songs');
assert.deepEqual(audios.slice(0,2).map(audio=>audio.src),manifest.files.filter(item=>item.kind==='song').map(item=>'./audio/'+item.file+'?v='+item.sha256));
assert.deepEqual(walk(gallery).filter(node=>node.tagName==='a').map(link=>link.href).sort(),audios.map(audio=>audio.src).sort());
audios[0].paused=false;audios[1].paused=false;
handlers.play({target:audios[1]});
assert.equal(audios[0].paused,true);
assert.equal(audios[1].paused,false,'the newly playing audition remains playing');
console.log('GALLERY_PASS all98 unique controls/downloads, two songs first, no eager loading, exclusive playback');
