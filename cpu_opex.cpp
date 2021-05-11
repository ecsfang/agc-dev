#include <stdio.h>
#include "cpu.h"

#define IS_L_OR_Q(x) (x == REG_L || x == REG_Q)
int CCpu::op0ex(void)
{
    int ret = -1;
    __uint16_t kc = opc & MASK_IO_ADDRESS;
    __uint16_t io;
    __uint16_t a = mem.getA();
    switch (opc & 077000) {
    case 000000:    // READ
        fprintf(logFile," I/O READ %03o\n", kc);
        if( IS_L_OR_Q(kc) )
            setA(mem.readIO(kc));
        else
            setA(SignExtend(mem.readIO(kc)));
        ret = 0;
        break;
    case 001000:    // WRITE
        fprintf(logFile," I/O WRITE %03o <-- %05o\n", kc, a);
        if( IS_L_OR_Q(kc) )
            mem.write12(kc, a);
        else
            mem.writeIO(kc, OverflowCorrected(a));
        ret = 0;
        break;
    case 002000:    // RAND
        fprintf(logFile," I/O RAND %03o\n", kc);
        if( IS_L_OR_Q(kc) )
            setA(a & mem.readIO(kc));
        else
            setA(SignExtend(OverflowCorrected(a) & mem.readIO(kc)));
        ret = 0;
        break;
    case 003000:    // WAND
        fprintf(logFile," I/O WAND %03o <-- %05o\n", kc, a);
        if( IS_L_OR_Q(kc) ) {
            io = a & mem.readIO(kc);
            setA(io);
            mem.writeIO(kc, io);
        } else {
            io = OverflowCorrected(a) & mem.readIO(kc);
            setA(SignExtend(io));
            mem.writeIO(kc, io);
        }
        ret = 0;
        break;
    case 004000:    // ROR
        fprintf(logFile," I/O ROR %03o\n", kc);
        if( IS_L_OR_Q(kc) )
            setA(a | mem.readIO(kc));
        else
            setA(SignExtend(OverflowCorrected(a) | mem.readIO(kc)));
        ret = 0;
        break;
    case 005000:    // WOR
        fprintf(logFile," I/O WOR %03o <-- %05o\n", kc, a);
        if( IS_L_OR_Q(kc) ) {
            io = a | mem.readIO(kc);
            setA(io);
            mem.writeIO(kc, io);
        } else {
            io = OverflowCorrected(a) | mem.readIO(kc);
            setA(SignExtend(io));
            mem.writeIO(kc, io);
        }
        ret = 0;
        break;
    case 006000:    // RXOR
        fprintf(logFile," I/O RXOR %03o\n", kc);
        if( IS_L_OR_Q(kc) )
            setA(a ^ mem.readIO(kc));
        else
            setA(SignExtend(OverflowCorrected(a) ^ mem.readIO(kc)));
        ret = 0;
        break;
    case 007000:    // EDRUPT
        mct = 3;
        break;
    default:
        fprintf(logFile,"Unknown opcode %05o!\n", opc);
        fflush(logFile);
    }
    return ret;
}

int CCpu::op1ex(void)
{
    int ret = -1;
    // The "Transfer Control to Fixed" instruction jumps to a
    // memory location in fixed (as opposed to erasable) memory.
    //    __uint16_t k = opc & MASK_12B_ADDRESS;
    switch (qc) {
    case 00:
        // Double divide
        //        pDis += sprintf(disBuf+pDis, "DV %04o", k12);
        //        SimulateDV(mem.getA(), mem.getL(), SignExtend(mem.read12(k12)));

        {
            int16_t AccPair[2], AbsA, AbsL, AbsK, Div16;
            int16_t Operand16;
            int Dividend, Divisor, Quotient, Remainder;

            AccPair[0] = OverflowCorrected(mem.getA());
            AccPair[1] = mem.getL();
            Dividend = SpToDecent(&AccPair[1]);
            DecentToSp(Dividend, &AccPair[1]);
            // Check boundary conditions.
            AbsA = AbsSP(AccPair[0]);
            AbsL = AbsSP(AccPair[1]);

            switch (k10) {
            case REG_A:
                // DV modifies A before reading the divisor, so in this
                // case the divisor is -|A|.
                Div16 = mem.getA();
                if ((Div16 & 0100000) == 0)
                    Div16 = 0177777 & ~Div16;
                break;
            case REG_L:
                // DV modifies L before reading the divisor. L is first
                // negated if the quotient A,L is negative according to
                // DV sign rules. Then, 40000 is added to it.
                Div16 = mem.getL();
                if (((AbsA == 0) && (0100000 & mem.getL())) || ((AbsA != 0) && (0100000 & mem.getA())))
                    Div16 = 0177777 & ~Div16;
                // Make sure to account for L's built-in overflow correction
                Div16 = SignExtend(OverflowCorrected(AddSP16((uint16_t)Div16, 040000)));
                break;
            case REG_Z:
                // DV modifies Z before reading the divisor. If the
                // quotient A,L is negative according to DV sign rules,
                // Z16 is set.
                Div16 = mem.getZ();
                if (((AbsA == 0) && (0100000 & mem.getZ())) || ((AbsA != 0) && (0100000 & mem.getA())))
                    Div16 |= 0100000;
                break;
            case REG_Q:
                Div16 = mem.getQ();
                break;
            default:
                Div16 = SignExtend(mem.read12(k10));
            }

            // Fetch the values;
            AbsK = AbsSP(OverflowCorrected(Div16));

            fprintf(logFile,"DIV: |a| = %05o |l| = %05o (%04o) = %05o\n",AbsA, AbsL, k10, AbsK);

            if (AbsA > AbsK || (AbsA == AbsK && AbsL != POS_ZERO) || ValueOverflowed(Div16) != POS_ZERO)
            {
                // The divisor is smaller than the dividend, or the divisor has
                // overflow. In both cases, we fall back on a slower simulation
                // of the hardware registers, which will produce "total nonsense"
                // (that nonetheless will match what the actual AGC would have gotten).
            fprintf(logFile,"Do SimulateDV()\n");
                SimulateDV(mem.getA(), mem.getL(), Div16);
                //  SimulateDV(State, Div16);
            }
            else if (AbsA == 0 && AbsL == 0)
            {
            fprintf(logFile,"Just zeros!\n");
                // The dividend is 0 but the divisor is not. The standard DV sign
                // convention applies to A, and L remains unchanged.
                if ((040000 & mem.getL()) == (040000 & OverflowCorrected(Div16)))
                {
                    if (AbsK == 0)
                        Operand16 = 037777; // Max positive value.
                    else
                        Operand16 = POS_ZERO;
                }
                else
                {
                    if (AbsK == 0)
                        Operand16 = (077777 & ~037777); // Max negative value.
                    else
                        Operand16 = NEG_ZERO;
                }

                mem.setA(SignExtend(Operand16));
            }
            else if (AbsA == AbsK && AbsL == POS_ZERO)
            {
            fprintf(logFile,"A==K, L == 0\n");
                // The divisor is equal to the dividend.
                if (AccPair[0] == OverflowCorrected(Div16)) // Signs agree?
                {
                    Operand16 = 037777; // Max positive value.
                }
                else
                {
                    Operand16 = (077777 & ~037777); // Max negative value.
                }
                mem.setL(SignExtend(AccPair[0]));
                mem.setA(SignExtend(Operand16));
            }
            else
            {
                // The divisor is larger than the dividend.  Okay to actually divide!
                // Fortunately, the sign conventions agree with those of the normal
                // C operators / and %, so all we need to do is to convert the
                // 1's-complement values to native CPU format to do the division,
                // and then convert back afterward.  Incidentally, we know we
                // aren't dividing by zero, since we know that the divisor is
                // greater (in magnitude) than the dividend.
            fprintf(logFile,"Divisor larger than dividend!\n");
                Dividend = agc2cpu2(Dividend);
                Divisor = agc2cpu(OverflowCorrected(Div16));
                Quotient = Dividend / Divisor;
                Remainder = Dividend % Divisor;
                mem.setA(SignExtend(cpu2agc(Quotient)));
            fprintf(logFile,"QUO %05o REM %05o DIV %05o\n", Quotient, Remainder, Dividend);
                if (Remainder == 0)
                {
                    // In this case, we need to make an extra effort, because we
                    // might need -0 rather than +0.
                    if (Dividend >= 0)
                        mem.setL(POS_ZERO);
                    else
                        mem.setL(NEG_ZERO);
                }
                else
                    mem.setL(SignExtend(cpu2agc(Remainder)));
            }
        }
        {
        uint16_t a = mem.getA();
        uint16_t l = mem.getL();
        mem.setA(l);
        mem.setL(a);
        }
        bOF = false;
        ret = 0;
        mct = 6;
        break;
    default:
        // BZF K
        if (mem.getA() == 0 || (mem.getA() & NEG_ZERO) == NEG_ZERO)
        {
            nextPC = k12; //nextPC = k12; //mem.setZ(k12);
            mct = 1;
        }
        ret = 0;
    }
    return ret;
}

int CCpu::op2ex(void)
{
    int ret = -1;
    __uint16_t  a, q, x;
    // The "Transfer Control to Fixed" instruction jumps to a
    // memory location in fixed (as opposed to erasable) memory.
    switch (qc) {
    case 00:
        // MSU - Modular subtraction
        /*        a = mem.getA();
        x = mem.read12(k10);
        mem.write(k10,x);   // Re-write k
        a = a-x;
        mem.setA(a);
        bOF = false;
        ret = 0;
*/
        {
            unsigned ui, uj;
            int diff;

            if (k10 < REG_EB)
            {
                ui = 0177777 & mem.getA();
                uj = 0177777 & ~mem.read12(k10);
            }
            else
            {
                ui = (077777 & OverflowCorrected(mem.getA()));
                uj = (077777 & ~mem.read12(k10));
            }
            diff = ui + uj + 1; // Two's complement subtraction -- add the complement plus one
            // The AGC sign-extends the result from A15 to A16, then checks A16 to see if
            // one needs to be subtracted. We'll go in the opposite order, which also works
            if (diff & 040000)
            {
                diff |= 0100000; // Sign-extend A15 into A16
                diff--;          // Subtract one from the result
            }
            if (k10 == REG_Q)
                mem.setA(0177777 & diff);
            else
            {
                uint16_t Operand16 = (077777 & diff);
                mem.setA(SignExtend(Operand16));
            }
            if (IS_EDIT_REG(k10))
                mem.update(k10); // Update (K)!
        }
        bOF = false;
        ret = 0;
        break;
    case 01:
        // QXCH
        q = mem.getQ();
        x = mem.read12(k10);
        mem.write12(k10, OverflowCorrected(q));
        mem.setQ(SignExtend(x));
//        fprintf(logFile,"QXCH %05o <-> %05o\n", q, x);
//        fflush(logFile);
        ret = 0;
        break;
    case 02:
        // AUG
        x = SignExtend(mem.read12(k10));
//        fprintf(logFile," AUG: %05o --> ", x);
        if( IS_POS(x) )
            x = AddSP16(x, POS_ONE);
        else
            x = AddSP16(x, SignExtend(NEG_ONE));
        bOF |= ValueOverflowed(x) != POS_ZERO;
        mem.write12(k10,bOF ? OverflowCorrected(x) : x);
//        fprintf(logFile,"%c (%05o) %05o\n", bOF ? '*':' ', x, OverflowCorrected(x));
        ret = 0;
        break;
    case 03:
        // DIM
        x = SignExtend(mem.read12(k10));
//        fprintf(logFile,"DIM: %05o --> ", x);
        if( IS_POS(x) && x != POS_ZERO )
            x = AddSP16(x, SignExtend(NEG_ONE));
        else if( IS_NEG(x) && (x&MASK_15_BITS) != NEG_ZERO )
            x = AddSP16(x, POS_ONE);
//        fprintf(logFile,"%05o [%05o]\n", x, OverflowCorrected(x));
        bOF |= ValueOverflowed(x) != POS_ZERO;
        mem.write12(k10,k10 < REG_EB ? x : OverflowCorrected(x));
        break;
    }
    return ret;
}

int CCpu::op3ex(void)
{
    // DCA
    // The "Double Clear and Add" instruction moves
    // the contents of a memory location into the accumulator.
    uint16_t k1 = mem.read12(k12-1);
    uint16_t k2 = mem.read12(k12);
    switch( k12-1 ) {
    case REG_A: // 30001   [0,1] = [0,1]
        // DCA A - Overlapping memory ... ??
        //setA(k2);
        setL(SignExtend(OverflowCorrected(k2)));
        break;
    case REG_L: // 30002 [0,1] = [1,2]
        // DCA L - Overlapping memory ... L is written before reading to A
        setL(k2);
        setL(SignExtend(OverflowCorrected(mem.getL())));
        setA(mem.getL());
        break;
    case REG_Q: // 30003 [0,1] = [2,3]
        // DCA Q - Move Q into A with all bits intact and EB to L
        setA(k1);
        setL(SignExtend(OverflowCorrected(k2)));
        break;
    default:
        setA(SignExtend(k1));
        setL(SignExtend(k2));
    }
//    setL(SignExtend(OverflowCorrected(mem.getL())));
    if( IS_EDIT_REG(k12-1) )
        mem.update(k12-1); // Update (K)!
    if( IS_EDIT_REG(k12) )
        mem.update(k12); // Update (K)!
    bOF = false;
    mct = 3;
    return 0;
}

int CCpu::op4ex(void)
{
    // DCS - The "Double Clear and Subtract" instruction moves
    // the 1's-complement contents of a memory location into the accumulator.
    uint16_t k1 = mem.read12(k12-1);
    uint16_t k2 = mem.read12(k12);
    switch( k12-1 ) {
    case REG_A:
        // DCOM
        setA(~k1);
        setL(~k2);
        break;
    case REG_L:
        // DCS L - Overlapping memory ... L is written before reading to A
        setA(mem.getQ());
        setL((~k2));// & NEG_ZERO);
        break;
    case REG_Q:
        // DCS Q - Move Q into A with all bits intact
        setA((~k1));
        setL(SignExtend(~k2));
        break;
    default:
        setA(SignExtend((~k1) & NEG_ZERO));
        setL(SignExtend((~k2) & NEG_ZERO));
    }
    if( IS_EDIT_REG(k12-1) )
        mem.update(k12-1); // Update (K)!
    if( IS_EDIT_REG(k12) )
        mem.update(k12); // Update (K)!
    bOF = false;
    mct = 3;
    return 0;
}

int CCpu::op5ex(void)
{
    // INDEX (NDX)
    int ret = -1;
    idx = mem.read12(k12);
    if( IS_EDIT_REG(k12) )
        mem.write12(k12,idx);
    ret = 0;
    bClrExtra = false;
    return ret;
}

int CCpu::op6ex(void)
{
    int ret = -1;
    __uint16_t  a,x;
    // The "Transfer Control to Fixed" instruction jumps to a
    // memory location in fixed (as opposed to erasable) memory.
    switch( qc ) {
    case 00: // SU
/*        a = mem.getA();
        x = mem.read12(k10);
        mem.setA(a - x);
        if (IS_EDIT_REG(k10))
            mem.update(k10); // Update (k)!
*/
        if (k10 == REG_A)
            mem.setA(SignExtend(NEG_ZERO));
        else //if (k10 < REG_EB)
            mem.setA(AddSP16(mem.getA(), 0177777 & ~mem.read12(k10)));
//        else
//            mem.setA(AddSP16(mem.getA(), 077777 & ~mem.read12(k10)));
        if (IS_EDIT_REG(k10))
            mem.update(k10); // Update (k)!
        break;
    default: // BZMF
        a = mem.getA();
        if( POS_OVF() ) {
            // Not zero - don't jump!
        } else if( a == POS_ZERO  || IS_NEG(a) ) {
            nextPC = k12; //mem.setZ(k12);
            mct = 1;
        }
        ret = 0;
        break;
    }
    return ret;
}


int CCpu::op7ex(void)
{
    // MP
/*    uint16_t  a,x;
    uint32_t  dp;
    a = mem.getA();
    x = SignExtend(mem.read12(k12));
    fprintf(logFile,"MP: A:%05o * K:%05o ", a, x);
    dp = a*x;
    fprintf(logFile,"--> (%05o : %05o)\n", dp>>15, dp &  MASK_15_BITS);
    mem.setA(dp>>15);
    mem.setL(dp&0x7FFF);
    return 0;*/
	{
	  // For MP A (i.e., SQUARE) the accumulator is NOT supposed to
	  // be overflow-corrected.  I do it anyway, since I don't know
	  // what it would mean to carry out the operation otherwise.
	  // Fix later if it causes a problem.
	  // FIX ME: Accumulator is overflow-corrected before SQUARE.
	  int16_t MsWord, LsWord, OtherOperand16;
	  int Product;
	  //WhereWord = FindMemoryWord (State, Address12);
	  int16_t Operand16 = OverflowCorrected (mem.getA());
	  if (k12 < REG_EB)
	    OtherOperand16 = OverflowCorrected (mem.read12(k12));
	  else
	    OtherOperand16 = mem.read12(k12);
	  if (OtherOperand16 == POS_ZERO || OtherOperand16 == NEG_ZERO)
	    MsWord = LsWord = POS_ZERO;
	  else if (Operand16 == POS_ZERO || Operand16 == NEG_ZERO)
	    {
	      if ((Operand16 == POS_ZERO && 0 != (040000 & OtherOperand16)) ||
		  (Operand16 == NEG_ZERO && 0 == (040000 & OtherOperand16)))
	      MsWord = LsWord = NEG_ZERO;
	      else
	      MsWord = LsWord = POS_ZERO;
	    }
	  else
	    {
	      int16_t WordPair[2];
	      Product =
	      agc2cpu (SignExtend (Operand16)) *
	      agc2cpu (SignExtend (OtherOperand16));
	      Product = cpu2agc2 (Product);
	      // Sign-extend, because it's needed for DecentToSp.
	      if (02000000000 & Product)
	      Product |= 004000000000;
	      // Convert back to DP.
	      DecentToSp (Product, &WordPair[1]);
	      MsWord = WordPair[0];
	      LsWord = WordPair[1];
	    }
	    setA(SignExtend (MsWord));
	    setL(SignExtend (LsWord));
	}
    bOF = false;
    mct = 3;
    return 0;
}
