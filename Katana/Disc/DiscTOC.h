#pragma once
#include <cstdint>
#include <vector>

enum class TrackType : uint8_t
{
    //CDDA, always 2352
    Audio,
    //2048, GD-ROM tracks
    Mode1,
    //2048, XA CD-Rom
    Mode2Form1,
    //2324, likely not used
    Mode2Form2
};

inline std::string TrackTypeToString(TrackType trackType)
{
    switch (trackType)
    {
        case TrackType::Audio:
            return "Audio";
        case TrackType::Mode1:
            return "Mode1";
        case TrackType::Mode2Form1:
            return "Mode2Form1";
        case TrackType::Mode2Form2:
            return "Mode2Form2";
        default:
        return "";
    }
}

enum class DiscType : uint8_t
{
    GDROM,
    CDROM,
    CDROM_XA,
    CDDA
};

inline std::string DiscTypeToString(DiscType discType)
{
    switch (discType)
    {
        case DiscType::GDROM:
            return "GDROM";
        case DiscType::CDROM:
            return "CDROM";
        case DiscType::CDROM_XA:
            return "CDROM_XA";
        case DiscType::CDDA:
            return "CDDA";
        default:
            return "";
    }
}

struct Track
{
    //1-99, the track number
    uint8_t Number;
    //1-based
    uint8_t Session;
    TrackType Type;
    uint8_t Control;
    uint32_t StartFad;
    uint32_t PregapLength;
    uint32_t Length;
};

struct DiscTOC
{
    DiscType Type;
    std::vector<Track> Tracks;
    std::vector<uint32_t> LeadOutFads;
    
    const Track* FindTrack(uint32_t fad) const
    {
        for (const Track& track : Tracks)
        {
            uint32_t trackStart = track.StartFad - track.PregapLength;
            uint32_t trackEnd = track.StartFad + track.Length;
            
            if (fad >= trackStart && fad < trackEnd)
                return &track;
        }
        return nullptr;
    }
    
    //used to find game binary
    const Track* FirstDataTrackOfLastSession() const
    {
        if (Tracks.empty())
            return nullptr;
        
        uint8_t lastSession = Tracks.back().Session;
        
        for (const Track& track : Tracks)
        {
           if (track.Session == lastSession && track.Type != TrackType::Audio)
               return &track;
        }
        return nullptr;
    }
};

