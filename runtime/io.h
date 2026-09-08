#pragma once

#include "texts.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void avium_print_bool(bool value);
void avium_print_real(double value);
void avium_print_text(avium_text value);
avium_text avium_input(unsigned line);

#ifdef __cplusplus
}
#endif
