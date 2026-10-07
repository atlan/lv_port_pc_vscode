/*  osiris_lcars.c — osiris (2 × 800 × 480) im LCARS-Stil, Entwurf 07.10.2026.
 *
 *  Beide Schirme teilen EIN Rahmenmuster: Winkel oben/unten links, dazwischen
 *  die Säule mit den Seitenblöcken (= die bisherigen Tabs), Kopfleiste mit
 *  Uhr und Titel, Fußleiste mit den Messwerten. Rechts das Hauptfeld.
 *
 *  TOP:    KATEGORIEN · RÄUME · SYSTEM            (Kacheln → BOTTOM zeigt die Gruppe)
 *  BOTTOM: UHR · STEUERUNG · TEMP · ENERGIE · BATTERIE · LÜFTUNG · WETTER
 *
 *  Beispieldaten sind fest eingetragen; in der Firmware kommen sie aus HA.
 *  Im Simulator: Säule anklicken = Seite, Titel anklicken = anderer Schirm.    */

#include "osiris_lcars.h"
#include "../../lcars/lcars.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

LV_FONT_DECLARE(ui_font_antonio16);
LV_FONT_DECLARE(ui_font_antonio20);
LV_FONT_DECLARE(ui_font_antonio28);
LV_FONT_DECLARE(ui_font_antonio36);
LV_FONT_DECLARE(ui_font_antonio44);
LV_FONT_DECLARE(ui_font_antonio80);

#define F16 (&ui_font_antonio16)
#define F20 (&ui_font_antonio20)
#define F28 (&ui_font_antonio28)
#define F36 (&ui_font_antonio36)
#define F44 (&ui_font_antonio44)
#define F80 (&ui_font_antonio80)

/* ---------------------------------------------------------------------------
 *  Maße
 * ------------------------------------------------------------------------- */
#define SCR_W     800
#define SCR_H     480
#define GAP       LCARS_GAP
#define SIDE_W    118          /* Säule */
#define TOP_H      38          /* Kopfleiste -- die 36er Uhr muss ganz hineinpassen */
#define BOT_H      26          /* Fußleiste */
#define R_OUT      40
#define R_IN       24
#define ELBOW_W   (SIDE_W + R_IN + 28)
#define ELBOW_TOP (TOP_H + R_IN + 14)
#define ELBOW_BOT (BOT_H + R_IN + 12)
#define MAIN_X    (SIDE_W + 30)
#define MAIN_Y    (TOP_H + 12)
#define MAIN_W    (SCR_W - MAIN_X - 14)
#define MAIN_H    (SCR_H - BOT_H - 10 - MAIN_Y)
#define COL_Y     (ELBOW_TOP + GAP)
#define COL_H     (SCR_H - ELBOW_BOT - GAP - COL_Y)

/* Farbrollen */
#define COL_FRAME   LCARS_ORANGE     /* Winkel */
#define COL_HEAD    LCARS_PEACH      /* Kopfleiste */
#define COL_FOOT    LCARS_TAN        /* Fußleiste */
#define COL_PAGE    LCARS_LAVENDER   /* Seitenblöcke */
#define COL_PAGE_ON LCARS_BUTTER     /* gewählte Seite */
#define COL_DATA    LCARS_ICE        /* Werte auf Schwarz */
#define COL_LABEL   LCARS_BLUE       /* Bezeichner auf Schwarz */
#define COL_ON      LCARS_ORANGE     /* an / aktiv */
#define COL_OFF     LCARS_SKY        /* aus */
#define COL_WARN    LCARS_RED

/* ---------------------------------------------------------------------------
 *  Beispieldaten (Namen aus tools/gen_ha_config.py)
 * ------------------------------------------------------------------------- */
static const struct { const char *name; int total; const char *state; } KAT[] = {
    { "LICHT", 63, "3 AN" },      { "SCHALTER", 72, "2 AN" },   { "KLIMA", 19, "1 HEIZT" },
    { "SENSOR", 88, "-" },        { "FENSTER & TÜREN", 50, "2 OFFEN" }, { "ENERGIE", 14, "1,8 KW" },
    { "KAMERAS", 6, "6 BEREIT" }, { "SKRIPTE", 22, "-" },       { "VENTILATOREN", 5, "1 AN" },
};
#define KAT_N 9
static const struct { const char *name; const char *state; } RAUM[] = {
    { "DACHBODEN", "" },  { "BAD", "1 AN" },      { "ANKLEIDE", "" },    { "BÜRO", "2 AN" },   { "SCHLAFZIMMER", "" },
    { "TREPPE", "" },     { "1. OG", "" },        { "ABSTELLRAUM", "" }, { "FLUR", "1 AN" },   { "KÜCHE", "3 AN" },
    { "WOHNZIMMER", "4 AN" }, { "PANTRY", "" },   { "TOILETTE", "" },    { "EINGANG", "1 OFFEN" }, { "GARTEN", "" },
    { "EINFAHRT", "" },   { "KELLER", "" },       { "HEIZUNG", "" },     { "LABOR", "2 AN" },  { "WASCHKÜCHE", "" },
};
#define RAUM_N 20
static const struct { const char *name; const char *state; bool on; bool dim; } CTRL[] = {
    { "DECKENLAMPE", "AN", true, true }, { "STEHLAMPE", "AUS", false, true }, { "REGAL", "AN", true, false },
    { "AMBIENTE", "AUS", false, true }, { "KÜCHE ARBEITSPLATTE", "AN", true, false }, { "ESSTISCH", "AUS", false, true },
    { "FLUR", "AN", true, false }, { "TREPPE", "AUS", false, false },
};
#define CTRL_N 8
static const struct { const char *name; const char *state; bool on; bool ctl; } FAV[] = {
    { "WOHNZIMMER", "AN", true, true },   { "KÜCHE", "AUS", false, true },     { "AMBIENTE", "AN", true, true },
    { "ROLLO SÜD", "ZU", false, true },   { "KAMIN", "21,5° / 22,0°", false, false }, { "AUSSEN", "12,4 °C", false, false },
    { "GARTEN", "AUS", false, true },     { "EINFAHRT", "AN", true, true },    { "RUNWAY", "AUS", false, true },
    { "TV", "AUS", false, true },         { "PV HEUTE", "8,4 KWH", false, false }, { "HAUS", "1,8 KW", false, false },
};
#define FAV_N 12
static const struct { const char *raum; const char *aktion; uint32_t col; const char *grund; } LUEFT[] = {
    { "SCHLAFZIMMER", "JETZT LÜFTEN", LCARS_ORANGE, "entfeuchtet, kühlt · 21,8 °C · 64 %" },
    { "BAD",          "JETZT LÜFTEN", LCARS_ORANGE, "Schimmelgefahr · 23,1 °C · 78 %" },
    { "WOHNZIMMER",   "GESCHLOSSEN HALTEN", LCARS_SKY, "draußen feuchter · 22,4 °C · 48 %" },
    { "BÜRO",         "FENSTER SCHLIESSEN", LCARS_RED, "offen seit 40 min, kühlt aus · 19,2 °C · 51 %" },
    { "KÜCHE",        "OK", LCARS_MINT, "21,0 °C · 45 %" },
};
#define LUEFT_N 5
static const struct { const char *tag, *lage, *temp, *regen; } FC[] = {
    { "HEUTE", "WOLKIG", "14° / 8°", "0,2 MM" }, { "DO", "REGEN", "11° / 7°", "6,4 MM" },
    { "FR", "BEWÖLKT", "12° / 6°", "1,1 MM" }, { "SA", "SONNIG", "15° / 5°", "0 MM" },
    { "SO", "SONNIG", "17° / 7°", "0 MM" }, { "MO", "WOLKIG", "16° / 9°", "0,5 MM" },
};
#define FC_N 6

/* ---------------------------------------------------------------------------
 *  Zustand
 * ------------------------------------------------------------------------- */
static lv_obj_t *scr[2];
static lv_obj_t *page_obj[2][OSL_BOT_PAGES];
static lv_obj_t *page_btn[2][OSL_BOT_PAGES];
static lv_obj_t *lbl_clock[2], *lbl_big_clock, *lbl_big_date, *lbl_foot_date[2];
static lv_obj_t *lbl_page_title[2];
static int cur_screen, cur_page[2] = { OSL_TOP_KATEGORIEN, OSL_BOT_UHR };
static int sel_tile = -1;
static lv_obj_t *tile_kat[KAT_N], *tile_raum[RAUM_N];
static lv_obj_t *fav_tile[FAV_N], *fav_state[FAV_N];
static bool      fav_on[FAV_N];
static lv_obj_t *ctrl_state[CTRL_N];
static bool      ctrl_on[CTRL_N];
static lv_obj_t *ss_pill[10];
static int       ss_sel = 4;
static lv_obj_t *bri_seg[10], *lbl_bri;
static int       bri_pct = 75;

static const char *TOP_PAGES[OSL_TOP_PAGES] = { "KATEGORIEN", "RÄUME", "SYSTEM" };
static const char *BOT_PAGES[OSL_BOT_PAGES] = { "UHR", "STEUERUNG", "TEMP", "ENERGIE", "BATTERIE", "LÜFTUNG", "WETTER" };
static const char *TOP_TITLE[OSL_TOP_PAGES] = { "KATEGORIEN", "RÄUME", "SYSTEM" };
static const char *BOT_TITLE[OSL_BOT_PAGES] = { "WOHNZIMMER", "LICHT · WOHNZIMMER", "TEMPERATUR 48 H", "LEISTUNG 24 H",
                                                "BATTERIE 24 H", "LÜFTUNG", "WETTER" };

/* ---------------------------------------------------------------------------
 *  Hilfen
 * ------------------------------------------------------------------------- */
static void on_page_clicked(lv_event_t *e)
{
    int code = (int)(intptr_t)lv_event_get_user_data(e);
    osiris_lcars_show(code >> 8, code & 0xFF);
}

static void on_title_clicked(lv_event_t *e)
{
    (void)e;
    osiris_lcars_show(cur_screen ^ 1, -1);
}

/* Beschrifteter Block, Schrift schwarz unten rechts (LCARS-Taste) */
static lv_obj_t *key(lv_obj_t *parent, int32_t w, int32_t h, uint32_t col, const lv_font_t *f,
                     const char *txt, int32_t radius)
{
    lv_obj_t *b = lcars_block(parent, 0, 0, w, h, radius, col);
    lv_obj_set_clickable(b, true);
    lcars_caption(b, f, txt, LV_ALIGN_BOTTOM_RIGHT, -8, -3);
    return b;
}

/* Hauptfeld einer Seite: Flex-Spalte, scrollbar wo nötig.
 * ⚠ Scrollen geht nur, wenn die Berührung ein KLICKBARES Objekt trifft und von
 *   dort aufwärts eine scrollbare Fläche findet — der Baukasten setzt Behälter
 *   auf nicht klickbar, deshalb hier ausdrücklich (07.10.2026 am osiris gelernt). */
static lv_obj_t *page(int s, int p, bool column)
{
    lv_obj_t *o = lcars_box(scr[s], MAIN_X, MAIN_Y, MAIN_W, MAIN_H);
    if (column) {
        lv_obj_set_flex_flow(o, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(o, GAP, 0);
        lv_obj_set_clickable(o, true);
        lv_obj_set_scrollable(o, true);
        lv_obj_set_scroll_dir(o, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_pad_bottom(o, 12, 0);
    }
    lv_obj_set_hidden(o, true);
    page_obj[s][p] = o;
    return o;
}

/* Zeile: Bezeichner (Farbpille) + Wert (Datenfarbe) */
static lv_obj_t *kv(lv_obj_t *parent, int32_t label_w, const char *k, const char *v, uint32_t col)
{
    lv_obj_t *row = lcars_box(parent, 0, 0, lv_pct(100), 26);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 12, 0);
    lv_obj_t *pill = lcars_halfpill(row, label_w, 22, col, true);
    lcars_caption(pill, F16, k, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_t *val = lcars_text(row, F20, v, COL_DATA);
    return val;
}

/* ---------------------------------------------------------------------------
 *  Rahmen (beide Schirme)
 * ------------------------------------------------------------------------- */
static void build_frame(int s, const char *title, const char *const *pages, int n_pages)
{
    lv_obj_t *sc = scr[s];
    lv_obj_set_style_bg_color(sc, lv_color_hex(LCARS_BLACK), 0);
    lv_obj_set_style_bg_opa(sc, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(sc, false);

    lcars_elbow(sc, 0, 0, ELBOW_W, ELBOW_TOP, SIDE_W, TOP_H, R_OUT, R_IN, false, COL_FRAME);
    lcars_elbow(sc, 0, SCR_H - ELBOW_BOT, ELBOW_W, ELBOW_BOT, SIDE_W, BOT_H, R_OUT, R_IN, true, COL_FRAME);

    /* Säule: Seitenblöcke, gleich hoch, füllen die Höhe */
    int h = (COL_H - (n_pages - 1) * GAP) / n_pages;
    for (int i = 0; i < n_pages; i++) {
        lv_obj_t *b = lcars_block(sc, 0, COL_Y + i * (h + GAP), SIDE_W, h, 0, COL_PAGE);
        lv_obj_set_clickable(b, true);
        lcars_caption(b, F16, pages[i], LV_ALIGN_BOTTOM_RIGHT, -8, -4);
        lv_obj_add_event_cb(b, on_page_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)((s << 8) | i));
        page_btn[s][i] = b;
    }

    /* Kopfleiste: Uhr · Balken · Seitentitel · Balken · Schirmtitel · Kappe */
    lv_obj_t *head = lcars_box(sc, ELBOW_W + GAP, 0, SCR_W - ELBOW_W - GAP, TOP_H);
    lv_obj_set_overflow_visible(head, true);
    lv_obj_set_flex_flow(head, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(head, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(head, 10, 0);
    lv_obj_set_style_pad_left(head, 4, 0);
    lbl_clock[s] = lcars_text(head, F36, "--:--", COL_DATA);
    lv_obj_set_style_translate_y(lbl_clock[s], -2, 0);   /* Antonio-Ziffern sitzen tief im Zeilenkasten */
    lv_obj_t *bar = lcars_block(head, 0, 0, 10, TOP_H, 0, COL_HEAD);
    lv_obj_set_flex_grow(bar, 1);
    lbl_page_title[s] = lcars_text(head, F28, "", COL_FRAME);
    lv_obj_t *bar2 = lcars_block(head, 0, 0, 30, TOP_H, 0, COL_HEAD);
    lv_obj_t *t = lcars_text(head, F28, title, COL_HEAD);
    lv_obj_set_clickable(t, true);
    lv_obj_add_event_cb(t, on_title_clicked, LV_EVENT_CLICKED, NULL);
    (void)bar2;
    lcars_cap(head, 36, TOP_H, COL_HEAD);

    /* Fußleiste: Datum · Balken · drei Messfelder · Kappe */
    lv_obj_t *foot = lcars_box(sc, ELBOW_W + GAP, SCR_H - BOT_H, SCR_W - ELBOW_W - GAP, BOT_H);
    lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(foot, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(foot, GAP, 0);
    lbl_foot_date[s] = lcars_text(foot, F20, "", COL_DATA);
    lv_obj_set_style_pad_right(lbl_foot_date[s], 6, 0);
    lv_obj_t *fb = lcars_block(foot, 0, 0, 10, BOT_H, 0, COL_FOOT);
    lv_obj_set_flex_grow(fb, 1);
    /* ⚠ Die Leiste ist knapp: Datum ~190 px + Felder + Kappe muessen in 624 px passen,
     *   sonst schiebt Flex die Endkappe aus dem Bild (07.10.2026 so gemessen). */
    static const char *F[3] = { "IN 22,0 °C · 48 %", "AUS 12,4 °C · 65 %", "1250 LX" };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *f = lcars_block(foot, 0, 0, i == 2 ? 84 : 140, BOT_H, 0, COL_FOOT);
        lcars_caption(f, F16, F[i], LV_ALIGN_RIGHT_MID, i == 2 ? -16 : -8, 0);   /* LX-Feld: mehr Luft zur Kappe (Atlan) */
    }
    lcars_cap(foot, 28, BOT_H, COL_FOOT);
}

/* ---------------------------------------------------------------------------
 *  TOP
 * ------------------------------------------------------------------------- */
static void tile_clicked(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    sel_tile = (sel_tile == idx) ? -1 : idx;
    for (int i = 0; i < KAT_N; i++)  lcars_set_color(tile_kat[i],  sel_tile == i ? COL_PAGE_ON : (i % 2 ? LCARS_PEACH : LCARS_ORANGE));
    for (int i = 0; i < RAUM_N; i++) lcars_set_color(tile_raum[i], sel_tile == 100 + i ? COL_PAGE_ON : (i % 2 ? LCARS_BLUE : LCARS_LAVENDER));
}

static void build_top_kategorien(void)
{
    lv_obj_t *p = page(OSL_TOP, OSL_TOP_KATEGORIEN, false);
    int cols = 3, rows = 3;
    int w = (MAIN_W - (cols - 1) * GAP) / cols, h = (MAIN_H - (rows - 1) * GAP) / rows;
    for (int i = 0; i < KAT_N; i++) {
        lv_obj_t *t = key(p, w, h, i % 2 ? LCARS_PEACH : LCARS_ORANGE, F20, KAT[i].name, 16);
        lv_obj_set_pos(t, (i % cols) * (w + GAP), (i / cols) * (h + GAP));
        char n[8]; snprintf(n, sizeof n, "%d", KAT[i].total);
        lcars_caption(t, F44, n, LV_ALIGN_TOP_LEFT, 10, 2);
        lcars_caption(t, F28, KAT[i].state, LV_ALIGN_LEFT_MID, 10, 14);
        lv_obj_add_event_cb(t, tile_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        tile_kat[i] = t;
    }
}

static void build_top_raeume(void)
{
    lv_obj_t *p = page(OSL_TOP, OSL_TOP_RAEUME, false);
    int cols = 5, rows = 4;
    int w = (MAIN_W - (cols - 1) * GAP) / cols, h = (MAIN_H - (rows - 1) * GAP) / rows;
    for (int i = 0; i < RAUM_N; i++) {
        lv_obj_t *t = key(p, w, h, i % 2 ? LCARS_BLUE : LCARS_LAVENDER, F16, RAUM[i].name, 12);
        lv_obj_set_pos(t, (i % cols) * (w + GAP), (i / cols) * (h + GAP));
        if (RAUM[i].state[0]) lcars_caption(t, F28, RAUM[i].state, LV_ALIGN_TOP_LEFT, 8, 2);
        lv_obj_add_event_cb(t, tile_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)(100 + i));
        tile_raum[i] = t;
    }
}

static void ss_clicked(lv_event_t *e)
{
    ss_sel = (int)(intptr_t)lv_event_get_user_data(e);
    for (int i = 0; i < 10; i++) lcars_set_color(ss_pill[i], i == ss_sel ? COL_PAGE_ON : LCARS_LAVENDER);
}

static void bri_show(void)
{
    for (int i = 0; i < 10; i++) lcars_set_color(bri_seg[i], (i + 1) * 10 <= bri_pct ? LCARS_ORANGE : LCARS_DIM);
    char b[8]; snprintf(b, sizeof b, "%d %%", bri_pct);
    lv_label_set_text(lbl_bri, b);
}

static void bri_clicked(lv_event_t *e)
{
    bri_pct = ((int)(intptr_t)lv_event_get_user_data(e) + 1) * 10;
    bri_show();
}

static void build_top_system(void)
{
    lv_obj_t *p = page(OSL_TOP, OSL_TOP_SYSTEM, false);
    int colw = (MAIN_W - 20) / 2;

    /* links: Status als Liste */
    lv_obj_t *l = lcars_box(p, 0, 0, colw, MAIN_H);
    lv_obj_set_flex_flow(l, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(l, 3, 0);
    lcars_text(l, F20, "SYSTEM", COL_LABEL);
    kv(l, 86, "IP", "10.10.30.55", LCARS_LAVENDER);
    kv(l, 86, "RSSI", "-52 DBM", LCARS_LAVENDER);
    kv(l, 86, "HEAP", "INT 41 KB · PSRAM 5,3 MB", LCARS_LAVENDER);
    kv(l, 86, "BOTTOM", "ONLINE", LCARS_MINT);
    kv(l, 86, "FIRMWARE", "9F3A21C · 06.10.2026", LCARS_LAVENDER);
    lv_obj_t *radar = lcars_text(l, F20, "RADAR", COL_LABEL);
    lv_obj_set_style_margin_top(radar, 6, 0);
    kv(l, 86, "PRÄSENZ", "ANWESEND", LCARS_ORANGE);
    kv(l, 86, "BEWEGUNG", "45 CM · E 72", LCARS_BLUE);
    kv(l, 86, "STATISCH", "-", LCARS_BLUE);
    kv(l, 86, "SCHONER", "IN 58:30", LCARS_BLUE);
    lv_obj_t *refresh = lcars_pill(l, 180, 30, LCARS_PEACH, F16, "HA AKTUALISIEREN");
    lv_obj_set_style_margin_top(refresh, 8, 0);

    /* rechts: Helligkeit, Schoner, Präsenzzone */
    lv_obj_t *r = lcars_box(p, colw + 20, 0, colw, MAIN_H);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(r, 6, 0);
    lv_obj_t *hrow = lcars_box(r, 0, 0, lv_pct(100), 24);
    lv_obj_set_flex_flow(hrow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hrow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lcars_text(hrow, F20, "HELLIGKEIT", COL_LABEL);
    lbl_bri = lcars_text(hrow, F20, "", COL_DATA);
    lv_obj_t *segs = lcars_box(r, 0, 0, lv_pct(100), 34);
    lv_obj_set_flex_flow(segs, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(segs, 3, 0);
    int sw = (colw - 70 - 9 * 3) / 10;
    for (int i = 0; i < 10; i++) {
        bri_seg[i] = lcars_block(segs, 0, 0, sw, 34, 0, LCARS_DIM);
        lv_obj_set_clickable(bri_seg[i], true);
        lv_obj_add_event_cb(bri_seg[i], bri_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
    lv_obj_t *autop = lcars_pill(segs, 60, 34, LCARS_ORANGE, F16, "AUTO");
    lv_obj_set_style_margin_left(autop, 4, 0);
    bri_show();

    lv_obj_t *ssl = lcars_text(r, F20, "BILDSCHIRMSCHONER · MIN", COL_LABEL);
    lv_obj_set_style_margin_top(ssl, 12, 0);
    static const char *SS[10] = { "1", "5", "10", "30", "60", "90", "120", "180", "240", "OO" };
    for (int row = 0; row < 2; row++) {
        lv_obj_t *pr = lcars_box(r, 0, 0, lv_pct(100), 30);
        lv_obj_set_flex_flow(pr, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(pr, GAP, 0);
        for (int i = 0; i < 5; i++) {
            int k = row * 5 + i;
            ss_pill[k] = lcars_pill(pr, (colw - 4 * GAP) / 5, 30, k == ss_sel ? COL_PAGE_ON : LCARS_LAVENDER,
                                    F20, k == 9 ? "OO" : SS[k]);
            lv_obj_add_event_cb(ss_pill[k], ss_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)k);
        }
    }

    lv_obj_t *pzl = lcars_text(r, F20, "PRÄSENZZONE", COL_LABEL);
    lv_obj_set_style_margin_top(pzl, 12, 0);
    lv_obj_t *pz = lcars_box(r, 0, 0, lv_pct(100), 34);
    lv_obj_set_flex_flow(pz, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pz, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(pz, 10, 0);
    lcars_pill(pz, 60, 34, LCARS_BLUE, F28, "-");
    lcars_text(pz, F28, "75 CM", COL_DATA);
    lcars_pill(pz, 60, 34, LCARS_BLUE, F28, "+");
}

/* ---------------------------------------------------------------------------
 *  BOTTOM
 * ------------------------------------------------------------------------- */
static void fav_clicked(lv_event_t *e)
{
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    if (!FAV[i].ctl) return;
    fav_on[i] = !fav_on[i];
    lcars_set_color(fav_tile[i], fav_on[i] ? COL_ON : COL_OFF);
    lv_label_set_text(fav_state[i], fav_on[i] ? "AN" : "AUS");
}

static void build_bot_uhr(void)
{
    lv_obj_t *p = page(OSL_BOTTOM, OSL_BOT_UHR, false);
    /* Uhrband oben: Zeit groß, rechts Datum und Matrix-Stellung */
    int band = 120;
    lbl_big_clock = lcars_text(p, F80, "--:--", COL_DATA);
    lv_obj_set_pos(lbl_big_clock, 0, 4);
    lv_obj_t *right = lcars_box(p, 300, 0, MAIN_W - 300, band);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_row(right, 6, 0);
    lbl_big_date = lcars_text(right, F28, "", COL_LABEL);
    lv_obj_t *mrow = lcars_box(right, 0, 0, LV_SIZE_CONTENT, 30);
    lv_obj_set_flex_flow(mrow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mrow, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(mrow, GAP, 0);
    lv_obj_t *ml = lcars_halfpill(mrow, 90, 28, LCARS_LAVENDER, true);
    lcars_caption(ml, F16, "MATRIX", LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_t *mv = lcars_halfpill(mrow, 130, 28, LCARS_PEACH, false);
    lcars_caption(mv, F20, "AUTOMATIK", LV_ALIGN_LEFT_MID, 10, 0);

    /* Favoriten: 3 Reihen × 4 */
    int cols = 4, rows = 3, y0 = band + GAP;
    int w = (MAIN_W - (cols - 1) * GAP) / cols, h = (MAIN_H - y0 - (rows - 1) * GAP) / rows;
    for (int i = 0; i < FAV_N; i++) {
        fav_on[i] = FAV[i].on;
        uint32_t col = FAV[i].ctl ? (FAV[i].on ? COL_ON : COL_OFF) : LCARS_LAVENDER;
        lv_obj_t *t = key(p, w, h, col, F16, FAV[i].name, 12);
        lv_obj_set_pos(t, (i % cols) * (w + GAP), y0 + (i / cols) * (h + GAP));
        fav_state[i] = lcars_caption(t, F28, FAV[i].state, LV_ALIGN_TOP_LEFT, 8, 2);
        lv_obj_add_event_cb(t, fav_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        fav_tile[i] = t;
    }
}

static void ctrl_clicked(lv_event_t *e)
{
    int code = (int)(intptr_t)lv_event_get_user_data(e);
    int i = code >> 1; bool on = code & 1;
    ctrl_on[i] = on;
    lcars_set_color(lv_obj_get_parent(ctrl_state[i]), on ? COL_ON : COL_OFF);
    lv_label_set_text(ctrl_state[i], on ? "AN" : "AUS");
}

static void build_bot_steuerung(void)
{
    lv_obj_t *p = page(OSL_BOTTOM, OSL_BOT_STEUERUNG, true);
    for (int i = 0; i < CTRL_N; i++) {
        ctrl_on[i] = CTRL[i].on;
        lv_obj_t *row = lcars_box(p, 0, 0, lv_pct(100), 42);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, GAP, 0);
        /* Name auf Schwarz, dahinter Füllbalken bis zu den Tasten */
        lv_obj_t *nm = lcars_text(row, F20, CTRL[i].name, COL_DATA);
        lv_obj_set_style_pad_right(nm, 6, 0);
        lv_obj_t *fill = lcars_block(row, 0, 0, 10, 14, 0, LCARS_DIM);
        lv_obj_set_flex_grow(fill, 1);
        lv_obj_t *st = lcars_block(row, 0, 0, 64, 34, 0, ctrl_on[i] ? COL_ON : COL_OFF);
        ctrl_state[i] = lcars_caption(st, F20, CTRL[i].state, LV_ALIGN_CENTER, 0, 0);
        if (CTRL[i].dim) {
            lv_obj_t *b = lcars_block(row, 0, 0, 64, 34, 0, LCARS_LAVENDER);
            lcars_caption(b, F20, "75 %", LV_ALIGN_CENTER, 0, 0);
            lcars_pill(row, 44, 34, LCARS_BLUE, F28, "-");
            lcars_pill(row, 44, 34, LCARS_BLUE, F28, "+");
        }
        lv_obj_t *off = lcars_halfpill(row, 64, 34, LCARS_PEACH, true);
        lcars_caption(off, F20, "AUS", LV_ALIGN_CENTER, 4, 0);
        lv_obj_set_clickable(off, true);
        lv_obj_add_event_cb(off, ctrl_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)(i << 1));
        lv_obj_t *on = lcars_halfpill(row, 64, 34, LCARS_ORANGE, false);
        lcars_caption(on, F20, "AN", LV_ALIGN_CENTER, -4, 0);
        lv_obj_set_clickable(on, true);
        lv_obj_add_event_cb(on, ctrl_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)((i << 1) | 1));
    }
}

/* Diagramm mit LCARS-Achsen: Beschriftung links in Datenfarbe, Legende als Pillen */
static lv_obj_t *chart(lv_obj_t *p, int32_t ymin, int32_t ymax, int ticks, const char *unit,
                       const char *const *legend, const uint32_t *lcol, int nleg)
{
    lv_obj_t *top = lcars_box(p, 0, 0, MAIN_W, 30);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(top, GAP, 0);
    for (int i = 0; i < nleg; i++) lcars_pill(top, 110, 24, lcol[i], F16, legend[i]);

    int axis_w = 56, ch_y = 34, ch_h = MAIN_H - ch_y - 26;
    lv_obj_t *ch = lv_chart_create(p);
    lv_obj_remove_style_all(ch);
    lv_obj_set_pos(ch, axis_w, ch_y);
    lv_obj_set_size(ch, MAIN_W - axis_w, ch_h);
    lv_obj_set_style_bg_opa(ch, LV_OPA_TRANSP, 0);
    lv_obj_set_style_line_color(ch, lv_color_hex(0x333355), 0);
    lv_obj_set_style_line_width(ch, 1, 0);
    lv_obj_set_style_line_width(ch, 3, LV_PART_ITEMS);
    lv_obj_set_style_size(ch, 0, 0, LV_PART_INDICATOR);
    lv_chart_set_type(ch, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(ch, ticks + 1, 7);
    lv_chart_set_axis_range(ch, LV_CHART_AXIS_PRIMARY_Y, ymin, ymax);
    for (int i = 0; i <= ticks; i++) {
        char t[12]; snprintf(t, sizeof t, "%d%s", (int)(ymax - (ymax - ymin) * i / ticks), unit);
        lv_obj_t *l = lcars_text(p, F16, t, COL_LABEL);
        lv_obj_set_pos(l, 0, ch_y + ch_h * i / ticks - 8);
        lv_obj_set_width(l, axis_w - 8);
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_RIGHT, 0);
    }
    static const char *T[7] = { "00", "04", "08", "12", "16", "20", "24" };
    for (int i = 0; i < 7; i++) {
        lv_obj_t *l = lcars_text(p, F16, T[i], COL_LABEL);
        lv_obj_set_pos(l, axis_w + (MAIN_W - axis_w) * i / 6 - (i == 6 ? 18 : 0), ch_y + ch_h + 6);
    }
    return ch;
}

static void series(lv_obj_t *ch, uint32_t col, int n, int base, int amp, int phase)
{
    lv_chart_set_point_count(ch, n);
    lv_chart_series_t *s = lv_chart_add_series(ch, lv_color_hex(col), LV_CHART_AXIS_PRIMARY_Y);
    for (int i = 0; i < n; i++) {
        /* ruhige Pseudo-Kurve: Sinus + kleine Welle */
        int32_t v = base + (int32_t)(amp * lv_trigo_sin((i * 360 / n + phase) % 360) / 32767)
                    + (int32_t)(amp / 4 * lv_trigo_sin((i * 1440 / n) % 360) / 32767);
        lv_chart_set_next_value(ch, s, v);
    }
}

static void build_bot_temp(void)
{
    lv_obj_t *p = page(OSL_BOTTOM, OSL_BOT_TEMP, false);
    static const char *L[3] = { "AUSSEN", "WOHNZIMMER", "LABOR" };
    static const uint32_t C3[3] = { LCARS_ICE, LCARS_ORANGE, LCARS_LAVENDER };
    lv_obj_t *ch = chart(p, -10, 40, 5, "°", L, C3, 3);
    lv_chart_set_axis_range(ch, LV_CHART_AXIS_PRIMARY_Y, -100, 400);   /* Zehntelgrad */
    series(ch, LCARS_ICE, 96, 100, 70, 240);
    series(ch, LCARS_ORANGE, 96, 220, 12, 200);
    series(ch, LCARS_LAVENDER, 96, 205, 25, 160);
}

static void build_bot_energie(void)
{
    lv_obj_t *p = page(OSL_BOTTOM, OSL_BOT_ENERGIE, false);
    static const char *L[3] = { "HAUS", "SOLAR", "NETZ" };
    static const uint32_t C3[3] = { LCARS_RED, LCARS_BUTTER, LCARS_ICE };
    lv_obj_t *ch = chart(p, 0, 3000, 6, " W", L, C3, 3);
    series(ch, LCARS_RED, 96, 900, 600, 300);
    series(ch, LCARS_BUTTER, 96, 1200, 1200, 270);
    series(ch, LCARS_ICE, 96, 400, 300, 30);
}

static void build_bot_batterie(void)
{
    lv_obj_t *p = page(OSL_BOTTOM, OSL_BOT_BATTERIE, false);
    static const char *L[1] = { "SOC" };
    static const uint32_t C1[1] = { LCARS_MINT };
    lv_obj_t *ch = chart(p, 0, 100, 5, " %", L, C1, 1);
    series(ch, LCARS_MINT, 96, 55, 40, 250);
}

static void build_bot_lueftung(void)
{
    lv_obj_t *p = page(OSL_BOTTOM, OSL_BOT_LUEFTUNG, true);
    lcars_text(p, F20, "2 RÄUME LÜFTEN · AUSSEN 12,4 °C · 7,1 G/M3", COL_LABEL);
    lv_obj_t *tip = lcars_block(p, 0, 0, lv_pct(100), 30, 15, LCARS_ICE);
    lcars_caption(tip, F16, "QUERLÜFTEN: SCHLAFZIMMER UND BAD ZUSAMMEN ÖFFNEN, FLURTÜR AUF", LV_ALIGN_LEFT_MID, 14, 0);
    for (int i = 0; i < LUEFT_N; i++) {
        lv_obj_t *row = lcars_box(p, 0, 0, lv_pct(100), 54);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 10, 0);
        lv_obj_t *act = lcars_halfpill(row, 170, 44, LUEFT[i].col, true);
        lcars_caption(act, F16, LUEFT[i].aktion, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_t *txt = lcars_box(row, 0, 0, 10, 44);
        lv_obj_set_flex_grow(txt, 1);
        lv_obj_set_flex_flow(txt, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(txt, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lcars_text(txt, F20, LUEFT[i].raum, COL_DATA);
        lcars_text(txt, F16, LUEFT[i].grund, LCARS_BLUE);
        lcars_block(row, 0, 0, 24, 44, 0, LUEFT[i].col);
    }
}

static void build_bot_wetter(void)
{
    lv_obj_t *p = page(OSL_BOTTOM, OSL_BOT_WETTER, true);
    /* Trend */
    lv_obj_t *trend = lcars_box(p, 0, 0, lv_pct(100), 74);
    lv_obj_set_flex_flow(trend, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(trend, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(trend, 12, 0);
    lv_obj_t *z = lcars_halfpill(trend, 110, 64, LCARS_PEACH, true);
    lcars_caption(z, F16, "ZAMBRETTI", LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_t *zt = lcars_box(trend, 0, 0, 10, 64);
    lv_obj_set_flex_grow(zt, 1);
    lv_obj_set_flex_flow(zt, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(zt, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lcars_text(zt, F28, "WECHSELHAFT, SPÄTER REGEN", COL_DATA);
    lv_obj_t *sg = lcars_text(zt, F16, "SAGER: NIEDERSCHLAG WAHRSCHEINLICH · 6 H BESSERND", LCARS_BLUE);
    lv_obj_set_width(sg, lv_pct(100));
    lv_label_set_long_mode(sg, LV_LABEL_LONG_CLIP);  /* eine Zeile, notfalls abgeschnitten -- nie umbrechen */
    lv_obj_t *pp = lcars_halfpill(trend, 90, 64, LCARS_ICE, false);
    lcars_caption(pp, F28, "60 %", LV_ALIGN_LEFT_MID, 10, 0);

    /* Live-Werte als Pillen */
    lv_obj_t *live = lcars_box(p, 0, 0, lv_pct(100), 30);
    lv_obj_set_flex_flow(live, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(live, GAP, 0);
    static const char *LV_[6] = { "1013 HPA  -1,8", "12,4 °C", "65 %", "TAU 6,1 °C", "SW 14 KM/H", "BÖEN 31" };
    for (int i = 0; i < 6; i++) {
        lv_obj_t *c = lcars_block(live, 0, 0, (MAIN_W - 5 * GAP) / 6, 30, 0, i % 2 ? LCARS_LAVENDER : LCARS_BLUE);
        lcars_caption(c, F16, LV_[i], LV_ALIGN_CENTER, 0, 0);
    }

    /* Vorhersage */
    lcars_text(p, F20, "VORHERSAGE", COL_LABEL);
    for (int i = 0; i < FC_N; i++) {
        lv_obj_t *row = lcars_box(p, 0, 0, lv_pct(100), 32);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 10, 0);
        lv_obj_t *d = lcars_halfpill(row, 84, 28, i == 0 ? LCARS_ORANGE : LCARS_PEACH, true);
        lcars_caption(d, F16, FC[i].tag, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_t *lage = lcars_text(row, F20, FC[i].lage, COL_DATA);
        lv_obj_set_flex_grow(lage, 1);
        lv_obj_t *tp = lcars_text(row, F20, FC[i].temp, COL_DATA);
        lv_obj_set_width(tp, 100);
        lv_obj_set_style_text_align(tp, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_t *rg = lcars_text(row, F20, FC[i].regen, LCARS_ICE);
        lv_obj_set_width(rg, 80);
        lv_obj_set_style_text_align(rg, LV_TEXT_ALIGN_RIGHT, 0);
        lcars_block(row, 0, 0, 24, 28, 0, i == 0 ? LCARS_ORANGE : LCARS_PEACH);
    }
}

/* ---------------------------------------------------------------------------
 *  Schnittstelle
 * ------------------------------------------------------------------------- */
void osiris_lcars_show(int screen, int p)
{
    if (screen < 0 || screen > 1) return;
    cur_screen = screen;
    int n = screen == OSL_TOP ? OSL_TOP_PAGES : OSL_BOT_PAGES;
    if (p >= 0 && p < n) cur_page[screen] = p;
    p = cur_page[screen];
    for (int i = 0; i < n; i++) {
        lv_obj_set_hidden(page_obj[screen][i], i != p);
        lcars_set_color(page_btn[screen][i], i == p ? COL_PAGE_ON : COL_PAGE);
    }
    lv_label_set_text(lbl_page_title[screen], screen == OSL_TOP ? TOP_TITLE[p] : BOT_TITLE[p]);
    lv_obj_set_hidden(lbl_clock[OSL_BOTTOM], screen == OSL_BOTTOM && p == OSL_BOT_UHR);   /* grosse Uhr reicht */
    if (lv_screen_active() != scr[screen]) lv_screen_load(scr[screen]);
}

int osiris_lcars_screen(void) { return cur_screen; }
int osiris_lcars_page(void)   { return cur_page[cur_screen]; }

bool osiris_lcars_init(void)
{
    for (int s = 0; s < 2; s++) scr[s] = lv_obj_create(NULL);
    build_frame(OSL_TOP, "OSIRIS", TOP_PAGES, OSL_TOP_PAGES);
    build_frame(OSL_BOTTOM, "OSIRIS", BOT_PAGES, OSL_BOT_PAGES);
    build_top_kategorien();
    build_top_raeume();
    build_top_system();
    build_bot_uhr();
    build_bot_steuerung();
    build_bot_temp();
    build_bot_energie();
    build_bot_batterie();
    build_bot_lueftung();
    build_bot_wetter();
    osiris_lcars_show(OSL_TOP, OSL_TOP_KATEGORIEN);
    osiris_lcars_tick();
    return true;
}

void osiris_lcars_tick(void)
{
    static int last_min = -1;
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    if (tm.tm_min == last_min) return;
    last_min = tm.tm_min;
    char hm[8], dt[32];
    snprintf(hm, sizeof hm, "%02d:%02d", tm.tm_hour, tm.tm_min);
    static const char *WD[7] = { "SONNTAG", "MONTAG", "DIENSTAG", "MITTWOCH", "DONNERSTAG", "FREITAG", "SAMSTAG" };
    snprintf(dt, sizeof dt, "%s · %02d.%02d.%04d", WD[tm.tm_wday], tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
    for (int s = 0; s < 2; s++) { lv_label_set_text(lbl_clock[s], hm); lv_label_set_text(lbl_foot_date[s], dt); }
    lv_label_set_text(lbl_big_clock, hm);
    lv_label_set_text(lbl_big_date, dt);
}
