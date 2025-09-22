#include <stdio.h>
#include "cpu.h"
#include <map>
#include <iostream>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <ncurses.h>
#include "argh.h"
#include "testing.h"

using namespace std;

// The instance of the AGC CPU
CCpu    cpu;

static volatile bool bRunning = false;

volatile uint32_t tickCounter = 0; // Variable to increment

// Signal handler for the timer
void timerHandler(int signum) {
    if( bRunning ) {
        // Update counter every 0.5 ms
        tickCounter++;
    }
}

FILE    *logFile=NULL;
bool bFileLogging = false;
bool bExtraLogging = false;
extern void dumpLog(void);

extern int dskyInit(void);


map<uint16_t,char*> symTab;

#define DIS_LINES   20
#define CLK_LINE    (DIS_LINES+6)

void updateScreen(WINDOW *wnd, CCpu *cpu, bool bRun)
{
    static bool _bRun = false;
    cpu->updateDSKY(wnd, bRun);
    if( bRun && (bRun == _bRun) )
        return;
    cpu->dispReg(wnd, bRun);
    if( !bRun ) {
        // Show previous and coming instructions
        for(int n=0; n<5; n++)
            mvwprintw(wnd,DIS_LINES+n,10,"%s           ", cpu->disasm(n-1));
    }
    const char *lbl = cpu->getLabel(cpu->getAbsPC());
    if( lbl ) {
        if( bRun )
            mvwprintw(wnd,DIS_LINES+1,0,"          ");
        else
            mvwprintw(wnd,DIS_LINES+1,0,"%s ", lbl );
    }
    if( bRun )
        mvwprintw(wnd,CLK_LINE,0,"Clk: -----\n");
    else
        mvwprintw(wnd,CLK_LINE,0,"Clk: %5d\n", cpu->getClock());
    refresh();			    // Print it on to the real screen 
    _bRun = bRun;
}

void readSymbols(const char *sym)
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
        if( addr < 04000 ) {
        } else {
            symTab[addr] = strdup(buf);
        }
        idx += 292-15;
        fseek(fs, idx, SEEK_SET);
    }
    //map<uint16_t, char*>::iterator it;
    //for(it=symTab.begin(); it!=symTab.end(); ++it) {
    //    printf("%06o -> %s\n", it->first, it->second);
    //}
}

WINDOW *myWindow = NULL;

void runAgc(bool bRS)
{
    bRunning = bRS;
    cpu.run(bRunning);
    nodelay(myWindow, bRunning);
    curs_set(bRS ? 0 : 1);
}

void startAgc(void)
{
    runAgc(true);
}
void stopAgc(void)
{
    runAgc(false);
}

int getOctValue(const char *msg, int row)
{
    char buf[80];
    int br = 0;
    mvwprintw(myWindow,row,0,"%s", msg);
    getstr(buf);
    sscanf(buf, "%o", &br);
    return br;
}

void help(char *pgm)
{
  printf("\nUsage\n=================\n");
  printf("%s [opt] bin-file [sym-file]\n\n", pgm);
  printf("Options:\n");
  printf(" -? --help    -- shows this help information\n");
  printf(" -d --debug   -- added debug output to consol\n");
  printf(" -r --run     -- include all RUN_TEST data in the output\n");
  printf(" -t --test    -- run a built in test\n");
  printf(" -l --log     -- enable logging\n");
  printf(" -x --extra   -- enable extra logging\n\n");
  printf("If 'out-file' is not given, then output is save in file 'output.json'\n\n"); 
}

int main(int argc, char *argv[])
{
    argh::parser cmdl(argv);
  
    if( argc == 1 || cmdl[{ "-?", "--help" }]) {
      help(argv[0]);
      exit(-1);
    }
  
    printf("agc - Apollo AGC simulator - v0.1beta\n");
    printf("=======================================\n\n");
  
//    if (cmdl[{ "-d", "--debug" }])
//      bDebug = true;

    if (cmdl[{ "-l", "--log" }]) {
        bFileLogging = true;
        if (cmdl[{ "-x", "--extra" }])
            bExtraLogging = true;
    }
    logFile = fopen("agc.log", "w");

struct itimerval timer;

    // Set up the timer to expire every 500 microseconds
    timer.it_value.tv_sec = 0; // Initial delay
    timer.it_value.tv_usec = 500; // First expiration in 500 microseconds
    timer.it_interval.tv_sec = 0; // No additional delay
    timer.it_interval.tv_usec = 500; // Repeat every 500 microseconds

    // Set up the signal handler
    signal(SIGALRM, timerHandler);

    // Start the timer
    setitimer(ITIMER_REAL, &timer, NULL);

    int n = 0;
    int br;
    char key;
    cpu.readCore(argv[1]);
    uint16_t brAddr = 0;

    if( cmdl.size() > 2 ) {
        fprintf(logFile,"Read symbols ...\n");
        readSymbols(cmdl(2).str().c_str());
    }

    fprintf(logFile,"Starting!\n");
    fflush(logFile);

    if (cmdl[{ "-t", "--test" }]) {
        testCpu();
        return 0;
    }

    myWindow = initscr();			/* Start curses mode 		  */
    noecho();

    if( brAddr != 0 ) {
        cpu.setBrkp(brAddr);
        timeout(-1);
        runAgc(true);
    }

    dskyInit();

    do {
        if( brAddr == cpu.getPC() && key !=  '&' ) {
            fprintf(logFile,"Break @%05o!\n", cpu.getPC());
            fflush(logFile);
            stopAgc();
        }
        updateScreen(myWindow, &cpu, bRunning);
	    key = getch();			/* Wait for user input */
        if( bRunning && key == 'b' ) {
            fprintf(logFile,"Stopping @%05o!\n", cpu.getPC());
            fflush(logFile);
            stopAgc();
            cpu.sst();
        }
        if( bRunning && key == 'r' ) {
            fprintf(logFile,"Stopping @%05o!\n", cpu.getPC());
            fflush(logFile);
            stopAgc();
            cpu.sst();
            key = '&';
        }
        if( bRunning  ) {
            if( (n++ & 0xFFFF) == 0 ) {
                static uint8_t ww = 0;
                mvwprintw(myWindow,12,0,"Running (%c)", "-\\|/"[ww++&3]);
            }
        } else {
            mvwprintw(myWindow,12,0,"                ");
        }

        switch( key ) {
        case -1:
            if( !bRunning )
                break;
        case 's': 
            cpu.sst();
            fflush(logFile);
            break;
        case 'q':
            continue;
        case 'r':
            timeout(-1);
            if( !bRunning ) {
                startAgc();
                //halfdelay(1);
                key = '&'; // Mark as running ...
            }
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

    dumpLog();
    cpu.memDump();

	return 0;
}