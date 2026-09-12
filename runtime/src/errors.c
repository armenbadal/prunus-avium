#include "errors.h"

#include <stdio.h>
#include <stdlib.h>

_Noreturn void avium_runtime_error(unsigned line, const char* message)
{
    fprintf(stderr, "%u: %s\n", line, message);
    exit(EXIT_FAILURE);
}
