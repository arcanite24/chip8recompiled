/**
 * @file main.cpp
 * @brief CHIP-8 Recompiler entry point
 * 
 * Command-line interface for the chip8recomp static recompiler.
 */

#include "recompiler/rom.h"
#include "recompiler/decoder.h"
#include "recompiler/analyzer.h"
#include "recompiler/generator.h"
#include "recompiler/config.h"
#include "recompiler/batch.h"

#include <iostream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

void print_usage(const char* program_name) {
    std::cout << "CHIP-8 Static Recompiler v0.1.0\n";
    std::cout << "Usage: " << program_name << " <rom_file> [options]\n";
    std::cout << "   or: " << program_name << " --batch <rom_dir> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o, --output <dir>     Output directory (default: current)\n";
    std::cout << "  -n, --name <name>      ROM name (default: derived from filename)\n";
    std::cout << "  -c, --config <file>    TOML configuration file\n";
    std::cout << "  --batch <dir>          Batch mode: compile all ROMs in directory\n";
    std::cout << "  --metadata <file>      JSON metadata file for batch mode\n";
    std::cout << "  --no-comments          Don't emit disassembly comments\n";
    std::cout << "  --single-function      Use single-function mode (for complex ROMs)\n";
    std::cout << "  --no-auto              Disable auto mode (don't fallback to single-function)\n";
    std::cout << "  --debug                Enable debug output\n";
    std::cout << "  --disasm               Print disassembly and exit\n";
    std::cout << "  -h, --help             Show this help message\n";
    std::cout << "\nBatch mode uses auto-mode by default: tries regular compilation first,\n";
    std::cout << "falls back to single-function mode if compilation would fail.\n";
}

void print_banner() {
    std::cout << R"(
   ____ _   _ ___ ____  ___    ____                            _ _          _ 
  / ___| | | |_ _|  _ \( _ )  |  _ \ ___  ___ ___  _ __ ___  ___ (_) | ___  __| |
 | |   | |_| || || |_) / _ \  | |_) / _ \/ __/ _ \| '_ ` _ \| '_ \| | |/ _ \/ _` |
 | |___|  _  || ||  __/ (_) | |  _ <  __/ (_| (_) | | | | | | |_) | | |  __/ (_| |
  \____|_| |_|___|_|   \___/  |_| \_\___|\___\___/|_| |_| |_| .__/|_|_|\___|\__,_|
                                                           |_|                   
)" << '\n';
}

int main(int argc, char* argv[]) {
    // Parse command line
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    std::string rom_path;
    std::string output_dir = ".";
    std::string rom_name;
    std::string config_path;
    std::string batch_dir;
    std::string metadata_file;
    bool emit_comments = true;
    bool debug_mode = false;
    bool disasm_only = false;
    bool single_function_mode = false;
    bool batch_mode = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-o" || arg == "--output") {
            if (++i >= argc) {
                std::cerr << "Error: --output requires an argument\n";
                return 1;
            }
            output_dir = argv[i];
        } else if (arg == "-n" || arg == "--name") {
            if (++i >= argc) {
                std::cerr << "Error: --name requires an argument\n";
                return 1;
            }
            rom_name = argv[i];
        } else if (arg == "-c" || arg == "--config") {
            if (++i >= argc) {
                std::cerr << "Error: --config requires an argument\n";
                return 1;
            }
            config_path = argv[i];
        } else if (arg == "--batch") {
            if (++i >= argc) {
                std::cerr << "Error: --batch requires an argument\n";
                return 1;
            }
            batch_dir = argv[i];
            batch_mode = true;
        } else if (arg == "--metadata") {
            if (++i >= argc) {
                std::cerr << "Error: --metadata requires an argument\n";
                return 1;
            }
            metadata_file = argv[i];
        } else if (arg == "--no-comments") {
            emit_comments = false;
        } else if (arg == "--debug") {
            debug_mode = true;
        } else if (arg == "--single-function") {
            single_function_mode = true;
        } else if (arg == "--no-auto") {
            /* Handled below when setting batch options */
        } else if (arg == "--disasm") {
            disasm_only = true;
        } else if (arg[0] != '-') {
            rom_path = arg;
        } else {
            std::cerr << "Error: Unknown option: " << arg << "\n";
            return 1;
        }
    }
    
    // Check for --no-auto flag
    bool auto_mode = true;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--no-auto") {
            auto_mode = false;
            break;
        }
    }
    
    print_banner();
    
    // Handle batch mode
    if (batch_mode) {
        chip8recomp::BatchOptions batch_opts;
        batch_opts.rom_dir = batch_dir;
        batch_opts.output_dir = output_dir;
        batch_opts.auto_mode = auto_mode && !single_function_mode;  // Disable auto if single-function forced
        if (!metadata_file.empty()) {
            batch_opts.metadata_file = metadata_file;
        }
        
        // Set generator options
        batch_opts.gen_opts.emit_comments = emit_comments;
        batch_opts.gen_opts.debug_mode = debug_mode;
        batch_opts.gen_opts.single_function_mode = single_function_mode;
        
        return chip8recomp::compile_batch(batch_opts);
    }
    
    // Single ROM mode
    if (rom_path.empty()) {
        std::cerr << "Error: No ROM file specified\n";
        print_usage(argv[0]);
        return 1;
    }
    
    // Load ROM
    std::cout << "Loading ROM: " << rom_path << "\n";
    auto rom = chip8recomp::load_rom(rom_path);
    if (!rom) {
        std::cerr << "Error: Failed to load ROM\n";
        return 1;
    }
    
    std::string error_msg;
    if (!chip8recomp::validate_rom(*rom, error_msg)) {
        std::cerr << "Error: Invalid ROM: " << error_msg << "\n";
        return 1;
    }
    
    if (rom_name.empty()) {
        rom_name = chip8recomp::extract_rom_name(rom_path);
    }
    
    chip8recomp::print_rom_info(*rom);
    std::cout << "\n";
    
    // Decode instructions
    std::cout << "Decoding instructions...\n";
    auto instructions = chip8recomp::decode_rom(rom->bytes(), rom->size());
    std::cout << "  Decoded " << instructions.size() << " instructions\n\n";
    
    // Disassembly only mode
    if (disasm_only) {
        std::cout << "Disassembly:\n";
        std::cout << "============\n";
        for (const auto& instr : instructions) {
            std::cout << chip8recomp::disassemble(instr) << "\n";
        }
        return 0;
    }
    
    // Analyze control flow
    std::cout << "Analyzing control flow...\n";
    auto analysis = chip8recomp::analyze(instructions);
    std::cout << "  Found " << analysis.stats.total_functions << " functions\n";
    std::cout << "  Found " << analysis.stats.total_blocks << " basic blocks\n";
    std::cout << "  " << analysis.label_addresses.size() << " labels needed\n\n";
    
    if (debug_mode) {
        chip8recomp::print_analysis_summary(analysis);
    }
    
    // Generate code
    std::cout << "Generating C code...\n";
    
    chip8recomp::GeneratorOptions gen_opts;
    gen_opts.output_prefix = rom_name;
    gen_opts.output_dir = output_dir;
    gen_opts.emit_comments = emit_comments;
    gen_opts.debug_mode = debug_mode;
    gen_opts.single_function_mode = single_function_mode;
    
    if (single_function_mode) {
        std::cout << "  Using single-function mode\n";
    }
    
    auto output = chip8recomp::generate(analysis, rom->bytes(), rom->size(), gen_opts);
    
    // Write output files
    fs::path out_path = output_dir;
    if (!fs::exists(out_path)) {
        std::cout << "Creating output directory: " << out_path << "\n";
        fs::create_directories(out_path);
    }
    
    if (!chip8recomp::write_output(output, out_path)) {
        std::cerr << "Error: Failed to write output files\n";
        return 1;
    }
    
    std::cout << "\nGenerated files:\n";
    std::cout << "  " << (out_path / output.header_file) << "\n";
    std::cout << "  " << (out_path / output.source_file) << "\n";
    std::cout << "  " << (out_path / output.main_file) << "\n";
    std::cout << "  " << (out_path / output.cmake_file) << "\n";
    if (gen_opts.embed_rom_data) {
        std::cout << "  " << (out_path / output.rom_data_file) << "\n";
    }
    
    std::cout << "\nBuild instructions:\n";
    std::cout << "  cd " << out_path << "\n";
    std::cout << "  mkdir build && cd build\n";
    std::cout << "  cmake -G Ninja ..\n";
    std::cout << "  cmake --build .\n";
    std::cout << "  ./" << rom_name << "\n";
    
    return 0;
}
