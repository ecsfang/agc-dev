#include <stdio.h>
#include "cpu.h"

int CCpu::op0(void)
{
    // TC
    int ret = -1;
    switch(opc) {
    case 000003:    // RELINT
        bInterrupt = true;
        break;
    case 000004:    // INHINT
        bInterrupt = false;
        break;
    case 000006:    // EXTEND
        bExtracode = true;
        break;
    default:        // TC (or TCR or XLQ or XXALQ)
        if( opc != 000002 ) // Not return ...
            mem.setQ( nextPC & MASK_16_BITS);   // Set return address
        nextPC = k12 & MASK_16_BITS;
    }
    mct = 1;
    return 0;
}

int CCpu::op1(void)
{
    int ret = -1;
    __uint16_t  m;
    __uint16_t  opr16;
    __uint16_t  jmp=0;
    switch( qc ) {
    case 00: // CCS
        // If (K) > 0, then we take the instruction at I + 1, and (A) will be reduced
        // by 1, i.e. (K) - 1. If (K) = + 0, we take the instruction at I + 2, and (A) will
        // be set to +O. If (K) < -0, we take the instruction at I + 3, and (A) will be set
        // to its absolute value less 1. If (K) = -0, we take the instruction at I + 4, and
        // (A) will be set to + 0. CCS always leaves a positive quantity in A. 
        m = mem.read12(k10);
        if( k10 < REG16 ) {
            m &= MASK_16_BITS;
            opr16 = OverflowCorrected(m);
            setA( DABS16(m) );
        } else {
            opr16 = m & MASK_15_BITS;
            setA( DABS(opr16) );
        }
        if( k10 < REG16 && ValueOverflowed(m) == POS_ONE) {
            // Positive overflow ...
            jmp = 0;
        } else if( k10 < REG16 && ValueOverflowed(m) == NEG_ONE) {
            // Negative overflow ...
            jmp = 2;
        } else if( opr16 == POS_ZERO ) {
            // Positive zero ...
            jmp = 1;
        } else if( opr16 == NEG_ZERO ) {
            // Negative zero ...
            jmp = 3;
        } else if( IS_NEG(opr16) ) {
            // Negative value ...
            jmp = 2;
        } else {
            // Positive value ...
            jmp = 0;
        }

        nextPC += jmp;
        if( IS_EDIT_REG(k10) )
            mem.update(k10); // Update (k)!
        fflush(logFile);
        break;
    default: // TCF
        // The "Transfer Control to Fixed" instruction jumps to a
        // memory location in fixed (as opposed to erasable) memory.
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
    uint16_t    mr1 = k10-1;
    uint16_t    mr2 = k10;
    uint16_t k = mem.read12ex(k10); //k10 < REG_EB ? mem.read12(k10) : SignExtend(mem.read12(k10));
    switch( qc ) {
    case 00: //DAS
        // Double Add to Storage
        a = mem.getA() & MASK_16_BITS;
        l = SignExtend(mem.getL()) & MASK_16_BITS;
        x1 = mem.read12ex(mr1) & MASK_16_BITS;
        x2 = mem.read12ex(mr2) & MASK_16_BITS;
        // Add (A,L)+(X1,X2) and store at k10,k10+1
        // L = +0, A=(+1, -1 or +0)
        // Exception
        ret = 0;

        Msw = AddSP16 (a, mr1 == REG_A ? a : x1); //(mr1 < REG_EB ? x1 : SignExtend(x1)));
        Lsw = AddSP16 (l, mr2 == REG_L ? l : x2); //(mr2 < REG_EB ? x2 : SignExtend(x2)));

        switch( IS_OF(Lsw)) {
        case POS_OF: Msw = AddSP16(Msw, POS_ONE);             break;
        case NEG_OF: Msw = AddSP16(Msw, SignExtend(NEG_ONE)); break;
        }
        Lsw = SignExtend(OverflowCorrected(Lsw)); // Force to 15 bits word

        if (mr2 == REG_L) { // DDOUBL (A+L)
            mem.setA( MASK_16_BITS & Msw );
            mem.setL( MASK_15_BITS & Lsw );
	    } else {
            // After the addition, the L register is set to +0, and the A register
            // is set to +1, -1, or +0, depending on whether there had been positive
            // overflow, negative overflow, or no overflow during the addition.
            switch(IS_OF(Msw)) {
            case POS_OF:    mem.setA(POS_ONE);             break;
            case NEG_OF:    mem.setA(SignExtend(NEG_ONE)); break;
            default:        mem.setA(POS_ZERO);
            }
            mem.setL(POS_ZERO);
            // Save the results.
            // First register
            mem.write12ex(mr1, Msw);
            // Second register
            mem.write12ex(mr2, Lsw);
        }
        bOF = false;
        mct = 3;
        break;
    case 01:    // LXCH
        switch( k10 ) {
        case REG_L:
            break;
        case REG_ZERO:
            mem.setL(POS_ZERO);
            break;
        default:
            l = mem.getL();
            x1 = mem.read12ex(k10);
            mem.write12ex(k10,l);
            mem.setL(x1);
        }
        ret = 0;
        break;
    case 02:    // INCR
        x1 = mem.read12ex(k10);
        x1 = AddSP16(POS_ONE, 0177777 & x1);
        mem.write12ex(k10, x1);
        if( k10 >= REG16 ) {
            bOF |= IsValueOverflowed(x1);
            if( bOF && k10 < REG_TIME6 ) {
                chkInterrupt(k10);
            }
        }
        ret = 0;
        break;
    case 03: // ADS
        a = add1st(mem.getA(), k);
        mem.write12ex(k10, a);
        setA(a);
        ret = 0;
        break;
    default:
        if( bFileLogging ) {
            fprintf(logFile,"Unknown opcode %05o!\n", opc);
            fflush(logFile);
        }
    }
    return ret;
}

int CCpu::op3(void)
{
    // CA CAE CAF
    // The "Clear and Add" (or "Clear and Add Erasable" or "Clear and Add Fixed") instruction moves
    // the contents of a memory location into the accumulator.
    uint16_t k = mem.read12ex(k12);
    setA(k);
    if( IS_EDIT_REG(k12) )
        mem.update(k12); // Update (K)!
    if( k12 != REG_A && k12 != REG_Q )
        bOF = false;
    return 0;
}

int CCpu::op4(void)
{
    // CS COM
    // The "Clear and Subtract" instruction moves the 1's-complement
    // (i.e., the negative) of a memory location into the accumulator..
    uint16_t k = mem.read12ex(k12);
    setA(~k);
    if( IS_EDIT_REG(k12) )
        mem.update(k12); // Update (k)!
    if( k12 != REG_A && k12 != REG_Q )
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
        a = mem.getA();
        l = mem.getL();
        switch( k ) {
        case REG_Q:
            mem.setA(mem.getQ());
            mem.setQ(a);
            break;
        case REG_L:
            mem.setA(mem.getQ());
            mem.setL(a);
            mem.setQ(l);
            break;
        default:
            // Upper word
            x = mem.read12ex(k+1);
            mem.write12ex(k+1, l);
            mem.setL(x);

            // Lower word
            x = mem.read12ex(k);
            mem.write12ex(k, a);
            mem.setA(x);
            if( k == REG_Z || (k+1) == REG_Z )
                nextPC = mem.getZ();
        }
        bOF = false;
        ret = 0;
        mct = 3;
        break;
    case 02: // TS
        a = mem.getA();
        switch( k10 ) {
        case REG_A: // OVSK - Overflow Skip
            if( OF() ) {
                // Overflow - skip one line!
                nextPC++;
            }
            break;
        case REG_Z: // Special case ... TCAA
            if( OF() ) {
                setA(SignExtend(POS_OVF() ? POS_ONE : NEG_ONE));
                nextPC = OVF_CORRECTION(a);
            } else
                nextPC = a;
            break;
        default:
            if( OF() ) {
                setA(SignExtend(POS_OVF() ? POS_ONE : NEG_ONE));
                mem.write12(k10, OVF_CORRECTION(a));
                nextPC++;
            } else
                mem.write12(k10, a);
        }
        bOF = false;
        ret = 0;
        break;
    case 03:
        // XCH
        // The "Exchange A and K" instruction exchanges the value
        // in the A register with a value stored in erasable memory.
        if( k10 == REG_A )
            break;
        a = mem.getA();
        mem.setA( mem.read12ex(k10) );
        mem.write12ex( k10, a );
        if( k10 == REG_Z )
            nextPC = a;
        ret = 0;
        break;
    case 00:
        if( k12 == 00017 ) {
            // RESUME
            // Resume Interrupted Program.
            nextPC = mem.read12(REG_ZRUPT);
            clrInterrupt();
        } else {
            // INDEX
            // The "Index Next Instruction" or "Index Extracode Instruction"
            // instruction causes the next instruction to be executed in
            // a modified way from its actual representation in memory.
            index( mem.read12(k10) );
            // A side-effect of this instruction is that K is rewritten after its value is interrogated;
            // this means that if K is CYR, SR, CYL, or EDOP, then it is re-edited.
            if( IS_EDIT_REG(k10) )
                mem.update(k10);
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
    // AD
    // The "Add" instruction adds the contents
    // of a memory location into the accumulator.
    mem.setA(add1st(mem.getA(), mem.read12ex(k12)));
    
    if( IS_EDIT_REG(k12) )
        mem.update(k12); // Update (K)!

    return 0;
}

int CCpu::op7(void)
{
    __uint16_t m = mem.read12ex(k12);
    __uint16_t a = mem.getA();

    // MASK
    // The "Mask A by K" instruction logically ANDs the contents
    // of a memory location bitwise into the accumulator.
    if( k12 < REG16 ) {
        setA( a & m );
    } else {
        setA( SignExtend(OverflowCorrected(a) & m) );
	}
    // Note: CYR, SR, CYL, or EDOP are unchanged
    return 0;
}
