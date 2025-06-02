#ifndef UTIL_H
#define UTIL_H

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>

#ifndef _STDBOOL_H // Because stdbool just doesnt work for me for some reason ?!?!?!
    #define bool int
    #define true 1
    #define false 0
#endif

#define INFER_PREFIX(x)                         \
    int x##_push(const void* src, size_t srcsize) {   \
        return push(&x, src, srcsize);          \
    }                                           \
                                                \
    void* x##_get(int h) {                      \
        return get(&x, h);                      \
    }                                           \

#define INFER_ARG(x, y)                         \
    int x##_push(const void* src, size_t srcsize) {   \
        return push(&y, src, srcsize);          \
    }                                           \
                                                \
    void* x##_get(int h) {                      \
        return get(&y, h);                      \
    }                                           \

#ifdef DEBUG
#   define DBG(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#   define DBG(fmt, ...)
#endif

#define assert(x, msg) \
    do { if(!(x)) { fprintf(stderr, "%s:%d assertion failed: '%s'\nPanic message: %s", __FILE__, __LINE__, #x, msg); exit(1); } } while(0)

typedef unsigned char byte;

void hexdump(const void *data, size_t size) {
    const byte *b = (byte *)data;
    size_t i, j;

    size_t colwidth = 4;

    for (i = 0; i < size; i += colwidth) {

        for (j = 0; j < colwidth; j++) {
            if (i + j < size) {
                printf("%2X ", b[i + j]);
            } else {
                printf("   ");
            }

            if (j == 7) {
                printf(" ");
            }
        }

        printf(" |");

        // Print ASCII characters
        for (j = 0; j < colwidth; j++) {
            if (i + j < size) {
                unsigned char c = b[i + j];
                printf("%2c", isprint(c) ? c : '.');
            } else {
                printf(" ");
            }
        }

        printf("\n");
    }
}

byte* to_byte(int f)
{
    assert(f >= 0, "cannot coerce negative int");

    static byte fb[4] = {0};
    memset(fb, 0, sizeof(fb));

    if(f >= (1 << (8 * 3)) - 1) {
        fb[0] = f & ((1 << 8) - 1);
        f = f >> 8;
        fb[1] = f & ((1 << 8) - 1);
        f = f >> 8;
        fb[2] = f & ((1 << 8) - 1);
        fb[3] = f >> 8;

        DBG("%d/%d/%d/%d larger than 2 bytes\n", fb[0], fb[1], fb[2], fb[3]);
    }
    else if(f >= (1 << (8 * 2)) - 1) {
        fb[0] = f & ((1 << 8) - 1);
        f = f >> 8;
        fb[1] = f & ((1 << 8) - 1);
        fb[2] = f >> 8;

        DBG("%d/%d/%d larger than 2 bytes\n", fb[0], fb[1], fb[2]);
    }
    else if(f > (1 << 8) - 1) {
        fb[0] = f & ((1 << 8) - 1);
        fb[1] = f >> 8;
        DBG("%d/%d larger than 1 byte\n", (byte)fb[0], fb[1], fb[2]);
    }
    else {
        fb[0] = f;
        DBG("%d within constraints", f);
    }

    return fb;
}

int from_byte(const byte* f)
{
    // static int fb[4] = {0};
    // memset(fb, 0, sizeof(fb));

    int e = 0;

    if(f[3]) {
        e = f[3];
        e = e << 8;
        e |= f[2];
        e = e << 8;
        e |= f[1];
        e = e << 8;
        e |= f[0];

        DBG("%d larger than 2 bytes\n", e);
    }
    else if(f[2]) {
        e = f[2];
        e = e << 8;
        e |= f[1];
        e = e << 8;
        e |= f[0];

        DBG("%d larger than 2 bytes\n", e);
    }
    else if(f[1]) {
        e = f[1];
        e = e << 8;
        e |= f[0];
        DBG("%d larger than 1 byte\n", e);
    }
    else e = f[0];

    return e;
}

#endif // UTIL_H