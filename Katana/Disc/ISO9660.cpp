#include "ISO9660.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <stdexcept>

static constexpr uint32_t SECTOR_SIZE = 2048;
static constexpr uint32_t MAX_DESCRIPTORS = 32;
static constexpr int MAX_DEPTH = 32;

//byte by byte so alignment and endianness dont matter
static uint16_t ReadLE16(const uint8_t* data)
{
    return static_cast<uint16_t>(data[0] | (data[1] << 8));
}

static uint32_t ReadLE32(const uint8_t* data)
{
    return static_cast<uint32_t>(data[0])
        | (static_cast<uint32_t>(data[1]) << 8)
        | (static_cast<uint32_t>(data[2]) << 16)
        | (static_cast<uint32_t>(data[3]) << 24);
}

static std::string TrimRight(std::string text)
{
    while (!text.empty() && (text.back() == ' ' || text.back() == '\0'))
        text.pop_back();
    return text;
}

ISO9660::ISO9660(Disc& disc, uint32_t sessionStartFad)
    : disc(disc)
{
    std::array<uint8_t, SECTOR_SIZE> sector;
    bool found = false;

    for (uint32_t i = 0; i < MAX_DESCRIPTORS; i++)
    {
        if (!disc.ReadUserData(sessionStartFad + 16 + i, sector.data()))
            break;

        //every descriptor has CD001 after the type byte, anything else means no filesystem
        if (std::memcmp(&sector[1], "CD001", 5) != 0)
            break;

        uint8_t type = sector[0];
        if (type == 0x01)
        {
            found = true;
            break;
        }
        //terminator
        if (type == 0xFF)
            break;
    }

    if (!found)
        throw std::runtime_error("ISO9660: no primary volume descriptor");

    uint16_t blockSize = ReadLE16(&sector[128]);
    if (blockSize != SECTOR_SIZE)
        throw std::runtime_error("ISO9660: unsupported block size " + std::to_string(blockSize));

    volumeId = TrimRight(std::string(reinterpret_cast<const char*>(&sector[40]), 32));

    //root record lives inside the pvd
    root = ParseRecord(&sector[156]);
    root.Name.clear();
}

const std::string& ISO9660::GetVolumeId() const
{
    return volumeId;
}

const ISOEntry& ISO9660::GetRoot() const
{
    return root;
}

bool ISO9660::ReadSector(uint32_t lba, uint8_t* buffer)
{
    return disc.ReadUserData(lba + 150, buffer);
}

ISOEntry ISO9660::ParseRecord(const uint8_t* record)
{
    ISOEntry entry;
    entry.Lba = ReadLE32(record + 2);
    entry.Size = ReadLE32(record + 10);

    uint8_t flags = record[25];
    entry.IsDirectory = (flags & 0x02) != 0;
    entry.IsMultiExtent = (flags & 0x80) != 0;

    uint8_t nameLength = record[32];
    entry.Name.assign(reinterpret_cast<const char*>(record + 33), nameLength);

    size_t semicolon = entry.Name.find(';');
    if (semicolon != std::string::npos)
        entry.Name.erase(semicolon);

    if (!entry.Name.empty() && entry.Name.back() == '.')
        entry.Name.pop_back();

    return entry;
}

std::vector<ISOEntry> ISO9660::ReadDirectory(const ISOEntry& directory)
{
    std::vector<ISOEntry> entries;
    std::array<uint8_t, SECTOR_SIZE> sector;

    uint32_t sectorCount = (directory.Size + SECTOR_SIZE - 1) / SECTOR_SIZE;

    for (uint32_t i = 0; i < sectorCount; i++)
    {
        if (!ReadSector(directory.Lba + i, sector.data()))
            throw std::runtime_error("ISO9660: cant read directory sector at lba " + std::to_string(directory.Lba + i));

        uint32_t offset = 0;
        while (offset < SECTOR_SIZE)
        {
            uint8_t recordLength = sector[offset];

            //records never cross sectors, 0 means the rest of this one is padding
            if (recordLength == 0)
                break;

            const uint8_t* record = &sector[offset];
            uint8_t nameLength = record[32];

            if (recordLength < 34 || offset + recordLength > SECTOR_SIZE || 33u + nameLength > recordLength)
                throw std::runtime_error("ISO9660: corrupt directory record at lba " + std::to_string(directory.Lba + i));

            //skip . and ..
            bool isDotEntry = nameLength == 1 && (record[33] == 0x00 || record[33] == 0x01);
            if (!isDotEntry)
                entries.push_back(ParseRecord(record));

            offset += recordLength;
        }
    }

    return entries;
}

void ISO9660::ExtractFile(const ISOEntry& file, const std::filesystem::path& outPath)
{
    if (file.IsMultiExtent)
        throw std::runtime_error("ISO9660: multi extent file not supported: " + file.Name);

    std::ofstream out(outPath, std::ios::binary);
    if (!out.is_open())
        throw std::runtime_error("ISO9660: cant create " + outPath.string());

    std::array<uint8_t, SECTOR_SIZE> sector;
    uint32_t remaining = file.Size;
    uint32_t lba = file.Lba;

    while (remaining > 0)
    {
        if (!ReadSector(lba, sector.data()))
            throw std::runtime_error("ISO9660: cant read " + file.Name + " at lba " + std::to_string(lba));

        //last sector is usually only partly used
        uint32_t chunk = std::min(remaining, SECTOR_SIZE);
        out.write(reinterpret_cast<const char*>(sector.data()), chunk);

        remaining -= chunk;
        lba++;
    }
}

uint32_t ISO9660::ExtractAll(const std::filesystem::path& outFolder)
{
    return ExtractDirectory(root, outFolder, 0);
}

uint32_t ISO9660::ExtractDirectory(const ISOEntry& directory, const std::filesystem::path& outFolder, int depth)
{
    if (depth > MAX_DEPTH)
        throw std::runtime_error("ISO9660: directories nested too deep");

    std::filesystem::create_directories(outFolder);

    uint32_t fileCount = 0;
    for (const ISOEntry& entry : ReadDirectory(directory))
    {
        //dont let a name escape the output folder
        if (entry.Name.empty() || entry.Name == ".." || entry.Name.find_first_of("/\\:") != std::string::npos)
            throw std::runtime_error("ISO9660: bad file name " + entry.Name);

        std::filesystem::path target = outFolder / entry.Name;

        if (entry.IsDirectory)
            fileCount += ExtractDirectory(entry, target, depth + 1);
        else
        {
            ExtractFile(entry, target);
            fileCount++;
        }
    }
    return fileCount;
}