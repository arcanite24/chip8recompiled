/**
 * @file rom.h
 * @brief ROM file loading and validation
 * 
 * Handles loading CHIP-8 ROM files from disk.
 */

#ifndef RECOMPILER_ROM_H
#define RECOMPILER_ROM_H

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include <optional>

namespace chip8recomp {

/* ============================================================================
 * ROM Data Structure
 * ========================================================================== */

/**
 * @brief Loaded ROM data and metadata
 */
struct Rom {
    /** ROM file path */
    std::filesystem::path path;
    
    /** ROM name (derived from filename) */
    std::string name;
    
    /** Raw ROM bytes */
    std::vector<uint8_t> data;
    
    /** ROM size in bytes */
    size_t size() const { return data.size(); }
    
    /** Check if ROM is empty */
    bool empty() const { return data.empty(); }
    
    /** Get pointer to raw data */
    const uint8_t* bytes() const { return data.data(); }
};

/* ============================================================================
 * ROM Loading
 * ========================================================================== */

/** Maximum allowed ROM size (3.5KB, leaving room for interpreter area) */
constexpr size_t MAX_ROM_SIZE = 4096 - 0x200;  // 3584 bytes

/** Minimum valid ROM size */
constexpr size_t MIN_ROM_SIZE = 2;  // At least one instruction

/**
 * @brief Load a ROM file from disk
 * 
 * @param path Path to the ROM file (.ch8)
 * @return Loaded ROM, or std::nullopt on error
 */
std::optional<Rom> load_rom(const std::filesystem::path& path);

/**
 * @brief Load a ROM from memory buffer
 * 
 * @param data Pointer to ROM data
 * @param size Size of ROM data
 * @param name Name to give the ROM
 * @return Loaded ROM, or std::nullopt on error
 */
std::optional<Rom> load_rom_from_memory(const uint8_t* data, 
                                         size_t size,
                                         const std::string& name = "rom");

/**
 * @brief Validate a loaded ROM
 * 
 * Checks:
 * - Size is within valid range
 * - Size is even (instructions are 2 bytes)
 * - Basic structure appears valid
 * 
 * @param rom ROM to validate
 * @param error_message Output: error message if validation fails
 * @return true if valid, false otherwise
 */
bool validate_rom(const Rom& rom, std::string& error_message);

/* ============================================================================
 * ROM Utilities
 * ========================================================================== */

/**
 * @brief Extract ROM name from file path
 * 
 * Removes extension and cleans up the name for use as an identifier.
 * Example: "Pong [David Winter].ch8" -> "pong"
 * 
 * @param path ROM file path
 * @return Cleaned ROM name
 */
std::string extract_rom_name(const std::filesystem::path& path);

/**
 * @brief Detect CHIP-8 variant from ROM content
 * 
 * Attempts to detect if the ROM is:
 * - Standard CHIP-8
 * - SUPER-CHIP
 * - Other variants
 * 
 * @param rom ROM to analyze
 * @return Detected variant name
 */
std::string detect_variant(const Rom& rom);

/**
 * @brief Print ROM information
 * 
 * @param rom ROM to describe
 */
void print_rom_info(const Rom& rom);

/**
 * @brief Dump ROM as hexadecimal
 * 
 * @param rom ROM to dump
 * @param bytes_per_line Bytes per line of output
 */
void dump_rom_hex(const Rom& rom, int bytes_per_line = 16);

} // namespace chip8recomp

#endif // RECOMPILER_ROM_H
