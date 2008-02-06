#include "common.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "vmachine.h"

#include "chars.h"
#include "cpu.h"
#include "joysticks.h"
#include "keyboard.h"
#include "opengl_framebuffer.h"
#include "options.h"
#include "rom.h"
#include "software_framebuffer.h"
#include "speedlimit.h"
#include "sprites.h"
#include "vdc.h"

void VirtualMachine::init(const char *romfile, const char *biosfile)
{
    g_rom.load(romfile, biosfile);

    {
        const SDL_version *version = SDL_Linked_Version();
        cout << "Initializing SDL version " << (int)version->major
             << '.' << (int)version->minor << '.' << (int)version->patch << endl;

        if (SDL_Init(SDL_INIT_VIDEO) < 0)
            throw runtime_error(SDL_GetError());
    }
    SDL_ShowCursor(SDL_DISABLE);
    SDL_EnableKeyRepeat(0, 0);

    g_chars.init();
    g_sprites.init();

    if (g_options.opengl) {
        g_framebuffer = new OpenGLFramebuffer;
        try {
            g_framebuffer->init();
        }
        catch (exception &e) {
            LOGWARNING << "Unable to initialize the OpenGL framebuffer: " << e.what() << endl;
            LOGWARNING << "Falling back to software rendering mode" << endl;
            delete g_framebuffer;
            g_framebuffer = new SoftwareFramebuffer;
        }
    }
    else {
        g_framebuffer = new SoftwareFramebuffer;
        g_framebuffer->init();
    }
    SDL_WM_SetCaption(PACKAGE_NAME " " PACKAGE_VERSION, PACKAGE_NAME);
    if (g_options.debug)
        SDL_WM_IconifyWindow();

    reset();
}

VirtualMachine::~VirtualMachine()
{
    delete g_framebuffer;

    SDL_Quit();
    cout << "Virtual machine quit" << endl;
}

inline void VirtualMachine::reset()
{
    g_p1 = g_p2 = 0xff;
    g_rom.calculate_current_bank();

    g_t1 = true;

    g_cpu.reset();
    g_vdc.reset();
}

void VirtualMachine::run()
{
    cout << "Emulation started" << endl;

    const int time_units = g_options.pal_emulation ? 10 : 9;

    int breakpoint = -1;

    while (true) {
        if (g_options.debug) {
            cout << "> ";
            cout.flush();
            string command;
            cin >> command;

            if (command == "b" || command == "breakpoint") {
                int addr;
                cin >> hex >> addr;
                if (cin.fail()) {
                    cout << "Invalid address" << endl;
                }
                else {
                    breakpoint = addr;
                    cout << "Breakpoint set at 0x" << hex << setw(2) << setfill('0') << addr << endl;
                }
            }
            else if (command == "c" || command == "continue") {
                g_options.debug = false;
            }
            else if (command == "e" || command == "extram") {
                g_extstorage.debug_dump_extram(cout);
            }
            else if (command == "?" || command == "h" || command == "help") {
                cout << "The following commands are recognized:\n" \
                        "c/continue Return to emulation\n" \
                        "i/intram   Dump the contents of the internal RAM\n" \
                        "p/print    Print the contents of some CPU structures\n" \
                        "q/quit     Quit " PACKAGE_NAME "\n" \
                        "r/reset    Reset the virtual machine\n" \
                        "s/step     Execute a single CPU step\n" \
                        "t/timing   Show timing information\n" \
                        "v/vdc      Dump the contents of the VDC memory\n";
                cout.flush();
            }
            else if (command == "i" || command == "intram") {
                g_cpu.debug_dump_intram(cout);
            }
            else if (command == "p" || command == "print") {
                g_cpu.debug_print(cout);
            }
            else if (command == "q" || command == "quit" || cin.eof()) {
                if (cin.eof())
                    cout << endl;
                return;
            }
            else if (command == "r" || command == "reset") {
                reset();
                cout << "Reset the virtual machine" << endl;
            }
            else if (command == "s" || command == "step") {
                const int ticks = g_cpu.step();
                for (int i = 0; i < time_units * ticks; ++i)
                    g_vdc.step();

                g_cpu.debug_print(cout);
            }
            else if (command == "t" || command == "timing") {
                g_vdc.debug_print_timing(cout);
            }
            else if (command == "v" || command == "vdc") {
                g_vdc.debug_dump(cout);
            }
            else {
                cout << "Unknown command, use \"help\" or \"h\" for help" << endl;
            }
        }
        else {
            bool paused = false;
            SpeedLimit limit;

            while (!g_options.debug) {
                // Check for SDL events
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    switch (event.type) {
                        case SDL_QUIT:
                            return;
                            break;
                        case SDL_KEYDOWN:
                            if (event.key.keysym.mod & KMOD_CAPS
                                    || !g_joysticks.handle_key_down(event.key.keysym))
                                g_keyboard.handle_key_down(event.key.keysym);
                            break;
                        case SDL_KEYUP:
                            switch (event.key.keysym.sym) {
                                case SDLK_ESCAPE:
                                    return;
                                    break;
                                case SDLK_F1:
                                    if (paused)
                                        cout << "Returning to emulation" << endl;
                                    else
                                        cout << "-- PAUSED (press F1 to return to emulation) --" << endl;
                                    paused = !paused;
                                    break;
                                case SDLK_F4:
                                    g_options.debug = true;
                                    break;
                                case SDLK_F5:
                                    reset();
                                    cout << "Reset the virtual machine" << endl;
                                    break;
                                case SDLK_PRINT:
                                    g_framebuffer->take_snapshot();
                                    break;
                                default:
                                    if (event.key.keysym.mod & KMOD_CAPS
                                            || !g_joysticks.handle_key_up(event.key.keysym))
                                        g_keyboard.handle_key_up(event.key.keysym);
                                    break;
                            }
                            break;
                    }
                }

                for (int i = 0; i < UNPOLLED_FRAMES; ++i) {
                    while (!g_vdc.entered_vblank() && !paused) {
                        const int ticks = g_cpu.step();
                        for (int i = 0; i < time_units * ticks; ++i)
                            g_vdc.step();

                        if (g_cpu.debug_get_pc() == breakpoint) {
                            cout << "Breakpoint reached" << endl;
                            g_cpu.debug_print(cout);
                            g_options.debug = true;
                            break;
                        }
                    }

                    if (g_options.debug)
                        break;

                    // Speed limiter
                    if (g_options.speed_limit)
                        limit.limit_on_frame_end();
                }
            }

            // Just entered debug mode
            SDL_WM_IconifyWindow();
        }
    }
}
