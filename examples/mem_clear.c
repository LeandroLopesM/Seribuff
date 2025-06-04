#include <stdlib.h>
#include <stdio.h>

#include "../lib/util.h"
#include "../lib/serizz.h"

MEMORY(c)

typedef struct {
    int secrets[64];
} stuff;


int main() {
    stuff super_secret_stuff;
    memset(super_secret_stuff.secrets, 69, sizeof(int) * 64);

    int handle = c_push(&super_secret_stuff, sizeof(stuff));

    c_clear();

    stuff* this_nulls = (stuff*)c_get(handle);

    assert(this_nulls != NULL, "Seems the memory clear cleared the memory!!");

    printf("This should be unreachable!!!!\n");
    free(this_nulls); // just in case ;)
    return 0;
}