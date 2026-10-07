#pragma once

#include <stdbool.h>
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

/* bes im LCARS-Stil — erster Entwurf: Rahmen, Betriebsarten-Säule und die
 * fünf Farbzustände. Kein Tastenfeld auf dem Schirm, dafür sind die echten
 * Taster da; der Schirm zeigt nur, wie viele Stellen eingegeben sind. */

typedef enum {
    BES_LCARS_DISARMED,     /* unscharf */
    BES_LCARS_EXIT_DELAY,   /* Austrittsverzögerung */
    BES_LCARS_ARMED,        /* scharf */
    BES_LCARS_ENTRY_DELAY,  /* Eintrittsverzögerung */
    BES_LCARS_TRIGGERED,    /* ausgelöst */
    BES_LCARS_STATE_COUNT
} bes_lcars_state_t;

#define BES_LCARS_MODE_COUNT 6   /* Alarmo: Aus, Abwesend, Zuhause, Nacht, Urlaub, Mit Bypass */
#define BES_LCARS_CODE_LEN   6

bool bes_lcars_init(void);
void bes_lcars_tick(void);

void bes_lcars_set_state(bes_lcars_state_t state);
void bes_lcars_set_mode(int mode);
/* Mit Rückruf wird ein Tipp auf eine Betriebsart NICHT lokal übernommen, sondern gemeldet
 * (die Firmware schickt ihn an Alarmo; der Zustand kommt dann von dort zurück). */
void bes_lcars_set_mode_cb(void (*cb)(int mode));
void bes_lcars_set_countdown(int seconds);
void bes_lcars_set_code_digits(int count);
void bes_lcars_set_message(const char *text);
void bes_lcars_set_time(int hour, int minute);
/* Anwesenheit: 1 = allein, 2 = zu zweit */
void bes_lcars_set_presence(int persons);
int  bes_lcars_get_presence(void);
/* Mit Rückruf wird ein Tipp auf den Umschalter NICHT lokal übernommen, sondern gemeldet
 * (die Firmware setzt input_number.horus_home_count; der Wert kommt von HA zurück). */
void bes_lcars_set_presence_cb(void (*cb)(int persons));
/* Raumklima in der Fußleiste (SHT31 im Gerät) */
void bes_lcars_set_climate(int zehntel_grad, int feuchte);
/* Menü (Endkappe der Kopfleiste): Wetter, Szenen, Schalter. Mit Rückruf übernimmt die
 * Firmware das Öffnen selbst; ohne Rückruf öffnet der Schirm sein eingebautes Menü. */
void bes_lcars_set_menu_cb(void (*cb)(void));
void bes_lcars_menu_open(bool open);
void bes_lcars_menu_page(int page);          /* 0 Wetter, 1 Szenen, 2 Schalter */

/* --- Wetterübersicht ----------------------------------------------------- */
#define BES_LCARS_FORECAST_MAX 5
typedef struct {
    const char *day;          /* "MO" */
    int         temp_min;     /* °C */
    int         temp_max;
    const char *condition;    /* "BEWÖLKT" */
} bes_lcars_forecast_t;

void bes_lcars_set_weather(const char *location, int temp, const char *condition,
                           int humidity, const char *wind, const char *pressure,
                           const bes_lcars_forecast_t *forecast, int count);

/* --- Szenen und Schalter aus Home Assistant ------------------------------ */
#define BES_LCARS_ITEMS_MAX 12
void bes_lcars_set_scenes(const char *const *names, int count);
void bes_lcars_set_scene_cb(void (*cb)(int index));          /* Szene angetippt */
void bes_lcars_set_switches(const char *const *names, const bool *on, int count);
void bes_lcars_set_switch(int index, bool on);               /* Zustand aus HA nachführen */
void bes_lcars_set_switch_cb(void (*cb)(int index, bool on)); /* Schalter angetippt */
