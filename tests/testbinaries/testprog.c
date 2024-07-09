/**
* Only intended to create a test binary we can test MachO lib 
* against. We should get a x86, arm64
* out of this one.
*/

#include <stdio.h>
#include <stdlib.h>
#include "barlib/bar.h"
#include "foolib/foo.h"

int main(void) {
    puts("From test program!");
    bar();
    foo();
    return 0;
}