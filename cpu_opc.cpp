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
        nextPC = k12 & MASK_16_BITS;
        ret = 0;
        mct = 1;
    }
    return ret;
}

int jmp1(__uint16_t k, __uint16_t adr)
{
    __int16_t   opr16;
    __uint16_t  jmp=0;

    if( adr < REG16 ) {
        adr &= MASK_16_BITS;
        opr16 = OverflowCorrected(k);
    } else {
        opr16 = k & MASK_15_BITS;
    }
    if( adr < REG16 && ValueOverflowed(k) == POS_ONE)
        jmp = 0;
    else if( adr < REG16 && ValueOverflowed(k) == NEG_ONE)
        jmp = 2;
    else if( opr16 == POS_ZERO )
        jmp = 1;
    else if( opr16 == NEG_ZERO )
        jmp = 3;
    else if( IS_NEG(opr16) )
        jmp = 2;
    return jmp;
}
int jmp2(__uint16_t k, __uint16_t adr)
{
    __int16_t   opr16;
    __uint16_t  jmp=0;

    if( adr < REG16 ) {
        adr &= MASK_16_BITS;
        opr16 = OverflowCorrected(k);
    } else {
        opr16 = k & MASK_15_BITS;
    }
    if( k > POS_ZERO && IS_POS(k) ) {
        jmp = 0;
    } else if( k == POS_ZERO ) {
        jmp = 1;
    } else if( (k&MASK_15_BITS) == NEG_ZERO ) {
        jmp = 3;
    } else {
        // Is negative ...
        jmp = 2;
    }
    return jmp;
}

int CCpu::op1(void)
{
    int ret = -1;
    __uint16_t  m;
    __int16_t   opr16;
    __uint16_t  jmp=0;
    switch( qc ) {
    case 00: // CCS
        // If (K) > 0, then we take the instruction at I + 1, and (A) will be reduced
        // by 1, i.e. (K) - 1. If (K) = + 0, we take the instruction at I + 2, and (A) will
        // be set to +O. If (K) < -0, we take the instruction at I + 3, and (A) will be set
        // to its absolute value less 1. If (K) = -0, we take the instruction at I + 4, and
        // (A) will be set to + 0. CCS always leaves a positive quantity in A. 
        m = mem.read12(k10);
        {
            __uint16_t A1, A2;
            char pcBuf[16];
            if( k10 < REG16 ) {
                m &= MASK_16_BITS;
                opr16 = OverflowCorrected(m);
                A1 = DABS16(m);
            } else {
                opr16 = m & MASK_15_BITS;
                A1 = DABS(opr16);
            }
            A2 = DABS(m);
            if( (jmp1(m,k10) != jmp2(m,k10)) || A1 != A2 ) {
                getPC(pcBuf);
                if( bFileLogging ) {
                    fprintf(logFile,"\n%s CCS -> A[%05o]: %05o (%d : %d) A: %05o/%05o\n", pcBuf, k10, m, jmp1(m,k10),jmp2(m,k10), A1, A2);
                    fflush(logFile);
                }
            }
        }
#if 1
        if( k10 < REG16 ) {
            m &= MASK_16_BITS;
            opr16 = OverflowCorrected(m);
            setA( DABS(m) );
        } else {
            opr16 = m & MASK_15_BITS;
            setA( DABS(opr16) );
        }
        if( k10 < REG16 && ValueOverflowed(m) == POS_ONE)
            jmp = 0;
        else if( k10 < REG16 && ValueOverflowed(m) == NEG_ONE)
            jmp = 2;
        else if( opr16 == POS_ZERO )
            jmp = 1;
        else if( opr16 == NEG_ZERO )
            jmp = 3;
        else if( IS_NEG(opr16) )
            jmp = 2;

#else
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
#endif
        nextPC += jmp;
//        fprintf(logFile," PC: %04o\n", nextPC);
        if( IS_EDIT_REG(k10) )
            mem.update(k10); // Update (k)!
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
    switch( qc ) {
    case 00: //DAS
        // Double Add to Storage
        a = mem.getA() & MASK_16_BITS;
        l = SignExtend(mem.getL()) & MASK_16_BITS;
        x1 = mem.read12(mr1) & MASK_16_BITS;
        x2 = mem.read12(mr2) & MASK_16_BITS;
        // Add (A,L)+(X1,X2) and store at k10,k10+1
        // L = +0, A=(+1, -1 or +0)
        // Exception
        ret = 0;

        Msw = AddSP16 (a, mr1 == REG_A ? a : (mr1 < REG_EB ? x1 : SignExtend(x1)));
        Lsw = AddSP16 (l, mr2 == REG_L ? l : (mr2 < REG_EB ? x2 : SignExtend(x2)));

        switch( IS_OF(Lsw)) {
        case POS_OF: Msw = AddSP16(Msw, POS_ONE);             break;
        case NEG_OF: Msw = AddSP16(Msw, SignExtend(NEG_ONE)); break;
        }
        Lsw = OverflowCorrected(Lsw);

        if (mr2 == REG_L) { // DDOUBL (A+L)
            if( IS_X_LOGGING ) {
                fprintf(logFile,"DDOUBLE (a: %05o, l: %05o)\n", a, l);
                fprintf(logFile,"(msw: %05o, lsw: %05o)\n", Msw, Lsw);
            }
            mem.setA( MASK_16_BITS & Msw );
            mem.setL( MASK_16_BITS & SignExtend (Lsw) );

            if( IS_X_LOGGING )
                fprintf(logFile,"l = %05o\n", SignExtend (Lsw));
	    } else {
            if( IS_X_LOGGING ) {
                fprintf(logFile,"DAS (a: %05o, l: %05o) + (%05o, %05o) -> [%05o]\n", a, l, x1, x2, k10);
                fprintf(logFile,"(a+x1): %05o, (l+x2): %05o)\n", Msw, Lsw);
            }
    /*
    DAS (a: 00003, l: 77775) + (37777, 140000) -> [01374]
    (a+x1): 40002, (l+x2): 37776)
    (msw: 40002, lsw: 37776)
    */

            if( IS_X_LOGGING )
                fprintf(logFile,"(msw: %05o, lsw: %05o)\n", Msw, Lsw);

            // After the addition, the L register is set to +0, and the A register
            // is set to +1, -1, or +0, depending on whether there had been positive
            // overflow, negative overflow, or no overflow during the addition.
            switch(IS_OF(Msw)) {
            case POS_OF: mem.setA(POS_ONE);             break;
            case NEG_OF: mem.setA(SignExtend(NEG_ONE)); break;
            default:      mem.setA(POS_ZERO);
            }
            mem.setL(POS_ZERO);

            // Save the results.
            // First register
            mem.write12(mr1, (mr1 < REG16) ? Msw : OverflowCorrected(Msw));
            // Second register
            mem.write12(mr2, (mr2 < REG16) ? SignExtend(Lsw) : Lsw);
        }
        bOF = false;
        mct = 3;
        break;
    case 01:    // LXCH
        l = mem.getL();
        x1 = mem.read12(k10);
        mem.write12(k10,l);
        mem.setL(x1);
        ret = 0;
        break;
    case 02:    // INCR
        mem.inc(k10);
        ret = 0;
        break;
    case 03: // ADS
        if( k10 < REG_EB ) {
            a = AddSP16(mem.getA(), mem.read12(k10));
            mem.write12(k10, a);
        } else {
            if( bFileLogging ) {
                fprintf(logFile,"A=%06o [%04o]=%06o ", mem.getA(), k10, SignExtend(mem.read12(k10)));
            }
            a = AddSP16(mem.getA(), SignExtend(mem.read12(k10)));
            mem.write12(k10, OverflowCorrected(a));
            if( bFileLogging ) {
                fprintf(logFile,"--> A:%05o : %05o\n", a, mem.read12(k10));
                fflush(logFile);
            }
        }
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
    //uint16_t k = SignExtend(mem.read12(k12));
    uint16_t k = k12 < REG_EB ? mem.read12(k12) : SignExtend(mem.read12(k12));
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
    uint16_t k = k12 < REG_EB ? mem.read12(k12) : SignExtend(mem.read12(k12));
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
        switch( k ) {
        case REG_Q:
            a = mem.getA();
            mem.setA(mem.getQ());
            mem.setQ(a);
            break;
        case REG_L:
            a = mem.getA();
            l = mem.getL();
            mem.setA(mem.getQ());
            mem.setL(a);
            mem.setQ(l);
            break;
        default:
            // Upper word
            if( (k+1) < REG_EB ) {
                x = mem.read12(k+1);
                mem.write12(k+1,mem.getL());
                mem.setL(x);
            } else {
                x = SignExtend(mem.read12(k+1));
                mem.write12(k+1,mem.getL()); //OverflowCorrected(mem.getL()));
                mem.setL(x);
            }

            // Lower word
            if( (k) < REG_EB ) {
                x = mem.read12(k);
                mem.write12(k,mem.getA());
                mem.setA(x);
           } else {
                x = SignExtend(mem.read12(k));
                mem.write12(k,OverflowCorrected(mem.getA()));
                mem.setA(x);
            }
            if( k == REG_Z || (k+1) == REG_Z )
                nextPC = mem.getZ();
        }
        bOF = false;
        ret = 0;
        mct = 3;
        break;
    case 02:
        // TS
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
            }
            nextPC = ovf_corr(a);
            break;
        default:
            if( OF() ) {
                setA(SignExtend(POS_OVF() ? POS_ONE : NEG_ONE));
                nextPC++;
            }
//            mem.write12(k10,k10 < REG_EB ? a : ovf_corr(a));
            mem.write12(k10, ovf_corr(a));
        }
        bOF = false;
        ret = 0;
        break;
    case 03:    // XCH
        if( k10 == REG_A )
            break;
        a = mem.getA();
        x = mem.read12(k10);
        mem.setA(k10 < REG_EB ? x : SignExtend(x));
        mem.write12(k10,k10 < REG_EB ? a : ovf_corr(a));
        if( k10 == REG_Z )
            nextPC = a;
        ret = 0;
        break;
    case 00:
        if( k12 == 00017 ) {
            // RESUME
            nextPC = mem.read12(REG_ZRUPT);
            bIntRunning = false;
            intRunning = 0;
        } else {
            // INDEX
            idx = mem.read12(k10);
            // A side-effect of this instruction is that K is rewritten after its value is interrogated;
            // this means that if K is CYR, SR, CYL, or EDOP, then it is re-edited.
            if( IS_EDIT_REG(k12) )
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
    
    // AD - add and update overflow
    mem.setA(AddSP16(mem.getA(), k12 < REG_EB ? m : SignExtend(m)));
    
    if( IS_EDIT_REG(k12) )
        mem.update(k12); // Update (K)!
    ret = 0;
    return ret;
}

int CCpu::op7(void)
{
    int ret = -1;
    __uint16_t m = mem.read12(k12);

    // MASK
    if( k12 < REG_EB ) {
        setA( mem.getA() & m );
    } else {
        __uint16_t a = OverflowCorrected(mem.getA());
        setA( SignExtend(a & m) );
	}
    ret = 0;
    return ret;
}
