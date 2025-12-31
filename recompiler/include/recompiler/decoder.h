/**
 * @file decoder.h
 * @brief CHIP-8 instruction decoder
 * 
 * Decodes 2-byte CHIP-8 opcodes into structured instruction representations.
 */

#ifndef RECOMPILER_DECODER_H
#define RECOMPILER_DECODER_H

#include <cstdint>
#include <string>
#include <vector>

namespace chip8recomp {

/* ============================================================================
 * Instruction Types
 * ========================================================================== */

/**
 * @brief CHIP-8 instruction categories
 */
enum class InstructionType {
    // System
    SYS,        // 0NNN - System call (ignored)
    CLS,        // 00E0 - Clear screen
    RET,        // 00EE - Return from subroutine
    
    // Jumps and calls
    JP,         // 1NNN - Jump to address
    CALL,       // 2NNN - Call subroutine
    JP_V0,      // BNNN - Jump to V0 + address
    
    // Skip instructions
    SE_VX_NN,   // 3XNN - Skip if Vx == NN
    SNE_VX_NN,  // 4XNN - Skip if Vx != NN
    SE_VX_VY,   // 5XY0 - Skip if Vx == Vy
    SNE_VX_VY,  // 9XY0 - Skip if Vx != Vy
    SKP,        // EX9E - Skip if key Vx pressed
    SKNP,       // EXA1 - Skip if key Vx not pressed
    
    // Register loads
    LD_VX_NN,   // 6XNN - Load immediate
    LD_VX_VY,   // 8XY0 - Copy register
    LD_I_NNN,   // ANNN - Load I register
    LD_VX_DT,   // FX07 - Load Vx from delay timer
    LD_VX_K,    // FX0A - Wait for key press
    LD_DT_VX,   // FX15 - Set delay timer
    LD_ST_VX,   // FX18 - Set sound timer
    LD_F_VX,    // FX29 - Set I to font sprite
    LD_B_VX,    // FX33 - Store BCD
    LD_I_VX,    // FX55 - Store registers to memory
    LD_VX_I,    // FX65 - Load registers from memory
    
    // Arithmetic
    ADD_VX_NN,  // 7XNN - Add immediate
    ADD_VX_VY,  // 8XY4 - Add with carry
    SUB_VX_VY,  // 8XY5 - Subtract with borrow
    SUBN_VX_VY, // 8XY7 - Subtract reverse
    ADD_I_VX,   // FX1E - Add Vx to I
    
    // Bitwise
    OR_VX_VY,   // 8XY1 - OR
    AND_VX_VY,  // 8XY2 - AND
    XOR_VX_VY,  // 8XY3 - XOR
    SHR_VX,     // 8XY6 - Shift right
    SHL_VX,     // 8XYE - Shift left
    
    // Other
    RND,        // CXNN - Random AND
    DRW,        // DXYN - Draw sprite
    
    // Invalid/unknown
    UNKNOWN
};

/**
 * @brief Decoded CHIP-8 instruction
 */
struct Instruction {
    uint16_t address;           // Address in ROM
    uint16_t opcode;            // Raw 2-byte opcode
    InstructionType type;       // Decoded type
    
    // Operands (depending on type)
    uint8_t x;                  // Register X (nibble 2)
    uint8_t y;                  // Register Y (nibble 3)
    uint8_t n;                  // 4-bit immediate (nibble 4)
    uint8_t nn;                 // 8-bit immediate (lower byte)
    uint16_t nnn;               // 12-bit address (lower 12 bits)
    
    // Flags for analysis
    bool is_jump;               // Changes control flow unconditionally
    bool is_branch;             // Conditional skip
    bool is_call;               // Subroutine call
    bool is_return;             // Subroutine return
    bool is_terminator;         // Ends a basic block
};

/* ============================================================================
 * Decoder Interface
 * ========================================================================== */

/**
 * @brief Decode a single 2-byte opcode
 * 
 * @param opcode The 16-bit opcode
 * @param address The address of this instruction in the ROM
 * @return Decoded instruction
 */
Instruction decode_opcode(uint16_t opcode, uint16_t address);

/**
 * @brief Decode an entire ROM
 * 
 * @param rom_data ROM bytes
 * @param rom_size Size of ROM in bytes
 * @param base_address Starting address (typically 0x200)
 * @return Vector of decoded instructions
 */
std::vector<Instruction> decode_rom(const uint8_t* rom_data, 
                                     size_t rom_size, 
                                     uint16_t base_address = 0x200);

/**
 * @brief Get a human-readable disassembly of an instruction
 * 
 * @param instr The decoded instruction
 * @return Disassembly string (e.g., "LD VA, 0x05")
 */
std::string disassemble(const Instruction& instr);

/**
 * @brief Get the mnemonic for an instruction type
 * 
 * @param type Instruction type
 * @return Mnemonic string (e.g., "LD", "ADD", "JP")
 */
const char* instruction_mnemonic(InstructionType type);

} // namespace chip8recomp

#endif // RECOMPILER_DECODER_H
