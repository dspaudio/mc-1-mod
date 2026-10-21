/* SPDX-License-Identifier: GPL-3.0-only
 * AudioWorklet 안에서 공유 C 코어와 실제 C UI를 실행한다.
 * 출력 레이트 변환도 C가 담당하며 pi32v2 CPU 타이밍은 모사하지 않는다. */
class Mc1ModProcessor extends AudioWorkletProcessor {
    constructor(options) {
        super();
        try {
        const mod = options.processorOptions.wasmModule || new WebAssembly.Module(options.processorOptions.wasmBytes);
        this.e = new WebAssembly.Instance(mod, {}).exports;
        const img = options.processorOptions.flashImage;
        const fl = new Uint8Array(this.e.memory.buffer, this.e.emu_flash_ptr(), this.e.emu_flash_size());
        fl.fill(255);
        if (img) {
            if (img.byteLength !== fl.length) throw new Error('Saved image size mismatch. The original browser save was not changed.');
            fl.set(new Uint8Array(img));
        }
        this.lastFlashGen = 0;
        this.e.emu_init();
        if (!this.e.emu_resample_init(options.processorOptions.outputRate)) throw new Error('Unsupported audio output rate.');
        const pack = new Uint8Array(this.e.memory.buffer, this.e.emu_pack_ptr(), this.e.emu_pack_bytes());
        const expectedPack = [77,79,68,49,1,0,5,0];
        if (pack.length !== expectedPack.length || pack.some((byte,i) => byte !== expectedPack[i])) throw new Error('mc-1-mod pack identity mismatch.');
        const saved = this.e.emu_store_status();
        if (saved === 3) this.port.postMessage({t:'storage-reset',message:'Another pack’s save was preserved. SAVE is locked.'});
        else if (saved === 2) this.port.postMessage({t:'storage-reset',message:'Could not restore the corrupted project. It was not automatically overwritten.'});
        else if (saved === 4) throw new Error('Could not read project storage.');
        this.lastFlashGen = this.e.emu_flash_gen();   // boot-time writes need no save
        this.fbPtr = this.e.emu_fb_ptr();
        this.audioPtr = this.e.emu_audio_ptr();
        this.ledPtr = this.e.emu_led_ptr();
        this.ledDimPtr = this.e.emu_led_dim_ptr();
        this.lastGen = -1;
        this.blocks = 0;
        this.running = true;
        this.inputNotes = 0;
        this.inputButtons = 0;
        this.requestedButtons = 0;
        this.buttonQueue = [];
        this.midiBatch = null; this.midiOutputPending = false;
        this.port.onmessage = (ev) => {
            const m = ev.data;
            if (m.t === 'input') {
                this.e.emu_input(m.notes,m.buttons);
            }
            else if (m.t === 'enc') this.e.emu_enc_at(m.e, m.steps, (currentTime * 1000) >>> 0);
            else if (m.t === 'midi-output-ack') this.midiOutputPending = false;
            else if (m.t === 'midi-reset') {
                this.midiBatch = null; this.e.emu_midi_reset(1);
            }
            else if (m.t === 'midi-input') {
                if (this.midiBatch || !Array.isArray(m.messages) || m.messages.length > 64 ||
                    m.messages.some(bytes => !Array.isArray(bytes) || !bytes.length || bytes.length > 3 || bytes.some(b => !Number.isInteger(b) || b < 0 || b > 255))) {
                    this.midiBatch = null; this.e.emu_midi_reset(1);
                    this.port.postMessage({t:'midi-overflow'});
                    this.port.postMessage({t:'midi-ack',epoch:m.epoch});
                } else this.midiBatch = m;
            }
            else if (m.t === 'start') this.e.emu_start();
            else if (m.t === 'adc') this.e.emu_adc_set(m.ch, m.v);

            else if (m.t === 'stop') this.running = false;
        };
        this.postFrame();
        this.port.postMessage({t:'ready'});
        } catch(error) { this.failed=true; this.port.postMessage({t:'error',message:String(error)}); }
    }

    postFrame() {
        const gen = this.e.emu_fb_gen();
        const leds = new Uint8Array(this.e.memory.buffer, this.ledPtr, 11).slice();
        const dims = new Uint8Array(this.e.memory.buffer, this.ledDimPtr, 11).slice();
        if (gen !== this.lastGen) {
            this.lastGen = gen;
            const fb = new Uint16Array(this.e.memory.buffer, this.fbPtr, 240 * 240).slice();
            this.port.postMessage({ t: 'frame', fb, leds, dims }, [fb.buffer]);
        } else {
            this.port.postMessage({ t: 'leds', leds, dims });
        }
    }

    process(inputs, outputs) {
        if(this.failed) return false;
        if (this.midiBatch) {
            const batch = this.midiBatch; this.midiBatch = null;
            for (const bytes of batch.messages) for (const byte of bytes) this.e.emu_midi_byte(1,byte);
            this.port.postMessage({t:'midi-ack',epoch:batch.epoch});
        }
        this.e.emu_ui_tick((currentTime * 1000) >>> 0);
        const out = outputs[0];
        const n = out[0].length;             // 128
        this.e.emu_render_out(n);
        const midi = [];
        if (!this.midiOutputPending) for (let i=0;i<272;i++) { const message = this.e.emu_midi_pop(); if (!message) break; midi.push(message); }
        if (midi.length) { this.midiOutputPending = true; this.port.postMessage({t:'midi-output',messages:midi}); }
        const a = new Float32Array(this.e.memory.buffer, this.audioPtr, 2 * n);
        const L = out[0], R = out[1] || out[0];
        for (let i = 0; i < n; i++) {
            L[i] = a[2 * i];
            R[i] = a[2 * i + 1];
        }
        if (++this.blocks % 6 === 0) {       // 출력 콜백 여섯 번마다 실제 C UI를 갱신한다.
            if (this.buttonQueue.length) {
                this.inputButtons = this.buttonQueue.shift();
                this.e.emu_input(this.inputNotes, this.inputButtons);
            }
            this.e.emu_frame();
            this.postFrame();
        }
        if (this.blocks % 256 === 0) {       // 실제 경과 시간은 출력 레이트에 따라 달라진다.
            const gen = this.e.emu_flash_gen();
            if (gen !== this.lastFlashGen) {
                this.lastFlashGen = gen;
                const fl = new Uint8Array(this.e.memory.buffer, this.e.emu_flash_ptr(), this.e.emu_flash_size()).slice();
                this.port.postMessage({ t: 'flash', data: fl.buffer }, [fl.buffer]);
            }
        }
        return this.running;
    }
}
registerProcessor('mc1mod', Mc1ModProcessor);
