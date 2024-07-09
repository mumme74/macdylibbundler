#include <stdio.h>
#include "foo.h"

#define EXPORT __attribute__((visibility("default")))

EXPORT
void foo(void)
{
    puts("From lpathlib\n");
    printf("lpathlib %s in %s", __func__, __FILE__);
}