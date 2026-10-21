// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 dspaudio
import assert from 'node:assert/strict';
import fs from 'node:fs';

const module = new WebAssembly.Module(fs.readFileSync(process.argv[2] || 'build/mod1.wasm'));
assert.deepEqual(WebAssembly.Module.imports(module), []);

function boot(rate, playing = true) {
    const e = new WebAssembly.Instance(module, {}).exports;
    new Uint8Array(e.memory.buffer, e.emu_flash_ptr(), e.emu_flash_size()).fill(255);
    e.emu_init();
    assert.equal(e.emu_resample_init(rate), 1);
    if (playing) {
        e.emu_input(0, 1 << 12);
        e.emu_input(0, 0);
        assert.equal(e.emu_state(4), 1);
    }
    return e;
}

function render(e, frames, chunks, direct = false) {
    const result = [];
    let produced = 0, index = 0;
    while (produced < frames) {
        const n = Math.min(chunks[index++ % chunks.length], frames - produced);
        if (direct) e.emu_render(n);
        else e.emu_render_out(n);
        const samples = new Float32Array(e.memory.buffer, e.emu_audio_ptr(), n * 2);
        for (const sample of samples) {
            assert(Number.isFinite(sample));
            assert(Math.abs(sample) < 1);
        }
        result.push(Buffer.from(new Uint8Array(e.memory.buffer, e.emu_audio_ptr(), n * 8)));
        produced += n;
    }
    return Buffer.concat(result);
}

// 같은 실제 C 이벤트열에서 44.1k bypass와 기존 렌더 출력이 같아야 한다.
{
    const direct = render(boot(44100), 8192, [128], true);
    const bypass = render(boot(44100), 8192, [128]);
    assert.deepEqual(bypass, direct);
    assert(direct.some(byte => byte !== 0));
}

// 렌더 호출을 나눠도 오디오와 유리수 위상·소비 프레임 수가 같아야 한다.
for (const rate of [48000, 96000]) {
    const whole = boot(rate), split = boot(rate);
    const reference = render(whole, 8192, [512]);
    const partitioned = render(split, 8192, [1, 7, 16, 127, 3, 256]);
    assert(reference.some(byte => byte !== 0));
    assert.deepEqual(partitioned, reference);
    for (const state of [44, 45, 46]) assert.equal(split.emu_state(state), whole.emu_state(state));
    assert.equal(whole.emu_state(44), (8192 * 44100) % rate);
    assert.equal(whole.emu_state(45), 2 + Math.floor(8192 * 44100 / rate));
    assert.equal(whole.emu_state(46), 8192);
    const before = [44, 45, 46].map(state => whole.emu_state(state));
    whole.emu_render_out(0);
    assert.deepEqual([44, 45, 46].map(state => whole.emu_state(state)), before);
    assert.equal(whole.emu_resample_init(32000), 0);
    assert.deepEqual([44, 45, 46].map(state => whole.emu_state(state)), before);
}

// 10분 상당의 실제 C 렌더 후에도 위상과 입력 프레임 누계가 정확해야 한다.
for (const rate of [48000, 96000]) {
    const e = boot(rate, false), frames = rate * 600;
    for (let left = frames; left > 0;) {
        const n = Math.min(512, left);
        e.emu_render_out(n);
        left -= n;
    }
    assert.equal(e.emu_state(44), (frames * 44100) % rate);
    assert.equal(e.emu_state(45), 2 + Math.floor(frames * 44100 / rate));
    assert.equal(e.emu_state(46), frames);
}
console.log('RESAMPLE_PASS bypass, partitions, bounded PCM, zero rational drift, invalid-rate state preservation');
