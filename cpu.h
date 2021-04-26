#ifndef __CPU_H__
#define __CPU_H__

#include <ncurses.h>
#include "memory.h"

extern FILE *logFile;

class CCpu {
    CMemory mem;
    bool bExtracode = false;
    bool bInterrupt = false;
    bool bStep = false;
    __uint16_t  opc;    // The current opcode
    __uint16_t  k12;    // Current 12 bit k value
    __uint16_t  k10;    // Current 10 bit k value
    __uint8_t   qc;     // Current Quarter Code
    __uint16_t  idx;    // Current index-value
    bool        bOF;
    __uint16_t  s2;     // The 16th bit ...
    char        disBuf[256];
    int         pDis;
public:
    CCpu() {
        mem.read(04000);
        mem.write(04000, 012345);
        mem.read(04000);
        mem.setZ(04000);
        idx = 0;
        bOF = false;
//        mem.init();
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
        __uint16_t s = x1 + x2;
        __uint16_t cs;

        if( s & 0x8000 )
            s++;
        bOF = s2 != (s & 0x4000);
        cs = s;
        //printf("(s2:%d s1:%d of:%d)", s2 ? 1:0, (s&0x4000)?1:0, );
        if( bOF ) {
            // Overflow correction
            cs &= 0x3FFF;
            cs |= s2 ? 0x4000 : 0;
        }
        fprintf(logFile,"1stADD: %05o + %05o = %05o %c [%05o] (S2:%d S1:%d)\n", x1, x2, s, bOF?'*':' ', cs, s2&0x4000?1:0, s&0x4000?1:0);
        fflush(logFile);
        return s & 0x7FFF;
    }
    void setA(uint16_t a) {
        mem.setA(a);
        s2 = a & 0x4000;
        bOF = false;
    }
    uint16_t getPC(void) {
        return mem.getZ();
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
};

#endif//__CPU_H__