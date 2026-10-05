#include <algorithm>
#include <array>
#include <format>
#include <fstream>

#include "Katana/Disc/Disc.h"
#include <string>

#include "Katana/Disc/ISO9660.h"

static void WriteLE32(std::ofstream& stream, uint32_t value)
{
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

static void WriteLE16(std::ofstream& stream, uint16_t value)
{
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

int main(int argc, char* argv[])
{
    if (argc < 2)
        return 0;
    
    std::string path = argv[1];
    
    std::unique_ptr<Disc> disc = Disc::Open(path);
    
    const DiscTOC& toc = disc->GetTOC();
    
    printf("TOC Type: %s\n", DiscTypeToString(toc.Type).c_str());
    
    static const uint8_t SYNC_PATTERN[12] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
    
    for (auto& Track : toc.Tracks)
    {
        if (Track.Type == TrackType::Audio) continue;
        
        std::array<uint8_t, 2352> sector;
        for (uint32_t fad = Track.StartFad; fad < Track.StartFad + Track.Length; fad++)
        {
            if (!disc->ReadRaw(fad, sector.data()))
            {
                printf("DISC BAD! fad %u failed to read.", fad);
                return 1;
            }
            
            bool syncMatch = std::memcmp(sector.data(), SYNC_PATTERN, 12) == 0;
            
            if (!syncMatch)
            {
                printf("DISC BAD! fad %u has invalid sync pattern.", fad);
                return 1;
            }
                
            
            uint8_t minutesBCD = sector[12];
            uint8_t secondsBCD = sector[13];
            uint8_t framesBCD = sector[14];
            
            uint32_t minutes = (minutesBCD >> 4) * 10 + (minutesBCD & 0xF);
            uint32_t seconds = (secondsBCD >> 4) * 10 + (secondsBCD & 0xF);
            uint32_t frames = (framesBCD >> 4) * 10 + (framesBCD & 0xF);
            
            uint32_t headerFad = (minutes * 60 + seconds) * 75 + frames;
            
            if (headerFad != fad)
            {
                printf("DISC BAD! fad %u has invalid address.", fad);
                return 1;
            }
            
            printf("Sector Address: %u:%u:%u fad: %u\n", minutes, seconds, frames, headerFad);
            
            if (sector[15] != 1)
            {
                printf("DISC BAD! fad %u has invalid mode %u.", fad, sector[15]);
                return 1;
            }
        }
    }
    
    std::array<uint8_t, 2048> hwid;
    
    disc->ReadUserData(45150, hwid.data());
    
    std::string hwidStr(reinterpret_cast<char*>(hwid.data()), 16);
    
    if (hwidStr != "SEGA SEGAKATANA ")
    {
        printf("DISC BAD! HWID %s is invalid.", hwidStr.c_str());
        return 1;
    }
    
    printf("HWID: %s\n", hwidStr.c_str());
    
    printf("Disc OK!\n");
    
    printf("Dumping Audio...\n");
    
    for (auto& Track : toc.Tracks)
    {
        if (Track.Type != TrackType::Audio) continue;
        
        std::ofstream wav(std::format("track{:02}.wav", Track.Number), std::ios::binary);
        
        uint32_t dataSize = Track.Length * 2352;
        
        wav << "RIFF";
        WriteLE32(wav, 36 + dataSize);
        wav << "WAVE";
        wav << "fmt ";
        WriteLE32(wav, 16);
        WriteLE16(wav, 1);
        WriteLE16(wav, 2);
        WriteLE32(wav, 44100);
        WriteLE32(wav, 44100 * 2 * 2);
        WriteLE16(wav, 4);
        WriteLE16(wav, 16);
        wav << "data";
        WriteLE32(wav, dataSize);
        
        std::array<uint8_t, 2352> sector;
        for (uint32_t fad = Track.StartFad; fad < Track.StartFad + Track.Length; fad++)
        {
            if (!disc->ReadRaw(fad, sector.data()))
            {
                printf("DISC BAD! fad %u failed to read.", fad);
                return 1;
            }
            
            wav.write(reinterpret_cast<const char*>(sector.data()), sector.size());        
        }
        
        wav.close();
        printf("Wrote %s\n", std::format("track{:02}.wav", Track.Number).c_str());
    }
    
    printf("Dumping Data...\n");

    uint8_t lastSession = toc.Tracks.back().Session;

    for (uint8_t session = 1; session <= lastSession; session++)
    {
        const Track* dataTrack = nullptr;
        for (const Track& track : toc.Tracks)
        {
            if (track.Session == session && track.Type != TrackType::Audio)
            {
                dataTrack = &track;
                break;
            }
        }

        if (!dataTrack)
            continue;

        try
        {
            std::string folder = std::format("track{:02}", dataTrack->Number);
        
            ISO9660 iso(*disc, dataTrack->StartFad);
            printf("%s: %s\n", folder.c_str(), iso.GetVolumeId().c_str());
            uint32_t count = iso.ExtractAll(folder);
            printf("  extracted %u files\n", count);
        }
        catch (std::exception& e)
        {
            printf("Failed to dump track %u\n", dataTrack->Number);
        }
    }
    
    return 0;
}
