/**
 * @file decoder.cpp
 * @brief CHIP-8 instruction decoder implementation
 */

#include "recompiler/decoder.h"
#include <sstream>
#include <iomanip>

namespace chip8recomp {

Instruction decode_opcode(uint16_t opcode, uint16_t address) {
    Instruction instr{};
    instr.address = address;
    instr.opcode = opcode;
    
    // Extract common fields
    instr.x = (opcode & 0x0F00) >> 8;
    instr.y = (opcode & 0x00F0) >> 4;
    instr.n = opcode & 0x000F;
    instr.nn = opcode & 0x00FF;
    instr.nnn = opcode & 0x0FFF;
    
    // Decode instruction type
    uint8_t op = (opcode & 0xF000) >> 12;
    
    switch (op) {
        case 0x0:
            if (opcode == 0x00E0) {
                instr.type = InstructionType::CLS;
            } else if (opcode == 0x00EE) {
                instr.type = InstructionType::RET;
                instr.is_return = true;
                instr.is_terminator = true;
            } else {
                instr.type = InstructionType::SYS;  // Ignored on modern interpreters
            }
            break;
            
        case 0x1:  // JP addr
            instr.type = InstructionType::JP;
            instr.is_jump = true;
            instr.is_terminator = true;
            break;
            
        case 0x2:  // CALL addr
            instr.type = InstructionType::CALL;
            instr.is_call = true;
            break;
            
        case 0x3:  // SE Vx, byte
            instr.type = InstructionType::SE_VX_NN;
            instr.is_branch = true;
            break;
            
        case 0x4:  // SNE Vx, byte
            instr.type = InstructionType::SNE_VX_NN;
            instr.is_branch = true;
            break;
            
        case 0x5:  // SE Vx, Vy
            if (instr.n == 0) {
                instr.type = InstructionType::SE_VX_VY;
                instr.is_branch = true;
            } else {
                instr.type = InstructionType::UNKNOWN;
            }
            break;
            
        case 0x6:  // LD Vx, byte
            instr.type = InstructionType::LD_VX_NN;
            break;
            
        case 0x7:  // ADD Vx, byte
            instr.type = InstructionType::ADD_VX_NN;
            break;
            
        case 0x8:  // Arithmetic/logic
            switch (instr.n) {
                case 0x0: instr.type = InstructionType::LD_VX_VY; break;
                case 0x1: instr.type = InstructionType::OR_VX_VY; break;
                case 0x2: instr.type = InstructionType::AND_VX_VY; break;
                case 0x3: instr.type = InstructionType::XOR_VX_VY; break;
                case 0x4: instr.type = InstructionType::ADD_VX_VY; break;
                case 0x5: instr.type = InstructionType::SUB_VX_VY; break;
                case 0x6: instr.type = InstructionType::SHR_VX; break;
                case 0x7: instr.type = InstructionType::SUBN_VX_VY; break;
                case 0xE: instr.type = InstructionType::SHL_VX; break;
                default:  instr.type = InstructionType::UNKNOWN; break;
            }
            break;
            
        case 0x9:  // SNE Vx, Vy
            if (instr.n == 0) {
                instr.type = InstructionType::SNE_VX_VY;
                instr.is_branch = true;
            } else {
                instr.type = InstructionType::UNKNOWN;
            }
            break;
            
        case 0xA:  // LD I, addr
            instr.type = InstructionType::LD_I_NNN;
            break;
            
        case 0xB:  // JP V0, addr
            instr.type = InstructionType::JP_V0;
            instr.is_jump = true;
            instr.is_terminator = true;
            break;
            
        case 0xC:  // RND Vx, byte
            instr.type = InstructionType::RND;
            break;
            
        case 0xD:  // DRW Vx, Vy, nibble
            instr.type = InstructionType::DRW;
            break;
            
        case 0xE:  // Skip key
            if (instr.nn == 0x9E) {
                instr.type = InstructionType::SKP;
                instr.is_branch = true;
            } else if (instr.nn == 0xA1) {
                instr.type = InstructionType::SKNP;
                instr.is_branch = true;
            } else {
                instr.type = InstructionType::UNKNOWN;
            }
            break;
            
        case 0xF:  // Misc
            switch (instr.nn) {
                case 0x07: instr.type = InstructionType::LD_VX_DT; break;
                case 0x0A: instr.type = InstructionType::LD_VX_K; break;
                case 0x15: instr.type = InstructionType::LD_DT_VX; break;
                case 0x18: instr.type = InstructionType::LD_ST_VX; break;
                case 0x1E: instr.type = InstructionType::ADD_I_VX; break;
                case 0x29: instr.type = InstructionType::LD_F_VX; break;
                case 0x33: instr.type = InstructionType::LD_B_VX; break;
                case 0x55: instr.type = InstructionType::LD_I_VX; break;
                case 0x65: instr.type = InstructionType::LD_VX_I; break;
                default:   instr.type = InstructionType::UNKNOWN; break;
            }
            break;
            
        default:
            instr.type = InstructionType::UNKNOWN;
            break;
    }
    
    return instr;
}

std::vector<Instruction> decode_rom(const uint8_t* rom_data, 
                                     size_t rom_size, 
                                     uint16_t base_address) {
    std::vector<Instruction> instructions;
    instructions.reserve(rom_size / 2);
    
    for (size_t i = 0; i + 1 < rom_size; i += 2) {
        uint16_t opcode = (static_cast<uint16_t>(rom_data[i]) << 8) | rom_data[i + 1];
        uint16_t address = base_address + static_cast<uint16_t>(i);
        instructions.push_back(decode_opcode(opcode, address));
    }
    
    return instructions;
}

const char* instruction_mnemonic(InstructionType type) {
    switch (type) {
        case InstructionType::SYS:        return "SYS";
        case InstructionType::CLS:        return "CLS";
        case InstructionType::RET:        return "RET";
        case InstructionType::JP:         return "JP";
        case InstructionType::CALL:       return "CALL";
        case InstructionType::JP_V0:      return "JP V0,";
        case InstructionType::SE_VX_NN:   return "SE";
        case InstructionType::SNE_VX_NN:  return "SNE";
        case InstructionType::SE_VX_VY:   return "SE";
        case InstructionType::SNE_VX_VY:  return "SNE";
        case InstructionType::SKP:        return "SKP";
        case InstructionType::SKNP:       return "SKNP";
        case InstructionType::LD_VX_NN:   return "LD";
        case InstructionType::LD_VX_VY:   return "LD";
        case InstructionType::LD_I_NNN:   return "LD";
        case InstructionType::LD_VX_DT:   return "LD";
        case InstructionType::LD_VX_K:    return "LD";
        case InstructionType::LD_DT_VX:   return "LD";
        case InstructionType::LD_ST_VX:   return "LD";
        case InstructionType::LD_F_VX:    return "LD";
        case InstructionType::LD_B_VX:    return "LD";
        case InstructionType::LD_I_VX:    return "LD";
        case InstructionType::LD_VX_I:    return "LD";
        case InstructionType::ADD_VX_NN:  return "ADD";
        case InstructionType::ADD_VX_VY:  return "ADD";
        case InstructionType::ADD_I_VX:   return "ADD";
        case InstructionType::SUB_VX_VY:  return "SUB";
        case InstructionType::SUBN_VX_VY: return "SUBN";
        case InstructionType::OR_VX_VY:   return "OR";
        case InstructionType::AND_VX_VY:  return "AND";
        case InstructionType::XOR_VX_VY:  return "XOR";
        case InstructionType::SHR_VX:     return "SHR";
        case InstructionType::SHL_VX:     return "SHL";
        case InstructionType::RND:        return "RND";
        case InstructionType::DRW:        return "DRW";
        case InstructionType::UNKNOWN:    return "???";
    }
    return "???";
}

std::string disassemble(const Instruction& instr) {
    std::ostringstream ss;
    
    // Address and opcode
    ss << std::hex << std::uppercase << std::setfill('0');
    ss << std::setw(3) << instr.address << ": ";
    ss << std::setw(4) << instr.opcode << "  ";
    
    // Mnemonic and operands
    ss << std::setfill(' ') << std::left << std::setw(5) 
       << instruction_mnemonic(instr.type);
    
    switch (instr.type) {
        case InstructionType::CLS:
        case InstructionType::RET:
            // No operands
            break;
            
        case InstructionType::JP:
        case InstructionType::CALL:
            ss << "0x" << std::hex << instr.nnn;
            break;
            
        case InstructionType::JP_V0:
            ss << "V0, 0x" << std::hex << instr.nnn;
            break;
            
        case InstructionType::SE_VX_NN:
        case InstructionType::SNE_VX_NN:
        case InstructionType::LD_VX_NN:
        case InstructionType::ADD_VX_NN:
        case InstructionType::RND:
            ss << "V" << std::hex << (int)instr.x << ", 0x" << (int)instr.nn;
            break;
            
        case InstructionType::SE_VX_VY:
        case InstructionType::SNE_VX_VY:
        case InstructionType::LD_VX_VY:
        case InstructionType::OR_VX_VY:
        case InstructionType::AND_VX_VY:
        case InstructionType::XOR_VX_VY:
        case InstructionType::ADD_VX_VY:
        case InstructionType::SUB_VX_VY:
        case InstructionType::SUBN_VX_VY:
            ss << "V" << std::hex << (int)instr.x << ", V" << (int)instr.y;
            break;
            
        case InstructionType::SHR_VX:
        case InstructionType::SHL_VX:
        case InstructionType::SKP:
        case InstructionType::SKNP:
            ss << "V" << std::hex << (int)instr.x;
            break;
            
        case InstructionType::LD_I_NNN:
            ss << "I, 0x" << std::hex << instr.nnn;
            break;
            
        case InstructionType::DRW:
            ss << "V" << std::hex << (int)instr.x 
               << ", V" << (int)instr.y 
               << ", " << std::dec << (int)instr.n;
            break;
            
        case InstructionType::LD_VX_DT:
            ss << "V" << std::hex << (int)instr.x << ", DT";
            break;
            
        case InstructionType::LD_VX_K:
            ss << "V" << std::hex << (int)instr.x << ", K";
            break;
            
        case InstructionType::LD_DT_VX:
            ss << "DT, V" << std::hex << (int)instr.x;
            break;
            
        case InstructionType::LD_ST_VX:
            ss << "ST, V" << std::hex << (int)instr.x;
            break;
            
        case InstructionType::LD_F_VX:
            ss << "F, V" << std::hex << (int)instr.x;
            break;
            
        case InstructionType::LD_B_VX:
            ss << "B, V" << std::hex << (int)instr.x;
            break;
            
        case InstructionType::LD_I_VX:
            ss << "[I], V" << std::hex << (int)instr.x;
            break;
            
        case InstructionType::LD_VX_I:
            ss << "V" << std::hex << (int)instr.x << ", [I]";
            break;
            
        case InstructionType::ADD_I_VX:
            ss << "I, V" << std::hex << (int)instr.x;
            break;
            
        case InstructionType::SYS:
            ss << "0x" << std::hex << instr.nnn << " (ignored)";
            break;
            
        case InstructionType::UNKNOWN:
            ss << "(unknown)";
            break;
    }
    
    return ss.str();
}

} // namespace chip8recomp
