#include <stdio.h>
#include "cpu.h"

typedef struct {
    const char *name;
    __uint16_t addr;
    __uint16_t mask;
} Reg_t;

Reg_t   regs[] = {
    { "A",  000, 0xFFFF },
    { "L",  001, 0x7FFF },
    { "Q",  002, 0xFFFF },
    { "EB", 003, 0x7FFF },
    { "FB", 004, 0x7FFF },
    { "Z",  005, 0x0FFF },
    { "BB", 006, 0x7FFF },
/*    { "zero", 007, 0x7FFF },
    { "ARUPT", 010, 0x7FFF },
    { "LRUPT", 011, 0x7FFF },
    { "QRUPT", 012, 0x7FFF },
    { "SAMPBB", 013, 0x7FFF },
    { "BB", 014, 0x7FFF },
    { "BB", 015, 0x7FFF },
    { "BB", 016, 0x7FFF },
    { "BB", 017, 0x7FFF },
    { "BB", 06, 0x7FFF },
    { "BB", 06, 0x7FFF },
    { "BB", 06, 0x7FFF },
    { "BB", 06, 0x7FFF },
    { "BB", 06, 0x7FFF },
    { "BB", 06, 0x7FFF },*/
    { NULL, 0, 0 }
};

void CCpu::dispReg(WINDOW *win)
{
    int y1, y = 0;
    getOP(false);
    for( ; regs[y].name; y++ ) {
        mvwprintw(win, y, 0, "%2s: %05o ", regs[y].name, mem.read(regs[y].addr));
    }
    y++;
    y1 = 0;
    mvwprintw(win, y1++, 15, "    OF: [%c]", OF() ? 'X' : ' ' );
    mvwprintw(win, y1++, 15, " INDEX: %04o", idx );
    mvwprintw(win, y1++, 15, "   OPC: %o", (opc & 070000) >> 12 );
    mvwprintw(win, y1++, 15, "    QC: %o", qc );
    mvwprintw(win, y1++, 15, "EXTEND: [%c]", bExtracode ? 'X' : ' ' );
//    mvwprintw(win, 2, 15, "  EMEM: %05o [%04o]", mem.read(k10), k10 );
//    mvwprintw(win, 2, 15, "  FMEM: %05o", mem.read(k12), k12 );

    mvwprintw(win, y++, 0, "[MEM10(k)] %04o [%05o] -> %05o", k10,  mem.addr2mem(k10), mem.read12(k10) );
    mvwprintw(win, y++, 0, "[MEM12(k)] %04o [%05o] -> %05o", k12,  mem.addr2mem(k12), mem.read12(k12) );
//    mvwprintw(win, 2, 0, "BB: %05o[fb:%o eb:%o] ", mem.read(6), (mem.read(6)>>10)&037, mem.read(6)&07);
//    mvwprintw(win, 3, 0, "EB: %05o(%o) ", mem.read(3), (mem.read(3)>>8)&07);
//    mvwprintw(win, 4, 0, "FB: %05o(%o) ", mem.read(4), (mem.read(4)>>10)&037);
}



void CCpu::dis0ex(void)
{
    __uint16_t kc = opc & MASK_IO_ADDRESS;
    switch(opc & 077000) {
        case 000000:
            pDis += sprintf(disBuf+pDis, "READ %03o", kc);
            break;
        case 001000:
            pDis += sprintf(disBuf+pDis, "WRITE %03o\n", kc);
            break;
        case 002000:
            pDis += sprintf(disBuf+pDis, "RAND %03o\n", kc);
            break;
        case 003000:
            pDis += sprintf(disBuf+pDis, "WAND %03o\n", kc);
            break;
        case 004000:
            pDis += sprintf(disBuf+pDis, "ROR %03o\n", kc);
            break;
        case 005000:
            pDis += sprintf(disBuf+pDis, "WOR %03o\n", kc);
            break;
        case 006000:
            pDis += sprintf(disBuf+pDis, "RXOR %03o\n", kc);
            break;
        default:
            pDis += sprintf(disBuf+pDis, "Unknown opcode %05o!\n", opc);
    }
}

void CCpu::dis1ex(void)
{
    // The "Transfer Control to Fixed" instruction jumps to a
    // memory location in fixed (as opposed to erasable) memory.
    if( k12 & 06000 == 0) {
        // Double divide
        pDis += sprintf(disBuf+pDis, "DV %05o\n", k12);
    } else {
        // BZF K
        pDis += sprintf(disBuf+pDis, "BZF %05o\n", k12);
    }
}

void CCpu::dis2ex(void)
{
    // The "Transfer Control to Fixed" instruction jumps to a
    // memory location in fixed (as opposed to erasable) memory.
    switch( qc ) {
    case 01:
        pDis += sprintf(disBuf+pDis, "QXCH %05o\n", k12);
        break;
    case 02:
        pDis += sprintf(disBuf+pDis, "AUG %05o\n", k12);
        break;
    case 03:
        pDis += sprintf(disBuf+pDis, "DIM %05o\n", k12);
        break;
    }
}


//******************************************************************

void CCpu::dis0(void)
{
    switch(opc) {
    case 000002:
        pDis += sprintf(disBuf+pDis, "RETURN\n");
        break;
    case 000003:
        pDis += sprintf(disBuf+pDis, "RELINT\n");
        break;
    case 000004:
        pDis += sprintf(disBuf+pDis, "INHINT\n");
        break;
    case 000006:
        pDis += sprintf(disBuf+pDis, "EXTEND\n");
        break;
    default:
        if( opc == 000000 )
            pDis += sprintf(disBuf+pDis, "XXALQ\n");
        else if( opc == 000001 )
            pDis += sprintf(disBuf+pDis, "XLQ\n");
        else
            pDis += sprintf(disBuf+pDis, "TC %05o\n", k12);
    }
}

void CCpu::dis1(void)
{
    switch( qc ) {
    case 00:
        // If (K) > 0, then we take the instruction at I + 1, and (A) will be reduced
        // by 1, i.e. (K) - 1. If (K) = + 0, we take the instruction at I + 2, and (A) will
        // be set to +O. If (K) < -0, we take the instruction at I + 3, and (A) will be set
        // to its absolute value less 1. If (K) = -0, we take the instruction at I + 4, and
        // (A) will be set to + 0. CCS always leaves a positive quantity in A. 
        pDis += sprintf(disBuf+pDis, "CCS %04o\n", k10);
        break;
    default:
        // The "Transfer Control to Fixed" instruction jumps to a
        // memory location in fixed (as opposed to erasable) memory.
        pDis += sprintf(disBuf+pDis, "TCF %05o\n", k12);
    }
}

void CCpu::dis2(void)
{
    switch( qc ) {
    case 00:
        // Double Add to Storage
        pDis += sprintf(disBuf+pDis, "DAS %04o! TBD!\n", k10);
        break;
    case 01:
        pDis += sprintf(disBuf+pDis, "LXCH %05o[%05o]\n", mem.addr2mem(k10), k10);
        break;
    case 02:
        pDis += sprintf(disBuf+pDis, "INCR %05o[%05o]\n", mem.addr2mem(k10), k10);
        break;
    case 03:
        pDis += sprintf(disBuf+pDis, "ADS %04o[%05o]\n", mem.addr2mem(k10), k10);
        break;
    default:
        pDis += sprintf(disBuf+pDis, "Unknown opcode %05o!\n", opc);
    }
}

void CCpu::dis3(void)
{
    // The "Clear and Add" (or "Clear and Add Erasable" or "Clear and Add Fixed") instruction moves
    // the contents of a memory location into the accumulator.
    if( qc == 0 )
        pDis += sprintf(disBuf+pDis, "CAE %04o\n", k12);
    else
        pDis += sprintf(disBuf+pDis, "CAF %05o\n", k12);
}

void CCpu::dis4(void)
{
    // The "Clear and Add" (or "Clear and Add Erasable" or "Clear and Add Fixed") instruction moves
    // the contents of a memory location into the accumulator.
    if( opc == 040000 )
        pDis += sprintf(disBuf+pDis, "COM\n");
    else
        pDis += sprintf(disBuf+pDis, "CS %05o\n", k12);
}

void CCpu::dis5(void)
{
    switch( qc ) {
    case 01: // swap [k-1,k] and [a,l]
        if( opc == 052005 )
            pDis += sprintf(disBuf+pDis, "DTCF\n");
        else if( opc == 052006 )
            pDis += sprintf(disBuf+pDis, "DTCB\n");
        else
            pDis += sprintf(disBuf+pDis, "DXCH %05o\n", k10-1);
        break;
    case 02:
        if( opc == 054000 )
            pDis += sprintf(disBuf+pDis, "OVSK\n");
        if( opc == 054005 )
            pDis += sprintf(disBuf+pDis, "TCAA\n");
        else
            pDis += sprintf(disBuf+pDis, "TS %04o\n", k10);
        break;
    case 03:
        pDis += sprintf(disBuf+pDis, "XCF %05o\n", k10);
        break;
    case 00:
        pDis += sprintf(disBuf+pDis, "INDEX %05o\n", k10);
        break;
    default:
        pDis += sprintf(disBuf+pDis, "Unknown opcode %05o!\n", opc);
    }
}

void CCpu::dis6(void)
{
    if( opc == 060000 )
        pDis += sprintf(disBuf+pDis, "DOUBLE\n");
    else
        pDis += sprintf(disBuf+pDis, "AD %05o\n", k12);
}

void CCpu::dis7(void)
{
    pDis += sprintf(disBuf+pDis, "MASK %05o\n", k12);
}

int CCpu::sst(void)
{
    int ret = -1;
    __uint16_t op = getOP();
    bStep = true;

    if( bExtracode ) {
        mvprintw(19,0,"EXTRA CODE!");
        switch( op & OPCODE_MASK ) {
            case 000000: ret = op0ex(); break;
            case 010000: ret = op1ex(); break;
            case 020000: ret = op2ex(); break;
//            case 030000: ret = op3ex(op); break;
//            case 050000: ret = op5ex(op); break;
//            default:
//                pDis += sprintf(disBuf+pDis, "Unknown extra code %05o!\n", op);
        }
        bExtracode = false;
    } else {
        switch( op & OPCODE_MASK ) {
            case 000000: ret = op0(); break;
            case 010000: ret = op1(); break;
            case 020000: ret = op2(); break;
            case 030000: ret = op3(); break;
            case 040000: ret = op4(); break;
            case 050000: ret = op5(); break;
            case 060000: ret = op6(); break;
            case 070000: ret = op7(); break;
//            default:
//                pDis += sprintf(disBuf+pDis, "Unknown opcode %05o!\n", op);
        }
    }
    if( bStep )
        mem.step();
    return ret;
}

char *CCpu::disasm(int offs)
{
    pDis = 0;
    getOP(false, offs);
    if( bExtracode && !offs) {
        pDis += sprintf(disBuf+pDis, "[%04o(%06o)] %05o # ", mem.getZ()+offs, mem.getPysZ()+offs, opc);
        switch( opc & OPCODE_MASK ) {
            case 000000: dis0ex(); break;
            case 010000: dis1ex(); break;
            case 020000: dis2ex(); break;
            default:
                pDis += sprintf(disBuf+pDis, "Unknown extra code %05o!\n", opc);
        }
    } else {
        if( offs )
            pDis += sprintf(disBuf+pDis, "               %05o   ", opc);
        else
            pDis += sprintf(disBuf+pDis, "[%04o(%06o)] %05o > ", mem.getZ()+offs, mem.getPysZ()+offs, opc);
        switch( opc & OPCODE_MASK ) {
            case 000000: dis0(); break;
            case 010000: dis1(); break;
            case 020000: dis2(); break;
            case 030000: dis3(); break;
            case 040000: dis4(); break;
            case 050000: dis5(); break;
            case 060000: dis6(); break;
            case 070000: dis7(); break;
            default:
                pDis += sprintf(disBuf+pDis, "Unknown opcode %05o!\n", opc);
        }
    }
    return disBuf;
}
