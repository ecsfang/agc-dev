#include <stdio.h>
#include "cpu.h"

typedef struct {
    const char *name;
    __uint16_t addr;
    __uint16_t mask;
} Reg_t;

Reg_t   regs[] = {
    { "A",  000, MASK_15_BITS },
    { "L",  001, MASK_15_BITS },
    { "Q",  002, MASK_15_BITS },
    { "EB", 003, MASK_15_BITS },
    { "FB", 004, MASK_15_BITS },
    { "Z",  005, MASK_12_BITS },
    { "BB", 006, MASK_15_BITS },
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
    wclear(win);
    uint16_t r;
    for( ; regs[y].name; y++ ) {
        r = mem.read(regs[y].addr);
        if( regs[y].addr < REG_Q ) {
            mvwprintw(win, y, 0, "%2s: %d:%05o ", regs[y].name, r & 0100000 ? 1 : 0, r & regs[y].mask);
        } else
            mvwprintw(win, y, 0, "%2s:   %05o ", regs[y].name, r & regs[y].mask);
    }
    y++;
    y1 = 0;
    mvwprintw(win, y1++, 15, "    OF: [%c]", OF() ? 'X' : ' ' );
    mvwprintw(win, y1++, 15, " INDEX: %04o", idx );
    mvwprintw(win, y1++, 15, "   OPC: %o", (opc & 070000) >> 12 );
    mvwprintw(win, y1++, 15, "    QC: %o", qc );
    mvwprintw(win, y1++, 15, "EXTEND: [%c]", bExtracode ? 'X' : ' ' );
    mvwprintw(win, y1++, 15, "   CYR: [%05o]", mem.read(CYR_REG));
    mvwprintw(win, y1++, 15, "    SR: [%05o]", mem.read(SR_REG));
    mvwprintw(win, y1++, 15, "   CYL: [%05o]", mem.read(CYL_REG));
    mvwprintw(win, y1++, 15, "  EDOP: [%05o]", mem.read(EDOP_REG));
//    mvwprintw(win, 2, 15, "  EMEM: %05o [%04o]", mem.read(k10), k10 );
//    mvwprintw(win, 2, 15, "  FMEM: %05o", mem.read(k12), k12 );

    y1 = 0;
    for( r=0; r<8; r++, y1++ ) {
        mvwprintw(win, y1, 40, "%05o: %05o", mwAddr+r, mem.read12(mwAddr+r) & MASK_15_BITS);
    }

    y+=2;

    if( k10 > 0 )
        mvwprintw(win, y, 0, "[MEM10(k)] %04o [%05o] -> %05o", k10-1,  mem.addr2mem(k10-1), mem.read12(k10-1) & MASK_15_BITS );
    y++;    
    mvwprintw(win, y++, 0, "[MEM10(k)] %04o [%05o] -> %05o", k10,  mem.addr2mem(k10), mem.read12(k10) & MASK_15_BITS );
    if( k12 > 0 )
        mvwprintw(win, y, 0, "[MEM12(k)] %04o [%05o] -> %05o", k12-1,  mem.addr2mem(k12-1), mem.read12(k12-1) & MASK_15_BITS );
    y++;
    mvwprintw(win, y++, 0, "[MEM12(k)] %04o [%05o] -> %05o", k12,  mem.addr2mem(k12), mem.read12(k12) & MASK_15_BITS );

    if( idx > 0 ) {
        mvwprintw(win, y++, 0, " IDX [%04o] -> [%05o] -> %05o", idx,  mem.addr2mem(k10+idx), mem.read12(k10+idx) );
        mvwprintw(win, y++, 0, " IDX [%04o] -> [%05o] -> %05o", idx,  mem.addr2mem(k12+idx), mem.read12(k12+idx) );
    }
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
            pDis += sprintf(disBuf+pDis, "WRITE %03o", kc);
            break;
        case 002000:
            pDis += sprintf(disBuf+pDis, "RAND %03o", kc);
            break;
        case 003000:
            pDis += sprintf(disBuf+pDis, "WAND %03o", kc);
            break;
        case 004000:
            pDis += sprintf(disBuf+pDis, "ROR %03o", kc);
            break;
        case 005000:
            pDis += sprintf(disBuf+pDis, "WOR %03o", kc);
            break;
        case 006000:
            pDis += sprintf(disBuf+pDis, "RXOR %03o", kc);
            break;
        default:
            pDis += sprintf(disBuf+pDis, "Unknown opcode %05o!", opc);
    }
}

void CCpu::dis1ex(void)
{
    // The "Transfer Control to Fixed" instruction jumps to a
    // memory location in fixed (as opposed to erasable) memory.
    switch( qc ) {
    case 00:
        // Double divide
        pDis += sprintf(disBuf+pDis, "DV %04o", k12);
        break;
    default:
        // BZF K
        pDis += sprintf(disBuf+pDis, "BZF %05o", k12);
    }
}

void CCpu::dis2ex(void)
{
    switch( qc ) {
    case 00:
        pDis += sprintf(disBuf+pDis, "MSU %04o", k10);
        break;
    case 01:
        pDis += sprintf(disBuf+pDis, "QXCH %05o", k12);
        break;
    case 02:
        pDis += sprintf(disBuf+pDis, "AUG %05o", k10);
        break;
    case 03:
        pDis += sprintf(disBuf+pDis, "DIM %04o", k10);
        break;
    }
}

void CCpu::dis3ex(void)
{
    // The "Double Clear and Add" (or "Clear and Add Erasable" or "Clear and Add Fixed") instruction moves
    // the contents of a memory location into the accumulator.
     pDis += sprintf(disBuf+pDis, "DCA %05o", k12);
}

void CCpu::dis4ex(void)
{
    // The "Double Clear and Subtract" (or "Clear and Subtract Erasable" or "Clear and Subtract Fixed") instruction moves
    // the contents of a memory location into the accumulator.
    pDis += sprintf(disBuf+pDis, "DCS %05o", k12);
}

void CCpu::dis5ex(void)
{
    pDis += sprintf(disBuf+pDis, "NDX %05o", k12);
}

void CCpu::dis6ex(void)
{
    switch( qc ) {
    case 00:
        pDis += sprintf(disBuf+pDis, "SU %04o", k10);
        break;
    default:
        pDis += sprintf(disBuf+pDis, "BZMF %05o", k12);
        break;
    }
}

void CCpu::dis7ex(void)
{
    pDis += sprintf(disBuf+pDis, "MP %05o", k12);
}

//******************************************************************

void CCpu::dis0(void)
{
    switch(opc) {
    case 000002:
        pDis += sprintf(disBuf+pDis, "RETURN");
        break;
    case 000003:
        pDis += sprintf(disBuf+pDis, "RELINT");
        break;
    case 000004:
        pDis += sprintf(disBuf+pDis, "INHINT");
        break;
    case 000006:
        pDis += sprintf(disBuf+pDis, "EXTEND");
        break;
    default:
        if( opc == 000000 )
            pDis += sprintf(disBuf+pDis, "XXALQ");
        else if( opc == 000001 )
            pDis += sprintf(disBuf+pDis, "XLQ");
        else
            pDis += sprintf(disBuf+pDis, "TC %05o", k12);
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
        pDis += sprintf(disBuf+pDis, "CCS %04o", k10);
        break;
    default:
        // The "Transfer Control to Fixed" instruction jumps to a
        // memory location in fixed (as opposed to erasable) memory.
        pDis += sprintf(disBuf+pDis, "TCF %05o", k12);
    }
}

void CCpu::dis2(void)
{
    switch( qc ) {
    case 00:
        // Double Add to Storage
        if( k10 == 000001 )
            pDis += sprintf(disBuf+pDis, "DDOUBLE");
        else
            pDis += sprintf(disBuf+pDis, "DAS %04o!", k10);
        break;
    case 01:
        pDis += sprintf(disBuf+pDis, "LXCH %05o[%05o]", mem.addr2mem(k10), k10);
        break;
    case 02:
        pDis += sprintf(disBuf+pDis, "INCR %05o[%05o]", mem.addr2mem(k10), k10);
        break;
    case 03:
        pDis += sprintf(disBuf+pDis, "ADS %04o[%05o]", mem.addr2mem(k10), k10);
        break;
    default:
        pDis += sprintf(disBuf+pDis, "Unknown opcode %05o!", opc);
    }
}

void CCpu::dis3(void)
{
    // The "Clear and Add" (or "Clear and Add Erasable" or "Clear and Add Fixed") instruction moves
    // the contents of a memory location into the accumulator.
    if( qc == 0 )
        pDis += sprintf(disBuf+pDis, "CAE %04o", k12);
    else
        pDis += sprintf(disBuf+pDis, "CAF %05o", k12);
}

void CCpu::dis4(void)
{
    // The "Clear and Subtract" (or "Clear and Subtract Erasable" or "Clear and Subtract Fixed") instruction moves
    // the contents of a memory location into the accumulator.
    if( opc == 040000 )
        pDis += sprintf(disBuf+pDis, "COM");
    else
        pDis += sprintf(disBuf+pDis, "CS %05o", k12);
}

void CCpu::dis5(void)
{
    switch( qc ) {
    case 01: // swap [k-1,k] and [a,l]
        if( opc == 052005 )
            pDis += sprintf(disBuf+pDis, "DTCF");
        else if( opc == 052006 )
            pDis += sprintf(disBuf+pDis, "DTCB");
        else
            pDis += sprintf(disBuf+pDis, "DXCH %05o", k10-1);
        break;
    case 02:
        switch(k10) {
            case 00000:
                pDis += sprintf(disBuf+pDis, "OVSK");
                break;
            case 00005:
                pDis += sprintf(disBuf+pDis, "TCAA");
                break;
            default:
                pDis += sprintf(disBuf+pDis, "TS %04o", k10);
        }
        break;
    case 03:
        pDis += sprintf(disBuf+pDis, "XCF %05o", k10);
        break;
    case 00:
        pDis += sprintf(disBuf+pDis, "INDEX %05o", k10);
        break;
    default:
        pDis += sprintf(disBuf+pDis, "Unknown opcode %05o!", opc);
    }
}

void CCpu::dis6(void)
{
    if( opc == 060000 )
        pDis += sprintf(disBuf+pDis, "DOUBLE");
    else
        pDis += sprintf(disBuf+pDis, "AD %05o", k12);
}

void CCpu::dis7(void)
{
    pDis += sprintf(disBuf+pDis, "MASK %05o", k12);
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
            case 030000: dis3ex(); break;
            case 040000: dis4ex(); break;
            case 050000: dis5ex(); break;
            case 060000: dis6ex(); break;
            case 070000: dis7ex(); break;
            default:
                pDis += sprintf(disBuf+pDis, "Unknown extra code %05o!", opc);
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
                pDis += sprintf(disBuf+pDis, "Unknown opcode %05o!", opc);
        }
    }
    return disBuf;
}
