/**
 * @file batch.h
 * @brief Batch recompilation for multiple ROMs
 */

#ifndef RECOMPILER_BATCH_H
#define RECOMPILER_BATCH_H

#include "generator.h"
#include <string>
#include <vector>
#include <filesystem>

namespace chip8recomp {

/**
 * @brief ROM metadata for batch compilation
 */
struct RomMetadata {
    std::string name;              // Identifier (filename without extension)
    std::string title;             // Display title
    std::string description;       // Optional description
    std::string authors;           // Optional author(s)
    std::string release;           // Optional release date
    int recommended_cpu_freq = 0;  // Recommended CPU frequency (0 = default)
    size_t rom_size = 0;           // Size of ROM data in bytes
};

/**
 * @brief Batch compilation options
 */
struct BatchOptions {
    std::filesystem::path rom_dir;      // Directory containing ROMs
    std::filesystem::path output_dir;   // Output directory
    std::filesystem::path metadata_file; // Optional metadata JSON
    GeneratorOptions gen_opts;          // Generator options for each ROM
    bool auto_mode = true;              // Try regular first, fallback to single-function
};

/**
 * @brief Compile all ROMs in a directory into a multi-ROM executable
 * 
 * @param options Batch compilation options
 * @return 0 on success, non-zero on error
 */
int compile_batch(const BatchOptions& options);

} // namespace chip8recomp

#endif // RECOMPILER_BATCH_H
