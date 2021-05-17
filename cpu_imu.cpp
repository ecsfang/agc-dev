#include <stdio.h>
#include "cpu.h"

// If COARSE_SMOOTH is 1, then the timing of coarse-alignment (in terms of
// bursting and separation of bursts) is according to the Delco manual.
// However, since the simulated IMU has no physical inertia, it adjusts
// instantly (and therefore jerkily).  The COARSE_SMOOTH constant creates
// smaller bursts, and therefore smoother FDAI motion.  Normally, there are
// 192 pulses in a burst.  In the simulation, there are 192/COARSE_SMOOTH
// pulses in a burst.  COARSE_SMOOTH should be in integral divisor of both
// 192 and of 50*1024.  This constrains it to be any power of 2, up to 64.
#define COARSE_SMOOTH 8

int CCpu::BurstOutput(int DriveBitMask, int CounterRegister, int Channel)
{
    static int CountCDUX = 0, CountCDUY = 0, CountCDUZ = 0; // In target CPU format.
    int DriveCount = 0, DriveBit, Direction = 0, Delta, DriveCountSaved;
    if (CounterRegister == REG_CDUXCMD)
        DriveCountSaved = CountCDUX;
    else if (CounterRegister == REG_CDUYCMD)
        DriveCountSaved = CountCDUY;
    else if (CounterRegister == REG_CDUZCMD)
        DriveCountSaved = CountCDUZ;
    else
        return (0);
    // Driving this axis?
    DriveBit = (mem.readIO(014) & DriveBitMask);
    // If so, we must retrieve the count from the counter register.
    if (DriveBit)
    {
        DriveCount = mem.read12(CounterRegister);
        mem.write12(CounterRegister, 0);
    }
    // The count may be negative.  If so, normalize to be positive and set the
    // direction flag.
    Direction = (040000 & DriveCount);
    if (Direction)
    {
        DriveCount ^= 077777;
        DriveCountSaved -= DriveCount;
    }
    else
        DriveCountSaved += DriveCount;
    if (DriveCountSaved < 0)
    {
        DriveCountSaved = -DriveCountSaved;
        Direction = 040000;
    }
    else
        Direction = 0;
    // Determine how many pulses to output.  The max is 192 per burst.
    Delta = DriveCountSaved;
    if (Delta >= 192 / COARSE_SMOOTH)
        Delta = 192 / COARSE_SMOOTH;
    // If the count is non-zero, pulse it.
    if (Delta > 0)
    {
        fprintf(logFile,"Update IMU (%03o:%05o)\n", Channel, Direction | Delta);
        mem.writeIO(Channel, Direction | Delta);
        DriveCountSaved -= Delta;
    }
    if (Direction)
        DriveCountSaved = -DriveCountSaved;
    if (CounterRegister == REG_CDUXCMD)
        CountCDUX = DriveCountSaved;
    else if (CounterRegister == REG_CDUYCMD)
        CountCDUY = DriveCountSaved;
    else if (CounterRegister == REG_CDUZCMD)
        CountCDUZ = DriveCountSaved;
    return (DriveCountSaved);
}

// Coarse-alignment.
// The IMU CDU drive emits bursts every 600 ms.  Each cycle is
// 12/1024000 seconds long.  This happens to mean that a burst is
// emitted every 51200 CPU cycles, but we multiply it out below
// to make it look pretty
#define IMUCDU_BURST_CYCLES ((600 * 1024000) / (1000 * 12 * COARSE_SMOOTH))
static uint64_t ImuCduCount = 0;
static unsigned ImuChannel14 = 0;

void CCpu::UpdateIMU(void)
{
    uint16_t i = (mem.readIO(014) & 070000); // Check IMU CDU drive bits.
    if (ImuChannel14 == 0 && i != 0)         // If suddenly active, start drive.
        ImuCduCount = clockCnt - IMUCDU_BURST_CYCLES;
    if (i != 0 && (clockCnt - ImuCduCount) >= IMUCDU_BURST_CYCLES) // Time for next burst.
    {
        // Adjust the cycle counter.
        ImuCduCount += IMUCDU_BURST_CYCLES;
        // Determine how many pulses are wanted on each axis this burst.
        ImuChannel14 = BurstOutput(040000, REG_CDUXCMD, 0174);
        ImuChannel14 |= BurstOutput(020000, REG_CDUYCMD, 0175);
        ImuChannel14 |= BurstOutput(010000, REG_CDUZCMD, 0176);
    }
}