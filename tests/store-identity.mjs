// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 dspaudio
import assert from 'node:assert/strict';
import fs from 'node:fs';

const module = new WebAssembly.Module(fs.readFileSync(process.argv[2] || 'build/mod1.wasm'));
const identity = [77, 79, 68, 49, 1, 0, 1, 0];

function boot(image) {
    const e = new WebAssembly.Instance(module, {}).exports;
    const flash = new Uint8Array(e.memory.buffer, e.emu_flash_ptr(), e.emu_flash_size());
    flash.fill(255);
    if (image) flash.set(image);
    e.emu_init();
    assert.equal(e.emu_pack_bytes(), identity.length);
    assert.deepEqual(Array.from(new Uint8Array(e.memory.buffer, e.emu_pack_ptr(), identity.length)), identity);
    return { e, flash };
}

function save(e) {
    e.emu_ui_tick(1);
    for (let press = 0; press < 2; press++) {
        e.emu_input(0, 1 << 9);
        e.emu_input(0, 0);
    }
}

// 실제 C UI의 명시적 SAVE로 만든 이미지만 CURRENT로 복원한다.
const current = boot();
assert.equal(current.e.emu_store_status(), 1);
const initialCutoff = current.e.emu_parameter(0, 4);
current.e.emu_enc(2, -7);
const savedCutoff = current.e.emu_parameter(0, 4);
assert.notEqual(savedCutoff, initialCutoff);
save(current.e);
assert.equal(current.e.emu_store_status(), 0);
assert(current.e.emu_flash_gen() > 0);
const image = current.flash.slice();
const restored = boot(image);
assert.equal(restored.e.emu_store_status(), 0);
assert.equal(restored.e.emu_parameter(0, 4), savedCutoff);

// 다른 magic과 HEADER 밖의 불명확한 데이터도 자동 초기화·덮어쓰기하지 않는다.
for (const dirtyTail of [false, true]) {
    const foreign = new Uint8Array(image.length).fill(255);
    if (dirtyTail) foreign[foreign.length - 1] = 0;
    else foreign.set([77, 67, 70, 77]);
    const blocked = boot(foreign);
    assert.equal(blocked.e.emu_store_status(), 3);
    const generation = blocked.e.emu_flash_gen();
    save(blocked.e);
    assert.equal(blocked.e.emu_flash_gen(), generation);
    assert.deepEqual(blocked.flash, foreign);
}

// 현재 A와 외래 B가 섞여도 A는 복원하되 SAVE는 잠근다.
const mixed = image.slice();
mixed.set([77, 67, 70, 77], mixed.length / 2);
const protectedCurrent = boot(mixed);
assert.equal(protectedCurrent.e.emu_store_status(), 3);
assert.equal(protectedCurrent.e.emu_parameter(0, 4), savedCutoff);
const before = protectedCurrent.flash.slice();
const generation = protectedCurrent.e.emu_flash_gen();
save(protectedCurrent.e);
assert.deepEqual(protectedCurrent.flash, before);
assert.equal(protectedCurrent.e.emu_flash_gen(), generation);

// 자신의 유효한 header/identity 아래 손상된 body는 프로젝트를 부분 복원하지 않는다.
const corrupt = image.slice();
corrupt[40] ^= 1;
const rejected = boot(corrupt);
assert.equal(rejected.e.emu_store_status(), 2);
assert.equal(rejected.e.emu_parameter(0, 4), initialCutoff);
assert.equal(rejected.e.emu_flash_gen(), 0);
console.log('STORE_IDENTITY_PASS current restore, foreign/tail protection, mixed banks, own-corrupt rejection');
