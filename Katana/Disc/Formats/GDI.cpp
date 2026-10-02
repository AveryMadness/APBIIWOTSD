#include "GDI.h"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <stdexcept>

//gdi line: <track> <start lba> <type> <sector size> <file name> <offset>
//type is the q-channel control nibble, 4 = data, 0 = audio

GDI::GDI(const std::string& path)
{
    std::ifstream gdiFile(path);
    if (!gdiFile.is_open())
        throw std::runtime_error("GDI: cant open " + path);

    //track files live next to the .gdi
    std::filesystem::path folder = std::filesystem::path(path).parent_path();

    uint32_t trackCount = 0;
    gdiFile >> trackCount;
    if (gdiFile.fail() || trackCount == 0 || trackCount > 99)
        throw std::runtime_error("GDI: bad track count");

    for (uint32_t i = 0; i < trackCount; i++)
    {
        uint32_t number = 0;
        uint32_t startLba = 0;
        uint32_t control = 0;
        uint32_t sectorSize = 0;
        std::string fileName;
        uint64_t offset = 0;

        //quoted handles "Track 01.bin" and plain track01.bin
        gdiFile >> number >> startLba >> control >> sectorSize >> std::quoted(fileName) >> offset;
        if (gdiFile.fail())
            throw std::runtime_error("GDI: bad line for track " + std::to_string(i + 1));

        if (sectorSize != 2048 && sectorSize != 2352)
            throw std::runtime_error("GDI: unsupported sector size " + std::to_string(sectorSize));

        //dunno what ts means yet its like something
        if (offset != 0)
            throw std::runtime_error("GDI: nonzero offset on track " + std::to_string(number));

        std::filesystem::path trackPath = folder / fileName;

        TrackFile trackFile;
        trackFile.file.open(trackPath, std::ios::binary);
        if (!trackFile.file.is_open())
            throw std::runtime_error("GDI: cant open track file " + trackPath.string());
        trackFile.sectorSize = sectorSize;

        uint64_t fileSize = std::filesystem::file_size(trackPath);

        Track track{};
        track.Number = static_cast<uint8_t>(number);
        //tracks 1-2 is single density, 3+ is high density
        track.Session = number < 3 ? 1 : 2;
        track.Control = static_cast<uint8_t>(control);
        track.Type = (control & 0x4) ? TrackType::Mode1 : TrackType::Audio;
        track.StartFad = startLba + 150;
        //gdi no pregaps
        track.PregapLength = 0;
        track.Length = static_cast<uint32_t>(fileSize / sectorSize);

        Toc.Tracks.push_back(track);
        TrackFiles.push_back(std::move(trackFile));
    }

    for (uint8_t session = 1; session <= 2; session++)
    {
        uint32_t end = 0;
        for (const Track& track : Toc.Tracks)
        {
            if (track.Session == session)
                end = std::max(end, track.StartFad + track.Length);
        }
        Toc.LeadOutFads.push_back(end);
    }

    Toc.Type = DiscType::GDROM;
}

uint32_t GDI::ReadStoredSector(const Track& track, uint32_t fad, uint8_t* buffer)
{
    size_t index = &track - Toc.Tracks.data();
    TrackFile& trackFile = TrackFiles[index];

    uint64_t sectorInFile = fad - track.StartFad;
    trackFile.file.seekg(sectorInFile * trackFile.sectorSize);
    trackFile.file.read(reinterpret_cast<char*>(buffer), trackFile.sectorSize);

    if (!trackFile.file)
    {
        //clear the error bits or bad things happen
        trackFile.file.clear();
        return 0;
    }
    return trackFile.sectorSize;
}