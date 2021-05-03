#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <memory.h>

#define ERASABLE_BLK_SIZE   0400
#define FIXED_BLK_SIZE      02000

#define MASK_12B_ADDRESS    07777
#define MASK_10B_ADDRESS    01777
#define MASK_IO_ADDRESS     00777
#define MASK_16_BITS        0177777
#define MASK_15_BITS        077777
#define MASK_12_BITS        07777

#define OPCODE_MASK         070000  // Bit 13, 14 and 15
#define QC_MASK             006000  // Bit 11 and 12

#define ERR_ADDR    0xFFFF

#define POS_ZERO    000000
#define NEG_ZERO    077777
#define POS_ONE     000001
#define NEG_ONE     077776

#define IS_POS(x) (((x)&0x4000) == 0)
#define IS_NEG(x) (((x)&0x4000) != 0)

#define TOTAL_SIZE  (8 * ERASABLE_BLK_SIZE + 38 * FIXED_BLK_SIZE)

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

enum {
    REG_A,
    REG_L,
    REG_Q,
    REG_EB,
    REG_FB,
    REG_Z,
    REG_BB
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
    __uint16_t  FEB;
public:
    CMemory() {
        memset(&mem, 0, sizeof(Mem_t));
        FEB = 0;
        //for(int i=0; i<TOTAL_SIZE; i++)
        //    mem.word[i] = i;
        setEB(0);
        setFB(0);    
    }
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
    void step() {
        mem.Z = mem.Z + 1;
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
    void writeBank(__uint8_t bank, __uint16_t addr, __uint16_t data);
    // Write data to a physical address
    void write(__uint16_t addr, __uint16_t data) {
        if( addr < TOTAL_SIZE ) {
            switch( addr ) {
                case 00003: setEB(data); break;
                case 00004: setFB(data); break;
                case 00006: setBB(data); break;
                default:
                    if( addr >= 04000 && addr < 010000 )
                        addr += 010000;
                    //printf("mem[%05o] = %05o\n", addr, data);
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
    // Write data to a physical address
    void write12(__uint16_t addr, __uint16_t data) {
        __uint16_t _addr = addr2mem(addr);
        if( _addr != ERR_ADDR )
            write(_addr, data);
    }
    void writeIO(__uint16_t io, __uint16_t data) {
        printf("(write %05o -> IO:%o!) ", data, io);
    }
    __uint16_t  readIO(__uint16_t io) {
        printf("(read IO:%o!) ", io);
        return 0;
    }
    __uint16_t  read(__uint16_t addr) {
        if( addr >= 04000 && addr < 010000 )
            addr += 010000;
        return addr < TOTAL_SIZE ? mem.word[addr] : 0;
    }
    __uint16_t  read12(__uint16_t addr) {
        __uint16_t _addr = addr2mem(addr);
        if( _addr != ERR_ADDR )
            return read(_addr);
        return ERR_ADDR;
    }
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
    void init(void);
};

#endif//__MEMORY_H__