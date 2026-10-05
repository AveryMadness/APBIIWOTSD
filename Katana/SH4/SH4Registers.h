#pragma once
#include <array>
#include <cstdint>

//sr bits
static constexpr uint32_t SR_T = 1u << 0;
static constexpr uint32_t SR_S = 1u << 1;
static constexpr uint32_t SR_IMASK = 0xFu << 4;
static constexpr uint32_t SR_Q = 1u << 8;
static constexpr uint32_t SR_M = 1u << 9;
static constexpr uint32_t SR_FD = 1u << 15;
static constexpr uint32_t SR_BL = 1u << 28;
static constexpr uint32_t SR_RB = 1u << 29;
static constexpr uint32_t SR_MD = 1u << 30;

//fpscr bits
static constexpr uint32_t FPSCR_PR = 1u << 19;
static constexpr uint32_t FPSCR_SZ = 1u << 20;
static constexpr uint32_t FPSCR_FR = 1u << 21;

class SH4Registers
{
public:
    //visible R0-R15, R0-R7 are whichever bank is active
    std::array<uint32_t, 16> R{};
    //the inactive bank of R0-R7
    std::array<uint32_t, 8> R_{};

    //visible FR0-15 and the inactive bank (XF0-15), raw bits, bit_cast to float when doing math
    std::array<uint32_t, 16> FR{};
    std::array<uint32_t, 16> XF{};

    uint32_t PC = 0;
    uint32_t SSR = 0, SPC = 0, GBR = 0, VBR = 0, SGR = 0, DBR = 0;
    uint32_t MACH = 0, MACL = 0, PR = 0;
    uint32_t FPUL = 0;

    uint32_t GetSR() const;
    //every SR write goes through here so bank switching cant be forgotten
    void SetSR(uint32_t value);

    uint32_t GetFPSCR() const;
    void SetFPSCR(uint32_t value);

    bool GetT() const;
    void SetT(bool value);

private:
    uint32_t SR = 0;
    uint32_t FPSCR = 0;
};