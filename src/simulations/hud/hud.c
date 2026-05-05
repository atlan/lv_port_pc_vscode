#include "hud.h"

#include <stdio.h>
#include <math.h>

#include "lvgl/lvgl.h"
#include "src/glyphs/navi_maneuver/navi_maneuver_glyphs.h"
#include "src/glyphs/hud_icons/hud_icons.h"
#include "src/glyphs/speedlimit_icons/speedlimit_glyphs.h"

LV_FONT_DECLARE(ui_font_mdi130);
LV_FONT_DECLARE(ui_font_mdi18);
LV_FONT_DECLARE(ui_font_mdi30);
LV_FONT_DECLARE(ui_font_mdi34);
LV_FONT_DECLARE(ui_font_star);
LV_FONT_DECLARE(ui_font_star100);
LV_FONT_DECLARE(ui_font_star160);
LV_FONT_DECLARE(ui_font_ver14);
LV_FONT_DECLARE(ui_font_ver16);
LV_FONT_DECLARE(ui_font_ver18);
LV_FONT_DECLARE(ui_font_ver18b);
LV_FONT_DECLARE(ui_font_ver24);
LV_FONT_DECLARE(ui_font_ver26);
LV_FONT_DECLARE(ui_font_ver30);
LV_FONT_DECLARE(ui_font_ver60);
LV_FONT_DECLARE(ui_font_ver70);
LV_FONT_DECLARE(ui_font_mdi100);
LV_FONT_DECLARE(ui_font_mdi80);

// ── Farbkonstanten ────────────────────────────────────────────────────────────
static uint32_t COL_WHITE    = 0xFFFFFF;
static uint32_t COL_GREEN    = 0x00CC44;
static uint32_t COL_ORANGE   = 0xFF8800;
static uint32_t COL_RED      = 0xFF2222;
static uint32_t COL_BLUE     = 0x00AAFF;
static uint32_t COL_GRAY     = 0x888888;
static uint32_t COL_DARKGRAY = 0x333333;
static uint32_t COL_BLACK    = 0x000000;

#define C(hex) lv_color_hex(hex)

#define SPEED_SEGMENTS 30
#define MAX_SPEED      230
static lv_obj_t *speed_segments[SPEED_SEGMENTS];

#define RPM_SEGMENTS  30
static lv_obj_t *s_rpm_segs[RPM_SEGMENTS];

#define FUEL_SEGMENTS 30
static lv_obj_t *s_fuel_segs[FUEL_SEGMENTS];

// ── Widget-Handles ────────────────────────────────────────────────────────────

static lv_obj_t *s_clock_lbl;
static lv_obj_t *s_lora_icon;
static lv_obj_t *s_gps_icon;
static lv_obj_t *s_gps_lbl;
static lv_obj_t *s_thermometer_icon;
static lv_obj_t *s_outside_lbl;

static lv_obj_t *s_rpm_bar;
static lv_obj_t *s_rpm_lbl;

static lv_obj_t *s_speed_val;
static lv_obj_t *s_speed_unit;

static lv_obj_t *s_engine_icon;
static lv_obj_t *s_coolant_lbl;
static lv_obj_t *s_fuel_icon;
static lv_obj_t *s_fuel_bar;
static lv_obj_t *s_fuel_lbl;
static lv_obj_t *s_dist_icon;
static lv_obj_t *s_dist_lbl;

static lv_obj_t *s_navi_row;
static lv_obj_t *s_navi_instr;
static lv_obj_t *s_navi_summ;

static lv_obj_t *s_navi_icon;
static lv_obj_t *s_navi_dist_lbl;

static lv_obj_t *s_speed_limit_cont;
static lv_obj_t *s_speed_limit_badge;
static lv_obj_t *s_speed_limit_lbl;
static int        s_speed_limit_kmh = 0;

static lv_obj_t *s_acc_l;
static lv_obj_t *s_acc_r;
static lv_obj_t *s_hline_top;
static lv_obj_t *s_hline_bottom;
static lv_obj_t *s_speed_gauge_cont;
static lv_obj_t *s_rpm_gauge_cont;
static lv_obj_t *s_fuel_gauge_cont;
static lv_obj_t *s_navi_icon_cont;
static lv_obj_t *s_info_row_obj;

// ── Hilfsfunktionen ──────────────────────────────────────────────────────────

static void clean_obj(lv_obj_t *o)
{
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(o, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *add_hline(lv_obj_t *parent, lv_coord_t y, uint32_t col)
{
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_set_size(line, 1240, 2);
    lv_obj_set_pos(line, 20, y);
    lv_obj_set_style_bg_color(line, C(col), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    return line;
}

static lv_color_t speed_color_for_index(int i)
{
    if (i < 7)  return lv_color_hex(0x00FF7F);
    if (i < 14) return lv_color_hex(0xFFD54F);
    return              lv_color_hex(0xFF5252);
}

static lv_obj_t *speed_gauge_create(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 230, 520);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_pad_row(cont, 4, 0);

    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN_REVERSE);
    lv_obj_set_flex_align(cont,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    int min_w = 20;
    int max_w = 220;

    for (int i = 0; i < SPEED_SEGMENTS; i++) {
        lv_obj_t *seg = lv_obj_create(cont);
        lv_obj_remove_style_all(seg);
        lv_obj_set_height(seg, (470 / SPEED_SEGMENTS) - 2);

        float t = (float)i / (SPEED_SEGMENTS - 1);
        float f = t * t;
        int w = min_w + (int)((max_w - min_w) * f);
        lv_obj_set_width(seg, w);

        lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(seg, lv_color_hex(0x555555), 0);
        lv_obj_set_style_radius(seg, 8, 0);

        speed_segments[i] = seg;
    }

    return cont;
}

static void speed_gauge_set_speed(int speed, int max_speed)
{
    if (max_speed <= 0) max_speed = 1;
    int lit = speed * SPEED_SEGMENTS / max_speed;
    if (lit < 0) lit = 0;
    if (lit > SPEED_SEGMENTS) lit = SPEED_SEGMENTS;

    for (int i = 0; i < SPEED_SEGMENTS; i++) {
        bool on = (i < lit);
        lv_obj_set_style_bg_color(speed_segments[i],
            on ? speed_color_for_index(i) : lv_color_hex(0x555555), 0);
        lv_obj_set_style_bg_opa(speed_segments[i], LV_OPA_COVER, 0);
    }
}

static lv_obj_t *rpm_gauge_create(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 230, 520);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_pad_row(cont, 4, 0);

    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN_REVERSE);
    lv_obj_set_flex_align(cont,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);

    int min_w = 20;
    int max_w = 220;

    for (int i = 0; i < RPM_SEGMENTS; i++) {
        lv_obj_t *seg = lv_obj_create(cont);
        lv_obj_remove_style_all(seg);
        lv_obj_set_height(seg, (470 / RPM_SEGMENTS) - 2);

        float t = (float)i / (RPM_SEGMENTS - 1);
        float f = t * t;
        int w = min_w + (int)((max_w - min_w) * f);
        lv_obj_set_width(seg, w);

        lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(seg, lv_color_hex(0x555555), 0);
        lv_obj_set_style_radius(seg, 8, 0);

        s_rpm_segs[i] = seg;
    }

    return cont;
}

static lv_obj_t *fuel_gauge_create(lv_obj_t *parent)
{
    lv_obj_t *fuel_grp = lv_obj_create(parent);
    lv_obj_set_size(fuel_grp, 40, 570);
    clean_obj(fuel_grp);

    lv_obj_t *cont = lv_obj_create(fuel_grp);
    lv_obj_set_size(cont, 30, 520);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_pad_row(cont, 4, 0);
    lv_obj_set_style_pad_left(cont, 10, 0);

    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN_REVERSE);
    lv_obj_set_flex_align(cont,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < FUEL_SEGMENTS; i++) {
        lv_obj_t *seg = lv_obj_create(cont);
        lv_obj_remove_style_all(seg);
        lv_obj_set_height(seg, (470 / FUEL_SEGMENTS) - 2);
        lv_obj_set_width(seg, 20);

        lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(seg, lv_color_hex(0x555555), 0);
        lv_obj_set_style_radius(seg, 8, 0);

        s_fuel_segs[i] = seg;
    }

    s_fuel_icon = lv_img_create(fuel_grp);
    lv_img_set_src(s_fuel_icon, &gas_station);
    lv_obj_set_style_img_recolor(s_fuel_icon, C(COL_GRAY), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(s_fuel_icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_pos(s_fuel_icon, 0, 525);

    return fuel_grp;
}

// ── Öffentliche set-Funktionen ────────────────────────────────────────────────

void hud_set_fuel(float pct)
{
    int ipct = (int)pct;
    if (ipct > 100) ipct = 100;

    int lit = ipct * FUEL_SEGMENTS / 100;
    if (lit < 0) lit = 0;
    if (lit > FUEL_SEGMENTS) lit = FUEL_SEGMENTS;

    uint32_t col;
    if (ipct <= 10)      col = COL_RED;
    else if (ipct <= 25) col = COL_ORANGE;
    else                 col = COL_GREEN;

    for (int i = 0; i < FUEL_SEGMENTS; i++) {
        bool on = (i < lit);
        if (on) {
            lv_obj_set_style_bg_color(s_fuel_segs[i], C(col), 0);
            lv_obj_set_style_bg_opa(s_fuel_segs[i], LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_bg_color(s_fuel_segs[i], C(COL_DARKGRAY), 0);
            lv_obj_set_style_bg_opa(s_fuel_segs[i], LV_OPA_60, 0);
        }
    }

    if (s_fuel_icon)
        lv_obj_set_style_img_recolor(s_fuel_icon, C(col), LV_PART_MAIN);
}

void hud_set_speed(float kmh)
{
    if (!s_speed_val) return;
    char buf[16];
    if (kmh < 0.0f) snprintf(buf, sizeof(buf), "--");
    else             snprintf(buf, sizeof(buf), "%.0f", kmh);
    lv_label_set_text(s_speed_val, buf);

    int over_limit = (s_speed_limit_kmh > 0) && (kmh > s_speed_limit_kmh + 2.0f);
    uint32_t col = over_limit ? COL_RED : COL_WHITE;
    lv_obj_set_style_text_color(s_speed_val, C(col), LV_PART_MAIN);

    int lit = (int)(kmh * SPEED_SEGMENTS / MAX_SPEED);
    if (lit < 0) lit = 0;
    if (lit > SPEED_SEGMENTS) lit = SPEED_SEGMENTS;

    for (int i = 0; i < SPEED_SEGMENTS; i++) {
        bool on = (i < lit);
        lv_obj_set_style_bg_color(speed_segments[i],
            on ? speed_color_for_index(i) : lv_color_hex(0x555555), 0);
        lv_obj_set_style_bg_opa(speed_segments[i], LV_OPA_COVER, 0);
    }
}

void hud_set_rpm(float rpm)
{
    if (!s_rpm_segs[0]) return;
    int irpm = (int)rpm;
    if (irpm < 0)    irpm = 0;
    if (irpm > 8000) irpm = 8000;

    int lit = irpm * RPM_SEGMENTS / 8000;
    if (lit < 0) lit = 0;
    if (lit > RPM_SEGMENTS) lit = RPM_SEGMENTS;

    uint32_t col_seg;
    if (irpm < 3500)      col_seg = COL_GREEN;
    else if (irpm < 5500) col_seg = COL_ORANGE;
    else                  col_seg = COL_RED;

    for (int i = 0; i < RPM_SEGMENTS; i++) {
        if (i < lit) {
            lv_obj_set_style_bg_color(s_rpm_segs[i], C(col_seg), 0);
            lv_obj_set_style_bg_opa(s_rpm_segs[i], LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_bg_color(s_rpm_segs[i], lv_color_hex(0x555555), 0);
            lv_obj_set_style_bg_opa(s_rpm_segs[i], LV_OPA_COVER, 0);
        }
    }
}

void hud_set_navi(const char *instruction, const char *summary, bool active)
{
    if (!s_navi_row) return;

    if (!active) {
        lv_obj_add_flag(s_navi_row, LV_OBJ_FLAG_HIDDEN);
        if (s_navi_icon)     lv_obj_add_flag(s_navi_icon, LV_OBJ_FLAG_HIDDEN);
        if (s_navi_dist_lbl) lv_obj_add_flag(s_navi_dist_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (s_navi_instr) lv_label_set_text(s_navi_instr, instruction ? instruction : "");
    if (s_navi_summ)  lv_label_set_text(s_navi_summ,  summary     ? summary     : "");

    lv_obj_clear_flag(s_navi_row, LV_OBJ_FLAG_HIDDEN);
}

void hud_set_navi_icon(int valhalla_type)
{
    if (!s_navi_icon) return;

    const lv_img_dsc_t *src = navi_maneuver_icon(valhalla_type);
    if (!src) {
        lv_obj_add_flag(s_navi_icon, LV_OBJ_FLAG_HIDDEN);
        if (s_navi_dist_lbl) lv_obj_add_flag(s_navi_dist_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_img_set_src(s_navi_icon, src);
    lv_obj_clear_flag(s_navi_icon, LV_OBJ_FLAG_HIDDEN);
}

void hud_set_navi_dist(float dist_km)
{
    if (!s_navi_dist_lbl || !s_navi_icon) return;

    if (dist_km < 0.0f || lv_obj_has_flag(s_navi_icon, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(s_navi_dist_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    char buf[16];
    if (dist_km < 0.100f)
        snprintf(buf, sizeof(buf), "%d m",   (int)(dist_km * 1000.0f + 0.5f));
    else
        snprintf(buf, sizeof(buf), "%.0f m", dist_km * 1000.0f);

    lv_label_set_text(s_navi_dist_lbl, buf);
    lv_obj_clear_flag(s_navi_dist_lbl, LV_OBJ_FLAG_HIDDEN);
}

static const lv_img_dsc_t *sl_img_for_kmh(int kmh)
{
    static const struct { int kmh; const lv_img_dsc_t *img; } map[] = {
        {   5, &sl005 }, {  10, &sl010 }, {  20, &sl020 }, {  30, &sl030 },
        {  40, &sl040 }, {  50, &sl050 }, {  60, &sl060 }, {  70, &sl070 },
        {  80, &sl080 }, {  90, &sl090 }, { 100, &sl100 }, { 110, &sl110 },
        { 120, &sl120 }, { 130, &sl130 },
    };
    for (int i = 0; i < (int)(sizeof(map) / sizeof(map[0])); i++)
        if (map[i].kmh == kmh) return map[i].img;
    return NULL;
}

void hud_set_speed_limit(int kmh)
{
    s_speed_limit_kmh = kmh;
    if (!s_speed_limit_badge) return;
    if (kmh <= 0) {
        lv_obj_add_flag(s_speed_limit_badge, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    const lv_img_dsc_t *img = sl_img_for_kmh(kmh);
    if (!img) {
        lv_obj_add_flag(s_speed_limit_badge, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_img_set_src(s_speed_limit_badge, img);
    lv_obj_clear_flag(s_speed_limit_badge, LV_OBJ_FLAG_HIDDEN);
}

static bool s_focus_mode = false;

void hud_set_focus_mode(bool focus)
{
#define SHOW(o, v) do { if (o) { if (v) lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN); \
                                 else   lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN); } } while(0)
    SHOW(s_acc_l,            !focus);
    SHOW(s_acc_r,            !focus);
    SHOW(s_speed_gauge_cont, !focus);
    SHOW(s_rpm_gauge_cont,   !focus);
#undef SHOW
    s_focus_mode = focus;
}

void hud_toggle_focus_mode(lv_event_t *e)
{
    (void)e;
    hud_set_focus_mode(!s_focus_mode);
}

// ── Screen-Aufbau ─────────────────────────────────────────────────────────────

bool hud_init(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, C(COL_BLACK), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Navigations-Streifen (oben, anfangs verborgen)
    s_navi_row = lv_obj_create(scr);
    lv_obj_set_size(s_navi_row, 1280, 100);
    lv_obj_set_pos(s_navi_row, 0, 0);
    clean_obj(s_navi_row);
    lv_obj_set_style_bg_color(s_navi_row, C(0x002233), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_navi_row, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_navi_row, 0, LV_PART_MAIN);

    lv_obj_t *navi_accent = lv_obj_create(s_navi_row);
    lv_obj_set_size(navi_accent, 5, 100);
    lv_obj_set_pos(navi_accent, 0, 0);
    lv_obj_set_style_bg_color(navi_accent, C(0x00E5FF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(navi_accent, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(navi_accent, 0, LV_PART_MAIN);

    s_navi_instr = lv_label_create(s_navi_row);
    lv_obj_set_style_text_font(s_navi_instr, &ui_font_ver60, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_navi_instr, C(COL_WHITE), LV_PART_MAIN);
    lv_label_set_long_mode(s_navi_instr, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_size(s_navi_instr, 880, 100);
    lv_obj_set_pos(s_navi_instr, 16, 0);
    lv_obj_set_style_text_align(s_navi_instr, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_label_set_text(s_navi_instr, "");

    s_navi_summ = lv_label_create(s_navi_row);
    lv_obj_set_style_text_font(s_navi_summ, &ui_font_ver60, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_navi_summ, C(0x00E5FF), LV_PART_MAIN);
    lv_obj_set_style_text_align(s_navi_summ, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_size(s_navi_summ, 350, 100);
    lv_obj_align(s_navi_summ, LV_ALIGN_RIGHT_MID, -16, 0);
    lv_label_set_text(s_navi_summ, "");

    lv_obj_add_flag(s_navi_row, LV_OBJ_FLAG_HIDDEN);

    // Linke Akzentlinie
    s_acc_l = lv_obj_create(scr);
    lv_obj_set_size(s_acc_l, 4, 575);
    lv_obj_set_pos(s_acc_l, 260, 115);
    lv_obj_set_style_bg_color(s_acc_l, C(COL_GREEN), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_acc_l, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_acc_l, 0, LV_PART_MAIN);

    // Tempolimit-Badge
    s_speed_limit_cont = lv_obj_create(scr);
    lv_obj_set_size(s_speed_limit_cont, 200, 590);
    lv_obj_set_pos(s_speed_limit_cont, 0, 115);
    lv_obj_set_style_bg_color(s_speed_limit_cont, C(COL_BLACK), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_speed_limit_cont, 0, LV_PART_MAIN);

    s_speed_limit_badge = lv_img_create(s_speed_limit_cont);
    lv_obj_set_size(s_speed_limit_badge, 150, 150);
    lv_obj_align(s_speed_limit_badge, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(s_speed_limit_badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_speed_limit_badge, LV_OBJ_FLAG_HIDDEN);

    // Navigations-Icon (rechts)
    s_navi_icon_cont = lv_obj_create(scr);
    lv_obj_set_size(s_navi_icon_cont, 340, 590);
    lv_obj_set_pos(s_navi_icon_cont, 940, 115);
    lv_obj_set_style_bg_color(s_navi_icon_cont, C(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_navi_icon_cont, 0, LV_PART_MAIN);

    s_navi_icon = lv_img_create(s_navi_icon_cont);
    lv_obj_set_size(s_navi_icon, 300, 300);
    lv_obj_align(s_navi_icon, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_img_recolor(s_navi_icon, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(s_navi_icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_img_set_src(s_navi_icon, NULL);
    lv_obj_add_flag(s_navi_icon, LV_OBJ_FLAG_HIDDEN);

    s_navi_dist_lbl = lv_label_create(s_navi_icon_cont);
    lv_obj_set_style_text_font(s_navi_dist_lbl, &lv_font_montserrat_40, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_navi_dist_lbl, C(0x00E5FF), LV_PART_MAIN);
    lv_obj_set_style_text_align(s_navi_dist_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_size(s_navi_dist_lbl, 300, 50);
    lv_obj_align(s_navi_dist_lbl, LV_ALIGN_CENTER, 0, 200);
    lv_label_set_text(s_navi_dist_lbl, "");
    lv_obj_add_flag(s_navi_dist_lbl, LV_OBJ_FLAG_HIDDEN);

    // Rechte Akzentlinie
    s_acc_r = lv_obj_create(scr);
    lv_obj_set_size(s_acc_r, 4, 575);
    lv_obj_set_pos(s_acc_r, 940, 115);
    lv_obj_set_style_bg_color(s_acc_r, C(COL_GREEN), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_acc_r, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_acc_r, 0, LV_PART_MAIN);

    // Geschwindigkeitszahl
    s_speed_val = lv_label_create(scr);
    lv_obj_set_style_text_color(s_speed_val, C(COL_WHITE), LV_PART_MAIN);
    lv_label_set_text(s_speed_val, "0");
    lv_obj_set_style_text_align(s_speed_val, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(s_speed_val, 556);
    lv_obj_align(s_speed_val, LV_ALIGN_CENTER, -40, 90);
    lv_obj_set_style_text_font(s_speed_val, &ui_font_star160, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Einheit "km/h"
    s_speed_unit = lv_label_create(scr);
    lv_obj_set_style_text_font(s_speed_unit, &lv_font_montserrat_36, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_speed_unit, C(COL_GRAY), LV_PART_MAIN);
    lv_label_set_text(s_speed_unit, "km/h");
    lv_obj_set_style_text_align(s_speed_unit, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(s_speed_unit, 360);
    lv_obj_align(s_speed_unit, LV_ALIGN_CENTER, -40, 210);

    s_speed_gauge_cont = speed_gauge_create(scr);
    lv_obj_align(s_speed_gauge_cont, LV_ALIGN_TOP_LEFT, 280, 115);

    s_rpm_gauge_cont = rpm_gauge_create(scr);
    lv_obj_align(s_rpm_gauge_cont, LV_ALIGN_TOP_LEFT, 690, 115);

    s_fuel_gauge_cont = fuel_gauge_create(scr);
    lv_obj_align(s_fuel_gauge_cont, LV_ALIGN_TOP_LEFT, 200, 115);

    fprintf(stderr, "[hud] screen built OK\n");
    return true;
}

// ── Simulations-Tick (Testdaten) ──────────────────────────────────────────────

void hud_tick(void)
{
    hud_set_fuel(50.0f);
    hud_set_speed(127.0f);
    hud_set_rpm(3200.0f);
    speed_gauge_set_speed(127, 230);
    hud_set_navi("Instruction", "Summary", true);
    hud_set_navi_icon(9);
    hud_set_navi_dist(0.3f);
    hud_set_speed_limit(130);
    hud_set_focus_mode(false);
}
