#include "texts.h"

#include <stdbool.h>
#include <stddef.h>

size_t avium_test_text_data_offset(void)
{
    return offsetof(avium_text, data);
}

size_t avium_test_text_length_offset(void)
{
    return offsetof(avium_text, length);
}

size_t avium_test_text_owned_offset(void)
{
    return offsetof(avium_text, owned);
}

size_t avium_test_text_size(void)
{
    return sizeof(avium_text);
}

size_t avium_test_text_alignment(void)
{
    return _Alignof(avium_text);
}

size_t avium_test_bool_size(void)
{
    return sizeof(bool);
}

size_t avium_test_int_size(void)
{
    return sizeof(int);
}

size_t avium_test_unsigned_size(void)
{
    return sizeof(unsigned);
}

size_t avium_test_size_type_size(void)
{
    return sizeof(size_t);
}

size_t avium_test_pointer_size(void)
{
    return sizeof(void*);
}
