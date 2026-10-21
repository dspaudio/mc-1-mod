# SPDX-License-Identifier: GPL-3.0-only
CC ?= cc
PYTHON ?= python3
OUT ?= build
CFLAGS ?= -O2 -g -std=c11 -Wall -Wextra -Werror -Wconversion -Wno-sign-conversion
CPPFLAGS += -Isrc
LDLIBS += -lm
CORE = src/effects.c src/demo.c src/engine.c src/factory.c src/project.c \
	src/store.c src/ui.c src/midi.c src/hybrid.c src/tables.c
HEADERS = src/mcfm.h src/parallel.h src/fixed.h src/midi.h src/presets.h \
	src/store.h src/ui.h src/tables.h src/hybrid.h src/hybrid_profiles.h \
	src/pcm_types.h src/pcm_bank.h $(wildcard src/pcm/*.h)

CORE_TESTS = core hybrid mixer hybrid-quality ui-input performance patterns output metronome song midi demo-songs \
	connection-routing storage ota-entry panel-modes env-graph encoder-input \
	pattern-title mixer-pan pcm-cache dualcore analog routing profiles native native-midi
INFRA_TESTS = connections connections-ota panel-leds panel-refresh perf-probe \
	perf-hardware stage-probe memory
PRIMITIVE_TESTS = fixed effects-modes pcm-bank
TESTS = $(CORE_TESTS) $(INFRA_TESTS) $(PRIMITIVE_TESTS)
CORE_BINS = $(addprefix $(OUT)/test-,$(CORE_TESTS))

.PHONY: all native target web wasm render auditions test test-primitives test-infrastructure \
	test-fx test-ota test-va-tables sanitize $(addprefix test-,$(TESTS))
all: native
native: $(OUT)/mc1mod-host
target:
	$(PYTHON) tools/build_target.py
web wasm:
	BUILD_DIR="$(OUT)" $(PYTHON) tools/build_web.py
$(OUT):
	mkdir -p $@
$(OUT)/mc1mod-host: tools/host.c $(CORE) $(HEADERS) | $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE) $< $(LDFLAGS) $(LDLIBS) -o $@
render: $(OUT)/mc1mod-host
	$(OUT)/mc1mod-host render $(OUT)/demo.wav 16
$(OUT)/mc1mod-auditions: tools/render.c tools/auditions.c $(CORE) $(HEADERS) | $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE) $< $(LDFLAGS) $(LDLIBS) -o $@
auditions: $(OUT)/mc1mod-auditions
	$(OUT)/mc1mod-auditions $(OUT)/auditions-audio $(OUT)/auditions.json

# 파일명이 다른 시험과 engine/adapter를 직접 포함하는 시험은 중복 링크하지 않는다.
$(CORE_BINS): TEST_CORE = $(CORE)
$(OUT)/test-core: TEST_SOURCE = tests/test.c
$(OUT)/test-hybrid: TEST_SOURCE = tests/test_hybrid.c
$(OUT)/test-mixer: TEST_SOURCE = tests/test_mixer.c
$(OUT)/test-hybrid-quality: TEST_SOURCE = tests/hybrid_quality.c
$(OUT)/test-ui-input: TEST_SOURCE = tests/ui_input.c
$(OUT)/test-connection-routing: TEST_SOURCE = tests/connection_routing.c
$(OUT)/test-ota-entry: TEST_SOURCE = tests/ota_entry_test.c
$(OUT)/test-panel-modes: TEST_SOURCE = tests/panel_modes.c
$(OUT)/test-env-graph: TEST_SOURCE = tests/env_graph.c
$(OUT)/test-encoder-input: TEST_SOURCE = tests/encoder_input.c
$(OUT)/test-pattern-title: TEST_SOURCE = tests/pattern_title.c
$(OUT)/test-mixer-pan: TEST_SOURCE = tests/mixer_pan.c
$(OUT)/test-demo-songs: TEST_SOURCE = tests/demo_songs.c
$(OUT)/test-pcm-cache: TEST_SOURCE = tests/pcm_cache.c
$(OUT)/test-native-midi: TEST_SOURCE = tests/native_midi.c
$(OUT)/test-routing $(OUT)/test-pcm-cache: TEST_CORE = $(filter-out src/engine.c,$(CORE))
$(OUT)/test-native $(OUT)/test-native-midi: src/mod1_wasm.c
$(OUT)/test-midi $(OUT)/test-core: tests/presets_012.h
$(OUT)/test-performance $(OUT)/test-dualcore: firmware/perf_fixture.h
$(OUT)/test-dualcore: TEST_CFLAGS += -pthread
$(OUT)/test-encoder-input: TEST_CFLAGS += -Wno-unused-function
.SECONDEXPANSION:
$(CORE_BINS): $(OUT)/test-%: $(CORE) $(HEADERS) $$(or $$(TEST_SOURCE),tests/$$*.c) | $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_CFLAGS) $(TEST_CORE) $(or $(TEST_SOURCE),tests/$*.c) $(LDFLAGS) $(LDLIBS) -o $@

$(OUT)/test-fixed: tests/fixed.c src/fixed.h | $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@
$(OUT)/test-effects-modes: tests/effects_modes.c src/effects.c src/fixed.h src/mcfm.h src/parallel.h | $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/effects.c $< $(LDFLAGS) $(LDLIBS) -o $@
$(OUT)/test-pcm-bank: tests/pcm_bank.c src/pcm_types.h src/pcm_bank.h $(wildcard src/pcm/*.h) | $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@

$(OUT)/test-connections $(OUT)/test-connections-ota: tests/connections.c firmware/update/usb.c firmware/connection/midi_uart.c firmware/hal/fm1_uart.h
$(OUT)/test-connections-ota: CPPFLAGS += -DFELUCCA_OTA=1
$(OUT)/test-panel-leds: tests/panel_leds.c firmware/panel_leds.h firmware/hal/fm1_input.h
$(OUT)/test-panel-refresh: tests/panel_refresh.c firmware/panel_refresh.c src/ui.h src/mcfm.h
$(OUT)/test-perf-probe: tests/perf_probe.c firmware/perf_probe.h firmware/update/ota.c
$(OUT)/test-perf-hardware: tests/perf_hardware.c firmware/perf_hardware.h
$(OUT)/test-stage-probe: tests/stage_probe.c firmware/stage_probe.h firmware/perf_probe.h $(HEADERS)
$(OUT)/test-memory: tests/memory.c firmware/libc.c
$(OUT)/test-memory: TEST_CFLAGS += -fno-builtin
$(addprefix $(OUT)/test-,$(INFRA_TESTS)): TEST_CFLAGS += -Wno-unused-function
$(addprefix $(OUT)/test-,$(INFRA_TESTS)): | $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_CFLAGS) $< $(LDFLAGS) -o $@

$(addprefix test-,$(TESTS)): test-%: $(OUT)/test-%
	$(OUT)/test-$*
test: $(addprefix test-,$(TESTS))
test-primitives: $(addprefix test-,$(PRIMITIVE_TESTS))
test-infrastructure: $(addprefix test-,$(INFRA_TESTS))
test-fx: test-effects-modes
test-va-tables:
	$(PYTHON) tools/gen_va.py --check
$(OUT)/test-ota: tests/ota_test.c firmware/update/ota.c | $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wno-unused-function $< $(LDFLAGS) -o $@
$(OUT)/ldr-test: tests/ldr_test.c firmware/loader/ldr_core.c firmware/update/ota.c | $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wno-unused-function $< $(LDFLAGS) -o $@
test-ota: $(OUT)/test-ota $(OUT)/ldr-test
	$(OUT)/test-ota $(OUT)/mc-1-EXPERIMENTAL.fwsc
	$(OUT)/ldr-test $(OUT)/previous-test-only.fwsc $(OUT)/mc-1-EXPERIMENTAL.fwsc

sanitize:
	$(MAKE) OUT=$(OUT)/sanitize CFLAGS="-O1 -g -std=c11 -Wall -Wextra -Werror -Wconversion -Wno-sign-conversion -fsanitize=address,undefined -fno-omit-frame-pointer" test render
