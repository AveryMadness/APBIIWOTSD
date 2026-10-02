#include "Disc.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <stdexcept>

#include "Formats/GDI.h"

static constexpr uint32_t RAW_SECTOR_SIZE = 2352;
static constexpr uint32_t USER_DATA_SIZE = 2048;

static int GetUserDataOffset(TrackType type, uint32_t storedSize)
{
    switch (storedSize)
    {
    case 2048:
        return 0;
    case 2336:
        //8 byte subheader
        return type == TrackType::Mode2Form1 ? 8 : -1;
    case 2352:
        if (type == TrackType::Mode1)
            return 16;
        if (type == TrackType::Mode2Form1)
            return 24;
        return -1;
    default:
        return -1;
    }
}

std::unique_ptr<Disc> Disc::Open(const std::string& path)
{
    std::string extension = std::filesystem::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (extension == ".gdi")
        return std::make_unique<GDI>(path);

    throw std::runtime_error("Disc: unsupported format " + extension);
}

const DiscTOC& Disc::GetTOC() const
{
    return Toc;
}

bool Disc::ReadUserData(uint32_t fad, uint8_t* buffer)
{
    const Track* track = Toc.FindTrack(fad);

    if (!track || track->Type == TrackType::Audio || fad < track->StartFad)
        return false;

    std::array<uint8_t, RAW_SECTOR_SIZE> sector;
    uint32_t storedSize = ReadStoredSector(*track, fad, sector.data());
    if (storedSize == 0)
        return false;

    int offset = GetUserDataOffset(track->Type, storedSize);
    if (offset < 0)
        return false;

    std::memcpy(buffer, sector.data() + offset, USER_DATA_SIZE);
    return true;
}

bool Disc::ReadRaw(uint32_t fad, uint8_t* buffer)
{
    const Track* track = Toc.FindTrack(fad);
    if (!track || fad < track->StartFad)
        return false;

    std::array<uint8_t, RAW_SECTOR_SIZE> sector;
    uint32_t storedSize = ReadStoredSector(*track, fad, sector.data());

    if (storedSize != RAW_SECTOR_SIZE)
        return false;

    std::memcpy(buffer, sector.data(), RAW_SECTOR_SIZE);
    return true;
}