#pragma once
#include <cstdint>

class SH4Bus
{
public:
    virtual ~SH4Bus() = default;
    
    virtual uint16_t FetchInstruction(uint32_t address) = 0;
    
    virtual uint8_t Read8(uint32_t address) = 0;
    virtual uint16_t Read16(uint32_t address) = 0;
    virtual uint32_t Read32(uint32_t address) = 0;
    virtual uint64_t Read64(uint32_t address) = 0;

    virtual void Write8(uint32_t address, uint8_t value) = 0;
    virtual void Write16(uint32_t address, uint16_t value) = 0;
    virtual void Write32(uint32_t address, uint32_t value) = 0;
    virtual void Write64(uint32_t address, uint64_t value) = 0;
};
