/**
 * @file generator.h
 * @brief C code generator for CHIP-8 instructions
 * 
 * Generates C source code from analyzed CHIP-8 programs.
 * The generated code uses the chip8rt runtime library.
 */

#ifndef RECOMPILER_GENERATOR_H
#define RECOMPILER_GENERATOR_H

#include "analyzer.h"
#include "config.h"
#include <string>
#include <ostream>
#include <filesystem>

namespace chip8recomp {

/* ============================================================================
 * Code Generator Options
 * ========================================================================== */

/**
 * @brief Options for code generation
 */
struct GeneratorOptions {
    // Output settings
    std::string output_prefix = "rom";       // Prefix for output files
    std::filesystem::path output_dir = ".";  // Output directory
    
    // Code style settings
    bool emit_comments = true;               // Include disassembly comments
    bool emit_address_comments = true;       // Include address comments
    bool emit_timing_calls = false;          // Insert timing checkpoints
    bool use_single_file = true;             // All code in one file vs. per-function
    bool single_function_mode = false;       // Put all code in one function (for complex ROMs)
    
    // Quirk modes (for CHIP-8 variants)
    bool quirk_shift_uses_vy = false;        // SHR/SHL use VY as source
    bool quirk_load_store_inc_i = true;      // FX55/FX65 increment I
    bool quirk_jump_uses_vx = false;         // BNNN uses VX instead of V0
    bool quirk_vf_reset = true;              // OR/AND/XOR reset VF to 0 (original CHIP-8)
    
    // ROM embedding
    bool embed_rom_data = true;              // Embed ROM for sprite data
    
    // Debug settings
    bool debug_mode = false;                 // Extra debug output in generated code
};

/* ============================================================================
 * Generated Output
 * ========================================================================== */

/**
 * @brief Result of code generation
 */
struct GeneratedOutput {
    std::string header_content;    // .h file content
    std::string source_content;    // .c file content
    std::string rom_data_content;  // rom_data.c content (embedded ROM)
    std::string main_content;      // main.c content
    std::string cmake_content;     // CMakeLists.txt content
    
    // File paths (relative to output_dir)
    std::string header_file;
    std::string source_file;
    std::string rom_data_file;
    std::string main_file;
    std::string cmake_file;
};

/* ============================================================================
 * Generator Interface
 * ========================================================================== */

/**
 * @brief Generate C code from analyzed CHIP-8 program
 * 
 * @param analysis Analysis result from analyze()
 * @param rom_data Original ROM bytes (for embedding)
 * @param rom_size ROM size in bytes
 * @param options Generator options
 * @return Generated code content
 */
GeneratedOutput generate(const AnalysisResult& analysis,
                         const uint8_t* rom_data,
                         size_t rom_size,
                         const GeneratorOptions& options);

/**
 * @brief Write generated output to files
 * 
 * Creates output directory if needed and writes all generated files.
 * 
 * @param output Generated output from generate()
 * @param output_dir Output directory path
 * @return true on success, false on failure
 */
bool write_output(const GeneratedOutput& output,
                  const std::filesystem::path& output_dir);

/* ============================================================================
 * Low-Level Generation Functions
 * ========================================================================== */

/**
 * @brief Generate C code for a single instruction
 * 
 * @param instr Instruction to generate code for
 * @param options Generator options
 * @param out Output stream
 */
void generate_instruction(const Instruction& instr,
                          const GeneratorOptions& options,
                          std::ostream& out);

/**
 * @brief Generate C code for a basic block
 * 
 * @param block Basic block to generate
 * @param instructions All program instructions
 * @param analysis Full analysis result
 * @param options Generator options
 * @param out Output stream
 */
void generate_block(const BasicBlock& block,
                    const std::vector<Instruction>& instructions,
                    const AnalysisResult& analysis,
                    const GeneratorOptions& options,
                    std::ostream& out);

/**
 * @brief Generate C function for a CHIP-8 function
 * 
 * @param func Function to generate
 * @param analysis Full analysis result
 * @param options Generator options
 * @param out Output stream
 */
void generate_function(const Function& func,
                       const AnalysisResult& analysis,
                       const GeneratorOptions& options,
                       std::ostream& out);

/**
 * @brief Generate header file content
 * 
 * @param analysis Full analysis result
 * @param options Generator options
 * @return Header file content as string
 */
std::string generate_header(const AnalysisResult& analysis,
                            const GeneratorOptions& options);

/**
 * @brief Generate main.c file content
 * 
 * @param options Generator options
 * @return main.c content as string
 */
std::string generate_main(const GeneratorOptions& options);

/**
 * @brief Generate CMakeLists.txt content
 * 
 * @param options Generator options
 * @return CMakeLists.txt content as string
 */
std::string generate_cmake(const GeneratorOptions& options);

/**
 * @brief Generate embedded ROM data file
 * 
 * @param rom_data ROM bytes
 * @param rom_size ROM size
 * @param options Generator options
 * @return rom_data.c content as string
 */
std::string generate_rom_data(const uint8_t* rom_data,
                               size_t rom_size,
                               const GeneratorOptions& options);

} // namespace chip8recomp

#endif // RECOMPILER_GENERATOR_H
