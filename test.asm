# Definitions of various registers.
A           EQUALS    0                                     
L           EQUALS    1                                     #  L AND Q ARE BOTH CHANNELS AND REGISTERS.
Q           EQUALS    2
ARUPT       EQUALS    10
QRUPT       EQUALS    12
TIME3       EQUALS    26
NEWJOB      EQUALS    67          # Location checked by the Night Watchman.
OVFCNTR     EQUALS    00034       # overflow counter

            SETLOC    4000        # The interrupt-vector table.

            # Come here at power-up or GOJAM
            INHINT                # Disable interrupts for a moment.
            # Set up the TIME3 interrupt, T3RUPT.  TIME3 is a 15-bit
            # register at address 026, which automatically increments every
            # 10 ms, and a T3RUPT interrupt occurs when the timer
            # overflows.  Thus if it is initially loaded with 037774,
            # and overflows when it hits 040000, then it will
            # interrupt after 40 ms.
            CA        O37774
            TS        TIME3
#            TCF       STARTUP    # Go to your "real" code.
            TCF       goMAIN     # Go to your "test" code.

            RESUME    # T6RUPT
            NOOP
            NOOP
            NOOP

            RESUME    # T5RUPT
            NOOP
            NOOP
            NOOP

            DXCH      ARUPT       # T3RUPT
            EXTEND                # Back up A, L, and Q registers
            QXCH      QRUPT
            TCF       T3RUPT

            RESUME    # T4RUPT
            NOOP
            NOOP
            NOOP

            RESUME    # KEYRUPT1
            NOOP
            NOOP
            NOOP

            RESUME    # KEYRUPT2
            NOOP
            NOOP
            NOOP

            RESUME    # UPRUPT
            NOOP
            NOOP
            NOOP

            RESUME    # DOWNRUPT
            NOOP
            NOOP
            NOOP

            RESUME    # RADAR RUPT
            NOOP
            NOOP
            NOOP

            RESUME    # RUPT10
            NOOP
            NOOP
            NOOP

# The interrupt-service routine for the TIME3 interrupt every 40 ms. 
T3RUPT      CAF     O37774      # Schedule another TIME3 interrupt in 40 ms.
            TS      TIME3

            # Tickle NEWJOB to keep Night Watchman GOJAMs from happening.
            # You normally would NOT do this kind of thing in an interrupt-service
            # routine, because it would actually prevent you from detecting
            # true misbehavior in the main program.  If you're concerned about
            # that, just comment out the next instruction and instead sprinkle
            # your main code with "CS NEWJOB" instructions at strategic points.
            CS      NEWJOB

            # If you want to build in your own behavior, do it right here!

            # And resume the main program
            DXCH    ARUPT       # Restore A, L, and Q, and exit the interrupt
            EXTEND
            QXCH    QRUPT
            RESUME       


STARTUP     RELINT    # Reenable interrupts.

            # Do your own stuff here!

            # If you're all done, a nice but complex infinite loop that
            # won't trigger a TC TRAP GOJAM.
ALLDONE     CS      NEWJOB      # Tickle the Night Watchman
            TCF     ALLDONE
# Define any constants that are needed.
O37774      OCT     37774


goMAIN          INHINT                # disable interrupts

                TCR        begin

        # Test basic instructions.
#                TCR        chkTC
#                TCR        chkCCS
#                TCR        chkINDEX
#                TCR        chkXCH
#                TCR        chkCS
                TCR        chkTS
                TCR        chkAD
                TCR        chkMASK

        # Passed all tests.
                TCR        finish

fail            XCH       curtest        # load last passed test into A
                TS        curtest

end             TC        end        # finished, TC trap

        # ----------------------------------------------
        # INITIALIZE FOR START OF TESTING

begin           XCH       STRTcode
                TS        curtest        # set current test code to START
                RETURN

        # ----------------------------------------------
        # TEST TC INSTRUCTION SUBROUTINE
        # L:        TC        K
        # Verifies the following:
        # - Set C(Q) = TC L+1
        # - Take next instruction from K, and proceed from there.

START       EQUALS    000
TCtst       EQUALS    001        # TC check failed
CCStst      EQUALS    002        # CCS check failed
INDEXtst    EQUALS    003        # INDEX check failed
XCHtst      EQUALS    004        # XCH check failed
CStst       EQUALS    005        # CS check failed
TStst       EQUALS    006        # TS check failed
ADtst       EQUALS    007        # AD check failed
MASKtst     EQUALS    010        # MASK check failed
PASS        EQUALS    012345     # PASSED all checks

TCcode      ADRES    TCtst       # code for this test
CCScode     ADRES    CCStst      # code for this test
INDEXcode   ADRES    INDEXtst    # code for this test

#Qtest      OCT      TCret1     # expected return address

curtest     EQUALS  0132        # current test
savQ        EQUALS  0133
STRTcode    EQUALS  0134        #  START
        # CCS test
CCSk        EQUALS  0135
INDXval     EQUALS  0136

        # XCH test
        # pre-set in erasable memory because we don't
        # want to use XCH to initialize them prior to testing XCH.
XCHkP0      EQUALS  0140       # +0
XCHkM0      EQUALS  0141       # -0
XCHkalt1    EQUALS  0142       # %52525        # alternating bit pattern 1
XCHkalt2    EQUALS  0143       # %25252        # alternating bit pattern 2

        # TS test
TSk         EQUALS  0144 #       -0

        # AD test
ADk         EQUALS  0145 #       -0

Qtest       ADRES   TCret1

chkTC       XCH     Q
            TS      savQ       # save return address

            CAF     TCcode
            TS      curtest    # set current test code to this test

    # attempt a jump
            TC      TCjmp1     # make test jump
TCret1      TC      fail       # failed to jump

    # verify correct return address in Q
TCjmp1      CS      Q
            AD      Qtest      # put (-Q) + val2 in A
            CCS      A         # A = DABS
            TC      fail       # >0 (Q < Qtest)
            TC      fail       # +0 (never happens)
            TC      fail       # <0 (Q > Qtest)

    # passed the test
            XCH     savQ
            TS      Q        # restore return address
            RETURN
    # --------------

        # ----------------------------------------------
        # PASSED ALL TESTS!

PASScode        OCT        12345

finish          CAF        PASScode
                TS        curtest        # set current test code to PASS
                RETURN

        # ----------------------------------------------
        # TEST CCS INSTRUCTION SUBROUTINE
        # L:        CCS        K
        # Verifies the following:
        # - take next instruction from L+n and proceed from there, where:
        # -- n = 1 if C(K) > 0
        # -- n = 2 if C(K) = +0
        # -- n = 3 if C(K) < 0
        # -- n = 4 if C(K) = -0
        # - set C(A) = DABS[C(K)], where DABS (diminished abs value):
        # -- DABS(a) = abs(a) - 1,        if abs(a) > 1
        # -- DABS(a) = +0,                 if abs(a) <= 1

# CCScode         DS        CCStst        # code for this test
        # test values (K)
CCSkM2          OCT       -2
CCSkM1          OCT       -1
CCSkM0          OCT       -0
CCSkP0          OCT       +0
CCSkP1          OCT       +1
CCSkP2          OCT       +2

		# expected DABS values
CCSdM2          OCT       1         # for K=-2, DABS = +1
CCSdM1          OCT       0        # for K=-1, DABS = +0
CCSdM0          OCT       0        # for K=-0, DABS = +0
CCSdP0          OCT       0        # for K=+0, DABS = +0
CCSdP1          OCT       0        # for K=+1, DABS = +0
CCSdP2          OCT       1        # for K=+2, DABS = +1

chkCCS          XCH        Q
                TS        savQ        # save return address

                CAF       CCScode
                TS        curtest        # set current test code to this test

        # set K to -2 and execute CCS: 
        # check for correct branch
                CAF        CCSkM2        # set K = -2
                TS        CCSk
                CCS        CCSk        # A = DABS[C(K)]
                TC        fail        # K > 0
                TC        fail        # K= +0
                TC        lCCS1       # K < 0
                TC        fail        # K= -0
        # check for correct DABS in A (for K=-2, it should be 1)
lCCS1           COM                # 1's compliment of A
                AD        CCSdM2        # put (-A) + expected value in A
                CCS        A        # A = DABS
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # set K to -1 and execute CCS: 
        # check for correct branch
                CAF        CCSkM1        # set K = -1
                TS        CCSk
                CCS        CCSk        # A = DABS[C(K)]
                TC        fail        # K > 0
                TC        fail        # K= +0
                TC        lCCS2       # K < 0
                TC        fail        # K= -0
        # check for correct DABS in A (for K=-1, it should be +0)
lCCS2           COM                # 1's compliment of A
                AD        CCSdM1        # put (-A) + expected value in A
                CCS        A        # A = DABS
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # set K to -0 and execute CCS: 
        # check for correct branch
                CAF        CCSkM0        # set K = -0
                TS        CCSk
                CCS        CCSk        # A = DABS[C(K)]
                TC        fail        # K > 0
                TC        fail        # K= +0
                TC        fail        # K < 0
        # check for correct DABS in A (for K=-0, it should be +0)
                COM                # 1's compliment of A
                AD        CCSdM0        # put (-A) + expected value in A
                CCS        A        # A = DABS
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # set K to +0 and execute CCS: 
        # check for correct branch
                CAF        CCSkP0        # set K = +0
                TS        CCSk
                CCS        CCSk        # A = DABS[C(K)]
                TC        fail        # K > 0
                TC        lCCS3       # K= +0
                TC        fail        # K < 0
                TC        fail        # K= -0
        # check for correct DABS in A (for K=+0, it should be +0)
lCCS3           COM                # 1's compliment of A
                AD        CCSdP0        # put (-A) + expected value in A
                CCS        A        # A = DABS
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # set K to +1 and execute CCS: 
        # check for correct branch
                CAF        CCSkP1        # set K = +1
                TS        CCSk
                CCS        CCSk        # A = DABS[C(K)]
                TC        lCCS4        # K > 0
                TC        fail        # K= +0
                TC        fail        # K < 0
                TC        fail        # K= -0
        # check for correct DABS in A (for K=+1, it should be +0)
lCCS4           COM                # 1's compliment of A
                AD        CCSdP1        # put (-A) + expected value in A
                CCS        A        # A = DABS
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # set K to +2 and execute CCS: 
        # check for correct branch
                CAF        CCSkP2        # set K = +2
                TS        CCSk
                CCS        CCSk        # A = DABS[C(K)]
                TC        lCCS5        # K > 0
                TC        fail        # K= +0
                TC        fail        # K < 0
                TC        fail        # K= -0
        # check for correct DABS in A (for K=+2, it should be +1)
lCCS5           COM                # 1's compliment of A
                AD        CCSdP2        # put (-A) + expected value in A
                CCS        A        # A = DABS
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # passed the test
                XCH        savQ
                TS        Q        # restore return address
                RETURN

        # ----------------------------------------------
        # TEST INDEX INSTRUCTION SUBROUTINE
        # L:        INDEX        K        (where K != 0025)
        # Verifies the following#
        # - Use the sum of C(L+1) + C(K) as the next instruction
        # -- just as if that sum had been taken from L+1.

# INDXcode        OCT       3        # code for this test
INDXst          OCT       5        # somewhere in fixed memory

INDXbas         OCT       0        # base address for indexing
                OCT       1
                OCT       2
                OCT       3
                OCT       4
                OCT       5

chkINDEX        XCH        Q
                TS        savQ        # save return address

                CAF       INDEXcode
                TS        curtest        # set current test code to this test

        # Decrementing loop
        #        - always executes at least once (tests at end of loop)
        #        - loops 'INDXst+1' times# decrements INDXval

                CAF       INDXst        # initialize loop counter

INDXlop         TS        INDXval

        # perform indexed CAF of values in INDXbas array#
        # index values range from 5 to 0
                INDEX        INDXval
                CAF        INDXbas

        # verify value retrieved using INDEX matches expected value
                COM                # get -A
                AD        INDXval        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

                CCS        INDXval        # done?
                TC        INDXlop        # not yet

                XCH        savQ
                TS        Q        # restore return address
                RETURN

        # ----------------------------------------------
        # TEST XCH INSTRUCTION SUBROUTINE
        # L:        XCH        K
        # Verifies the following:
        # - set C(A) = b(K)
        # - set C(K) = b(A)
        # - take next instruction from L+1

XCHcode         ADRES     XCHtst        # code for this test
        # XCH test values
XCHfP0          OCT       +0
XCHfM0          OCT       -0
XCHfalt1        OCT       52525        # alternating bit pattern 1
XCHfalt2        OCT       25252        # alternating bit pattern 2

chkXCH          XCH        Q
                TS        savQ        # save return address

            # FAKE - Should be preloaded ...
            CAF     XCHfP0
            TS      XCHkP0
            TS      TSk
            TS      ADk
            CAF     XCHfM0
            TS      XCHkM0
            CAF     XCHfalt1
            TS      XCHkalt1
            CAF     XCHfalt2
            TS      XCHkalt2

                CAF        XCHcode
                TS        curtest        # set current test code to this test

        # test - initial conditions: K=+0, A=-0
        # initialize A
                CS        XCHfP0
        # exchange A and K
                XCH        XCHkP0
        # test contents of A for expected value
                COM              # get -A
                AD      XCHfP0        # put (-A) + expected value in A
                CCS     A        # A = DABS
                TC      fail        # >0 (A < expected value)
                TC      fail        # +0
                TC      fail        # <0 (A > expected value)
        # test contents of K for expected value
                CS        XCHkP0        # get -A
                AD        XCHfM0        # put (-A) + expected value in A
                CCS        A        # A = DABS
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # test - initial conditions: K=-0, A=+0
        # initialize A
                CS        XCHfM0
        # exchange A and K
                XCH        XCHkM0
        # test contents of A for expected value
                COM                # get -A
                AD        XCHfM0        # put (-A) + expected value in A
                CCS        A        # A = DABS
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)
        # test contents of K for expected value
                CS        XCHkM0        # get -A
                AD        XCHfP0        # put (-A) + expected value in A
                CCS        A        # A = DABS
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # test - initial conditions: K=52525, A=25252
        # initialize A
                CS        XCHfalt1
        # exchange A and K
                XCH        XCHkalt1
        # test contents of A for expected value
                COM                # get -A
                AD        XCHfalt1        # put (-A) + expected value in A
                CCS        A        # A = DABS
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)
        # test contents of K for expected value
                CS        XCHkalt1        # get -A
                AD        XCHfalt2        # put (-A) + expected value in A
                CCS        A        # A = DABS
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # test - initial conditions: K=25252, A=52525
        # initialize A
                CS        XCHfalt2
        # exchange A and K
                XCH        XCHkalt2
        # test contents of A for expected value
                COM                # get -A
                AD        XCHfalt2        # put (-A) + expected value in A
                CCS        A        # A = DABS
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)
        # test contents of K for expected value
                CS        XCHkalt2        # get -A
                AD        XCHfalt1        # put (-A) + expected value in A
                CCS        A        # A = DABS
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # passed the test
                XCH        savQ
                TS        Q        # restore return address
                RETURN

        # ----------------------------------------------
        # TEST CS INSTRUCTION SUBROUTINE
        # L:        CS        K
        # Verifies the following:
        # - Set C(A) = -C(K)
        # - Take next instruction from L+1

CScode          ADRES     CStst        # code for this test
        # test values (K)
CSkP0           OCT       +0
CSkM0           OCT       -0
CSkalt1         OCT       52525        # 1's C of CSkalt2
CSkalt2         OCT       25252        # 1's C of CSkalt1

chkCS           XCH        Q
                TS        savQ        # save return address

                CAF        CScode
                TS        curtest        # set current test code to this test

        # clear and subtract +0
                CS        CSkP0        # load 1's compliment of K into A
                AD        CSkP0        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # clear and subtract -0
                CS        CSkM0        # load 1's compliment of K into A
                AD        CSkM0        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # clear and subtract alternating bit pattern %52525
                CS        CSkalt1        # load 1's compliment of K into A
                AD        CSkalt1        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # clear and subtract alternating bit pattern %25252
                CS        CSkalt2        # load 1's compliment of K into A
                AD        CSkalt2        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # passed the test
                XCH        savQ
                TS        Q        # restore return address
                RETURN
        # ----------------------------------------------
        # TEST TS INSTRUCTION SUBROUTINE
        # L#        TS         K
        # Verifies the following:
        # - Set C(K) = b(A)
        # - If b(A) contains no overflow, 
        # -- C(A) = b(A)# take next instruction from L+1
        # - If b(A) has positive overflow, C(A) = 000001# 
        # -- take next instruction from L+2
        # - If b(A) has negative overflow, C(A) = 177776# 
        # -- take next instruction from L+2

TScode          ADRES     TStst        # code for this test
TSone           OCT       +1
TSzero          OCT       +0
TSmzero         OCT       -0
TSmone          OCT       -1
TSkP1           OCT       37777        # TEST1: largest positive number w/no overflow
TSkM1           OCT       40000        # TEST2: largest negative number w/no overflow

chkTS           XCH        Q
                TS        savQ        # save return address

                CAF        TScode
                TS        curtest        # set current test code to this test

        # initialize TSk to -0
                CAF        TSmzero
                XCH        TSk

        # TEST 1: store positive number, no overflow
                CAF        TSkP1
                TS        TSk
                TC        lTS1        # no overflow
                TC        fail        # overflow
        # verify C(A) = b(A)
lTS1            COM                # get -A
                AD        TSkP1        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)
        # verify C(K) = b(A)
                CS        TSkP1        # get -expected value
                AD        TSk        # put (-expected value) + C(K) into A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # TEST 2: store negative number, no overflow
                CAF        TSkM1
                TS        TSk
                TC        lTS2        # no overflow
                TC        fail        # overflow
        # verify C(A) = b(A)
lTS2            COM                # get -A
                AD        TSkM1        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)
        # verify C(K) = b(A)
                CS        TSkM1        # get -expected value
                AD        TSk        # put (-expected value) + C(K) into A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)
                
        # TEST 3: store positive number, overflow
                CAF        TSkP1        # get largest positive number
                AD        TSone        # make it overflow# A = negative overflow
                TS        TSk        # store the positive overflow
                TC        fail        # no overflow
        # verify C(A) = 000001
                COM                # get -A
                AD        TSone        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)
        # verify C(K) = positive overflow
                CS        TSzero        # get -expected value
                AD        TSk        # put (-expected value) + C(K) into A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # TEST 4: store negative number, overflow
                CAF        TSkM1        # get largest negative number
                AD        TSmone        # make it overflow# A = negative overflow
                TS        TSk        # store the negative overflow
                TC        fail        # no overflow
        # verify C(A) = 177776
                COM                # get -A
                AD        TSmone        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)
        # verify C(K) = negative overflow
                CS        TSmzero        # get -expected value
                AD        TSk        # put (-expected value) + C(K) into A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

                XCH        savQ
                TS        Q        # restore return address
                RETURN
        # ----------------------------------------------
        # TEST AD INSTRUCTION SUBROUTINE
        # L:        AD        K
        # Verifies the following:
        # - Set C(A) = b(A) + C(K)
        # - Take next instruction from L+1
        # - if C(A) has positive overflow,
        # -- increment overflow counter by 1
        # - if C(A) has negative overflow,
        # -- decrement overflow counter by 1

ADcode          ADRES     ADtst        # code for this test
ADplus0         OCT       +0
ADplus1         OCT       1
ADmin1          OCT       -1

AD25252         OCT       25252        # +10922 decimal
AD12525         OCT       12525        # +5461 decimal
AD37777         OCT       37777        # largest positive number
AD12524         OCT       12524        # positive overflow of %25252+%25252

AD52525         OCT       52525        # -10922 decimal
AD65252         OCT       65252        # -5461 decimal
AD40000         OCT       40000        # largest negative number
AD65253         OCT       65253        # negative overflow of %52525+65252

chkAD           XCH        Q
                TS        savQ        # save return address

                CAF        ADcode
                TS        curtest        # set current test code to this test

        # TEST1: sum positive, no overflow
        # add: %25252 + %12525 = %37777 (sign + 14 magnitude)
                CAF        AD25252
                AD        AD12525
        # verify C(A) = %37777
                COM                # get -A
                AD        AD37777        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # TEST2: sum negative, no overflow (sign + 14 magnitude)
        # add: %52525 + %65252 = %40000
                CAF        AD52525
                AD        AD65252
        # verify C(A) = %40000
                COM                # get -A
                AD        AD40000        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # TEST3: sum positive, overflow
        # initialize overflow counter and positive overflow storage
                CAF        ADplus0
                TS        OVFCNTR
                TS        ADk
        # add: %25252 + %25252 = %52524 (sign + 14 magnitude)
                CAF        AD25252
                AD        AD25252
                TS        ADk        # store positive overflow
                TC        fail
        # verify ADk = %12524
                CS        ADk        # get -A
                AD        AD12524        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)
        # verify overflow counter =%00001
                CS        OVFCNTR        # get -A
                AD        ADplus1        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

        # TEST4: sum negative, overflow
                CAF        ADplus0
                TS        OVFCNTR
                TS        ADk
        # add: %52525 + %52525 = %25253 (sign + 14 magnitude)
                CAF        AD52525
                AD        AD52525
                TS        ADk        # store negative overflow
                TC        fail
        # verify ADk = %65253
                CS        ADk        # get -A
                AD        AD65253        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)
        # verify overflow counter =%77776
                CS        OVFCNTR        # get -A
                AD        ADmin1        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

                XCH        savQ
                TS        Q        # restore return address
                RETURN
        # ----------------------------------------------
        # TEST MASK INSTRUCTION SUBROUTINE
        # L:        MASK        K
        # Verifies the following:
        # - Set C(A) = b(A) & C(K)

MASKcode        ADRES     MASKtst        # code for this test
MASK1           OCT       46314
MASK2           OCT       25252
MASKval         OCT       04210        # expected result of MASK1 & MASK2

chkMASK         XCH        Q
                TS        savQ        # save return address

        # perform logical and of MASK1 and MASK2
                CAF        MASK1
                MASK        MASK2
        # verify C(A) = b(A) & C(K)
                COM                # get -A
                AD        MASKval        # put (-A) + expected value in A
                CCS        A        # compare
                TC        fail        # >0 (A < expected value)
                TC        fail        # +0
                TC        fail        # <0 (A > expected value)

                CAF        MASKcode
                TS        curtest        # set current test code to this test

        # passed the test
                XCH        savQ
                TS        Q        # restore return address
                RETURN
        # ----------------------------------------------
        # PASSED ALL TESTS!
