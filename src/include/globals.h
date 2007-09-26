#ifndef GLOBALS_H
#define GLOBALS_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

extern "C" {
#include <SDL.h>
#include <inttypes.h>
}
#include <cstdlib>
#include <iostream>

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

extern uint8_t g_junk;
extern uint8_t g_p1, g_p2;
extern bool g_t1;

#endif
