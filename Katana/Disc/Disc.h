#pragma once
#include <memory>
#include <string>

#include "DiscTOC.h"

class Disc
{
public:
    virtual ~Disc() = default;
    
    static std::unique_ptr<Disc> Open(const std::string& path);
    
    const DiscTOC& GetTOC() const;
    
    bool ReadUserData(uint32_t fad, uint8_t* buffer);
    bool ReadRaw(uint32_t fad, uint8_t* buffer);
    
protected:
    DiscTOC Toc;
    
    virtual uint32_t ReadStoredSector(const Track& track, uint32_t fad, uint8_t* buffer) = 0;
};
