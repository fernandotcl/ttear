#include "common.h"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <sstream>

#include "rom.h"

void Rom::load(const char *romfile, const char *biosfile)
{
    exceptions(badbit);

    cout << "Attempting to load ROM file " << romfile << endl;
    open(romfile, in | ate | binary);
    if (!is_open())
        throw runtime_error("Unable to open file");

    {
        size_t len = tellg();
        seekg(0);
        switch (len) {
            case 2048:
                cout << "Found 2k ROM, loading..." << endl;
                read((char *)&banks_[0][1024], 2048);
                memcpy(&banks_[1][1024], &banks_[0][1024], 2048);
                memcpy(&banks_[2][1024], &banks_[0][1024], 2048);
                memcpy(&banks_[3][1024], &banks_[0][1024], 2048);
                break;
            case 3072:
                cout << "Found 3k ROM, loading..." << endl;
                read((char *)&banks_[0][1024], 3072);
                memcpy(&banks_[1][1024], &banks_[0][1024], 3072);
                memcpy(&banks_[2][1024], &banks_[0][1024], 3072);
                memcpy(&banks_[3][1024], &banks_[0][1024], 3072);
                break;
            case 4096:
                cout << "Found 4k ROM, loading..." << endl;
                read((char *)&banks_[0][1024], 2048);
                read((char *)&banks_[1][1024], 2048);
                memcpy(&banks_[2][1024], &banks_[0][1024], 2048);
                memcpy(&banks_[3][1024], &banks_[1][1024], 2048);
                break;
            case 6144:
                cout << "Found 6k ROM, loading..." << endl;
                read((char *)&banks_[0][1024], 3072);
                read((char *)&banks_[1][1024], 3072);
                memcpy(&banks_[2][1024], &banks_[0][1024], 3072);
                memcpy(&banks_[3][1024], &banks_[1][1024], 3072);
                break;
            case 8192:
                cout << "Found 8k ROM, loading..." << endl;
                read((char *)&banks_[0][1024], 2048);
                read((char *)&banks_[1][1024], 2048);
                read((char *)&banks_[2][1024], 2048);
                read((char *)&banks_[3][1024], 2048);
                break;
            case 12288:
                cout << "Found 12k ROM, loading..." << endl;
                read((char *)&banks_[0][1024], 3072);
                read((char *)&banks_[1][1024], 3072);
                read((char *)&banks_[2][1024], 3072);
                read((char *)&banks_[3][1024], 3072);
                break;
            default:
                throw runtime_error("Unrecognized ROM type");
                break;
        }
    }

    cout << "ROM loaded successfully" << endl;
    close();

    cout << "Attempting to load BIOS file " << biosfile << endl;
    open(biosfile, in | ate | binary);
    if (!is_open())
        throw runtime_error("Unable to open file");

    {
        size_t len;
        if ((len = tellg()) != 1024) {
            ostringstream oss;
            oss << "Incorrect BIOS image size (" << (len / 1024) << "kb, expected 1kb)";
            throw runtime_error(oss.str());
        }
    }

    seekg(0);
    read((char *)&banks_[0][0], 1024);
    memcpy(&banks_[1][0], &banks_[0][0], 1024);
    memcpy(&banks_[2][0], &banks_[0][0], 1024);
    memcpy(&banks_[3][0], &banks_[0][0], 1024);

    cout << "BIOS loaded successfully" << endl;
    close();
}
