#include <stdio.h>
#include "cpu.h"
#include <iostream>
#include <fcntl.h>      // open()
#include <termios.h>    // termios, TCSANOW, etc.
#include <unistd.h>     // write(), read(), close()
#include <string.h>     // memset()

static int fdDSKY = 0;



/*
Bit 1 lights the "PRIO DISP" indicator.
Bit 2 lights the "NO DAP" indicator.
Bit 3 lights the  "VEL" indicator.
Bit 4 lights the "NO ATT" indicator.
Bit 5 lights the  "ALT" indicator.
Bit 6 lights the "GIMBAL LOCK" indicator.
Bit 8 lights the "TRACKER" indicator.
Bit 9 lights the "PROG" indicator.
*/

typedef struct {
    uint8_t m[2];
    uint8_t v[2];
    uint8_t n[2];
    uint8_t x1s;
    uint8_t x1[5];
    uint8_t x2s;
    uint8_t x2[5];
    uint8_t x3s;
    uint8_t x3[5];
} DSP_t;

char val2chr(uint8_t v)
{
    switch(v) {
        case 0:  return 'x';
        case 21: return '0';
        case 3:  return '1';
        case 25: return '2';
        case 27: return '3';
        case 15: return '4';
        case 30: return '5';
        case 28: return '6';
        case 19: return '7';
        case 29: return '8';
        case 31: return '9';
        default: return '?';
    }
}

#define COL_4   70
#define COL_5   55

void send2dsky(uint16_t addr, uint16_t data)
{
    static uint16_t oData = 9999;
    if( fdDSKY > 0 && oData != data ) {
        char cmd[16];
        int n = 0;
        if( addr == 010 )
            n = sprintf(cmd, "#%04X\n", data & 0xFFFF);
        if( addr == 011 )
            n = sprintf(cmd, "&%04X\n", data & 0xFFFF);
        fprintf(logFile,"Send to DSKY: %5.5s", cmd);
        fflush(logFile);
        write(fdDSKY, cmd, n);
        oData = data;
    }
}

void CCpu::updateDSKY(WINDOW *win, bool bRun)
{
    if( bRun && !mem.dsky() )
        return;

    fprintf(logFile,"Update DSKY!\n");

    static DSP_t dsp;
    static DSP_t bkDsp;

    dsp.x1s = dsp.x2s = dsp.x3s = ' ';

    uint16_t *out = mem.getOutMem();
    dsp.m[0] = val2chr((out[11] & 0b1111100000) >> 5);
    dsp.m[1] = val2chr(out[11] & 0b0000011111);

    dsp.v[0] = val2chr((out[10] & 0b1111100000) >> 5);
    dsp.v[1] = val2chr(out[10] & 0b0000011111);

    dsp.n[0] = val2chr((out[9] & 0b1111100000) >> 5);
    dsp.n[1] = val2chr(out[9] & 0b0000011111);

    // Digit 11
    dsp.x1[0] = val2chr(out[8] & 0b0000011111);
    // Digit 12+13
    dsp.x1[1] = val2chr((out[7] & 0b1111100000) >> 5);
    dsp.x1[2] = val2chr(out[7] & 0b0000011111);
    if( out[7] & 0b10000000000 )
        dsp.x1s = '+';
    // Digit 14+15
    dsp.x1[3] = val2chr((out[6] & 0b1111100000) >> 5);
    dsp.x1[4] = val2chr(out[6] & 0b0000011111);
    if( out[6] & 0b10000000000 )
        dsp.x1s = '-';
    // Digit 21+22
    dsp.x2[0] = val2chr((out[5] & 0b1111100000) >> 5);
    dsp.x2[1] = val2chr(out[5] & 0b0000011111);
    if( out[5] & 0b10000000000 )
        dsp.x2s = '+';
    // Digit 23+24
    dsp.x2[2] = val2chr((out[4] & 0b1111100000) >> 5);
    dsp.x2[3] = val2chr(out[4] & 0b0000011111);
    if( out[4] & 0b10000000000 )
        dsp.x2s = '-';
    // Digit 25
    dsp.x2[4] = val2chr((out[3] & 0b1111100000) >> 5);
    // Digit 31
    dsp.x3[0] = val2chr(out[3] & 0b0000011111);
    // Digit 32+33
    dsp.x3[1] = val2chr((out[2] & 0b1111100000) >> 5);
    dsp.x3[2] = val2chr(out[2] & 0b0000011111);
    if( out[2] & 0b10000000000 )
        dsp.x3s = '+';
    // Digit 34+35
    dsp.x3[3] = val2chr((out[1] & 0b1111100000) >> 5);
    dsp.x3[4] = val2chr(out[1] & 0b0000011111);
    if( out[1] & 0b10000000000 )
        dsp.x3s = '-';

	start_color();			/* Start color 			*/
	init_pair(1, COLOR_GREEN, COLOR_BLACK);
	init_pair(2, COLOR_BLACK, COLOR_WHITE);
    attron(COLOR_PAIR(1));

    int y = 0;
    mvwprintw(win, y++, COL_4, "    %2.2s", dsp.m);
    mvwprintw(win, y++, COL_4, "%2.2s  %2.2s", dsp.v, dsp.n);
    mvwprintw(win, y++, COL_4, "%c%5.5s", dsp.x1s, dsp.x1);
    mvwprintw(win, y++, COL_4, "%c%5.5s", dsp.x2s, dsp.x2);
    mvwprintw(win, y++, COL_4, "%c%5.5s", dsp.x3s, dsp.x3);
/*
    if( fdDSKY > 0 ) {
        char cmd[16];

        if( bkDsp.m[0] != dsp.m[0] ) {
            sprintf(cmd, "#P1%c", dsp.m[0]);
            write(fdDSKY, cmd, 4);
            bkDsp.m[0] = dsp.m[0];
        }
        if( bkDsp.m[1] != dsp.m[1] ) {
            sprintf(cmd, "#P2%c", dsp.m[1]);
            write(fdDSKY, cmd, 4);
            bkDsp.m[1] = dsp.m[1];
        }
        if( bkDsp.v[0] != dsp.v[0] ) {
            sprintf(cmd, "#V1%c", dsp.v[0]);
            write(fdDSKY, cmd, 4);
            bkDsp.v[0] = dsp.v[0];
        }
        if( bkDsp.v[1] != dsp.v[1] ) {
            sprintf(cmd, "#V2%c", dsp.v[1]);
            write(fdDSKY, cmd, 4);
            bkDsp.v[1] = dsp.v[1];
        }
        if( bkDsp.n[0] != dsp.n[0] ) {
            sprintf(cmd, "#N1%c", dsp.n[0]);
            write(fdDSKY, cmd, 4);
            bkDsp.n[0] = dsp.n[0];
        }
        if( bkDsp.n[1] != dsp.n[1] ) {
            sprintf(cmd, "#N2%c", dsp.n[1]);
            write(fdDSKY, cmd, 4);
            bkDsp.n[1] = dsp.n[1];
        }
    }
***/
    attroff(COLOR_PAIR(1));

    y=0;
typedef struct {
    __uint16_t  b;
    const char *lbl;
} Lamp_t;
Lamp_t ind[8] = {
    { BIT_1, " PRIO DISP " },
    { BIT_2, " NO DAP " },
    { BIT_3, " VEL " },
    { BIT_4, " NO ATT " },
    { BIT_5, " ALT " },
    { BIT_6, " GIMBAL LOCK " },
    { BIT_8, " TRACKER " },
    { BIT_9, " PROG " }
};

    attron(COLOR_PAIR(2));
    for(int n=0; n<8; n++)
        if( out[12] & ind[n].b )
            mvwprintw(win, n, COL_5, "%s", ind[n].lbl);
    attroff(COLOR_PAIR(2));

    for(int n=0; n<8; n++)
        if( !(out[12] & ind[n].b) )
            mvwprintw(win, n, COL_5, "%s", ind[n].lbl);
/*
    mvwprintw(win, y++, COL_5, out[12] & BIT_1 ? "PRIO DISP" : "          ");
    mvwprintw(win, y++, COL_5, out[12] & BIT_2 ? "NO DAP" : "          ");
    mvwprintw(win, y++, COL_5, out[12] & BIT_3 ? "VEL" : "          ");
    mvwprintw(win, y++, COL_5, out[12] & BIT_4 ? "NO ATT" : "          ");
    mvwprintw(win, y++, COL_5, out[12] & BIT_5 ? "ALT" : "          ");
    mvwprintw(win, y++, COL_5, out[12] & BIT_6 ? "GIMBAL LOCK" : "          ");
    mvwprintw(win, y++, COL_5, out[12] & BIT_8 ? "TRACKER" : "          ");
    mvwprintw(win, y++, COL_5, out[12] & BIT_9 ? "PROG" : "          ");
*/
//    fprintf(logFile, "Update DSKY %05o\n", out[12]);
}

void CCpu::keyPress(Key_e key)
{
    uint16_t    io;
    switch(key) {
        case DSKY_PRO:
            io = mem.readIO(032);
            mem.writeIO(032, io & ~BIT_15);
            addInterrupt(iKEYRUPT1);
            break;
        default:
            mem.writeIO(015, key);
            addInterrupt(iKEYRUPT1);
            fprintf(logFile,"KEYRUPT interrupt! (key=%02o\n", key);
    }
}

int dskyInit() {
#if 0
    return 0;
#else
    const char* device = "/dev/ttyACM0"; // <-- ändra om behövs

    if( fdDSKY > 0 )
        return 0;
    if (fdDSKY < 0) {
        return 1;
    }
    
    fdDSKY = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fdDSKY < 0) {
        perror("open");
        return 1;
    }

    // --- Ställ in serialport ---
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fdDSKY, &tty) != 0) {
        perror("tcgetattr");
        close(fdDSKY);
        return 1;
    }

    cfsetospeed(&tty, B115200); // Baudrate ut
    cfsetispeed(&tty, B115200); // Baudrate in

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                     // disable break processing
    tty.c_lflag = 0;                            // no signaling chars, no echo
    tty.c_oflag = 0;                            // no remapping, no delays
    tty.c_cc[VMIN]  = 1;                        // read blocks till 1 byte
    tty.c_cc[VTIME] = 5;                        // 0.5 sec timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);     // shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD);            // ignore modem controls
    tty.c_cflag &= ~(PARENB | PARODD);          // shut off parity
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fdDSKY, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        close(fdDSKY);
        return 1;
    }
    std::cout << "DSKY init ok!\n";
    /*
    // --- Skicka några bytes ---
    const char* msg = "Hello Pico!\n";
    ssize_t written = write(fdDSKY, msg, strlen(msg));
    if (written < 0) {
        perror("write");
    } else {
        std::cout << "DSKY\n";
    }

    // --- Läs svar (valfritt) ---
    char buf[100];
    ssize_t n = read(fdDSKY, buf, sizeof(buf)-1);
    if (n > 0) {
        buf[n] = '\0';
        std::cout << "Mottaget: " << buf << "\n";
    } else {
        perror("read");
    }
**/
    //close(fdDSKY);
    return 0;
#endif
}

void closeDsky()
{
    if( fdDSKY ) {
        close(fdDSKY);
        fdDSKY = 0;
    }
}