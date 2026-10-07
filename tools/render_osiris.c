/*  render_osiris.c — osiris_lcars ohne Fenster in PNGs rendern.
 *
 *      bin/render_osiris <zielordner>
 *
 *  Eigenes lv_display in LV_DISPLAY_RENDER_MODE_DIRECT (XRGB8888), je Seite
 *  einmal lv_refr_now(), Puffer als PNG schreiben (zlib). Kein SDL.          */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "lvgl/lvgl.h"
#include "../src/simulations/osiris_lcars/osiris_lcars.h"

#define W 800
#define H 480

static uint32_t fb[W * H];
static uint32_t ticks;

static void flush_cb(lv_display_t *d, const lv_area_t *a, uint8_t *px)
{
    (void)a; (void)px;
    lv_display_flush_ready(d);
}

static uint32_t tick_cb(void) { return ticks += 16; }

static void be32(FILE *f, uint32_t v) { uint8_t b[4] = { v >> 24, v >> 16, v >> 8, v }; fwrite(b, 1, 4, f); }

static void chunk(FILE *f, const char *typ, const uint8_t *data, uint32_t len)
{
    be32(f, len);
    fwrite(typ, 1, 4, f);
    if (len) fwrite(data, 1, len, f);
    uint32_t crc = crc32(0, (const Bytef *)typ, 4);
    if (len) crc = crc32(crc, data, len);
    be32(f, crc);
}

static int write_png(const char *path)
{
    size_t raw_len = (size_t)H * (W * 3 + 1);
    uint8_t *raw = malloc(raw_len);
    for (int y = 0; y < H; y++) {
        uint8_t *r = raw + (size_t)y * (W * 3 + 1);
        *r++ = 0;
        for (int x = 0; x < W; x++) {
            uint32_t p = fb[y * W + x];          /* XRGB8888 */
            *r++ = (p >> 16) & 0xFF; *r++ = (p >> 8) & 0xFF; *r++ = p & 0xFF;
        }
    }
    uLongf zlen = compressBound(raw_len);
    uint8_t *z = malloc(zlen);
    if (compress2(z, &zlen, raw, raw_len, 6) != Z_OK) return -1;
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    static const uint8_t sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    fwrite(sig, 1, 8, f);
    uint8_t ihdr[13] = { W >> 24, W >> 16, W >> 8, W & 0xFF, H >> 24, H >> 16, H >> 8, H & 0xFF, 8, 2, 0, 0, 0 };
    chunk(f, "IHDR", ihdr, 13);
    chunk(f, "IDAT", z, (uint32_t)zlen);
    chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(raw); free(z);
    return 0;
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    lv_init();
    lv_tick_set_cb(tick_cb);
    lv_display_t *d = lv_display_create(W, H);
    lv_display_set_color_format(d, LV_COLOR_FORMAT_XRGB8888);
    lv_display_set_buffers(d, fb, NULL, sizeof fb, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(d, flush_cb);

    osiris_lcars_init();

    static const char *TOPN[] = { "top_1_kategorien", "top_2_raeume", "top_3_system" };
    static const char *BOTN[] = { "bot_1_uhr", "bot_2_steuerung", "bot_3_temp", "bot_4_energie",
                                  "bot_5_batterie", "bot_6_lueftung", "bot_7_wetter" };
    for (int s = 0; s < 2; s++) {
        int n = s == 0 ? OSL_TOP_PAGES : OSL_BOT_PAGES;
        for (int p = 0; p < n; p++) {
            osiris_lcars_show(s, p);
            for (int i = 0; i < 5; i++) { lv_timer_handler(); lv_refr_now(d); }
            char path[512];
            snprintf(path, sizeof path, "%s/%s.png", dir, s == 0 ? TOPN[p] : BOTN[p]);
            if (write_png(path)) { fprintf(stderr, "PNG fehlgeschlagen: %s\n", path); return 1; }
            printf("%s\n", path);
        }
    }
    return 0;
}
