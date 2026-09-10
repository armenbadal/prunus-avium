#include "texts.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _avium_text {
    const char *data;
    size_t length;
    bool owned;
} avium_text;

static void text_error(unsigned line, const char* message)
{
    fprintf(stderr, "%u: %s\n", line, message);
    exit(EXIT_FAILURE);
}

static avium_text empty_text(void)
{
    return (avium_text){
        .data = "",
        .length = 0,
        .owned = false,
    };
}

avium_text avium_text_create(const char* data, size_t length, unsigned line)
{
    if( length == SIZE_MAX )
        text_error(line, "Տեքստը չափազանց երկար է։");

    char* copy = malloc(length + 1);
    if( copy == NULL )
        text_error(line, "Տեքստի համար հիշողություն հատկացնել չհաջողվեց։");

    if( length != 0 )
        memcpy(copy, data, length);
    copy[length] = '\0';

    return (avium_text){
        .data = copy,
        .length = length,
        .owned = true,
    };
}

avium_text avium_text_copy(avium_text value, unsigned line)
{
    return avium_text_create(value.data, value.length, line);
}

void avium_text_destroy(avium_text* value)
{
    if( value == NULL )
        return;

    if( value->owned )
        free((void*)value->data);
    *value = empty_text();
}

void avium_text_move_assign(avium_text* target, avium_text* source)
{
    if( target == NULL || source == NULL || target == source )
        return;

    avium_text_destroy(target);
    *target = *source;
    *source = empty_text();
}

avium_text avium_text_concat(avium_text left, avium_text right, unsigned line)
{
    if( left.length > SIZE_MAX - right.length )
        text_error(line, "Տեքստը չափազանց երկար է։");

    const size_t length = left.length + right.length;
    if( length == SIZE_MAX )
        text_error(line, "Տեքստը չափազանց երկար է։");

    char* data = malloc(length + 1);
    if( data == NULL )
        text_error(line, "Տեքստի համար հիշողություն հատկացնել չհաջողվեց։");

    if( left.length != 0 )
        memcpy(data, left.data, left.length);
    if( right.length != 0 )
        memcpy(data + left.length, right.data, right.length);
    data[length] = '\0';

    return (avium_text){
        .data = data,
        .length = length,
        .owned = true,
    };
}

int avium_text_compare(avium_text left, avium_text right)
{
    const size_t common_length = left.length < right.length ? left.length : right.length;
    const int comparison = common_length == 0 ? 0 : memcmp(left.data, right.data, common_length);

    if( comparison != 0 )
        return comparison;
    if( left.length < right.length )
        return -1;
    if( left.length > right.length )
        return 1;
    return 0;
}

avium_text avium_str(double value, unsigned line)
{
    const int length = snprintf(NULL, 0, "%.17g", value);
    if( length < 0 )
        text_error(line, "REAL արժեքը տեքստի փոխարկել չհաջողվեց։");

    const size_t size = (size_t)length + 1;
    char* data = malloc(size);
    if( data == NULL )
        text_error(line, "Տեքստի համար հիշողություն հատկացնել չհաջողվեց։");

    snprintf(data, size, "%.17g", value);
    return (avium_text){
        .data = data,
        .length = (size_t)length,
        .owned = true,
    };
}

double avium_num(avium_text value, unsigned line)
{
    avium_text copy = avium_text_copy(value, line);
    char* end = NULL;
    errno = 0;
    const double result = strtod(copy.data, &end);

    while( (size_t)(end - copy.data) < copy.length
        && isspace((unsigned char)*end) )
        ++end;

    const bool has_value = end != copy.data;
    const bool consumed = (size_t)(end - copy.data) == copy.length;
    const bool valid = errno != ERANGE && has_value && consumed;
    avium_text_destroy(&copy);

    if( !valid )
        text_error(line, "Տեքստը REAL արժեք չի ներկայացնում։");
    return result;
}
