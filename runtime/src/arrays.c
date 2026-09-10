#include "arrays.h"

#include "errors.h"

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

struct avium_array {
    avium_element_type type;
    void* elements;
    size_t length;
};

static size_t element_size(avium_element_type type, unsigned line)
{
    switch( type ) {
        case AVIUM_ARRAY_TEXT:
            return sizeof(avium_text);
        case AVIUM_ARRAY_REAL:
            return sizeof(double);
        case AVIUM_ARRAY_BOOL:
            return sizeof(bool);
    }

    avium_runtime_error(line, "Զանգվածի տարրի տիպն անհայտ է։");
}

static void* array_element_at(avium_array* array, avium_element_type type, double index, unsigned line)
{
    if( array == NULL )
        avium_runtime_error(line, "Զանգվածային հղումը դատարկ է։");
    if( array->type != type )
        avium_runtime_error(line, "Զանգվածի տարրի տիպը չի համապատասխանում։");
    if( !isfinite(index) || trunc(index) != index )
        avium_runtime_error(line, "Զանգվածի ինդեքսը պետք է լինի ամբողջ թիվ։");
    if( index < 0.0 || index >= (double)array->length )
        avium_runtime_error(line, "Զանգվածի ինդեքսը սահմաններից դուրս է։");

    const size_t position = (size_t)index;
    const size_t size = element_size(type, line);
    return (char*)array->elements + position * size;
}

avium_array* avium_array_create(avium_element_type type, double length,
    unsigned line)
{
    const size_t size = element_size(type, line);
    const bool integral = isfinite(length) && trunc(length) == length;
    const double largest_exact_length = ldexp(1.0, DBL_MANT_DIG);
    const bool representable = length <= largest_exact_length && length <= (double)SIZE_MAX;
    if( !integral || length <= 0.0 || !representable )
        avium_runtime_error(line, "Զանգվածի չափը պետք է լինի դրական ամբողջ թիվ։");

    const size_t count = (size_t)length;
    if( count > SIZE_MAX / size )
        avium_runtime_error(line, "Զանգվածը չափազանց մեծ է։");

    avium_array* array = malloc(sizeof(*array));
    if( array == NULL )
        avium_runtime_error(line, "Զանգվածի համար հիշողություն հատկացնել չհաջողվեց։");

    array->elements = calloc(count, size);
    if( array->elements == NULL ) {
        free(array);
        avium_runtime_error(line, "Զանգվածի համար հիշողություն հատկացնել չհաջողվեց։");
    }

    array->type = type;
    array->length = count;

    if( type == AVIUM_ARRAY_TEXT ) {
        avium_text* elements = array->elements;
        for( size_t index = 0; index < count; ++index ) {
            elements[index] = (avium_text){
                .data = "",
                .length = 0,
                .owned = false,
            };
        }
    }

    return array;
}

void avium_array_destroy(avium_array* array)
{
    if( array == NULL )
        return;

    if( array->type == AVIUM_ARRAY_TEXT ) {
        avium_text* elements = array->elements;
        for( size_t index = 0; index < array->length; ++index )
            avium_text_destroy(&elements[index]);
    }

    free(array->elements);
    free(array);
}

double avium_array_length(const avium_array* array, unsigned line)
{
    if( array == NULL )
        avium_runtime_error(line, "Զանգվածային հղումը դատարկ է։");
    return (double)array->length;
}

avium_text* avium_text_array_at(avium_array* array, double index, unsigned line)
{
    return array_element_at(array, AVIUM_ARRAY_TEXT, index, line);
}

double* avium_real_array_at(avium_array* array, double index, unsigned line)
{
    return array_element_at(array, AVIUM_ARRAY_REAL, index, line);
}

bool* avium_bool_array_at(avium_array* array, double index, unsigned line)
{
    return array_element_at(array, AVIUM_ARRAY_BOOL, index, line);
}
