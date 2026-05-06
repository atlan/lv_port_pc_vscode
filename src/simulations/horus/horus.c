#include "horus.h"
#include "src/glyphs/horus_icons/horus_icons.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "lvgl/lvgl.h"

LV_FONT_DECLARE(ui_font_star);
LV_FONT_DECLARE(ui_font_ver30);

static uint32_t COL_WHITE    = 0xFFFFFF;
static uint32_t COL_GREEN    = 0x00CC44;
static uint32_t COL_ORANGE   = 0xFF8800;
static uint32_t COL_RED      = 0xFF2222;
static uint32_t COL_BLUE     = 0x00AAFF;
static uint32_t COL_GRAY     = 0x888888;
static uint32_t COL_DARKGRAY = 0x333333;
static uint32_t COL_BLACK    = 0x000000;

#define C(hex) lv_color_hex(hex)

static lv_obj_t *ui_lbl_status_msg = NULL;
static lv_obj_t *ui_bt_off = NULL;
static lv_obj_t *ui_bt_away = NULL;
static lv_obj_t *ui_bt_vacation = NULL;
static lv_obj_t *ui_bt_home_a = NULL;
static lv_obj_t *ui_bt_home_b = NULL;
static lv_obj_t *ui_bt_custom = NULL;

static lv_style_t style_header_footer;
static lv_style_t style_button;
static lv_style_t style_button_pressed;
static lv_style_t style_button_checked;
static lv_style_t style_tabview;
static lv_style_t style_kb_main;
static lv_style_t style_kb_btn;
static lv_style_t style_kb_btn_pressed;
static lv_style_t style_kb_btn_disabled;
static lv_obj_t *ta;
static lv_obj_t *kb;

lv_obj_t *cont_main;
lv_obj_t *cont_wait;

static lv_timer_t *status_timer = NULL;
static char *status_msg_copy = NULL;
static bool s_wake_requested = false;

static lv_obj_t *cont_countdown;
static lv_obj_t *lbl_countdown_num;
static lv_obj_t *lbl_countdown_info;
static lv_timer_t *countdown_timer;
static int countdown_secs = 0;
static bool countdown_is_entry = false;

static uint16_t btn_id_x = LV_BTNMATRIX_BTN_NONE;
static uint16_t btn_id_dummy_left = LV_BTNMATRIX_BTN_NONE;

static lv_obj_t *btn_map[] = {
    NULL, // ui_bt_off
    NULL, // ui_bt_away
    NULL, // ui_bt_vacation
    NULL, // ui_bt_home_a
    NULL, // ui_bt_home_b
    NULL  // ui_bt_custom
};

/* Map: 4 Zeilen + abschließender Terminator "" */
static const char *kb_map_num[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    " ", "0", "X", "", // Dummy links/rechts
    ""                 // Terminator
};

/* ctrl_map: gleicher „Fluss“ wie map[], aber OHNE das letzte "".
   Danach EIN abschließendes LV_BTNMATRIX_CTRL_HIDDEN. */
static const lv_btnmatrix_ctrl_t kb_ctrl_num[] = {
    // "1","2","3","\n"
    0, 0, 0, 0,

    // "4","5","6","\n"
    0, 0, 0, 0,

    // "7","8","9","\n"
    0, 0, 0, 0,

    // "","0","X",""
    0, 0, 0, 0, // leere Taste links, 0, X, leere Taste rechts

    LV_BTNMATRIX_CTRL_HIDDEN // Abschluss
};

static uint16_t find_button_id_for_text(lv_obj_t *kb, const char *needle)
{
    const char *const *map = lv_btnmatrix_get_map(kb);
    uint16_t btn_id = 0;

    for (uint16_t i = 0; map[i][0] != '\0'; i++)
    {
        if (map[i][0] == '\n')
        {
            continue; // kein Button, btn_id nicht erhöhen
        }

        if (strcmp(map[i], needle) == 0)
        {
            return btn_id; // das ist die Button-ID
        }

        btn_id++; // nächster Button
    }

    return LV_BTNMATRIX_BTN_NONE;
}

static void kb_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_user_data(e);

    if (code != LV_EVENT_VALUE_CHANGED)
        return;

    const char *txt = lv_btnmatrix_get_btn_text(obj,
                                                lv_btnmatrix_get_selected_btn(obj));
    if (txt == NULL)
        return;

    if (strcmp(txt, "X") == 0)
    {
        lv_textarea_set_text(ta, "");
    }

    // Nach dem Update Text prüfen und X aktiv/deaktivieren
    const char *ta_txt = lv_textarea_get_text(ta);
    bool empty = (!ta_txt || ta_txt[0] == '\0');

    if (btn_id_x != LV_BTNMATRIX_BTN_NONE)
    {
        if (empty)
            lv_btnmatrix_set_btn_ctrl(obj, btn_id_x, LV_BTNMATRIX_CTRL_DISABLED);
        else
            lv_btnmatrix_clear_btn_ctrl(obj, btn_id_x, LV_BTNMATRIX_CTRL_DISABLED);
    }
}

static void single_select_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED)
        return;

    lv_obj_t *btn = lv_event_get_target(e);

    for (int i = 0; i < (int)6U; i++)
    {
        if (btn_map[i] == btn)
        {
            const char *code_txt = lv_textarea_get_text(ta);
            const char *code_arg = (code_txt && code_txt[0] != '\0') ? code_txt : NULL;

            lv_textarea_set_text(ta, "");
            if (btn_id_x != LV_BTNMATRIX_BTN_NONE)
                lv_btnmatrix_set_btn_ctrl(kb, btn_id_x, LV_BTNMATRIX_CTRL_DISABLED);
            break;
        }
    }
}

// Must be called with LVGL lock held (runs in LVGL context).
static void wake_display_lv()
{
    // lv_obj_add/clear_flag always calls lv_obj_invalidate even when state unchanged.
    // Guard to prevent a continuous full-screen dirty area when already awake.
    if (!lv_obj_has_flag(cont_wait, LV_OBJ_FLAG_HIDDEN))
        lv_obj_add_flag(cont_wait, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_has_flag(cont_main, LV_OBJ_FLAG_HIDDEN))
        lv_obj_clear_flag(cont_main, LV_OBJ_FLAG_HIDDEN);
    lv_disp_trig_activity(NULL);
    //board_backlight_set(true);
}

static void cont_wait_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        wake_display_lv();
    }
}

void apply_style(void)
{
    /* Tabview style */
    lv_style_init(&style_tabview);
    lv_style_set_bg_opa(&style_tabview, LV_OPA_TRANSP);
    lv_style_set_border_color(&style_tabview, (lv_color_t)lv_color_hex(0x9966CC));
    lv_style_set_border_width(&style_tabview, 0);
    lv_style_set_text_color(&style_tabview, (lv_color_t)lv_color_hex(0xFFFFFF));
    lv_style_set_pad_all(&style_tabview, 0);

    /* Button base style */
    lv_style_init(&style_button);
    lv_style_set_bg_color(&style_button, (lv_color_t)lv_color_hex(0xB19CD9));
    lv_style_set_bg_grad_color(&style_button, (lv_color_t)lv_color_hex(0x7D4CDB));
    lv_style_set_bg_grad_dir(&style_button, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_button, LV_OPA_COVER);
    lv_style_set_border_color(&style_button, (lv_color_t)lv_color_hex(0x9966CC));
    lv_style_set_border_width(&style_button, 1);
    lv_style_set_shadow_spread(&style_button, 2);
    lv_style_set_shadow_color(&style_button, (lv_color_t)lv_color_hex(0xFFFFFF));
    lv_style_set_text_color(&style_button, (lv_color_t)lv_color_hex(0xFFFFFF));

    /* Button pressed (separate style) */
    lv_style_init(&style_button_pressed);
    lv_style_set_bg_color(&style_button_pressed, (lv_color_t)lv_color_hex(0x8A6BC7));
    lv_style_set_bg_grad_color(&style_button_pressed, (lv_color_t)lv_color_hex(0x5B2C87));

    /* Button checked (separate style) */
    lv_style_init(&style_button_checked);
    lv_style_set_bg_color(&style_button_checked, (lv_color_t)lv_color_hex(0x7D4CDB));
    lv_style_set_bg_grad_color(&style_button_checked, (lv_color_t)lv_color_hex(0x4A1A5C));
    lv_style_set_text_color(&style_button_checked, (lv_color_t)lv_color_hex(0xFFF300));

    /* Header/Footer style */
    lv_style_init(&style_header_footer);
    lv_style_set_bg_color(&style_header_footer, (lv_color_t)lv_color_hex(0xB19CD9));
    lv_style_set_bg_grad_color(&style_header_footer, (lv_color_t)lv_color_hex(0x7D4CDB));
    lv_style_set_bg_grad_dir(&style_header_footer, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_header_footer, LV_OPA_COVER);
    lv_style_set_border_opa(&style_header_footer, LV_OPA_TRANSP);
    lv_style_set_radius(&style_header_footer, 0);
    lv_style_set_pad_all(&style_header_footer, 0);
    lv_style_set_pad_row(&style_header_footer, 0);
    lv_style_set_pad_column(&style_header_footer, 0);
    lv_style_set_border_color(&style_header_footer, (lv_color_t)lv_color_hex(0x9966CC));
    lv_style_set_text_color(&style_header_footer, (lv_color_t)lv_color_hex(0xFFFFFF));

    lv_style_init(&style_kb_main);
    lv_style_set_bg_opa(&style_kb_main, LV_OPA_TRANSP); // Hintergrund komplett transparent
    lv_style_set_border_width(&style_kb_main, 0);
    lv_style_set_pad_all(&style_kb_main, 10);

    lv_style_init(&style_kb_btn);
    lv_style_set_radius(&style_kb_btn, LV_RADIUS_CIRCLE); // runde Tasten
    lv_style_set_bg_color(&style_kb_btn, lv_color_white());
    lv_style_set_bg_opa(&style_kb_btn, LV_OPA_40); // halbtransparent
    lv_style_set_border_width(&style_kb_btn, 0);
    lv_style_set_text_color(&style_kb_btn, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_kb_btn, &ui_font_star);
    lv_style_set_pad_all(&style_kb_btn, 5);
    lv_style_set_pad_gap(&style_kb_btn, 8); // Abstand zwischen Tasten

    lv_style_init(&style_kb_btn_pressed);
    lv_style_set_radius(&style_kb_btn_pressed, LV_RADIUS_CIRCLE); // runde Tasten
    lv_style_set_bg_color(&style_kb_btn_pressed, lv_color_hex(0x7D4CDB));
    lv_style_set_bg_opa(&style_kb_btn_pressed, LV_OPA_60); // weniger transparent bei gedrückt
    lv_style_set_border_width(&style_kb_btn_pressed, 0);
    lv_style_set_text_color(&style_kb_btn_pressed, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_kb_btn_pressed, &ui_font_star);
    lv_style_set_pad_all(&style_kb_btn_pressed, 5);
    lv_style_set_pad_gap(&style_kb_btn_pressed, 8); // Abstand zwischen Tasten

    lv_style_init(&style_kb_btn_disabled);
    lv_style_set_radius(&style_kb_btn_disabled, LV_RADIUS_CIRCLE); // runde Tasten
    lv_style_set_bg_color(&style_kb_btn_disabled, lv_color_white());
    lv_style_set_bg_opa(&style_kb_btn_disabled, LV_OPA_20); // halbtransparent
    lv_style_set_border_width(&style_kb_btn_disabled, 0);
    lv_style_set_text_color(&style_kb_btn_disabled, lv_color_hex(0x333333));
    lv_style_set_text_font(&style_kb_btn_disabled, &ui_font_star);
    lv_style_set_pad_all(&style_kb_btn_disabled, 5);
    lv_style_set_pad_gap(&style_kb_btn_disabled, 8);

    /* Apply global background color to active screen */
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, (lv_color_t)lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* Add tabview main style to screen so tabview children can inherit if desired */
    lv_obj_add_style(scr, &style_tabview, LV_PART_MAIN);
}

bool horus_init(void)
{
    apply_style();

    lv_obj_t *scr = lv_scr_act();

    cont_wait = lv_obj_create(scr);
    lv_obj_set_pos(cont_wait, 0, 0);
    lv_obj_set_size(cont_wait, lv_pct(100), lv_pct(100));
    lv_obj_add_style(cont_wait, &style_header_footer, LV_PART_MAIN);
    lv_obj_clear_flag(cont_wait, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cont_wait, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cont_wait, cont_wait_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(cont_wait, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *ui_img_horus = lv_img_create(cont_wait);
    lv_img_set_src(ui_img_horus, &horus);
    lv_obj_align(ui_img_horus, LV_ALIGN_TOP_RIGHT, -50, -10);

    lv_obj_t *ui_wait_lbl_top = lv_label_create(cont_wait);
    lv_obj_set_align(ui_wait_lbl_top, LV_ALIGN_CENTER);
    lv_obj_align(ui_wait_lbl_top, LV_ALIGN_CENTER, -150, -150);
    lv_label_set_text(ui_wait_lbl_top, "Horus Perimeter");
    lv_obj_set_style_text_font(ui_wait_lbl_top, &ui_font_star, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_wait_lbl_top, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    cont_main = lv_obj_create(scr);
    lv_obj_set_pos(cont_main, 0, 0);
    lv_obj_set_size(cont_main, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(cont_main, C(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_main, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(cont_main, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(cont_main, &style_header_footer, LV_PART_MAIN);
    
    lv_obj_t *cont_top_line = lv_obj_create(cont_main);
    lv_obj_set_size(cont_top_line, 1023, 60);
    lv_obj_set_pos(cont_top_line, 0, 0);
    lv_obj_clear_flag(cont_top_line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(cont_top_line, &style_header_footer, LV_PART_MAIN);

    lv_obj_t *ui_lbl_top = lv_label_create(cont_top_line);
    lv_obj_set_align(ui_lbl_top, LV_ALIGN_CENTER);
    lv_obj_align(ui_lbl_top, LV_ALIGN_CENTER, 0, -5);
    lv_label_set_text(ui_lbl_top, "Horus Perimeter");
    lv_obj_set_style_text_font(ui_lbl_top, &ui_font_star, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_lbl_top, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    lv_obj_t *cont_ta_line = lv_obj_create(cont_main);
    lv_obj_set_size(cont_ta_line, 512, 308);
    lv_obj_set_pos(cont_ta_line, 0, 61);
    lv_obj_clear_flag(cont_ta_line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(cont_ta_line, &style_header_footer, LV_PART_MAIN);

    lv_obj_t *cont_home = lv_obj_create(cont_ta_line);
    lv_obj_set_size(cont_home, 190, 100);
    lv_obj_set_pos(cont_home, 10, 10);
    lv_obj_clear_flag(cont_home, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(cont_home, C(COL_WHITE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_home, LV_OPA_0, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(cont_home, LV_OPA_30, 0);
    lv_obj_set_style_border_width(cont_home, 0, 0);
    lv_obj_set_style_pad_all(cont_home, 0, 0);

    lv_obj_t *ui_bt_one = lv_btn_create(cont_home);
    lv_obj_remove_style_all(ui_bt_one);
    lv_obj_set_width(ui_bt_one, 95);
    lv_obj_set_height(ui_bt_one, 100);
    lv_obj_set_x(ui_bt_one, 0);
    lv_obj_set_y(ui_bt_one, 0);
    lv_obj_add_flag(ui_bt_one, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_bt_one, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_bt_one, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui_bt_one, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(ui_bt_one, C(0x7D4CDB), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_bt_one, LV_OPA_0, 0);
    lv_obj_set_style_radius(ui_bt_one, 10, 0);
    lv_obj_add_event_cb(ui_bt_one, single_select_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *ui_bt_one_icon = lv_img_create(ui_bt_one);
    lv_img_set_src(ui_bt_one_icon, &alone);
    lv_obj_set_style_img_recolor(ui_bt_one_icon, C(COL_WHITE), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(ui_bt_one_icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_align(ui_bt_one_icon, LV_ALIGN_CENTER);

    lv_obj_t *ui_bt_pair = lv_btn_create(cont_home);
    lv_obj_remove_style_all(ui_bt_pair);
    lv_obj_set_width(ui_bt_pair, 95);
    lv_obj_set_height(ui_bt_pair, 100);
    lv_obj_set_x(ui_bt_pair, 95);
    lv_obj_set_y(ui_bt_pair, 0);
    lv_obj_add_flag(ui_bt_pair, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_bt_pair, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_bt_pair, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui_bt_pair, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(ui_bt_pair, C(0x7D4CDB), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_bt_pair, LV_OPA_0, 0);
    lv_obj_set_style_radius(ui_bt_pair, 10, 0);
    lv_obj_add_event_cb(ui_bt_pair, single_select_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *ui_bt_pair_icon = lv_img_create(ui_bt_pair);
    lv_img_set_src(ui_bt_pair_icon, &pair);
    lv_obj_set_style_img_recolor(ui_bt_pair_icon, C(COL_WHITE), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(ui_bt_pair_icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_align(ui_bt_pair_icon, LV_ALIGN_CENTER);

    ta = lv_textarea_create(cont_ta_line);
    lv_textarea_set_one_line(ta, true);
    lv_obj_set_width(ta, 240);
    lv_obj_align(ta, LV_ALIGN_TOP_LEFT, 0, 120);
    lv_obj_set_style_text_font(ta, &ui_font_star, LV_PART_MAIN);

    // Cursor optisch unsichtbar machen
    lv_obj_set_style_bg_opa(ta, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_set_style_border_opa(ta, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_set_style_text_opa(ta, LV_OPA_TRANSP, LV_PART_CURSOR);

    ui_lbl_status_msg = lv_label_create(cont_ta_line);
    lv_obj_set_size(ui_lbl_status_msg, 485, 50);
    lv_label_set_text(ui_lbl_status_msg, "");
    lv_obj_set_style_text_font(ui_lbl_status_msg, &ui_font_ver30, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_lbl_status_msg, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(ui_lbl_status_msg, LV_ALIGN_TOP_LEFT, 10, 230);
    lv_label_set_long_mode(ui_lbl_status_msg, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(ui_lbl_status_msg, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_lbl_status_msg, C(COL_GRAY), LV_PART_MAIN);

    lv_obj_t *ui_img_eye = lv_img_create(cont_ta_line);
    lv_img_set_src(ui_img_eye, &eye_of_horus);
    lv_obj_set_style_img_recolor(ui_img_eye, C(COL_WHITE), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(ui_img_eye, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_align(ui_img_eye, LV_ALIGN_TOP_RIGHT, 0, 10);

    lv_obj_t *cont_btn_line = lv_obj_create(cont_main);
    lv_obj_set_size(cont_btn_line, 512, 230);
    lv_obj_set_pos(cont_btn_line, 0, 370);
    lv_obj_clear_flag(cont_btn_line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(cont_btn_line, &style_header_footer, LV_PART_MAIN);
    
    lv_obj_t *cont = lv_obj_create(cont_btn_line);
    lv_obj_set_size(cont, 492, 200);
    lv_obj_set_pos(cont, 10, 10);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(cont, C(COL_WHITE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(cont, LV_OPA_30, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);

    // 3-column button grid: [OFF][AWAY][VACATION] / [HOME_A][HOME_B][CUSTOM]
    // 580px container / 3 cols = 193+193+194px
    ui_bt_off = lv_btn_create(cont);
    lv_obj_remove_style_all(ui_bt_off);
    lv_obj_set_width(ui_bt_off, 164);
    lv_obj_set_height(ui_bt_off, 100);
    lv_obj_set_x(ui_bt_off, 0);
    lv_obj_set_y(ui_bt_off, 0);
    lv_obj_add_flag(ui_bt_off, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_bt_off, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_bt_off, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui_bt_off, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(ui_bt_off, C(0x7D4CDB), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_bt_off, LV_OPA_0, 0);
    lv_obj_set_style_radius(ui_bt_off, 10, 0);
    lv_obj_add_event_cb(ui_bt_off, single_select_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *ui_bt_off_icon = lv_img_create(ui_bt_off);
    lv_img_set_src(ui_bt_off_icon, &off);
    lv_obj_set_style_img_recolor(ui_bt_off_icon, C(COL_WHITE), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(ui_bt_off_icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_align(ui_bt_off_icon, LV_ALIGN_CENTER);

    ui_bt_away = lv_btn_create(cont);
    lv_obj_remove_style_all(ui_bt_away);
    lv_obj_set_width(ui_bt_away, 164);
    lv_obj_set_height(ui_bt_away, 100);
    lv_obj_set_x(ui_bt_away, 164);
    lv_obj_set_y(ui_bt_away, 0);
    lv_obj_add_flag(ui_bt_away, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_bt_away, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_bt_away, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui_bt_away, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(ui_bt_away, C(0x7D4CDB), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_bt_away, LV_OPA_0, 0);
    lv_obj_set_style_radius(ui_bt_away, 10, 0);
    lv_obj_add_event_cb(ui_bt_away, single_select_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *ui_bt_away_icon = lv_img_create(ui_bt_away);
    lv_img_set_src(ui_bt_away_icon, &away);
    lv_obj_set_style_img_recolor(ui_bt_away_icon, C(COL_WHITE), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(ui_bt_away_icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_align(ui_bt_away_icon, LV_ALIGN_CENTER);

    ui_bt_vacation = lv_btn_create(cont);
    lv_obj_remove_style_all(ui_bt_vacation);
    lv_obj_set_width(ui_bt_vacation, 164);
    lv_obj_set_height(ui_bt_vacation, 100);
    lv_obj_set_x(ui_bt_vacation, 328);
    lv_obj_set_y(ui_bt_vacation, 0);
    lv_obj_add_flag(ui_bt_vacation, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_bt_vacation, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_bt_vacation, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui_bt_vacation, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(ui_bt_vacation, C(0x7D4CDB), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_bt_vacation, LV_OPA_0, 0);
    lv_obj_set_style_radius(ui_bt_vacation, 10, 0);
    lv_obj_add_event_cb(ui_bt_vacation, single_select_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *ui_bt_vacation_icon = lv_img_create(ui_bt_vacation);
    lv_img_set_src(ui_bt_vacation_icon, &vacation);
    lv_obj_set_style_img_recolor(ui_bt_vacation_icon, C(COL_WHITE), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(ui_bt_vacation_icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_align(ui_bt_vacation_icon, LV_ALIGN_CENTER);

    ui_bt_home_a = lv_btn_create(cont);
    lv_obj_remove_style_all(ui_bt_home_a);
    lv_obj_set_width(ui_bt_home_a, 164);
    lv_obj_set_height(ui_bt_home_a, 100);
    lv_obj_set_x(ui_bt_home_a, 0);
    lv_obj_set_y(ui_bt_home_a, 100);
    lv_obj_add_flag(ui_bt_home_a, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_bt_home_a, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_bt_home_a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui_bt_home_a, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(ui_bt_home_a, C(0x7D4CDB), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_bt_home_a, LV_OPA_0, 0);
    lv_obj_set_style_radius(ui_bt_home_a, 10, 0);
    lv_obj_add_event_cb(ui_bt_home_a, single_select_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *ui_bt_home_a_icon = lv_img_create(ui_bt_home_a);
    lv_img_set_src(ui_bt_home_a_icon, &home_a);
    lv_obj_set_style_img_recolor(ui_bt_home_a_icon, C(COL_WHITE), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(ui_bt_home_a_icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_align(ui_bt_home_a_icon, LV_ALIGN_CENTER);

    ui_bt_home_b = lv_btn_create(cont);
    lv_obj_remove_style_all(ui_bt_home_b);
    lv_obj_set_width(ui_bt_home_b, 164);
    lv_obj_set_height(ui_bt_home_b, 100);
    lv_obj_set_x(ui_bt_home_b, 164);
    lv_obj_set_y(ui_bt_home_b, 100);
    lv_obj_add_flag(ui_bt_home_b, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_bt_home_b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_bt_home_b, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui_bt_home_b, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(ui_bt_home_b, C(0x7D4CDB), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_bt_home_b, LV_OPA_0, 0);
    lv_obj_set_style_radius(ui_bt_home_b, 10, 0);
    lv_obj_add_event_cb(ui_bt_home_b, single_select_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *ui_bt_home_b_icon = lv_img_create(ui_bt_home_b);
    lv_img_set_src(ui_bt_home_b_icon, &home_b);
    lv_obj_set_style_img_recolor(ui_bt_home_b_icon, C(COL_WHITE), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(ui_bt_home_b_icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_align(ui_bt_home_b_icon, LV_ALIGN_CENTER);

    ui_bt_custom = lv_btn_create(cont);
    lv_obj_remove_style_all(ui_bt_custom);
    lv_obj_set_width(ui_bt_custom, 164);
    lv_obj_set_height(ui_bt_custom, 100);
    lv_obj_set_x(ui_bt_custom, 328);
    lv_obj_set_y(ui_bt_custom, 100);
    lv_obj_add_flag(ui_bt_custom, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_bt_custom, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_bt_custom, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui_bt_custom, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(ui_bt_custom, C(0x7D4CDB), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_bt_custom, LV_OPA_0, 0);
    lv_obj_set_style_radius(ui_bt_custom, 10, 0);
    lv_obj_add_event_cb(ui_bt_custom, single_select_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *ui_bt_custom_icon = lv_img_create(ui_bt_custom);
    lv_img_set_src(ui_bt_custom_icon, &custom);
    lv_obj_set_style_img_recolor(ui_bt_custom_icon, C(COL_WHITE), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(ui_bt_custom_icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_align(ui_bt_custom_icon, LV_ALIGN_CENTER);

    lv_obj_t *cont_kb_line = lv_obj_create(cont_main);
    lv_obj_set_size(cont_kb_line, 511, 523);
    lv_obj_set_pos(cont_kb_line, 512, 61);
    lv_obj_clear_flag(cont_kb_line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(cont_kb_line, &style_header_footer, LV_PART_MAIN);

    kb = lv_keyboard_create(cont_kb_line);
    lv_keyboard_set_textarea(kb, ta);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, kb_map_num, kb_ctrl_num);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);
    lv_obj_align(kb, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(kb, 511, 503);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_VALUE_CHANGED, ta);
    lv_obj_add_style(kb, &style_kb_main, LV_PART_MAIN);
    lv_obj_add_style(kb, &style_kb_btn, (uint32_t)LV_PART_ITEMS | (uint32_t)LV_STATE_DEFAULT);
    lv_obj_add_style(kb, &style_kb_btn_pressed, (uint32_t)LV_PART_ITEMS | (uint32_t)LV_STATE_PRESSED);
    lv_obj_add_style(kb, &style_kb_btn_disabled, (uint32_t)LV_PART_ITEMS | (uint32_t)LV_STATE_DISABLED);

    /* IDs der relevanten Buttons suchen */
    btn_id_x = find_button_id_for_text(kb, "X");
    btn_id_dummy_left = find_button_id_for_text(kb, " ");

    /* linken Dummy verstecken */
    if (btn_id_dummy_left != LV_BTNMATRIX_BTN_NONE)
    {
        lv_btnmatrix_set_btn_ctrl(kb, btn_id_dummy_left, LV_BTNMATRIX_CTRL_HIDDEN);
    }

    /* X initial deaktivieren */
    if (btn_id_x != LV_BTNMATRIX_BTN_NONE)
    {
        lv_btnmatrix_set_btn_ctrl(kb, btn_id_x, LV_BTNMATRIX_CTRL_DISABLED);
    }

    btn_map[0] = ui_bt_off;
    btn_map[1] = ui_bt_away;
    btn_map[2] = ui_bt_vacation;
    btn_map[3] = ui_bt_home_a;
    btn_map[4] = ui_bt_home_b;
    btn_map[5] = ui_bt_custom;

    // Countdown-Overlay (erzeugt nach cont_main → liegt oben drüber)
    cont_countdown = lv_obj_create(scr);
    lv_obj_set_size(cont_countdown, 302, 194);
    lv_obj_align(cont_countdown, LV_ALIGN_TOP_RIGHT, -10, 80);
    lv_obj_clear_flag(cont_countdown, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(cont_countdown, 0, 0);
    lv_obj_set_style_radius(cont_countdown, 0, 0);
    lv_obj_set_style_bg_opa(cont_countdown, LV_OPA_COVER, 0);
    lv_obj_add_flag(cont_countdown, LV_OBJ_FLAG_HIDDEN);

    lbl_countdown_info = lv_label_create(cont_countdown);
    lv_obj_align(lbl_countdown_info, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_style_text_font(lbl_countdown_info, &ui_font_star, 0);
    lv_obj_set_style_text_color(lbl_countdown_info, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(lbl_countdown_info, "");

    lbl_countdown_num = lv_label_create(cont_countdown);
    lv_obj_align(lbl_countdown_num, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_text_font(lbl_countdown_num, &ui_font_star, 0);
    lv_obj_set_style_text_color(lbl_countdown_num, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(lbl_countdown_num, "");
    
    fprintf(stderr, "[horus] screen built OK\n");
    return true;
}

static void countdown_tick_cb(lv_timer_t *t)
{
    (void)t;
    if (countdown_secs > 0)
        countdown_secs--;

    if (countdown_secs == 0)
    {
        lv_timer_del(countdown_timer);
        countdown_timer = NULL;
        if (!countdown_is_entry)
        {
            lv_obj_add_flag(cont_countdown, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(cont_main, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        }
        return;
    }

    char buf[8];
    snprintf(buf, sizeof(buf), "%d", countdown_secs);
    lv_label_set_text(lbl_countdown_num, buf);
}

static void start_countdown(int seconds, bool is_entry, uint32_t color, const char *info)
{
    if (!cont_countdown)
        return;
    if (countdown_timer)
    {
        lv_timer_del(countdown_timer);
        countdown_timer = NULL;
    }
    countdown_secs = seconds;
    countdown_is_entry = is_entry;
    lv_obj_set_style_bg_color(cont_countdown, lv_color_hex(color), 0);
    lv_obj_set_style_bg_color(cont_main, lv_color_hex(color), LV_PART_MAIN);
    lv_label_set_text(lbl_countdown_info, info);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", seconds);
    lv_label_set_text(lbl_countdown_num, buf);
    lv_obj_clear_flag(cont_countdown, LV_OBJ_FLAG_HIDDEN);
    if (seconds > 0)
        countdown_timer = lv_timer_create(countdown_tick_cb, 1000, NULL);
}

void ui_start_exit_countdown(int seconds)
{
    start_countdown(seconds, false, 0xFF8800, "Arming...");
}

// Caller must hold lvgl_port_lock — wake_display_lv() and start_countdown() are LVGL API calls.
void ui_start_entry_countdown(int seconds)
{
    wake_display_lv();
    start_countdown(seconds, true, 0xCC0000, "ALARM IN");
}

void ui_show_triggered()
{
    if (!cont_countdown)
        return;
    wake_display_lv();
    if (countdown_timer)
    {
        lv_timer_del(countdown_timer);
        countdown_timer = NULL;
    }
    lv_obj_set_style_bg_color(cont_countdown, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_color(cont_main, lv_color_hex(0xFF0000), LV_PART_MAIN);
    lv_label_set_text(lbl_countdown_info, "! ALARM !");
    lv_label_set_text(lbl_countdown_num, "");
    lv_obj_clear_flag(cont_countdown, LV_OBJ_FLAG_HIDDEN);
}

void ui_cancel_countdown()
{
    if (!cont_countdown)
        return;
    if (countdown_timer)
    {
        lv_timer_del(countdown_timer);
        countdown_timer = NULL;
    }
    lv_obj_add_flag(cont_countdown, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(cont_main, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
}

void ui_wake()
{
    s_wake_requested = true;
}

static void status_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (ui_lbl_status_msg)
        lv_label_set_text(ui_lbl_status_msg, "");
    if (status_msg_copy)
    {
        //free(status_msg_copy);
        status_msg_copy = NULL;
    }
    if (status_timer)
    {
        lv_timer_del(status_timer);
        status_timer = NULL;
    }
}

/* Wird in LVGL-Context ausgeführt (via lv_async_call). Takes ownership of data (heap string). */
static void ui_set_status_msg_lv(void *data)
{
    char *msg_owned = (char *)data;

    if (status_timer)
    {
        lv_timer_del(status_timer);
        status_timer = NULL;
    }
    if (status_msg_copy)
    {
        //free(status_msg_copy);
        status_msg_copy = NULL;
    }

    status_msg_copy = msg_owned;
    if (ui_lbl_status_msg)
        lv_label_set_text(ui_lbl_status_msg, status_msg_copy);

    status_timer = lv_timer_create(status_timer_cb, 3000, NULL);
}

/* Thread-/Task-sicherer wrapper: kann von außerhalb LVGL aufgerufen werden */
void ui_set_status_msg(const char *msg)
{
    /* strdup here so the caller's buffer (possibly stack) need not outlive this call */
    char *copy = strdup(msg ? msg : "");
    if (!copy) return;

    //lvgl_port_lock(-1);
    lv_async_call(ui_set_status_msg_lv, (void *)copy);
    //lvgl_port_unlock();
}

void ui_set_status(int state)
{
    for (int i = 0; i < (int)6U; i++)
    {
        if (i == state)
        {
            lv_obj_add_state(btn_map[i], LV_STATE_CHECKED);
            lv_obj_set_style_bg_opa(btn_map[i], LV_OPA_100, 0);
        }
        else
        {
            lv_obj_clear_state(btn_map[i], LV_STATE_CHECKED);
            lv_obj_set_style_bg_opa(btn_map[i], LV_OPA_0, 0);
        }
    }
}


void horus_tick(void)
{
    
}
