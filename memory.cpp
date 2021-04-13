#include <stdio.h>
#include <memory.h>

#define ERASABLE_BLK_SIZE   0400
#define FIXED_BLK_SIZE      02000

#define TOTAL_SIZE  (8 * ERASABLE_BLK_SIZE + 38 * FIXED_BLK_SIZE)

typedef union {
    struct {
        __uint16_t  A;          // 00
        __uint16_t  L;          // 01
        __uint16_t  Q;          // 02
        __uint16_t  EB;         // 03 "erasable bank register"
        __uint16_t  FB;         // 04 "fixed bank register"
        __uint16_t  Z;          // 05
        __uint16_t  BB;         // 06
        __uint16_t  _res;       // 07
        __uint16_t  ARUPT;      // 10
        __uint16_t  LRUPT;      // 11
        __uint16_t  QRUPT;      // 12
        __uint32_t  SAMPTIME;   // 13-14
        __uint16_t  ZRUPT;      // 15
        __uint16_t  BBRUPT;     // 16
        __uint16_t  BRUPT;      // 17
        __uint16_t  SYR;        // 20
        __uint16_t  SR;         // 21
        __uint16_t  CYL;        // 22
        __uint16_t  EDOP;       // 23
        __uint16_t  TIME2;      // 24
        __uint16_t  TIME1;      // 25
        __uint16_t  TIME3;      // 26
        __uint16_t  TIME4;      // 27
        __uint16_t  TIME5;      // 30
        __uint16_t  TIME6;      // 31
        __uint16_t  CDUX;       // 32
        __uint16_t  CDUY;       // 33
        __uint16_t  CDUZ;       // 34
        __uint16_t  OPTY;       // 35
        __uint16_t  OPTX;       // 36
        __uint16_t  PIPAX;      // 37
        __uint16_t  PIPAY;      // 40
        __uint16_t  PIPAZ;      // 41
        __uint16_t  Q_RHCCTR;   // 42
        __uint16_t  P_RHCCTR;   // 43
        __uint16_t  R_RHCCTR;   // 44
        __uint16_t  INLINK;     // 45
        __uint16_t  RNRAD;      // 46
        __uint16_t  GYROCTR;    // 47
        __uint16_t  CDUXCMD;    // 50
        __uint16_t  CDUYCMD;    // 51
        __uint16_t  CDUZCMD;    // 52
        __uint16_t  OPTYCMD;    // 53
        __uint16_t  OPTXCMD;    // 54
        __uint16_t  THRUST;     // 55
        __uint16_t  LEMONM;     // 56
        __uint16_t  OUTLINK;    // 57
        __uint16_t  ALTM;       // 60
    };
    __uint16_t  word[TOTAL_SIZE];
}   Mem_t;

class CMemory {
    Mem_t   mem;
    __uint16_t  FEB;
public:
    CMemory() {
        memset(&mem, 0, sizeof(Mem_t));
        FEB = 0;
        for(int i=0; i<TOTAL_SIZE; i++)
            mem.word[i] = i;
        setEB(0);
        setFB(0);    
    }
    void setEB(__uint16_t eb) {
        mem.EB = eb;
        mem.BB = mem.EB | mem.FB;
    }
    void setFB(__uint16_t eb) {
        mem.FB = eb;
        mem.BB = mem.EB | mem.FB;
    }
    __uint16_t  read(__uint16_t addr) {
        __uint16_t  _addr = 0;
        if( addr < 01400) {
            // Unswitched erasable memory
            _addr = addr;
        } else if( addr < 02000) {
            // Switched erasable memory
            if( mem.EB & ~03400 ) {
                printf("Invalid EB: %05o!\n", mem.EB);
                return 0;
            }
            _addr = addr-01400 + mem.EB;
        } else if( addr < 04000) {
            // Switched fixed memory
            __int16_t   bank = mem.FB;
            if( bank > 027 && FEB == 1 )
                bank += 010;
            if( bank > 033 ) {
                printf("Invalid bank: %03o!\n", bank);
                return 0;
            }
            _addr = addr + bank*FIXED_BLK_SIZE;
        } else if( addr < 010000) {
            // Unswitched fixed memory
            _addr = addr;
        } else {
            printf("Invalid address: %06o!\n", addr);
            return 0;
        }
        printf("Addr: %06o -> mem[%05o] : %05o\n", addr, _addr, mem.word[_addr]);
        return mem.word[_addr];
    }
};



int main(int argc, char *argv[])
{
    CMemory mem;

    printf("Address: %06o - %06o\n", 0, TOTAL_SIZE-1);

    mem.read(1);
    mem.read(01400);
    mem.setEB(02000);
    mem.read(01400);
    mem.setEB(03400);
    mem.read(01400);
    return 0;
}