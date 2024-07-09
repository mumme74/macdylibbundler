#include <stdio.h>
#include "bar.h"


#define EXPORT __attribute__((visibility("default")))

EXPORT
void bar(void)
{
    puts("From rpathlib\n");
    printf("rpathlib %s in %s", __func__, __FILE__);
}