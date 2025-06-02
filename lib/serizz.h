/*
||  A   |   | -> 4 first bytes are the object counter index
|     B     | -> first 1/4 - 4 bytes are reserved for object headers
|===========|
|           |
|           |
|     D     | -> All the rest reserved for data storage
|           |
|===========|
*/


#ifndef SERIZZ_H
#define SERIZZ_H

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include <string.h>
#include "util.h"

#ifndef MEM_SIZE
#   define MEM_SIZE 1024
#endif

#define INFO_START 4 // 4 Bytes reserved for object_counter_index

typedef struct {
    bool initialized;

    // Memory Management
    byte vhd[MEM_SIZE];                 // el VirtHD
    unsigned int data_offset;           // data storage section start

    // Object Management
    unsigned int obj_info_section;      // obj info section start
    unsigned int last_handle;           // latest obj handle
    unsigned int object_counter_index;  // obj counter index
} memory;

typedef union {
    memory m;
    char b[sizeof(memory)];
} memory_transfer;

typedef union {
    byte b[4];
    int i;
} byte_group;

// byte_group init_bg(int x) { byte_group b = {}; b.i = x; return b; }

static struct obj_header {
    byte_group id;
    byte_group offset;
    byte_group size;
} obj_default = { {.i = 0}, {.i = 0}, {.i = 0} };

static void init_obj(const memory* mem, struct obj_header* dest, size_t size)
{
    DBG("OFFSET: \t");
    const byte_group offset = { .i = (int) mem->data_offset };
    memcpy(&dest->offset, &offset, 4 * sizeof(byte));

    DBG("SIZE: \t\t");
    const byte_group _size = { .i = (int) size };
    memcpy(&dest->size, &_size, 4 * sizeof(byte));

    DBG("ID: \t\t");
    const byte_group objid = { .i = (int) mem->last_handle };
    memcpy(&dest->id, &objid, 4 * sizeof(byte));

    DBG("[%d]\n\tOFFSET:\t%d\n\tSIZE:\t%d\n", last_handle, data_offset, size);
    // just HOPE that this shit is valid :pray:
}

static int push_header(memory* mem, size_t size) {
    assert(mem->obj_info_section + sizeof(struct obj_header) != MEM_SIZE / 4, "info section overflow");
    struct obj_header obj = obj_default;

    init_obj(mem, &obj, size);

    memcpy(mem->vhd + mem->obj_info_section, &obj, sizeof(obj));
    mem->obj_info_section += sizeof(struct obj_header);

    if(mem->vhd[mem->object_counter_index] == (1<<8) - 1) mem->object_counter_index++;
    // Memory overlap :!
    assert(mem->object_counter_index != 4, "object counter overflow");

    mem->vhd[mem->object_counter_index]++;

    return mem->last_handle++;
}

static void init(memory* mem)
{
    if(mem->initialized) return;
    memset(mem->vhd, 0, MEM_SIZE);
    mem->data_offset = MEM_SIZE / 4;
    mem->obj_info_section = INFO_START;
    mem->last_handle = 0;
    mem->object_counter_index = 0;
    mem->initialized = true;
}

int push(memory* mem, const void* src, size_t srcsize) {
    if(!mem->initialized) init(mem);
    if(mem->data_offset + srcsize == MEM_SIZE) return -1;
    int id = -1;

    if((id = push_header(mem, srcsize)) == -1) return -1;

    memcpy(mem->vhd + mem->data_offset, src, srcsize);
    mem->data_offset += srcsize;

    return id;
}

int query_obj_count(memory* mem) {
    int objc = 0;
    if(mem->object_counter_index != 0) {
        int x = mem->object_counter_index;
        while(x)
        {
            objc += mem->vhd[x--];
        }
    } else objc += mem->vhd[0];

    return objc;
}

void* get(memory* mem, int h)
{
    assert(mem->initialized, "query attempt on unitialized data");
    if(mem->vhd[mem->object_counter_index] == 0) return NULL;

    int obj_count = query_obj_count(mem);
    DBG("Queried obj count of %d\n", obj_count);

    int id = -1;
    int dptr = INFO_START;
    struct obj_header obj;

    while(id != h)
    {
        assert(id != obj_count, "invalid handle");

        memcpy(&obj, mem->vhd + dptr, sizeof(struct obj_header));

        id = obj.id.i;
        dptr += sizeof(struct obj_header);
    }

    DBG("MEM[%d]\n\
            \tOFFSET:\t%d\n\
            \tSIZE:\t%d\n",
        obj.id.i, obj.offset.i, obj.size.i);

    void* tmpret = malloc(obj.size.i);
    assert(tmpret != NULL, "memory allocator fault");

    memcpy(tmpret, mem->vhd + obj.offset.i, obj.size.i);
    return tmpret;
}

void write(const memory* mem, const char* path)
{
    FILE* f = NULL;

    f = fopen(path, "wb");
    assert(f != NULL, "Invalid path");
    memory_transfer mt = {.m = *mem};

    fwrite(mt.b, sizeof(byte), sizeof(memory), f);
    fclose(f);
}

void read(memory* mem, const char* path)
{
    FILE* f = NULL;

    f = fopen(path, "rb");
    assert(f != NULL, "Invalid path");

    memory_transfer mt;

    fread(mt.b, sizeof(byte), sizeof(memory), f);
    *mem = mt.m;
    fclose(f);
}

#endif // SERIZZ_H