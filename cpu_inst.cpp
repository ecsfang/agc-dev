#include <stdio.h>
#include "cpu.h"
#include "cpu.h"

extern void stopAgc(void);

typedef uint16_t logsz_t;

static char *pLog[0x10000];
static logsz_t pLogCnt = 0;

#if 1
void CCpu::readCore(char *core)
{
    FILE *fp = fopen(core,"r");
    char buf[1024];
    int bank = -1;
    __uint16_t  addr;
    // Make fixed bank writable
    mem.protection(false);
    while(fgets(buf,1024,fp)) {
        if( strncmp("BANK=", buf, 5) == 0 ) {
            // Start of a new bank ...
            sscanf(buf, "BANK=%o", &bank);
            addr = 02000; // Start address of every fixed bank
            fprintf(logFile, "BANK=%o [%06o]\n", bank, addr);
            if( bank >= 040 ) {
                // Superbank 40-43 are 30-33 with FEB = 1
                bank -= 010;
                mem.setFEB(1);
            } else {
                // Bank 00-37 with FEB = 0
                mem.setFEB(0);
            }
            mem.setFB(bank << FB_SHIFT);
        }
        if( buf[0] >= '0' && buf[0] < '8') {
            // Data for the current bank ...
            char *p = buf;
            unsigned int word;
            p = strtok(buf, ", ");
            while( p && sscanf(p, "%o", &word) == 1 ) {
                if( bExtraLogging )
                    fprintf(logFile, "%02o,%04o [%06o] : %05o\n", bank, addr, mem.addr2mem(addr), word);
                mem.write12(addr++, word);
                p = strtok(NULL, ", ");
            }
        }
    }
    // Make fixed bank only readable
    mem.protection(true);
//#define SELF_TEST
#ifdef SELF_TEST
    mem.setPC(043,03363);
#else
    // FAILSW # IF POSITIVE NO RCSMONIT, OTHERWISE 0
    mem.write12(01510, 0777);
    // Reset PC to boot entry ...
    mem.setEB(0);
    mem.setFB(0);
    mem.setFEB(0);
    mem.setZ(BOOT);
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

char *CCpu::getTime(void)
{
    static char tBuf[32];
    uint64_t us = (uint64_t)(clockCnt*11.7);
    uint16_t usec = us % 1000;
    uint16_t msec = (us/1000) % 1000;
    uint32_t s = us/(1000*1000);
    uint8_t sec = s % 60;
    uint8_t min = (s/60) % 60;
    uint8_t h = (s/(60*60));
    sprintf(tBuf, "%d:%02d:%02d.%03d%03d", h, min, sec, msec, usec);
    return tBuf;
}

void dumpLog()
{
    logsz_t pb = pLogCnt;

    fprintf(logFile,"\n=======================================");
    fprintf(logFile,"\n= Saved log ===========================");
    fprintf(logFile,"\n=======================================\n");

    do {
        if( pLog[pb] )
            fprintf(logFile,"%s\n", pLog[pb]);
        pb++;
    } while(pb != pLogCnt);
    fprintf(logFile,"\n= End of saved log ====================\n");
    fflush(logFile);
}

extern volatile uint32_t tickCounter; // Variable to increment

const OPX_t opX[] = {
    &CCpu::op0,    &CCpu::op1,    &CCpu::op2,    &CCpu::op3,    &CCpu::op4,    &CCpu::op5,    &CCpu::op6,    &CCpu::op7,
    &CCpu::op0ex,  &CCpu::op1ex,  &CCpu::op2ex,  &CCpu::op3ex,  &CCpu::op4ex,  &CCpu::op5ex,  &CCpu::op6ex,  &CCpu::op7ex
};

int CCpu::logline(char *buf, int ln)
{
    __uint16_t r = mem.getA();
    int p = 0;
    int nl = LOG_TAB_1-ln;
    if (nl < 1) nl = 1;
    // Dump register on the log line
    p += sprintf(buf+p,"%*.*s[%d:%05o] ", nl,nl,"A",IS_NEG16(r)?1:0, r & MASK_15_BITS);
    r = mem.getL();
    p += sprintf(buf+p,"L[%05o] ", r & MASK_15_BITS);
    r = mem.getQ();
    p += sprintf(buf+p,"Q[%d:%05o] ", IS_NEG16(r)?1:0, r & MASK_15_BITS);
    r = mem.getBB();
    p += sprintf(buf+p,"BB[%05o] ", r & MASK_15_BITS);
    p += sprintf(buf+p,"IDX[%05o] ", index());
#if 0
    p += sprintf(buf+p,"T3[%05o] ", mem.read12(REG_TIME3));
    p += sprintf(buf+p,"T4[%05o] ", mem.read12(REG_TIME4));
    p += sprintf(buf+p,"T5[%05o] ", mem.read12(REG_TIME5));
#endif
#if 0
    p += sprintf(buf+p,"IZ[%05o] ", mem.read12(REG_ZRUPT));
    p += sprintf(buf+p,"IB[%05o] ", mem.read12(REG_BRUPT));
    p += sprintf(buf+p,"IBB[%05o] ", mem.read12(REG_BBRUPT));
#endif
    return p;
}

uint16_t CCpu::testOp(int x, TestPattern_t *pTst) {
    return testOp(x, pTst->op, pTst->pc, pTst->regA);
}

uint16_t CCpu::testOp(int x, uint16_t op, uint16_t pc, uint16_t a)
{
    static char logBuf[1024];
    static int ln;
    __uint16_t opi = (op & OPCODE_MASK) >> 12;
    nextPC = pc;
    mem.setZ(nextPC);
    mem.setA(a);
    setOF(a);
    mem.setFB(2 << FB_SHIFT);
    mem.setFEB(0);

    mem.protection(false);

    mem.writePys(pc+FB_MEM_START, op);

    ln = sprintf(logBuf,"\n[Test %03d]", x);
    ln += logline(logBuf+ln, 32+ln-1);
    fprintf(logFile,"%s", logBuf);
    fprintf(logFile,"PC[%05o] ", nextPC);
    int16_t of = ValueOverflowed(getA());
    if( of )
        fprintf(logFile,"OF:%c", of == POS_ONE?'+':' ');
    else
        fprintf(logFile,"OF:");
    fprintf(logFile,"\n");

    // Add disassembled instruction to log
    ln = sprintf(logBuf, "%s", disasm(0,false));

    int ret = (this->*opX[opi])();
    ln += logline(logBuf+ln, ln+32);

    nextPC++;

    fprintf(logFile,"%s", logBuf);
    fprintf(logFile,"PC[%05o] ", nextPC);
    of = ValueOverflowed(getA());
    if( of )
        fprintf(logFile,"OF:%c\n", of == POS_ONE?'+':' ');
    else
        fprintf(logFile,"OF:\n");
    fflush(logFile);

    return nextPC;
}

int CCpu::sst(void)
{
    static char logBuf[1024];
    static int ln;
    static uint32_t tTick = 0;

    int ret = -1;
    uint16_t    omem = mem.readPys(mwBreak);
    // Get the instruction group code
    __uint16_t opi = (getOP() & OPCODE_MASK) >> 12;
    uint32_t currTick = tickCounter;

    // Assume that we will clear the extra code flag
    bClrExtra = true;

    if( bRunning && tTick != currTick) {
        // 0.5 ms has ellapsed
        incTime();
        // Reset timer
        tTick++;
    }

    UpdateIMU();

    // Check if overflow in accumulator A
    bOF = IsValueOverflowed( mem.getA() );

    ln = sprintf(logBuf, "[%c%s]%c", bIntRunning ? '*':' ', getTime(), bOF ? 'O':' ');
    // Add disassembled instruction to log
    ln += sprintf(logBuf+ln, "%s", disasm(0,false));

    clrIndex(); // Clear the INDEX value

    // PC is incremented before execution starts!
    nextPC = mem.step();

    // Assume every instruction takes 2 MCT.
    mct = 2;

    // Execute the instruction
    if( bExtracode ) {
        ret = (this->*opX[opi|010])();
        if( bClrExtra )
            bExtracode = false;
    } else {
        ret = (this->*opX[opi])();
    }

    // Normally clear, but some EX functions retain the flag
    clockCnt += mct;
    if( !bRunning ) {
        dTime += mct*12;
    }
    dT1600 += mct;
    dT3200 += mct;

    ln += logline(logBuf+ln, ln);

    if( trace() || bFileLogging )
        fprintf(logFile,"%s\n", logBuf);

    // Save some log for crashes ...
    if( pLog[pLogCnt] )
        delete[] pLog[pLogCnt];
    pLog[pLogCnt] = strdup(logBuf);
    pLogCnt++;

    if( dT3200 > T1_3200 ) {
        // 1/3200 seconds has ellapsed
        dT3200 = 0;
    }
    if( dT1600 > T1_1600 ) {
        // 1/1600 seconds has ellapsed
        dT1600 = 0;
    }

    // While running, the timers are updated in the background
    // Otherwise update manually ...
    if( !bRunning ) {
        if( dTime > T500US ) {
            // 0.5 ms has ellapsed
            incTime();
            dTime = 0;
        }
    }

    mem.setZ(nextPC);
    // Check if we should hanlde interrupt ...
    //   bInterrupt  - true if interrupts are enabled
    //   gInterrupt  - any pending interrupts?
    //   bIntRunning - true if an interrupt is already running
    //   bExtracode  - if true then disable interrupt
    //   index       - no interrupts if index is set
    //   OF()        - no interrupts if overflow
    if( bInterrupt && gInterrupt && !bIntRunning && !bExtracode && !index() && !OF() ) {
        nextPC = handleInterrupt();
    }
    if( nextPC )
        mem.setZ(nextPC);

    if( mwBreak && omem != mem.readPys(mwBreak) )
        stopAgc();
        
    return ret;
}

#define BOOT        04000   // Power-up or GOJ signal.

// Return number of microseconds since 'start'
uint16_t CCpu::elapsedUS(void)
{
    // End time
    gettimeofday(&end, NULL);

    // Calculate elapsed time in microseconds
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    long elapsed = seconds * 1000000 + microseconds;
    return elapsed < 0 ? 0 : (uint16_t)elapsed;
}

uint16_t CCpu::handleInterrupt(void)
{
    uint16_t pc = getPC();
    if( bFileLogging )
        fprintf(logFile,"HandleInterrupt [%03X]  @ %05o [%05o]\n", gInterrupt, pc, mem.getOP());
    mem.write12(REG_ZRUPT, pc);
    mem.write12(REG_BRUPT, mem.getOP());
    for(int i=0; i<NR_INTS;i++) {
        // Is interrupt flag set ?
        if( gInterrupt & IRUPT(i) ) {
            uint16_t iPc = BOOT + i*4;
            nextPC = iPc;
            if( bFileLogging )
                fprintf(logFile,"Interrupt and continue @ %05o\n", nextPC);
            // Init the interrupt ...
            setInterrupt(iPc, i);
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
    if( bFileLogging ) {
        int n = 0;
        fprintf(logFile, "[ %s] INTERRUPT (%o): ", getTime(), gInterrupt);
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
}

void CCpu::addInterrupt(int i)
{
    gInterrupt |= i;
    showInterrupt();
}

// If overflow - check for timer registers
void CCpu::chkInterrupt(uint16_t reg) {
    switch( reg ) {
    case REG_TIME1: mem.incTimer(REG_TIME2);    break;
    case REG_TIME5: addInterrupt(iT5RUPT);      break;
    case REG_TIME3: addInterrupt(iT3RUPT);      break;
    case REG_TIME4: addInterrupt(iT4RUPT);      break;
    }
}

void CCpu::incTime(void) {
    // Count all 0.5ms
    static int32_t ms05 = 0;
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
    case 19:
        // Reset counter (so we couunt 0 -> 19 -> 0 ...)
        ms05 = -1;
    }
    if( downrupt > 42 ) {
        if( bFileLogging )
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

    if( bFileLogging )
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
    a = quotient_sign | (quotient & MASK_15_BITS);

    // The final value for A is negated if the dividend sign and the
    // divisor sign did not match
    mem.setA((dividend_sign != divisor_sign) ? ~a : a);
    // The final value for L is negated if the dividend was negative
    mem.setL((dividend_sign) ? remainder : ~remainder);
    if( bFileLogging )
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
