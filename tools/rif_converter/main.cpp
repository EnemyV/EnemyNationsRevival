#include "rif_converter.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    std::cout << "=== Enemy Nations RIF to PNG Converter ===" << std::endl;
    std::cout << "Phase 0: Asset Preparation Tool" << std::endl << std::endl;

    // Define RIF files to convert
    struct RIFFile {
        const char* path;
        const char* description;
        const char* outputDir;
    };

    RIFFile rifFiles[] = {
        { "d:/Enemy Nations/src/enations/data/TERRAIN/terrain.rif", "Terrain sprites", "output/terrain" },
        { "d:/Enemy Nations/src/enations/data/building/building.rif", "Building sprites", "output/buildings" },
        { "d:/Enemy Nations/src/enations/data/vehicle/vehicle.rif", "Vehicle sprites", "output/vehicles" },
        { "d:/Enemy Nations/src/enations/data/UNITS/units.rif", "Unit sprites", "output/units" },
        { "d:/Enemy Nations/src/enations/data/EFFECT/effect.rif", "Effect sprites", "output/effects" },
    };

    const int numFiles = sizeof(rifFiles) / sizeof(rifFiles[0]);

    std::cout << "Found " << numFiles << " RIF files to convert:" << std::endl;
    for (int i = 0; i < numFiles; ++i) {
        std::cout << "  [" << i << "] " << rifFiles[i].description << std::endl;
        std::cout << "       Path: " << rifFiles[i].path << std::endl;
        if (fs::exists(rifFiles[i].path)) {
            auto size = fs::file_size(rifFiles[i].path);
            std::cout << "       Size: " << (size / (1024 * 1024)) << " MB" << std::endl;
        } else {
            std::cout << "       Status: NOT FOUND" << std::endl;
        }
    }

    std::cout << "\n=== Starting Conversion ===" << std::endl;

    int successCount = 0;

    for (int i = 0; i < numFiles; ++i) {
        std::cout << "\n[" << (i + 1) << "/" << numFiles << "] Processing: "
                  << rifFiles[i].description << std::endl;

        if (!fs::exists(rifFiles[i].path)) {
            std::cerr << "ERROR: File not found - " << rifFiles[i].path << std::endl;
            continue;
        }

        RIFConverter converter;

        if (converter.LoadRIF(rifFiles[i].path)) {
            converter.PrintStructure();

            if (converter.SaveAsPNG(rifFiles[i].outputDir)) {
                std::cout << "✓ Successfully converted " << rifFiles[i].description << std::endl;
                successCount++;
            } else {
                std::cerr << "✗ Failed to save " << rifFiles[i].description << std::endl;
            }
        } else {
            std::cerr << "✗ Failed to load " << rifFiles[i].path << std::endl;
        }
    }

    std::cout << "\n=== Conversion Summary ===" << std::endl;
    std::cout << "Successfully converted: " << successCount << "/" << numFiles << " files" << std::endl;

    if (successCount == numFiles) {
        std::cout << "\n✓ All assets converted successfully!" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ Some conversions failed. Check errors above." << std::endl;
        return 1;
    }
}
