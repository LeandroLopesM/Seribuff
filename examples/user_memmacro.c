#include <stdlib.h>
#include <stdio.h>

#include "../lib/util.h"
#include "../lib/seribuff.h"

MEMORY(mem)

typedef struct {
    int age;
    char name[64];
    int cpf;
} user;


int main() {
    user u = (user){0, "Liam Nielson", 01010101};

    int handle = mem_push(&u, sizeof(user));
    printf("HANDLE: %d\n", handle);

    user *ut = (user *)mem_get(handle);
    assert(ut != NULL, "Failed to find user");

    printf("NAME: %s\n", ut->name);
    
    free(ut);
    
    return 0;
}