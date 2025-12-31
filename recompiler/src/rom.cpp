/**
 * @file rom.cpp
 * @brief ROM loading implementation
 */

#include "recompiler/rom.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace chip8recomp {

std::optional<Rom> load_rom(const std::filesystem::path& path) {
    // Check if file exists
    if (!std::filesystem::exists(path)) {
        std::cerr << "Error: ROM file not found: " << path << "\n";
        return std::nullopt;
    }
    
    // Get file size
    auto file_size = std::filesystem::file_size(path);
    
    if (file_size > MAX_ROM_SIZE) {
        std::cerr << "Error: ROM too large (" << file_size << " bytes, max " 
                  << MAX_ROM_SIZE << ")\n";
        return std::nullopt;
    }
    
    if (file_size < MIN_ROM_SIZE) {
        std::cerr << "Error: ROM too small (" << file_size << " bytes)\n";
        return std::nullopt;
    }
    
    // Read file
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open ROM file: " << path << "\n";
        return std::nullopt;
    }
    
    Rom rom;
    rom.path = path;
    rom.name = extract_rom_name(path);
    rom.data.resize(file_size);
    
    file.read(reinterpret_cast<char*>(rom.data.data()), file_size);
    
    if (!file) {
        std::cerr << "Error: Could not read ROM file\n";
        return std::nullopt;
    }
    
    return rom;
}

std::optional<Rom> load_rom_from_memory(const uint8_t* data, 
                                         size_t size,
                                         const std::string& name) {
    if (size > MAX_ROM_SIZE) {
        std::cerr << "Error: ROM too large\n";
        return std::nullopt;
    }
    
    if (size < MIN_ROM_SIZE) {
        std::cerr << "Error: ROM too small\n";
        return std::nullopt;
    }
    
    Rom rom;
    rom.name = name;
    rom.data.assign(data, data + size);
    
    return rom;
}

bool validate_rom(const Rom& rom, std::string& error_message) {
    if (rom.empty()) {
        error_message = "ROM is empty";
        return false;
    }
    
    if (rom.size() > MAX_ROM_SIZE) {
        error_message = "ROM is too large (" + std::to_string(rom.size()) + 
                       " bytes, max " + std::to_string(MAX_ROM_SIZE) + ")";
        return false;
    }
    
    if (rom.size() < MIN_ROM_SIZE) {
        error_message = "ROM is too small (" + std::to_string(rom.size()) + " bytes)";
        return false;
    }
    
    // Warn if size is odd (instructions are 2 bytes)
    if (rom.size() % 2 != 0) {
        std::cerr << "Warning: ROM size is odd (" << rom.size() 
                  << " bytes), last byte will be ignored\n";
    }
    
    return true;
}

std::string extract_rom_name(const std::filesystem::path& path) {
    std::string name = path.stem().string();
    
    // Remove common suffixes/metadata in brackets
    size_t bracket = name.find('[');
    if (bracket != std::string::npos) {
        name = name.substr(0, bracket);
    }
    
    // Remove parentheses
    size_t paren = name.find('(');
    if (paren != std::string::npos) {
        name = name.substr(0, paren);
    }
    
    // Trim whitespace
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    name.erase(name.begin(), std::find_if(name.begin(), name.end(), not_space));
    name.erase(std::find_if(name.rbegin(), name.rend(), not_space).base(), name.end());
    
    // Convert to lowercase
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Replace spaces and special chars with underscores
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { 
                       return std::isalnum(c) ? c : '_'; 
                   });
    
    // Remove consecutive underscores
    auto new_end = std::unique(name.begin(), name.end(),
                               [](char a, char b) { return a == '_' && b == '_'; });
    name.erase(new_end, name.end());
    
    // Remove leading/trailing underscores
    if (!name.empty() && name.front() == '_') {
        name.erase(0, 1);
    }
    if (!name.empty() && name.back() == '_') {
        name.pop_back();
    }
    
    if (name.empty()) {
        name = "rom";
    }
    
    // Ensure name starts with a letter (valid C identifier)
    // If it starts with a digit, prefix with "rom_"
    if (!name.empty() && std::isdigit(static_cast<unsigned char>(name.front()))) {
        name = "rom_" + name;
    }
    
    return name;
}

std::string detect_variant(const Rom& rom) {
    // Look for SUPER-CHIP specific opcodes
    // 00FD, 00FE, 00FF, 00Cn, 00FB, 00FC, Dxy0
    
    for (size_t i = 0; i + 1 < rom.size(); i += 2) {
        uint16_t opcode = (static_cast<uint16_t>(rom.data[i]) << 8) | rom.data[i + 1];
        
        // SUPER-CHIP specific
        if (opcode == 0x00FD || // EXIT
            opcode == 0x00FE || // LOW
            opcode == 0x00FF || // HIGH
            opcode == 0x00FB || // SCR
            opcode == 0x00FC) { // SCL
            return "SUPER-CHIP";
        }
        
        // Scroll down (00Cn)
        if ((opcode & 0xFFF0) == 0x00C0) {
            return "SUPER-CHIP";
        }
        
        // DXYN with N=0 (16x16 sprite, SUPER-CHIP only)
        if ((opcode & 0xF00F) == 0xD000) {
            return "SUPER-CHIP";
        }
        
        // FX30 - HI-res font (SUPER-CHIP)
        if ((opcode & 0xF0FF) == 0xF030) {
            return "SUPER-CHIP";
        }
        
        // FX75/FX85 - HP48 flags (SUPER-CHIP)
        if ((opcode & 0xF0FF) == 0xF075 || (opcode & 0xF0FF) == 0xF085) {
            return "SUPER-CHIP";
        }
    }
    
    return "CHIP-8";
}

void print_rom_info(const Rom& rom) {
    std::cout << "ROM Information:\n";
    std::cout << "  Name: " << rom.name << "\n";
    if (!rom.path.empty()) {
        std::cout << "  Path: " << rom.path << "\n";
    }
    std::cout << "  Size: " << rom.size() << " bytes\n";
    std::cout << "  Instructions: ~" << rom.size() / 2 << "\n";
    std::cout << "  Variant: " << detect_variant(rom) << "\n";
}

void dump_rom_hex(const Rom& rom, int bytes_per_line) {
    std::cout << std::hex << std::uppercase << std::setfill('0');
    
    for (size_t i = 0; i < rom.size(); ++i) {
        if (i % bytes_per_line == 0) {
            std::cout << std::setw(3) << (0x200 + i) << ": ";
        }
        
        std::cout << std::setw(2) << static_cast<int>(rom.data[i]);
        
        if (i % 2 == 1) {
            std::cout << " ";
        }
        
        if ((i + 1) % bytes_per_line == 0) {
            std::cout << "\n";
        }
    }
    
    if (rom.size() % bytes_per_line != 0) {
        std::cout << "\n";
    }
    
    std::cout << std::dec;
}

} // namespace chip8recomp
