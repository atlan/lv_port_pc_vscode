#pragma once
#include "lvgl.h"

/* Geschwindigkeits-Icons 5..130 km/h (Dateien: sl005.c, sl010.c, ...) */

#if LVGL_VERSION_MAJOR >= 9
extern const lv_image_dsc_t sl005;
extern const lv_image_dsc_t sl010;
extern const lv_image_dsc_t sl020;
extern const lv_image_dsc_t sl030;
extern const lv_image_dsc_t sl040;
extern const lv_image_dsc_t sl050;
extern const lv_image_dsc_t sl060;
extern const lv_image_dsc_t sl070;
extern const lv_image_dsc_t sl080;
extern const lv_image_dsc_t sl090;
extern const lv_image_dsc_t sl100;
extern const lv_image_dsc_t sl110;
extern const lv_image_dsc_t sl120;
extern const lv_image_dsc_t sl130;
#else
extern const lv_img_dsc_t sl005;
extern const lv_img_dsc_t sl010;
extern const lv_img_dsc_t sl020;
extern const lv_img_dsc_t sl030;
extern const lv_img_dsc_t sl040;
extern const lv_img_dsc_t sl050;
extern const lv_img_dsc_t sl060;
extern const lv_img_dsc_t sl070;
extern const lv_img_dsc_t sl080;
extern const lv_img_dsc_t sl090;
extern const lv_img_dsc_t sl100;
extern const lv_img_dsc_t sl110;
extern const lv_img_dsc_t sl120;
extern const lv_img_dsc_t sl130;
#endif
