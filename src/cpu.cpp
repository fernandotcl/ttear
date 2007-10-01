#include "globals.h"

#include <iomanip>
#include <iostream>

#include "cpu.h"

#include "opcodes.h"
#include "options.h"
#include "rom.h"

using namespace std;

void Cpu::tcnt_increment()
{
    if (++tcnt_ == 0) {
        tcnt_overflow_ = true;
        if (tcntirq_en_)
            tcntirq_pending_ = true;
    }
}

void Cpu::debug_print(ostream &out)
{
    out << setfill('0') << hex;

    out << "0x" << setw(4) << last_pc_ << ": " << opcode_names[rom_[last_pc_]]
        << " [0x" << setw(2) << (int)rom_[last_pc_ + 1] << "] (0x" << setw(2) << (int)rom_[last_pc_] << ")\n";

    out << "A: 0x" << setw(2) << acc_ << " PSW: 0x" << setw(2) << (int)psw_()
        << " (CY: " << (psw_.cy ? '1' : '0')
        << ", AC: " << (psw_.ac ? '1' : '0')
        << ", F0: " << (psw_.f0 ? '1' : '0')
        << ", BS: " << (psw_.bs ? '1' : '0')
        << ", SP: " << "0x" << setw(2) << (int)psw_.sp << ")\n";

    keyboard_.calculate_p2();
    out << "P1: 0x" << setw(2) << (int)g_p1 << " P2: 0x" << setw(2) << (int)g_p2 << "\n";

    if (psw_.bs)
        out << "RB0       RB1 (*)\n";
    else
        out << "RB0 (*)   RB1\n";
    for (int i = 0; i < 8; ++i)
        out << 'R' << i << ": 0x" << setw(2) << (int)intram_[i]
            << "  R" << i << ": 0x" << setw(2) << (int)intram_[STACK_START + STACK_SIZE + i] << "\n";

    out.flush();
}

inline void Cpu::push(uint8_t val)
{
    intram_[STACK_START + psw_.sp] = val;
    ++psw_.sp %= STACK_SIZE;
}

inline uint8_t Cpu::pop()
{
    return intram_[STACK_START + --psw_.sp];
}

inline void Cpu::add(uint8_t val)
{
    psw_.cy = psw_.ac = 0;

    if ((acc_ & 0x0f) + (val & 0x0f) > 0x0f)
        psw_.set_ac();

    if ((acc_ += val) > 0xff) {
        psw_.set_cy();
        acc_ &= 0xff;
    }
}

inline void Cpu::addc(uint8_t val)
{
    psw_.ac = 0;

    if ((acc_ & 0x0f) + (val & 0x0f) + psw_.cy > 0x0f)
        psw_.set_ac();

    uint8_t old_cy = psw_.cy;
    psw_.cy = 0;
    if ((acc_ += val + old_cy) > 0xff) {
        psw_.set_cy();
        acc_ &= 0xff;
    }
}

inline void Cpu::jmp_if(bool val)
{
    if (val)
        pc_ = pc_ & 0xf00 | rom_[pc_];
    else
        ++pc_;
}

inline void Cpu::jb(int index)
{
    jmp_if(acc_ & 1 << index);
}

inline void Cpu::jmp(int page)
{
    pc_ = rom_[pc_] | page << 8;
    if (a11_on_ && !in_irq_)
        pc_ |= 1 << 11;
}

inline void Cpu::call(int page)
{
    push(pc_ + 1 & 0xff);
    push((pc_ + 1 & 0xf00) >> 8 | psw_() & 0xf0);
    jmp(page);
}

inline void Cpu::irq(int addr)
{
    in_irq_ = true;
    push(pc_ & 0xff);
    push((pc_ & 0xf00) >> 8 | psw_() & 0xf0);
    last_pc_ = pc_;
    pc_ = addr;
}

int Cpu::step()
{
    if (!in_irq_) {
        if (extirq_pending_) {
            irq(CPU_EXTIRQ_INTERRUPT_VECTOR);
            return 2;
        }
        if (tcntirq_pending_) {
            tcntirq_pending_ = false;
            irq(CPU_TCNTIRQ_INTERRUPT_VECTOR);
            return 2;
        }
    }

    last_pc_ = pc_;
    uint8_t opcode = rom_[pc_++];
    pc_ = pc_ & Rom::BANK_SIZE - 1;
    int clock;

#ifdef DEBUG
    // Make sure we always set clock to something meanful
    clock = 42;
#endif

    uint8_t tmp;
    switch (opcode)
    {
        case 0x00: // NOP
            clock = 1;
            break;
        case 0x03: // ADD A, #data
            add(rom_[pc_++]);
            clock = 1;
            break;
        case 0x04: // JMP (page 0)
            jmp(0);
            clock = 2;
            break;
        case 0x05: // EN I
            extirq_en_ = true;
            clock = 1;
            break;
        case 0x07: // DEC A
            acc_ = acc_ - 1 & 0xff;
            clock = 1;
            break;
        case 0x08: // INS A, BUS
            acc_ = joysticks_.get_bus();
            clock = 2;
            break;
        case 0x09: // IN A, P1
            acc_ = g_p1;
            clock = 2;
            break;
        case 0x0a: // IN A, P2
            keyboard_.calculate_p2();
            acc_ = g_p2;
            clock = 2;
            break;
#define MOVD_A_P(n, pn) \
        case 0x0c + n: \
            acc_ &= 0xf0; \
            clock = 2; \
            break;
        // MOVD A, Pn
        MOVD_A_P(0, p4_)
        MOVD_A_P(1, p5_)
        MOVD_A_P(2, p6_)
        MOVD_A_P(3, p7_)
#define INC_RPTR(n) \
        case 0x10 + n: \
            intram_[r(n) & INTRAM_SIZE - 1]++; \
            clock = 1; \
            break;
        // INC @Rn
        INC_RPTR(0)
        INC_RPTR(1)
        case 0x12: // JB0
            jb(0);
            clock = 2;
            break;
        case 0x13: // ADDC A, #data
            addc(rom_[pc_++]);
            clock = 2;
            break;
        case 0x14: // CALL (page 0)
            call(0);
            clock = 2;
            break;
        case 0x15: // DIS I
            extirq_en_ = false;
            clock = 1;
            break;
        case 0x16: // JTF
            jmp_if(tcnt_overflow_);
            tcnt_overflow_ = false;
            clock = 2;
            break;
        case 0x17: // INC A
            acc_ = acc_ + 1 & 0xff;
            clock = 1;
            break;
#define INC_R(n) \
        case 0x18 + n: \
            ++r(n); \
            clock = 1; \
            break;
        // INC Rn
        INC_R(0)
        INC_R(1)
        INC_R(2)
        INC_R(3)
        INC_R(4)
        INC_R(5)
        INC_R(6)
        INC_R(7)
#define XCH_A_RPTR(n) \
        case 0x20 + n: \
            tmp = (uint8_t)acc_; \
            acc_ = intram_[r(n) & INTRAM_SIZE - 1]; \
            intram_[r(n) & INTRAM_SIZE - 1] = tmp; \
            clock = 1; \
            break;
        // XCH A, @Rn
        XCH_A_RPTR(0)
        XCH_A_RPTR(1)
        case 0x23: // MOV A, #data
            acc_ = rom_[pc_++];
            clock = 2;
            break;
        case 0x24: // JMP (page 1)
            jmp(1);
            clock = 2;
            break;
        case 0x25: // EN TCNTI
            tcntirq_en_ = true;
            clock = 1;
            break;
        case 0x26: // JNT0
            jmp_if(true);
            clock = 2;
            break;
        case 0x27: // CLR A
            acc_ = 0;
            clock = 1;
            break;
#define XCH_A_R(n) \
        case 0x28 + n: \
            tmp = (uint8_t)acc_; \
            acc_ = r(n); \
            r(n) = tmp; \
            clock = 1; \
            break;
        // XCH A, Rn
        XCH_A_R(0)
        XCH_A_R(1)
        XCH_A_R(2)
        XCH_A_R(3)
        XCH_A_R(4)
        XCH_A_R(5)
        XCH_A_R(6)
        XCH_A_R(7)
#define XCHD_A_RPTR(n) \
        case 0x30 + n: \
            tmp = (uint8_t)acc_ & 0x0f; \
            acc_ = acc_ & 0xf0 | intram_[r(n) & INTRAM_SIZE - 1] & 0x0f; \
            intram_[r(n) & 0x3f] = intram_[r(n) & INTRAM_SIZE - 1] & 0xf0 | tmp; \
            clock = 1; \
            break;
        // XCHD A, @Rn
        XCHD_A_RPTR(0)
        XCHD_A_RPTR(1)
        case 0x32: // JB1
            jb(1);
            clock = 2;
            break;
        case 0x34: // CALL (page 1)
            call(1);
            clock = 2;
            break;
        case 0x35: // DIS TCNTI
            tcntirq_en_ = false;
            clock = 1;
            break;
        case 0x36: // JT0
            jmp_if(false);
            clock = 2;
            break;
        case 0x37: // CPL A
            acc_ ^= 0xff;
            clock = 1;
            break;
        case 0x39: // OUTL P1, A
            g_p1 = acc_;
            rom_.calculate_current_bank();
            clock = 2;
            break;
        case 0x3a: // OUTL P2, A
            g_p2 = acc_;
            clock = 2;
            break;
#define MOVD_P_A(n, pn) \
        case 0x3c + n: \
            clock = 2; \
            break;
        // MOVD Pn, A
        MOVD_P_A(0, p4_)
        MOVD_P_A(1, p5_)
        MOVD_P_A(2, p6_)
        MOVD_P_A(3, p7_)
#define ORL_A_RPTR(n) \
        case 0x40 + n: \
            acc_ |= intram_[r(n) & INTRAM_SIZE - 1]; \
            clock = 1; \
            break;
        // ORL A, @Rn
        ORL_A_RPTR(0)
        ORL_A_RPTR(1)
        case 0x42: // MOV A, T
            acc_ = tcnt_;
            clock = 1;
            break;
        case 0x43: // ORL A, #data
            acc_ |= rom_[pc_++];
            clock = 2;
            break;
        case 0x44: // JMP (page 2)
            jmp(2);
            clock = 2;
            break;
        case 0x45: // STRT CNT
            tcnt_status_ = TCNT_STATUS_COUNTER_ON;
            clock = 1;
            break;
        case 0x46: // JNT1
            jmp_if(!g_t1);
            clock = 2;
            break;
        case 0x47: // SWAP A
            acc_ = (acc_ & 0x0f) << 4 | (acc_ & 0xf0) >> 4;
            clock = 1;
            break;
#define ORL_A_R(n) \
        case 0x48 + n: \
            acc_ |= r(n); \
            clock = 1; \
            break;
        // ORL A, Rn
        ORL_A_R(0)
        ORL_A_R(1)
        ORL_A_R(2)
        ORL_A_R(3)
        ORL_A_R(4)
        ORL_A_R(5)
        ORL_A_R(6)
        ORL_A_R(7)
#define ANL_A_RPTR(n) \
        case 0x50 + n: \
            acc_ &= intram_[r(n) & INTRAM_SIZE - 1]; \
            clock = 1; \
            break;
        // ANL A, @Rn
        ANL_A_RPTR(0)
        ANL_A_RPTR(1)
        case 0x52: // JB2
            jb(2);
            clock = 2;
            break;
        case 0x53: // ANL A, #data
            acc_ &= rom_[pc_++];
            clock = 2;
            break;
        case 0x54: // CALL (page 2)
            call(2);
            clock = 2;
            break;
        case 0x55: // STRT T
            tcnt_status_ = TCNT_STATUS_TIMER_ON;
            clock = 1;
            break;
        case 0x56: // JT1
            jmp_if(g_t1);
            clock = 2;
            break;
        case 0x57: // DA A
            if ((acc_ & 0x0f) > 9 || psw_.ac) {
                acc_ += 6;
                if (acc_ > 0xff) {
                    acc_ &= 0xff;
                    psw_.set_cy();
                }
            }
            tmp = (acc_ & 0xf0) >> 4;
            if (tmp > 9 || psw_.cy) {
                tmp += 6;
                psw_.set_cy();
            }
            acc_  = (acc_ & 0x0f | tmp << 4) & 0xff;
            clock = 1;
            break;
#define ANL_A_R(n) \
        case 0x58 + n: \
            acc_ &= r(n); \
            clock = 1; \
            break;
        // ANL A, Rn
        ANL_A_R(0)
        ANL_A_R(1)
        ANL_A_R(2)
        ANL_A_R(3)
        ANL_A_R(4)
        ANL_A_R(5)
        ANL_A_R(6)
        ANL_A_R(7)
#define ADD_A_RPTR(n) \
        case 0x60 + n: \
            add(intram_[r(n) & INTRAM_SIZE - 1]); \
            clock = 1; \
            break;
        // ADD A, @Rn
        ADD_A_RPTR(0)
        ADD_A_RPTR(1)
        case 0x62: // MOV T, A
            tcnt_ = acc_;
            clock = 1;
            break;
        case 0x64: // JMP (page 3)
            jmp(3);
            clock = 2;
            break;
        case 0x65: // STOP TCNT
            tcnt_status_ = TCNT_STATUS_ALL_OFF;
            clock = 1;
            break;
        case 0x67: // RRC A
            acc_ |= psw_.cy << 8;
            psw_.cy = acc_ & 1 << 0;
            acc_ >>= 1;
            clock = 1;
            break;
#define ADD_A_R(n) \
        case 0x68 + n: \
            add(r(n)); \
            clock = 1; \
            break;
        // ADD A, Rn
        ADD_A_R(0)
        ADD_A_R(1)
        ADD_A_R(2)
        ADD_A_R(3)
        ADD_A_R(4)
        ADD_A_R(5)
        ADD_A_R(6)
        ADD_A_R(7)
#define ADDC_A_RPTR(n) \
        case 0x70 + n: \
            addc(intram_[r(n) & INTRAM_SIZE - 1]); \
            clock = 1; \
            break;
        // ADDC A, @Rn
        ADDC_A_RPTR(0)
        ADDC_A_RPTR(1)
        case 0x72: // JB3
            jb(3);
            clock = 2;
            break;
        case 0x74: // CALL (page 3)
            call(3);
            clock = 2;
            break;
        case 0x76: // JF1
            jmp_if(f1_);
            clock = 2;
            break;
        case 0x77: // RR A
            acc_ |= (acc_ & 0x01) << 8;
            acc_ >>= 1;
            clock = 1;
            break;
#define ADDC_A_R(n) \
        case 0x78 + n: \
            addc(r(n));
            clock = 1;
            break;
        // ADDC A, Rn
        ADDC_A_R(0)
        ADDC_A_R(1)
        ADDC_A_R(2)
        ADDC_A_R(3)
        ADDC_A_R(4)
        ADDC_A_R(5)
        ADDC_A_R(6)
        ADDC_A_R(7)
#define MOVX_A_RPTR(n) \
        case 0x80 + n: \
            extstorage_.read(r(n), acc_); \
            clock = 2; \
            break;
        // MOVX A, @Rn
        MOVX_A_RPTR(0)
        MOVX_A_RPTR(1)
        case 0x83: // RET
            pc_ = (pop() & 0x0f) << 8;
            pc_ |= pop();
            clock = 2;
            break;
        case 0x84: // JMP (page 4)
            jmp(4);
            clock = 2;
            break;
        case 0x85: // CLR F0
            psw_.f0 = 0;
            clock = 1;
            break;
        case 0x86: // JNI
            jmp_if(extirq_pending_);
            clock = 2;
            break;
        case 0x89: // ORL P1, #data
            g_p1 |= rom_[pc_++];
            rom_.calculate_current_bank();
            clock = 2;
            break;
        case 0x8a: // ORL P2, #data
            g_p2 |= rom_[pc_++];
            clock = 2;
            break;
#define ORLD_P_A(n, pn) \
        case 0x8c + n: \
            clock = 2; \
            break;
        // ORLD Pn, A
        ORLD_P_A(0, p4_)
        ORLD_P_A(1, p5_)
        ORLD_P_A(2, p6_)
        ORLD_P_A(3, p7_)
#define MOVX_RPTR_A(n) \
        case 0x90 + n: \
            extstorage_.write(r(n), acc_); \
            clock = 2; \
            break;
        // MOVX @Rn, A
        MOVX_RPTR_A(0)
        MOVX_RPTR_A(1)
        case 0x92: // JB4
            jb(4);
            clock = 2;
            break;
        case 0x93: // RETR
            tmp = pop();
            pc_ = (tmp & 0x0f) << 8;
            pc_ |= pop();
            load_psw_no_sp(tmp);
            in_irq_ = false;
            clock = 2;
            break;
        case 0x94: // CALL (page 4)
            call(4);
            clock = 2;
            break;
        case 0x95: // CPL F0
            psw_.cpl_f0();
            clock = 1;
            break;
        case 0x96: // JNZ
            jmp_if(acc_);
            clock = 2;
            break;
        case 0x97: // CLR C
            psw_.cy = 0;
            clock = 1;
            break;
        case 0x99: // ANL P1, #data
            g_p1 &= rom_[pc_++];
            rom_.calculate_current_bank();
            clock = 2;
            break;
        case 0x9a: // ANL P2, #data
            g_p2 &= rom_[pc_++];
            clock = 2;
            break;
#define ANLD_P_A(n, pn) \
        case 0x9c + n: \
            clock = 2; \
            break;
        // ANLD Pn, A
        ANLD_P_A(0, p4_)
        ANLD_P_A(1, p5_)
        ANLD_P_A(2, p6_)
        ANLD_P_A(3, p7_)
#define MOV_RPTR_A(n) \
        case 0xa0 + n: \
            intram_[r(n) & INTRAM_SIZE - 1] = acc_; \
            clock = 1; \
            break;
        // MOV @Rn, A
        MOV_RPTR_A(0)
        MOV_RPTR_A(1)
        case 0xa3: // MOVP A, @A
            acc_ = rom_[pc_ & 0xf00 | acc_];
            clock = 2;
            break;
        case 0xa4: // JMP (page 5)
            jmp(5);
            clock = 2;
            break;
        case 0xa5: // CLR F1
            f1_ = false;
            clock = 1;
            break;
        case 0xa7: // CPL C
            psw_.cpl_cy();
            clock = 1;
            break;
#define MOV_R_A(n) \
        case 0xa8 + n: \
            r(n) = acc_; \
            clock = 1; \
            break;
        // MOV Rn, A
        MOV_R_A(0)
        MOV_R_A(1)
        MOV_R_A(2)
        MOV_R_A(3)
        MOV_R_A(4)
        MOV_R_A(5)
        MOV_R_A(6)
        MOV_R_A(7)
#define MOV_RPTR_DATA(n) \
        case 0xb0 + n: \
            intram_[r(n) & INTRAM_SIZE - 1] = rom_[pc_++]; \
            clock = 2; \
            break;
        // MOV @Rn, #data
        MOV_RPTR_DATA(0)
        MOV_RPTR_DATA(1)
        case 0xb2: // JB5
            jb(5);
            clock = 2;
            break;
        case 0xb3: // JMPP @A
            pc_ = pc_ & 0xf00 | rom_[pc_ & 0xf00 | acc_];
            clock = 2;
            break;
        case 0xb4: // CALL (page 5)
            call(5);
            clock = 2;
            break;
        case 0xb5: // CPL F1
            f1_ = !f1_;
            clock = 1;
            break;
        case 0xb6: // JF0
            jmp_if(psw_.f0);
            clock = 2;
            break;
#define MOV_R_DATA(n) \
        case 0xB8 + n: \
            r(n) = rom_[pc_++]; \
            clock = 2; \
            break;
        // MOV Rn, #data
        MOV_R_DATA(0)
        MOV_R_DATA(1)
        MOV_R_DATA(2)
        MOV_R_DATA(3)
        MOV_R_DATA(4)
        MOV_R_DATA(5)
        MOV_R_DATA(6)
        MOV_R_DATA(7)
        case 0xc4: // JMP (page 6)
            jmp(6);
            clock = 2;
            break;
        case 0xc5: // SEL RB0
            sel_rb0();
            clock = 1;
            break;
        case 0xc6: // JZ
            jmp_if(!acc_);
            clock = 2;
            break;
        case 0xc7: // MOV A, PSW
            acc_ = psw_();
            clock = 1;
            break;
#define DEC_R(n) \
        case 0xc8 + n: \
            --r(n); \
            clock = 1; \
            break;
        // DEC Rn
        DEC_R(0)
        DEC_R(1)
        DEC_R(2)
        DEC_R(3)
        DEC_R(4)
        DEC_R(5)
        DEC_R(6)
        DEC_R(7)
#define XRL_A_RPTR(n) \
        case 0xd0 + n: \
            acc_ ^= rom_[r(n) & INTRAM_SIZE - 1]; \
            clock = 1; \
            break;
        // XRL A, @Rn
        XRL_A_RPTR(0)
        XRL_A_RPTR(1)
        case 0xd2: // JB6
            jb(6);
            clock = 2;
            break;
        case 0xd3: // XRL A, #data
            acc_ ^= rom_[pc_++];
            clock = 2;
            break;
        case 0xd4: // CALL (page 6)
            call(6);
            clock = 2;
            break;
        case 0xd5: // SEL RB1
            sel_rb1();
            clock = 1;
            break;
        case 0xd7: // MOV PSW, A
            load_psw(acc_);
            clock = 1;
            break;
#define XRL_A_R(n) \
        case 0xd8 + n: \
            acc_ ^= r(n); \
            clock = 1; \
            break;
        // XRL A, Rn
        XRL_A_R(0)
        XRL_A_R(1)
        XRL_A_R(2)
        XRL_A_R(3)
        XRL_A_R(4)
        XRL_A_R(5)
        XRL_A_R(6)
        XRL_A_R(7)
        case 0xe3: // MOVP3 A, @A
            acc_ = rom_[0x300 | acc_];
            clock = 2;
            break;
        case 0xe4: // JMP (page 7)
            jmp(7);
            clock = 2;
            break;
        case 0xe5: // SEL MB0
            a11_on_ = false;
            clock = 1;
            break;
        case 0xe6: // JNC
            jmp_if(!psw_.cy);
            clock = 2;
            break;
        case 0xe7: // RL A
            acc_ = (acc_ << 1 | (acc_ & 1 << 7) >> 7) & 0xff;
            clock = 1;
            break;
#define DJNZ_R(n) \
        case 0xe8 + n: \
            jmp_if(--r(n)); \
            clock = 2; \
            break;
        // DJNZ Rn
        DJNZ_R(0)
        DJNZ_R(1)
        DJNZ_R(2)
        DJNZ_R(3)
        DJNZ_R(4)
        DJNZ_R(5)
        DJNZ_R(6)
        DJNZ_R(7)
#define MOV_A_RPTR(n) \
        case 0xf0 + n: \
            acc_ = intram_[r(n) & INTRAM_SIZE - 1]; \
            clock = 1; \
            break;
        // MOV A, @Rn
        MOV_A_RPTR(0)
        MOV_A_RPTR(1)
        case 0xf2: // JB7
            jb(7);
            clock = 2;
            break;
        case 0xf4: // CALL (page 7)
            call(7);
            clock = 2;
            break;
        case 0xf5: // SEL MB1
            if (!in_irq_)
                a11_on_ = true;
            clock = 1;
            break;
        case 0xf6: // JC
            jmp_if(psw_.cy);
            clock = 2;
            break;
        case 0xf7: // RLC A
            acc_ <<= 1;
            acc_ |= psw_.cy;
            psw_.cy = (acc_ & 1 << 8) >> 8;
            acc_ &= 0xff;
            clock = 1;
            break;
#define MOV_A_R(n) \
        case 0xf8 + n: \
            acc_ = r(n); \
            clock = 1; \
            break;
        // MOV A, Rn
        MOV_A_R(0)
        MOV_A_R(1)
        MOV_A_R(2)
        MOV_A_R(3)
        MOV_A_R(4)
        MOV_A_R(5)
        MOV_A_R(6)
        MOV_A_R(7)
        default:
            cout << "Caught illegal instruction!" << endl;
            if (g_options.debug_on_ill) {
                debug_print(cout);
                g_options.debug = true;
            }
            clock = 1;
            break;
    }

    if (tcnt_status_ == TCNT_STATUS_TIMER_ON) {
        // Increment the timer every 32 8048 cycles
        timer_timer_ -= clock;
        if (timer_timer_ <= 0) {
            timer_timer_ = TIMER_TIMER_INITIAL_VALUE;
            tcnt_increment();
        }
    }

    assert(pc_ >= 0 && pc_ < Rom::BANK_SIZE);
    assert(acc_ >= 0 && acc_ <= 0xff);
    assert(clock == 1 || clock == 2);
    return clock;
}
