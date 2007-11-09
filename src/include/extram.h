#ifndef EXTRAM_H
#define EXTRAM_H

#include "common.h"

#include <iostream>
#include <vector>

#include "util.h"

class ExternalRam
{
    private:
        static const int EXTRAM_SIZE = 256;
        vector<uint8_t> buffer_;

    public:
        ExternalRam() : buffer_(EXTRAM_SIZE) {}

        void debug_dump(ostream &out) const { dump_memory(out, buffer_, EXTRAM_SIZE); }

        uint8_t read(int offset) const { return buffer_[offset]; }
        void write(int offset, uint8_t value) { buffer_[offset] = value; }
};

#endif
