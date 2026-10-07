#pragma once
/*  osiris_lcars.h — die beiden osiris-Schirme (TOP und BOTTOM, je 800 × 480)
 *  im LCARS-Stil. Entwurf im Simulator; die Firmware ruft später dieselben
 *  Aufbaufunktionen und die Setter.                                          */

#include <stdbool.h>
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

enum { OSL_TOP = 0, OSL_BOTTOM = 1 };

/* Seiten der beiden Schirme (= Blöcke in der linken Säule) */
enum { OSL_TOP_KATEGORIEN = 0, OSL_TOP_RAEUME, OSL_TOP_SYSTEM, OSL_TOP_PAGES };
enum { OSL_BOT_UHR = 0, OSL_BOT_STEUERUNG, OSL_BOT_TEMP, OSL_BOT_ENERGIE, OSL_BOT_BATTERIE,
       OSL_BOT_LUEFTUNG, OSL_BOT_WETTER, OSL_BOT_PAGES };

bool osiris_lcars_init(void);
void osiris_lcars_tick(void);

/* Simulator/Renderer: Schirm und Seite wählen */
void osiris_lcars_show(int screen, int page);
int  osiris_lcars_screen(void);
int  osiris_lcars_page(void);
