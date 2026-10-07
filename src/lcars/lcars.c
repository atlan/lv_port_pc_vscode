/*  lcars.c — siehe lcars.h  */
#include "lcars.h"

#define C(hex) lv_color_hex(hex)

lv_obj_t *lcars_box(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_scrollable(o, false);
    lv_obj_set_clickable(o, false);
    return o;
}

lv_obj_t *lcars_block(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                      int32_t radius, uint32_t color)
{
    lv_obj_t *o = lcars_box(parent, x, y, w, h);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(o, C(color), 0);
    return o;
}

lv_obj_t *lcars_black(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius)
{
    return lcars_block(parent, x, y, w, h, radius, LCARS_BLACK);
}

lv_obj_t *lcars_text(lv_obj_t *parent, const lv_font_t *font, const char *txt, uint32_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, C(color), 0);
    lv_label_set_text(l, txt);
    return l;
}

lv_obj_t *lcars_caption(lv_obj_t *parent, const lv_font_t *font, const char *txt,
                        lv_align_t align, int32_t dx, int32_t dy)
{
    lv_obj_t *l = lcars_text(parent, font, txt, LCARS_BLACK);
    lv_obj_align(l, align, dx, dy);
    return l;
}

/* LVGL rundet immer alle vier Ecken. Deshalb: beschneidender Rahmen mit zwei
 * übergroßen Kindern — die Farbfläche zeigt nur ihre eine runde Außenecke,
 * die schwarze Fläche darüber nur ihre eine runde Ecke = der Innenbogen. */
void lcars_elbow(lv_obj_t *parent, int32_t x, int32_t y, int32_t clip_w, int32_t clip_h,
                 int32_t side_w, int32_t bar_h, int32_t r_out, int32_t r_in,
                 bool bottom, uint32_t color)
{
    lv_obj_t *clip = lcars_box(parent, x, y, clip_w, clip_h);
    if (!bottom) {
        lcars_block(clip, 0, 0, clip_w + r_out, clip_h + r_out, r_out, color);
        lcars_black(clip, side_w, bar_h, clip_w, clip_h, r_in);
    } else {
        lcars_block(clip, 0, -r_out, clip_w + r_out, clip_h + r_out, r_out, color);
        lcars_black(clip, side_w, -bar_h, clip_w, clip_h, r_in);
    }
}

lv_obj_t *lcars_halfpill(lv_obj_t *parent, int32_t w, int32_t h, uint32_t color, bool round_left)
{
    lv_obj_t *clip = lcars_box(parent, 0, 0, w, h);
    lcars_block(clip, round_left ? 0 : -h, 0, w + h, h, h / 2, color);
    return clip;
}

lv_obj_t *lcars_cap(lv_obj_t *parent, int32_t w, int32_t h, uint32_t color)
{
    return lcars_halfpill(parent, w, h, color, false);
}

lv_obj_t *lcars_pill(lv_obj_t *parent, int32_t w, int32_t h, uint32_t color,
                     const lv_font_t *font, const char *txt)
{
    lv_obj_t *p = lcars_block(parent, 0, 0, w, h, h / 2, color);
    lv_obj_set_clickable(p, true);
    if (txt) lcars_caption(p, font, txt, LV_ALIGN_CENTER, 0, 0);
    return p;
}

void lcars_set_color(lv_obj_t *o, uint32_t color)
{
    /* halfpill/cap: der Rahmen ist durchsichtig, die Farbe sitzt im ersten Kind */
    if (lv_obj_get_style_bg_opa(o, 0) == LV_OPA_TRANSP && lv_obj_get_child_count(o) > 0)
        o = lv_obj_get_child(o, 0);
    lv_obj_set_style_bg_color(o, C(color), 0);
}
