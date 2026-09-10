#ifndef AVIUM_RUNTIME_TEXTS_H
#define AVIUM_RUNTIME_TEXTS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct avium_text {
    const char* data;
    size_t length;
    bool owned;
} avium_text;

avium_text avium_text_create(const char* data, size_t length, unsigned line);
avium_text avium_text_copy(avium_text value, unsigned line);
void avium_text_destroy(avium_text* value);
void avium_text_move_assign(avium_text* target, avium_text* source);
avium_text avium_text_concat(avium_text left, avium_text right, unsigned line);
int avium_text_compare(avium_text left, avium_text right);
avium_text avium_str(double value, unsigned line);
double avium_num(avium_text value, unsigned line);

#endif /* AVIUM_RUNTIME_TEXTS_H */
