#pragma once

#include <stdbool.h>
#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

bool osiris_top_init(void);
void osiris_top_tick(void);

#ifdef __cplusplus
}
#endif
