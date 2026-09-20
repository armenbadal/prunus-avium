#include "io.h"
#include "texts.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static void verify_signatures(void)
{
    void (*const create)(avium_text*, const char*, size_t, unsigned) =
        avium_text_create;
    void (*const copy)(avium_text*, const avium_text*, unsigned) =
        avium_text_copy;
    void (*const destroy)(avium_text*) = avium_text_destroy;
    void (*const move_assign)(avium_text*, avium_text*) =
        avium_text_move_assign;
    void (*const concat)(avium_text*, const avium_text*, const avium_text*,
        unsigned) = avium_text_concat;
    int (*const compare)(const avium_text*, const avium_text*) =
        avium_text_compare;
    void (*const str)(avium_text*, double, unsigned) = avium_str;
    void (*const str_bool)(avium_text*, bool, unsigned) = avium_str_bool;
    double (*const num)(const avium_text*, unsigned) = avium_num;
    double (*const text_length)(const avium_text*) = avium_text_length;
    void (*const print_text)(const avium_text*, unsigned) = avium_print_text;
    void (*const input)(avium_text*, unsigned) = avium_input;

    (void)create;
    (void)copy;
    (void)destroy;
    (void)move_assign;
    (void)concat;
    (void)compare;
    (void)str;
    (void)str_bool;
    (void)num;
    (void)text_length;
    (void)print_text;
    (void)input;
}

int main(void)
{
    verify_signatures();

    avium_text hello;
    avium_text_create(&hello, "Hello", 5, 1);
    assert(hello.length == 5);
    assert(hello.owned);
    assert(memcmp(hello.data, "Hello", 5) == 0);

    avium_text copy;
    avium_text_copy(&copy, &hello, 2);
    assert(copy.data != hello.data);
    assert(avium_text_compare(&copy, &hello) == 0);

    const avium_text suffix = {"!", 1, false};
    avium_text greeting;
    avium_text_concat(&greeting, &hello, &suffix, 3);
    assert(greeting.length == 6);
    assert(memcmp(greeting.data, "Hello!", 6) == 0);
    assert(avium_text_length(&greeting) == 6.0);

    avium_text number;
    avium_str(&number, 3.5, 4);
    assert(number.length == 3);
    assert(memcmp(number.data, "3.5", 3) == 0);
    assert(avium_num(&number, 5) == 3.5);

    avium_text boolean;
    avium_str_bool(&boolean, true, 6);
    assert(boolean.length == 4);
    assert(memcmp(boolean.data, "TRUE", 4) == 0);
    avium_text_destroy(&boolean);
    avium_str_bool(&boolean, false, 7);
    assert(boolean.length == 5);
    assert(memcmp(boolean.data, "FALSE", 5) == 0);

    avium_text target = {"", 0, false};
    avium_text_move_assign(&target, &greeting);
    assert(target.length == 6);
    assert(target.owned);
    assert(greeting.length == 0);
    assert(!greeting.owned);

    avium_text_destroy(&target);
    avium_text_destroy(&boolean);
    avium_text_destroy(&number);
    avium_text_destroy(&copy);
    avium_text_destroy(&hello);
    return 0;
}
