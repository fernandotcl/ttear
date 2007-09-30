#include "globals.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "options.h"
#include "vmachine.h"

uint8_t g_junk;
uint8_t g_p1, g_p2;
bool g_t1;

using namespace std;

int main(int argc, char **argv)
{
    try {
        if (!g_options.parse(argc, argv))
            return EXIT_SUCCESS;
    }
    catch (exception &e) {
        cout << "Unable to parse options: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    try {
        VirtualMachine vm(g_options.rom.c_str(), g_options.bios.c_str());
        vm.run();
        cout << "Emulation terminated" << endl;
    }
    catch (exception &e) {
        LOGERROR << e.what() << endl;
        cerr << "Send bug reports to " << PACKAGE_BUGREPORT << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
