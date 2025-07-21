#ifndef __CPU_H__
#define __CPU_H__

#include <ncurses.h>
#include "memory.h"

extern FILE *logFile;

#define POS_OVF() (bOF && s2 == 0)
#define NEG_OVF() (bOF && s2 != 0)

typedef enum {
    DSKY_0          = 16,
    DSKY_1          = 1,
    DSKY_2          = 2,
    DSKY_3          = 3,
    DSKY_4          = 4,
    DSKY_5          = 5,
    DSKY_6          = 6,
    DSKY_7          = 7,
    DSKY_8          = 8,
    DSKY_9          = 9,
    DSKY_VERB       = 17,
    DSKY_RSET       = 18,
    DSKY_KEY_REL    = 25,
    DSKY_PLUS       = 26,
    DSKY_MINUS      = 27,
    DSKY_ENTR       = 28,
    DSKY_CLR        = 30,
    DSKY_NOUN       = 31,
    DSKY_PRO        = 128
} Key_e;

#define NR_INTS     11
#define iBOOT       (1<<0)
#define iT6RUPT     (1<<1)
#define iT5RUPT     (1<<2)
#define iT3RUPT     (1<<3)
#define iT4RUPT     (1<<4)
#define iKEYRUPT1   (1<<5)
#define iKEYRUPT2   (1<<6)
#define iUPRUPT     (1<<7)
#define iDOWNRUPT   (1<<8)
#define iRADARRUPT  (1<<9)
#define iRUPT10     (1<<10)

class CCpu {
    CMemory mem;
    bool bExtracode = false;
    bool bClrExtra = true;
    bool bInterrupt = false;
    bool bIntRunning = false;
    bool bTime6Enabled = false;
    uint16_t    intRunning = 0;
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
    uint32_t    getClock(void) {
        return clockCnt;
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
            opc  = AddSP16(SignExtend(opc), SignExtend(idx)); //+= idx;
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
        bOF |= s2 != (s & S1_MASK);
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
        s2 = a & S1_MASK;
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
    char *disasm(int offs=0, bool bUp=true);
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
    void updateDSKY(WINDOW *win, bool bRun);

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
//        fprintf(logFile,"AddSP16: %05o + %05o = %05o\n", Addend1, Addend2, Sum);
        if (Sum & OVF_MASK) {
            Sum += POS_ONE;
            Sum &= MASK_16_BITS;
//            fprintf(logFile,"AddSP16: Overflow correctio: %05o\n", Sum);
        }
        return (Sum);
    }

    //-----------------------------------------------------------------------------
    // Here are functions to convert a DP into a more-decent 1's-
    // complement format in which there's not an extra sign-bit to contend with.
    // (In other words, a 29-bit format in which there's a single sign bit, rather
    // than a 30-bit format in which there are two sign bits.)  And vice-versa.
    // The DP value consists of two adjacent SP values, MSW first and LSW second,
    // and we're given a pointer to the second word.  The main difficulty here
    // is dealing with the case when the two SP words don't have the same sign,
    // and making sure all of the signs are okay when one or more words are zero.
    // A sign-extension is added a la the normal accumulator.

    int SpToDecent (int16_t * LsbSP)
    {
        int16_t Msb, Lsb;
        int Value, Complement;
        Msb = LsbSP[-1];
        Lsb = *LsbSP;
        if (Msb == POS_ZERO || Msb == NEG_ZERO)	{ // Msb is zero.
            // As far as the case of the sign of +0-0 or -0+0 is concerned,
            // we follow the convention of the DV instruction, in which the
            // overall sign is the sign of the less-significant word.
            Value = SignExtend (Lsb);
            if (Value & 0100000)
                Value |= ~0177777;
            return (07777777777 & Value);	// Eliminate extra sign-ext. bits.
        }
        // If signs of Msb and Lsb words don't match, then make them match.
        if ((040000 & Lsb) != (040000 & Msb)) {
            if (Lsb == POS_ZERO || Lsb == NEG_ZERO)	{ // Lsb is zero.
                // Adjust sign of Lsb to match Msb.
                if (0 == (040000 & Msb))
                    Lsb = POS_ZERO;
                else
                    Lsb = NEG_ZERO;	// 2005-08-17 RSB.  Was "Msb".  Oops!
            } else {	// Lsb is not zero.
                // The logic will be easier if the Msb is positive.
                Complement = (040000 & Msb);
                if (Complement) {
                    Msb = (077777 & ~Msb);
                    Lsb = (077777 & ~Lsb);
                }
                // We now have Msb positive non-zero and Lsb negative non-zero.
                // Subtracting 1 from Msb is equivalent to adding 2**14 (i.e.,
                // 0100000, accounting for the parity) to Lsb.  An additional 1 
                // must be added to account for the negative overflow.
                Msb--;
                Lsb = ((Lsb + 040000 + POS_ONE) & 077777);
                // Restore the signs, if necessary.
                if (Complement) {
                    Msb = (077777 & ~Msb);
                    Lsb = (077777 & ~Lsb);
                }
            }
        }
        // We now have an Msb and Lsb of the same sign; therefore,
        // we can simply juxtapose them, discarding the sign bit from the 
        // Lsb.  (And recall that the 0-position is still the parity.)
        Value = (03777740000 & (Msb << 14)) | (037777 & Lsb);
        // Also, sign-extend for further arithmetic.
        if (02000000000 & Value)
            Value |= 04000000000;
        return (Value);
    }

    int16_t AbsSP (int16_t Value)
    {
        if (040000 & Value)
            return (077777 & ~Value);
        return (Value);
    }

    void SimulateDV(uint16_t a, uint16_t l, uint16_t divisor);

    int agc2cpu(int Input)
    {
        if (0 != (S1_MASK & Input))
            return (-(MANTISSA_MASK & ~Input));
        else
            return (MANTISSA_MASK & Input);
    }

    int cpu2agc (int Input)
    {
        if (Input < 0)
            return (077777 & ~(-Input));
        else
            return (077777 & Input);
    }

    int agc2cpu2 (int Input)
    {
        if (0 != (02000000000 & Input))
            return (-(01777777777 & ~Input));
        else
            return (01777777777 & Input);
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
    void showInterrupt(void);
    void addInterrupt(int i);
    uint16_t handleInterrupt(void);
    void incTime(void);
    void incTIME1(void);
    void divTest(uint16_t a, uint16_t l, uint16_t q) {
        mem.setA(SignExtend(a));
        mem.setL(SignExtend(l));
        mem.setQ(SignExtend(q));
        k10 = REG_Q;
        qc = 0;
        printf("A: %05o L: %05o D: %05o ", mem.getA(), mem.getL(), mem.getQ());
        op1ex();
        printf("==> A: %05o L: %05o\n", mem.getA(), mem.getL());
    }
    int BurstOutput (int DriveBitMask, int CounterRegister, int Channel);
    void UpdateIMU(void);
    void keyPress(Key_e key);
};


#endif//__CPU_H__