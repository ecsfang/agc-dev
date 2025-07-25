#include <stdio.h>
#include <memory.h>
#include "cpu.h"
#include <map>

using namespace std;

#include <ncurses.h>

FILE    *logFile=NULL;
bool bFileLogging = false;

extern int dskyInit(void);

//#define DO_TEST
#ifdef DO_TEST
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

//----------------------------------------------------------------------------
// This function implements a model of what happens in the actual AGC hardware
// during a divide -- but made a bit more readable / software-centric than the 
// actual register transfer level stuff. It should nevertheless give accurate
// results in all cases, including those that result in "total nonsense".
// If A, L, or Z are the divisor, it assumes that the unexpected transformations
// have already been applied to the "divisor" argument.
static void
SimulateDV(uint16_t a, uint16_t l, uint16_t divisor)
{
    uint16_t dividend_sign = 0;
    uint16_t divisor_sign = 0;
    uint16_t remainder;
    uint16_t remainder_sign = 0;
    uint16_t quotient_sign = 0;
    uint16_t quotient = 0;
    uint16_t sum = 0;
    int i;

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
    uint16_t ra = (dividend_sign != divisor_sign) ? ~a : a;
    // The final value for L is negated if the dividend was negative
    uint16_t rl = (dividend_sign) ? remainder : ~remainder;

    double f = btof(a, l);
    double div = btof(divisor, 0);
    fprintf(logFile, "\n  %05o %05o -> %lf", a, l, f);
    fprintf(logFile, "\n        %05o -> %lf", divisor, div);
//    double quo = f / div;
//    uint16_t r1, r2;
//    r1 = ftob(quo, &r2);
    fprintf(logFile, "\n---------------------\n/ %05o %05o", ra, rl); //, quo);
    fprintf(logFile, "\n");
}

void test1st(void)
{
    __uint16_t x1, x2;
    bool of;
/*    testAdd(002000, 006000);
    testAdd(022000, 026000);
    testAdd(074000, 070000);
    testAdd(054000, 050000);
    testAdd(14908, 8265);
    testAdd(037777, 040000);
    testAdd(040000, 037777);
    testAdd(037777, 000001);
    testAdd(000001, 037777);
    testAdd(040000, 040000);
*/
    SimulateDV(017777, 040000, 020000);
    SimulateDV(017777, 040000, 057777);
    SimulateDV(060000, 037777, 020000);
    SimulateDV(060000, 037777, 057777);
    SimulateDV(017777, 037777, 020000);
    SimulateDV(037776, 000000, 037776);
    SimulateDV(000000, 077777, 000000);
    SimulateDV(000000, 077777, 077777);
    SimulateDV(077777, 000000, 000000);
    SimulateDV(077777, 077777, 077777);
}
#endif

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

map<uint16_t,char*> symTab;

void updateScreen(WINDOW *wnd, CCpu *cpu, bool bRun)
{
    cpu->updateDSKY(wnd, bRun);
    if( bRun )
        return;
    cpu->dispReg(wnd);
    if( !bRun ) {
        for(int n=0; n<5; n++)
            mvwprintw(wnd,18+n,0,"%s           ", cpu->disasm(n-1));
    }
    mvwprintw(wnd,24,0,"Clk: %5d\n", cpu->getClock());
    map<uint16_t, char*>::iterator it;
    it = symTab.find(cpu->getAbsPC());
    if( it != symTab.end() )
        mvwprintw(wnd,25,0,"%05o -> %s\n", it->first, it->second);
    refresh();			    /* Print it on to the real screen */
}

void readSymbols(char *sym)
{
    FILE *fs = fopen(sym,"r");
    int idx = 0x409;
    uint16_t addr;
    char buf[16];
    fseek(fs, idx, SEEK_SET);
    while( fgets(buf, 16, fs) ) {
        idx += 15;
        fseek(fs, idx, SEEK_SET);
        fread(&addr, 2, 1, fs);
        // printf("%04X ", addr);
        if( addr < 04000 ) {
        } else {
            symTab[addr] = strdup(buf);
#if 0
            if( addr >= 04000 && addr < 010000 )
                printf("%s:    %05o\n", buf, addr);
            else {
                uint8_t blk = (addr - 010000) / FIXED_BLK_SIZE;
                printf("%s: %02o %05o\n", buf, blk, (addr % FIXED_BLK_SIZE) + FIXED_BLK_SIZE);
            }
#endif
        }
        idx += 292-15;
        fseek(fs, idx, SEEK_SET);
    }
    map<uint16_t, char*>::iterator it;
    for(it=symTab.begin(); it!=symTab.end(); ++it) {
        printf("%05o -> %s\n", it->first, it->second);
    }
}
static bool bRunning = false;
WINDOW *myWindow = NULL;

void stopAgc(void)
{
    bRunning = false;
    nodelay(myWindow, false);
}

int getOctValue(const char *msg, int row)
{
    //char mesg[]="Breakpoint address:";
    char buf[80];
    int br = 0;
    mvwprintw(myWindow,row,0,"%s", msg);
    getstr(buf);
    sscanf(buf, "%o", &br);
    return br;
}

int main(int argc, char *argv[])
{
    logFile = fopen("agc.log", "w");
#ifdef DO_TEST
    test1st();
#else

    CCpu    cpu;

//#define MEMORY_TEST
#ifdef MEMORY_TEST
    doMemTest(cpu.getMem());
    return 0;
#endif
//#define DIV_TEST
#ifdef DIV_TEST
    doDivTest(&cpu);
    return 0;
#endif


    int n = 0;
    int br;
    char key;
    cpu.readCore(argv[1]);
    uint16_t brAddr = 0;

    if( argc == 3 ) {
        fprintf(logFile,"Read symbols ...\n");
        readSymbols(argv[argc-1]);
//        return -1;
//        sscanf(argv[argc-1], "%o", &n);
//        brAddr = n;
    }

    fprintf(logFile,"Starting!\n");
    fflush(logFile);

    myWindow = initscr();			/* Start curses mode 		  */
    noecho();

    if( brAddr != 0 ) {
        cpu.setBrkp(brAddr);
        timeout(-1);
        bRunning = true;
        nodelay(myWindow, true);
    }

    dskyInit();

    do {
        if( brAddr == cpu.getPC() && key !=  '&' ) {
            stopAgc();
        }
        updateScreen(myWindow, &cpu, bRunning);
	    key = getch();			/* Wait for user input */
        if( bRunning && key == 'b' ) {
            stopAgc();
            cpu.sst();
        }
        if( bRunning  ) {
            mvwprintw(myWindow,12,0,"Running (%05o)", cpu.getPC());
        } else {
            mvwprintw(myWindow,12,0,"                ");
        }

        switch( key ) {
        case -1:
            if( !bRunning )
                break;
        case 's': 
            cpu.sst();
            break;
        case 'q':
            continue;
        case 'r':
            timeout(-1);
            bRunning = true;
            nodelay(myWindow, true);
            //halfdelay(1);
            key = '&'; // Mark as running ...
            break;
        case 'b':
            brAddr = (uint16_t)getOctValue("Breakpoint address:", 15);
            cpu.setBrkp(brAddr);
            break;
        case 'm':
            br = getOctValue("Memory address:", 15);
            cpu.memWatch(br);
            break;
        case 'w':
            br = getOctValue("Memory address:", 15);
            cpu.memBreak(br);
            break;
        case 'p':
            br = getOctValue("Enter new PC:", 12);
            cpu.setPC(br);
            break;

        // Handle DSKY keys ...
        case 'P': cpu.keyPress(DSKY_PRO);     break;
        case 'V': cpu.keyPress(DSKY_VERB);    break;
        case 'N': cpu.keyPress(DSKY_NOUN);    break;
        case 'E': cpu.keyPress(DSKY_ENTR);    break;
        case '0': cpu.keyPress(DSKY_0);       break;
        case '1': cpu.keyPress(DSKY_1);       break;
        case '2': cpu.keyPress(DSKY_2);       break;
        case '3': cpu.keyPress(DSKY_3);       break;
        case '4': cpu.keyPress(DSKY_4);       break;
        case '5': cpu.keyPress(DSKY_5);       break;
        case '6': cpu.keyPress(DSKY_6);       break;
        case '7': cpu.keyPress(DSKY_7);       break;
        case '8': cpu.keyPress(DSKY_8);       break;
        case '9': cpu.keyPress(DSKY_9);       break;
        case 'R': cpu.keyPress(DSKY_RSET);    break;
        case 'K': cpu.keyPress(DSKY_KEY_REL); break;
        case '+': cpu.keyPress(DSKY_PLUS);    break;
        case '-': cpu.keyPress(DSKY_MINUS);   break;
        case 'C': cpu.keyPress(DSKY_CLR);     break;
        };
    } while(key != 'q');
	endwin();			/* End curses mode		  */
#endif
	return 0;
}