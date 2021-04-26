#include <stdio.h>
#include "cpu.h"

#if 0
void CCpu::readCore(char *core)
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
#endif
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
//        printf("[%05o %05o] %05o %05o ", word >> 1, word2, word, word2);
        word2 = word;
        mem.write(addr, w & 077777);
        addr++;
    }
    mem.setFB(0); //020 * FIXED_BLK_SIZE);
    mem.setZ(04000);
    printf("Start: [%06o](%06o) : %05o\n", mem.getZ(), mem.addr2mem(mem.getZ()), mem.getOP());
    fclose(fp);
}

int CCpu::op0ex(void)
{
    int ret = -1;
    __uint16_t kc = opc & MASK_IO_ADDRESS;
    __uint16_t io;
    switch (opc & 077000) {
    case 000000:
        io = mem.readIO(kc);
        mem.write(0, io);
        ret = 0;
        break;
    case 001000:
        mem.writeIO(kc, mem.read(0));
        ret = 0;
        break;
    case 002000:
        io = mem.readIO(kc);
        mem.write(0, io & mem.read(0));
        ret = 0;
        break;
    case 003000:
        io = mem.readIO(kc);
        mem.write(0, io & mem.read(0));
        mem.writeIO(kc, mem.read(0));
        ret = 0;
        break;
    case 004000:
        io = mem.readIO(kc);
        mem.write(0, io | mem.read(0));
        ret = 0;
        break;
    case 005000:
        io = mem.readIO(kc);
        mem.write(0, io | mem.read(0));
        mem.writeIO(kc, mem.read(0));
        ret = 0;
        break;
    case 006000:
        io = mem.readIO(kc);
        mem.write(0, io ^ mem.read(0));
        mem.writeIO(kc, mem.read(0));
        ret = 0;
        break;
    default:
        printf("Unknown opcode %05o!\n", opc);
    }
    return ret;
}

int CCpu::op1ex(void)
{
    int ret = -1;
    // The "Transfer Control to Fixed" instruction jumps to a
    // memory location in fixed (as opposed to erasable) memory.
    __uint16_t k = opc & MASK_12B_ADDRESS;
    if( k & 06000 == 0) {
        // Double divide
    } else {
        // BZF K
        if( mem.getA() == 0 || (mem.getA()&NEG_ZERO) == NEG_ZERO ) {
            mem.setZ(k);
            bStep = false;
        }
        ret = 0;
    }
    return ret;
}

int CCpu::op2ex(void)
{
    int ret = -1;
    __uint16_t  q, x;
    mvprintw(19,0,"OP2EX");
    // The "Transfer Control to Fixed" instruction jumps to a
    // memory location in fixed (as opposed to erasable) memory.
    switch( qc ) {
    case 01:
        q = mem.getQ();
        x = mem.read(k10);
        mem.write(k10,q);
        mem.setQ(x);
        mvprintw(18,0,"%5o <-> %05o", q, x);
        ret = 0;
        break;
    case 02:
        break;
    case 03:
        break;
    }
    return ret;
}


//******************************************************************

int CCpu::op0(void)
{
    int ret = -1;
    switch(opc) {
    case 000002:
        mem.setZ(mem.getQ());
        bStep = false;
        ret = 0;
        break;
    case 000003:
        bInterrupt = true;
        ret = 0;
        break;
    case 000004:
        bInterrupt = false;
        ret = 0;
        break;
    case 000006:
        bExtracode = true;
        ret = 0;
        break;
    default:
        mem.setQ(mem.getZ() + 1);   // Set return address
        mem.setZ(k12);
        bStep = false;
        ret = 0;
    }
    return ret;
}

#define IS_POS(x) (((x)&0x4000) == 0)
#define IS_NEG(x) (((x)&0x4000) != 0)

int CCpu::op1(void)
{
    int ret = -1;
    __uint16_t  m;
    switch( qc ) {
    case 00:
        // If (K) > 0, then we take the instruction at I + 1, and (A) will be reduced
        // by 1, i.e. (K) - 1. If (K) = + 0, we take the instruction at I + 2, and (A) will
        // be set to +O. If (K) < -0, we take the instruction at I + 3, and (A) will be set
        // to its absolute value less 1. If (K) = -0, we take the instruction at I + 4, and
        // (A) will be set to + 0. CCS always leaves a positive quantity in A. 
        m = mem.read(k10);
        if( m > 0 && IS_POS(m) ) {
            mem.setA(m-1);
        } else if( m == 0 ) {
            mem.setA(0);
            mem.setZ(mem.getZ() + 1);
        } else if( m == NEG_ZERO ) {
            mem.setA(0);
            mem.setZ(mem.getZ() + 3);
        } else {
            // Is negative ...
            mem.setA(((~mem.getA()) & 0x7FFF) - 1); // TBD
            mem.setZ(mem.getZ() + 2);
        }
        //bStep = true;
        break;
    default:
        // The "Transfer Control to Fixed" instruction jumps to a
        // memory location in fixed (as opposed to erasable) memory.
        mem.setZ(k12);
        bStep = false;
        ret = 0;
    }
    return ret;
}

int CCpu::op2(void)
{
    int ret = -1;
    __uint16_t a, l, x1, x2;
    switch( qc ) {
    case 00:
        // Double Add to Storage
        a = mem.getA();
        l = mem.read(01);
        x1 = mem.read12(k10);
        x2 = mem.read12(k10+1);
        // Add (A,L)+(X1,X2) and store at k10,k10+1
        // L = +0, A=(+1, -1 or +0)
        mem.write(01,0);
        ret = 0;
        break;
    case 01:
        l = mem.read(01);
        x1 = mem.read(k10);
        mem.write(k10,l);
        mem.write(01,x1);
        ret = 0;
        break;
    case 02:
        x1 = mem.read(k10);
        mem.write(k10,x1+1);
        ret = 0;
        break;
    case 03:
        a = mem.getA();
        x1 = mem.read(k10);
        a += x1;
        mem.setA(a);
        mem.write(k10,a);
        ret = 0;
        break;
    default:
        printf("Unknown opcode %05o!\n", opc);
    }
    return ret;
}

int CCpu::op3(void)
{
    // The "Clear and Add" (or "Clear and Add Erasable" or "Clear and Add Fixed") instruction moves
    // the contents of a memory location into the accumulator.
    mem.setA(mem.read12(k12));
    return 0;
}

int CCpu::op4(void)
{
    // The "Clear and Add" (or "Clear and Add Erasable" or "Clear and Add Fixed") instruction moves
    // the contents of a memory location into the accumulator.
    mem.setA((~mem.read12(k12)) & NEG_ZERO);
    s2 = s2 ? 0 : 0x4000;
    return 0;
}

int CCpu::op5(void)
{
    int ret = -1;
    __uint16_t a, l, x, x1;
    switch( qc ) {
    case 01: // swap [k-1,k] and [a,l]
        x = mem.read12(k10-1);
        x1 = mem.read12(k10);
        a = mem.read(0);
        l = mem.read(1);
        mem.write12(k10-1,a);
        mem.write12(k10,l);
        mem.write(0,x);
        mem.write(1,x1);
        ret = 0;
        break;
    case 02:
        // TS
        a = mem.read(0);
        if( OF() ) {
            mem.write(k10,(a & 0x3FFF) | s2);
            mem.setZ(mem.getZ() + 1);   // Overflow - skip one line!
            mem.setA(s2 ? NEG_ONE : POS_ONE);
        } else {
            mem.write(k10,a);
        }
        ret = 0;
        break;
    case 03:
        a = mem.read(0);
        x = mem.read12(k10);
        mem.write(0,x);
        mem.write12(k10,a);
        ret = 0;
        break;
    case 00:
        idx = mem.read(k10);
        ret = 0;
        break;
    default:
        printf("Unknown opcode %05o!\n", opc);
    }
    return ret;
}

int CCpu::op6(void)
{
    int ret = -1;
    __uint16_t m = mem.read12(k12);
    __uint16_t a = mem.getA();
    
    // AD - add and update overflow
    mem.write(0, add1st(a, m));
    // Note! Rewrite K!
    mem.write(k12, m);
    ret = 0;
    return ret;
}

int CCpu::op7(void)
{
    int ret = -1;
    __uint16_t m = mem.read12(k12);
    __uint16_t a = mem.getA();
    
    mem.write(0, a & m);
    ret = 0;
    return ret;
}
