# Provenance and licenses

This project's code, original presets, mathematical sound sources, and UI are provided under GPL-3.0-only. The full license is in `LICENSE`. Copyright (C) 2026 dspaudio.

The completed initial POC used the MC-1 C core from a separate checkout on a read-only basis. The standalone firmware/web integration brings the required source copies into this project and records per-file provenance and modification status in `data/provenance-*.json`. Original copyright and SPDX notices are retained. The POC verification scope is described in `docs/POC.md`.

- Original: <https://github.com/dspaudio/mc-1>
- Verification baseline: `10a2240d7f3c63e3ed551774a8e17fb24ed002b8`
- New sound generation path: this project's hybrid PCM/2op FM/1osc VA voices.
- Reused components: original waveform/pitch tables, synthesized drums, part mixer, four shared FX, and limiter.

The POC's short PCM consisted of original test signals generated mathematically at initialization. The revised product uses original mathematical material and the independently sourced CC0 attacks described below, never the original LA sound bank. The new melody path does not call the legacy LA renderer. The size of a host binary must not be reported as the size of target firmware with LA removed.

## Firmware foundation

MC-1's HAL, startup code, LCD transport, freestanding libc, and RAM update loader derive from the GPL-3.0-only foundation at `07e3fed30e8e6757dbe3c6810276599987e4970e` in the public `dspaudio/orbit` repository. Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments. The lineage is ORBIT → SLOOP → Felucca; ORBIT records SLOOP baseline `d691ba7b2d922f1a1f41a3622cffe29ce41c5506`.

The CPU1 startup, stack entry, and safe-stop foundation derives from SLOOP v2.7.1 at `4ec1ac829fd2bdf2c571ea215fb3d62477d01220`. Credits in those notices for Charles Vestal, Kerem Kilic, Leo Kuroshita, Hügelton Instruments, and Andre Rezemini are retained. MC-1's private job, transactional completion, quarantine, and memory-guard integration are dspaudio's code.

## Web foundation

The FM-1 faceplate, input/LED mapping, responsive layout, IndexedDB, and AudioWorklet foundation are GPL-3.0-only code derived from `533f6ca9a97110249577427752475fb3381fbf40` in `dspaudio/orbit-web-emu`. Its upstream foundation is `sabliran/sloop-web-emu` at `bbfd8e5b3bfc452520f61afb0d94e2fdfe93fd9d`. The adapter connecting the MC-1 C screen, sound engine, and storage, along with the product-site styles, is brought over from dspaudio's MC-1 implementation. Each source's original notices are retained.

## SDK package assets

The JieLi compiler and SDK checkout are external build dependencies and are not included in this repository. `uboot.boot`, `cfg_tool.bin`, and `cfg/eq_cfg_hw.bin`, read when creating `.fwsc`, are Apache-2.0 assets from the official AC79 SDK at `d179b4484759423312073f5fbb232501aa491047` (tag `AC79NN_SDK_V1.2.1_2023-12-13`). The license text is in `LICENSES/Apache-2.0-AC79.txt`; input hashes are preserved in the packaging tools and actual build reports. The bootloader head in the package is metadata; it does not authorize the RAM loader to write below flash `0x4000`.

`FM-1_900` and `FELUCCA-LOADER-1` are update protocol identifiers, not mc-1-mod versions or hardware verification results. Target compilation, offline loader tests, and web audio operation are distinct from verification of boot, real-time deadlines, and USB on a physical device.

## Sound sources and distribution

Three upright-piano attacks and one offline-combined trumpet, F horn and violin-section impact derive from VS Chamber Orchestra: Community Edition, `sgossner/VSCO-2-CE`, commit `440300901dfe9275fd84e0b7763af1f8443ae62e`. Its CC0 1.0 Universal text is preserved in `LICENSES/CC0-1.0-VSCO.txt`. Credits: Sam Gossner and Simon Dalzell; sample cutting by Elan Hickler / Soundemote. The upright piano was sampled by Simon Dalzell of Ivy Audio in Winter 2015-16; its source notice encourages credit to him and Versilian Studios LLC.

`data/pcm-source/vsco-attacks.json` contains the processed mono inputs, pinned original paths and SHA-256 values, official piano and SFZ key mappings, measured tuning, used frame counts, section gains/delays and processing. The ensemble includes an explicitly documented mathematical C4 foundation. These inputs are committed for independent offline reproduction; the generator does not download recordings. Runtime still reads a single PCM stream per voice. No audio from the user's reference videos is incorporated.

The legacy LA renderer and LA sample bank are not included in the new product sound path. Provenance, processing settings, and hashes for new PCM assets are recorded separately in the generated assets' manifest. Recording provenance from the existing MC-1 is not presented as provenance for new assets that do not use those recordings. Yamaha/Roland ROMs, samples, patches, and logos are not included.

Binary distributions are accompanied by the corresponding source for the sources used, the full GPL text, and applicable asset licenses and provenance. M-VAVE, FM-1, Yamaha, and Roland name target hardware or reference products and do not imply sponsorship, affiliation, or sound compatibility.
