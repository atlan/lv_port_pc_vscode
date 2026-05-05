#pragma once

#include <stdbool.h>

typedef struct {
    const char *name;
    int32_t     width;
    int32_t     height;
    bool (*init)(void);
    void (*tick)(void);
} sim_t;
