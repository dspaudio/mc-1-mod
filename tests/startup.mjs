// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';

const source = fs.readFileSync('site/emu.js', 'utf8');
const start = source.indexOf('async function powerOn()');
const end = source.indexOf("document.getElementById('power').addEventListener", start);

async function run(mode) {
    const status = { textContent: '' };
    const caption = { textContent: '' };
    const power = { style: {}, removed: false, querySelector: () => caption,
        remove() { this.removed = true; } };
    let closed = false, disconnected = false;
    const cleanupStarted = Promise.withResolvers();
    const cleanupGate = Promise.withResolvers();
    const sandbox = {
        ctx: null, node: null, FW: 'mod1', console, midi:{message(){},disconnect(){}},
        document: { getElementById: id => id === 'power' ? power : status },
        loadFlash: async () => null, saveFlash() {}, drawFB() {}, applyLeds() {}, sendMaster() {},
        fetch: async () => ({ ok: mode !== 'http', status: 404, arrayBuffer: async () => new ArrayBuffer(8) }),
        WebAssembly: { compile: async () => ({}) },
        setTimeout: (fn, ms) => setTimeout(fn, ms >= 1000 ? 10 : ms), clearTimeout,
        AudioContext: class {
            get sampleRate() {
                return mode === 'rate48' ? 48000 : mode === 'rate96' ? 96000 :
                    mode === 'rateUnsupported' ? 32000 : 44100;
            }
            audioWorklet = { addModule: async () => { if (mode === 'module') throw new Error('Module unavailable'); } };
            async resume() {
                if (mode === 'resume' || mode === 'cleanup') throw new Error('Resume denied');
                if (mode === 'suspended') await new Promise(() => {});
            }
            async close() {
                if (mode === 'cleanup') {
                    cleanupStarted.resolve();
                    await cleanupGate.promise;
                }
                closed = true;
            }
        },
        AudioWorkletNode: class {
            constructor(context, name, options) {
                assert.equal(name, 'mc1mod');
                assert.equal(options.processorOptions.outputRate, context.sampleRate);
            }
            port = { postMessage() {} };
            connect() {
                if (mode === 'timeout') return;
                queueMicrotask(() => {
                    if (mode === 'processor') this.onprocessorerror();
                    else {
                    if (mode === 'storage-reset') this.port.onmessage({data:{t:'storage-reset',message:'pack-warning-sentinel'}});
                    this.port.onmessage({ data: mode === 'init' ?
                        { t: 'error', message: 'Invalid WASM' } : { t: 'ready' } });
                    }
                });
            }
            disconnect() { disconnected = true; }
        },
    };
    vm.createContext(sandbox);
    vm.runInContext(source.slice(start, end), sandbox);
    const starting = vm.runInContext('powerOn()', sandbox);
    if (mode === 'cleanup') {
        let deadline;
        try {
            await Promise.race([cleanupStarted.promise, new Promise((resolve, reject) => {
                deadline = setTimeout(() => reject(new Error('Cleanup did not start')), 5000);
            })]);
            assert.equal(power.style.pointerEvents, 'none');
            assert.equal(closed, false);
        } finally {
            clearTimeout(deadline);
            cleanupGate.resolve();
        }
    }
    await starting;
    if (['ready', 'storage-reset', 'rate48', 'rate96'].includes(mode)) {
        assert(power.removed);
        assert.match(status.textContent, /audio ready/);
        if (mode === 'storage-reset') assert(status.textContent.endsWith(' · pack-warning-sentinel'));
        assert(!closed);
        sandbox.node.onprocessorerror();
        assert.match(status.textContent, /error/i);
    } else {
        assert.equal(power.removed, false, `${mode}: POWER ON must remain available on startup failure`);
        assert.match(caption.textContent, /Cannot start:/, mode);
        assert.equal(power.style.pointerEvents, '', mode);
        assert(closed, mode);
        assert.equal(sandbox.node, null, mode);
        if (['init', 'processor', 'timeout', 'resume', 'suspended', 'cleanup'].includes(mode)) assert(disconnected, mode);
        mode = 'ready';
        await vm.runInContext('powerOn()', sandbox);
        assert(power.removed, 'A second POWER ON attempt must succeed after failure');
        assert.match(status.textContent, /audio ready/);
    }
}
for (const mode of ['init', 'processor', 'timeout', 'module', 'http', 'resume', 'suspended', 'cleanup', 'ready', 'storage-reset', 'rate48', 'rate96', 'rateUnsupported']) await run(mode);
console.log('PASS startup ready, processor/init/module/HTTP/resume failure, timeout, cleanup and retry state');
