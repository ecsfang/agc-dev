#ifndef __CPU_H__
#define __CPU_H__

#include <ncurses.h>
#include "memory.h"

extern FILE *logFile;

#define POS_OVF() (bOF && s2 == 0)
#define NEG_OVF() (bOF && s2 != 0)


class CCpu {
    CMemory mem;
    bool bExtracode = false;
    bool bClrExtra = true;
    bool bInterrupt = false;
    bool bIntRunning = false;
    __uint16_t  opc;    // The current opcode
    __uint16_t  k12;    // Current 12 bit k value
    __uint16_t  k10;    // Current 10 bit k value
    __uint8_t   qc;     // Current Quarter Code
    __uint16_t  idx;    // Current index-value
    __uint16_t  useIdx;
    bool        bOF;
    __uint16_t  s2;     // The 16th bit ...
    char        disBuf[256];
    int         pDis;
    bool        bps[TOTAL_SIZE];
    uint16_t    mwAddr;
    uint16_t    mwBreak;
    uint32_t    clockCnt;
    uint16_t    dTime;
    uint8_t     mct;
    uint32_t    gInterrupt;
    uint16_t    nextPC;

public:
    CCpu() {
//        mem.read12(04000);
//        mem.write12(04000, 012345);
//        mem.read12(04000);
        mem.setZ(BOOT);
        idx = 0;
        bOF = false;
        memset(bps,0,TOTAL_SIZE*sizeof(bool));
        mwAddr = 0;
        clockCnt = 0;
        dTime = 0;
        gInterrupt = 0;
//        mem.init();
    }
    // Set breakpoint in memory
    void setBrkp(uint16_t bp) {
        bps[bp] = true;
    }
    // Set breakpoint in memory
    void memWatch(uint16_t mw) {
        mwAddr = mw;
    }
    void memBreak(uint16_t mw) {
        mwBreak = mw;
    }
    uint16_t ovf_corr(uint16_t ov) {
        return OF() ? (ov & 0x3FFF) | s2 : ov;
    }
    void readCore(char *core);
    // Read opcode
    __uint16_t getOP(bool bUpdate=true, int offs=0) {
        opc = mem.getOP(offs) & MASK_15_BITS;
        if( bUpdate )
            opc += idx;
        k12 = opc & MASK_12B_ADDRESS;
        k10 = opc & MASK_10B_ADDRESS;
        qc = (opc & QC_MASK) >> 10;
        if( bUpdate )
            idx = 0;
        return opc;
    }
    bool OF(void) {
        return bOF;
    }
    __uint16_t add1st(__uint16_t x1, __uint16_t x2)
    {
//        s2 = x1 & 040000;
        __uint16_t s = AddSP16(x1,x2);
        __uint16_t cs;

//        fprintf(logFile,"1stADD: s: %05o\n", s);

//        if( s & 0x8000 )
//            s++;
        bOF |= s2 != (s & 0x4000);
        cs = s;
        //printf("(s2:%d s1:%d of:%d)", s2 ? 1:0, (s&0x4000)?1:0, );
        if( bOF ) {
            // Overflow correction
            cs = ovf_corr(cs);
        }
//        fprintf(logFile,"1stADD: %05o + %05o = %05o %c [%05o] (S2:%d S1:%d)\n", x1, x2, s, bOF?'*':' ', cs, s2&0x4000?1:0, s&0x4000?1:0);
//        fflush(logFile);
        return s; // & 0x7FFF;
    }
    void setA(uint16_t a) {
        mem.setA(a);
        s2 = a & 0x4000;
        //bOF = false;
    }
    void setL(uint16_t l) {
        mem.setL(l);
    }
    uint16_t getPC(void) {
        return mem.getZ();
    }
    uint16_t getAbsPC(void) {
        return mem.getPysZ();
    }
    void setPC(uint16_t pc) {
        return mem.setZ(pc);
    }
    char *mAddr(uint16_t a);

    CMemory *getMem(void) {
        return &mem;
    }

    int sst(void);

    int op0(void);
    int op1(void);
    int op2(void);
    int op3(void);
    int op4(void);
    int op5(void);
    int op6(void);
    int op7(void);
    int op0ex(void);
    int op1ex(void);
    int op2ex(void);
    int op3ex(void);
    int op4ex(void);
    int op5ex(void);
    int op6ex(void);
    int op7ex(void);

    // Disassembler
    char *disasm(int offs=0);
    void dis0(void);
    void dis1(void);
    void dis2(void);
    void dis3(void);
    void dis4(void);
    void dis5(void);
    void dis6(void);
    void dis7(void);
    void dis0ex(void);
    void dis1ex(void);
    void dis2ex(void);
    void dis3ex(void);
    void dis4ex(void);
    void dis5ex(void);
    void dis6ex(void);
    void dis7ex(void);

    void dispReg(WINDOW *win);

    int16_t OverflowCorrected (int Value) {
        return ((Value & MANTISSA_MASK) | ((Value >> 1) & S1_MASK));
    }

    // Sign-extend a 15-bit SP value so that it can go into the 16-bit (plus parity)
    // accumulator.
    int SignExtend (int16_t Word) {
        return ((Word & MASK_15_BITS) | ((Word << 1) & S2_MASK));
    }

    int16_t ValueOverflowed (int Value) {
        switch (Value & (S1_MASK|S2_MASK)) {
        case S1_MASK:
            return (POS_ONE);
        case S2_MASK:
            return (NEG_ONE);
        default:
            return (POS_ZERO);
        }
    }
    // Adds two sign-extended SP values.  The result may contain overflow.
    uint16_t AddSP16 (uint32_t Addend1, uint32_t Addend2) {
        uint32_t Sum;
        Sum = Addend1 + Addend2;
        if (Sum & OVF_MASK) {
            Sum += POS_ONE;
            Sum &= MASK_16_BITS;
        }
        //fprintf(logFile,"AddSP16: %05o + %05o = %05o\n", Addend1, Addend2, Sum);
        return (Sum);
    }
    void SimulateDV(uint16_t a, uint16_t l, uint16_t divisor);

    int agc2cpu(int Input)
    {
        if (0 != (S1_MASK & Input))
            return (-(MANTISSA_MASK & ~Input));
        else
            return (MANTISSA_MASK & Input);
    }

    int cpu2agc2(int Input)
    {
        if (Input < 0)
            return (03777777777 & ~(01777777777 & (-Input)));
        else
            return (01777777777 & Input);
    }

    void DecentToSp(int Decent, int16_t *LsbSP)
    {
        int Sign;
        Sign = (Decent & 04000000000);
        *LsbSP = (MANTISSA_MASK & Decent);
        if (Sign)
            *LsbSP |= S1_MASK;
        LsbSP[-1] = OverflowCorrected(MASK_16_BITS & (Decent >> 14)); // Was 13.
    }
    void addInterrupt(int i);
    uint16_t handleInterrupt(void);
    void incTime(void);
    void incTIME1(void);
};

#endif//__CPU_H__