// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';

const manifest=JSON.parse(fs.readFileSync('build/site/FIRMWARE.json','utf8'));
const source=fs.readFileSync('site/install.js','utf8');
const html=fs.readFileSync('site/install.html','utf8');
class Element {
    constructor() { this.children=[];this.dataset={}; }
    append(...nodes) { this.children.push(...nodes); }
    setAttribute(name,value) { this[name]=value; }
}
async function run(response,cause) {
    const nodes=Object.fromEntries(['metadata','files','build-metadata','limitations',
        'manifest-section','fetch-error','manifest-loading','manifest-slot'].map(id=>[id,new Element()]));
    for (const [id,node] of Object.entries(nodes)) {
        const tag=html.match(new RegExp(`<[^>]+id="${id}"[^>]*>`))?.[0];
        assert(tag,`missing DOM node ${id}`);
        node.hidden=/\bhidden\b/.test(tag);
    }
    let release,finish,actualRequest;
    const request=new Promise(resolve=>{release=resolve;});
    const completed=new Promise(resolve=>{finish=resolve;});
    nodes['manifest-slot'].setAttribute=(name,value)=>{
        nodes['manifest-slot'][name]=value;
        if(name==='aria-busy'&&value==='false')finish();
    };
    const context=vm.createContext({
        document:{currentScript:{src:'https://example.invalid/mc-1-mod/install.js?v=package-build-B'},
            querySelector:selector=>nodes[selector.slice(1)],createElement:()=>new Element()},
        URL,
        fetch:async(url,options)=>{
            actualRequest={url:String(url),cache:options?.cache};
            await request;return response();
        },
    });
    vm.runInContext(source,context);
    assert.equal(nodes['manifest-loading'].hidden,false);
    assert.equal(nodes['manifest-section'].hidden,true);
    assert.equal(nodes['fetch-error'].hidden,true);
    let timer;
    const deadline=new Promise((_,reject)=>{timer=setTimeout(()=>reject(new Error('manifest completion deadline')),5000);});
    release();
    try { await Promise.race([completed,deadline]); }
    finally { clearTimeout(timer); }
    assert.deepEqual(actualRequest,{url:'./FIRMWARE.json?v=package-build-B',cache:'no-store'});
    assert.equal(nodes['manifest-loading'].hidden,true);
    assert.equal(nodes['manifest-slot']['aria-busy'],'false');
    assert.equal(nodes['fetch-error'].hidden,!cause);
    assert.equal(nodes['manifest-section'].hidden,Boolean(cause));
    if(cause)assert.equal(nodes['fetch-error'].dataset.cause,cause);
    else assert.deepEqual(nodes.files.children.map(entry=>entry.children[2].href),manifest.files.map(file=>file.url+'?v='+file.sha256));
}
await run(()=>({ok:true,json:async()=>manifest}));
await run(()=>({ok:false,status:503}),'HTTP 503');
await run(()=>{throw new Error('offline');},'NETWORK');
await run(()=>({ok:true,json:async()=>{throw new SyntaxError();}}),'JSON');
await run(()=>({ok:true,json:async()=>({})}),'SCHEMA');
console.log('INSTALL_PASS pending/success/HTTP/network/JSON/schema states and real download URLs');
