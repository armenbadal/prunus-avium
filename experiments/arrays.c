#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *data;
    size_t length;
    bool owned;
} text;

typedef enum {
    ARRAY_TEXT,
    ARRAY_REAL,
    ARRAY_BOOL
} element_type;

typedef struct {
    element_type type;
    void *elements;
    size_t length;
} array_descriptor;

static text create_text(const char *data, size_t length)
{
    text result = {0};
    char *copy = malloc(length + 1);
    if( copy == NULL )
        return result;

    memcpy(copy, data, length);
    copy[length] = '\0';
    result.data = copy;
    result.length = length;
    result.owned = true;
    return result;
}

static void destroy_text(text *value)
{
    if( value == NULL || !value->owned )
        return;

    free((void *)value->data);
    *value = (text){0};
}

static size_t element_size(element_type type)
{
    switch( type ) {
        case ARRAY_TEXT:
            return sizeof(text);
        case ARRAY_REAL:
            return sizeof(double);
        case ARRAY_BOOL:
            return sizeof(bool);
    }

    return 0;
}

array_descriptor *create_array(element_type type, size_t length)
{
    size_t size = element_size(type);
    if( size == 0 || length > SIZE_MAX / size )
        return NULL;

    array_descriptor *array = malloc(sizeof(*array));
    if( array == NULL )
        return NULL;

    array->type = type;
    array->length = length;

    array->elements = calloc(length, size);
    if( length != 0 && array->elements == NULL ) {
        free(array);
        return NULL;
    }

    return array;
}

void destroy_array(array_descriptor *array)
{
    if( array == NULL )
        return;

    if( array->type == ARRAY_TEXT ) {
        text *elements = array->elements;
        for( size_t i = 0; i < array->length; ++i )
            destroy_text(&elements[i]);
    }

    free(array->elements);
    free(array);
}


void f(array_descriptor *array)
{
    if( array == NULL || array->type != ARRAY_REAL || array->length == 0 )
        return;

    double *elements = array->elements;
    elements[0] = 3.14;
}


int main()
{
    // array of double
    array_descriptor *double_array = create_array(ARRAY_REAL, 2);
    if( double_array == NULL )
        return EXIT_FAILURE;

    double *double_elements = double_array->elements;
    double_elements[0] = 3.14;
    double_elements[1] = 2.71;
    destroy_array(double_array);

    array_descriptor *text_array = create_array(ARRAY_TEXT, 2);
    if( text_array == NULL )
        return EXIT_FAILURE;

    text *text_elements = text_array->elements;
    text_elements[0].data = "Hello";
    text_elements[0].length = 5;
    text_elements[1].data = "World";
    text_elements[1].length = 5;
    destroy_array(text_array);

    // array of dynamically allocated strings
    array_descriptor *dynamic_text_array = create_array(ARRAY_TEXT, 2);
    if( dynamic_text_array == NULL )
        return EXIT_FAILURE;

    text *dynamic_text_elements = dynamic_text_array->elements;
    dynamic_text_elements[0] = create_text("A string created at runtime", sizeof("A string created at runtime") - 1);
    dynamic_text_elements[1] = create_text("Another runtime string", sizeof("Another runtime string") - 1);
    if( dynamic_text_elements[0].data == NULL || dynamic_text_elements[1].data == NULL ) {
        destroy_array(dynamic_text_array);
        return EXIT_FAILURE;
    }
    destroy_array(dynamic_text_array);

    return EXIT_SUCCESS;
}
