#ifndef COMMON_H
#define COMMON_H

#include <SDL.h>
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

#define LOGERROR (cerr << "E: ")
#define LOGWARNING (cerr << "W: ")

using namespace std;

extern uint8_t g_junk;
extern uint8_t g_p1, g_p2;
extern bool g_t1;

#endif
