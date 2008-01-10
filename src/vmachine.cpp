#include "common.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "vmachine.h"

#include "opengl_framebuffer.h"
#include "options.h"
#include "software_framebuffer.h"
#include "speedlimit.h"

VirtualMachine::VirtualMachine(const char *romfile, const char *biosfile)
    : extstorage_(vdc_),
      cpu_(rom_, extstorage_, keyboard_, joysticks_)
{
    rom_.load(romfile, biosfile);

    {
        const SDL_version *version = SDL_Linked_Version();
        cout << "Initializing SDL version " << (int)version->major
             << '.' << (int)version->minor << '.' << (int)version->patch << endl;

        if (SDL_Init(SDL_INIT_VIDEO) < 0)
            throw runtime_error(SDL_GetError());
    }

    SDL_ShowCursor(SDL_DISABLE);
    SDL_EnableKeyRepeat(0, 0);

    if (g_options.opengl) {
        framebuffer_ = new OpenGLFramebuffer;
        try {
            framebuffer_->init();
        }
        catch (exception &e) {
            LOGWARNING << "Unable to initialize the OpenGL framebuffer: " << e.what() << endl;
            LOGWARNING << "Falling back to software rendering mode" << endl;
            delete framebuffer_;
            framebuffer_ = new SoftwareFramebuffer;
        }
    }
    else {
        framebuffer_ = new SoftwareFramebuffer;
        framebuffer_->init();
    }
    SDL_WM_SetCaption(PACKAGE_NAME " " PACKAGE_VERSION, PACKAGE_NAME);
    if (g_options.debug)
        SDL_WM_IconifyWindow();

    vdc_.init(framebuffer_, &cpu_);
    reset();
}

VirtualMachine::~VirtualMachine()
{
    SDL_Quit();
    cout << "Virtual machine quit" << endl;
}

inline void VirtualMachine::reset()
{
    g_p1 = g_p2 = 0xff;
    rom_.calculate_current_bank();

    g_t1 = true;

    cpu_.reset();
    vdc_.reset();
}

void VirtualMachine::run()
{
    int cpu_ticks = 0, vdc_ticks = 0;
    static const int cpu_units = 228 * 10;
    const int vdc_units = g_options.pal_emulation ? 25.3 * 10 : 22.8 * 10;

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
                extstorage_.debug_dump_extram(cout);
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
                cpu_.debug_dump_intram(cout);
            }
            else if (command == "p" || command == "print") {
                cpu_.debug_print(cout);
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
                while (vdc_ticks < cpu_ticks) {
                    vdc_.step();
                    vdc_ticks += vdc_units;
                }
                cpu_ticks += cpu_.step() * cpu_units;

                if (vdc_.entered_vblank())
                    cpu_ticks = vdc_ticks = 0;

                cpu_.debug_print(cout);
            }
            else if (command == "t" || command == "timing") {
                vdc_.debug_print_timing(cout);
            }
            else if (command == "v" || command == "vdc") {
                vdc_.debug_dump(cout);
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
                                    || !joysticks_.handle_key_down(event.key.keysym))
                                keyboard_.handle_key_down(event.key.keysym);
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
                                    framebuffer_->take_snapshot();
                                    break;
                                default:
                                    if (event.key.keysym.mod & KMOD_CAPS
                                            || !joysticks_.handle_key_up(event.key.keysym))
                                        keyboard_.handle_key_up(event.key.keysym);
                                    break;
                            }
                            break;
                    }
                }

                for (int i = 0; i < UNPOLLED_FRAMES; ++i) {
                    while (!vdc_.entered_vblank() && !paused) {
                        if (vdc_ticks <= cpu_ticks) {
                            vdc_.step();
                            vdc_ticks += vdc_units;
                        }
                        else {
                            cpu_ticks += cpu_.step() * cpu_units;
                            if (cpu_.debug_get_pc() == breakpoint) {
                                cout << "Breakpoint reached" << endl;
                                cpu_.debug_print(cout);
                                g_options.debug = true;
                                break;
                            }
                        }
                    }
                    if (cpu_ticks > vdc_ticks) {
                        cpu_ticks -= vdc_ticks;
                        vdc_ticks = 0;
                    }
                    else {
                        vdc_ticks -= cpu_ticks;
                        cpu_ticks = 0;
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
