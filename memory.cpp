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

int16_t ValueOverflowed (int Value)
{
    switch (Value & (S1_MASK|S2_MASK)) {
    case S1_MASK:
        return POS_ONE;
    case S2_MASK:
        return NEG_ONE;
    default:
        return POS_ZERO;
    }
}

int16_t OverflowCorrected (int Value)
{
    return ((Value & MASK_14_BITS) | ((Value >> 1) & S1_MASK));
}

