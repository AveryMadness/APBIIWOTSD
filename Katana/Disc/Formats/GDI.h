#pragma once
#include "../Disc.h"

#pragma once
#include <fstream>
#include <string>
#include <vector>

#include "../Disc.h"

class GDI : public Disc
{
public:
    explicit GDI(const std::string& path);

protected:
    uint32_t ReadStoredSector(const Track& track, uint32_t fad, uint8_t* buffer) override;

private:
    struct TrackFile
    {
        std::ifstream file;
        uint32_t sectorSize = 0;
    };
    std::vector<TrackFile> TrackFiles;
};