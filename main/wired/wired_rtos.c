/*
 * Copyright (c) 2021-2022, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include "adapter/adapter.h"
#include "wired_rtos.h"

typedef void (*wired_init_t)(void);

static const wired_init_t wired_init[WIRED_MAX] = {
    NULL, /* WIRED_AUTO */
    NULL, /* PARALLEL_1P */
    NULL, /* PARALLEL_2P */
    NULL, /* NES */
    NULL, /* PCE */
    NULL, /* GENESIS */
    NULL, /* SNES */
    NULL, /* CDI */
    NULL, /* CD32 */
    NULL, /* REAL_3DO */
    NULL, /* JAGUAR */
    NULL, /* PSX */
    NULL, /* SATURN */
    NULL, /* PCFX */
    NULL, /* JVS */
    NULL, /* N64 */
    NULL, /* DC */
    NULL, /* PS2 */
    NULL, /* GC */
    NULL, /* WII_EXT */
    NULL, /* VB */
    NULL, /* PARALLEL_1P_OD */
    NULL, /* PARALLEL_2P_OD */
    NULL, /* SEA_BOARD */
};

void wired_rtos_init(void) {
    if (wired_init[wired_adapter.system_id]) {
        wired_init[wired_adapter.system_id]();
    }
}

