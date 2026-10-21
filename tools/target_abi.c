/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 dspaudio */
#include "hybrid.h"
#include "parallel.h"

/* 앱에 링크하지 않는 타깃 ABI 측정용 상수다. 호스트 sizeof로 대체하지 않는다. */
const uint32_t mc_target_abi[] __attribute__((section(".mc_abi"),used)) = {
    UINT32_C(0x4d434142),1,
    sizeof(McPatch),sizeof(McVoice),sizeof(McWorkerVoice),sizeof(McSynthJob),
    sizeof(McFxJob),sizeof(McDual),sizeof(McEngine),sizeof(McPcmZone)
};
