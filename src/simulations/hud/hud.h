#pragma once

#include <stdbool.h>
#include "lvgl/lvgl.h"

bool hud_init(void);
void hud_tick(void);

void hud_set_speed(float kmh);
void hud_set_rpm(float rpm);
void hud_set_fuel(float pct);
void hud_set_navi(const char *instruction, const char *summary, bool active);
void hud_set_navi_icon(int valhalla_type);
void hud_set_navi_dist(float dist_km);
void hud_set_speed_limit(int kmh);
void hud_set_focus_mode(bool focus);
void hud_toggle_focus_mode(lv_event_t *e);
