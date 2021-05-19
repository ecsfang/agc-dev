#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <memory.h>
#include <cstring>

#define ERASABLE_BLK_SIZE   0400
#define FIXED_BLK_SIZE      02000

#define MASK_12B_ADDRESS    07777
#define MASK_10B_ADDRESS    01777
#define MASK_16_BITS        0177777
#define MASK_15_BITS        0077777
#define MASK_14_BITS        0037777
#define MASK_12_BITS        0007777
#define MASK_10_BITS        0001777
#define MASK_8_BITS         0000377
#define MASK_IO_ADDRESS     0000777

#define FB_MEM_START        010000

#define OPCODE_MASK         070000  // Bit 13, 14 and 15
#define QC_MASK             006000  // Bit 11 and 12

#define ERR_ADDR    0xFFFF

#define POS_ZERO     000000
#define NEG_ZERO     077777
#define POS_ONE      000001
#define NEG_ONE      077776

#define BIT_1               (1<<0)
#define BIT_2               (1<<1)
#define BIT_3               (1<<2)
#define BIT_4               (1<<3)
#define BIT_5               (1<<4)
#define BIT_6               (1<<5)
#define BIT_7               (1<<6)
#define BIT_8               (1<<7)
#define BIT_9               (1<<8)
#define BIT_10              (1<<9)
#define BIT_11              (1<<10)
#define BIT_12              (1<<11)
#define BIT_13              (1<<12)
#define BIT_14              (1<<13)
#define BIT_15              (1<<14)
#define BIT_16              (1<<15)

#define S1_MASK             BIT_15
#define S2_MASK             BIT_16
#define OVF_MASK            (1<<16)
#define MANTISSA_MASK       (MASK_15_BITS>>1)

#define IS_POS(x) (((x)&S1_MASK) == 0)
#define IS_NEG(x) (((x)&S1_MASK) != 0)

#define TOTAL_SIZE  (8 * ERASABLE_BLK_SIZE + 38 * FIXED_BLK_SIZE)
#define IO_SIZE     8

#define OUT_IO_SIZE 00020
#define IN_IO_SIZE  01000

#define EB_MASK     003400 // 000 0EE E00 000 000
#define EB_SHIFT         8

#define FB_MASK     076000 // FFF FF0 000 000 000
#define FB_SHIFT        10

#define BB_MASK     076007 // FFF FF0 000 000 EEE

#define CYR_REG      0020
#define SR_REG       0021
#define CYL_REG      0022
#define EDOP_REG     0023
#define IS_EDIT_REG(a) ((a) >= CYR_REG && (a) <= EDOP_REG)

#define BOOT        04000   // Power-up or GOJ signal.
                            // This is where the program begins executing at power-up, and where hardware resets cause execution to go.
#define T6RUPT      04004   // Counter-register TIME6 decremented to 0.
                            // The digital autopilot (DAP) for controlling thrust times of the jets of the reaction control system (RCS).
#define T5RUPT      04010   // Overflow of counter-timer TIME5.
                            // Used by the autopilot.
#define T3RUPT      04014   // Overflow of counter-timer TIME3.
                            // Used by the task scheduler (WAITLIST).
#define T4RUPT      04020   // Overflow of counter-timer TIME4.
                            // Used for various DSKY-related activities such as monitoring the PRO key and updating display data.
#define KEYRUPT1    04024   // Keystroke received from DSKY.
                            // The DSKY transmits codes representing keystrokes to the AGC.  Reception of these codes by the AGC hardware triggers an interrupt.
#define KEYRUPT2    04030   // Keystroke received from secondary DSKY.
                            // In the CM, there was a second DSKY at the navigator's station, used for star-sighting data, in conjunction with the Alignment Optical Telescope (AOT).  There was no 2nd DSKY in the LM, but the interrupt was not re-purposed.
#define UPRUPT      04034   // Uplink word available in the INLINK register.
                            // Data transmitted from ground-control for the purpose of controlling or monitoring the AGC is in the form of a serial data stream which is assembled in AGC INLINK counter-register.  When a word has been assembled in this fashion, an interrupt is triggered.
#define DOWNRUPT    04040   // The downlink shift register is ready for new data (output channels 34 & 35).
                            // Used for telemetry-downlink.
#define RADAR_RUPT  04044   // Automatically generated inside the AGC after a set pulse sequence has been sent to the radars.
                            // Data from the rendezvous radar is assembled similarly to the uplink data described above.  When a data word is complete, an interrupt is triggered.
#define RUPT10      04050   // aka HANDRUPT
                            // Selectable from three possible sources:
                            // Trap 31A, Trap 31B, and Trap 32.

/**
#  ENGINE ON BIT 13 OF CHANNEL 11
#  0 MEANS OFF
#  1 MEANS ON

#  BIT10 OF CHANNEL 30 IS PNGCS CONTROL OF S/C

#  BIT13 OF CHANNEL 31 ATTITUDE HOLD BIT. INVERTED.
*/

extern FILE    *logFile;

enum {
    REG_A,          // 00
    REG_L,          // 01
    REG_Q,          // 02
    REG_EB,         // 03 "erasable bank register"
    REG_FB,         // 04 "fixed bank register"
    REG_Z,          // 05
    REG_BB,         // 06
    REG_ZERO,       // 07
    REG_ARUPT,      // 10
    REG_LRUPT,      // 11
    REG_QRUPT,      // 12
    REG_SAMPTIME2,  // 13
    REG_SAMPTIME1,  // 14
    REG_ZRUPT,      // 15
    REG_BBRUPT,     // 16
    REG_BRUPT,      // 17
    REG_SYR,        // 20
    REG_SR,         // 21
    REG_CYL,        // 22
    REG_EDOP,       // 23
    REG_TIME2,      // 24
    REG_TIME1,      // 25
    REG_TIME3,      // 26
    REG_TIME4,      // 27
    REG_TIME5,      // 30
    REG_TIME6,      // 31
    REG_CDUX,       // 32
    REG_CDUY,       // 33
    REG_CDUZ,       // 34
    REG_OPTY,       // 35
    REG_OPTX,       // 36
    REG_PIPAX,      // 37
    REG_PIPAY,      // 40
    REG_PIPAZ,      // 41
    REG_Q_RHCCTR,   // 42
    REG_P_RHCCTR,   // 43
    REG_R_RHCCTR,   // 44
    REG_INLINK,     // 45
    REG_RNRAD,      // 46
    REG_GYROCTR,    // 47
    REG_CDUXCMD,    // 50
    REG_CDUYCMD,    // 51
    REG_CDUZCMD,    // 52
    REG_OPTYCMD,    // 53
    REG_OPTXCMD,    // 54
    REG_THRUST,     // 55
    REG_LEMONM,     // 56
    REG_OUTLINK,    // 57
    REG_ALTM,       // 60
};


typedef union {
    struct {
        __uint16_t  A;          // 00
        __uint16_t  L;          // 01
        __uint16_t  Q;          // 02
        __uint16_t  EB;         // 03 "erasable bank register"
        __uint16_t  FB;         // 04 "fixed bank register"
        __uint16_t  Z;          // 05
        __uint16_t  BB;         // 06
        __uint16_t  _res;       // 07
        __uint16_t  ARUPT;      // 10
        __uint16_t  LRUPT;      // 11
        __uint16_t  QRUPT;      // 12
        __uint32_t  SAMPTIME;   // 13-14
        __uint16_t  ZRUPT;      // 15
        __uint16_t  BBRUPT;     // 16
        __uint16_t  BRUPT;      // 17
        __uint16_t  SYR;        // 20
        __uint16_t  SR;         // 21
        __uint16_t  CYL;        // 22
        __uint16_t  EDOP;       // 23
        __uint16_t  TIME2;      // 24
        __uint16_t  TIME1;      // 25
        __uint16_t  TIME3;      // 26
        __uint16_t  TIME4;      // 27
        __uint16_t  TIME5;      // 30
        __uint16_t  TIME6;      // 31
        __uint16_t  CDUX;       // 32
        __uint16_t  CDUY;       // 33
        __uint16_t  CDUZ;       // 34
        __uint16_t  OPTY;       // 35
        __uint16_t  OPTX;       // 36
        __uint16_t  PIPAX;      // 37
        __uint16_t  PIPAY;      // 40
        __uint16_t  PIPAZ;      // 41
        __uint16_t  Q_RHCCTR;   // 42
        __uint16_t  P_RHCCTR;   // 43
        __uint16_t  R_RHCCTR;   // 44
        __uint16_t  INLINK;     // 45
        __uint16_t  RNRAD;      // 46
        __uint16_t  GYROCTR;    // 47
        __uint16_t  CDUXCMD;    // 50
        __uint16_t  CDUYCMD;    // 51
        __uint16_t  CDUZCMD;    // 52
        __uint16_t  OPTYCMD;    // 53
        __uint16_t  OPTXCMD;    // 54
        __uint16_t  THRUST;     // 55
        __uint16_t  LEMONM;     // 56
        __uint16_t  OUTLINK;    // 57
        __uint16_t  ALTM;       // 60
    };
    __uint16_t  word[TOTAL_SIZE];
}   Mem_t;

class CMemory {
    Mem_t   mem;
    __uint16_t  inIoMem[IN_IO_SIZE];
    __uint16_t  outIoMem[OUT_IO_SIZE];
    __uint16_t  FEB;
    bool        bDSky;
public:
    CMemory() {
        // Clear all memory
        memset(&mem, 0, sizeof(Mem_t));
        // Clear i/o channels.
        memset(inIoMem,0,IN_IO_SIZE*sizeof(__uint16_t));
        memset(outIoMem,0,OUT_IO_SIZE*sizeof(__uint16_t));
        // Init some i/o channels
        inIoMem[030] = 037777;
        inIoMem[031] = 077777;
        inIoMem[032] = 077777;
        inIoMem[033] = 077777;
        bDSky = false;
        FEB = 0;
    }
    bool dsky(void) {
        bool dsky = bDSky;
        bDSky = false;
        return dsky;
    }
    __uint16_t  *getOutMem(void) { return outIoMem; }
    __uint16_t getOP(int offs = 0) {
        return read12((mem.Z+offs) & MASK_12B_ADDRESS);
    }
    void setZ(__uint16_t pc) {
        mem.Z = pc;
    }
    __uint16_t getZ(void) {
        return mem.Z;
    }
    __uint16_t getPysZ(void) {
        return addr2mem(mem.Z);
    }
    void setA(__uint16_t acc) {
        mem.A = acc;
    }
    __uint16_t getA(void) {
        return mem.A;
    }
    void setL(__uint16_t acc) {
        mem.L = acc;
    }
    __uint16_t getL(void) {
        return mem.L;
    }
    void setQ(__uint16_t rtn) {
        mem.Q = rtn;
    }
    __uint16_t getQ(void) {
        return mem.Q;
    }
    __uint16_t getBB(void) {
        return mem.BB;
    }
    __uint16_t getT1(void) {
        return mem.TIME1;
    }
    __uint16_t getT2(void) {
        return mem.TIME2;
    }
    __uint16_t getT3(void) {
        return mem.TIME3;
    }
    __uint16_t getT4(void) {
        return mem.TIME4;
    }

    bool incTimer(__uint16_t t) {
        __uint16_t tv = inc(t);
        if( tv & BIT_15 ) {
            write12(t, 0);
            return true;
        }
        return false;
    }

    __uint16_t step() {
        return ++mem.Z;
    }
    void setEB(__uint16_t eb) {
        mem.EB = eb;
        mem.BB = mem.FB | (mem.EB >> EB_SHIFT);
    }
    void setFB(__uint16_t fb) {
        mem.FB = fb;
        mem.BB = mem.FB | (mem.EB >> EB_SHIFT);
    }
    void setBB(__uint16_t bb) {
        mem.BB = bb;
        mem.EB = (mem.BB & 07) << EB_SHIFT;
        mem.FB = mem.BB & FB_MASK;
    }
    // Write data to a logical address
    void write12(__uint16_t addr, __uint16_t data) {
        __uint16_t _addr = addr2mem(addr);
        if( _addr != ERR_ADDR )
            writePys(_addr, data);
    }
    // Write data to a physical address
    void writePys(__uint16_t addr, __uint16_t data) {
        if( addr < TOTAL_SIZE ) {
            switch( addr ) {
                case 00000: setA(data); break;
                case 00003: setEB(data); break;
                case 00004: setFB(data); break;
                case 00006: setBB(data); break;
                default:
                    if( addr >= 04000 && addr < 010000 )
                        addr += 010000;
//                    if( addr == REG_Z )
//                        fprintf(logFile, "UPDATING Z-REG!! mem[%05o] = %05o\n", addr, data);
                    switch(addr) {
                        case CYR_REG:
                            data = (((data & 0x7FFF) >> 1 ) | (data << 14) ) & 0x7FFF;
                            break;
                        case SR_REG:
                            data = ((data & 0x4000) | (data >> 1)) & 0x7FFF;
                            break;
                        case CYL_REG:
                            data = (((data & 0x4000) >> 14 ) | (data << 1)) & 0x7FFF;
                            break;
                        case EDOP_REG:
                            data = (data >> 8) & 0x7F;
                            break;
                    }
                    mem.word[addr] = data;
            }
        }
    }
    __uint16_t readIO(__uint16_t addr)
    {
        if (addr < 0 || addr > 0777)
            return 0;
        switch( addr ) {
            case REG_L:
                return getL();
            case REG_Q:
                return getQ();
//            case 012: return BIT_4;
            default:
                return inIoMem[addr];
        }
    }

    void writeIO(__uint16_t addr, __uint16_t data)
    {
        // The value should be in AGC format. 
        data &= 077777;
        if (addr < 0 || addr > 0777)
            return;
        switch( addr ) {
        case REG_L:
            setL(data);
            break;
        case REG_Q:
            setQ(data);
            break;
        default:
            switch(addr) {
            case 010:
                // Channel 10 is converted externally to the CPU into up to 16 ports,
                // by means of latching relays.  We need to capture this data.
                outIoMem[(data>>11) & 017] = data;
                bDSky = true;
                break;
            case 013:
                // Enable the appropriate traps for HANDRUPT. Note that the trap
                // settings cannot be read back out, so after setting the traps the
                // enable bits are masked out.
#if 0
                if (data & 004000)
                    State->Trap31A = 1;
                if (data & 010000)
                    State->Trap31B = 1;
                if (data & 020000)
                    State->Trap32 = 1;
#endif
                data &= 043777;
                break;    
            case 015:
            case 016:
                if( data == 022 ) {
                    // RSET being pressed on either DSKY clears the RESTART light
                    // flip-flop directly, without software intervention
                    // RestartLight = 0;
                }
                break;
            case 033:
                // Channel 33 bits 11-15 are controlled internally, so don't let
                // anybody write to them
                data = (inIoMem[addr] & 076000) | (data & 001777);
                break;
            }
            inIoMem[addr] = data;
        }
    }

    void  update(__uint16_t addr) {
        write12(addr, read12(addr));
    }
    __uint16_t  inc(__uint16_t addr) {
        __uint16_t i = read12(addr)+1; 
        write12(addr, i);
        return i;
    }
#define SIGN_EXTEND(w) ((w & MASK_15_BITS) | ((w << 1) & S2_MASK))
    __uint16_t  readPys(__uint16_t addr) {
        if( addr >= 04000 && addr < 010000 )
            addr += 010000;
        return addr < TOTAL_SIZE ? mem.word[addr] : 0;
    }
    __uint16_t  read12(__uint16_t addr) {
        __uint16_t _addr = addr2mem(addr);
        if( _addr != ERR_ADDR )
            return readPys(_addr);
        return ERR_ADDR;
    }
#if 0
    // Map 12 bit address to physical address
    __uint16_t  addr2mem(__uint16_t addr) {
        __uint16_t  _addr = 0;
        if( addr < 01400) {
            // Unswitched erasable memory
            _addr = addr;
        } else if( addr < 02000) {
            // Switched erasable memory
            if( mem.EB & ~EB_MASK ) {
                printf("Invalid EB: %05o!\n", mem.EB);
                return ERR_ADDR;
            }
            _addr = addr-01400 + mem.EB;
        } else if( addr < 04000) {
            // Common fixed memory
            __int16_t   bank = (mem.FB & FB_MASK) >> FB_SHIFT;
            if( bank > 027 && FEB == 1 )
                bank += 010;
            if( bank > 033 ) {
                printf("Invalid bank: %03o!\n", bank);
                return ERR_ADDR;
            }
            _addr = addr + bank*FIXED_BLK_SIZE + 010000 - 02000;
        } else if( addr < 010000) {
            // Unswitched fixed memory
            _addr = addr;
        } else {
            printf("Invalid address: %06o!\n", addr);
            return ERR_ADDR;
        }
        //printf("Addr: %06o -> %06o\n", addr, _addr);
        return _addr;
    }
#else
    // Map 12 bit address to physical address
    __uint16_t  addr2mem(__uint16_t addr) {
        __uint16_t  _addr;
        __uint8_t   sub = 0;
        if( (addr & (BIT_11|BIT_12)) == 00 ) {
            if( (addr & (BIT_9|BIT_10)) == (BIT_9|BIT_10) ) {
                // Erasable-switched memory ...
                return (addr & MASK_8_BITS) | (mem.EB & EB_MASK);
            } else {
                // Erasable memory ...
                return addr & MASK_10_BITS;
            }
        }
        if( (addr & BIT_12) == 0 ) {
            // Fixed-switched memory
            _addr = (addr & MASK_10_BITS) | (mem.FB & FB_MASK);
            if( sub && (_addr & (BIT_14|BIT_15)) == (BIT_14|BIT_15) ) {
                return (_addr | BIT_16) + FB_MEM_START;
            } else {
                return _addr + FB_MEM_START;
            }
        } else {
            // Fixed-fixed memory (4000-7777)
            return addr & MASK_12_BITS;
        }
    }
#endif
    void init(void);
};

#endif//__MEMORY_H__