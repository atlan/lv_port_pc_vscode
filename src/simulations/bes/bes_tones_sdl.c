#include "bes_tones.h"

#include <SDL.h>
#include <stdbool.h>
#include <stdlib.h>

/* Tonausgabe der Simulation über SDL: die ganze Folge wird als Rechteck in
 * einen Puffer gerechnet und in die Warteschlange des Audiogeräts gelegt. */

#define RATE     44100
#define VOLUME    6000      /* von 32767 — Rechteck ist laut */
#define RAMP_MS      2      /* An- und Abklingen, sonst knackt jede Flanke */

static SDL_AudioDeviceID dev;
static bool              tried;

static bool open_device(void)
{
    if (tried) return dev != 0;
    tried = true;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return false;
    SDL_AudioSpec want = { .freq = RATE, .format = AUDIO_S16SYS, .channels = 1, .samples = 1024 };
    dev = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
    if (dev) SDL_PauseAudioDevice(dev, 0);
    return dev != 0;
}

void bes_tones_play(bes_tone_id_t id)
{
    if (id >= BES_TONE_COUNT || !open_device()) return;
    const bes_tone_seq_t *seq = &BES_TONE_SEQ[id];

    uint32_t total_ms = 0;
    for (int i = 0; i < seq->count; i++) total_ms += seq->tone[i].ms + (i ? BES_TONE_GAP_MS : 0);
    uint32_t total = (uint32_t)((uint64_t)total_ms * RATE / 1000);
    int16_t *pcm = calloc(total, sizeof(int16_t));
    if (!pcm) return;

    uint32_t pos = 0;
    const uint32_t ramp = RATE * RAMP_MS / 1000;
    for (int i = 0; i < seq->count; i++) {
        if (i) pos += RATE * BES_TONE_GAP_MS / 1000;
        uint32_t n = (uint32_t)((uint64_t)seq->tone[i].ms * RATE / 1000);
        for (uint32_t k = 0; k < n; k++) {
            /* Rechteck aus der Phase in Halbperioden gezählt */
            bool high = ((uint64_t)k * 2 * seq->tone[i].hz / RATE) % 2 == 0;
            int32_t v = high ? VOLUME : -VOLUME;
            if (k < ramp)          v = v * (int32_t)k / (int32_t)ramp;
            else if (n - k < ramp) v = v * (int32_t)(n - k) / (int32_t)ramp;
            pcm[pos + k] = (int16_t)v;
        }
        pos += n;
    }

    SDL_ClearQueuedAudio(dev);
    SDL_QueueAudio(dev, pcm, total * sizeof(int16_t));
    free(pcm);
}
