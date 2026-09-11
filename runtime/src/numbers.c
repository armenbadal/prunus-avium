#include "numbers.h"

#include "errors.h"

#include <math.h>

double avium_sqr(double value, unsigned line)
{
    if( value < 0.0 )
        avium_runtime_error(line, "Բացասական թվի քառակուսի արմատը սահմանված չէ։");
    return sqrt(value);
}
