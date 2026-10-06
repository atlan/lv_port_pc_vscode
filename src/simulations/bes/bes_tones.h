#pragma once

#include <stdint.h>

/* Bedientöne für bes im LCARS-Stil: kurze Rechtecktöne, 800…2400 Hz.
 *
 * Diese Datei und bes_tones.c hängen an nichts (kein LVGL, kein SDL) und
 * wandern unverändert in die Firmware; dort ersetzt der Ton-Treiber nur
 * bes_tones_play(). In der Simulation spielt bes_tones_sdl.c sie ab.
 *
 * ⚠ Alarm- und Warntöne stehen hier NICHT — die spielt chons. */

typedef struct {
    uint16_t hz;    /* Frequenz */
    uint16_t ms;    /* Dauer */
} bes_tone_t;

typedef enum {
    BES_TONE_KEY,       /* Taste gedrückt */
    BES_TONE_SELECT,    /* Betriebsart gewählt, Ansicht gewechselt */
    BES_TONE_CONFIRM,   /* Code angenommen */
    BES_TONE_REJECT,    /* Code falsch, Eingabe verworfen */
    BES_TONE_BOOT,      /* Start abgeschlossen */
    BES_TONE_COUNT
} bes_tone_id_t;

#define BES_TONE_MAX      4     /* Töne je Folge */
#define BES_TONE_GAP_MS  30     /* Pause zwischen zwei Tönen einer Folge */

typedef struct {
    uint8_t    count;
    bes_tone_t tone[BES_TONE_MAX];
} bes_tone_seq_t;

extern const bes_tone_seq_t BES_TONE_SEQ[BES_TONE_COUNT];

/* Spielt eine Folge ab, ohne zu blockieren; eine laufende Folge wird ersetzt. */
void bes_tones_play(bes_tone_id_t id);
