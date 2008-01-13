#ifndef VMACHINE_H
#define VMACHINE_H

#include "common.h"

class VirtualMachine
{
    private:
        static const int UNPOLLED_FRAMES = 3;

        void reset();

    public:
        ~VirtualMachine();

        void init(const char *romfile, const char *biosfile);
        void run();
};

#endif
