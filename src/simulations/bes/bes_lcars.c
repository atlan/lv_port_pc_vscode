#include "bes_lcars.h"
#include "bes_tones.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

LV_FONT_DECLARE(ui_font_antonio24);
LV_FONT_DECLARE(ui_font_antonio36);
LV_FONT_DECLARE(ui_font_antonio64);
LV_FONT_DECLARE(ui_font_antonio110);
LV_FONT_DECLARE(ui_font_antonio200);
LV_IMAGE_DECLARE(bes_gitter);     /* Bes als Drahtgitter, 283 x 600, nur Deckkraft */

#define C(hex) lv_color_hex(hex)

/* ---------------------------------------------------------------------------
 *  Maße — alles hängt an diesen Zahlen (Schirm 1280 x 800 quer)
 * ------------------------------------------------------------------------- */
#define SCR_W     1280
#define SCR_H      800
#define GAP          6      /* schwarzer Abstand zwischen allen Blöcken */
#define SIDE_W     210      /* Breite der Betriebsarten-Säule */
#define TOP_H       64      /* Höhe der Kopfleiste */
#define BOT_H       44      /* Höhe der Fußleiste */
#define R_OUT       60      /* Außenradius der Winkel */
#define R_IN        40      /* Innenbogen der Winkel */
#define ELBOW_W    (SIDE_W + R_IN + 50)
#define ELBOW_TOP  (TOP_H + R_IN + 26)
#define ELBOW_BOT  (BOT_H + R_IN + 26)
#define MAIN_X     (SIDE_W + 46)
#define MAIN_Y     (TOP_H + 26)
#define MAIN_W     (SCR_W - MAIN_X - 30)
#define MAIN_H     (SCR_H - BOT_H - 26 - MAIN_Y)
#define FIGURE_W   283      /* Breite des Bes-Gitterbilds */
#define FOOT_FIELD_W 228    /* Felder der Fussleiste, wie bei vier gleichen Feldern */
#define MENU_W      70      /* Endkappe der Kopfleiste = Menuetaste */

/* ---------------------------------------------------------------------------
 *  Farbrollen und Paletten — ein Zustandswechsel tauscht nur die Palette
 * ------------------------------------------------------------------------- */
enum {
    ROLE_FRAME_A,           /* Winkel */
    ROLE_FRAME_B,           /* Kopfleiste */
    ROLE_FRAME_C,           /* Fußleiste */
    ROLE_MODE0,             /* sechs Betriebsarten-Blöcke */
    ROLE_HILITE = ROLE_MODE0 + BES_LCARS_MODE_COUNT,   /* gewählte Betriebsart */
    ROLE_TEXT,              /* große Statusschrift */
    ROLE_ACCENT,            /* kleine Überschriften, Linien */
    ROLE_DATA,              /* Countdown, eingegebene Stellen */
    ROLE_DIM,               /* leere Stellen, Hinweise */
    ROLE_COUNT
};

typedef struct {
    uint32_t    c[ROLE_COUNT];
    uint32_t    pulse_period;   /* ms, 0 = kein Pulsieren */
    uint32_t    pulse_alt;      /* Wechselfarbe der pulsierenden Leisten */
    const char *title;
    const char *message;
    int         countdown;      /* Startwert in s, 0 = kein Countdown */
} palette_t;

static const palette_t PALETTE[BES_LCARS_STATE_COUNT] = {
    [BES_LCARS_DISARMED] = {
        .c = { 0xFF9900, 0xFF9966, 0xCC99CC,
               0xCC99CC, 0x9999FF, 0xFF9966, 0xFFCC99, 0xCC9966, 0x99CCFF,
               0xF5F6FA, 0xFFCC99, 0xCC99FF, 0x99CCFF, 0x333333 },
        .title = "UNSCHARF", .message = "SYSTEM BEREIT",
    },
    [BES_LCARS_EXIT_DELAY] = {
        .c = { 0xFFAA00, 0xFF8800, 0xFFCC99,
               0xFF9966, 0xFFAA00, 0xFF8866, 0xFFCC99, 0xCC9966, 0xFF9900,
               0xF5F6FA, 0xFFAA00, 0xFFCC99, 0xF5F6FA, 0x333333 },
        .pulse_period = 500, .pulse_alt = 0x664400,
        .title = "AUSTRITT", .message = "SCHARF IN", .countdown = 30,
    },
    [BES_LCARS_ARMED] = {
        .c = { 0x2266FF, 0x6699FF, 0x9966CC,
               0x2233FF, 0x6699FF, 0x9966CC, 0x2266FF, 0x5566FF, 0x9999CC,
               0xF5F6FA, 0x88BBFF, 0x9966CC, 0x6699FF, 0x222244 },
        .title = "SCHARF", .message = "ALLE ZONEN AKTIV",
    },
    [BES_LCARS_ENTRY_DELAY] = {
        .c = { 0xFFCC00, 0xFFDD33, 0xFFAA00,
               0xFFCC00, 0xFFAA00, 0xFFDD33, 0xFFCC99, 0xFFCC00, 0xFFAA00,
               0xF5F6FA, 0xFFDD33, 0xFFCC99, 0xF5F6FA, 0x333333 },
        .pulse_period = 300, .pulse_alt = 0x665200,
        .title = "EINTRITT", .message = "CODE EINGEBEN", .countdown = 20,
    },
    [BES_LCARS_TRIGGERED] = {
        .c = { 0xFF2200, 0xFF2200, 0xFF2200,
               0xFF2200, 0xFF5555, 0xCC2233, 0xFF2200, 0xFF5555, 0xCC2233,
               0xF5F6FA, 0xF5F6FA, 0xFF5555, 0xF5F6FA, 0x441111 },
        .pulse_period = 250, .pulse_alt = 0xF5F6FA,
        .title = "ALARM", .message = "EINBRUCH ERKANNT",
    },
};

/* Die Betriebsarten von Alarmo (Home Assistant), AUS zuoberst:
 *   disarmed, armed_away, armed_home, armed_night, armed_vacation, armed_custom_bypass */
static const char *MODE_NAME[BES_LCARS_MODE_COUNT] = {
    "AUS", "ABWESEND", "ZUHAUSE", "NACHT", "URLAUB", "MIT BYPASS",
};

/* ---------------------------------------------------------------------------
 *  Zustand
 * ------------------------------------------------------------------------- */
typedef enum { KIND_BG, KIND_TEXT, KIND_IMAGE } themed_kind_t;

typedef struct {
    lv_obj_t     *obj;
    uint8_t       role;
    themed_kind_t kind;
} themed_t;

static themed_t themed[200];
static int      themed_n;

static bes_lcars_state_t state = BES_LCARS_DISARMED;
static int               mode = 0;
static int               countdown = 0;
static int               code_digits = 0;
static bool              pulse_phase = false;

static lv_obj_t *mode_block[BES_LCARS_MODE_COUNT];
static lv_obj_t *mode_marker;
static lv_obj_t *lbl_clock;
static lv_obj_t *lbl_presence;        /* Beschriftung des Umschalters in der Fussleiste */
static lv_obj_t *lbl_climate;         /* Raumklima in der Fussleiste */
static void    (*presence_cb)(int);   /* Firmware: Umschalter angetippt -> HA */
static int       presence = 1;
static lv_obj_t *code_slot[BES_LCARS_CODE_LEN];
static lv_obj_t *top_bar;
static lv_obj_t *foot_block[3];       /* Fuellbalken, Klima, Anwesenheit */
static lv_obj_t *lbl_status;
static lv_obj_t *lbl_message;
static lv_obj_t *lbl_mode;
static lv_obj_t *lbl_count;
static lv_obj_t *lbl_count_unit;

static lv_timer_t *pulse_timer;
static lv_timer_t *countdown_timer;
static lv_timer_t *flash_timer;
static void (*menu_cb)(void);
static void (*mode_cb)(int);

/* --- Menü ------------------------------------------------------------------ */
enum { PAGE_WEATHER, PAGE_SCENES, PAGE_SWITCHES, PAGE_DEVICE, PAGE_COUNT };
static const char *PAGE_NAME[PAGE_COUNT] = { "WETTER", "SZENEN", "SCHALTER", "GERÄT" };
static void    (*device_cb)(int);         /* Firmware: Tür/Fach angetippt (bes_lcars_device_t) */

static bool      menu_open;
static int       menu_page = PAGE_WEATHER;
static lv_obj_t *main_home;                 /* Statusansicht */
static lv_obj_t *main_menu;                 /* Menüseiten */
static lv_obj_t *page[PAGE_COUNT];
static lv_obj_t *menu_block[PAGE_COUNT + 1]; /* Säule im Menü: drei Seiten + ZURÜCK */

static lv_obj_t *lbl_wx_location, *lbl_wx_temp, *lbl_wx_unit, *lbl_wx_condition;
static lv_obj_t *lbl_wx_detail[3];
static lv_obj_t *wx_day[BES_LCARS_FORECAST_MAX];
static lv_obj_t *wx_day_lbl[BES_LCARS_FORECAST_MAX][3];

static lv_obj_t *scene_block[BES_LCARS_ITEMS_MAX];
static int       scene_n;
static void    (*scene_cb)(int);
static lv_timer_t *scene_flash_timer;
static int       scene_flash_idx = -1;

static lv_obj_t *switch_block[BES_LCARS_ITEMS_MAX];
static lv_obj_t *switch_state_lbl[BES_LCARS_ITEMS_MAX];
static bool      switch_on[BES_LCARS_ITEMS_MAX];
static int       switch_n;
static void    (*switch_cb)(int, bool);

/* ---------------------------------------------------------------------------
 *  Bausteine
 * ------------------------------------------------------------------------- */

/* Nacktes Rechteck ohne Theme-Stile; alles weitere setzt der Aufrufer */
static lv_obj_t *box(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_scrollable(o, false);
    lv_obj_set_clickable(o, false);
    return o;
}

static lv_obj_t *themed_add(lv_obj_t *o, uint8_t role, themed_kind_t kind)
{
    LV_ASSERT(themed_n < (int)(sizeof(themed) / sizeof(themed[0])));
    themed[themed_n++] = (themed_t){ o, role, kind };
    return o;
}

/* Farbfläche in einer Rolle */
static lv_obj_t *block(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                       int32_t radius, uint8_t role)
{
    lv_obj_t *o = box(parent, x, y, w, h);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    return themed_add(o, role, KIND_BG);
}

static lv_obj_t *black(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius)
{
    lv_obj_t *o = box(parent, x, y, w, h);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(o, C(0x000000), 0);
    return o;
}

static lv_obj_t *text(lv_obj_t *parent, const lv_font_t *font, const char *txt, uint8_t role)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_label_set_text(l, txt);
    return themed_add(l, role, KIND_TEXT);
}

/* Schwarze Schrift auf einem Farbblock */
static lv_obj_t *caption(lv_obj_t *parent, const lv_font_t *font, const char *txt,
                         lv_align_t align, int32_t dx, int32_t dy)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, C(0x000000), 0);
    lv_label_set_text(l, txt);
    lv_obj_align(l, align, dx, dy);
    return l;
}

/* Der LCARS-Winkel. LVGL rundet immer alle vier Ecken, deshalb ein
 * beschneidender Rahmen mit zwei übergroßen Kindern: die Farbfläche zeigt
 * nur ihre eine runde Außenecke, die schwarze Fläche darüber nur ihre eine
 * runde Ecke — das ist der Innenbogen. */
static void elbow(lv_obj_t *parent, int32_t y, int32_t h, int32_t bar_h, bool bottom)
{
    lv_obj_t *clip = box(parent, 0, y, ELBOW_W, h);
    if (!bottom) {
        block(clip, 0, 0, ELBOW_W + R_OUT, h + R_OUT, R_OUT, ROLE_FRAME_A);
        black(clip, SIDE_W, bar_h, ELBOW_W, h, R_IN);
    } else {
        block(clip, 0, -R_OUT, ELBOW_W + R_OUT, h + R_OUT, R_OUT, ROLE_FRAME_A);
        black(clip, SIDE_W, -bar_h, ELBOW_W, h, R_IN);
    }
}

/* Block mit EINER runden Seite (links oder rechts), die andere gerade.
 * Gibt den beschneidenden Rahmen zurueck; die Farbflaeche ist sein erstes Kind. */
static lv_obj_t *halfpill(lv_obj_t *parent, int32_t w, int32_t h, uint8_t role, bool round_left)
{
    lv_obj_t *clip = box(parent, 0, 0, w, h);
    block(clip, round_left ? 0 : -h, 0, w + h, h, h / 2, role);
    return clip;
}

/* Endkappe: links gerade, rechts rund */
static lv_obj_t *cap(lv_obj_t *parent, int32_t w, int32_t h, uint8_t role)
{
    return halfpill(parent, w, h, role, false);
}

/* ---------------------------------------------------------------------------
 *  Darstellung
 * ------------------------------------------------------------------------- */

static void apply_palette(void)
{
    const palette_t *p = &PALETTE[state];

    for (int i = 0; i < themed_n; i++) {
        uint8_t role = themed[i].role;
        if (themed[i].obj == mode_block[mode]) role = ROLE_HILITE;
        if (themed[i].obj == menu_block[menu_page]) role = ROLE_HILITE;
        if (scene_flash_idx >= 0 && themed[i].obj == scene_block[scene_flash_idx]) role = ROLE_HILITE;
        for (int k = 0; k < switch_n; k++)
            if (themed[i].obj == switch_block[k]) role = switch_on[k] ? ROLE_HILITE : ROLE_FRAME_C;
        switch (themed[i].kind) {
        case KIND_TEXT:  lv_obj_set_style_text_color(themed[i].obj, C(p->c[role]), 0); break;
        case KIND_IMAGE: lv_obj_set_style_image_recolor(themed[i].obj, C(p->c[role]), 0); break;
        default:         lv_obj_set_style_bg_color(themed[i].obj, C(p->c[role]), 0); break;
        }
    }
    for (int i = 0; i < BES_LCARS_CODE_LEN; i++)
        lv_obj_set_style_bg_color(code_slot[i], C(p->c[i < code_digits ? ROLE_DATA : ROLE_DIM]), 0);

    lv_obj_align_to(mode_marker, mode_block[mode], LV_ALIGN_OUT_RIGHT_MID, GAP, 0);
    lv_label_set_text(lbl_mode, MODE_NAME[mode]);
}

static void show_countdown(void)
{
    bool on = countdown > 0;
    lv_obj_set_hidden(lbl_count, !on);
    lv_obj_set_hidden(lbl_count_unit, !on);
    if (on) lv_label_set_text_fmt(lbl_count, "%d", countdown);
}

static void pulse_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    const palette_t *p = &PALETTE[state];
    pulse_phase = !pulse_phase;
    /* Kopf- und Fußleiste blinken im Wechsel */
    lv_obj_set_style_bg_color(top_bar, C(pulse_phase ? p->pulse_alt : p->c[ROLE_FRAME_B]), 0);
    for (int i = 0; i < 3; i++)
        lv_obj_set_style_bg_color(foot_block[i], C(pulse_phase ? p->c[ROLE_FRAME_C] : p->pulse_alt), 0);
}

static void countdown_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    if (countdown <= 0) return;
    countdown--;
    show_countdown();
#ifndef BES_LCARS_DEVICE
    /* Nur für die Simulation: der Ablauf führt in den Folgezustand */
    if (countdown == 0) {
        if (state == BES_LCARS_EXIT_DELAY)  bes_lcars_set_state(BES_LCARS_ARMED);
        else if (state == BES_LCARS_ENTRY_DELAY) bes_lcars_set_state(BES_LCARS_TRIGGERED);
    }
#endif
}

/* ---------------------------------------------------------------------------
 *  Schnittstelle
 * ------------------------------------------------------------------------- */

void bes_lcars_set_state(bes_lcars_state_t s)
{
    if (s >= BES_LCARS_STATE_COUNT) return;
    state = s;
    const palette_t *p = &PALETTE[state];

    lv_label_set_text(lbl_status, p->title);
    lv_label_set_text(lbl_message, p->message);
    countdown = p->countdown;
    show_countdown();

    pulse_phase = false;
    apply_palette();
    if (p->pulse_period) {
        lv_timer_set_period(pulse_timer, p->pulse_period);
        lv_timer_resume(pulse_timer);
    } else {
        lv_timer_pause(pulse_timer);
    }
}

void bes_lcars_set_mode(int m)
{
    if (m < 0 || m >= BES_LCARS_MODE_COUNT) return;
    mode = m;
    apply_palette();
}

void bes_lcars_set_mode_cb(void (*cb)(int)) { mode_cb = cb; }

void bes_lcars_set_countdown(int seconds)
{
    countdown = seconds < 0 ? 0 : seconds;
    show_countdown();
}

void bes_lcars_set_code_digits(int count)
{
    if (count < 0) count = 0;
    if (count > BES_LCARS_CODE_LEN) count = BES_LCARS_CODE_LEN;
    code_digits = count;
    apply_palette();
}

void bes_lcars_set_message(const char *txt)
{
    lv_label_set_text(lbl_message, txt ? txt : "");
}

void bes_lcars_set_time(int hour, int minute)
{
    lv_label_set_text_fmt(lbl_clock, "%02d:%02d", hour, minute);
}

void bes_lcars_set_presence(int persons)
{
    presence = persons >= 2 ? 2 : 1;
    lv_label_set_text_fmt(lbl_presence, "ANWESENHEIT %d", presence);
}

int bes_lcars_get_presence(void)
{
    return presence;
}

void bes_lcars_set_menu_cb(void (*cb)(void))
{
    menu_cb = cb;
}

void bes_lcars_menu_open(bool open)
{
    menu_open = open;
    lv_obj_set_hidden(main_home, open);
    lv_obj_set_hidden(main_menu, !open);
    for (int i = 0; i < BES_LCARS_MODE_COUNT; i++) lv_obj_set_hidden(mode_block[i], open);
    lv_obj_set_hidden(mode_marker, open);
    for (int i = 0; i <= PAGE_COUNT; i++) lv_obj_set_hidden(menu_block[i], !open);
    for (int i = 0; i < PAGE_COUNT; i++) lv_obj_set_hidden(page[i], i != menu_page);
    apply_palette();
}

void bes_lcars_menu_page(int pg)
{
    if (pg < 0 || pg >= PAGE_COUNT) return;
    menu_page = pg;
    for (int i = 0; i < PAGE_COUNT; i++) lv_obj_set_hidden(page[i], i != menu_page);
    apply_palette();
}

void bes_lcars_set_weather(const char *location, int temp, const char *condition,
                           int humidity, const char *wind, const char *pressure,
                           const bes_lcars_forecast_t *forecast, int count)
{
    lv_label_set_text_fmt(lbl_wx_location, "WETTER · %s", location ? location : "");
    lv_label_set_text_fmt(lbl_wx_temp, "%d", temp);
    lv_obj_update_layout(lbl_wx_temp);              /* die Einheit haengt an der Breite der Zahl */
    lv_obj_align_to(lbl_wx_unit, lbl_wx_temp, LV_ALIGN_OUT_RIGHT_TOP, 8, 44);
    lv_label_set_text(lbl_wx_condition, condition ? condition : "");
    lv_label_set_text_fmt(lbl_wx_detail[0], "FEUCHTE %d %%", humidity);
    lv_label_set_text_fmt(lbl_wx_detail[1], "WIND %s", wind ? wind : "");
    lv_label_set_text_fmt(lbl_wx_detail[2], "%s", pressure ? pressure : "");
    for (int i = 0; i < BES_LCARS_FORECAST_MAX; i++) {
        bool da = forecast && i < count;
        lv_obj_set_hidden(wx_day[i], !da);
        if (!da) continue;
        lv_label_set_text(wx_day_lbl[i][0], forecast[i].day);
        lv_label_set_text_fmt(wx_day_lbl[i][1], "%d° / %d°", forecast[i].temp_max, forecast[i].temp_min);
        lv_label_set_text(wx_day_lbl[i][2], forecast[i].condition);
    }
}

void bes_lcars_set_scenes(const char *const *names, int count)
{
    scene_n = count > BES_LCARS_ITEMS_MAX ? BES_LCARS_ITEMS_MAX : count;
    for (int i = 0; i < BES_LCARS_ITEMS_MAX; i++) {
        lv_obj_set_hidden(scene_block[i], i >= scene_n);
        if (i < scene_n) lv_label_set_text(lv_obj_get_child(scene_block[i], 0), names[i]);
    }
}

void bes_lcars_set_scene_cb(void (*cb)(int)) { scene_cb = cb; }

void bes_lcars_set_switches(const char *const *names, const bool *on, int count)
{
    switch_n = count > BES_LCARS_ITEMS_MAX ? BES_LCARS_ITEMS_MAX : count;
    for (int i = 0; i < BES_LCARS_ITEMS_MAX; i++) {
        lv_obj_set_hidden(switch_block[i], i >= switch_n);
        if (i >= switch_n) continue;
        lv_label_set_text(lv_obj_get_child(switch_block[i], 0), names[i]);
        switch_on[i] = on ? on[i] : false;
        lv_label_set_text(switch_state_lbl[i], switch_on[i] ? "AN" : "AUS");
    }
    apply_palette();
}

void bes_lcars_set_switch(int index, bool on)
{
    if (index < 0 || index >= switch_n) return;
    switch_on[index] = on;
    lv_label_set_text(switch_state_lbl[index], on ? "AN" : "AUS");
    apply_palette();
}

void bes_lcars_set_switch_cb(void (*cb)(int, bool)) { switch_cb = cb; }

/* ---------------------------------------------------------------------------
 *  Bedienung in der Simulation
 * ------------------------------------------------------------------------- */

static void mode_clicked(lv_event_t *e)
{
    int m = (int)(intptr_t)lv_event_get_user_data(e);
    bes_tones_play(BES_TONE_SELECT);
    if (mode_cb) mode_cb(m);          /* Geraet: Alarmo entscheidet, die Anzeige folgt dem Abo */
    else         bes_lcars_set_mode(m);
}

static void flash_ende(lv_timer_t *t)
{
    LV_UNUSED(t);
    lv_label_set_text(lbl_message, PALETTE[state].message);
    lv_timer_pause(flash_timer);
}

static void presence_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    int neu = presence == 1 ? 2 : 1;
    bes_tones_play(BES_TONE_SELECT);
    if (presence_cb) presence_cb(neu);      /* die Firmware setzt HA; der Wert kommt zurueck */
    else bes_lcars_set_presence(neu);
}

void bes_lcars_set_presence_cb(void (*cb)(int)) { presence_cb = cb; }

void bes_lcars_set_climate(int zehntel_grad, int feuchte)
{
    if (!lbl_climate) return;
    lv_label_set_text_fmt(lbl_climate, "%d,%d °C · %d %%", zehntel_grad / 10, abs(zehntel_grad % 10), feuchte);
}

static void menu_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    bes_tones_play(BES_TONE_SELECT);
    if (menu_cb) { menu_cb(); return; }
    bes_lcars_menu_open(!menu_open);
}

static void menu_block_clicked(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    bes_tones_play(BES_TONE_SELECT);
    if (idx == PAGE_COUNT) bes_lcars_menu_open(false);
    else                   bes_lcars_menu_page(idx);
}

static void scene_flash_ende(lv_timer_t *t)
{
    LV_UNUSED(t);
    scene_flash_idx = -1;
    lv_timer_pause(scene_flash_timer);
    apply_palette();
}

static void scene_clicked(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    bes_tones_play(BES_TONE_CONFIRM);
    scene_flash_idx = idx;                      /* kurz hell: Szene ausgelöst */
    apply_palette();
    lv_timer_reset(scene_flash_timer);
    lv_timer_resume(scene_flash_timer);
    if (scene_cb) scene_cb(idx);
}

static void switch_clicked(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    bes_tones_play(BES_TONE_KEY);
    bes_lcars_set_switch(idx, !switch_on[idx]);  /* sofort umschalten, HA bestätigt später */
    if (switch_cb) switch_cb(idx, switch_on[idx]);
}

/* Klick auf die Code-Stellen: so klingt ein falscher Code */
static void code_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
    bes_lcars_set_code_digits(0);
    bes_tones_play(BES_TONE_REJECT);
}

/* Klick ins Hauptfeld: nächster Zustand, dabei wandert die Code-Anzeige mit */
static void main_clicked(lv_event_t *e)
{
    LV_UNUSED(e);
#ifndef BES_LCARS_DEVICE   /* am Geraet bestimmt Alarmo den Zustand, nicht ein Tipp ins Feld */
    bes_lcars_set_code_digits((code_digits + 2) % (BES_LCARS_CODE_LEN + 1));
    bes_lcars_set_state((state + 1) % BES_LCARS_STATE_COUNT);
    /* zurück auf unscharf heißt: der Code wurde angenommen */
    bes_tones_play(state == BES_LCARS_DISARMED ? BES_TONE_CONFIRM : BES_TONE_KEY);
#endif
}

/* ---------------------------------------------------------------------------
 *  Aufbau
 * ------------------------------------------------------------------------- */

static void build_device_page(void);

static void build_frame(lv_obj_t *scr)
{
    /* Winkel oben und unten, dazwischen die Säule */
    elbow(scr, 0, ELBOW_TOP, TOP_H, false);
    elbow(scr, SCR_H - ELBOW_BOT, ELBOW_BOT, BOT_H, true);

    /* Kopfleiste: Balken, Titel, Endkappe */
    lv_obj_t *head = box(scr, ELBOW_W + GAP, 0, SCR_W - ELBOW_W - GAP, TOP_H);
    lv_obj_set_overflow_visible(head, true);    /* die Titelschrift ist höher als die Leiste */
    lv_obj_set_flex_flow(head, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(head, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(head, 14, 0);
    lv_obj_set_style_pad_left(head, 10, 0);
    lbl_clock = text(head, &ui_font_antonio64, "--:--", ROLE_DATA);

    top_bar = block(head, 0, 0, 10, TOP_H, 0, ROLE_FRAME_B);
    lv_obj_set_flex_grow(top_bar, 1);
    text(head, &ui_font_antonio64, "BES PERIMETER", ROLE_FRAME_A);
    /* die Endkappe rechts vom Titel ist die Menuetaste -- ohne Beschriftung */
    lv_obj_t *menu = cap(head, MENU_W, TOP_H, ROLE_FRAME_B);
    lv_obj_set_clickable(menu, true);
    lv_obj_add_event_cb(menu, menu_clicked, LV_EVENT_CLICKED, NULL);

    /* Fußleiste: Fuellbalken, dann zwei Felder in der alten Breite (Klima, Anwesenheit), Endkappe */
    lv_obj_t *foot = box(scr, ELBOW_W + GAP, SCR_H - BOT_H, SCR_W - ELBOW_W - GAP, BOT_H);
    lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(foot, GAP, 0);
    foot_block[0] = block(foot, 0, 0, 10, BOT_H, 0, ROLE_FRAME_C);
    lv_obj_set_flex_grow(foot_block[0], 1);
    for (int i = 1; i < 3; i++) foot_block[i] = block(foot, 0, 0, FOOT_FIELD_W, BOT_H, 0, ROLE_FRAME_C);
    lbl_climate = caption(foot_block[1], &ui_font_antonio24, "--,- °C · -- %", LV_ALIGN_RIGHT_MID, -12, 0);
    lbl_presence = caption(foot_block[2], &ui_font_antonio24, "", LV_ALIGN_RIGHT_MID, -12, 0);
    lv_obj_set_clickable(foot_block[2], true);
    lv_obj_add_event_cb(foot_block[2], presence_clicked, LV_EVENT_CLICKED, NULL);
    cap(foot, 40, BOT_H, ROLE_FRAME_C);
}

static void build_modes(lv_obj_t *scr)
{
    int32_t y0 = ELBOW_TOP + GAP;
    int32_t total = SCR_H - ELBOW_BOT - GAP - y0;
    int32_t h = (total - (BES_LCARS_MODE_COUNT - 1) * GAP) / BES_LCARS_MODE_COUNT;

    for (int i = 0; i < BES_LCARS_MODE_COUNT; i++) {
        int32_t y = y0 + i * (h + GAP);
        /* der letzte Block nimmt den Rest, damit unten keine Lücke bleibt */
        int32_t bh = (i == BES_LCARS_MODE_COUNT - 1) ? (y0 + total - y) : h;
        mode_block[i] = block(scr, 0, y, SIDE_W, bh, 0, ROLE_MODE0 + i);
        lv_obj_set_clickable(mode_block[i], true);
        lv_obj_add_event_cb(mode_block[i], mode_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        caption(mode_block[i], &ui_font_antonio36, MODE_NAME[i], LV_ALIGN_BOTTOM_RIGHT, -12, 0);
    }
    mode_marker = block(scr, 0, 0, 14, h, 7, ROLE_HILITE);
}

/* Die Säule im Menü: drei Seiten und ZURÜCK, zunächst verborgen */
static void build_menu_blocks(lv_obj_t *scr)
{
    int32_t y0 = ELBOW_TOP + GAP;
    int32_t total = SCR_H - ELBOW_BOT - GAP - y0;
    int32_t n = PAGE_COUNT + 1;
    int32_t h = (total - (n - 1) * GAP) / n;
    for (int i = 0; i < n; i++) {
        int32_t y = y0 + i * (h + GAP);
        int32_t bh = (i == n - 1) ? (y0 + total - y) : h;
        menu_block[i] = block(scr, 0, y, SIDE_W, bh, 0, i == PAGE_COUNT ? ROLE_ACCENT : ROLE_FRAME_C);
        lv_obj_set_clickable(menu_block[i], true);
        lv_obj_add_event_cb(menu_block[i], menu_block_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        caption(menu_block[i], &ui_font_antonio36, i == PAGE_COUNT ? "ZURÜCK" : PAGE_NAME[i], LV_ALIGN_BOTTOM_RIGHT, -12, 0);
        lv_obj_set_hidden(menu_block[i], true);
    }
}

static void build_menu(lv_obj_t *mm)
{
    for (int i = 0; i < PAGE_COUNT; i++) {
        page[i] = box(mm, 0, 0, MAIN_W, MAIN_H);
        lv_obj_set_hidden(page[i], i != menu_page);
    }

    /* --- Wetter: aktuell links, Vorhersage als Blockreihe unten --- */
    lv_obj_t *w = page[PAGE_WEATHER];
    lbl_wx_location = text(w, &ui_font_antonio24, "WETTER", ROLE_ACCENT);
    lv_obj_set_pos(lbl_wx_location, 0, 14);
    lbl_wx_temp = text(w, &ui_font_antonio200, "", ROLE_DATA);
    lv_obj_set_pos(lbl_wx_temp, 0, 46);
    lbl_wx_unit = text(w, &ui_font_antonio64, "°C", ROLE_DATA);
    lbl_wx_condition = text(w, &ui_font_antonio64, "", ROLE_TEXT);
    lv_obj_set_pos(lbl_wx_condition, 440, 70);
    for (int i = 0; i < 3; i++) {
        lv_obj_t *b = block(w, 440 + i * 186, 160, 180, 44, 0, ROLE_FRAME_C);
        lbl_wx_detail[i] = caption(b, &ui_font_antonio24, "", LV_ALIGN_RIGHT_MID, -12, 0);
    }
    block(w, 0, 240, MAIN_W, 4, 2, ROLE_ACCENT);
    lv_obj_set_pos(text(w, &ui_font_antonio24, "VORHERSAGE", ROLE_ACCENT), 0, 262);
    for (int i = 0; i < BES_LCARS_FORECAST_MAX; i++) {
        int32_t bw = (MAIN_W - (BES_LCARS_FORECAST_MAX - 1) * 10) / BES_LCARS_FORECAST_MAX;
        wx_day[i] = block(w, i * (bw + 10), 300, bw, 150, 0, ROLE_FRAME_C);
        wx_day_lbl[i][0] = caption(wx_day[i], &ui_font_antonio36, "", LV_ALIGN_TOP_LEFT, 12, 6);
        wx_day_lbl[i][1] = caption(wx_day[i], &ui_font_antonio36, "", LV_ALIGN_TOP_RIGHT, -12, 6);
        wx_day_lbl[i][2] = caption(wx_day[i], &ui_font_antonio24, "", LV_ALIGN_BOTTOM_RIGHT, -12, -8);
    }

    /* --- Szenen: Pillen in drei Spalten --- */
    lv_obj_t *sc = page[PAGE_SCENES];
    lv_obj_set_pos(text(sc, &ui_font_antonio24, "SZENEN · HOME ASSISTANT", ROLE_ACCENT), 0, 14);
    for (int i = 0; i < BES_LCARS_ITEMS_MAX; i++) {
        int32_t bw = (MAIN_W - 2 * 12) / 3;
        scene_block[i] = block(sc, (i % 3) * (bw + 12), 60 + (i / 3) * 112, bw, 100, 50, ROLE_FRAME_C);
        lv_obj_set_clickable(scene_block[i], true);
        lv_obj_add_event_cb(scene_block[i], scene_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        caption(scene_block[i], &ui_font_antonio36, "", LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_hidden(scene_block[i], true);
    }

    /* --- Schalter: Blöcke in zwei Spalten, AN hell, AUS gedämpft --- */
    lv_obj_t *sw = page[PAGE_SWITCHES];
    lv_obj_set_pos(text(sw, &ui_font_antonio24, "SCHALTER · HOME ASSISTANT", ROLE_ACCENT), 0, 14);
    for (int i = 0; i < BES_LCARS_ITEMS_MAX; i++) {
        int32_t bw = (MAIN_W - 12) / 2;
        switch_block[i] = block(sw, (i % 2) * (bw + 12), 60 + (i / 2) * 90, bw, 80, 0, ROLE_FRAME_C);
        lv_obj_set_clickable(switch_block[i], true);
        lv_obj_add_event_cb(switch_block[i], switch_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        caption(switch_block[i], &ui_font_antonio36, "", LV_ALIGN_LEFT_MID, 16, 0);
        switch_state_lbl[i] = caption(switch_block[i], &ui_font_antonio36, "AUS", LV_ALIGN_RIGHT_MID, -16, 0);
        lv_obj_set_hidden(switch_block[i], true);
    }
    build_device_page();
}

static void device_clicked(lv_event_t *e)
{
    int aktion = (int)(intptr_t)lv_event_get_user_data(e);
    bes_tones_play(BES_TONE_SELECT);
    if (device_cb) device_cb(aktion);
#ifndef BES_LCARS_DEVICE
    else bes_lcars_set_message("FINGER AUFLEGEN");
#endif
}

void bes_lcars_set_device_cb(void (*cb)(int)) { device_cb = cb; }

/* --- Gerät: Tür und Fach -- Ausführung erst nach erkanntem Finger (Firmware) --- */
static void build_device_page(void)
{
    lv_obj_t *d = page[PAGE_DEVICE];
    lv_obj_set_pos(text(d, &ui_font_antonio24, "GERÄT · FREIGABE PER FINGERABDRUCK", ROLE_ACCENT), 0, 14);
    static const char *T[3] = { "SCHLÜSSELBRETT ÖFFNEN", "FACH AUSFAHREN", "FACH EINFAHREN" };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *b = block(d, 0, 60 + i * 100, MAIN_W, 86, 43, i == 2 ? ROLE_FRAME_C : ROLE_FRAME_B);
        lv_obj_set_clickable(b, true);
        lv_obj_add_event_cb(b, device_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        caption(b, &ui_font_antonio36, T[i], LV_ALIGN_LEFT_MID, 30, 0);
    }
}

static void build_main(lv_obj_t *scr)
{
    lv_obj_t *root = box(scr, MAIN_X, MAIN_Y, MAIN_W, MAIN_H);
    lv_obj_set_clickable(root, true);
    lv_obj_add_event_cb(root, main_clicked, LV_EVENT_CLICKED, NULL);

    /* Statusansicht; die Menüseiten liegen daneben und werden umgeschaltet */
    lv_obj_t *m = main_home = box(root, 0, 0, MAIN_W, MAIN_H);
    main_menu = box(root, 0, 0, MAIN_W, MAIN_H);
    lv_obj_set_clickable(main_menu, true);          /* schluckt Klicks, sonst schaltet der Hintergrund den Zustand */
    lv_obj_set_hidden(main_menu, true);
    build_menu(main_menu);

    lv_obj_set_pos(text(m, &ui_font_antonio24, "SICHERHEITSSTATUS", ROLE_ACCENT), 0, 14);
    lbl_status = text(m, &ui_font_antonio110, "", ROLE_TEXT);
    lv_obj_set_pos(lbl_status, 0, 40);
    lbl_message = text(m, &ui_font_antonio36, "", ROLE_TEXT);
    lv_obj_set_pos(lbl_message, 0, 180);
    block(m, 0, 240, 560, 4, 2, ROLE_ACCENT);

    lv_obj_set_pos(text(m, &ui_font_antonio24, "CODE", ROLE_ACCENT), 0, 270);
    for (int i = 0; i < BES_LCARS_CODE_LEN; i++) {
        code_slot[i] = box(m, i * (84 + 10), 306, 84, 52);
        lv_obj_set_style_radius(code_slot[i], 26, 0);
        lv_obj_set_style_bg_opa(code_slot[i], LV_OPA_COVER, 0);
        lv_obj_set_clickable(code_slot[i], true);
        lv_obj_add_event_cb(code_slot[i], code_clicked, LV_EVENT_CLICKED, NULL);
    }

    lv_obj_set_pos(text(m, &ui_font_antonio24, "BETRIEBSART", ROLE_ACCENT), 0, 400);
    lbl_mode = text(m, &ui_font_antonio64, "", ROLE_DATA);
    lv_obj_set_pos(lbl_mode, 0, 426);

    /* Countdown oben, zwischen Statusschrift und Figur */
    lbl_count = text(m, &ui_font_antonio200, "", ROLE_DATA);
    lv_obj_align(lbl_count, LV_ALIGN_TOP_RIGHT, -FIGURE_W - 30, 0);
    lbl_count_unit = text(m, &ui_font_antonio36, "SEKUNDEN", ROLE_ACCENT);
    lv_obj_align(lbl_count_unit, LV_ALIGN_TOP_RIGHT, -FIGURE_W - 30, 182);

    /* Bes als Drahtgitter in der Farbe des Rahmens */
    lv_obj_t *fig = lv_image_create(m);
    lv_image_set_src(fig, &bes_gitter);
    lv_obj_align(fig, LV_ALIGN_RIGHT_MID, 0, 0);
    themed_add(fig, ROLE_FRAME_A, KIND_IMAGE);

#ifndef BES_LCARS_DEVICE   /* der Hinweis gilt nur fuer die Simulation am Rechner */
    lv_obj_align(text(m, &ui_font_antonio24, "SIMULATION · KLICK INS FELD = NÄCHSTER ZUSTAND · KLICK AUF CODE = FALSCHER CODE", ROLE_DIM),
                 LV_ALIGN_BOTTOM_LEFT, 0, 0);
#endif
}

bool bes_lcars_init(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, C(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(scr, false);

    build_frame(scr);
    build_modes(scr);
    build_menu_blocks(scr);
    build_main(scr);

    pulse_timer = lv_timer_create(pulse_cb, 500, NULL);
    lv_timer_pause(pulse_timer);
    countdown_timer = lv_timer_create(countdown_cb, 1000, NULL);
    flash_timer = lv_timer_create(flash_ende, 2000, NULL);
    lv_timer_pause(flash_timer);
    scene_flash_timer = lv_timer_create(scene_flash_ende, 400, NULL);
    lv_timer_pause(scene_flash_timer);

    /* Beispieldaten der Simulation -- die Firmware liefert sie aus Home Assistant */
    static const bes_lcars_forecast_t VORHERSAGE[5] = {
        { "DI", 7, 14, "BEWÖLKT" }, { "MI", 6, 12, "REGEN" }, { "DO", 4, 11, "SCHAUER" },
        { "FR", 3, 13, "HEITER" }, { "SA", 5, 15, "SONNIG" },
    };
    bes_lcars_set_weather("ZUHAUSE", 12, "BEWÖLKT", 62, "NW 15 KM/H", "1023 HPA", VORHERSAGE, 5);
    static const char *const SZENEN[] = { "ABEND", "KINO", "LESEN", "GUTE NACHT", "ALLES AUS", "BESUCH" };
    bes_lcars_set_scenes(SZENEN, 6);
    static const char *const SCHALTER[] = { "FLURLICHT", "TERRASSE", "GARAGE", "STECKDOSE TV", "WEIHNACHTSBAUM", "POOLPUMPE" };
    static const bool AN[] = { true, false, false, true, false, true };
    bes_lcars_set_switches(SCHALTER, AN, 6);

    bes_lcars_set_mode(1);
    bes_lcars_set_presence(1);
    bes_lcars_set_state(BES_LCARS_DISARMED);
    bes_tones_play(BES_TONE_BOOT);

    fprintf(stderr, "[bes_lcars] screen built OK (%d themed objects)\n", themed_n);
    return true;
}

/* In der Simulation kommt die Uhrzeit vom Rechner; die Firmware ruft bes_lcars_set_time() */
void bes_lcars_tick(void)
{
    static time_t last;
    time_t now = time(NULL);
    if (now == last) return;
    last = now;
    struct tm *t = localtime(&now);
    if (t) bes_lcars_set_time(t->tm_hour, t->tm_min);
}
