#include "io.h"

#include "errors.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void finish_output(unsigned line)
{
    if( fputc('\n', stdout) == EOF || fflush(stdout) == EOF )
        avium_runtime_error(line, "Արժեքն արտածել չհաջողվեց։");
}

void avium_print_bool(bool value, unsigned line)
{
    if( fputs(value ? "TRUE" : "FALSE", stdout) == EOF )
        avium_runtime_error(line, "Արժեքն արտածել չհաջողվեց։");
    finish_output(line);
}

void avium_print_real(double value, unsigned line)
{
    if( fprintf(stdout, "%.17g", value) < 0 )
        avium_runtime_error(line, "Արժեքն արտածել չհաջողվեց։");
    finish_output(line);
}

void avium_print_text(avium_text value, unsigned line)
{
    if( value.length != 0 && fwrite(value.data, 1, value.length, stdout) != value.length )
        avium_runtime_error(line, "Արժեքն արտածել չհաջողվեց։");
    finish_output(line);
}

avium_text avium_input(unsigned line)
{
    size_t capacity = 64;
    size_t length = 0;
    char* data = malloc(capacity);
    if( data == NULL )
        avium_runtime_error(line, "Ներմուծվող տեքստի համար հիշողություն հատկացնել չհաջողվեց։");

    int character = 0;
    while( (character = fgetc(stdin)) != '\n' && character != EOF ) {
        if( length + 1 == capacity ) {
            if( capacity > SIZE_MAX / 2 ) {
                free(data);
                avium_runtime_error(line, "Ներմուծվող տեքստը չափազանց երկար է։");
            }
            capacity *= 2;
            char* larger = realloc(data, capacity);
            if( larger == NULL ) {
                free(data);
                avium_runtime_error(line, "Ներմուծվող տեքստի համար հիշողություն հատկացնել չհաջողվեց։");
            }
            data = larger;
        }
        data[length++] = (char)character;
    }

    if( ferror(stdin) ) {
        free(data);
        avium_runtime_error(line, "Տեքստը ներմուծել չհաջողվեց։");
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
