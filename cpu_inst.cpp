#include <stdio.h>
#include "cpu.h"

#if 1
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
            fprintf(logFile, "BANK=%o [%06o]\n", bank, addr);
        }
        if( buf[0] >= '0' && buf[0] < '8') {
            char *p = buf;
            unsigned int word;
            p = strtok(buf, ", ");
            while( p && sscanf(p, "%o", &word) == 1 ) {
                fprintf(logFile, "%02o,%04o [%06o] : %05o\n", bank, addr-(010000 + bank * FIXED_BLK_SIZE), addr, word);
                p = strtok(NULL, ", "); //+= 6;
                mem.write(addr++, word);
            }
//            printf("\n");
        }
    }
    mem.setFB(020 * FIXED_BLK_SIZE);
    mem.setZ(02070);
    fprintf(logFile, "Start: [%06o](%06o) : %05o\n", mem.getZ(), mem.addr2mem(mem.getZ()), mem.getOP());
    fclose(fp);
}
#else
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
        fprintf(logFile, "[%05o %05o] %05o %05o\n", word >> 1, word2, word, word2);
        word2 = word;
        mem.write(addr, w & 077777);
        addr++;
    }
//    mem.setFB(0); //020 * FIXED_BLK_SIZE);
//    mem.setZ(04000);
    mem.setFB(020 * FIXED_BLK_SIZE);
    mem.setZ(02070);
    fprintf(logFile, "Start: [%06o](%06o) : %05o\n", mem.getZ(), mem.addr2mem(mem.getZ()), mem.getOP());
    fflush(logFile);
    fclose(fp);
}
#endif

int CCpu::op0ex(void)
{
    int ret = -1;
    __uint16_t kc = opc & MASK_IO_ADDRESS;
    __uint16_t io;
    switch (opc & 077000) {
    case 000000:
        io = mem.readIO(kc);
        setA(io);
        ret = 0;
        break;
    case 001000:
        mem.writeIO(kc, mem.read(0));
        ret = 0;
        break;
    case 002000:
        io = mem.readIO(kc);
        setA(io & mem.read(0));
        ret = 0;
        break;
    case 003000:
        io = mem.readIO(kc);
        setA(io & mem.read(0));
        mem.writeIO(kc, mem.read(0));
        ret = 0;
        break;
    case 004000:
        io = mem.readIO(kc);
        setA(io | mem.read(0));
        ret = 0;
        break;
    case 005000:
        io = mem.readIO(kc);
        setA(io | mem.read(0));
        mem.writeIO(kc, mem.read(0));
        ret = 0;
        break;
    case 006000:
        io = mem.readIO(kc);
        setA(io ^ mem.read(0));
        mem.writeIO(kc, mem.read(0));
        ret = 0;
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
        SimulateDV(mem.getA(), mem.getL(), k12);
        break;
    default:
        // BZF K
        if( mem.getA() == 0 || (mem.getA()&NEG_ZERO) == NEG_ZERO ) {
            mem.setZ(k12);
            bStep = false;
        }
        ret = 0;
    }
    return ret;
}

int CCpu::op2ex(void)
{
    int ret = -1;
    __uint16_t  a, q, x;
    fprintf(logFile,"OP2EX ");
    fflush(logFile);
    // The "Transfer Control to Fixed" instruction jumps to a
    // memory location in fixed (as opposed to erasable) memory.
    switch( qc ) {
    case 00:
        // MSU - Modular subtraction
        a = mem.getA();
        x = mem.read(k10);
        mem.write(k10,x);   // Re-write k
        a = a-x;
        mem.setA(a);
        ret = 0;
        break;
    case 01:
        // QXCH
        q = mem.getQ();
        x = mem.read(k10);
        mem.write(k10, OverflowCorrected(q));
        mem.setQ(SignExtend(x));
        fprintf(logFile,"QXCH %05o <-> %05o\n", q, x);
        fflush(logFile);
        ret = 0;
        break;
    case 02:
        // AUG
        x = SignExtend(mem.read(k10));
        if( IS_POS(x) )
            x = AddSP16(x, POS_ONE);
        else
            x = AddSP16(x, NEG_ONE);
        bOF = ValueOverflowed(x) != POS_ZERO;
        mem.write(k10,OverflowCorrected(x));
        break;
    case 03:
        // DIM
        x = SignExtend(mem.read(k10));
        fprintf(logFile,"DIM: %05o --> ", x);
        if( IS_POS(x) && x != POS_ZERO )
            x = AddSP16(x, SignExtend(NEG_ONE));
        else if( IS_NEG(x) && (x&MASK_15_BITS) != NEG_ZERO )
            x = AddSP16(x, POS_ONE);
        fprintf(logFile,"%05o [%05o]\n", x, OverflowCorrected(x));
        bOF = ValueOverflowed(x) != POS_ZERO;
        mem.write(k10,k10 < REG_EB ? x : OverflowCorrected(x));
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
    switch( k12 ) {
    case 000002:
        // DCA L - Overlapping memory ... L is written before reading to A
        setA(k2);
        setL(k2);
        break;
    case 000003:
        // DCA Q - Move Q into A with all bits intact
        setA(k1);
        setL(k2);
        break;
    default:
        setA(SignExtend(k1));
        setL(SignExtend(k2));
    }
    if( IS_EDIT_REG(k12-1) )
        mem.write(k12-1, k1); // Update (K)!
    if( IS_EDIT_REG(k12) )
        mem.write(k12, k2); // Update (K)!
    return 0;
}

int CCpu::op4ex(void)
{
    // DCS - The "Double Clear and Subtract" instruction moves
    // the 1's-complement contents of a memory location into the accumulator.
    uint16_t k1 = mem.read12(k12-1);
    uint16_t k2 = mem.read12(k12);
    switch( k12 ) {
    case 000002:
        // DCS L - Overlapping memory ... L is written before reading to A
        setA(mem.getQ());
        setL((~k2) & NEG_ZERO);
        break;
    case 000003:
        // DCA Q - Move Q into A with all bits intact
        setA((~k1));
        setL((~k2));
        break;
    default:
        setA(SignExtend((~k1) & NEG_ZERO));
        setL(SignExtend((~k2) & NEG_ZERO));
    }
    if( IS_EDIT_REG(k12-1) )
        mem.write(k12-1, k1); // Update (K)!
    if( IS_EDIT_REG(k12) )
        mem.write(k12, k2); // Update (K)!
    return 0;
}

#define POS_OVF() (bOF && s2 == 0)
#define NEG_OVF() (bOF && s2 != 0)

int CCpu::op5ex(void)
{
    // INDEX (NDX)
    int ret = -1;
    idx = mem.read(k12);
    if( IS_EDIT_REG(k12) )
        mem.write(k12,idx);
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
    case 00:  // SU
        a = mem.getA();
        x = mem.read(k10);
        mem.setA(a-x);
        if( IS_EDIT_REG(k10) )
            mem.write(k10, x); // Update (k)!
        break;
    default:  // BZMF
        a = mem.getA();
        if( POS_OVF() ) {
            // Not zero - don't jump!
        } else if( a == POS_ZERO  || IS_NEG(a) ) {
            mem.setZ(k12);
            bStep = false;
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
    return 0;
}

//******************************************************************

int CCpu::op0(void)
{
    // TC
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

uint16_t DABS(uint16_t x)
{
    fprintf(logFile,"DABS(%05o) -> ", x);
    if( IS_NEG(x) ) 
        x = (~x) & 0x7FFF;
    if( x > 1 ) {
        fprintf(logFile,"%05o\n", x);
        return x-1;
    }
    fprintf(logFile,"%05o\n", 0);
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
        if( m > POS_ZERO && IS_POS(m) ) {
            jmp = 0;
        } else if( m == POS_ZERO ) {
            jmp = 1;
        } else if( (m&MASK_15_BITS) == NEG_ZERO ) {
            jmp = 3;
        } else {
            // Is negative ...
            jmp = 2;
        }
        setA( DABS(m) );
        mem.setZ(mem.getZ() + jmp);
        if( IS_EDIT_REG(k10) )
            mem.write(k10, m); // Update (k)!
        //bStep = true;
        break;
    default: // TCF
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
    int Lsw, Msw;
    switch( qc ) {
    case 00:
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
            mem.write(k10, Lsw); //SignExtend(Lsw));
        if( (k10-1) < 3 )
            mem.write(k10-1, Msw);
        else
            mem.write(k10-1, OverflowCorrected(Msw));
        break;
    case 01:
        l = mem.getL();
        x1 = mem.read(k10);
        mem.write(k10,l);
        mem.setL(x1);
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
        a = AddSP16(a,x1);
        setA(a);
        mem.write(k10,a);
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
    if( IS_EDIT_REG(k10) )
        mem.write(k12, k); // Update (K)!
    return 0;
}

int CCpu::op4(void)
{
    // The "Clear and Add" (or "Clear and Add Erasable" or "Clear and Add Fixed") instruction moves
    // the contents of a memory location into the accumulator.
    uint16_t k = SignExtend(mem.read12(k12));
    setA(SignExtend((~k) & NEG_ZERO));
    if( IS_EDIT_REG(k10) )
        mem.write(k10, k); // Update (k)!
    return 0;
}


int CCpu::op5(void)
{
    int ret = -1;
    __uint16_t a, l, x;
    switch( qc ) {
    case 01: // DXCH swap [k-1,k] and [a,l]
        if( k10 == REG_L ) {
            mem.setL(SignExtend(OverflowCorrected(mem.getL())));
            fprintf(logFile," DXCH L -> L:%05o\n", mem.getL());
        } else {
            // Upper word
            if( k10 < 3 ) {
                x = mem.read12(k10);
                mem.write12(k10,mem.getL());
                mem.setL(x);
            } else {
                x = SignExtend(mem.read12(k10));
                mem.write12(k10,mem.getL()); //OverflowCorrected(mem.getL()));
                mem.setL(x);
            }
//            mem.setL(SignExtend(OverflowCorrected(mem.getL())));
            fprintf(logFile," DXCH %04o -> L:%05o [%05o]\n", k10, mem.getL(), mem.read12(k10));

            // Lower word
            if( (k10-1) < 3 ) {
                x = mem.read12(k10-1);
                mem.write12(k10,mem.getA());
                mem.setA(x);
           } else {
                x = SignExtend(mem.read12(k10-1));
                mem.write12(k10-1,OverflowCorrected(mem.getA()));
                mem.setA(x);
            }
            fprintf(logFile," DXCH %04o -> A:%05o [%05o]\n", k10-1, mem.getA(), mem.read12(k10-1));
/*
            a = mem.read(0);
            l = mem.read(1);
            mem.write(0,x);
            mem.write(1,x1);

            }
            x = mem.read12(k10-1);
            x1 = mem.read12(k10);
            a = mem.read(0);
            l = mem.read(1);
            mem.write12(k10-1,a);
            mem.write12(k10,l);
            mem.write(0,x);
            mem.write(1,x1);
            */
        }
        ret = 0;
        break;
    case 02:
        // TS
        a = mem.read(0);
        switch( k12 ) {
        case 00000:
            if( OF() ) {
                mem.setZ(mem.getZ() + 1);   // Overflow - skip one line!
            }
            break;
        case 00005: // Special case ... TCAA
            if( OF() ) {
                setA(POS_OVF() ? POS_ONE : NEG_ONE);
            }
            mem.setZ(ovf_corr(a));
            break;
        default:
            if( OF() ) {
                mem.write(k10,ovf_corr(a));
                mem.setZ(mem.getZ() + 1);   // Overflow - skip one line!
                setA(POS_OVF() ? POS_ONE : NEG_ONE);
            } else {
                mem.write(k10,a);
            }
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
        mem.write(k10,idx);
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
    mem.write(0, add1st(a, SignExtend(m)));
    if( IS_EDIT_REG(k10) )
        mem.write(k12, m); // Update (K)!
    ret = 0;
    return ret;
}

int CCpu::op7(void)
{
    int ret = -1;
    __uint16_t m = mem.read12(k12);
    __uint16_t a = mem.getA();
    
    setA(a & m);
    ret = 0;
    return ret;
}

int CCpu::sst(void)
{
    int ret = -1;
    bStep = true;

    fprintf(logFile,"%s\n", disasm());

    __uint16_t op = getOP();
    bClrExtra = true;
    if( bExtracode ) {
        mvprintw(19,0,"EXTRA CODE!");
        switch( op & OPCODE_MASK ) {
            case 000000: ret = op0ex(); break;
            case 010000: ret = op1ex(); break;
            case 020000: ret = op2ex(); break;
            case 030000: ret = op3ex(); break;
            case 040000: ret = op4ex(); break;
            case 050000: ret = op5ex(); break;
            case 060000: ret = op6ex(); break;
            case 070000: ret = op7ex(); break;
//            default:
//                pDis += sprintf(disBuf+pDis, "Unknown extra code %05o!", op);
        }
        if( bClrExtra )
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
//                pDis += sprintf(disBuf+pDis, "Unknown opcode %05o!", op);
        }
    }
    if( bStep )
        mem.step();
    return ret;
}

//----------------------------------------------------------------------------
// This function implements a model of what happens in the actual AGC hardware
// during a divide -- but made a bit more readable / software-centric than the 
// actual register transfer level stuff. It should nevertheless give accurate
// results in all cases, including those that result in "total nonsense".
// If A, L, or Z are the divisor, it assumes that the unexpected transformations
// have already been applied to the "divisor" argument.
void CCpu::SimulateDV(uint16_t a, uint16_t l, uint16_t divisor)
{
    uint16_t dividend_sign = 0;
    uint16_t divisor_sign = 0;
    uint16_t remainder;
    uint16_t remainder_sign = 0;
    uint16_t quotient_sign = 0;
    uint16_t quotient = 0;
    uint16_t sum = 0;
    int i;

    // Assume A contains the sign of the dividend
    dividend_sign = a & 0100000;

    // Negate A if it was positive
    if (!dividend_sign)
      a = ~a;
    // If A is now -0, take the dividend sign from L
    if (a == 0177777)
      dividend_sign = l & 0100000;
    // Negate L if the dividend is negative.
    if (dividend_sign)
      l = ~l;

    // Add 40000 to L
    l = AddSP16(l, 040000);
    // If this did not cause positive overflow, add one to A
    if (ValueOverflowed(l) != POS_ONE)
      a = AddSP16(a, 1);
    // Initialize the remainder with the current value of A
    remainder = a;

    // Record the sign of the divisor, and then take its absolute value
    divisor_sign = divisor & 0100000;
    if (divisor_sign)
      divisor = ~divisor;
    // Initialize the quotient via a WYD on L (L's sign is placed in bits
    // 16 and 1, and L bits 14-1 are placed in bits 15-2).
    quotient_sign = l & 0100000;
    quotient = quotient_sign | ((l & 037777) << 1) | (quotient_sign >> 15);

    for (i = 0; i < 14; i++)
    {
        // Shift up the quotient
        quotient <<= 1;
        // Perform a WYD on the remainder
        remainder_sign = remainder & 0100000;
        remainder = remainder_sign | ((remainder & 037777) << 1);
        // The sign is only placed in bit 1 if the quotient's new bit 16 is 1
        if ((quotient & 0100000) == 0)
          remainder |= (remainder_sign >> 15);
        // Add the divisor to the remainder
        sum = AddSP16(remainder, divisor);
        if (sum & 0100000)
          {
            // If the resulting sum has its bit 16 set, OR a 1 onto the
            // quotient and take the sum as the new remainder
            quotient |= 1;
            remainder = sum;
          }
    }
    // Restore the proper quotient sign
    a = quotient_sign | (quotient & 077777);

    // The final value for A is negated if the dividend sign and the
    // divisor sign did not match
    mem.setA((dividend_sign != divisor_sign) ? ~a : a);
    // The final value for L is negated if the dividend was negative
    mem.setL((dividend_sign) ? remainder : ~remainder);
/*
    double f = btof(a, l);
    double div = btof(divisor, 0);
    fprintf(logFile, "\n  %05o %05o -> %lf", a, l, f);
    fprintf(logFile, "\n        %05o -> %lf", divisor, div);
//    double quo = f / div;
//    uint16_t r1, r2;
//    r1 = ftob(quo, &r2);
    fprintf(logFile, "\n---------------------\n/ %05o %05o", ra, rl); //, quo);
    fprintf(logFile, "\n");
    ***/
}
