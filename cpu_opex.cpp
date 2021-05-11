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
    switch( qc ) {
    case 00:
        // Double divide
//        pDis += sprintf(disBuf+pDis, "DV %04o", k12);
        SimulateDV(mem.getA(), mem.getL(), SignExtend(mem.read12(k12)));
        bOF = false;
        ret = 0;
        mct = 6;
        break;
    default:
        // BZF K
        if( mem.getA() == 0 || (mem.getA()&NEG_ZERO) == NEG_ZERO ) {
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
