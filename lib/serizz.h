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

static struct obj_header {
    byte id[4];
    byte offset[4];
    byte size[4];
} obj_default = { {0}, {0}, {0} };

static void init_obj(memory* mem, struct obj_header* dest, size_t size)
{
    DBG("OFFSET: \t");
    const byte* offset = to_byte(mem->data_offset);
    memcpy(&dest->offset, offset, 4 * sizeof(byte));

    DBG("SIZE: \t\t");
    const byte* _size = to_byte(size);
    memcpy(&dest->size, _size, 4 * sizeof(byte));

    DBG("ID: \t\t");
    const byte* objid = to_byte(mem->last_handle);
    memcpy(&dest->id, objid, 4 * sizeof(byte));

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

void* get(memory* mem, int h)
{
    assert(mem->initialized, "query attempt on unitialized data");
    if(mem->vhd[mem->object_counter_index] == 0) return NULL;
    int obj_count = 0;

    if(mem->object_counter_index != 0) {
        int x = mem->object_counter_index;
        while(x)
        {
            obj_count += mem->vhd[x--];
        }
    } else obj_count += mem->vhd[0];

    DBG("Queried obj count of %d\n", obj_count);
    int id = -1;
    int dptr = INFO_START;

    struct obj_header obj;
    while(id != h)
    {
        assert(id != obj_count, "invalid handle");

        memcpy(&obj, mem->vhd + dptr, sizeof(struct obj_header));

        id = from_byte(obj.id);
        dptr += sizeof(struct obj_header);
    }
    DBG("[%d]\n\tOFFSET:\t%d\n\tSIZE:\t%d\n", from_byte(obj.id), from_byte(obj.offset), from_byte(obj.size));

    void* tmpret = malloc(from_byte(obj.size));
    assert(tmpret != NULL, "memory allocator fault");

    memcpy(tmpret, mem->vhd + from_byte(obj.offset), from_byte(obj.size));
    return tmpret;
}

#endif // SERIZZ_H