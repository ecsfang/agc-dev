#ifndef __CPU_H__
#define __CPU_H__

#include <ncurses.h>
#include "memory.h"

extern FILE *logFile;

class CCpu {
    CMemory mem;
    bool bExtracode = false;
    bool bClrExtra = true;
    bool bInterrupt = false;
    bool bStep = false;
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
public:
    CCpu() {
        mem.read(04000);
        mem.write(04000, 012345);
        mem.read(04000);
        mem.setZ(04000);
        idx = 0;
        bOF = false;
        memset(bps,0,TOTAL_SIZE*sizeof(bool));
//        mem.init();
    }
    // Set breakpoint in memory
    void setBrkp(uint16_t bp) {
        bps[bp] = true;
    }
    uint16_t ovf_corr(uint16_t ov) {
        return OF() ? (ov & 0x3FFF) | s2 : ov;
    }
    void readCore(char *core);
    // Read opcode
    __uint16_t getOP(bool bUpdate=true, int offs=0) {
        opc = mem.getOP(offs);
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
        s2 = x1 & 0x4000;
        __uint16_t s = AddSP16(x1,x2);
        __uint16_t cs;

        fprintf(logFile,"1stADD: s: %05o\n", s);

//        if( s & 0x8000 )
//            s++;
        bOF = s2 != (s & 0x4000);
        cs = s;
        //printf("(s2:%d s1:%d of:%d)", s2 ? 1:0, (s&0x4000)?1:0, );
        if( bOF ) {
            // Overflow correction
            cs = ovf_corr(cs);
        }
        fprintf(logFile,"1stADD: %05o + %05o = %05o %c [%05o] (S2:%d S1:%d)\n", x1, x2, s, bOF?'*':' ', cs, s2&0x4000?1:0, s&0x4000?1:0);
        fflush(logFile);
        return s; // & 0x7FFF;
    }
    void setA(uint16_t a) {
        mem.setA(a);
        s2 = a & 0x4000;
        bOF = false;
    }
    void setL(uint16_t l) {
        mem.setL(l);
    }
    uint16_t getPC(void) {
        return mem.getZ();
    }
    void setPC(uint16_t pc) {
        return mem.setZ(pc);
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
        return ((Value & 037777) | ((Value >> 1) & 040000));
    }

    // Sign-extend a 15-bit SP value so that it can go into the 16-bit (plus parity)
    // accumulator.
    int SignExtend (int16_t Word) {
        return ((Word & 077777) | ((Word << 1) & 0100000));
    }

    int16_t ValueOverflowed (int Value) {
        switch (Value & 0140000) {
        case 0040000:
            return (POS_ONE);
        case 0100000:
            return (NEG_ONE);
        default:
            return (POS_ZERO);
        }
    }
    // Adds two sign-extended SP values.  The result may contain overflow.
    int AddSP16 (int Addend1, int Addend2) {
        int Sum;
        Sum = Addend1 + Addend2;
        if (Sum & 0200000) {
            Sum += POS_ONE;
            Sum &= 0177777;
        }
        return (Sum);
    }
    void SimulateDV(uint16_t a, uint16_t l, uint16_t divisor);
    int agc2cpu (int Input)
{
  if (0 != (040000 & Input))
    return (-(037777 & ~Input));
  else
    return (037777 & Input);
}

    int cpu2agc2 (int Input)
{
  if (Input < 0)
    return (03777777777 & ~(01777777777 & (-Input)));
  else
    return (01777777777 & Input);
}

    void DecentToSp (int Decent, int16_t * LsbSP)
{
  int Sign;
  Sign = (Decent & 04000000000);
  *LsbSP = (037777 & Decent);
  if (Sign)
    *LsbSP |= 040000;
  LsbSP[-1] = OverflowCorrected (0177777 & (Decent >> 14));	// Was 13.
}

};

#endif//__CPU_H__