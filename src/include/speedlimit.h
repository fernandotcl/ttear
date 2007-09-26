#ifndef SPEED_LIMIT_H
#define SPEED_LIMIT_H

#include "globals.h"

#ifdef HAVE_GETTIMEOFDAY
extern "C" {
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# endif
# ifdef HAVE_TIME_H
#  include <time.h>
# endif
}
#endif
#include <iostream>

class SpeedLimit
{
    private:
        const uint32_t ticks_per_frame_;
        uint32_t last_ticks_;
        int32_t delta_;

    public:
        SpeedLimit();

        uint32_t get_ticks();
        void limit_on_frame_end();
};

inline SpeedLimit::SpeedLimit()
    : ticks_per_frame_(1000000 / ((g_options.pal_emulation ? 50 : 60) * g_options.speed_limit / 100.0)),
      last_ticks_(get_ticks()),
      delta_(0)
{
}

inline uint32_t SpeedLimit::get_ticks()
{
#ifdef HAVE_GETTIMEOFDAY
    timeval now;
    gettimeofday(&now, 0);
    return now.tv_sec * 1000000 + now.tv_usec;
#else
    return SDL_GetTicks() * 1000;
#endif
}

inline void SpeedLimit::limit_on_frame_end()
{
    delta_ += last_ticks_ + ticks_per_frame_ - get_ticks();
    if (delta_ < 0) {
        last_ticks_ = get_ticks();
        delta_ = 0;
    }
    else if (delta_ > 10000) {
        SDL_Delay(delta_ / 1000);
        last_ticks_ = get_ticks();
        delta_ = 0;
    }
}

#endif
