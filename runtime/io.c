#include "io.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void io_error(unsigned line, const char* message)
{
    fprintf(stderr, "%u: %s\n", line, message);
    exit(EXIT_FAILURE);
}

void avium_print_bool(bool value)
{
    puts(value ? "TRUE" : "FALSE");
}

void avium_print_real(double value)
{
    printf("%.17g\n", value);
}

void avium_print_text(avium_text value)
{
    if( value.length != 0 )
        fwrite(value.data, 1, value.length, stdout);
    fputc('\n', stdout);
}

avium_text avium_input(unsigned line)
{
    size_t capacity = 64;
    size_t length = 0;
    char* data = malloc(capacity);
    if( data == NULL )
        io_error(line, "Ներմուծվող տեքստի համար հիշողություն հատկացնել չհաջողվեց։");

    int character = 0;
    while( (character = fgetc(stdin)) != '\n' && character != EOF ) {
        if( length + 1 == capacity ) {
            if( capacity > SIZE_MAX / 2 ) {
                free(data);
                io_error(line, "Ներմուծվող տեքստը չափազանց երկար է։");
            }
            capacity *= 2;
            char* larger = realloc(data, capacity);
            if( larger == NULL ) {
                free(data);
                io_error(line, "Ներմուծվող տեքստի համար հիշողություն հատկացնել չհաջողվեց։");
            }
            data = larger;
        }
        data[length++] = (char)character;
    }

    if( ferror(stdin) ) {
        free(data);
        io_error(line, "Տեքստը ներմուծել չհաջողվեց։");
    }

    if( length != 0 && data[length - 1] == '\r' )
        --length;
    data[length] = '\0';
    return (avium_text){
        .data = data,
        .length = length,
        .owned = true,
    };
}
