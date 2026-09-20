#ifndef AVIUM_RUNTIME_IO_H
#define AVIUM_RUNTIME_IO_H

#include "texts.h"

#include <stdbool.h>

void avium_print_bool(bool value, unsigned line);
void avium_print_real(double value, unsigned line);
void avium_print_text(const avium_text* value, unsigned line);
void avium_input(avium_text* result, unsigned line);

#endif /* AVIUM_RUNTIME_IO_H */
