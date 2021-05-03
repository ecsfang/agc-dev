#include <stdio.h>
#include <memory.h>
#include "cpu.h"

#include <ncurses.h>

FILE    *logFile=NULL;

//#include <unistd.h>
//#include <termios.h>

/*
char getch() {
        char buf = 0;
        struct termios old = {0};
        if (tcgetattr(0, &old) < 0)
                perror("tcsetattr()");
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &old) < 0)
                perror("tcsetattr ICANON");
        if (read(0, &buf, 1) < 0)
                perror ("read()");
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &old) < 0)
                perror ("tcsetattr ~ICANON");
        return (buf);
}
*/

/*
006244,000034:    4000                                           COUNT*   $$/RUPTS                              #  FIX-FIX LEAD INS
006245,000035:    4000           00004                           INHINT                                         #  GO
006246,000036:    4001           34054                           CAF      GOBB                                  
006247,000037:    4002           56006                           XCH      BBANK                                 
006248,000038:    4003           12667                           TCF      GOPROG                                
*/
/*
int main(int argc, char *argv[])
{
    CCpu    cpu;

    printf("Use file: %s\n", argv[1]);

    cpu.readCore(argv[1]);

    printf("Address: %06o - %06o\n", 0, TOTAL_SIZE-1);

    __uint16_t op = cpu.getOP();
    printf("OP: %06o\n", op);
    cpu.dispReg();
    cpu.disasm();
    int n = 50;
    char key = 0;
    while( key != 'q' ) {
        key = getch();
        switch( key ) {
        case 's': 
            cpu.sst();
            break;
        case 'q':
            continue;
        };
        cpu.dispReg();
        cpu.getOP();
        cpu.disasm();
    }
    return 0;
}
*/

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

void updateScreen(WINDOW *wnd, CCpu *cpu, bool bRun)
{
    cpu->dispReg(wnd);
    if( !bRun ) {
        for(int n=0; n<5; n++)
            mvwprintw(wnd,16+n,0,"%s           ", cpu->disasm(n-1));
    }
    refresh();			    /* Print it on to the real screen */
}

int main(int argc, char *argv[])
{
    logFile = fopen("agc.log", "w");
#ifdef DO_TEST
    test1st();
#else

    CCpu    cpu;
    int n = 0;
    char key;
    cpu.readCore(argv[1]);
    bool bRunning = false;
    uint16_t brAddr = 0;

    fprintf(logFile,"Starting!\n");

    WINDOW *myWindow = initscr();			/* Start curses mode 		  */
    noecho();
    do {
        if( brAddr == cpu.getPC() ) {
            bRunning = false;
            nodelay(myWindow, false);
        }
        updateScreen(myWindow, &cpu, bRunning);
	    key = getch();			/* Wait for user input */
        if( bRunning && key == 'b' ) {
            bRunning = false;
            nodelay(myWindow, false);
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
            break;
        case 'b':
            {
                char mesg[]="Breakpoint address:";
                char buf[80];
                int row, col;
                int br = 0;
                mvwprintw(myWindow,15,0,"%s", mesg);
                getstr(buf);
                sscanf(buf, "%o", &br);
                brAddr = (uint16_t)br;
                cpu.setBrkp(brAddr);
            }
            break;
        case 'm':
            {
                char mesg[]="Memory address:";
                char buf[80];
                int row, col;
                int br = 0;
                mvwprintw(myWindow,15,0,"%s", mesg);
                getstr(buf);
                sscanf(buf, "%o", &br);
                cpu.memWatch(br);
            }
            break;
        case 'p':
            {
                char mesg[]="Enter new PC:";
                char buf[80];
                int row, col;
                int pc = 0;
                mvwprintw(myWindow,12,0,"%s", mesg);
                getstr(buf);
                sscanf(buf, "%o", &pc);
                cpu.setPC(pc);
            }

        };
    } while(key != 'q');
	endwin();			/* End curses mode		  */
#endif
	return 0;
}