/**
 * @file main.c
 */

#ifndef _DEFAULT_SOURCE
  #define _DEFAULT_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#ifdef _MSC_VER
  #include <Windows.h>
#else
  #include <unistd.h>
  #include <pthread.h>
#endif

#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"
#include <SDL.h>

#include "hal/hal.h"
#include "simulations/sim.h"
#include "simulations/osiris_bottom/osiris_bottom.h"

/* Aktive Simulation — zum Wechseln einfach eine andere sim_t eintragen */
static const sim_t active_sim = { "osiris_bottom", 800, 480, osiris_bottom_init, osiris_bottom_tick };

#if LV_USE_OS != LV_OS_FREERTOS

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    lv_init();
    sdl_hal_init(active_sim.width, active_sim.height);

    active_sim.init();

    while (1) {
        uint32_t sleep_time_ms = lv_timer_handler();
        active_sim.tick();
        if (sleep_time_ms == LV_NO_TIMER_READY)
            sleep_time_ms = LV_DEF_REFR_PERIOD;
#ifdef _MSC_VER
        Sleep(sleep_time_ms);
#else
        usleep(sleep_time_ms * 1000);
#endif
    }

    return 0;
}

#endif
