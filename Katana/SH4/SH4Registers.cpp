#include "SH4Registers.h"

void SH4Registers::SetSR(uint32_t value)
{
    //bank 1 is only active in privileged mode
    bool oldBank = (SR & SR_MD) && (SR & SR_RB);
    bool newBank = (value & SR_MD) && (value & SR_RB);

    if (oldBank != newBank)
    {
        for (int i = 0; i < 8; i++)
            std::swap(R[i], R_[i]);
    }

    //unused bits always read as 0
    SR = value & 0x700083F3;
}

void SH4Registers::SetFPSCR(uint32_t value)
{
    if ((FPSCR ^ value) & FPSCR_FR)
        std::swap(FR, XF);

    FPSCR = value & 0x003FFFFF;
}