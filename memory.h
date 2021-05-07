#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <memory.h>

#define ERASABLE_BLK_SIZE   0400
#define FIXED_BLK_SIZE      02000

#define MASK_12B_ADDRESS    07777
#define MASK_10B_ADDRESS    01777
#define MASK_16_BITS        0177777
#define MASK_15_BITS        0077777
#define MASK_14_BITS        0037777
#define MASK_12_BITS        0007777
#define MASK_IO_ADDRESS     0000777

#define OPCODE_MASK         070000  // Bit 13, 14 and 15
#define QC_MASK             006000  // Bit 11 and 12

#define ERR_ADDR    0xFFFF

#define POS_ZERO     000000
#define NEG_ZERO     077777
#define POS_ONE      000001
#define NEG_ONE      077776

#define S1_MASK             (1<<14)
#define S2_MASK             (1<<15)
#define OVF_MASK            (1<<16)
#define MANTISSA_MASK       (MASK_15_BITS>>1)

#define IS_POS(x) (((x)&S1_MASK) == 0)
#define IS_NEG(x) (((x)&S1_MASK) != 0)

#define TOTAL_SIZE  (8 * ERASABLE_BLK_SIZE + 38 * FIXED_BLK_SIZE)
#define IO_SIZE     8

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

extern FILE    *logFile;

enum {
    REG_A,          // 00
    REG_L,          // 01
    REG_Q,          // 02
    REG_EB,         // 03 "erasable bank register"
    REG_FB,         // 04 "fixed bank register"
    REG_Z,          // 05
    REG_BB,         // 06
    REG__res,       // 07
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
    __uint16_t  ioMem[IO_SIZE];
    __uint16_t  FEB;
public:
    CMemory() {
        memset(&mem, 0, sizeof(Mem_t));
        memset(ioMem,0,IO_SIZE*sizeof(__uint16_t));
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

    __uint16_t incTimer(__uint16_t t) {
        __uint16_t tv = read(t);
        write(t, (tv+1) & MASK_14_BITS);
        return (tv == MASK_14_BITS);
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
                    if( addr == REG_Z )
                        printf("UPDATING Z-REG!! mem[%05o] = %05o\n", addr, data);
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
    __uint16_t readIO(__uint16_t addr)
    {
        switch( addr ) {
            case REG_L:
                return getL();
            case REG_Q:
                return getQ();
            default:
                if( addr < IO_SIZE )
                    return ioMem[addr];
                return 0;
        }
    }

    void writeIO(__uint16_t addr, __uint16_t data)
    {
        switch( addr ) {
            case REG_L:
                setL(data);
                break;
            case REG_Q:
                setQ(data);
                break;
            default:
                if( addr < IO_SIZE )
                    ioMem[addr] = data;
        }
    }

    void  update(__uint16_t addr) {
        write(addr, read(addr));
    }
    void  inc(__uint16_t addr) {
        write(addr, read(addr)+1);
    }
#define SIGN_EXTEND(w) ((w & MASK_15_BITS) | ((w << 1) & S2_MASK))
    __uint16_t  read(__uint16_t addr) {
        if( addr >= 04000 && addr < 010000 )
            addr += 010000;
//        return addr < TOTAL_SIZE ? SIGN_EXTEND(mem.word[addr]) : 0;
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