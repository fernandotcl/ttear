#ifndef EXTSTORAGE_H
#define EXTSTORAGE_H

#include "common.h"

class ExternalStorage;

#include "extram.h"
#include "vdc.h"

class ExternalStorage
{
    private:
        ExternalRam extram_;
        Vdc &vdc_;

        bool p1_bit_high(int index) const;
        bool p1_bit_low(int index) const;

    public:
        ExternalStorage(Vdc &vdc) : vdc_(vdc) {}

        void debug_dump_extram(ostream &out) const { extram_.debug_dump(out); }

        template<typename T> void read(uint8_t offset, T &reg) const;
        void write(uint8_t offset, uint8_t value);
};

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
        reg = vdc_.read(offset);
    else if ((p1_bit_low(3) && p1_bit_low(4) && p1_bit_high(6)) || (p1_bit_high(3) && p1_bit_low(4)))
        reg = extram_.read(offset);
    else if (!g_p1)
        reg = g_junk;
}

inline void ExternalStorage::write(uint8_t offset, uint8_t value)
{
    if (p1_bit_low(3))
        vdc_.write(offset, value);
    if (p1_bit_low(4) && p1_bit_low(6))
        extram_.write(offset, value);
}

#endif
