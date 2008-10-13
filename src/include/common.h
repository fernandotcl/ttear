#ifndef COMMON_H
#define COMMON_H

#include <inttypes.h>
#include <iostream>

#include "config.h"

#ifdef _DEBUG
# ifndef DEBUG
#  define DEBUG
# endif
#endif
#ifdef DEBUG
# ifdef NDEBUG
#  undef NDEBUG
# endif
#else
# ifndef NDEBUG
#  define NDEBUG
# endif
#endif
#include <cassert>

// SDL recommends this non-conventional way to include its main include file
#include "SDL.h"

#define LOGERROR (cerr << "E: ")
#define LOGWARNING (cerr << "W: ")

using namespace std;

// g_junk is a variable filled with junk, used when we want some degree of
// randomness in order to increase realism (for example, when reading from a
// register in a state which could lead to undetermined behavior)
extern uint8_t g_junk;

// The two I/O ports, P1 and P2
extern uint8_t g_p1, g_p2;

// T1 test input (set when in VBLANK, clear otherwise)
extern bool g_t1;

#endif
