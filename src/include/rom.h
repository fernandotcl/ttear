#ifndef ROM_H
#define ROM_H

#include "common.h"

#include <fstream>
#include <vector>

class Rom : public ifstream
{
    private:
        vector<vector<uint8_t> > banks_;
        uint8_t *current_bank_;

    public:
        static const int BANK_SIZE = 4096;

        Rom();

        void load(const char *romfile, const char *biosfile);
        void calculate_current_bank() { current_bank_ = &banks_[g_p1 & (1 << 0 | 1 << 1)][0]; }

        uint8_t &operator[](int index) { return current_bank_[index % BANK_SIZE]; }
        uint8_t operator[](int index) const { return current_bank_[index % BANK_SIZE]; }
};

extern Rom g_rom;

inline Rom::Rom()
    : banks_(4)
{
    for (int i = 0; i < 4; ++i)
        banks_[i].resize(BANK_SIZE);
}

#endif
