#include "arrays.h"

static const avium_text avium_literal_copy = {.data = "_copy", .length = 5, .owned = false};
static const avium_text avium_literal_One = {.data = "One", .length = 3, .owned = false};
static const avium_text avium_literal_Two = {.data = "Two", .length = 3, .owned = false};

void avium_g(avium_array* a, avium_array* b)
{
    const double begin = 0.0;
    const double end = avium_array_length(a, 2) - 1.0;
    const double step = 1.0;
    double i = begin;

    while( i <= end ) {
        const avium_text* source = avium_text_array_at(b, i, 3);
        avium_text value = avium_text_concat(*source, avium_literal_copy, 3);
        avium_text* target = avium_text_array_at(a, i, 3);
        avium_text_move_assign(target, &value);
        i = i + step;
    }
}

void avium_Main()
{
    avium_array *x = avium_array_create(AVIUM_ARRAY_TEXT, 2, 8);
    avium_text *p = avium_text_array_at(x, 0, 9);
    p = &avium_literal_One;
    p = avium_text_array_at(x, 1, 10);
    p = &avium_literal_Two;

    avium_array *y = avium_array_create(AVIUM_ARRAY_TEXT, 2, 12);
    avium_g(x, y);
}
