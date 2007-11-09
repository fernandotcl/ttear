#ifndef VMACHINE_H
#define VMACHINE_H

#include "common.h"

#include "cpu.h"
#include "extstorage.h"
#include "framebuffer.h"
#include "joysticks.h"
#include "keyboard.h"
#include "rom.h"
#include "vdc.h"

class VirtualMachine
{
    private:
        static const int UNPOLLED_FRAMES = 3;

        Rom rom_;
        Framebuffer *framebuffer_;
        Vdc vdc_;
        ExternalStorage extstorage_;
        Keyboard keyboard_;
        Joysticks joysticks_;
        Cpu cpu_;

        void reset();

    public:
        VirtualMachine(const char *romfile, const char *biosfile);
        ~VirtualMachine();

        void run();
};

#endif
