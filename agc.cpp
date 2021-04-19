#include <stdio.h>
#include <memory.h>
#include "memory.h"

class CAgc {
    CMemory mem;
    bool bExtracode = false;
    bool bInterrupt = false;
    bool bStep = false;
public:
    CAgc() {
        mem.read(04000);
        mem.write(04000, 012345);
        mem.read(04000);
        mem.setZ(04000);
//        mem.init();
    }
    void readCore(char *core);
    __uint16_t getOP() {
        return mem.getOP();
    }
    int sst(void);
    int op0(__uint16_t opc);
    int op1(__uint16_t opc);
    int op2(__uint16_t opc);
    int op3(__uint16_t opc);
    int op4(__uint16_t opc);
    int op5(__uint16_t opc);
    int op6(__uint16_t opc);
    int op7(__uint16_t opc);
    int op0ex(__uint16_t opc);
    int op1ex(__uint16_t opc);
    int op2ex(__uint16_t opc);
    int op3ex(__uint16_t opc);
    int op4ex(__uint16_t opc);
    int op5ex(__uint16_t opc);
    int op6ex(__uint16_t opc);
    int op7ex(__uint16_t opc);
    void dispReg(void);
};

void CAgc::dispReg(void)
{
    printf("A: %05o ", mem.read(0));
    printf("Z: %05o (%05o) ", mem.read(5), mem.addr2mem(mem.read(5)));
    printf("BB: %05o[fb:%o eb:%o] ", mem.read(6), (mem.read(6)>>10)&037, mem.read(6)&07);
    printf("EB: %05o(%o) ", mem.read(3), (mem.read(3)>>8)&07);
    printf("FB: %05o(%o) ", mem.read(4), (mem.read(4)>>10)&037);
    printf("\n");
}

void CAgc::readCore(char *core)
{
    FILE *fp = fopen(core,"r");
    char buf[1024];
    int bank = 0;
    __uint16_t  addr;
    while(fgets(buf,1024,fp)) {
        if( strncmp("BANK=", buf, 5) == 0 ) {
            sscanf(buf, "BANK=%o", &bank);
            addr = 010000 + bank * FIXED_BLK_SIZE;
            printf("BANK=%o [%06o]\n", bank, addr);
        }
        if( buf[0] >= '0' && buf[0] < '8') {
            char *p = buf;
            unsigned int word;
            p = strtok(buf, ", ");
            while( p && sscanf(p, "%o", &word) == 1 ) {
                printf("%02o,%04o [%06o] : %05o\n", bank, addr-(010000 + bank * FIXED_BLK_SIZE), addr, word);
                p = strtok(NULL, ", "); //+= 6;
                mem.write(addr++, word);
            }
//            printf("\n");
        }
    }
    mem.setFB(020 * FIXED_BLK_SIZE);
    mem.setZ(02101);
    printf("Start: [%06o](%06o) : %05o\n", mem.getZ(), mem.addr2mem(mem.getZ()), mem.getOP());
    fclose(fp);
}

int CAgc::op0ex(__uint16_t opc)
{
    int ret = -1;
    __uint16_t kc = opc & MASK_IO_ADDRESS;
    __uint16_t io;
    switch(opc & 077000) {
        case 000000:
            {
                io = mem.readIO(kc);
                mem.write(0, io);
                printf("READ %03o\n", kc);
                ret = 0;
            }
            break;
        case 001000:
            {
                mem.writeIO(kc, mem.read(0));
                printf("WRITE %03o\n", kc);
                ret = 0;
            }
            break;
        case 002000:
            {
                io = mem.readIO(kc);
                mem.write(0, io & mem.read(0));
                printf("RAND %03o\n", kc);
                ret = 0;
            }
            break;
        case 003000:
            {
                io = mem.readIO(kc);
                mem.write(0, io & mem.read(0));
                mem.writeIO(kc, mem.read(0));
                printf("WAND %03o\n", kc);
                ret = 0;
            }
            break;
        case 004000:
            {
                io = mem.readIO(kc);
                mem.write(0, io | mem.read(0));
                printf("ROR %03o\n", kc);
                ret = 0;
            }
            break;
        case 005000:
            {
                io = mem.readIO(kc);
                mem.write(0, io | mem.read(0));
                mem.writeIO(kc, mem.read(0));
                printf("WOR %03o\n", kc);
                ret = 0;
            }
            break;
        case 006000:
            {
                io = mem.readIO(kc);
                mem.write(0, io ^ mem.read(0));
                mem.writeIO(kc, mem.read(0));
                printf("RXOR %03o\n", kc);
                ret = 0;
            }
            break;
        default:
            printf("Unknown opcode %05o!\n", opc);
    }
    return ret;
}

int CAgc::op1ex(__uint16_t opc)
{
    int ret = -1;
    // The "Transfer Control to Fixed" instruction jumps to a
    // memory location in fixed (as opposed to erasable) memory.
    __uint16_t k = opc & MASK_12B_ADDRESS;
    if( k & 06000 == 0) {
        // Double divide
        printf("DV %04o\n", k);
    } else {
        // BZF K
        printf("BZF %04o\n", k);
        if( mem.getA() == 0 || (mem.getA()&NEG_ZERO) == NEG_ZERO ) {
            mem.setZ(k);
            bStep = false;
        }
        ret = 0;
    }
    return ret;
}

int CAgc::op2ex(__uint16_t opc)
{
    int ret = -1;
    __uint16_t  q, x;
    // The "Transfer Control to Fixed" instruction jumps to a
    // memory location in fixed (as opposed to erasable) memory.
    __uint16_t k = opc & MASK_10B_ADDRESS;
    switch((opc & QC_MASK) >> 10) {
    case 01:
        q = mem.getQ();
        x = mem.read(k);
        mem.write(k,q);
        mem.setQ(x);
        printf("QXCH %04o\n", k);
        ret = 0;
        break;
    case 02:
        printf("AUG %04o\n", k);
        break;
    case 03:
        printf("DIM %04o\n", k);
        break;
    }
    return ret;
}


//******************************************************************

int CAgc::op0(__uint16_t opc)
{
    int ret = -1;
    switch(opc) {
/*
        case 000000:
            printf("XXALQ\n");
            //ret = 0;
            break;
        case 000001:
            printf("XLQ\n");
            //ret = 0;
            break;
*/
        case 000002:
            printf("RETURN\n");
            mem.setZ(mem.getQ());
            bStep = false;
            ret = 0;
            break;
        case 000003:
            printf("RELINT\n");
            bInterrupt = true;
            ret = 0;
            break;
        case 000004:
            printf("INHINT\n");
            bInterrupt = false;
            ret = 0;
            break;
        case 000006:
            printf("EXTEND\n");
            bExtracode = true;
            ret = 0;
            break;
        default:
            {
                __uint16_t k = opc & MASK_12B_ADDRESS;
                mem.setQ(mem.getZ() + 1);   // Set return address
                mem.setZ(k);
                printf("TC %05o\n", k);
                bStep = false;
                ret = 0;
            }
    }
    return ret;
}

int CAgc::op1(__uint16_t opc)
{
    __uint16_t qc = opc & QC_MASK;
    if( qc ) {
        // The "Transfer Control to Fixed" instruction jumps to a
        // memory location in fixed (as opposed to erasable) memory.
        __uint16_t k = opc & MASK_12B_ADDRESS;
        mem.setZ(k);
        printf("TCF %05o\n", mem.getZ());
        bStep = false;
    } else {
        // If (K) > 0, then we take the instruction at I + 1, and (A) will be reduced
        // by 1, i.e. (K) - 1. If (K) = + 0, we take the instruction at I + 2, and (A) will
        // be set to +O. If (K) < -0, we take the instruction at I + 3, and (A) will be set
        // to its absolute value less 1. If (K) = -0, we take the instruction at I + 4, and
        // (A) will be set to + 0. CCS always leaves a positive quantity in A. 
        __uint16_t k = opc & MASK_10B_ADDRESS;
        printf("CCS %04o\n", k);
        __uint16_t  m = mem.read12(k);
        if( m > 0 && !(k & 040000) ) {
            mem.setA(k-1);
        } else if( m == 0 ) {
            mem.setA(0);
            mem.setZ(mem.getZ() + 1);
        } else if( m == NEG_ZERO ) {
            mem.setA(0);
            mem.setZ(mem.getZ() + 3);
        } else {
            mem.setA(mem.getA()-1); // TBD
            mem.setZ(mem.getZ() + 2);
        }
        bStep = true;
    }
    return 0;
}

int CAgc::op2(__uint16_t opc)
{
    int ret = -1;
    __uint16_t a, l, x, k10 = opc & MASK_10B_ADDRESS;
    switch((opc & QC_MASK) >> 10) {
        case 00:
            printf("DAS %04o! TBD!\n", k10);
            break;
        case 01:
            l = mem.read(1);
            x = mem.read12(k10);
            mem.write12(k10,l);
            mem.write(1,x);
            printf("LXCH %05o[%05o] (%05o<->%05o)\n", mem.addr2mem(k10), k10, l, x);
            ret = 0;
            break;
        case 02:
            x = mem.read12(k10);
            mem.write12(k10,x+1);
            printf("INCR %05o[%05o] (%05o->%05o)\n", mem.addr2mem(k10), k10, x, mem.read12(k10));
            ret = 0;
            break;
        case 03:
            a = mem.getA();
            x = mem.read12(k10);
            mem.setA(a + x);
            printf("ADS %04o[%05o] (%05o + %05o)\n", mem.addr2mem(k10), k10, a, x);
            ret = 0;
            break;
        default:
            printf("Unknown opcode %05o!\n", opc);
    }
    return ret;
}

int CAgc::op3(__uint16_t opc)
{
    // The "Clear and Add" (or "Clear and Add Erasable" or "Clear and Add Fixed") instruction moves
    // the contents of a memory location into the accumulator.
    __uint16_t k = opc & MASK_12B_ADDRESS;
    mem.write(0, mem.read12(k));
    if( (opc & QC_MASK) == 0 )
        printf("CAE %04o (%05o -> A)\n", mem.read12(k), mem.read(0));
    else
        printf("CAF %04o (%05o -> A)\n", mem.read12(k), mem.read(0));
    return 0;
}

int CAgc::op4(__uint16_t opc)
{
    // The "Clear and Add" (or "Clear and Add Erasable" or "Clear and Add Fixed") instruction moves
    // the contents of a memory location into the accumulator.
    __uint16_t k = opc & MASK_12B_ADDRESS;
    mem.write(0, ~mem.read12(k));
    printf("CS %04o (%05o -> A)\n", ~mem.read12(k), mem.read(0));
    return 0;
}

int CAgc::op5(__uint16_t opc)
{
    int ret = -1;
    __uint16_t a, l, x, x1, k10 = opc & MASK_10B_ADDRESS;
    switch((opc & QC_MASK) >> 10) {
        case 01: // swap [k-1,k] and [a,l]
            x = mem.read12(k10-1);
            x1 = mem.read12(k10);
            a = mem.read(0);
            l = mem.read(1);
            mem.write12(k10-1,a);
            mem.write12(k10,l);
            mem.write(0,x);
            mem.write(1,x1);
            if( opc == 052005 )
                printf("DTCF\n");
            else if( opc == 052006 )
                printf("DTCB\n");
            else
                printf("DXCH %05o\n", k10-1);
            ret = 0;
            break;
        case 02:
            a = mem.read(0);
            mem.write12(k10,a);
            if( opc == 054000 )
                printf("OVSK\n");
            if( opc == 054005 )
                printf("TCAA\n");
            else
                printf("TS %04o\n", k10);
            ret = 0;
            break;
        case 03:
            a = mem.read(0);
            x = mem.read12(k10);
            mem.write(0,x);
            mem.write12(k10,a);
            printf("XCF A[%05o] <-> %04o[%05o]\n", a, k10, x);
            ret = 0;
            break;
        case 00:
            printf("INDEX\n");
            //ret = 0;
            break;
        default:
            printf("Unknown opcode %05o!\n", opc);
    }
    return ret;
}

int CAgc::op6(__uint16_t opc)
{
    int ret = -1;
    __uint16_t k = opc & MASK_12B_ADDRESS;
    __uint16_t m = mem.read12(k);
    __uint16_t a = mem.read(0);
    
    mem.write(0, a + m);
    printf("AD %04o [%05o]\n", k, m);
    ret = 0;
    return ret;
}

int CAgc::op7(__uint16_t opc)
{
    int ret = -1;
    __uint16_t k = opc & MASK_12B_ADDRESS;
    __uint16_t m = mem.read12(k);
    __uint16_t a = mem.read(0);
    
    mem.write(0, a & m);
    printf("MASK %04o [%05o]\n", k, m);
    ret = 0;
    return ret;
}

int CAgc::sst(void)
{
    int ret = -1;
    __uint16_t op = mem.getOP();
    bStep = true;
    if( bExtracode ) {
        printf("[%04o(%06o)] %05o # ", mem.getZ(), mem.getPysZ(), op);
        switch( op & OPCODE_MASK ) {
            case 000000: ret = op0ex(op); break;
            case 010000: ret = op1ex(op); break;
            case 020000: ret = op2ex(op); break;
//            case 030000: ret = op3ex(op); break;
//            case 050000: ret = op5ex(op); break;
            default:
                printf("Unknown extra code %05o!\n", op);
        }
        bExtracode = false;
    } else {
        printf("\n[%04o(%06o)] %05o > ", mem.getZ(), mem.getPysZ(), op);
        switch( op & OPCODE_MASK ) {
            case 000000: ret = op0(op); break;
            case 010000: ret = op1(op); break;
            case 020000: ret = op2(op); break;
            case 030000: ret = op3(op); break;
            case 040000: ret = op4(op); break;
            case 050000: ret = op5(op); break;
            case 060000: ret = op6(op); break;
            case 070000: ret = op7(op); break;
            default:
                printf("Unknown opcode %05o!\n", op);
        }
    }
    if( bStep )
        mem.step();
    return ret;
}
/*
006244,000034:    4000                                           COUNT*   $$/RUPTS                              #  FIX-FIX LEAD INS
006245,000035:    4000           00004                           INHINT                                         #  GO
006246,000036:    4001           34054                           CAF      GOBB                                  
006247,000037:    4002           56006                           XCH      BBANK                                 
006248,000038:    4003           12667                           TCF      GOPROG                                
*/

int main(int argc, char *argv[])
{
    CAgc    cpu;

    printf("Use file: %s\n", argv[1]);

    cpu.readCore(argv[1]);

    printf("Address: %06o - %06o\n", 0, TOTAL_SIZE-1);

    __uint16_t op = cpu.getOP();
    printf("OP: %06o\n", op);
    int n = 50;
    while( cpu.sst() == 0 && n--) {
        cpu.dispReg();
    }
    cpu.dispReg();
    return 0;
}