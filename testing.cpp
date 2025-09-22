#include <stdio.h>
#include "cpu.h"

// The instance of the AGC CPU
extern CCpu    cpu;

void prtBin(__uint16_t x)
{
    for(int b=14; b>=0; b--) {
        fprintf(logFile, "%c", x & (1<<b) ? '1' : '0');
    }
}
void prt1st(__uint16_t x)
{
    bool bNeg = (x&0x4000) ? true : false;
    __uint16_t d = x & 0x7FFF;

    if( bNeg )
        d = ((~x) & 0x7FFF);
    fprintf(logFile, "%05o (%d ", x & 0x7FFF, x & 0x8000 ? 1 : 0); prtBin(x);
    fprintf(logFile, ") [%c", bNeg ? '-' : '+');
    fprintf(logFile, "%5d]", d);
}

void testAdd(__uint16_t x1, __uint16_t x2)
{
    CCpu    tst;
    bool of;
    fprintf(logFile, "\n  "); prt1st(x1);
    fprintf(logFile, ")\n  "); prt1st(x2);
    fprintf(logFile, ")\n-------------------------------\n+ "); prt1st(tst.add1st(x1,x2));
    if( of )
        fprintf(logFile, "Overflow!");
    fprintf(logFile, "\n");
}

double btof(uint16_t x1, uint16_t x2)
{
    double r = 0.0;
    bool bNeg = false;
    double n = 0.5;
    if( IS_NEG(x1) ) {
        bNeg = true;
        x1 = (~x1) & 0x7FFF;
    }
    for(int b=13; b>=0; b--) {
        if( (x1>>b)&1 )
            r += n;
        n /= 2;
    }
    if( bNeg )
        r = -r;

    bNeg = false;
    if( IS_NEG(x2) ) {
        bNeg = true;
        x2 = (~x2) & 0x7FFF;
    }
    for(int b=13; b>=0; b--) {
        if( (x2>>b)&1 )
            r += bNeg ? -n : n;
        n /= 2;
    }

    return r;
}

uint16_t ftob(double f, uint16_t *remain)
{
    uint16_t r1 = 0;
    uint16_t r2 = 0;
    double of = f;
    double rf;
    bool bNeg = false;
    if( f < 0){
        bNeg = true;
        f = -f;
    }
    for(int n=0; n<14; n++) {
        r1 <<= 1;
        f = f*2;
        if( f>=1.0 ) {
            r1 |= 1;
            f -= 1.0;
        }
    }
    for(int n=0; n<14; n++) {
        r2 <<= 1;
        f = f*2;
        if( f>=1.0 ) {
            r2 |= 1;
            f -= 1.0;
        }
    }
    if( bNeg ) {
        r1 = (~r1) & 0x7FFF;
        r2 = (~r2) & 0x7FFF;
    }
    *remain = r2;
    return r1;
 }

void testDiv(__uint16_t x1, __uint16_t x2, uint16_t k)
{
    CCpu    tst;
    bool of;
    double f = btof(x1, x2);
    double div = btof(k, 0);
    fprintf(logFile, "\n  %05o %05o -> %lf", x1, x2, f);
    fprintf(logFile, "\n        %05o -> %lf", k, div);
    double quo = f / div;
    uint16_t r1, r2;
    r1 = ftob(quo, &r2);
//    x2 = div - (x1*k);
    fprintf(logFile, "\n---------------------\n/ %05o %05o -> %lf", r1, r2, quo);
    fprintf(logFile, "\n");
}

void memTest(CMemory *mem, uint16_t a0, uint16_t a1, uint8_t eb, uint8_t fb, uint8_t feb)
{
    uint16_t    addr0, addr1;
    mem->setEB(eb<<EB_SHIFT);
    mem->setFB(fb<<FB_SHIFT);
    mem->setFEB(feb);
    addr0 = mem->addr2mem(a0);
    addr1 = mem->addr2mem(a1);

    printf("%05o-%05o\t%02o\t%02o\t%o\t%04o-%04o\n", addr0, addr1, eb, fb, feb, a0, a1);
}
void doMemTest(CMemory *mem)
{
    printf("Memory test!\n");
        printf("Erasable fixed\n");
        printf("Pseudo address\tEBANK\tFBANK\tFEB\tS-Reg value\n");
        for(uint8_t e=0; e<8; e++) {
            memTest(mem, 00000, 01377, e, 0, 0);
        }
        printf("\nErasable switched\n");
        printf("Pseudo address\tEBANK\tFBANK\tFEB\tS-Reg value\n");
        for(uint8_t e=0; e<8; e++) {
            memTest(mem, 01400, 01777, e, 0, 0);
        }
        printf("\nFixed un-switched\n");
        printf("Pseudo address\tEBANK\tFBANK\tFEB\tS-Reg value\n");
        for(uint8_t e=0; e<8; e++) {
            memTest(mem, 04000, 07777, e, 0, 0);
        }
        printf("\nFixed switched-switched (superbank 0)\n");
        printf("Pseudo address\tEBANK\tFBANK\tFEB\tS-Reg value\n");
        for(uint8_t f=0; f<040; f++) {
            memTest(mem, 02000, 03777, 0, f, 0);
        }
        printf("\nFixed switched-switched (superbank 1)\n");
        printf("Pseudo address\tEBANK\tFBANK\tFEB\tS-Reg value\n");
        for(uint8_t f=0; f<034; f++) {
            memTest(mem, 02000, 03777, 0, f, 1);
        }
}

typedef struct {
    uint16_t a;
    uint16_t l;
    uint16_t d;
    uint16_t ra;
    uint16_t rl;
} Div_t;

Div_t dTest[] = {
    { 017777, 040000, 020000, 037774, 000001 },
    { 017777, 040000, 057777, 040003, 000001 },
    { 060000, 037777, 020000, 040003, 077776 },
    { 060000, 037777, 057777, 037774, 077776 },
    { 017777, 037777, 020000, 037777, 017777 },
    { 037776, 000000, 037776, 037777, 037776 },
    { 000000, 077777, 000000, 040000, 077777 },
    { 000000, 077777, 077777, 037777, 077777 },
    { 077777, 000000, 000000, 037777, 000000 },
    { 077777, 000000, 077777, 040000, 000000 },
    { 077777, 020000, 037776, 040000, 000000 }
};


void doDivTest(CCpu *cpu)
{
    printf("Division test!\n");
    Div_t *dt = dTest;
    for( int n=0; n < sizeof(dTest)/sizeof(Div_t); n++, dt++)
        cpu->divTest(dt->a,dt->l,dt->d);
}

TestPattern_t opTest[] = {
  /* 00 */    { 054000, 0, 0012345,           0, 0, 1,     REA,     NAN, NAN, 0 },
  /* 01 */    { 054001, 0, 0012345,           0, 0, 1,     REA,     REA, NAN, 0 },
  /* 02 */    { 054001, 0, 0012345 | POS_OF,  0, 0, 2, POS_ONE,  012345, NAN, 0 },
  /* 03 */    { 054001, 0, 0032107 | NEG_OF,  0, 0, 2, NEG_ONE| NEG_OF, 032107|S1_MASK, NAN, OF_MASK },
  /* 04 */    { 054002, 0, 0012345,           0, 0, 1,     REA,     NAN, REA, 0 },
              // Test CCS
  /* 05 */    { 010000, 0, POS_ONE,           0, 0, 1, POS_ZERO,    NAN, NAN, 0 },
  /* 06 */    { 010000, 0, POS_ZERO|POS_OF,   0, 0, 1, MAX_POS,     NAN, NAN, 0 },
  /* 07 */    { 010000, 0, POS_ONE|POS_OF,    0, 0, 1, POS_ZERO|POS_OF,     NAN, NAN, POS_OF },
  /* 08 */    { 010000, 0, POS_ZERO|POS_OF,   0, 0, 1, MAX_POS,     NAN, NAN, 0 },
  /* 09 */    { 010000, 0, POS_ZERO,          0, 0, 2,     REA,     NAN, NAN, 0 },
  /* 10 */    { 010000, 0, POS_ZERO|NEG_OF,   0, 0, 3, NEG_ONE,     NAN, NAN, POS_OF },
  /* 11 */    { 010000, 0, NEG_ZERO|NEG_OF,   0, 0, 4, POS_ZERO,     NAN, NAN, 0 },
  };
  
  bool doTest(int x)
  {
      uint16_t    thePC = 004001;
      uint16_t    newPC = 0;
      uint16_t    expPC = 0;
      char        errBuf[2048];
      int         nErr = 0;
      TestPattern_t *pTst = &opTest[x];
  
      printf("Execute test %d\n", x);
  
      pTst->pc = thePC;
      // Setup all registers
      if( pTst->resA == REA )
          pTst->resA = pTst->regA;
      if( pTst->resL == REA )
          pTst->resL = pTst->regA;
      if( pTst->resQ == REA )
          pTst->resQ = pTst->regA;
  
      // Execute the test
      newPC = cpu.testOp(x, pTst);
  
      // Update next PC
      expPC = thePC + pTst->offs;
  
      // Check that the PC is incremented correctly
      if( !(newPC == expPC) ) {
          if( !nErr ) nErr = sprintf(errBuf, "Error in test %d: ", x);
          nErr += sprintf(errBuf+nErr, " * Next PC == %05o - expected %05o!\n", newPC, expPC);
      }
      // Check result in register A->Q
      if( pTst->resA != NAN && !(pTst->resA == cpu.getA() ) ) {
          if( !nErr ) nErr = sprintf(errBuf, "Error in test %d: ", x);
          nErr += sprintf(errBuf+nErr, " * REG A == %05o - expected %05o!\n", cpu.getA(), pTst->resA);
      }
      if( pTst->resL != NAN && !(pTst->resL == cpu.getL() ) ) {
          if( !nErr ) nErr = sprintf(errBuf, "Error in test %d: ", x);
          nErr += sprintf(errBuf+nErr, " * REG L == %05o - expected %05o!\n", cpu.getL(), pTst->resL);
      }
      if( pTst->resQ != NAN &&  !(pTst->resQ == cpu.getQ() ) ) {
          if( !nErr ) nErr = sprintf(errBuf, "Error in test %d: ", x);
          nErr += sprintf(errBuf+nErr, " * REG Q == %05o - expected %05o!\n", cpu.getQ(), pTst->resQ);
      }
      // Check expected overflow
      if( !(pTst->resOf == IS_OF(cpu.getA())) ) {
          if( !nErr ) nErr = sprintf(errBuf, "Error in test %d: ", x);
          nErr += sprintf(errBuf+nErr, " * Got overflow %05o - expected %05o!\n", IS_OF(cpu.getA()), pTst->resOf);
      }
      if( nErr )
          printf("%s", errBuf);
      return true;
  }
  
  void testCpu(void)
  {
      printf("\nTest memory...\n");
      doMemTest(cpu.getMem());
      printf("\nTest division...\n");
      doDivTest(&cpu);
      printf("\nTest instructions...\n");
      for(int t=0; t<(sizeof(opTest)/sizeof(TestPattern_t)); t++) {
          doTest(t);
      }
  }
  
  