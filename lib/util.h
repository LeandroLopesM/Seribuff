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

#define INFER_PREFIX(x)                                 \
    int x##_push(const void* src, size_t srcsize) {     \
        return push(&x, src, srcsize);                  \
    }                                                   \
                                                        \
    void* x##_get(int h) {                              \
        return get(&x, h);                              \
    }                                                   \
                                                        \
    void x##_write(char* path) {                        \
        write(&x, path);                                \
    }                                                   \
    void x##_clear() {                                  \
        clear(&x);                                      \
    }                                                   \
    int x##_size() {                                    \
        return size(&x);                                \
    }                                                   \
    int x##_clear() {					\
        return clear(&x);				\
    }							\

#define INFER_ARG(x, y)                                 \
    int x##_push(const void* src, size_t srcsize) {     \
        return push(&y, src, srcsize);                  \
    }                                                   \
                                                        \
    void* x##_get(int h) {                              \
        return get(&y, h);                              \
    }                                                   \
    void x##_write(char* path) {                        \
        return write(&y, path);                         \
    }                                                   \
    void x##_clear() {                                  \
        clear(&y);                                      \
    }                                                   \
    int x##_size() {                                    \
        return size(&y);                                \
    }                                                   \

#define MEMORY(x)       \
    memory x;           \
    INFER_PREFIX(x)     \

// #define DEBUG
#ifdef DEBUG
#   define DBG(fmt)                                         \
        do{                                                 \
            char s[strlen(fmt) + strlen(__func__) + 4];\
            sprintf(s, "%s: %s\n", __func__, fmt);            \
            printf(s);                                      \
        } while(0)
#   define DBGF(fmt, ...)                                   \
        do{                                                 \
            char s[strlen(fmt) + strlen(__func__) + 4];\
            sprintf(s, "%s: %s\n", __func__, fmt);            \
            printf(s, __VA_ARGS__);                         \
        } while(0)
#else
#   define DBG(fmt, ...)
#   define DBGF(fmt, ...)
#endif

#define ASSERT(x, msg) \
    do { if(!(x)) { fprintf(stderr, "%s:%d assertion failed: '%s'\nPanic message:\t%s\n", __FILE__, __LINE__, #x, msg); exit(1); } } while(0)

#if ASSERT_FATAL && ASSERT_BREAK
#   define SOFT_ASSERT(expr, msg, nonfatal) do {raise(SIGTRAP); ASSERT(expr, msg)} while(0)
#elif ASSERT_FATAL
#   define SOFT_ASSERT(expr, msg, nonfatal) ASSERT(expr, msg)
#else
#   define SOFT_ASSERT(expr, msg, nonfatal) do {if(!(expr)) nonfatal;} while(0)
#endif

typedef unsigned char byte;

// shamelessly ripped from a stackexchange thread i lost the link of
// *slightly* tweaked
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

#define STRUCT_STACK(size, struct_body)\
struct stack struct_body;\
struct stack s[size];\
int sp = 0;

#define STACK_PUSH(...) \
    s[sp++] = (struct stack){__VA_ARGS__}

#define STACK_VAR() \
    s
    

#endif // UTIL_H
