#include <stdio.h>
#include <memory.h>
#include "memory.h"


void CMemory::init(void) {
#if 0
    write(04000, 000004);
    write(04001, 034054);
    write(04002, 056006);
    write(04003, 012667);
//006302,000092:    4054  E3,1400                                  EBANK=   LST1                                  #  RESTART USES E0, E3
//006303,000093:    4054           12103        GOBB               BBCON    GOPROG      
    write(04054, 012103);
/*
008764,000212: 05,2667  E3,1400                                  EBANK=   LST1                                  
008765,000213: 05,2667           24320        GOPROG             INCR     REDOCTR                               #  ADVANCE RESTART COUNTER.
008766,000214: 
008767,000215: 05,2670           22002                           LXCH     Q                                     
008768,000216: 05,2671           00006                           EXTEND                                         
008769,000217: 05,2672           04007                           ROR      SUPERBNK                              
008770,000218: 05,2673           53433                           DXCH     RSBBQ                                 
008771,000219: 05,2674           31036                           CA       DSPTAB     +11D                       
008772,000220: 05,2675           74750                           MASK     BIT4                                  
008773,000221: 05,2676           00006                           EXTEND                                         
008774,000222: 05,2677           12703                           BZF      +4                                    
008775,000223: 05,2700           64746                           AD       BIT6                                  #  SET ERROR COUNTER ENABLE
008776,000224: 05,2701           00006                           EXTEND                                         
008777,000225: 05,2702           05012                           WOR      CHAN12                                #  ISS WAS IN COARS ALIGN SO GO BACK TO
008778,000226: 05,2703           03070        BUTTONS            TC       LIGHTSET        
*/
    writeBank(05,02667, 024320); //        GOPROG             INCR     REDOCTR                               #  ADVANCE RESTART COUNTER.
    writeBank(05,02670, 022002); //                           LXCH     Q                                     
    writeBank(05,02671, 000006); //                           EXTEND                                         
    writeBank(05,02672, 004007); //                           ROR      SUPERBNK                              
    writeBank(05,02673, 053433); //                           DXCH     RSBBQ                                 
    writeBank(05,02674, 031036); //                           CA       DSPTAB     +11D                       
    writeBank(05,02675, 074750); //                           MASK     BIT4                                  
    writeBank(05,02676, 000006); //                           EXTEND                                         
    writeBank(05,02677, 012703); //                           BZF      +4                                    
    writeBank(05,02700, 064746); //                           AD       BIT6                                  #  SET ERROR COUNTER ENABLE
    writeBank(05,02701, 000006); //                           EXTEND                                         
    writeBank(05,02702, 005012); //                           WOR      CHAN12                                #  ISS WAS IN COARS ALIGN SO GO BACK TO
    writeBank(05,02703, 003070); //        BUTTONS            TC       LIGHTSET

    writeBank(05,03070, 034747); //        LIGHTSET           CAF      BIT5                                  #  CHECK FOR MARK REJECT AND ERROR RESET
    writeBank(05,03071, 000006); //                           EXTEND                                         
    writeBank(05,03072, 002016); //                           RAND     NAVKEYIN                              
    writeBank(05,03073, 000006); //                           EXTEND                                         
    writeBank(05,03074, 013102); //                           BZF      NONAVKEY                              #  NO MARK REJECT
    writeBank(05,03075, 000006); //                           EXTEND                                         
    writeBank(05,03076, 000015); //                           READ     MNKEYIN                               #  CHECK IF KEYS 2M AND 5M ON
    writeBank(05,03077, 063361); //                           AD       -ELR                                  #  MAIN DSKY KEYCODE (BITS 1-5)
    writeBank(05,03100, 000006); //                           EXTEND                                         
    writeBank(05,03101, 013103); //                           BZF      +2                                    
    writeBank(05,03102, 000002); //        NONAVKEY           TC       Q                                     
    writeBank(05,03103, 003107); //                           TC       STARTSUB                              
    writeBank(05,03104, 012474); //                           TCF      DOFSTART                              
    writeBank(05,03105, 003107); //                 +3        TC       STARTSUB                              
    writeBank(05,03106, 012501); //                           TCF      DOFSTRT1                              #  DO FRESH START BUT DON'T TOUCH ENGINE    
#endif
}
void CMemory::writeBank(__uint8_t bank, __uint16_t addr, __uint16_t data) {
    __uint16_t _addr = bank * FIXED_BLK_SIZE + addr;
    write(_addr, data);
}
/*
int main(int argc, char *argv[])
{
    CMemory mem;

    printf("Address: %06o - %06o\n", 0, TOTAL_SIZE-1);

    mem.read(1);
    mem.read(01400);
    mem.setEB(02000);
    mem.read(01400);
    mem.setEB(03400);
    mem.read(01401);
    return 0;
}
*/