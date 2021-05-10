#include <stdio.h>
#include "cpu.h"

int CCpu::op0(void)
{
    // TC
    int ret = -1;
    switch(opc) {
/*    case 000002:    // RETURN
        mem.setZ(mem.getQ());
        mem.setQ(00003);
        ret = 0;
        break;*/
    case 000003:    // RELINT
        bInterrupt = true;
        ret = 0;
        mct = 1;
        break;
    case 000004:    // INHINT
        bInterrupt = false;
        ret = 0;
        mct = 1;
        break;
    case 000006:    // EXTEND
        bExtracode = true;
        ret = 0;
        mct = 1;
        break;
    default:        // TC (or TCR or XLQ or XXALQ)
        if( opc != 000002 ) // Not return ...
            mem.setQ( nextPC & MASK_16_BITS);   // Set return address
        // mem.setZ(k12);
        nextPC = k12 & MASK_16_BITS;
        ret = 0;
        mct = 1;
    }
    return ret;
}

uint16_t DABS(uint16_t x)
{
//    fprintf(logFile,"DABS(%05o) -> ", x);
    if( IS_NEG(x) ) 
        x = (~x) & 0x7FFF;
    if( x > 1 ) {
//        fprintf(logFile,"%05o\n", x);
        return x-1;
    }
//    fprintf(logFile,"%05o\n", 0);
    return 0;
}

int CCpu::op1(void)
{
    int ret = -1;
    __uint16_t  m;
    __uint16_t  jmp=0;
    switch( qc ) {
    case 00: // CCS
        // If (K) > 0, then we take the instruction at I + 1, and (A) will be reduced
        // by 1, i.e. (K) - 1. If (K) = + 0, we take the instruction at I + 2, and (A) will
        // be set to +O. If (K) < -0, we take the instruction at I + 3, and (A) will be set
        // to its absolute value less 1. If (K) = -0, we take the instruction at I + 4, and
        // (A) will be set to + 0. CCS always leaves a positive quantity in A. 
        m = mem.read(k10);
//        fprintf(logFile,"A: %05o - ", m);
        if( m > POS_ZERO && IS_POS(m) ) {
//            fprintf(logFile,"> +0");
            jmp = 0;
        } else if( m == POS_ZERO ) {
//            fprintf(logFile,"= +0");
            jmp = 1;
        } else if( (m&MASK_15_BITS) == NEG_ZERO ) {
//            fprintf(logFile,"= -0");
            jmp = 3;
        } else {
            // Is negative ...
//            fprintf(logFile,"< -0");
            jmp = 2;
        }
        setA( DABS(m) );
        nextPC += jmp;
        //mem.setZ(mem.getZ() + jmp);
        if( IS_EDIT_REG(k10) )
            mem.write(k10, m); // Update (k)!
        break;
    default: // TCF
        // The "Transfer Control to Fixed" instruction jumps to a
        // memory location in fixed (as opposed to erasable) memory.
        //mem.setZ(k12);
        nextPC = k12;
        mct = 1;
        ret = 0;
    }
    return ret;
}

int CCpu::op2(void)
{
    int ret = -1;
    __uint16_t a, l, x1, x2;
    int Lsw, Msw;
    switch( qc ) {
    case 00: //DAS
        // Double Add to Storage
        a = mem.getA();
        l = SignExtend(mem.getL());
        x1 = mem.read12(k10-1);
        x2 = mem.read12(k10);
        // Add (A,L)+(X1,X2) and store at k10,k10+1
        // L = +0, A=(+1, -1 or +0)
        ret = 0;

        if (k10 == 000001) { // DDOUBL
            fprintf(logFile,"DDOUBLE (a: %05o, l: %05o)\n", a, l);
            Lsw = AddSP16 (MASK_16_BITS & l, MASK_16_BITS & l);
            Msw = AddSP16 (a, a);
            fprintf(logFile,"(msw: %05o, lsw: %05o)\n", Msw, Lsw);
            if ((0140000 & Lsw) == 0040000)
                Msw = AddSP16 (Msw, POS_ONE);
            else if ((0140000 & Lsw) == 0100000)
                Msw = AddSP16 (Msw, SignExtend (NEG_ONE));
            Lsw = OverflowCorrected (Lsw);
            mem.setA( MASK_16_BITS & Msw );
            mem.setL( MASK_16_BITS & SignExtend (Lsw) );
            fprintf(logFile,"l = %05o\n", SignExtend (Lsw));
            break;
	    }
        fprintf(logFile,"DAS (a: %05o, l: %05o) + (%05o, %05o) -> [%05o]\n", a, l, x1, x2, k10);
        if( k10 < REG_EB )
            Lsw = AddSP16(MASK_16_BITS & l, MASK_16_BITS & x2);
        else
            Lsw = AddSP16(MASK_16_BITS & l, SignExtend(x2));
        if( (k10-1) < REG_EB )
            Msw = AddSP16(a, MASK_16_BITS & x1);
        else
            Msw = AddSP16(a, SignExtend(x1));

        fprintf(logFile,"(a+x1): %05o, (l+x2): %05o)\n", Msw, Lsw);
/*
DAS (a: 00003, l: 77775) + (37777, 140000) -> [01374]
(a+x1): 40002, (l+x2): 37776)
(msw: 40002, lsw: 37776)
*/
        if ((0140000 & Lsw) == 0040000)
            Msw = AddSP16(Msw, POS_ONE);
        else if ((0140000 & Lsw) == 0100000)
            Msw = AddSP16(Msw, SignExtend(NEG_ONE));
        Lsw = OverflowCorrected(Lsw);
        fprintf(logFile,"(msw: %05o, lsw: %05o)\n", Msw, Lsw);

        if ((0140000 & Msw) == 0100000)
            mem.setA(SignExtend(NEG_ONE));
        else if ((0140000 & Msw) == 0040000)
            mem.setA(POS_ONE);
        else
            mem.setA(POS_ZERO);
        mem.setL(POS_ZERO);
        // Save the results.
        if( k10 < 3 )
            mem.write(k10, SignExtend(Lsw));
        else
            mem.write12(k10, Lsw); //SignExtend(Lsw));
        if( (k10-1) < 3 )
            mem.write(k10-1, Msw);
        else
            mem.write12(k10-1, OverflowCorrected(Msw));
        bOF = false;
        mct = 3;
        break;
    case 01:    // LXCH
        l = mem.getL();
        x1 = mem.read(k10);
        mem.write12(k10,l);
        mem.setL(x1);
        ret = 0;
        break;
    case 02:    // INCR
        x1 = mem.read(k10);
        mem.write12(k10,x1+1);
        ret = 0;
        break;
    case 03: // ADS
        //a = mem.getA();
        //x1 = mem.read(k10);
        //a = AddSP16(a,x1);add1st
        if( k10 < REG_EB )
            a = add1st(mem.getA(), mem.read(k10));
        else
            a = add1st(mem.getA(), SignExtend(mem.read(k10)));
        setA(a);
        mem.write12(k10,a);
        ret = 0;
        break;
    default:
        fprintf(logFile,"Unknown opcode %05o!\n", opc);
        fflush(logFile);
    }
    return ret;
}

int CCpu::op3(void)
{
    // The "Clear and Add" (or "Clear and Add Erasable" or "Clear and Add Fixed") instruction moves
    // the contents of a memory location into the accumulator.
    uint16_t k = SignExtend(mem.read12(k12));
    setA(k);
    if( IS_EDIT_REG(k12) )
        mem.write(k12, k); // Update (K)!
    if( k12 != REG_A && k12 != REG_Q )
        bOF = false;
    return 0;
}

int CCpu::op4(void)
{
    // The "Clear and Add" (or "Clear and Add Erasable" or "Clear and Add Fixed") instruction moves
    // the contents of a memory location into the accumulator.
    uint16_t k = k12 < REG_EB ? mem.read12(k12) : SignExtend(mem.read12(k12));
    setA(~k);
//    setA(SignExtend((~k) & NEG_ZERO));
    if( IS_EDIT_REG(k12) )
        mem.update(k12); // Update (k)!
    if( k12 > REG_Q )
        bOF = false;
    return 0;
}


int CCpu::op5(void)
{
    int ret = -1;
    __uint16_t a, l, x, k;
    k = k10 - 1;
    switch( qc ) {
    case 01: // DXCH swap [k-1,k] and [a,l]
        switch( k ) {
        case REG_Q:
            a = mem.getA();
            mem.setA(mem.getQ());
            mem.setQ(a);
//            fprintf(logFile," DXCH Q -> Q:%05o\n", mem.getQ());
            break;
        case REG_L:
            a = mem.getA();
            l = mem.getL();
            mem.setA(mem.getQ());
            mem.setL(a);
            mem.setQ(l);
//            fprintf(logFile," DXCH L -> L:%05o\n", mem.getQ());
            break;
        default:
            // Upper word
            if( (k+1) < 3 ) {
                x = mem.read12(k+1);
                mem.write12(k+1,mem.getL());
                mem.setL(x);
            } else {
                x = SignExtend(mem.read12(k+1));
                mem.write12(k+1,mem.getL()); //OverflowCorrected(mem.getL()));
                mem.setL(x);
            }
//            fprintf(logFile," DXCH %04o -> L:%05o [%05o]\n", k10, mem.getL(), mem.read12(k10));

            // Lower word
            if( (k) < 3 ) {
                x = mem.read12(k);
                mem.write12(k,mem.getA());
                mem.setA(x);
           } else {
                x = SignExtend(mem.read12(k));
                mem.write12(k,OverflowCorrected(mem.getA()));
                mem.setA(x);
            }
            if( k == 4 || k == 5 )
                nextPC = mem.getZ();
        }
        bOF = false;
        ret = 0;
        mct = 3;
        break;
    case 02:
        // TS
        a = mem.read(0);
        switch( k10 ) {
        case 00000:
            if( OF() ) {
                //mem.setZ(mem.getZ() + 1);   // Overflow - skip one line!
                nextPC++;
            }
            break;
        case 00005: // Special case ... TCAA
            if( OF() ) {
                setA(POS_OVF() ? POS_ONE : NEG_ONE);
            }
            nextPC = ovf_corr(a);
            //mem.setZ(ovf_corr(a));
            break;
        default:
            if( OF() ) {
                mem.write12(k10,ovf_corr(a));
                //mem.setZ(mem.getZ() + 1);   // Overflow - skip one line!
                nextPC++;
//                fprintf(logFile,"TS OF:%d S2:%d (%d)\n", bOF, s2, POS_OVF());
                setA(POS_OVF() ? POS_ONE : NEG_ONE);
            } else {
                mem.write12(k10,a);
            }
        }
        bOF = false;
        ret = 0;
        break;
    case 03:    // XCH
        a = mem.read(0);
        x = mem.read12(k10);
        mem.write(0,x);
        mem.write12(k10,a);
        ret = 0;
        break;
    case 00:
        if( k12 == 00017 ) {
            // RESUME
            nextPC = mem.read(REG_ZRUPT);
            bIntRunning = false;
        } else {
            // INDEX
            idx = mem.read(k10);
            mem.write12(k10,idx);
        }
        ret = 0;
        break;
    default:
        fprintf(logFile,"Unknown opcode %05o!\n", opc);
        fflush(logFile);
    }
    return ret;
}

int CCpu::op6(void)
{
    int ret = -1;
    __uint16_t m = mem.read12(k12);
    __uint16_t a = mem.getA();
    
    // AD - add and update overflow
//    mem.write(0, add1st(SignExtend(a), SignExtend(m)));
    mem.write(0, add1st(a, SignExtend(m)));
    
    if( IS_EDIT_REG(k12) )
        mem.write(k12, m); // Update (K)!
    ret = 0;
    return ret;
}

int CCpu::op7(void)
{
    int ret = -1;
    __uint16_t m = mem.read12(k12);
    
    setA( OverflowCorrected(mem.getA()) );
    setA( SignExtend(mem.getA() & m) );

    ret = 0;
    return ret;
}
