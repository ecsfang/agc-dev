#include <stdio.h>
#include <cstdint>
#include "memory.h"


void CMemory::init(void) {
}

void CMemory::dump(void)
{
  fprintf(logFile,"\n=======================================");
  fprintf(logFile,"\n= Memory Dump =========================");
  fprintf(logFile,"\n=======================================\n");

  for(int x=0; x<04000; x+=8) {
    int blk = x / 0400;
    if( x && x % 0400 == 0 )
      fprintf(logFile,"\n");
    if( blk < 3 )
      fprintf(logFile,"\n         ");
    else
      fprintf(logFile,"\n[%o:%05o]", blk, (x%0400)+01400);
    fprintf(logFile," %05o:", x);
    for(int n=0; n<8; n++) {
      if( x==0 && (n == REG_A || n==REG_Q) )
        fprintf(logFile," %06o", readPys(x+n) & MASK_16_BITS);
      else
        fprintf(logFile,"  %05o", readPys(x+n) & MASK_15_BITS);
    }
  }
  fprintf(logFile,"\n\n= End of memory dump ==================\n");
  fflush(logFile);

}

uint16_t DABS(uint16_t x)
{
    if( IS_NEG(x) ) 
        x = (~x) & MASK_15_BITS;
    if( x > 1 ) {
        return x-1;
    }
    return 0;
}
uint16_t DABS16(uint16_t x)
{
    if( IS_NEG16(x) ) 
        x = (~x) & MASK_16_BITS;
    if( x > 1 ) {
        return x-1;
    }
    return 0;
}

int16_t ValueOverflowed (int16_t Word)
{
    switch (IS_OF(Word)) {
    case POS_OF: return POS_ONE;
    case NEG_OF: return NEG_ONE;
    }
    return POS_ZERO;
}
bool IsValueOverflowed(int16_t word)
{
    return ValueOverflowed(word) != POS_ZERO;
}

#if 0
// Replace S1 with the S2 bit
int16_t OverflowCorrected (int16_t Word)
{
    // Overflow correction from 16 to 15 bits
    return OVF_CORRECTION(Word);
}

// Sign-extend a 15-bit SP value so that it can go into the 16-bit (plus parity)
// accumulator.
int SignExtend (int Word) {
    return SIGN_EXTEND(Word);
}
#endif
