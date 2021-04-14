#include <stdio.h>
#include <memory.h>
#include "memory.h"

class CAgc {
    CMemory mem;
public:
    CAgc() {
        mem.read(04000);
        mem.write(04000, 012345);
        mem.read(04000);
        mem.setZ(04000);
    }
    __uint16_t getOP() {
        return mem.getOP();
    }

};

int main(int argc, char *argv[])
{
    CAgc    cpu;

    printf("Address: %06o - %06o\n", 0, TOTAL_SIZE-1);

    __uint16_t op = cpu.getOP();
    
    return 0;
}