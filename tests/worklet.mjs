// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 dspaudio
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';

const module = new WebAssembly.Module(fs.readFileSync(process.argv[2] || 'build/mod1.wasm'));
const source = fs.readFileSync('site/worklet.js', 'utf8');

function create(rate, flashImage) {
    const messages = [];
    let Processor;
    const context = vm.createContext({
        currentTime: 0, WebAssembly, console,
        AudioWorkletProcessor: class {
            constructor() {
                this.port = {
                    onmessage: null,
                    postMessage(message, transfer = []) {
                        messages.push(structuredClone(message, { transfer }));
                    },
                };
            }
        },
        registerProcessor(name, constructor) {
            assert.equal(name, 'mc1mod');
            Processor = constructor;
        },
    });
    vm.runInContext(source, context);
    const processor = new Processor({ processorOptions: {
        wasmModule: module, outputRate: rate, flashImage,
    } });
    return { processor, context, messages };
}

function renderBlock(state) {
    const left = new Float32Array(128), right = new Float32Array(128);
    const running = state.processor.process([], [[left, right]]);
    state.context.currentTime += 128 / 48000;
    return { running, left, right };
}

// 모사한 것은 브라우저 port뿐이며 C 코어·UI·저장·오디오는 실제 WASM이다.
const state = create(48000);
assert(state.messages.some(message => message.t === 'ready'));
const frame = state.messages.find(message => message.t === 'frame');
assert.equal(frame.fb.length, 240 * 240);
assert(frame.fb.some(pixel => pixel !== 0));
assert.equal(frame.leds.length, 11);
state.processor.port.onmessage({ data: {
    t: 'midi-input', epoch: 9, messages: [[0xc0, 8], [0xb0, 17, 80], [0x90, 60, 100]],
} });
const rendered = renderBlock(state);
assert.equal(rendered.running, true);
assert(rendered.left.some(sample => sample !== 0) || rendered.right.some(sample => sample !== 0));
assert([...rendered.left, ...rendered.right].every(sample => Number.isFinite(sample) && Math.abs(sample) < 1));
assert.equal(state.processor.e.emu_state(43), 8);
assert.equal(state.processor.e.emu_state(41), 80);
assert(state.messages.some(message => message.t === 'midi-ack' && message.epoch === 9));
assert.equal(state.processor.e.emu_state(46), 128);

// callback 이전의 짧은 패널 press/release도 C transport에 반영한다.
state.processor.port.onmessage({ data: { t: 'input', notes: 0, buttons: 1 << 12 } });
state.processor.port.onmessage({ data: { t: 'input', notes: 0, buttons: 0 } });
assert.equal(state.processor.e.emu_state(4), 1);

// 잘못된 MIDI 배치는 ACK 후 노트를 정리하며 조용히 보류하지 않는다.
state.processor.port.onmessage({ data: {
    t: 'midi-input', epoch: 10, messages: [[0x90, 60, 256]],
} });
assert(state.messages.some(message => message.t === 'midi-overflow'));
assert(state.messages.some(message => message.t === 'midi-ack' && message.epoch === 10));

// 외래 저장은 RAM factory로 실행하되 C SAVE와 브라우저 flash 전송은 잠긴다.
const foreign = new Uint8Array(state.processor.e.emu_flash_size()).fill(255);
foreign.set([77, 67, 70, 77]);
const foreignSnapshot = foreign.slice();
const protectedState = create(48000, foreign.buffer);
assert(protectedState.messages.some(message => message.t === 'ready'));
assert(protectedState.messages.some(message => message.t === 'storage-reset'));
for (let press = 0; press < 2; press++) {
    protectedState.processor.port.onmessage({ data: { t: 'input', notes: 0, buttons: 1 << 9 } });
    protectedState.processor.port.onmessage({ data: { t: 'input', notes: 0, buttons: 0 } });
}
for (let block = 0; block < 256; block++) renderBlock(protectedState);
assert.equal(protectedState.processor.e.emu_store_status(), 3);
assert.equal(protectedState.processor.e.emu_flash_gen(), 0);
assert(!protectedState.messages.some(message => message.t === 'flash'));
assert.deepEqual(new Uint8Array(foreign.buffer), foreignSnapshot);
assert.deepEqual(new Uint8Array(protectedState.processor.e.memory.buffer,
    protectedState.processor.e.emu_flash_ptr(), foreignSnapshot.length), foreignSnapshot);

// 불명확한 길이와 지원하지 않는 rate는 원본을 바꾸지 않고 시작을 거부한다.
const invalidImage = new Uint8Array([1, 2, 3]);
for (const bad of [create(32000), create(48000, invalidImage.buffer)]) {
    assert(bad.messages.some(message => message.t === 'error'));
    assert(!bad.messages.some(message => message.t === 'ready'));
    assert.equal(renderBlock(bad).running, false);
}
assert.deepEqual(invalidImage, new Uint8Array([1, 2, 3]));
console.log('WORKLET_PASS real WASM audio/UI/MIDI, protected storage, transferred frames, startup failures');
