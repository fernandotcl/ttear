#ifndef EXTSTORAGE_H
#define EXTSTORAGE_H

#include <vector>

#include "common.h"

#include "vdc.h"
#include "util.h"

class ExternalStorage
{
    private:
        static const int EXTRAM_SIZE = 256;
        vector<uint8_t> extram_;

        bool p1_bit_high(int index) const;
        bool p1_bit_low(int index) const;

    public:
        ExternalStorage();

        void debug_dump_extram(ostream &out) const { dump_memory(out, extram_, EXTRAM_SIZE); }

        template<typename T> void read(uint8_t offset, T &reg) const;
        void write(uint8_t offset, uint8_t value);
};

extern ExternalStorage g_extstorage;

inline ExternalStorage::ExternalStorage()
    : extram_(EXTRAM_SIZE)
{
}

inline bool ExternalStorage::p1_bit_high(int index) const
{
    return g_p1 & 1 << index;
}

inline bool ExternalStorage::p1_bit_low(int index) const
{
    return !(g_p1 & 1 << index);
}

template<typename T> inline void ExternalStorage::read(uint8_t offset, T &reg) const
{
    if (p1_bit_low(3) && p1_bit_high(4) && p1_bit_low(6))
        reg = g_vdc.read(offset);
    else if ((p1_bit_low(3) && p1_bit_low(4) && p1_bit_high(6)) || (p1_bit_high(3) && p1_bit_low(4)))
        reg = extram_[offset];
    else if (!g_p1)
        reg = g_junk;
}

inline void ExternalStorage::write(uint8_t offset, uint8_t value)
{
    if (p1_bit_low(3))
        g_vdc.write(offset, value);
    if (p1_bit_low(4) && p1_bit_low(6))
        extram_[offset] = value;
}

#endif
