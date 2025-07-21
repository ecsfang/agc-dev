#include <stdio.h>
#include "cpu.h"

extern void stopAgc(void);

#if 1
void CCpu::readCore(char *core)
{
    FILE *fp = fopen(core,"r");
    char buf[1024];
    int bank = 0;
    __uint16_t  addr;
    while(fgets(buf,1024,fp)) {
        if( strncmp("BANK=", buf, 5) == 0 ) {
            sscanf(buf, "BANK=%o", &bank);
//            addr = 010000 + bank * FIXED_BLK_SIZE;
            addr = 02000;
            fprintf(logFile, "BANK=%o [%06o]\n", bank, addr);
            if( bank >= 040 ) {
                bank -= 010;
                mem.setFEB(1);
            } else
                mem.setFEB(0);
            mem.setFB(bank << FB_SHIFT);
        }
        if( buf[0] >= '0' && buf[0] < '8') {
            char *p = buf;
            unsigned int word;
            p = strtok(buf, ", ");
            while( p && sscanf(p, "%o", &word) == 1 ) {
//                fprintf(logFile, "%02o,%04o [%06o] : %05o\n", bank, addr-(010000 + bank * FIXED_BLK_SIZE), addr, word);
                fprintf(logFile, "%02o,%04o [%06o] : %05o\n", bank, addr, mem.addr2mem(addr), word);
                p = strtok(NULL, ", "); //+= 6;
                mem.writePys(mem.addr2mem(addr), word);
                addr++;
            }
//            printf("\n");
        }
    }
//#define SELF_TEST
#ifdef SELF_TEST
    mem.setFB(020 * FIXED_BLK_SIZE);
    mem.setZ(02070);
#else
    mem.setEB(6<<EB_SHIFT);
    mem.write12(01510, 0777); // FAILSW # IF POSITIVE NO RCSMONIT, OTHERWISE 0
    mem.setEB(0);
    mem.setFB(0); //020 * FIXED_BLK_SIZE);
    mem.setFEB(0);
    mem.setZ(BOOT); //02070);
#endif
    fprintf(logFile, "Start: [%06o](%06o) : %05o\n", mem.getZ(), mem.addr2mem(mem.getZ()), mem.getOP());
    fclose(fp);
}
#else
/*
void CCpu::readCore(char *core)
{
    FILE *fp = fopen(core,"r");
    char buf[1024];
    int bank = 0;
    int  addr, a2, word, b, b2;
    while(fgets(buf,1024,fp)) {
        if( strncmp("BANK=", buf, 5) == 0 ) {
            sscanf(buf, "BANK=%o", &bank);
            addr = 010000 + bank * FIXED_BLK_SIZE;
            printf("BANK=%o [%06o]\n", bank, addr);
        }
        if( buf[0] >= '0' && buf[0] < '8') {
            if( sscanf(buf, "%o %o", &addr, &word) == 2 ) {
                mem.write12(addr, word & 077777);
            }
        }
    }
    mem.setFB(0); //020 * FIXED_BLK_SIZE);
    mem.setZ(02000);
    printf("Start: [%06o](%06o) : %05o\n", mem.getZ(), mem.addr2mem(mem.getZ()), mem.getOP());
    fclose(fp);
}
*/

void CCpu::readCore(char *core)
{
    FILE *fp = fopen(core,"rb");
    char buf[1024];
    int bank = 0;
    int  addr, a2, word, word2, b, b2;
    uint16_t w;
    word2 = 0;
    addr = 04000;
    b = 1000;
    while(b-- && fread(&word, 2, 1, fp) == 1) {
        word = (word&0xFF)<<8 | (word>>8) & 0xFF;
        w = word >> 1;
        fprintf(logFile, "[%05o %05o] %05o %05o\n", word >> 1, word2, word, word2);
        word2 = word;
        mem.write(addr, w & 077777);
        addr++;
    }
//    mem.setFB(0); //020 * FIXED_BLK_SIZE);
//    mem.setZ(04000);
    mem.setFB(020 * FIXED_BLK_SIZE);
    mem.setZ(02070);
    fprintf(logFile, "Start: [%06o](%06o) : %05o\n", mem.getZ(), mem.addr2mem(mem.getZ()), mem.getOP());
    fflush(logFile);
    fclose(fp);
}
#endif


//******************************************************************


int CCpu::sst(void)
{
    int ret = -1;
    char logBuf[1024];
    int ln;
    uint16_t    omem = mem.readPys(mwBreak);
    __uint16_t op = getOP();
    bClrExtra = true;

    static uint32_t clk;

    ln = sprintf(logBuf, "[%c%5d]%s", bIntRunning ? '*':' ', clk++, disasm(0,false));

    UpdateIMU();

    bOF = ValueOverflowed (mem.getA() & MASK_16_BITS ) != POS_ZERO;

    // PC is incremented before execution starts!
    nextPC = mem.step();

    // Assume every instruction takes 2 MCT.
    mct = 2;
    if( bExtracode ) {
        switch( op & OPCODE_MASK ) {
            case 000000: ret = op0ex(); break;
            case 010000: ret = op1ex(); break;
            case 020000: ret = op2ex(); break;
            case 030000: ret = op3ex(); break;
            case 040000: ret = op4ex(); break;
            case 050000: ret = op5ex(); break;
            case 060000: ret = op6ex(); break;
            case 070000: ret = op7ex(); break;
//            default:
//                pDis += sprintf(disBuf+pDis, "Unknown extra code %05o!", op);
        }
        if( bClrExtra )
            bExtracode = false;
    } else {
        switch( op & OPCODE_MASK ) {
            case 000000: ret = op0(); break;
            case 010000: ret = op1(); break;
            case 020000: ret = op2(); break;
            case 030000: ret = op3(); break;
            case 040000: ret = op4(); break;
            case 050000: ret = op5(); break;
            case 060000: ret = op6(); break;
            case 070000: ret = op7(); break;
//            default:
//                pDis += sprintf(disBuf+pDis, "Unknown opcode %05o!", op);
        }
    }
    clockCnt += mct;
    dTime += mct;

    __uint16_t r = mem.getA();
    int nl = 55-ln;
    if (nl < 1) nl = 1;
    ln += sprintf(logBuf+ln,"%*.*s[%d:%05o] ", nl,nl,"A",r & 0100000 ? 1 : 0, r & MASK_15_BITS);
    r = mem.getL();
    ln += sprintf(logBuf+ln,"L[%d:%05o] ", r & 0100000 ? 1 : 0, r & MASK_15_BITS);
    r = mem.getQ();
    ln += sprintf(logBuf+ln,"Q[%d:%05o] ", r & 0100000 ? 1 : 0, r & MASK_15_BITS);
    r = mem.getBB();
    ln += sprintf(logBuf+ln,"BB[%05o] ", r & MASK_15_BITS);
    ln += sprintf(logBuf+ln,"IDX[%05o] ", idx);
#if 0
    ln += sprintf(logBuf+ln,"IZ[%05o] ", mem.read12(REG_ZRUPT));
    ln += sprintf(logBuf+ln,"IB[%05o] ", mem.read12(REG_BRUPT));
    ln += sprintf(logBuf+ln,"IBB[%05o] ", mem.read12(REG_BBRUPT));
#endif
    fprintf(logFile,"%s\n", logBuf);

    if( dTime > 43 ) { // 0.5ms/11.7us
        // 0.5 ms has ellapsed
        incTime();
        dTime = 0;
    }

    mem.setZ(nextPC);
    if( bInterrupt && !bIntRunning && !bExtracode && !idx && !OF() && gInterrupt != 0 ) {
        nextPC = handleInterrupt();
    }
    if( nextPC )
        mem.setZ(nextPC);

    if( mwBreak && omem != mem.readPys(mwBreak) )
        stopAgc();
        
    return ret;
}

#define BOOT        04000   // Power-up or GOJ signal.

uint16_t CCpu::handleInterrupt(void)
{
    uint16_t pc = getPC();
    fprintf(logFile,"HandleInterrupt [%03X]  @ %05o [%05o]\n", gInterrupt, pc, mem.getOP());
    mem.write12(REG_ZRUPT, pc);
    mem.write12(REG_BRUPT, mem.getOP());
    for(int i=0; i<NR_INTS;i++) {
        // Is interrupt flag set ?
        if( gInterrupt & (1<<i) ) {
            uint16_t iPc = BOOT + i*4;
            nextPC = iPc;
            fprintf(logFile,"Stop and continue @ %05o\n", nextPC);
            bIntRunning = true;
            intRunning = iPc;
            //stopAgc();
            // Clear the interrupt flag ...
            gInterrupt &= ~(1<<i);
            return nextPC;
        }
    }
    return 0;
}

#define IPRT(i,s)                           \
     if( gInterrupt & i ) {                 \
        if( n++ ) fprintf(logFile, " | ");  \
        fprintf(logFile, "%s", s);          \
    }

void CCpu::showInterrupt(void)
{
    if( !gInterrupt )
        return;
    int n = 0;
    fprintf(logFile, "INTERRUPT (%o): ", gInterrupt);
    IPRT( iBOOT,      "BOOT")
    IPRT( iT6RUPT,    "T6RUPT")
    IPRT( iT5RUPT,    "T5RUPT")
    IPRT( iT3RUPT,    "T3RUPT")
    IPRT( iT4RUPT,    "T4RUPT")
    IPRT( iKEYRUPT1,  "KEYRUPT1")
    IPRT( iKEYRUPT2,  "KEYRUPT2")
    IPRT( iUPRUPT,    "UPRUPT")
    IPRT( iDOWNRUPT,  "DOWNRUPT")
    IPRT( iRADARRUPT, "RADARRUPT")
    IPRT( iRUPT10,    "RUPT10")
    fprintf(logFile, "\n");
}

void CCpu::addInterrupt(int i)
{
    gInterrupt |= i;
    showInterrupt();
}

void CCpu::incTime(void) {
    // Count all 0.5ms
    static uint32_t ms05 = 0;
    static uint8_t downrupt = 0;

    switch( ms05 % 20 ) {
    case 0: // Every 10 ms
        incTIME1(); // Increment every 10ms
        if( mem.incTimer(REG_TIME3) )
            addInterrupt(iT3RUPT);
        break;
    case 10: // Every 10m (5ms out of phase)
        if( mem.incTimer(REG_TIME5) )
            addInterrupt(iT5RUPT);
        break;
    case 15: // Every 10ms (7.5ms out of phase)
        if( mem.incTimer(REG_TIME4) )
            addInterrupt(iT4RUPT);
        break;
    }
    if( downrupt > 42 ) {
        fprintf(logFile,"ADD DOWNRUPT!\n");
        addInterrupt(iDOWNRUPT);
        downrupt = 0;
    }
    // Every 0.5ms (~1/1600s)
    if( bTime6Enabled ) {
        if( mem.incTimer(REG_TIME6) ) {
            addInterrupt(iT6RUPT);
            bTime6Enabled = false;
        }
    }
    ms05++;
    downrupt++;
}

void CCpu::incTIME1(void)
{
    if( mem.incTimer(REG_TIME1) )
        mem.incTimer(REG_TIME2);
}


//----------------------------------------------------------------------------
// This function implements a model of what happens in the actual AGC hardware
// during a divide -- but made a bit more readable / software-centric than the 
// actual register transfer level stuff. It should nevertheless give accurate
// results in all cases, including those that result in "total nonsense".
// If A, L, or Z are the divisor, it assumes that the unexpected transformations
// have already been applied to the "divisor" argument.
void CCpu::SimulateDV(uint16_t a, uint16_t l, uint16_t divisor)
{
    uint16_t dividend_sign = 0;
    uint16_t divisor_sign = 0;
    uint16_t remainder;
    uint16_t remainder_sign = 0;
    uint16_t quotient_sign = 0;
    uint16_t quotient = 0;
    uint16_t sum = 0;
    int i;

    fprintf(logFile,"DV: a|%05o l|%05o / %05o\n", a, l, divisor);

    // Assume A contains the sign of the dividend
    dividend_sign = a & 0100000;

    // Negate A if it was positive
    if (!dividend_sign)
      a = ~a;
    // If A is now -0, take the dividend sign from L
    if (a == 0177777)
      dividend_sign = l & 0100000;
    // Negate L if the dividend is negative.
    if (dividend_sign)
      l = ~l;

    // Add 40000 to L
    l = AddSP16(l, 040000);
    // If this did not cause positive overflow, add one to A
    if (ValueOverflowed(l) != POS_ONE)
      a = AddSP16(a, 1);
    // Initialize the remainder with the current value of A
    remainder = a;

    // Record the sign of the divisor, and then take its absolute value
    divisor_sign = divisor & 0100000;
    if (divisor_sign)
      divisor = ~divisor;
    // Initialize the quotient via a WYD on L (L's sign is placed in bits
    // 16 and 1, and L bits 14-1 are placed in bits 15-2).
    quotient_sign = l & 0100000;
    quotient = quotient_sign | ((l & 037777) << 1) | (quotient_sign >> 15);

    for (i = 0; i < 14; i++)
    {
        // Shift up the quotient
        quotient <<= 1;
        // Perform a WYD on the remainder
        remainder_sign = remainder & 0100000;
        remainder = remainder_sign | ((remainder & 037777) << 1);
        // The sign is only placed in bit 1 if the quotient's new bit 16 is 1
        if ((quotient & 0100000) == 0)
          remainder |= (remainder_sign >> 15);
        // Add the divisor to the remainder
        sum = AddSP16(remainder, divisor);
        if (sum & 0100000)
          {
            // If the resulting sum has its bit 16 set, OR a 1 onto the
            // quotient and take the sum as the new remainder
            quotient |= 1;
            remainder = sum;
          }
    }
    // Restore the proper quotient sign
    a = quotient_sign | (quotient & 077777);

    // The final value for A is negated if the dividend sign and the
    // divisor sign did not match
    mem.setA((dividend_sign != divisor_sign) ? ~a : a);
    // The final value for L is negated if the dividend was negative
    mem.setL((dividend_sign) ? remainder : ~remainder);
    fprintf(logFile,"DV ==> %05o %05o\n", mem.getA(), mem.getL());

/*
    double f = btof(a, l);
    double div = btof(divisor, 0);
    fprintf(logFile, "\n  %05o %05o -> %lf", a, l, f);
    fprintf(logFile, "\n        %05o -> %lf", divisor, div);
//    double quo = f / div;
//    uint16_t r1, r2;
//    r1 = ftob(quo, &r2);
    fprintf(logFile, "\n---------------------\n/ %05o %05o", ra, rl); //, quo);
    fprintf(logFile, "\n");
    ***/
}
