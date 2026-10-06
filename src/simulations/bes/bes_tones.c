#include "bes_tones.h"

/* Vorlage: Tonkatalog aus github.com/dorofino/LCARS-ESP32 (lcars_audio.cpp).
 * ★ Die Taste weicht ab: 1800 Hz / 80 ms ist die am Gerät festgelegte
 *   bes-Quittung (bes_audio_quittung), nicht die 1760 Hz / 60 ms der Vorlage. */
const bes_tone_seq_t BES_TONE_SEQ[BES_TONE_COUNT] = {
    [BES_TONE_KEY]     = { 1, { { 1800, 80 } } },
    [BES_TONE_SELECT]  = { 1, { {  880, 80 } } },
    [BES_TONE_CONFIRM] = { 2, { { 1200, 60 }, { 1800, 80 } } },
    [BES_TONE_REJECT]  = { 3, { { 1400, 50 }, { 1400, 50 }, { 1400, 50 } } },
    [BES_TONE_BOOT]    = { 4, { {  880, 60 }, { 1320, 60 }, { 1760, 60 }, { 2200, 100 } } },
};
