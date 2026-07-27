#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

// Sprite metadata structure
struct SpriteMetadata {
    std::string id;
    int width;
    int height;
    int bitsPerPixel;
    std::vector<uint8_t> paletteData;  // 256 * 4 bytes for 8-bit indexed color
    std::vector<uint8_t> pixelData;

    // Animation info
    struct AnimationFrame {
        int frameIndex;
        int holdTime;  // Number of 24-FPS frames to display
    };
    std::vector<AnimationFrame> animationFrames[4];  // 4 animation types

    // Hotspot info
    struct HotSpot {
        int offsetX;
        int offsetY;
        int type;  // Hotspot type ID
    };
    std::vector<HotSpot> hotspots;
};

// RIFF format constants
constexpr uint32_t RIFF_FOURCC = 0x46464952;  // "RIFF"
constexpr uint32_t LIST_FOURCC = 0x5453494C;  // "LIST"
constexpr uint32_t DIB_FOURCC = 0x20424944;   // "DIB "
constexpr uint32_t SPRT_FOURCC = 0x54525053;  // "SPRT"

// RIFF chunk structure
struct RIFFChunk {
    uint32_t fourcc;
    uint32_t size;
    uint8_t* data;
};

class RIFConverter {
public:
    RIFConverter();
    ~RIFConverter();

    // Load a RIF file and extract all sprites
    bool LoadRIF(const std::string& rifPath);

    // Save sprites as PNG files with JSON metadata
    bool SaveAsPNG(const std::string& outputDir);

    // Get metadata for a sprite
    const SpriteMetadata* GetSpriteMetadata(const std::string& spriteId) const;

    // Get total sprite count
    int GetSpriteCount() const { return m_sprites.size(); }

    // Print RIF structure (for debugging)
    void PrintStructure() const;

private:
    std::vector<SpriteMetadata> m_sprites;
    std::map<std::string, size_t> m_spriteIndex;

    // RIFF parsing helpers
    bool ParseRIFFFile(const std::string& rifPath);
    bool ReadChunk(FILE* file, RIFFChunk& chunk);
    bool ParseDIBChunk(const RIFFChunk& chunk, SpriteMetadata& metadata);
    bool ParseSPRTChunk(const RIFFChunk& chunk, SpriteMetadata& metadata);

    // PNG export
    bool SaveSpriteAsPNG(const SpriteMetadata& sprite, const std::string& outputPath);

    // JSON export
    bool SaveMetadataAsJSON(const std::string& outputDir);
    bool SaveSpriteJSON(const SpriteMetadata& sprite, FILE* jsonFile);
};
