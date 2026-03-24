/*
 * Copyright (c) 2024, BlueRetro
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UI_H_
#define _UI_H_

/* ------------------------------------------------------------------ */
/*  Button GPIO assignments — must not conflict with ps_spi.c          */
/*  PS/P1: DTR=34, SCK=33, TXD=32, RXD=19, DSR=21                    */
/*  PS/P2: DTR=5,  SCK=26, TXD=27, RXD=22, DSR=25                    */
/*  Analog LEDs: P1=12, P2=15                                          */
/*  HW2-only pins (safe for HW1): POWER_ON=13, RESET=14, POWER_OFF=16 */
/* ------------------------------------------------------------------ */
#define BTN_SWAP_P1P2_PIN           23  /* toggle P1<->P2 BT mapping */
#define BTN_FORCE_DISCONNECT_P1_PIN 14  /* force-disconnect P1        */
#define BTN_FORCE_DISCONNECT_P2_PIN 13  /* force-disconnect P2        */

/* Software debounce threshold (ms) */
#define BTN_DEBOUNCE_MS  50

/* OLED refresh interval when idle (ms) */
#define UI_REFRESH_MS    200

/*
 * Initialize the OLED display, GPIO buttons, and start the UI task.
 * Must be called after BT stack and adapter are up.
 */
void ui_init(void);

#endif /* _UI_H_ */
