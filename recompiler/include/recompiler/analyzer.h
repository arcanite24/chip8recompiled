/**
 * @file analyzer.h
 * @brief Control flow analysis for CHIP-8 programs
 * 
 * Analyzes decoded instructions to identify functions, basic blocks,
 * and jump targets for optimal code generation.
 */

#ifndef RECOMPILER_ANALYZER_H
#define RECOMPILER_ANALYZER_H

#include "decoder.h"
#include <cstdint>
#include <map>
#include <set>
#include <vector>
#include <string>

namespace chip8recomp {

/* ============================================================================
 * Control Flow Structures
 * ========================================================================== */

/**
 * @brief A basic block - sequence of instructions without branches
 * 
 * A basic block is a maximal sequence of instructions where:
 * - Control flow enters only at the first instruction
 * - Control flow leaves only at the last instruction
 */
struct BasicBlock {
    uint16_t start_address;     // Address of first instruction
    uint16_t end_address;       // Address after last instruction
    
    std::vector<uint16_t> instruction_indices;  // Indices into instruction list
    
    // Successors (where control can go after this block)
    std::vector<uint16_t> successors;
    
    // Predecessors (where control can come from)
    std::vector<uint16_t> predecessors;
    
    // Labels needed within this block (for skip instructions)
    std::set<uint16_t> internal_labels;
    
    // Is this block the entry to a function?
    bool is_function_entry = false;
    
    // Is this block reachable from the entry point?
    bool is_reachable = false;
};

/**
 * @brief A function - collection of basic blocks with a single entry point
 * 
 * Functions are identified by CALL instructions (2NNN).
 */
struct Function {
    std::string name;           // Generated function name
    uint16_t entry_address;     // Entry point address
    
    // All basic blocks belonging to this function
    std::vector<uint16_t> block_addresses;
    
    // Labels needed at the start of this function
    bool needs_entry_label = false;
    
    // Can this function be reached via computed jump (BNNN)?
    bool is_computed_target = false;
};

/* ============================================================================
 * Analysis Result
 * ========================================================================== */

/**
 * @brief Complete control flow analysis result
 */
struct AnalysisResult {
    // Original instructions
    std::vector<Instruction> instructions;
    
    // Basic blocks indexed by start address
    std::map<uint16_t, BasicBlock> blocks;
    
    // Functions indexed by entry address
    std::map<uint16_t, Function> functions;
    
    // All addresses that need labels (jump/branch targets)
    std::set<uint16_t> label_addresses;
    
    // All addresses that are CALL targets (function entry points)
    std::set<uint16_t> call_targets;
    
    // Addresses that are computed jump (BNNN) targets
    std::set<uint16_t> computed_jump_bases;
    
    // Entry point of the program
    uint16_t entry_point = 0x200;
    
    // Statistics
    struct {
        size_t total_instructions = 0;
        size_t total_blocks = 0;
        size_t total_functions = 0;
        size_t unreachable_instructions = 0;
    } stats;
};

/* ============================================================================
 * Analyzer Interface
 * ========================================================================== */

/**
 * @brief Analyze control flow of a decoded ROM
 * 
 * Performs the following analysis:
 * 1. Identifies all jump/branch targets (labels)
 * 2. Builds basic blocks
 * 3. Identifies function boundaries (CALL targets)
 * 4. Computes reachability
 * 
 * @param instructions Decoded instructions from decode_rom()
 * @param entry_point Entry point address (typically 0x200)
 * @return Analysis result with blocks, functions, and labels
 */
AnalysisResult analyze(const std::vector<Instruction>& instructions,
                       uint16_t entry_point = 0x200);

/**
 * @brief Generate a unique function name for an address
 * 
 * @param address Function entry address
 * @param prefix Optional prefix (e.g., ROM name)
 * @return Function name (e.g., "func_0x200")
 */
std::string generate_function_name(uint16_t address, 
                                    const std::string& prefix = "");

/**
 * @brief Generate a label name for an address
 * 
 * @param address Label address
 * @return Label name (e.g., "label_0x210")
 */
std::string generate_label_name(uint16_t address);

/**
 * @brief Print analysis summary to stdout (for debugging)
 * 
 * @param result Analysis result
 */
void print_analysis_summary(const AnalysisResult& result);

/* ============================================================================
 * Utility Functions
 * ========================================================================== */

/**
 * @brief Check if an instruction at given address might be data (not code)
 * 
 * Heuristic check for sprite data, etc.
 * 
 * @param result Analysis result
 * @param address Address to check
 * @return true if likely data, false if likely code
 */
bool is_likely_data(const AnalysisResult& result, uint16_t address);

/**
 * @brief Find all possible targets of a computed jump (BNNN)
 * 
 * Analyzes the possible values of V0 to determine jump targets.
 * This is a heuristic and may not find all targets.
 * 
 * @param result Analysis result
 * @param base_address Base address from BNNN instruction
 * @return Set of possible target addresses
 */
std::set<uint16_t> find_computed_jump_targets(const AnalysisResult& result,
                                               uint16_t base_address);

} // namespace chip8recomp

#endif // RECOMPILER_ANALYZER_H
