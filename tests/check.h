/*
    Minimal helpers for the "make check" test programs. No test framework is
    needed. A failed check prints its location and makes the program exit with
    a non-zero status.
*/

#pragma once

#include <cstdio>
#include <cstdlib>

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
            exit(1); \
        } \
    } while (0)
