#pragma once
/*  lcars.h — kleiner Baukasten für LCARS-Oberflächen in LVGL 9.
 *
 *  Herausgelöst aus der bes-Oberfläche (bes_lcars.c), damit osiris dieselben
 *  Bausteine bekommt. Nichts hier kennt eine konkrete Seite: nur Flächen,
 *  Winkel, Kappen, Pillen und schwarze Beschriftung darauf.
 *
 *  Leitfaden (dorofino/LCARS-ESP32, LCARS_DESIGN_GUIDE.md): Hintergrund IMMER
 *  schwarz · warme Farben = Struktur (Rahmen, Tasten) · kühle = Daten ·
 *  violett = Akzent · Schrift auf Farbflächen schwarz, Schrift auf Schwarz in
 *  der Datenfarbe · Blöcke mit gleichem schwarzen Abstand (GAP) getrennt.    */

#include <stdbool.h>
#include <stdint.h>
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

/* TNG-Palette (dorofino) */
#define LCARS_ORANGE    0xFF9900
#define LCARS_PEACH     0xFFCC99
#define LCARS_TAN       0xFFCC66
#define LCARS_GOLD      0xFFAA00
#define LCARS_RED       0xCC6666
#define LCARS_BRICK     0xCC4444
#define LCARS_LAVENDER  0xCC99CC
#define LCARS_VIOLET    0x9966CC
#define LCARS_BLUE      0x9999FF
#define LCARS_ICE       0x99CCFF
#define LCARS_SKY       0x6688CC
#define LCARS_BUTTER    0xFFFF99
#define LCARS_MINT      0x99CC99
#define LCARS_WHITE     0xFFFFFF
#define LCARS_DIM       0x555577      /* ausgegraut, Hinweise */
#define LCARS_BLACK     0x000000

#define LCARS_GAP 6                   /* schwarzer Abstand zwischen Blöcken */

#ifdef __cplusplus
extern "C" {
#endif

/* Nackter Behälter ohne Stil, nicht scrollbar, nicht klickbar */
lv_obj_t *lcars_box(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h);
/* Farbfläche */
lv_obj_t *lcars_block(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                      int32_t radius, uint32_t color);
/* Schwarze Fläche (Innenbögen, Abdeckungen) */
lv_obj_t *lcars_black(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius);
/* Schrift in einer Farbe (auf Schwarz) */
lv_obj_t *lcars_text(lv_obj_t *parent, const lv_font_t *font, const char *txt, uint32_t color);
/* Schwarze Schrift auf einem Farbblock, ausgerichtet */
lv_obj_t *lcars_caption(lv_obj_t *parent, const lv_font_t *font, const char *txt,
                        lv_align_t align, int32_t dx, int32_t dy);

/* Der Winkel. Füllt links eine Säule der Breite side_w und oben (bzw. unten)
 * einen Balken der Höhe bar_h; der Innenbogen hat den Radius r_in, die
 * Außenecke r_out. clip_w/clip_h ist die Fläche, die der Winkel einnimmt. */
void lcars_elbow(lv_obj_t *parent, int32_t x, int32_t y, int32_t clip_w, int32_t clip_h,
                 int32_t side_w, int32_t bar_h, int32_t r_out, int32_t r_in,
                 bool bottom, uint32_t color);

/* Block mit EINER runden Seite; gibt den beschneidenden Rahmen zurück, die
 * Farbfläche ist sein erstes Kind. */
lv_obj_t *lcars_halfpill(lv_obj_t *parent, int32_t w, int32_t h, uint32_t color, bool round_left);
/* Endkappe: links gerade, rechts rund */
lv_obj_t *lcars_cap(lv_obj_t *parent, int32_t w, int32_t h, uint32_t color);
/* Pille (beide Seiten rund) mit schwarzer Beschriftung, klickbar */
lv_obj_t *lcars_pill(lv_obj_t *parent, int32_t w, int32_t h, uint32_t color,
                     const lv_font_t *font, const char *txt);

/* Farbe einer Fläche oder Pille nachträglich setzen (Hervorhebung) */
void lcars_set_color(lv_obj_t *o, uint32_t color);

#ifdef __cplusplus
}
#endif
