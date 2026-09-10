#ifndef AVIUM_RUNTIME_IO_H
#define AVIUM_RUNTIME_IO_H

#include "texts.h"

#include <stdbool.h>

void avium_print_bool(bool value);
void avium_print_real(double value);
void avium_print_text(avium_text value);
avium_text avium_input(unsigned line);

#endif /* AVIUM_RUNTIME_IO_H */
