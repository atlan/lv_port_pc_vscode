#pragma once

#include <stdbool.h>
#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

bool osiris_bottom_init(void);
void osiris_bottom_tick(void);

#ifdef __cplusplus
}
#endif
