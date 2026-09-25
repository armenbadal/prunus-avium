#include <stdio.h>

static void cerasus_word(int x)
{
    if( x == 0 )
        puts("zero");
    else if( x == 1 )
        puts("one");
}

int main()
{
    cerasus_word(0);

    int k = 1;
    cerasus_word(k);

    return 0;
}
