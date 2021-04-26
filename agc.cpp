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
/*
__uint16_t add1st(__uint16_t x1, __uint16_t x2, bool *bOF)
{
    __uint16_t s2 = (x1 & 0x4000) << 1;
    __uint16_t s = x1 + x2;

    if( s & 0x8000 )
        s++;
    *bOF = (s2>>1) != (s & 0x4000);
    //printf("(s2:%d s1:%d of:%d)", s2 ? 1:0, (s&0x4000)?1:0, );
    if( *bOF ) {
        s &= 0x3FFF;
        s |= s2 ? 0x4000 : 0;
    }
    fprintf(logFile,"1stADD: %05o + %05o = %05o (S1:%d S2:%d)\n", x1, x2, s, s&0x4000?1:0, s2&0x8000?1:0);
    fflush(logFile);
    return (s & 0x7FFF) | s2;
}***/

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
void test1st(void)
{
    __uint16_t x1, x2;
    bool of;
    testAdd(002000, 006000);
    testAdd(022000, 026000);
    testAdd(074000, 070000);
    testAdd(054000, 050000);
    testAdd(14908, 8265);
    testAdd(037777, 040000);
    testAdd(040000, 037777);
    testAdd(037777, 000001);
    testAdd(000001, 037777);
    testAdd(040000, 040000);
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
    fprintf(logFile,"Starting!\n");

    WINDOW *myWindow = initscr();			/* Start curses mode 		  */
    noecho();
    do {
        cpu.dispReg(myWindow);
        if( !bRunning ) {
            for(int n=0; n<5; n++)
                mvwprintw(myWindow,14+n,0,"%s", cpu.disasm(n-1));
        }
        refresh();			    /* Print it on to the real screen */
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
            //nodelay(myWindow, true);
            halfdelay(1);
            break;
        };
//        cpu.dispReg(myWindow);
//        mvwprintw(myWindow,14,0,"%s", cpu.disasm(-1));
//        mvwprintw(myWindow,15,0,"%s", cpu.disasm());
//        mvwprintw(myWindow,16,0,"%s", cpu.disasm(+1));
    } while(key != 'q');
	endwin();			/* End curses mode		  */
#endif
	return 0;
}