#ifndef INSTRUCTION_H_INCLUDED
#define INSTRUCTION_H_INCLUDED

#include <cstdint>

enum class Opcode : uint8_t
{
    LUI = 0b0'01101'11,
    AUIPC = 0b0'00101'11,
    ALUI = 0b0'00100'11,
    ALU = 0b0'01100'11,
    ENV = 0b0'11100'11,
    LOAD = 0b0'00000'11,
    STORE = 0b0'01000'11,
    JAL = 0b0'11011'11,
    JALR = 0b0'11001'11,
    BRANCH = 0b0'11000'11,
};

enum class InstructionType
{
    R_TYPE,
    I_TYPE,
    S_TYPE,
    U_TYPE
};

struct RInstruction
{
    uint8_t rd;
    uint8_t funct3;
    uint8_t rs1;
    uint8_t rs2;
    uint8_t funct7;
};

struct IInstruction
{
    uint8_t rd;
    uint8_t funct3;
    uint8_t rs1;
    uint16_t imm;
};

struct SInstruction
{
    uint8_t funct3;
    uint8_t rs1;
    uint8_t rs2;
    uint16_t imm;
};

struct UInstruction
{
    uint8_t rd;
    uint32_t imm;
};

struct DecodedInstruction
{
    Opcode opcode;
    union
    {
        RInstruction r;
        IInstruction i;
        SInstruction s;
        UInstruction u;
    };
};

#endif