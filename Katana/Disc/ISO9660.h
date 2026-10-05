#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "Disc.h"

struct ISOEntry
{
    std::string Name;
    uint32_t Lba = 0;
    uint32_t Size = 0;
    bool IsDirectory = false;
    bool IsMultiExtent = false;
};

class ISO9660
{
public:
    ISO9660(Disc& disc, uint32_t sessionStartFad);

    const std::string& GetVolumeId() const;
    const ISOEntry& GetRoot() const;

    std::vector<ISOEntry> ReadDirectory(const ISOEntry& directory);
    void ExtractFile(const ISOEntry& file, const std::filesystem::path& outPath);

    uint32_t ExtractAll(const std::filesystem::path& outFolder);

private:
    Disc& disc;
    std::string volumeId;
    ISOEntry root;

    bool ReadSector(uint32_t lba, uint8_t* buffer);
    static ISOEntry ParseRecord(const uint8_t* record);
    uint32_t ExtractDirectory(const ISOEntry& directory, const std::filesystem::path& outFolder, int depth);
};