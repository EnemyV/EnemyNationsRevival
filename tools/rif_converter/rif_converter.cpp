#include "rif_converter.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

RIFConverter::RIFConverter() = default;

RIFConverter::~RIFConverter() = default;

bool RIFConverter::LoadRIF(const std::string& rifPath) {
    std::cout << "Loading RIF file: " << rifPath << std::endl;

    FILE* file = fopen(rifPath.c_str(), "rb");
    if (!file) {
        std::cerr << "Failed to open RIF file: " << rifPath << std::endl;
        return false;
    }

    bool success = ParseRIFFFile(rifPath);

    fclose(file);

    if (success) {
        std::cout << "Successfully loaded " << m_sprites.size() << " sprites" << std::endl;
    }

    return success;
}

bool RIFConverter::ParseRIFFFile(const std::string& rifPath) {
    FILE* file = fopen(rifPath.c_str(), "rb");
    if (!file) {
        return false;
    }

    // Read RIFF header
    uint32_t riffMagic;
    if (fread(&riffMagic, 4, 1, file) != 1 || riffMagic != RIFF_FOURCC) {
        std::cerr << "Not a valid RIFF file (missing RIFF magic)" << std::endl;
        fclose(file);
        return false;
    }

    // Read RIFF size
    uint32_t riffSize;
    if (fread(&riffSize, 4, 1, file) != 1) {
        std::cerr << "Failed to read RIFF size" << std::endl;
        fclose(file);
        return false;
    }

    // Read RIFF type (should be file type, typically "SPRT" or similar)
    uint32_t riffType;
    if (fread(&riffType, 4, 1, file) != 1) {
        std::cerr << "Failed to read RIFF type" << std::endl;
        fclose(file);
        return false;
    }

    std::cout << "RIFF file size: " << riffSize << " bytes" << std::endl;

    // Read chunks until end of file
    size_t spriteCount = 0;
    long remainingSize = riffSize - 4;  // Subtract the 4 bytes for RIFF type

    while (remainingSize > 0 && !feof(file)) {
        // Read chunk header (8 bytes: fourcc + size)
        uint32_t fourcc;
        uint32_t size;

        if (fread(&fourcc, 4, 1, file) != 1 || fread(&size, 4, 1, file) != 1) {
            break;
        }

        remainingSize -= 8;

        // Print chunk info for debugging
        char fourccStr[5];
        memcpy(fourccStr, &fourcc, 4);
        fourccStr[4] = '\0';
        std::cout << "Found chunk: " << fourccStr << " (size: " << size << ")" << std::endl;

        if (fourcc == DIB_FOURCC || fourcc == SPRT_FOURCC) {
            // This is a sprite chunk
            SpriteMetadata sprite;

            // Read the chunk data
            std::vector<uint8_t> chunkData(size);
            if (fread(chunkData.data(), size, 1, file) != 1) {
                std::cerr << "Failed to read chunk data" << std::endl;
                break;
            }

            // Parse based on chunk type
            if (fourcc == DIB_FOURCC) {
                // DIB chunk contains bitmap data
                // Format: BITMAPINFOHEADER (40 bytes) + palette (256*4) + pixel data
                if (size >= 40 + 1024) {
                    // Read BITMAPINFOHEADER
                    uint32_t headerSize = *(uint32_t*)chunkData.data();
                    uint32_t width = *(uint32_t*)(chunkData.data() + 4);
                    uint32_t height = *(uint32_t*)(chunkData.data() + 8);
                    uint16_t planes = *(uint16_t*)(chunkData.data() + 12);
                    uint16_t bitsPerPixel = *(uint16_t*)(chunkData.data() + 14);

                    sprite.width = width;
                    sprite.height = height;
                    sprite.bitsPerPixel = bitsPerPixel;

                    std::cout << "  DIB: " << width << "x" << height << " (" << bitsPerPixel << "bpp)" << std::endl;

                    // Extract palette (256 colors * 4 bytes each = 1024 bytes)
                    sprite.paletteData.assign(
                        chunkData.begin() + 40,
                        chunkData.begin() + 40 + 1024
                    );

                    // Extract pixel data
                    sprite.pixelData.assign(
                        chunkData.begin() + 40 + 1024,
                        chunkData.end()
                    );

                    // Generate sprite ID
                    sprite.id = "sprite_" + std::to_string(spriteCount);
                    spriteCount++;

                    m_sprites.push_back(sprite);
                    m_spriteIndex[sprite.id] = m_sprites.size() - 1;
                }
            }

            remainingSize -= size;

            // Align to 2-byte boundary if necessary
            if (size & 1) {
                fseek(file, 1, SEEK_CUR);
                remainingSize--;
            }
        } else if (fourcc == LIST_FOURCC) {
            // LIST chunk - skip for now
            fseek(file, size, SEEK_CUR);
            remainingSize -= size;

            // Align to 2-byte boundary
            if (size & 1) {
                fseek(file, 1, SEEK_CUR);
                remainingSize--;
            }
        } else {
            // Unknown chunk - skip
            fseek(file, size, SEEK_CUR);
            remainingSize -= size;

            // Align to 2-byte boundary
            if (size & 1) {
                fseek(file, 1, SEEK_CUR);
                remainingSize--;
            }
        }
    }

    fclose(file);

    return !m_sprites.empty();
}

bool RIFConverter::SaveAsPNG(const std::string& outputDir) {
    std::cout << "Saving sprites to: " << outputDir << std::endl;

    // Create output directory if it doesn't exist
    fs::create_directories(outputDir);

    // For now, just save metadata as JSON
    // PNG saving would require a PNG library (libpng, stb_image_write, etc.)
    return SaveMetadataAsJSON(outputDir);
}

bool RIFConverter::SaveMetadataAsJSON(const std::string& outputDir) {
    std::string jsonPath = outputDir + "/sprites_metadata.json";
    FILE* jsonFile = fopen(jsonPath.c_str(), "w");
    if (!jsonFile) {
        std::cerr << "Failed to create JSON metadata file: " << jsonPath << std::endl;
        return false;
    }

    fprintf(jsonFile, "{\n");
    fprintf(jsonFile, "  \"sprites\": [\n");

    for (size_t i = 0; i < m_sprites.size(); ++i) {
        const auto& sprite = m_sprites[i];

        fprintf(jsonFile, "    {\n");
        fprintf(jsonFile, "      \"id\": \"%s\",\n", sprite.id.c_str());
        fprintf(jsonFile, "      \"width\": %d,\n", sprite.width);
        fprintf(jsonFile, "      \"height\": %d,\n", sprite.height);
        fprintf(jsonFile, "      \"bitsPerPixel\": %d,\n", sprite.bitsPerPixel);
        fprintf(jsonFile, "      \"pixelCount\": %zu,\n", sprite.pixelData.size());
        fprintf(jsonFile, "      \"paletteSize\": %zu", sprite.paletteData.size());

        if (!sprite.hotspots.empty()) {
            fprintf(jsonFile, ",\n");
            fprintf(jsonFile, "      \"hotspots\": [\n");
            for (size_t j = 0; j < sprite.hotspots.size(); ++j) {
                const auto& hs = sprite.hotspots[j];
                fprintf(jsonFile, "        {\"x\": %d, \"y\": %d, \"type\": %d}",
                    hs.offsetX, hs.offsetY, hs.type);
                if (j < sprite.hotspots.size() - 1) fprintf(jsonFile, ",");
                fprintf(jsonFile, "\n");
            }
            fprintf(jsonFile, "      ]\n");
        } else {
            fprintf(jsonFile, "\n");
        }

        fprintf(jsonFile, "    }");
        if (i < m_sprites.size() - 1) fprintf(jsonFile, ",");
        fprintf(jsonFile, "\n");
    }

    fprintf(jsonFile, "  ]\n");
    fprintf(jsonFile, "}\n");

    fclose(jsonFile);

    std::cout << "Metadata saved to: " << jsonPath << std::endl;
    return true;
}

const SpriteMetadata* RIFConverter::GetSpriteMetadata(const std::string& spriteId) const {
    auto it = m_spriteIndex.find(spriteId);
    if (it != m_spriteIndex.end()) {
        return &m_sprites[it->second];
    }
    return nullptr;
}

void RIFConverter::PrintStructure() const {
    std::cout << "\n=== RIF File Structure ===" << std::endl;
    std::cout << "Total sprites: " << m_sprites.size() << std::endl;

    for (size_t i = 0; i < m_sprites.size(); ++i) {
        const auto& sprite = m_sprites[i];
        std::cout << "  [" << i << "] " << sprite.id
                  << " - " << sprite.width << "x" << sprite.height
                  << " (" << sprite.bitsPerPixel << "bpp)"
                  << " - pixels: " << sprite.pixelData.size() << std::endl;
    }
}
