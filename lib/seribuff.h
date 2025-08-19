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
#include <stdint.h>
#include <ctype.h>
#include <stdio.h>

#include <string.h>
#include "util.h"

#ifndef MEM_SIZE
#   define MEM_SIZE 1024
#endif

#if MEM_SIZE < 256
#   define MEMPOS uint8_t
#elif MEM_SIZE < 65536
#   define MEMPOS uint16_t
#elif MEM_SIZE < 4294967296
#   define MEMPOS uint32_t
#elif MEM_SIZE < 18446744073709551616
#   define MEMPOS uint64_t
#endif

#ifndef INFO_START
#   define INFO_START 4 // 4 Bytes reserved for object_counter_index
#endif // INFO_START

typedef MEMPOS mempos;

typedef struct {
    bool initialized;

    // Memory Management
    byte vhd[MEM_SIZE];           // el VirtHD
    mempos data_offset;           // data storage section start

    // Object Management
    mempos obj_info_section;      // obj info section start
    mempos last_handle;           // latest obj handle
    mempos object_counter_index;  // obj counter index
} memory;

typedef union {
    memory m;
    char b[sizeof(memory)];
} memory_transfer;

typedef union {
    byte b[4];
    int i;
} int_byte;

typedef union {
    byte b[sizeof(mempos)];
    mempos i;
} mempos_byte;

// int_byte init_bg(int x) { int_byte b = {}; b.i = x; return b; }

static struct obj_header {
    int_byte id;
    mempos_byte offset;
    int_byte size;
} obj_default = { {.i = 0}, {.i = 0}, {.i = 0} };

static void init_obj(const memory* mem, struct obj_header* dest, size_t size)
{
    DBG("OFFSET: \t");
    const mempos_byte offset = { .i = mem->data_offset };
    memcpy(&dest->offset, &offset, sizeof(mempos) * sizeof(byte));

    DBG("SIZE: \t\t");
    const int_byte _size = { .i = (int) size };
    memcpy(&dest->size, &_size, 4 * sizeof(byte));

    DBG("ID: \t\t");
    const int_byte objid = { .i = (int) mem->last_handle };
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
    assert(mem != NULL, "null memory param");

    if(!mem->initialized) init(mem);
    if(mem->data_offset + srcsize == MEM_SIZE) return -1;
    int id = -1;

    if((id = push_header(mem, srcsize)) == -1) return -1;

    memcpy(mem->vhd + mem->data_offset, src, srcsize);
    mem->data_offset += srcsize;

    return id;
}

static int query_obj_count(memory* mem) {
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

int size(memory* mem) {
    return query_obj_count(mem);
}

void* get(memory* mem, int h)
{
    assert(mem != NULL, "null memory param");
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
    assert(mem != NULL, "null memory param");
    FILE* f = NULL;

    f = fopen(path, "wb");
    assert(f != NULL, "Invalid path");
    memory_transfer mt = {.m = *mem};

    fwrite(mt.b, sizeof(byte), sizeof(memory), f);
    fclose(f);
}

void read(memory* mem, const char* path)
{
    assert(mem != NULL, "null memory param");
    FILE* f = NULL;

    f = fopen(path, "rb");
    assert(f != NULL, "Invalid path");

    memory_transfer mt;

    fread(mt.b, sizeof(byte), sizeof(memory), f);
    *mem = mt.m;
    fclose(f);
}

void clear(memory* mem)
{
    assert(mem != NULL, "null memory param");

    mem->initialized = false;
    init(mem);
}

#endif // SERIZZ_H
