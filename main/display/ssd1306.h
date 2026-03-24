/*
 * Copyright (c) 2024, BlueRetro
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SSD1306_H_
#define _SSD1306_H_

#include <stdint.h>

/* I2C and OLED pin assignments — must not conflict with ps_spi.c */
/* PS/P1: DTR=34, SCK=33, TXD=32, RXD=19, DSR=21                 */
/* PS/P2: DTR=5,  SCK=26, TXD=27, RXD=22, DSR=25                 */
/* Analog LEDs: P1=12, P2=15                                        */
#define OLED_SDA_PIN   16
#define OLED_SCL_PIN   17
/* OLED_RST_PIN is optional; set to -1 to skip hardware reset       */
#define OLED_RST_PIN   18

#define OLED_I2C_ADDR  0x3C

#define OLED_WIDTH     128
#define OLED_HEIGHT    64
#define OLED_PAGES     (OLED_HEIGHT / 8)

/*
 * Initialize the SSD1306 over I2C.
 * If OLED_RST_PIN >= 0 a hardware reset is performed.
 * Returns 0 on success, non-zero on error.
 */
int ssd1306_init(void);

/* Fill the framebuffer with 0 (all pixels off). */
void ssd1306_clear(void);

/* Push the entire framebuffer to the display over I2C. */
void ssd1306_update(void);

/* Set or clear a single pixel (x in [0,127], y in [0,63]). */
void ssd1306_draw_pixel(uint8_t x, uint8_t y, uint8_t on);

/*
 * Draw one character from a built-in 5×8 font at position (x, y).
 * Only printable ASCII (0x20–0x7E) is supported; others are skipped.
 * Each glyph occupies 6 pixels wide (5 data + 1 spacing).
 */
void ssd1306_draw_char(uint8_t x, uint8_t y, char c);

/*
 * Draw a NUL-terminated string starting at (x, y).
 * max_w limits the pixel width used; text is clipped there.
 * Returns the x-position after the last character drawn.
 */
uint8_t ssd1306_draw_string(uint8_t x, uint8_t y, const char *str, uint8_t max_w);

/*
 * Draw a 1-bit-per-pixel, row-major bitmap of size w×h at (x, y).
 * MSB is the leftmost pixel in each byte.
 * The bitmap must contain ceil(w/8)*h bytes.
 */
void ssd1306_draw_bitmap(uint8_t x, uint8_t y,
                          const uint8_t *bmp, uint8_t w, uint8_t h);

#endif /* _SSD1306_H_ */
