#include <stdlib.h>
#include <stdio.h>

#include "../lib/util.h"
#include "../lib/serizz.h"

memory mem;

typedef struct {
    char name[64];
    char content[128];
    int flags;
} file;


int main() {
    FILE* a = fopen("../files.sav", "r");
    assert(a != NULL, "Example disk_write.c must be ran before this one!");
    fclose(a);
    
    read(&mem, "../files.sav");    
    file* files[3] = {
        (file*)get(&mem, 0),
        (file*)get(&mem, 1),
        (file*)get(&mem, 2)
    };

    printf("Files:\n");
    printf("    %s\n", files[0]->name);
    printf("    %s\n", files[1]->name);
    printf("    %s\n", files[2]->name);

    free(files[0]);
    free(files[1]);
    free(files[2]);
    return 0;
}