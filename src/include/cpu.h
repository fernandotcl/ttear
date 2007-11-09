#ifndef CPU_H
#define CPU_H

#include "common.h"

#include <iostream>

#include "extstorage.h"
#include "joysticks.h"
#include "keyboard.h"
#include "rom.h"
#include "util.h"

class Cpu
{
    private:
        // Some constants
        static const int INTRAM_SIZE = 64;
        static const int STACK_START = 0x08;
        static const int STACK_SIZE  = 0x10;

        static const int CPU_EXTIRQ_INTERRUPT_VECTOR = 0x003;
        static const int CPU_TCNTIRQ_INTERRUPT_VECTOR = 0x007;

        struct psw_t {
            // It's easier to do arithmetics with integers instead of bits
            uint8_t cy; // carry
            uint8_t ac; // aux carry
            uint8_t f0; // programmable flag 0
            uint8_t bs; // working register bank set
            uint8_t sp; // stack pointer

            psw_t() : bs(0), sp(0) {}

            // The carry is used intensively for ADDC, hence why we store it as bit 0 instead of bit 7
            void set_cy() { cy = 1; };
            void set_ac() { ac = 1 << 6; };
            void set_f0() { f0 = 1 << 5; };
            void set_bs() { bs = 1 << 4; };

            void cpl_cy() { cy ^= 1; }
            void cpl_f0() { f0 ^= 1 << 5; };

            uint8_t operator()() { return cy << 7 | ac | f0 | bs | sp; }
        } psw_;

        void load_psw_no_sp(uint8_t val)
        {
            psw_.cy = (val & 1 << 6) >> 7;
            psw_.ac = val & 1 << 6;
            psw_.f0 = val & 1 << 5;
            psw_.bs = val & 1 << 4;
            regptr_ = psw_.bs ? &intram_[STACK_START + STACK_SIZE] : &intram_[0];
        }

        void load_psw(uint8_t val)
        {
            load_psw_no_sp(val);
            psw_.sp = val & (1 << 2 | 1 << 1 | 1 << 0);
        }

        // The program counter
        int pc_, last_pc_;
        bool a11_on_;
        Rom &rom_;

        // acc_ is int instead of uint8_t to speed up some arithmetics
        int acc_;

        // Programmable flag 1
        bool f1_;

        // The counter
        enum {
            TCNT_STATUS_ALL_OFF,
            TCNT_STATUS_COUNTER_ON,
            TCNT_STATUS_TIMER_ON,
        } tcnt_status_;
        bool tcnt_overflow_;
        uint8_t tcnt_;
        void tcnt_increment();
        int timer_timer_;
        static const int TIMER_TIMER_INITIAL_VALUE = 31;

        // Internal RAM
        vector<uint8_t> intram_;

        // External storage
        ExternalStorage &extstorage_;

        // The registers
        uint8_t *regptr_;
        void sel_rb0() { psw_.bs = 0; regptr_ = &intram_[0]; }
        void sel_rb1() { psw_.set_bs(); regptr_ = &intram_[STACK_START + STACK_SIZE]; };
        uint8_t &r(int index) { return *(regptr_ + index); }

        // Interrupts
        bool extirq_en_, tcntirq_en_;
        bool extirq_pending_, tcntirq_pending_;
        bool in_irq_;

        Keyboard &keyboard_;
        Joysticks &joysticks_;

        // Stack operations
        void push(uint8_t val);
        uint8_t pop();

        // Abstracted operations
        void add(uint8_t val);
        void addc(uint8_t val);
        void jmp(int page);
        void jmp_if(bool val);
        void jb(int index);
        void call(int page);
        void irq(int addr);

    public:
        static const int EXTRAM_SIZE = 128;

        Cpu(Rom &rom, ExternalStorage &extstorage, Keyboard &keyboard, Joysticks &joysticks);

        // Debug stuff
        int debug_get_pc() { return last_pc_; }
        void debug_print(ostream &out);
        void debug_dump_intram(ostream &out) { dump_memory(out, intram_, INTRAM_SIZE); }

        void reset();
        int step();

        void external_irq();
        void clear_external_irq() { extirq_pending_ = false; }
        void counter_increment() { if (tcnt_status_ == TCNT_STATUS_COUNTER_ON) tcnt_increment(); }
};

inline Cpu::Cpu(Rom &rom, ExternalStorage &extstorage, Keyboard &keyboard, Joysticks &joysticks)
    : rom_(rom),
      intram_(INTRAM_SIZE),
      extstorage_(extstorage),
      keyboard_(keyboard),
      joysticks_(joysticks)
{
    reset();
}

inline void Cpu::reset()
{
    pc_ = last_pc_ = 0;
    a11_on_ = false;

    acc_ = 0;

    extirq_en_ = tcntirq_en_ = false;
    extirq_pending_ = tcntirq_pending_ = false;
    in_irq_ = false;

    tcnt_status_ = TCNT_STATUS_ALL_OFF;
    tcnt_overflow_ = false;
    timer_timer_ = TIMER_TIMER_INITIAL_VALUE;

    regptr_ = &intram_[0];
}

inline void Cpu::external_irq()
{
    if (extirq_en_)
        extirq_pending_ = true;
}

#endif
