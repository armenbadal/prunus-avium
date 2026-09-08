#pragma once

#include "texts.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AVIUM_ARRAY_TEXT,
    AVIUM_ARRAY_REAL,
    AVIUM_ARRAY_BOOL
} avium_element_type;

typedef struct avium_array avium_array;

avium_array* avium_array_create(avium_element_type type, double length,
    unsigned line);
void avium_array_destroy(avium_array* array);
double avium_array_length(const avium_array* array);
avium_text* avium_text_array_at(avium_array* array, double index,
    unsigned line);
double* avium_real_array_at(avium_array* array, double index, unsigned line);
bool* avium_bool_array_at(avium_array* array, double index, unsigned line);

#ifdef __cplusplus
}
#endif
