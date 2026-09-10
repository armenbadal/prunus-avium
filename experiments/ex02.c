#include "arrays.h"

static const avium_text avium_literal_copy = {
    .data = "_copy",
    .length = 5,
    .owned = false,
};

void avium_g(avium_array* a, avium_array* b)
{
    double i = 0.0;
    const double begin = 0.0;
    const double end = avium_array_length(a, 2) - 1.0;
    const double step = 1.0;
    i = begin;

    while( i <= end ) {
        const avium_text* source = avium_text_array_at(b, i, 3);
        avium_text value = avium_text_concat(*source, avium_literal_copy, 3);
        avium_text* target = avium_text_array_at(a, i, 3);
        avium_text_move_assign(target, &value);
        i = i + step;
    }
}
