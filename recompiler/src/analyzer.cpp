/**
 * @file analyzer.cpp
 * @brief Control flow analysis implementation
 */

#include "recompiler/analyzer.h"
#include <iostream>
#include <algorithm>
#include <queue>
#include <sstream>
#include <iomanip>

namespace chip8recomp {

std::string generate_function_name(uint16_t address, const std::string& prefix) {
    std::ostringstream ss;
    if (!prefix.empty()) {
        ss << prefix << "_";
    }
    ss << "func_0x" << std::hex << std::uppercase << std::setfill('0') 
       << std::setw(3) << address;
    return ss.str();
}

std::string generate_label_name(uint16_t address) {
    std::ostringstream ss;
    ss << "label_0x" << std::hex << std::uppercase << std::setfill('0') 
       << std::setw(3) << address;
    return ss.str();
}

AnalysisResult analyze(const std::vector<Instruction>& instructions,
                       uint16_t entry_point) {
    AnalysisResult result;
    result.instructions = instructions;
    result.entry_point = entry_point;
    result.stats.total_instructions = instructions.size();
    
    if (instructions.empty()) {
        return result;
    }
    
    // Build address-to-index map
    std::map<uint16_t, size_t> addr_to_idx;
    for (size_t i = 0; i < instructions.size(); ++i) {
        addr_to_idx[instructions[i].address] = i;
    }
    
    // Pass 1: Identify all jump/branch targets and call targets
    result.call_targets.insert(entry_point);  // Entry point is a function
    
    for (const auto& instr : instructions) {
        switch (instr.type) {
            case InstructionType::JP:
                result.label_addresses.insert(instr.nnn);
                break;
                
            case InstructionType::CALL:
                result.call_targets.insert(instr.nnn);
                result.label_addresses.insert(instr.nnn);
                break;
                
            case InstructionType::JP_V0:
                // Computed jump - need special handling
                result.computed_jump_bases.insert(instr.nnn);
                break;
                
            case InstructionType::SE_VX_NN:
            case InstructionType::SNE_VX_NN:
            case InstructionType::SE_VX_VY:
            case InstructionType::SNE_VX_VY:
            case InstructionType::SKP:
            case InstructionType::SKNP:
                // Skip instructions - both the next instruction AND the skip target need labels
                result.label_addresses.insert(instr.address + 2);  // The instruction that might be skipped
                result.label_addresses.insert(instr.address + 4);  // The skip target
                break;
                
            default:
                break;
        }
    }
    
    // Pass 2: Build basic blocks
    std::set<uint16_t> block_starts;
    block_starts.insert(entry_point);
    block_starts.insert(result.label_addresses.begin(), result.label_addresses.end());
    block_starts.insert(result.call_targets.begin(), result.call_targets.end());
    
    // Also start new blocks after terminators
    for (const auto& instr : instructions) {
        if (instr.is_terminator && addr_to_idx.count(instr.address + 2)) {
            block_starts.insert(instr.address + 2);
        }
    }
    
    // Create blocks
    for (uint16_t start_addr : block_starts) {
        if (!addr_to_idx.count(start_addr)) {
            continue;  // Address not in ROM
        }
        
        BasicBlock block;
        block.start_address = start_addr;
        block.is_function_entry = result.call_targets.count(start_addr) > 0;
        
        size_t idx = addr_to_idx[start_addr];
        while (idx < instructions.size()) {
            const auto& instr = instructions[idx];
            
            // Check if this instruction belongs to a different block
            if (instr.address != start_addr && block_starts.count(instr.address)) {
                break;
            }
            
            block.instruction_indices.push_back(idx);
            block.end_address = instr.address + 2;
            
            // Determine successors
            if (instr.is_jump) {
                if (instr.type == InstructionType::JP) {
                    block.successors.push_back(instr.nnn);
                }
                // JP_V0 handled separately
                break;
            } else if (instr.is_return) {
                break;  // No successors
            } else if (instr.is_branch) {
                // Skip instructions have two successors
                block.successors.push_back(instr.address + 2);  // Next instruction
                block.successors.push_back(instr.address + 4);  // Skip target
                block.internal_labels.insert(instr.address + 4);
                break;
            } else if (instr.is_terminator) {
                break;
            }
            
            ++idx;
        }
        
        // Fall-through successor
        if (!block.instruction_indices.empty()) {
            size_t last_idx = block.instruction_indices.back();
            const auto& last_instr = instructions[last_idx];
            if (!last_instr.is_terminator && !last_instr.is_return && 
                addr_to_idx.count(block.end_address)) {
                block.successors.push_back(block.end_address);
            }
        }
        
        result.blocks[start_addr] = block;
    }
    
    result.stats.total_blocks = result.blocks.size();
    
    // Pass 3: Build predecessor lists
    for (auto& [addr, block] : result.blocks) {
        for (uint16_t succ : block.successors) {
            if (result.blocks.count(succ)) {
                result.blocks[succ].predecessors.push_back(addr);
            }
        }
    }
    
    // Pass 4: Mark reachable blocks (BFS from entry point)
    std::queue<uint16_t> worklist;
    worklist.push(entry_point);
    
    // Also add all call targets as reachable
    for (uint16_t target : result.call_targets) {
        worklist.push(target);
    }
    
    while (!worklist.empty()) {
        uint16_t addr = worklist.front();
        worklist.pop();
        
        if (!result.blocks.count(addr)) continue;
        if (result.blocks[addr].is_reachable) continue;
        
        result.blocks[addr].is_reachable = true;
        
        for (uint16_t succ : result.blocks[addr].successors) {
            worklist.push(succ);
        }
    }
    
    // Pass 5: Create function entries
    for (uint16_t target : result.call_targets) {
        if (!result.blocks.count(target)) continue;
        
        Function func;
        func.name = generate_function_name(target);
        func.entry_address = target;
        
        // Simple approach: assign blocks starting from entry until we hit
        // another function or return. More sophisticated analysis could
        // use dominance trees.
        std::set<uint16_t> visited;
        std::queue<uint16_t> func_worklist;
        func_worklist.push(target);
        
        while (!func_worklist.empty()) {
            uint16_t block_addr = func_worklist.front();
            func_worklist.pop();
            
            if (visited.count(block_addr)) continue;
            if (!result.blocks.count(block_addr)) continue;
            
            // Don't cross into other functions (except entry)
            if (block_addr != target && result.call_targets.count(block_addr)) {
                continue;
            }
            
            visited.insert(block_addr);
            func.block_addresses.push_back(block_addr);
            
            const auto& block = result.blocks[block_addr];
            for (uint16_t succ : block.successors) {
                func_worklist.push(succ);
            }
        }
        
        result.functions[target] = func;
    }
    
    result.stats.total_functions = result.functions.size();
    
    // Count unreachable instructions
    for (const auto& [addr, block] : result.blocks) {
        if (!block.is_reachable) {
            result.stats.unreachable_instructions += block.instruction_indices.size();
        }
    }
    
    return result;
}

void print_analysis_summary(const AnalysisResult& result) {
    std::cout << "\n=== Analysis Summary ===\n\n";
    
    std::cout << "Statistics:\n";
    std::cout << "  Total instructions: " << result.stats.total_instructions << "\n";
    std::cout << "  Total basic blocks: " << result.stats.total_blocks << "\n";
    std::cout << "  Total functions: " << result.stats.total_functions << "\n";
    std::cout << "  Unreachable instructions: " << result.stats.unreachable_instructions << "\n";
    std::cout << "\n";
    
    std::cout << "Functions:\n";
    for (const auto& [addr, func] : result.functions) {
        std::cout << "  " << func.name << " @ 0x" << std::hex << addr << std::dec
                  << " (" << func.block_addresses.size() << " blocks)\n";
    }
    std::cout << "\n";
    
    std::cout << "Labels needed: " << result.label_addresses.size() << "\n";
    for (uint16_t addr : result.label_addresses) {
        std::cout << "  " << generate_label_name(addr) << "\n";
    }
    std::cout << "\n";
    
    if (!result.computed_jump_bases.empty()) {
        std::cout << "Computed jumps (JP V0):\n";
        for (uint16_t base : result.computed_jump_bases) {
            std::cout << "  Base 0x" << std::hex << base << std::dec << "\n";
        }
        std::cout << "\n";
    }
}

bool is_likely_data(const AnalysisResult& result, uint16_t address) {
    // Check if address is in any reachable block
    for (const auto& [addr, block] : result.blocks) {
        if (!block.is_reachable) continue;
        if (address >= block.start_address && address < block.end_address) {
            return false;  // It's code
        }
    }
    return true;  // Probably data
}

std::set<uint16_t> find_computed_jump_targets(const AnalysisResult& /*result*/,
                                               uint16_t base_address) {
    // Simple heuristic: assume V0 can be 0, 2, 4, ... up to some limit
    // A more sophisticated analysis would track V0's value
    std::set<uint16_t> targets;
    
    // Common pattern: jump table with 2-byte entries
    for (int i = 0; i < 16; ++i) {
        targets.insert(base_address + i * 2);
    }
    
    return targets;
}

} // namespace chip8recomp
