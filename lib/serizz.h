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

#ifndef VHD_SIZE
#   define VHD_SIZE 1024
#endif

#if VHD_SIZE < 256
#   define MEMPOS uint8_t
#elif VHD_SIZE < 65536
#   define MEMPOS uint16_t
#elif VHD_SIZE < 4294967296
#   define MEMPOS uint32_t
#elif VHD_SIZE < 18446744073709551616
#   define MEMPOS uint64_t
#endif

#ifndef OBJ_SECTION_START
#   define OBJ_SECTION_START 4 // 4 Bytes reserved for object_counter_index
#endif // OBJ_SECTION_START

#ifndef MAX_FRAGMENTS
#   define MAX_FRAGMENTS 4
#endif

typedef MEMPOS mempos_t;

typedef struct {
    size_t size;
    mempos_t start;
} free_space;

typedef struct {
    bool initialized;

    // Memory Management
    byte vhd[VHD_SIZE];             // el VirtHD
    
    mempos_t data_offset;           // data storage section start

    // Object Management
    mempos_t obj_info_section;      // obj header storage start
    mempos_t last_handle;           // latest obj handle
    mempos_t object_counter_index;  // obj counter index
    free_space fragments[MAX_FRAGMENTS];
    int free_space_count;
} memory;

typedef union {
    char b[sizeof(memory)];
    memory m;
} memory_byte;

typedef union {
    byte b[sizeof(int)];
    int i;
} int_byte;

typedef union {
    byte b[sizeof(mempos_t)];
    mempos_t i;
} mempos_byte;


static struct obj_header {
    int_byte    id;     // Unique ID for the object
    mempos_byte offset; // Object offset inside the VHD
    int_byte    size;   // Object size inside the VHD
    bool        valid; // Here to prevent access to headers invalidaded due to deletion
} obj_default = { {.i = 0}, {.i = 0}, {.i = 0}, false };

#define obj_header struct obj_header

/****************************** FUNCTIONS *******************************/

int     push(memory* mem, const void* src, size_t srcsize);         // pushes an elemento to mem.vhd
int     size(memory* mem);                                          // Returns the count of items in mem.vhd
void*   get(memory* mem, int h);                                    // Copies the element with handle h to heap, returns a ptr to it
void    read(memory* mem, const char* path);                        // Reads path to mem
void    write(const memory* mem, const char* path);                 // writes mem to disk, to be used in conjunction with read
void    clear(memory* mem);                                         // Fully clears mem.vhd
void*   destroy(memory* mem, int handle);                           // Deletes header and element in mem.vhd with handle
void    fs_stat(memory* mem);                                       // Displays the current status of the free fragments in mem.vhd (fragments)
void    obj_stat(memory* mem);                                      // Displays the current status of object headers 
void    find_obj(memory* mem, obj_header* dest, int handle);        // Finds obj with handle in mem, copies it to dest

static int      query_header_offset(const memory* mem, const obj_header* o);        // Finds the object offset of o in mem
static int      query_obj_count(memory* mem);                                       // Fetches the obj count from first 4 bytes of mem.vhd
static int      push_header(memory* mem, size_t size);                              // Creates a new header and commits it to the obj section of mem.vhd
static void     init_obj_header(const memory* mem, obj_header* dest, size_t size);  // Initiates the obj_header structure
static void     destroy_header(memory* mem, const obj_header* obj);                 // Deletes the header, popping all the ones in front back by a slot
static void     init(memory* mem);                                                  // Initiates the memory structure
static void     pop_at(free_space* arr, int change, int fstop);                     // removes the element
static void     update_space(memory* mem, mempos_t change, size_t changesize);      // Deletes the changed free space
static mempos_t lowest_available_offset(memory* mem, size_t required_size);         // Finds the lowest available offset in the list of free_spaces

/****************************** IMPLEMENTATIONS *******************************/

static void init_obj_header(const memory* mem, obj_header* dest, size_t size)
{
    const mempos_byte offset = { .i = mem->data_offset };
    memcpy(&dest->offset, &offset, sizeof(mempos_t) * sizeof(byte));

    const int_byte _size = { .i = (int) size };
    memcpy(&dest->size, &_size, 4 * sizeof(byte));

    const int_byte objid = { .i = (int) mem->last_handle };
    memcpy(&dest->id, &objid, 4 * sizeof(byte));

    dest->valid = true;
    DBGF("\n\t[%d]\n\t\tOFFSET:\t\t%d\n\t\tSIZE:\t\t%lu\n", mem->last_handle, offset.i, (unsigned long)size);
    // just HOPE that this shit is valid :pray:
}

static int push_header(memory* mem, size_t size)
{
    ASSERT(mem->obj_info_section + sizeof(obj_header) != VHD_SIZE / 4, "info section overflow");
    obj_header obj = obj_default;

    init_obj_header(mem, &obj, size);

    memcpy(mem->vhd + mem->obj_info_section, &obj, sizeof(obj));
    mem->obj_info_section += sizeof(obj_header);

    if(mem->vhd[mem->object_counter_index] == (1<<8) - 1) mem->object_counter_index++;
    // Memory overlap :!
    ASSERT(mem->object_counter_index != 4, "object counter overflow");

    mem->vhd[mem->object_counter_index]++;

    return mem->last_handle++;
}

static void init(memory* mem)
{
    if(mem->initialized) return;
    
    mem->data_offset            = VHD_SIZE / 4;
    
    mem->obj_info_section       = OBJ_SECTION_START;
    mem->object_counter_index   = 0;
    mem->last_handle            = 0;

    mem->free_space_count                 = 0;
    
    memset(mem->fragments, 0, MAX_FRAGMENTS * sizeof(free_space));
    memset(mem->vhd, 0, VHD_SIZE);

                                //  *might* introduce a 1 byte gap between obj and data sections
    mem->fragments[mem->free_space_count].size = (size_t)floor(VHD_SIZE * (3./4.)),
    mem->fragments[mem->free_space_count].start = VHD_SIZE / 4;
    
    mem->initialized            = true;
}

static void pop_at(free_space* arr, int change, int fstop)
{
    for(int i = change; i < fstop; ++i)
    {
        arr[i] = arr[i + 1];
    }

}

static void update_space(memory* mem, mempos_t change, size_t changesize)
{
// don't trust this code right now, but dont wanna forget to implement it as well
#ifdef DISK_DEFRAG_IMPL
    if(mem->free_space_count > 1)
    {
        // find weldable fragments
        int ptr = mem->free_space_count;

        CUSTOM_STACK(MAX_FRAGMENTS, {int welder; int weldee;});

        while(ptr > 0)
        {
            if( ptr + 1 < mem->free_space_count ) {
                // regular middle-member check
                mempos_t end_before   = mem->fragments[ptr - 1].size + mem->fragments[ptr - 1].start;
                mempos_t end_after    = mem->fragments[ptr + 1].size + mem->fragments[ptr + 1].start;
                mempos_t end_current  = mem->fragments[ptr].size + mem->fragments[ptr].start;
                
                free_space before   = mem->fragments[ptr - 1];
                free_space after    = mem->fragments[ptr + 1];
                free_space current  = mem->fragments[ptr];
                
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

        // Still gotta check at fragments[0];
        
        if(mem->fragments[0].start >= mem->fragments[1].start)
        {
            // welds .weldee into .welder
            STACK_PUSH(0, .welder = ptr - 1, .weldee = ptr);
        }
    }
#endif // DISK_DEFRAG_IMPL

    // for(int i = 0; i < mem->free_space_count; ++i)
    // {
    //     DBG("Space[%d]\n\tSIZE:\t%d\n\tSTART:\t%d\n", i, mem->fragments[i].size, mem->fragments[i].start)
    //     int curr_section_end = mem->fragments[i].start + mem->fragments[i].size;
    //     free_space curr_section = mem->fragments[i];
    //     if(curr_section_end > change)
    //     {
    //         mem[mem->free_space_count++] = {.size = curr_section.size - }
    //     }
    // }
    ASSERT(change <= mem->free_space_count, "Free space cache overflow inside the library");

    // for now, just walk the section forward.
    if(mem->fragments[change].size - changesize == 0) {
        pop_at(mem->fragments, change, mem->free_space_count);
        mem->free_space_count--;
    };
    mem->fragments[change].size -= changesize;
    mem->fragments[change].start += changesize;
}

static mempos_t lowest_available_offset(memory* mem, size_t required_size)
{
    int ptr = mem->free_space_count;
    mempos_t ret = -1;

    while(ptr >= 0)
    {
        DBGF("\FPL[%d]\n\t\tSIZE:\t%llu\n\t\tSTART:\t%d\n", ptr, mem->fragments[ptr].size, mem->fragments[ptr].start);
        if(mem->fragments[ptr].size >= required_size) {
            ret = mem->fragments[ptr].start;
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
    ASSERT(0, "Memory overflow check inside the header failed somewhere");
    // return -1;
}

int push(memory* mem, const void* src, size_t srcsize)
{
    SOFT_ASSERT(mem != NULL, "null memory param", return -1);
    if(!mem->initialized) init(mem);
    
    SOFT_ASSERT(mem->data_offset + srcsize == VHD_SIZE, "VHD Full" , return -1;);
    int id = -1;

    if((id = push_header(mem, srcsize)) == -1) return -1;

    memcpy(mem->vhd + lowest_available_offset(mem, srcsize), src, srcsize);
    mem->data_offset += srcsize;
    return id;
}

static int query_obj_count(memory* mem)
{
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

int size(memory* mem)
{
    return query_obj_count(mem);
}

void find_obj(memory* mem, obj_header* dest, int handle)
{
    int obj_count = query_obj_count(mem);
    DBGF("Queried obj count of %d\n", obj_count);

    int id = -1;
    int dptr = OBJ_SECTION_START;    

    while(id != handle)
    {
        ASSERT(id != obj_count, "invalid handle");

        memcpy(dest, mem->vhd + dptr, sizeof(obj_header));

        id = dest->id.i;
        dptr += sizeof(obj_header);
    }

    ASSERT(!dest->valid, "Attempt to fetch a destroyed object");
}

void* get(memory* mem, int h)
{
    ASSERT(mem != NULL, "null memory param");
    ASSERT(mem->initialized, "query attempt on unitialized data");
    if(mem->vhd[mem->object_counter_index] == 0) return NULL;

    obj_header obj;

    find_obj(mem, &obj, h);

DBGF("\
MEM[%d]\n\
    \tOFFSET:\t%d\n\
    \tEXISTS:\t%d\n\
    \tSIZE:\t%d\n",
        obj.id.i, obj.offset.i, obj.valid, obj.size.i);

    void* tmpret = malloc(obj.size.i);
    ASSERT(tmpret != NULL, "memory allocator fault");

    memcpy(tmpret, mem->vhd + obj.offset.i, obj.size.i);
    return tmpret;
}

void write(const memory* mem, const char* path)
{
    ASSERT(mem != NULL, "null memory param");
    FILE* f = NULL;

    f = fopen(path, "wb");
    ASSERT(f != NULL, "Invalid path");
    memory_byte mt = {.m = *mem};

    fwrite(mt.b, sizeof(byte), sizeof(memory), f);
    fclose(f);
}

void read(memory* mem, const char* path)
{
    ASSERT(mem != NULL, "null memory param");
    FILE* f = NULL;

    f = fopen(path, "rb");
    ASSERT(f != NULL, "Invalid path");

    memory_byte mt;

    fread(mt.b, sizeof(byte), sizeof(memory), f);
    *mem = mt.m;
    fclose(f);
}

void clear(memory* mem)
{
    ASSERT(mem != NULL, "null memory param");

    mem->initialized = false;
    init(mem);
}

void fs_stat(memory* mem)
{
    int ptr = mem->free_space_count;

    DBGF("FS_SIZE: %d\n", mem->free_space_count + 1);
    
    while(ptr >= 0)
    {
        DBGF("FS[%d]:\n\tSize: %2lu\tStart: %2d\n", ptr, (unsigned long)mem->fragments[ptr].size, mem->fragments[ptr].start);
        ptr--;
    }
}

void obj_stat(memory* mem)
{
    // int ptr = mem->free_space_count;
    int objc = query_obj_count(mem);
    if(objc == 0) {
        DBG("Header section empty!");
        return;
    }

    DBGF("ObjCount: %d\n", objc);
    
    // for(int i = OBJ_SECTION_START; i <= mem->obj_info_section; i += sizeof(obj_header))
    for(mempos_t i = OBJ_SECTION_START, count = 0; i < mem->obj_info_section || count < objc; i += sizeof(obj_header), count++)
    {
        obj_header o;
        memcpy(&o, mem->vhd + i, sizeof(obj_header));
        if(!o.valid) continue;

        DBGF("OBJ[%lld]:\n\tId: %2d\tSize: %2lu\tOffset: %2d\n", (i - OBJ_SECTION_START) / sizeof(obj_header), o.id.i, (unsigned long)o.size.i, o.offset.i);
    }
}

static int query_header_offset(const memory* mem, const obj_header* o)
{
    const obj_header* copy = NULL;
    ASSERT(o->valid, "Attempt to query offset of deleted header");

    for(int i = OBJ_SECTION_START; i <= mem->obj_info_section; i += sizeof(obj_header))
    {
        copy = (obj_header*)(mem->vhd + i);
        if(copy->id.i == o->id.i) return i;
    }

    return -1;
}

static void destroy_header(memory* mem, const obj_header* obj) {
    int offset = query_header_offset(mem, obj);
    ASSERT(obj->valid, "Header was destroyed before the function could reach it!");
    ASSERT(offset > 0, "Something went really bad");

    int copysz = sizeof(obj_header);

    for(int i = offset, counter = 0; counter <= query_obj_count(mem); i += copysz, ++counter)
    {
        DBGF("Shifting struct at %d to %d\n", i + copysz, i);
        memcpy(mem->vhd + i, mem->vhd + i + copysz, copysz);
    }

    if(mem->vhd[mem->object_counter_index] == 0) mem->object_counter_index--;
    mem->vhd[mem->object_counter_index]--;
    mem->data_offset -= sizeof(obj_header);
}

void try_weld(memory* mem) {
    ASSERT(0, "Unimplemented!");
    /* ******************************************************************************************* *\
       *       Current problem:                                                                  *
       *           Stuck in a forever const-source loop                                          *
       *               If a member in source meets criteria, remove from source                  *
       *               But cant risk remove from source without screwing up the current loop     *
       *       Solution?                                                                         *
       *          A stack, another loop and checking entries for the same index;                 *
    \* ******************************************************************************************* */
    // // STRUCT_STACK(mem->free_space_count * mem->free_space_count, {int lower; int higher});
    // struct { int low; int high } welds[mem->free_space_count];
    // int wp = 0;

    // for(int i = 0; i < mem->free_space_count; ++i)
    // {
    //     int i_end = mem->fragments[i].size + mem->fragments[i].start;

    //     for(int j = 0; j < mem->free_space_count; ++j)
    //     {
    //         if(i == j) continue;

    //         int j_end = mem->fragments[j].size + mem->fragments[j].start;
    //         // [1 2 {3 4] 5 6 7 8}
    //         // 
    //         if(i_end >= mem->fragments[j].start && mem->fragments[i].start <= mem->fragments[j].start)
    //         {
    //             welds[wp].high =    (i > j)? j : i;
    //             welds[wp++].low =   (i > j)? i : j;
    //             break;
    //         }
    //     }
    // }

    // for(int i = 0; i < STACK_SIZE(); ++i) {

    // }
}

// !BEWARE! Destroying objects does not automatically defragment the VHD
//! This will eventually be a feature, for now just use it as a VWORM
//!                 (Virtual Write-Once-Read-Many)
void* destroy(memory* mem, int handle)
{
    obj_header obj;
    find_obj(mem, &obj, handle);
    
    void* ret = get(mem, handle);    
    if(ret == NULL) return NULL; // dont even bother resizing shit

    /*
        Might cause the vhd to fragment itself, but,
        The logic here would be that we push new open fragments through the top of the list
        And since find_lowest_offset_available starts from the top, it should find the most recent fragments
        Consequence of this is that the resizing will likely be with an item of a different size
        Which instead of taking up the space, will just break it into a smaller piece 
    */
    mem->fragments[++mem->free_space_count].size = obj.size.i;
    mem->fragments[mem->free_space_count].start = obj.offset.i;

    destroy_header(mem, &obj);

#ifdef WELD_ON_DESTROY
    try_weld(mem);
#endif
    
    return ret;
}

#endif // SERIZZ_H