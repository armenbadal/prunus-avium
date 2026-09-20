#ifndef AVIUM_RUNTIME_TEXTS_H
#define AVIUM_RUNTIME_TEXTS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct _avium_text {
    const char* data;
    size_t length;
    bool owned;
} avium_text;

void avium_text_create(avium_text* result, const char* data, size_t length,
    unsigned line);
void avium_text_copy(avium_text* result, const avium_text* value, unsigned line);
void avium_text_destroy(avium_text* value);
void avium_text_move_assign(avium_text* target, avium_text* source);
void avium_text_concat(avium_text* result, const avium_text* left,
    const avium_text* right, unsigned line);
int avium_text_compare(const avium_text* left, const avium_text* right);
void avium_str(avium_text* result, double value, unsigned line);
void avium_str_bool(avium_text* result, bool value, unsigned line);
double avium_num(const avium_text* value, unsigned line);
double avium_text_length(const avium_text* value);

#endif /* AVIUM_RUNTIME_TEXTS_H */
