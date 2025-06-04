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
#include <math.h>

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

#ifndef MAX_FRAGMENTS
#   define MAX_FRAGMENTS 4
#endif

typedef MEMPOS mempos;

typedef struct {
    size_t size;
    mempos start;
} free_space;

typedef struct {
    bool initialized;

    // Memory Management
    byte vhd[MEM_SIZE];           // el VirtHD
    mempos data_offset;           // data storage section start

    // Object Management
    mempos obj_info_section;      // obj info section start
    mempos last_handle;           // latest obj handle
    mempos object_counter_index;  // obj counter index
    free_space spaces[MAX_FRAGMENTS];
    int fs_top;
} memory;

typedef union {
    memory m;
    char b[sizeof(memory)];
} memory_transfer;

typedef union {
    // byte b[sizeof(int)];
    int i;
} int_byte;

typedef union {
    // byte b[sizeof(mempos)];
    mempos i;
} mempos_byte;

// int_byte init_bg(int x) { int_byte b = {}; b.i = x; return b; }

static struct obj_header {
    int_byte id;
    mempos_byte offset;
    int_byte size;
} obj_default = { {.i = 0}, {.i = 0}, {.i = 0} };

#define obj_header struct obj_header

static void init_obj(const memory* mem, obj_header* dest, size_t size)
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

    DBGF("[%d]\n\tOFFSET:\t%d\n\tSIZE:\t%lu\n", mem->last_handle, offset.i, (unsigned long)size);
    // just HOPE that this shit is valid :pray:
}

static int push_header(memory* mem, size_t size) {
    assert(mem->obj_info_section + sizeof(obj_header) != MEM_SIZE / 4, "info section overflow");
    obj_header obj = obj_default;

    init_obj(mem, &obj, size);

    memcpy(mem->vhd + mem->obj_info_section, &obj, sizeof(obj));
    mem->obj_info_section += sizeof(obj_header);

    if(mem->vhd[mem->object_counter_index] == (1<<8) - 1) mem->object_counter_index++;
    // Memory overlap :!
    assert(mem->object_counter_index != 4, "object counter overflow");

    mem->vhd[mem->object_counter_index]++;

    return mem->last_handle++;
}

static void init(memory* mem)
{
    if(mem->initialized) return;
    
    mem->data_offset            = MEM_SIZE / 4;
    
    mem->obj_info_section       = INFO_START;
    mem->object_counter_index   = 0;
    mem->last_handle            = 0;

    mem->fs_top                 = 0;
    
    memset(mem->spaces, 0, MAX_FRAGMENTS * sizeof(free_space));
    memset(mem->vhd, 0, MEM_SIZE);

                                //  *might* introduce a 1 byte gap between obj and data sections
    mem->spaces[mem->fs_top].size = (size_t)floor(MEM_SIZE * (3./4.)),
    mem->spaces[mem->fs_top].start = MEM_SIZE / 4;
    
    mem->initialized            = true;
}

void pop_at(free_space* arr, int change, int fstop) {
    for(int i = change; i < fstop; ++i)
    {
        arr[i] = arr[i + 1];
    }

}

void update_space(memory* mem, mempos change, size_t changesize)
{
// don't trust this code right now, but dont wanna forget to implement it as well
#ifdef DISK_DEFRAG_IMPL
    if(mem->fs_top > 1)
    {
        // find weldable spaces
        int ptr = mem->fs_top;

        CUSTOM_STACK(MAX_FRAGMENTS, {int welder; int weldee;});

        while(ptr > 0)
        {
            if( ptr + 1 < mem->fs_top ) {
                // regular middle-member check
                mempos end_before   = mem->spaces[ptr - 1].size + mem->spaces[ptr - 1].start;
                mempos end_after    = mem->spaces[ptr + 1].size + mem->spaces[ptr + 1].start;
                mempos end_current  = mem->spaces[ptr].size + mem->spaces[ptr].start;
                
                free_space before   = mem->spaces[ptr - 1];
                free_space after    = mem->spaces[ptr + 1];
                free_space current  = mem->spaces[ptr];
                
                /***********************************THIS IS HERE SO I DONT FORGET**********************************************\
                Checks both directions just to garantee we didn't miss anyone, for example in a case where                     |
                Before fits in Current, but Current not in Before we simply continue and check it again when Before is Current |
                Visualization:                                                                                                 |
                ptr = 2                                                                                                        |
                [        [ [ B ] C ]        ] where B fits in C but C is currently being checked and does not fit in C         |
                ptr = 1                                                                                                        |
                [        [ [ C ] A ]        ] Now, when checking weldability for C to A, they fit!                             |
                \**************************************************************************************************************/
                if(end_current <= end_before && current.start >= before.start)
                {
                    // welds .weldee into .welder
                    STACK_PUSH(0, .welder = ptr - 1, .weldee = ptr);
                }
                else if(end_current <= end_after && current.start >= after.start)
                {
                    // welds .weldee into .welder
                    STACK_PUSH(0, .welder = ptr + 1, .weldee = ptr);
                }
            };
            ptr--;
        }

        // Still gotta check at spaces[0];
        
        if(mem->spaces[0].start >= mem->spaces[1].start)
        {
            // welds .weldee into .welder
            STACK_PUSH(0, .welder = ptr - 1, .weldee = ptr);
        }
    }
#endif // DISK_DEFRAG_IMPL

    // for(int i = 0; i < mem->fs_top; ++i)
    // {
    //     DBG("Space[%d]\n\tSIZE:\t%d\n\tSTART:\t%d\n", i, mem->spaces[i].size, mem->spaces[i].start)
    //     int curr_section_end = mem->spaces[i].start + mem->spaces[i].size;
    //     free_space curr_section = mem->spaces[i];
    //     if(curr_section_end > change)
    //     {
    //         mem[mem->fs_top++] = {.size = curr_section.size - }
    //     }
    // }
    assert(change <= mem->fs_top, "Free space cache overflow inside the library");

    // for now, just walk the section forward.
    if(mem->spaces[change].size - changesize == 0) {
        pop_at(mem->spaces, change, mem->fs_top);
        mem->fs_top--;
    };
    mem->spaces[change].size -= changesize;
    mem->spaces[change].start += changesize;
}

static mempos lowest_available_offset(memory* mem, size_t required_size)
{
    int ptr = mem->fs_top;
    mempos ret = -1;

    while(ptr >= 0)
    {
        DBGF("Space[%d]\n\tSIZE:\t%llu\n\tSTART:\t%d\n", ptr, mem->spaces[ptr].size, mem->spaces[ptr].start);
        if(mem->spaces[ptr].size >= required_size) {
            ret = mem->spaces[ptr].start;
            goto success;
        };
        ptr--;
    }
    goto fail;

success:
    update_space(mem, ptr, required_size);
    return ret;
fail:
    // If this fires somehow none were found (push() should check if the object fits)
    assert(0, "Memory overflow check inside the header failed somewhere");
    // return -1;
}

int push(memory* mem, const void* src, size_t srcsize) {
    assert(mem != NULL, "null memory param");

    if(!mem->initialized) init(mem);
    if(mem->data_offset + srcsize == MEM_SIZE) return -1;
    int id = -1;

    if((id = push_header(mem, srcsize)) == -1) return -1;

    memcpy(mem->vhd + lowest_available_offset(mem, srcsize), src, srcsize);
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

void find_obj(memory* mem, obj_header* dest, int handle)
{
    int obj_count = query_obj_count(mem);
    DBGF("Queried obj count of %d\n", obj_count);

    int id = -1;
    int dptr = INFO_START;    

    while(id != handle)
    {
        assert(id != obj_count, "invalid handle");

        memcpy(dest, mem->vhd + dptr, sizeof(obj_header));

        id = dest->id.i;
        dptr += sizeof(obj_header);
    }
}

void* get(memory* mem, int h)
{
    assert(mem != NULL, "null memory param");
    assert(mem->initialized, "query attempt on unitialized data");
    if(mem->vhd[mem->object_counter_index] == 0) return NULL;

    obj_header obj;

    find_obj(mem, &obj, h);

    DBGF("MEM[%d]\n\
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

void fs_stat(memory* mem)
{
    int ptr = mem->fs_top;

    DBGF("FS_SIZE: %d\n", mem->fs_top + 1);
    
    while(ptr >= 0)
    {
        DBGF("FS[%d]:\n\tSize: %2lu\tStart: %2d\n", ptr, (unsigned long)mem->spaces[ptr].size, mem->spaces[ptr].start);
        ptr--;
    }
}

void obj_stat(memory* mem)
{
    // int ptr = mem->fs_top;
    int objc = query_obj_count(mem);
    if(objc == 0) {
        DBG("Header section empty!");
        return;
    }

    DBGF("ObjCount: %d\n", objc);
    
    // for(int i = INFO_START; i <= mem->obj_info_section; i += sizeof(obj_header))
    for(mempos i = INFO_START, count = 0; i < mem->obj_info_section || count < objc; i += sizeof(obj_header), count++)
    {
        obj_header o;
        memcpy(&o, mem->vhd + i, sizeof(obj_header));

        DBGF("OBJ[%lld]:\n\tId: %2d\tSize: %2lu\tOffset: %2d\n", (i - INFO_START) / sizeof(obj_header), o.id.i, (unsigned long)o.size.i, o.offset.i);
    }
}

int query_header_offset(const memory* mem, const obj_header* o)
{
    const obj_header* copy = NULL;

    for(int i = INFO_START; i <= mem->obj_info_section; i += sizeof(obj_header))
    {
        copy = (obj_header*)(mem->vhd + i);
        if(copy->id.i == o->id.i) return i;
    }

    return -1;
}

void destroy_header(memory* mem, const obj_header* obj) {
    int offset = query_header_offset(mem, obj);
    assert(offset > 0, "Something went really bad");

    int copysz = sizeof(obj_header);

    for(int i = offset, counter = 0; counter <= query_obj_count(mem); i += copysz, ++counter)
    {
        // if(i + copysz == mem->obj_info_section) break;
        memcpy(mem->vhd + i, mem->vhd + i + copysz, copysz);
    }

    if(mem->vhd[mem->object_counter_index] == 0) mem->object_counter_index--;
    mem->vhd[mem->object_counter_index]--;
    mem->data_offset -= sizeof(obj_header);
}

void* destroy(memory* mem, int handle)
{
    obj_header obj;
    find_obj(mem, &obj, handle);
    
    void* ret = get(mem, handle);    
    if(ret == NULL) return NULL; // dont even bother resizing shit

    /*
        Might cause the vhd to fragment itself, but,
        The logic here would be that we push new open spaces through the top of the list
        And since find_lowest_offset_available starts from the top, it should find the most recent fragments
        Consequence of this is that the resizing will likely be with an item of a different size
        Which instead of taking up the space, will just break it into a smaller piece 
    */
    mem->spaces[++mem->fs_top].size = obj.size.i;
    mem->spaces[mem->fs_top].start = obj.offset.i;

    destroy_header(mem, &obj);
    // try_weld(mem);
    
    //! Still need to impl deletion of object headers!!
    //! For now cleanup only happens in data-space

    return ret;
}

#endif // SERIZZ_H