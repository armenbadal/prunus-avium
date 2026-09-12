#include "arrays.h"

void avium_f(double n)
{
    const double array_size = n * 2.0;
    avium_array* a = avium_array_create(AVIUM_ARRAY_TEXT, array_size, 2);

    double i = 0.0;
    const double begin = 0.0;
    const double end = avium_array_length(a, 3) - 1.0;
    const double step = 1.0;
    i = begin;

    while( i <= end ) {
        avium_text value = avium_str(i, 4);
        avium_text* target = avium_text_array_at(a, i, 4);
        avium_text_move_assign(target, &value);
        i = i + step;
    }

    avium_array_destroy(a);
}
