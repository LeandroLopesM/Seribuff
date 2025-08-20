#include <stdlib.h>
#include <stdio.h>

#include "../lib/util.h"
#include "../lib/seribuff.h"

memory mem;

typedef struct {
    char name[64];
    char content[128];
    int flags;
} file;


int main() {
    {
        file u = (file){"main.c", "int main(){printf(\"hai\"); return 0;}", 0xf00f};

        int handle = push(&mem, &u, sizeof(file));
        printf("HANDLE: %d\n", handle);    
    }
    {
        file u = (file){"main.py", "print('hai')", 0xf00f};

        int handle = push(&mem, &u, sizeof(file));
        printf("HANDLE: %d\n", handle);    
    }
    {
        file u = (file){"main.js", "console.log('hai')", 0xf00f};

        int handle = push(&mem, &u, sizeof(file));
        printf("HANDLE: %d\n", handle);    
    }

    write(&mem, "../files.sav");
    return 0;
}