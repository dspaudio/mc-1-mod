#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""과거 장치 집계를 보정점으로 사용하는 비보증 조건부 부하 모델."""

import argparse
from dataclasses import asdict, dataclass, replace
from fractions import Fraction as F
import hashlib
import json
import math
from pathlib import Path
import sys
from typing import Final, Literal, TypedDict


ROOT: Final = Path(__file__).resolve().parents[1]
UPSTREAM: Final = ROOT.parent / "mc-1" / "mcfm-1"
APP: Final = "9ebcd27952284c2298f4f8f51ac490b3f49cf012aa64ab15fe899d344ef51256"
SOURCE: Final = "54122b1698f1880d7b5895890e65ab7d87c57276dcbcc6f0747fd1f955081556"
COMMIT: Final = "10a2240d7f3c63e3ed551774a8e17fb24ed002b8"
STAGES: Final = (
    "synth", "route", "rd1", "cf1", "wait", "transfer", "output",
    "rd2", "cf2", "non_render", "stage_audio",
)
STABLE: Final = ((29, 30), (31, 32), (33, 34), (35, 36), (39, 40))
DEADLINE: Final = 69648
PHYSICAL: Final = F(24000000 * 128, 44100)
LABEL: Final = "과거 이미지 실측 기반 조건부 예측; 측정 성능 판정·MCU 적합 보증 아님"
LIMITATIONS: Final = (
    "현재 hybrid의 PCM/FM/VA·Analog·64bit 나눗셈 비용은 측정되지 않았다. "
    "곱셈·나눗셈 호출 횟수나 ISA 사이클을 계수한 모델도 아니다.",
    "CPU1 합성 시간·코어 점유율·슬라이스 분포는 export되지 않았다. "
    "균등 8×16 스케줄에서 wait를 맞춘 잠재값과 그 분할은 식별된 비용이 아니다.",
    "평균 재현은 같은 집계에 대한 대수적 적합이다. 독립 검증·새 이미지 예측 정확도 증명이 아니다. "
    "단계별 최대값의 동시성·블록 분포·p99는 복원할 수 없다.",
    "XIP/cache 경합·IRQ 진입 지연·락/flash 정지·타이머 실제 편차는 미지다. "
    "시나리오 범위는 가능한 전 범위를 덮는 상한/하한이 아니다.",
    "실제 render_prepare는 제어/step/demo/arp 경계에서 16보다 짧게 자른다. "
    "8×16은 지정된 가상 스케줄이며 실제 모든 블록의 슬라이스 수가 아니다.",
    "app/source 해시는 원본 문서의 역사 메타데이터 일치다. "
    "과거 바이너리 재해시나 현재 장치 실행을 검증하지 않았다.",
)


def require(ok: bool, label: str) -> None:
    if not ok:
        raise RuntimeError("보정 불가: " + label)


def u64(words: tuple[int, ...], index: int) -> int:
    return words[index] | (words[index + 1] << 32)


@dataclass(frozen=True, slots=True)
class Capture:
    index: int
    name: str
    workload: str
    words: tuple[int, ...]
    stage_words: tuple[int, ...]

    def mean(self, index: int) -> F:
        return F(u64(self.words, index), self.words[2])

    def stage(self, index: int) -> F:
        return F(u64(self.stage_words, 4 + 3 * index), self.stage_words[2])

    def drift(self) -> tuple[tuple[int, int], ...]:
        return tuple((a, b) for a, b in STABLE if self.words[a] != self.words[b])


@dataclass(frozen=True, slots=True)
class Costs:
    prep: F
    local: F
    worker: F
    drums: F
    route: F
    transfer: F
    output: F
    banks: tuple[F, ...]
    render_residual: F
    outside_render: F
    observed_wait: F


@dataclass(frozen=True, slots=True)
class Scenario:
    name: str
    mask: int = 7
    melodic_multiplier: F = F(1)
    worker_multiplier: F = F(1)
    drum_multiplier: F = F(1)
    fx_multiplier: F = F(1)
    copy_multiplier: F = F(1)
    arithmetic_multiplier: F = F(1)
    xip_ticks: F = F(0)
    irq_ticks: F = F(0)
    serial: bool = False
    prep_fraction: F = F(15, 100)
    worker_share: F = F(3, 5)
    # 총 melodic 비용의 55%는 공통, 45%는 세 소스에 균등 분할하는 가정이다.
    source_fraction: F = F(45, 100)
    cpu0_multiplier: F = F(1)
    worker_combined_multiplier: F = F(1)
    route_multiplier: F = F(1)
    output_multiplier: F = F(1)


@dataclass(frozen=True, slots=True)
class Round:
    iteration: int
    next_slice: int | None
    previous_fx_slice: int | None
    prepare: tuple[F, F]
    copy_in: tuple[F, F]
    cpu0_local: tuple[F, F]
    cpu1_fx: tuple[F, F]
    cpu1_synth: tuple[F, F]
    join_wait: tuple[F, F]
    copy_out: tuple[F, F]
    output_previous: tuple[F, F]
    route_next: tuple[F, F]


@dataclass(frozen=True, slots=True)
class Prediction:
    render_ticks: F
    audio_ticks: F
    wait_ticks: F
    overlap_ticks: F
    timeline: tuple[Round, ...]


def fit(c: Capture, scenario: Scenario) -> Costs:
    """균등 slice·prep 분할 가정 아래 관측 평균 wait를 맞추는 잠재 해."""
    synth = c.stage(0)
    prep = synth * scenario.prep_fraction
    local = synth - prep
    banks = tuple(c.stage(i) for i in (2, 3, 7, 8))
    fx = sum(banks, F(0))
    wait = c.stage(4)
    # 8개 synth, 8개 FX, 9회 submit/join. 마지막 FX는 CPU0 local과 겹치지 않는다.
    # wait = max(W-L,0) + 7*max(W+F/8-L,0) + F/8.
    divisor = 8 if wait >= fx else 7
    worker_per_slice = local / 8 + (wait - fx) / divisor
    require(wait >= fx / 8 and worker_per_slice >= 0,
            f"capture {c.index}: 균등 timeline에서 wait를 재현할 수 없음")
    latent_synth = worker_per_slice * 8
    path = synth + c.stage(1) + wait + c.stage(5) + c.stage(6)
    return Costs(
        prep, local, latent_synth * scenario.worker_share,
        latent_synth * (1 - scenario.worker_share), c.stage(1), c.stage(5),
        c.stage(6), banks, c.mean(5) - path, c.mean(8) - c.mean(5), wait,
    )


def simulate(cost: Costs, scenario: Scenario) -> Prediction:
    """실제 코드의 prepare → submit → local → join → output → route 순서."""
    active = scenario.mask.bit_count()
    source_scale = 1 - scenario.source_fraction + scenario.source_fraction * F(active, 3)
    melodic = source_scale * scenario.melodic_multiplier
    # arithmetic의 영향 비중 20% 역시 측정값이 아닌 sensitivity 가정이다.
    arithmetic = 1 + F(1, 5) * (scenario.arithmetic_multiplier - 1)
    local = cost.local * melodic * arithmetic * scenario.cpu0_multiplier / 8
    worker = (cost.worker * melodic * arithmetic * scenario.worker_multiplier
              * scenario.worker_combined_multiplier / 8)
    drums = (cost.drums * scenario.drum_multiplier * scenario.worker_multiplier
             * scenario.worker_combined_multiplier / 8)
    fx = sum(cost.banks, F(0)) * scenario.fx_multiplier / 8
    prep = cost.prep * scenario.cpu0_multiplier / 8
    route = cost.route * scenario.route_multiplier / 8
    output = cost.output * scenario.output_multiplier / 8
    copy = cost.transfer * scenario.copy_multiplier / 18
    clock, total_wait, overlap = F(0), F(0), F(0)
    rounds = []
    for i in range(9):
        has_next, has_previous = i < 8, i > 0
        begin = clock
        clock += prep if has_next else 0
        prepare = (begin, clock)
        begin = clock
        clock += copy
        copy_in = (begin, clock)
        dispatch = clock
        local_end = dispatch + (local if has_next else 0)
        bank_end = dispatch + (fx if has_previous else 0)
        worker_end = bank_end + (worker + drums if has_next else 0)
        if scenario.serial:
            # READY에서 중간 실패한 비용은 제외한 dual 비활성 정상 serial fallback.
            # local 8 voices+drums 뒤에 FX 실행. 순서는 총합에 영향을 주지 않는다.
            clock = local_end + (worker + drums if has_next else 0) + (fx if has_previous else 0)
            cpu_fx = (local_end + (worker + drums if has_next else 0), clock)
            cpu_synth = (local_end, cpu_fx[0])
            joined = (clock, clock)
        else:
            clock = max(local_end, worker_end)
            total_wait += clock - local_end
            overlap += min(local_end - dispatch, worker_end - dispatch)
            cpu_fx, cpu_synth = (dispatch, bank_end), (bank_end, worker_end)
            joined = (local_end, clock)
        begin = clock
        clock += copy
        copy_out = (begin, clock)
        begin = clock
        clock += output if has_previous else 0
        out = (begin, clock)
        begin = clock
        clock += route if has_next else 0
        rounds.append(Round(
            i, i if has_next else None, i - 1 if has_previous else None,
            prepare, copy_in, (dispatch, local_end), cpu_fx, cpu_synth,
            joined, copy_out, out, (begin, clock),
        ))
    render = clock + cost.render_residual
    return Prediction(
        render, render + cost.outside_render + scenario.xip_ticks + scenario.irq_ticks,
        total_wait, overlap, tuple(rounds),
    )


def check_timeline(pred: Prediction, serial: bool) -> None:
    rows = pred.timeline
    require(len(rows) == 9 and rows[0].previous_fx_slice is None
            and rows[-1].next_slice is None, "prologue/epilogue")
    require(rows[0].cpu1_fx[0] == rows[0].cpu1_fx[1], "prologue FX 없음")
    require(rows[-1].cpu1_synth[0] == rows[-1].cpu1_synth[1], "epilogue synth 없음")
    for i, row in enumerate(rows):
        require(all(a <= b for a, b in (
            row.prepare, row.copy_in, row.cpu0_local, row.cpu1_fx,
            row.cpu1_synth, row.join_wait, row.copy_out,
            row.output_previous, row.route_next,
        )), "시간 역전")
        require(row.copy_out[0] >= max(row.cpu1_fx[1], row.cpu1_synth[1]),
                "미완료 worker copy-out 금지")
        if i:
            require(row.prepare[0] == rows[i - 1].route_next[1], "round 의존성")
        if not serial:
            require(row.cpu1_synth[0] == row.cpu1_fx[1], "CPU1 FX 다음 synth")
            require(row.join_wait[1] == max(row.cpu0_local[1], row.cpu1_synth[1]),
                    "join max; CPU1 bank 시간 중복 가산 금지")
    require(serial or pred.overlap_ticks > 0, "dual overlap")


def source_fingerprint() -> tuple[str, dict[str, str]]:
    """Use exactly the current build_target.py source-input convention."""
    paths = [
        p for folder in ("src", "firmware") for p in (ROOT / folder).rglob("*")
        if p.is_file() and p.suffix in (".c", ".h", ".S", ".ld")
    ]
    paths += [ROOT / p for p in (
        "tools/build_target.py", "tools/gen_pcm.py", "tools/gen_va.py",
        "tools/target_abi.c", "data/pcm-manifest.json",
        "data/pcm-source/vsco-attacks.json", "data/pcm-source/generaluser-strings.json",
    )]
    digest = hashlib.sha256()
    inputs = {}
    for path in sorted(paths):
        relative = path.relative_to(ROOT).as_posix()
        data = path.read_bytes()
        digest.update(relative.encode() + b"\0" + data + b"\0")
        inputs["current/" + relative] = hashlib.sha256(data).hexdigest()
    return digest.hexdigest(), inputs


HOST_STAGES: Final = ("cpu0_synth", "worker_combined", "transfer", "route", "output")
PREP_SHARES: Final = (F(1, 20), F(15, 100), F(3, 10))


@dataclass(frozen=True, slots=True)
class StageSamples:
    cpu0_synth: tuple[F, ...]
    worker_combined: tuple[F, ...]
    transfer: tuple[F, ...]
    route: tuple[F, ...]
    output: tuple[F, ...]

    def medians(self) -> dict[str, F]:
        return {key: sorted(values)[1] for key, values in asdict(self).items()}


@dataclass(frozen=True, slots=True)
class MeasuredRow:
    profile: int
    name: str
    phase: Literal["attack", "held"]
    analog: int
    register: Literal["middle", "high"]
    source_mask: int
    aux_enabled: bool
    ratios: StageSamples
    host_ns: StageSamples
    baseline_ns: StageSamples
    activity_json: str


@dataclass(frozen=True, slots=True)
class MeasuredInput:
    source_input_sha256: str
    original_commit: str
    compiler: str
    original_source_proxy: bool
    rows: tuple[MeasuredRow, ...]


class MeasuredMetrics(TypedDict):
    predicted_mean_ticks: F
    predicted_mean_ms: F
    predicted_mean_budget_percent: F
    conditional_mean_margin_ticks: F
    delta_from_anchor_ticks: F
    max_mean_shaped_sensitivity_ticks: F
    max_mean_shaped_sensitivity_ms: F
    max_mean_shaped_budget_percent: F
    render_ticks: F
    wait_ticks: F
    overlap_ticks: F


def parse_measured(data: bytes, source_hash: str) -> MeasuredInput:
    """Parse schema 2 once; reject stale bindings and inconsistent triples."""
    raw = json.loads(data, parse_constant=lambda token: require(False, f"Nonfinite JSON: {token}"))
    require(isinstance(raw, dict), "Measured input must be an object")
    require({"schema", "source_input_sha256", "original_commit", "compiler", "rows"} <= raw.keys(),
            "Missing measured input fields")
    require(type(raw["schema"]) is int and raw["schema"] == 2, "Measured schema must be 2")
    require(raw["source_input_sha256"] == source_hash, "Measured source fingerprint is stale")
    require(raw["original_commit"] == COMMIT, "Measured original_commit mismatch")
    require(isinstance(raw["compiler"], str) and bool(raw["compiler"].strip()), "Missing compiler")
    proxy = raw.get("original_source_proxy", True)
    require(type(proxy) is bool, "original_source_proxy must be boolean")
    require(isinstance(raw["rows"], list) and bool(raw["rows"]), "Measured rows must be nonempty")
    rows = []
    keys = {"profile", "name", "phase", "analog", "register", "source_mask", "aux_enabled",
            "ratios", "host_ns", "baseline_ns", "activity"}
    for index, item in enumerate(raw["rows"]):
        require(isinstance(item, dict) and keys <= item.keys(), f"Missing row fields at {index}")
        for key, maximum in (("profile", 63), ("analog", 127), ("source_mask", 7)):
            require(type(item[key]) is int and 0 <= item[key] <= maximum, f"Invalid {key} at {index}")
        require(isinstance(item["name"], str) and bool(item["name"].strip()), f"Missing name at {index}")
        require(item["phase"] in ("attack", "held"), f"Invalid phase at {index}")
        require(item["register"] in ("middle", "high"), f"Invalid register at {index}")
        require(type(item["aux_enabled"]) is bool, f"Invalid aux_enabled at {index}")
        require(isinstance(item["activity"], dict), f"Invalid activity at {index}")
        stages = {}
        for group in ("ratios", "host_ns", "baseline_ns"):
            values = item[group]
            require(isinstance(values, dict) and set(values) == set(HOST_STAGES),
                    f"Invalid {group} stage keys at {index}")
            parsed = {}
            for key in HOST_STAGES:
                triple = values[key]
                require(isinstance(triple, list) and len(triple) == 3,
                        f"{group}.{key} needs three repeats at {index}")
                require(all(type(n) in (int, float) and math.isfinite(n) and n > 0 for n in triple),
                        f"{group}.{key} must be positive finite numbers at {index}")
                parsed[key] = tuple(F(str(n)) for n in triple)
            stages[group] = StageSamples(**parsed)
        for key in HOST_STAGES:
            for ratio, host, baseline in zip(item["ratios"][key], item["host_ns"][key],
                                             item["baseline_ns"][key]):
                expected = F(str(host)) / F(str(baseline))
                require(abs(F(str(ratio)) - expected) <= max(F(1, 10**9), expected / 10**6),
                        f"Inconsistent {key} ratio/ns at {index}")
        rows.append(MeasuredRow(
            item["profile"], item["name"], item["phase"], item["analog"], item["register"],
            item["source_mask"], item["aux_enabled"], stages["ratios"], stages["host_ns"],
            stages["baseline_ns"], json.dumps(item["activity"], sort_keys=True),
        ))
    return MeasuredInput(source_hash, COMMIT, raw["compiler"], proxy, tuple(rows))


def measured_scenario(row: MeasuredRow, portability: F = F(1),
                      p_old: F = F(15, 100)) -> Scenario:
    ratios = row.ratios.medians()
    # Ratios already include source gating; do not apply legacy mask scaling again.
    return Scenario(
        "measured-stage-ratios", prep_fraction=p_old, source_fraction=F(0),
        cpu0_multiplier=1 + portability * (ratios["cpu0_synth"] - 1),
        worker_combined_multiplier=1 + portability * (ratios["worker_combined"] - 1),
        copy_multiplier=ratios["transfer"], output_multiplier=ratios["output"],
    )


def measured_metrics(capture: Capture, scenario: Scenario,
                     p_new: F = F(15, 100)) -> MeasuredMetrics:
    # Fit the historical worker using p_old, then split the scaled CPU0 aggregate using p_new.
    cost = fit(capture, scenario)
    cost = replace(cost, prep=capture.stage(0) * p_new, local=capture.stage(0) * (1 - p_new))
    prediction = simulate(cost, scenario)
    check_timeline(prediction, False)
    mean = prediction.audio_ticks
    shaped = mean * capture.words[10] / capture.mean(8)
    return {
        "predicted_mean_ticks": mean, "predicted_mean_ms": mean / 24000,
        "predicted_mean_budget_percent": 100 * mean / DEADLINE,
        "conditional_mean_margin_ticks": DEADLINE - mean,
        "delta_from_anchor_ticks": mean - capture.mean(8),
        "max_mean_shaped_sensitivity_ticks": shaped,
        "max_mean_shaped_sensitivity_ms": shaped / 24000,
        "max_mean_shaped_budget_percent": 100 * shaped / DEADLINE,
        "render_ticks": prediction.render_ticks, "wait_ticks": prediction.wait_ticks,
        "overlap_ticks": prediction.overlap_ticks,
    }


def measured_report(measured: MeasuredInput, captures: list[Capture],
                    inputs: dict[str, str]) -> tuple[str, str]:
    """Return a compact report; no IO, measurements, or global ISR scaling."""
    ones = StageSamples(*((F(1),) * 3 for _ in HOST_STAGES))
    for index in (2, 5, 6, 7):
        for share in PREP_SHARES:
            unit = measured_scenario(replace(measured.rows[0], ratios=ones), p_old=share)
            result = measured_metrics(captures[index], unit, share)
            require(result["predicted_mean_ticks"] == captures[index].mean(8)
                    and result["render_ticks"] == captures[index].mean(5)
                    and result["wait_ticks"] == captures[index].stage(4),
                    f"Unit-ratio exact reproduction at capture {index}, prep {share}")

    rows = []
    for index, row in enumerate(measured.rows):
        scenario = measured_scenario(row)
        record = asdict(row)
        record["activity"] = json.loads(record.pop("activity_json"))
        rows.append({
            "row_index": index, "measurement": record, "ratios_median": row.ratios.medians(),
            "primary": measured_metrics(captures[2], scenario),
            "night_ratio_portability_sensitivity": [
                {"capture_index": i, **measured_metrics(captures[i], scenario)} for i in (5, 6, 7)
            ],
        })
    top = sorted(range(len(rows)),
                 key=lambda i: (-rows[i]["primary"]["predicted_mean_ticks"], i))[:5]
    envelopes = []
    for index in top:
        row = measured.rows[index]
        primary = measured_scenario(row)
        families = {
            "independent_prep_shares": [
                ({"p_old": old, "p_new": new}, replace(primary, prep_fraction=old), new)
                for old in PREP_SHARES for new in PREP_SHARES
            ],
            "synthesis_delta_portability": [
                ({"factor": factor}, measured_scenario(row, factor), F(15, 100))
                for factor in (F(3, 4), F(1), F(5, 4))
            ],
            "copy_fixed_vs_measured": [
                ({"copy_multiplier": factor}, replace(primary, copy_multiplier=factor), F(15, 100))
                for factor in (F(1), primary.copy_multiplier)
            ],
            "route_anchor_vs_measured_check": [
                ({"route_multiplier": factor}, replace(primary, route_multiplier=factor), F(15, 100))
                for factor in (F(1), row.ratios.medians()["route"])
            ],
            "output_anchor_vs_measured_check": [
                ({"output_multiplier": factor}, replace(primary, output_multiplier=factor), F(15, 100))
                for factor in (F(1), primary.output_multiplier)
            ],
            "incremental_xip": [
                ({"xip_ticks": ticks}, replace(primary, xip_ticks=ticks), F(15, 100))
                for ticks in (F(0), F(256), F(1024), F(4096))
            ],
            "incremental_irq": [
                ({"irq_ticks": ticks}, replace(primary, irq_ticks=ticks), F(15, 100))
                for ticks in (F(0), F(256), F(1024), F(4096))
            ],
        }
        for name, variants in families.items():
            results, excluded = [], []
            for parameters, scenario, p_new in variants:
                if scenario.cpu0_multiplier <= 0 or scenario.worker_combined_multiplier <= 0:
                    excluded.append(parameters)
                    continue
                results.append(measured_metrics(captures[2], scenario, p_new))
            envelopes.append({
                "row_index": index, "family": name,
                "parameters": [parameters for parameters, _, _ in variants],
                "excluded_nonpositive_synthesis": excluded,
                "envelope": {
                    key: {"min": min(result[key] for result in results),
                          "max": max(result[key] for result in results)}
                    for key in (
                        "predicted_mean_ticks", "predicted_mean_ms", "predicted_mean_budget_percent",
                        "max_mean_shaped_sensitivity_ticks", "max_mean_shaped_sensitivity_ms",
                        "max_mean_shaped_budget_percent", "wait_ticks",
                    )
                },
            })
    assumptions = {
        "primary": "acid0 index2 single point; p_old=p_new=0.15; medians of three paired host "
                   "ratios scale CPU0 prep+local, latent worker combined, transfer, and output. "
                   "Route and each FX bank retain historical costs.",
        "worker_combined": "Synth-enabled jobs only: job_ticks minus bank ticks, including "
                           "job wrapper residue; not pure DSP. FX-only epilogue coordination "
                           "is excluded by the runner and remains unmeasured target overhead.",
        "parallelism": "Uniform 8x16 frames, nine submit/join rounds. CPU1 previous FX then "
                       "current synth; waits are recomputed by max/join, never held fixed.",
        "prep": "p_old fits the historical latent worker; independently p_new redistributes "
                "the current CPU0 aggregate. Nine share pairs are sensitivity assumptions.",
        "portability": "1+factor*(host_ratio-1), factors 0.75/1/1.25, on CPU0 and combined "
                       "worker only. Assumptions, not learned intervals. Nonpositive variants "
                       "are excluded explicitly, not clipped.",
        "common_work": "Output uses its measured ratio because limiter branches can differ. "
                       "Route=1 and FX=1 assume matching sends/types/slices/state. Measured "
                       "route checks and output-anchor checks are separate top-row sensitivities.",
        "copy": "Measured transfer ratio is primary, fixed1 is sensitivity. Runner subtracts "
                "the complete synchronous notify span; copy/coordination are not identified.",
        "sources": "Input masks, auxiliary state, and activity are metadata, not extra "
                   "multipliers. Measured ratios already include source gating and lifetime.",
        "host": "Synchronous notify measures real worker kernels without host worker-thread "
                "scheduling. It does not reproduce MCU cache/XIP contention or overlap.",
        "residuals": "Historical render residual and perf audio-render remain fixed assumptions. "
                     "TIMER5, non_render, and bank time are not added again.",
        "shaped": "Historical max/mean times predicted mean is a separate shape-persistence "
                  "sensitivity, not a future maximum, p99, or confidence interval.",
        "budget": "Percent of the integer 69648-tick block boundary, not CPU utilization. "
                  "TIMER4 nominal 24MHz; CPU Hz unknown; LRCLK not measured.",
        "night": "Captures 5/6/7 are separate cross-workload ratio-portability sensitivities, "
                 "not matched candidates, pooled regression evidence, or validation of acid0.",
        "envelopes": "Only the five largest primary row means get one-family-at-a-time "
                     "envelopes; no Cartesian stress sweep. XIP and IRQ increments are "
                     "independent examples 0/256/1024/4096 ticks, not bounds.",
        "provenance": "original_source_proxy=false is the runner's historical-source assertion, "
                      "not a git or binary verification by this model. Omission means proxy.",
    }
    report = {
        "schema": 2, "mode": "measured-stage-ratios", "new_firmware_device_evidence": False,
        "binding": {
            "source_input_sha256": measured.source_input_sha256, "original_commit": measured.original_commit,
            "original_source_proxy": measured.original_source_proxy, "compiler": measured.compiler,
            "historical_app_sha256": APP, "historical_source_input_sha256": SOURCE,
            "input_sha256": inputs,
        },
        "timer": {"nominal_timer4_hz": 24000000, "cpu_hz": None,
                  "deadline_ticks_integer": DEADLINE, "deadline_ticks_physical": PHYSICAL},
        "references": {
            "primary_capture_index": 2, "primary_inference": "Single-point acid0 calibration",
            "night_capture_indices": [5, 6, 7], "excluded_drift_indices": [1, 9, 10],
            "raw_captures": len(captures), "raw_blocks": sum(c.words[2] for c in captures),
            "anchors": [
                {"capture_index": i, "audio_mean_ticks": captures[i].mean(8),
                 "audio_max_ticks": captures[i].words[10],
                 "stage_mean_ticks": {name: captures[i].stage(k) for k, name in enumerate(STAGES)}}
                for i in (2, 5, 6, 7)
            ],
        },
        "assumptions": assumptions, "rows": rows, "top_row_indices": top, "envelopes": envelopes,
        "checks": {"raw_manual_drift_overlap": "PASS", "unit_ratio_exact_reproduction": True},
    }
    text = json.dumps(report, ensure_ascii=False, indent=2,
                      default=lambda value: float(value) if isinstance(value, F) else asdict(value),
                      allow_nan=False) + "\n"
    lines = [
        "# Measured Stage-Ratio Conditional Load Model", "",
        "Historical device anchors plus host stage ratios; NOT current device measurements.", "",
        f"- Current source-input: `{measured.source_input_sha256}`.",
        f"- Original commit: `{measured.original_commit}`; source proxy: "
        f"`{str(measured.original_source_proxy).lower()}` (runner assertion).",
        f"- Compiler: {measured.compiler}. Rows: {len(rows)}.",
        "- Primary: acid0 capture2, a single point. NIGHT5/6/7 remain separate sensitivities.",
        "- Budget percentages use 69648 TIMER4 ticks; they are not CPU utilization.",
        "- Shape sensitivity is NOT a future maximum, p99, or guaranteed upper bound.", "",
        "## Per-row Primary Predictions", "",
        "| row | profile | name | phase | Analog | register | mask | aux | mean ticks | ms | budget % | shaped ticks | shaped budget % | wait ticks | route ratio check |",
        "|---:|---:|---|---|---:|---|---:|---|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for record, row in zip(rows, measured.rows):
        metrics = record["primary"]
        name = row.name.replace("|", "\\|").replace("\n", " ").replace("\r", " ")
        lines.append(
            f"| {record['row_index']} | {row.profile} | {name} | {row.phase} | {row.analog} | "
            f"{row.register} | {row.source_mask} | {str(row.aux_enabled).lower()} | "
            f"{float(metrics['predicted_mean_ticks']):.3f} | {float(metrics['predicted_mean_ms']):.6f} | "
            f"{float(metrics['predicted_mean_budget_percent']):.3f} | "
            f"{float(metrics['max_mean_shaped_sensitivity_ticks']):.3f} | "
            f"{float(metrics['max_mean_shaped_budget_percent']):.3f} | "
            f"{float(metrics['wait_ticks']):.3f} | {float(row.ratios.medians()['route']):.6f} |"
        )
    lines += ["", "## Top-row One-family Sensitivity Envelopes", "",
              "Conditional ranges, not measured intervals. Other primary assumptions stay fixed.", "",
              "| row | family | mean ticks | shaped ticks | excluded nonpositive variants |",
              "|---:|---|---:|---:|---:|"]
    for envelope in envelopes:
        mean = envelope["envelope"]["predicted_mean_ticks"]
        shaped = envelope["envelope"]["max_mean_shaped_sensitivity_ticks"]
        lines.append(
            f"| {envelope['row_index']} | {envelope['family']} | "
            f"{float(mean['min']):.3f}..{float(mean['max']):.3f} | "
            f"{float(shaped['min']):.3f}..{float(shaped['max']):.3f} | "
            f"{len(envelope['excluded_nonpositive_synthesis'])} |"
        )
    lines += ["", "## NIGHT Ratio-portability Sensitivity", "",
              "Different workload, not a matched comparison or regression fit.", "",
              "| row | separate capture5/6/7 mean ticks |", "|---:|---|"]
    for record in rows:
        values = record["night_ratio_portability_sensitivity"]
        means = " / ".join(f"{float(item['predicted_mean_ticks']):.3f}" for item in values)
        lines.append(f"| {record['row_index']} | {means} |")
    lines += ["", "## Assumptions and Limits", ""]
    lines += [f"- **{key}**: {value}" for key, value in assumptions.items()]
    lines += ["", "## Deterministic Checks", "",
              "Raw/manual/drift/overlap PASS; unit stage ratios exactly reproduce each historical "
              "anchor at equal old/new prep shares 0.05/0.15/0.30. "
              "Exact reproduction is algebraic, not held-out device validation.", ""]
    return text, "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Conditional historical load calibration.")
    parser.add_argument("--measured", type=Path, help="Schema-2 measured host stage ratios.")
    args = parser.parse_args(argv)
    # 입력 manifest는 파일 내용 해시다. mtime·호스트 실행 시간은 보고서에 넣지 않는다.
    inputs = {}

    def read_input(root: Path, relative: str) -> bytes:
        data = (root / relative).read_bytes()
        prefix = "current/" if root == ROOT else "original/"
        inputs[prefix + relative] = hashlib.sha256(data).hexdigest()
        return data

    baseline = json.loads(read_input(ROOT, "data/baseline-mc1-1.2.0.json"))
    original = json.loads(read_input(UPSTREAM, "docs/la-pcm-hardware.json"))
    for p in ("src/engine.c", "src/parallel.h", "src/mcfm.h", "src/hybrid.c",
              "src/hybrid_profiles.h", "firmware/perf_fixture.h",
              "tools/load_model.py", "docs/LOAD_MODEL.md"):
        read_input(ROOT, p)
    for p in ("firmware/perf_probe.h", "firmware/stage_probe.h",
              "firmware/hal/fm1_time.h", "firmware/app.c", "tools/perf_capture.py",
              "src/parallel.h", "firmware/perf_fixture.h"):
        read_input(UPSTREAM, p)
    require(baseline["source_commit"] == COMMIT, "source commit")
    require(baseline["measured_app_sha256"] == original["app_sha256"] == APP, "app hash")
    require(baseline["measured_source_input_sha256"] == original["source_input_sha256"]
            == SOURCE, "historical source-input hash")
    require(original["package_sha256"]
            == "cb0b5454d3e459da11489f73818d4537093907aa13adee0230f3404227a701e4",
            "package hash")
    require((baseline["timer_hz"], baseline["device_cpu_hz"], baseline["sample_rate"],
             baseline["frames_per_block"], baseline["stage_schema"])
            == (24000000, None, 44100, 128, 5), "시간축/스키마")
    require(tuple(baseline["stage_names"]) == STAGES, "stage 순서")
    require(len(baseline["captures"]) == len(original["captures"]) == 11, "11 captures")
    captures = []
    decoded = []
    for i, (b, o) in enumerate(zip(baseline["captures"], original["captures"])):
        w, s = tuple(b["perf_words"]), tuple(b["stage_words"])
        require(w == tuple(o["words"]) and s == tuple(o["stage_words"]), f"raw 동일 {i}")
        require(b["capture"] == o["capture"] and b["workload"] == o["workload"],
                f"workload 동일 {i}")
        require(len(w) == len(s) == 42
                and all(type(n) is int and 0 <= n <= 0xffffffff for n in w + s), "u32[42]")
        require(w[0] == 1 and s[0] == 5 and s[1] == w[1] and w[1] != 0
                and w[2] == s[2] == 8192 and w[3] == 1048576 and s[3] == 0, "schema/window")
        require((w[24] & 255) == 9 and w[25] == 0xffffffff, "TIMER4 설정")
        require(all(w[k] == 0 for k in (12, 13, 21, 22, 23, 41)), "과거 오류 카운터")
        require(s[37] == 1 and s[39:42] == (0, 3524, 1), "worker 상태")
        c = Capture(i, b["capture"], b["workload"], w, s)
        require(bool(c.drift()) == b["volume_changed"] == o["volume_changed"], "drift 표시")
        require(w[10] == b["max_audio_ticks"] == o["max_audio_ticks"], "raw maximum")
        for total, peak in ((u64(w, 5), w[7]), (u64(w, 8), w[10])):
            require(0 < total <= w[4] and peak <= total <= peak * 8192, "sum/max 관계")
        for k in range(11):
            require(0 <= c.stage(k) <= s[6 + 3 * k], "stage mean/max 관계")
        captures.append(c)
        decoded.append({
            "index": i, "capture": c.name, "workload": c.workload,
            "perf_words": w, "stage_words": s, "drift_pairs": c.drift(),
            "matched_eligible": not c.drift(), "render_mean_ticks": c.mean(5),
            "audio_mean_ticks": c.mean(8), "timer5_mean_ticks": c.mean(14),
            "render_max_ticks": w[7], "audio_max_ticks": w[10],
            "interval_max_ticks": w[11], "interval_mean_proxy_ticks": F(w[4], 8192),
            "observed_interval_excess_ticks": w[11] - F(w[4], 8192),
            "interval_note": "elapsed/8192는 창 경과 proxy; 정확한 진입 간격 평균·IRQ bound 아님",
            "headroom_ticks": DEADLINE - w[10],
            "stages": {name: {"mean_ticks": c.stage(k), "max_ticks": s[6 + 3 * k]}
                       for k, name in enumerate(STAGES)},
        })
    require([c.index for c in captures if c.drift()] == [1, 9, 10], "drift 1/9/10")
    require(len({c.words[1] for c in captures}) == 11, "독립 session")
    repeat = captures[5:8]
    require(len({(c.workload, c.words[24:41]) for c in repeat}) == 1, "matched 5/6/7")
    # 0은 TIMER4 CON 전체 tuple이 다르다. workload 이름만 보고 반복군에 합치지 않는다.
    require(captures[0].words[24:41] != repeat[0].words[24:41], "idx0 교차확인만")
    c5 = captures[5]
    require(c5.words[4] == 570398577 and c5.words[10:12] == (69184, 69879), "raw idx5")
    require(c5.mean(5) == F(357413655, 8192)
            and c5.mean(8) == F(364584342, 8192), "정확 mean 손계산")
    peak_counterexample = sum(c5.stage_words[6 + 3 * k] for k in range(9))
    require(peak_counterexample == 84686 and peak_counterexample - c5.words[10] == 15502,
            "서로 다른 stage maxima 합 반례")
    require(DEADLINE == 2902 * 24 and DEADLINE - c5.words[10] == 464
            and PHYSICAL == F(10240000, 147), "deadline 손계산")
    require(max(c.words[10] for c in captures) == 69184
            and sum(c.words[2] for c in captures) == 90112, "전체 raw 앵커")

    target = None
    copy_bytes = None
    measured = None
    if args.measured or (ROOT / "build/target-report.json").is_file():
        actual_source_hash, source_inputs = source_fingerprint()
        inputs.update(source_inputs)
    if args.measured:
        measured_bytes = args.measured.read_bytes()
        inputs["measured_input"] = hashlib.sha256(measured_bytes).hexdigest()
        measured = parse_measured(measured_bytes, actual_source_hash)
    if (ROOT / "build/target-report.json").is_file():
        target = json.loads(read_input(ROOT, "build/target-report.json"))
        abi = target.get("target_abi_bytes", {})
        if all(k in abi for k in ("McSynthJob", "McFxJob")):
            copy_bytes = {
                "synth_submit_payload_bytes_per_block": 8 * abi["McSynthJob"],
                "fx_submit_payload_bytes_per_block": 8 * abi["McFxJob"],
                "fx_bus_return_bytes_per_block": 8 * 4 * 16 * 2 * 4,
                "note": "명시적 payload/bus만 계수; sparse FX history와 state commit 제외. "
                        "바이트 수는 전송 시간·대역폭 측정이 아니다.",
            }
        target = {
            "report": target, "actual_source_input_sha256": actual_source_hash,
            "source_input_matches": actual_source_hash == target["source_input_sha256"],
            "source_file_count": len(source_inputs), "device_evidence": False,
            "note": "기존 타깃 보고서; 이번에 빌드/바이너리 재해시하지 않음. 불일치 시 stale 표시.",
        }

    # 모든 clean 캡처를 개별 대수 재현한다. drift는 관측만 남기고 적합하지 않는다.
    fits = []
    for c in captures:
        if c.drift():
            continue
        scenario = Scenario("baseline-algebra")
        cost = fit(c, scenario)
        prediction = simulate(cost, scenario)
        check_timeline(prediction, False)
        require(prediction.render_ticks == c.mean(5) and prediction.audio_ticks == c.mean(8)
                and prediction.wait_ticks == c.stage(4), f"exact fit {c.index}")
        fits.append({
            "capture_index": c.index, "costs": asdict(cost),
            "predicted_audio_mean_ticks": prediction.audio_ticks,
            "residual_after_fit_ticks": prediction.audio_ticks - c.mean(8),
            "note": "관측 평균의 대수적 재현; 독립 정확도 검증 아님",
        })
    if measured is not None:
        text, markdown = measured_report(measured, captures, inputs)
        output = ROOT / "build/load64"
        output.mkdir(parents=True, exist_ok=True)
        (output / "model.json").write_text(text, encoding="utf-8")
        (output / "model.md").write_text(markdown, encoding="utf-8")
        print(f"PASS raw/manual/drift/overlap; measured rows={len(measured.rows)}; "
              "unit-ratio exact reproduction; firmware verdict=NONE")
        return 0
    scenarios = [Scenario("all-source-8voice-6drum-4FX")]
    for mask in range(8):
        scenarios.append(Scenario(f"source-gating-mask-{mask}", mask=mask))
    scenarios.append(Scenario("serial-fallback", serial=True))
    sweeps = (
        ("melodic_multiplier", (F(1, 2), F(5, 4), F(3, 2), F(2))),
        ("worker_multiplier", (F(1, 2), F(3, 2), F(2))),
        ("drum_multiplier", (F(1, 2), F(3, 2), F(2))),
        ("fx_multiplier", (F(1, 2), F(3, 2), F(2))),
        ("copy_multiplier", (F(1, 2), F(3, 2), F(2))),
        ("arithmetic_multiplier", (F(1, 2), F(3, 2), F(2))),
        ("xip_ticks", (F(256), F(1024), F(4096))),
        ("irq_ticks", (F(256), F(1024), F(4096))),
        ("prep_fraction", (F(1, 20), F(3, 10))),
        ("worker_share", (F(1, 3), F(4, 5))),
        ("source_fraction", (F(1, 5), F(7, 10))),
    )
    for field, values in sweeps:
        for value in values:
            base_scenario = Scenario(f"{field}={float(value):g}")
            if field in ("source_fraction", "worker_share"):
                base_scenario = replace(base_scenario, mask=1)
            if field == "prep_fraction":
                base_scenario = replace(base_scenario, melodic_multiplier=F(3, 2))
            scenarios.append(replace(base_scenario, **{field: value}))
    scenarios += [
        Scenario("combined-stress", melodic_multiplier=F(3, 2), worker_multiplier=F(3, 2),
                 fx_multiplier=F(5, 4), copy_multiplier=F(5, 4),
                 arithmetic_multiplier=F(3, 2), xip_ticks=F(1024), irq_ticks=F(1024)),
        Scenario("serial-stress", serial=True, melodic_multiplier=F(3, 2),
                 arithmetic_multiplier=F(3, 2)),
    ]
    results = []
    # 반복군이 가장 강한 보정 근거다. acid0는 최대 활성 fixture지만 n=1로 별도 표시한다.
    for indices, name in (((5, 6, 7), "NIGHT-3repeat"), ((2,), "acid0-single-point")):
        ref = captures[indices[0]]
        for scenario in scenarios:
            predictions = []
            detail = None
            for index in indices:
                c = captures[index]
                cost = fit(c, scenario)
                pred = simulate(cost, scenario)
                check_timeline(pred, scenario.serial)
                require(pred.audio_ticks > 0, "양수 시나리오")
                # 이 비율 이전도 이후도 분포 최대나 p99 추정이 아니다.
                scaled = c.words[10] * pred.audio_ticks / c.mean(8)
                predictions.append({
                    "capture_index": index, "predicted_mean_ticks": pred.audio_ticks,
                    "delta_from_observed_mean_ticks": pred.audio_ticks - c.mean(8),
                    "observed_max_ratio": F(c.words[10], 1) / c.mean(8),
                    "observed_max_scaled_sensitivity_ticks": scaled,
                    "conditional_mean_margin_ticks": DEADLINE - pred.audio_ticks,
                    "conditional_scaled_margin_ticks": DEADLINE - scaled,
                    "conditional_mean_crossing": pred.audio_ticks >= DEADLINE,
                    "conditional_scaled_crossing": scaled >= DEADLINE,
                    "wait_ticks": pred.wait_ticks, "overlap_ticks": pred.overlap_ticks,
                })
                if index == ref.index and scenario.name in (
                    "all-source-8voice-6drum-4FX", "serial-fallback", "combined-stress",
                ):
                    detail = asdict(pred)
            results.append({
                "reference": name, "reference_indices": indices,
                "inference": "세 반복군 내부 감도" if len(indices) == 3 else "단일점 보정; 비교 추론 금지",
                "scenario": asdict(scenario), "topology": {
                    "frames": 128, "slices": 8, "slice_frames": 16,
                    "local_voices": 5, "worker_voices": 3, "drums": 6, "fx_banks": 4,
                    "bpm": 240, "sends": [[127] * 4 for _ in range(4)],
                    "fx_types": [4, 5, 5, 5],
                    "units": {"local_voice_frames": 5 * 128, "worker_voice_frames": 3 * 128,
                              "drum_frames": 6 * 128, "fx_bank_frames": 4 * 128,
                              "source_frames": 8 * 128 * scenario.mask.bit_count()},
                },
                "predictions": predictions, "timeline_example": detail, "label": LABEL,
            })
    # 독립 최대값을 합하지 않고 같은 입력에서 overlap의 효과를 검증한다.
    refcost = fit(c5, Scenario("check"))
    dual = simulate(refcost, Scenario("check"))
    serial = simulate(refcost, Scenario("check", serial=True))
    require(serial.audio_ticks - dual.audio_ticks == dual.overlap_ticks, "serial-dual=overlap")
    gated = simulate(refcost, Scenario("check", mask=0))
    require(gated.audio_ticks <= dual.audio_ticks, "source-gating monotonic")
    heavier = simulate(refcost, Scenario("check", melodic_multiplier=F(2)))
    require(heavier.audio_ticks >= dual.audio_ticks, "cost monotonic")
    require(any(p["conditional_scaled_crossing"] for r in results for p in r["predictions"]),
            "deadline-crossing 시나리오 포함")
    require(any(not p["conditional_scaled_crossing"] for r in results for p in r["predictions"]),
            "양수 조건부 margin 시나리오 포함")

    report = {
        "schema": 1, "label": LABEL, "new_firmware_device_evidence": False,
        "binding": {"source_commit": COMMIT, "historical_app_sha256": APP,
                    "historical_source_input_sha256": SOURCE,
                    "historical_hash_verification": "원본 메타데이터/raw 동치; 바이너리 재해시 아님",
                    "input_sha256": inputs},
        "timer": {"nominal_timer4_hz": 24000000, "cpu_hz": None, "sample_rate": 44100,
                  "frames": 128, "deadline_ticks_integer": DEADLINE,
                  "deadline_ticks_physical": PHYSICAL, "physical_exact": "10240000/147",
                  "lrclk_measured": False},
        "observed": {"captures": decoded, "worst_audio_ticks": 69184,
                     "worst_audio_us": F(69184, 24), "headroom_ticks": 464,
                     "headroom_us": F(464, 24), "excluded_drift_indices": [1, 9, 10],
                     "matched_repeat_indices": [5, 6, 7],
                     "single_point_indices": [0, 2, 3, 4, 8],
                     "settings_note": "release JSON은 settings 문자열을 보존하지 않음. "
                                      "명시 workload+raw 24..40+batch만 확인; 신규 후보 비교 없음",
                     "counterexample_only_unrelated_peak_sum_ticks": peak_counterexample},
        "assumptions": {
            "status": "ASSUMPTION/SENSITIVITY; 학습된 hybrid 계수 아님",
            "virtual_schedule": "8×16 균등, 9회 dispatch/join; CPU1 FX(s-1) 다음 synth(s)",
            "prep_fraction": "0.15; CPU0 synth 집계에서 prep/공통 EG 비중 가정",
            "worker_synth": "wait 역산 잠재값; CPU1 synth export 없음; 실제 시간 식별 아님",
            "worker_share": "잠재 synth의 melodic3/drum6 비용 분할 0.6/0.4",
            "source_cost": "공통55% + PCM15% + FM15% + VA15%; all-source 비용=역사 대비1× 가정",
            "arithmetic": "melodic 비용20%에 64bit/division 배율 적용; 명령어별 비용 식별 아님",
            "transfer": "관측 transfer 총합을 9회 copy-in/out에 균등 분배; b 고정항 미식별",
            "baseline_work_units": "각 소스 활성수·note age 기록 없음; 가상 활성 work units에 "
                                   "stage_mean/u를 배정; 동적 C 계측 아님",
            "timer5": "관측 elapsed/stage의 preemption 포함 가능; 별도 가산하면 중복이므로 관측만",
            "residual": "render-CPU0 경로 합과 audio-render를 고정 잔차로 유지; 원인/상한 미지",
            "max_scaling": "observed_max*(pred_mean/obs_mean); 역사 peak/mean 모양 유지 가정, "
                           "예측 max/p99 아님; 정수 deadline과 조건부 비교",
            "xip_irq": "기본 추가 증분0은 '없음' 아님. [256,1024,4096] ticks는 임의 감도 예시, "
                       "487 interval excess와 관계없는 독립 선택; 검증 범위 아님",
            "serial": "dual 비활성 정상 fallback; mid-job timeout/격리·회복 비용 미포함. "
                      "dual과 비용 비교를 위해 transfer 그대로 유지",
            "workload": "현재 acid0 fixture: 8 all-source+Analog127, drums6, FX4, "
                        "BPM240, sends127; NOTE/step/EG 종료·PCM 끝·FM 수명은 정상 구현에서 "
                        "활성수를 줄일 수 있음. 고정 활성8은 조건부 스트레스 가정",
            "reference_topology": "NIGHT 각 프레임 활성수 미지. acid0는 역사 최대활성 fixture 단일점. "
                                  "역사 FX type/Analog/프로그램은 현재와 다르므로 동등 workload 아님",
        },
        "predicted": {"baseline_algebra": fits, "scenarios": results},
        "unknown": {"cpu_hz": None, "cpu1_synth_measured_ticks": None,
                    "hybrid_source_cost_measured": None, "xip_measured_ticks": None,
                    "irq_bound_ticks": None, "division_cycles": None, "p99": None,
                    "native_host_ratio_used": False, "largest_limitations": LIMITATIONS},
        "current_target": target, "copy_work_units": copy_bytes,
        "checks": {"raw_manual_drift_overlap": "PASS", "baseline_exact_residual_ticks": 0,
                   "no_firmware_verdict": True, "scenario_count": len(results)},
    }
    text = json.dumps(report, ensure_ascii=False, indent=2,
                      default=lambda value: float(value) if isinstance(value, F) else asdict(value)) + "\n"
    lines = [
        "# 조건부 부하 민감도 보고서", "", LABEL, "",
        "## 관측", "",
        f"- 역사 app `{APP}`, source-input `{SOURCE}`.",
        "- 원본과 raw 11×(perf42+stage42) 일치, stage schema5; 90,112 blocks.",
        "- drift 1/9/10 제외. 5/6/7만 clean 3회 반복군. 0/2/3/4/8은 단일점.",
        "- TIMER4 명목24MHz; MCU CPU Hz=null. 정수 deadline=69,648 ticks, "
        "물리 계산=69,659.863946 ticks(LRCLK 직접 측정 아님).",
        "- 관측 worst=69,184 ticks=2,882.666667us; 여유464 ticks=19.333333us.",
        "- stage 최대 합84,686은 동시 최악값이 아닌 금지 반례. "
        "interval excess는 IRQ 지연 상한이 아니다.", "",
        "## 예측", "",
        "관측 stage 평균으로 균등 8×16 timeline을 구성했다. CPU0 prep → submit/copy → "
        "local5 → join/copy → 이전 output → 현재 route; CPU1 이전 FX4 → 현재 melodic3+drums6. "
        "첫 회 FX 없음, 마지막 회 synth 없음. CPU1 bank 시간을 CPU0 경로에 다시 더하지 않는다.",
        "TIMER5는 추가 가산하지 않는다. render 잔차 및 audio-render 차이를 고정 항으로 남긴다.",
        "", "### 기준 평균 대수 재현", "",
        "| capture | 관측 audio mean ticks | 재현 mean ticks | 오차 | render 미분해 잔차 |",
        "|---|---:|---:|---:|---:|",
    ]
    for item in fits:
        index = item["capture_index"]
        lines.append(f"| {index} | {float(captures[index].mean(8)):.6f} | "
                     f"{float(item['predicted_audio_mean_ticks']):.6f} | 0 | "
                     f"{float(item['costs']['render_residual']):.6f} |")
    lines += [
        "", "### 민감도 스윕", "",
        "아래는 각 보정군 입력별 결과의 범위다. 각 행은 서로 다른 capture의 감도 결과이며 "
        "확률 구간·측정 최대·보증 상한이 아니다. scaled는 관측 max/mean 비율을 유지한 별도 감도값이다. "
        "margin≤0은 해당 가정의 정수 deadline 도달/초과일 뿐 MCU 실측 판정이 아니다.",
        "", "| 기준 | 시나리오 | 조건부 mean ticks | scaled 감도 margin ticks | 조건부 crossing |",
        "|---|---|---:|---:|---|",
    ]
    for result in results:
        ps = result["predictions"]
        means = [float(p["predicted_mean_ticks"]) for p in ps]
        margins = [float(p["conditional_scaled_margin_ticks"]) for p in ps]
        crossing = any(p["conditional_scaled_crossing"] for p in ps)
        lines.append(
            f"| {result['reference']} | {result['scenario']['name']} | "
            f"{min(means):.2f}..{max(means):.2f} | {min(margins):.2f}..{max(margins):.2f} | "
            f"{'가정하 도달/초과' if crossing else '가정하 양수 여유'} |"
        )
    lines += ["", "## ASSUMPTION/SENSITIVITY", ""]
    for name, value in report["assumptions"].items():
        lines.append(f"- `{name}`: {value}")
    lines += ["", "## 현재 타깃 (기존 보고서)", ""]
    if target:
        lines += [
            f"- 보고 app SHA-256: `{target['report']['sha256']}`.",
            f"- 현재 source-input: `{target['actual_source_input_sha256']}`; "
            f"보고서와 일치: `{target['source_input_matches']}`.",
            "- ABI bytes: `" + json.dumps(target["report"].get("target_abi_bytes", {})) + "`.",
            "- 메모리: " + ", ".join(
                f"{k}={target['report'].get(k)}" for k in (
                    "app_bytes", "app_limit", "data_bss_bytes", "data_bss_policy_limit",
                    "pool_bytes", "pool_policy_limit", "ram_text_bytes",
                )) + ".",
            "- 빌드/장치 시험을 이번에 수행한 것이 아니다. source 해시 불일치는 stale metadata다.",
        ]
    else:
        lines.append("- target-report 없음; ABI/메모리 미확인.")
    lines += ["", "## 미확인 및 가장 큰 한계", ""]
    lines += [f"- {limitation}" for limitation in LIMITATIONS]
    lines += [
        "", "## 재현 및 검사", "",
        f"`python3 tools/load_model.py`: raw/manual/drift/overlap PASS; "
        f"{len(fits)} clean 평균 exact 재현; {len(results)}개 조건부 시나리오.",
        "시나리오별 입력·보정 계수·작업 단위·대표 timeline과 모든 읽은 입력의 SHA-256은 "
        "`model.json`에 있다. 같은 입력에는 byte-identical 출력이다.",
        "문서: `docs/LOAD_MODEL.md`. firmware pass는 출력하지 않는다.", "",
    ]
    output = ROOT / "build/load"
    output.mkdir(parents=True, exist_ok=True)
    (output / "model.json").write_text(text, encoding="utf-8")
    (output / "model.md").write_text("\n".join(lines), encoding="utf-8")
    print(f"PASS raw/manual/drift/overlap; exact fits={len(fits)}; "
          f"conditional scenarios={len(results)}; firmware verdict=NONE")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, OSError, json.JSONDecodeError) as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1) from error
