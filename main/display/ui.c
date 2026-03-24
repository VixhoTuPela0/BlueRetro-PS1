/*
 * Copyright (c) 2024, BlueRetro
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/gpio.h"
#include "bluetooth/host.h"
#include "bluetooth/hci.h"
#include "display/ssd1306.h"
#include "display/ui.h"

/* ------------------------------------------------------------------ */
/*  Controller icon — 32 × 32 pixels, row-major, MSB = left pixel      */
/* ------------------------------------------------------------------ */
/* A simplified gamepad silhouette:                                     */
/*  - rectangular body with shoulder bumps at the top                  */
/*  - two circular grips at the bottom                                 */
static const uint8_t controller_icon[32 * 4] = {
    /* row  0 */ 0x07, 0xE0, 0x07, 0xE0,
    /* row  1 */ 0x0F, 0xF0, 0x0F, 0xF0,
    /* row  2 */ 0x1F, 0xF8, 0x1F, 0xF8,
    /* row  3 */ 0x3F, 0xFC, 0x3F, 0xFC,
    /* row  4 */ 0xFF, 0xFF, 0xFF, 0xFF,
    /* row  5 */ 0xFF, 0xFF, 0xFF, 0xFF,
    /* row  6 */ 0xFF, 0xFF, 0xFF, 0xFF,
    /* row  7 */ 0xFF, 0x81, 0x81, 0xFF,
    /* row  8 */ 0xFF, 0x88, 0x11, 0xFF,
    /* row  9 */ 0xFF, 0x9C, 0x39, 0xFF,
    /* row 10 */ 0xFF, 0x88, 0x11, 0xFF,
    /* row 11 */ 0xFF, 0x81, 0x81, 0xFF,
    /* row 12 */ 0xFF, 0xFF, 0xFF, 0xFF,
    /* row 13 */ 0xFF, 0xFF, 0xFF, 0xFF,
    /* row 14 */ 0xFF, 0xFF, 0xFF, 0xFF,
    /* row 15 */ 0x7F, 0xFF, 0xFF, 0xFE,
    /* row 16 */ 0x3F, 0xFF, 0xFF, 0xFC,
    /* row 17 */ 0x3F, 0xFF, 0xFF, 0xFC,
    /* row 18 */ 0x3C, 0x3F, 0xFC, 0x3C,
    /* row 19 */ 0x7C, 0x3F, 0xFC, 0x3E,
    /* row 20 */ 0x7E, 0x3F, 0xFC, 0x7E,
    /* row 21 */ 0xFF, 0xFF, 0xFF, 0xFF,
    /* row 22 */ 0xFF, 0xFF, 0xFF, 0xFF,
    /* row 23 */ 0xFF, 0xFF, 0xFF, 0xFF,
    /* row 24 */ 0xFF, 0xFF, 0xFF, 0xFF,
    /* row 25 */ 0x7F, 0xFF, 0xFF, 0xFE,
    /* row 26 */ 0x3F, 0xFF, 0xFF, 0xFC,
    /* row 27 */ 0x1F, 0xFF, 0xFF, 0xF8,
    /* row 28 */ 0x0F, 0xFF, 0xFF, 0xF0,
    /* row 29 */ 0x07, 0xFF, 0xFF, 0xE0,
    /* row 30 */ 0x03, 0xFF, 0xFF, 0xC0,
    /* row 31 */ 0x01, 0xFF, 0xFF, 0x80,
};

/* ------------------------------------------------------------------ */
/*  Button debounce state                                               */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t  pin;
    uint8_t  raw;        /* last raw reading */
    uint16_t counter_ms; /* how long current raw level has been stable */
    uint8_t  stable;     /* debounced (settled) level */
    uint8_t  triggered;  /* set on falling-edge (press) event */
} btn_state_t;

static btn_state_t buttons[] = {
    { BTN_SWAP_P1P2_PIN,           1, 0, 1, 0 },
    { BTN_FORCE_DISCONNECT_P1_PIN, 1, 0, 1, 0 },
    { BTN_FORCE_DISCONNECT_P2_PIN, 1, 0, 1, 0 },
};
#define BTN_COUNT  (sizeof(buttons) / sizeof(buttons[0]))
#define BTN_SWAP   0
#define BTN_DISC1  1
#define BTN_DISC2  2

/* ------------------------------------------------------------------ */
/*  Helpers                                                             */
/* ------------------------------------------------------------------ */

static void buttons_init(void)
{
    gpio_config_t io = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    for (int i = 0; i < (int)BTN_COUNT; i++) {
        io.pin_bit_mask = 1ULL << buttons[i].pin;
        gpio_config(&io);
        int lvl = gpio_get_level(buttons[i].pin);
        buttons[i].raw    = (uint8_t)lvl;
        buttons[i].stable = (uint8_t)lvl;
    }
}

/*
 * Call every SCAN_MS milliseconds.  Returns non-zero if any button fired.
 */
#define SCAN_MS 10
static int buttons_scan(void)
{
    int fired = 0;
    for (int i = 0; i < (int)BTN_COUNT; i++) {
        int lvl = gpio_get_level(buttons[i].pin);
        if ((uint8_t)lvl == buttons[i].raw) {
            if (buttons[i].counter_ms < BTN_DEBOUNCE_MS) {
                buttons[i].counter_ms += SCAN_MS;
            }
            if (buttons[i].counter_ms >= BTN_DEBOUNCE_MS &&
                (uint8_t)lvl != buttons[i].stable) {
                uint8_t prev = buttons[i].stable;
                buttons[i].stable = (uint8_t)lvl;
                /* falling edge → button pressed (active-low) */
                if (prev == 1 && lvl == 0) {
                    buttons[i].triggered = 1;
                    fired = 1;
                }
            }
        } else {
            /* level changed — restart debounce */
            buttons[i].raw        = (uint8_t)lvl;
            buttons[i].counter_ms = 0;
        }
    }
    return fired;
}

/* ------------------------------------------------------------------ */
/*  OLED rendering                                                      */
/* ------------------------------------------------------------------ */

/* Layout constants */
#define ICON_W   32
#define ICON_H   32
#define HALF_W   64          /* half display width */
#define TEXT_Y   (OLED_HEIGHT - 8)  /* bottom row for names (page 7) */
#define LABEL_Y  (TEXT_Y - 10)      /* "P1"/"P2" label row */

/* Centre a 32-pixel-wide icon within a 64-px column starting at x_base */
#define ICON_X(x_base) ((uint8_t)((x_base) + (HALF_W - ICON_W) / 2))
/* Vertically centre the icon in the area above the text labels */
#define ICON_Y  ((uint8_t)((LABEL_Y - ICON_H) / 2))

static void render_player(uint8_t x_base, uint8_t player_idx,
                           int connected, const char *dev_name)
{
    /* Label "P1" or "P2" */
    char label[4];
    label[0] = 'P';
    label[1] = (char)('1' + player_idx);
    label[2] = '\0';
    ssd1306_draw_string(x_base + 1, LABEL_Y, label, HALF_W - 2);

    /* Controller icon (or absence indicator) */
    if (connected) {
        ssd1306_draw_bitmap(ICON_X(x_base), ICON_Y,
                            controller_icon, ICON_W, ICON_H);
    }
    /* else: leave that area blank (ssd1306_clear already zeroed it) */

    /* Device name at the very bottom, up to 10 characters */
    const char *name = (connected && dev_name && dev_name[0]) ? dev_name
                                                               : "No ctrl";
    ssd1306_draw_string(x_base + 1, TEXT_Y, name, HALF_W - 2);
}

static void render_display(int c0_connected, const char *name0,
                            int c1_connected, const char *name1)
{
    ssd1306_clear();

    if (!c0_connected && !c1_connected) {
        /* No controllers: single icon in the left half, vertically centred */
        ssd1306_draw_bitmap(ICON_X(0), ICON_Y,
                            controller_icon, ICON_W, ICON_H);
        ssd1306_draw_string(1, TEXT_Y, "No ctrl", OLED_WIDTH - 2);
    } else {
        /* Vertical separator between the two player areas */
        for (int y = 0; y < OLED_HEIGHT; y++) {
            ssd1306_draw_pixel(HALF_W - 1, (uint8_t)y, 1);
        }
        render_player(0,      0, c0_connected, name0);
        render_player(HALF_W, 1, c1_connected, name1);
    }

    ssd1306_update();
}

/* ------------------------------------------------------------------ */
/*  Main UI task                                                        */
/* ------------------------------------------------------------------ */

static void ui_task(void *arg)
{
    /* State from previous iteration — refresh only on change */
    int prev_c0 = -1, prev_c1 = -1;
    char prev_name0[32] = {0};
    char prev_name1[32] = {0};

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SCAN_MS));

        /* --- Button scanning ---------------------------------------- */
        buttons_scan();

        if (buttons[BTN_SWAP].triggered) {
            buttons[BTN_SWAP].triggered = 0;
            bt_host_swap_players(0, 1);
            printf("# UI: swap P1<->P2\n");
        }
        if (buttons[BTN_DISC1].triggered) {
            buttons[BTN_DISC1].triggered = 0;
            bt_host_disconnect_player(0);
        }
        if (buttons[BTN_DISC2].triggered) {
            buttons[BTN_DISC2].triggered = 0;
            bt_host_disconnect_player(1);
        }

        /* --- Display refresh --------------------------------------- */
        struct bt_dev *dev0 = NULL;
        struct bt_dev *dev1 = NULL;
        bt_host_get_active_dev_from_out_idx(0, &dev0);
        bt_host_get_active_dev_from_out_idx(1, &dev1);

        int c0 = (dev0 != NULL) ? 1 : 0;
        int c1 = (dev1 != NULL) ? 1 : 0;

        /* dev->name points to a static bt_name_type entry; name->name is a
         * char array (not a pointer) so it is always valid when name != NULL. */
        const char *n0 = (c0 && dev0->name) ? dev0->name->name : "";
        const char *n1 = (c1 && dev1->name) ? dev1->name->name : "";

        /* Only redraw if something changed */
        if (c0 != prev_c0 || c1 != prev_c1 ||
            strcmp(n0, prev_name0) != 0 ||
            strcmp(n1, prev_name1) != 0) {

            render_display(c0, n0, c1, n1);

            prev_c0 = c0;
            prev_c1 = c1;
            strncpy(prev_name0, n0, sizeof(prev_name0) - 1);
            prev_name0[sizeof(prev_name0) - 1] = '\0';
            strncpy(prev_name1, n1, sizeof(prev_name1) - 1);
            prev_name1[sizeof(prev_name1) - 1] = '\0';
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Public entry point                                                  */
/* ------------------------------------------------------------------ */

void ui_init(void)
{
    if (ssd1306_init() != 0) {
        printf("# ui_init: OLED init failed\n");
        /* Continue anyway — buttons still work */
    }

    buttons_init();

    xTaskCreatePinnedToCore(ui_task, "ui_task", 4096, NULL, 3, NULL, 0);
}
