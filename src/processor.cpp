#include "processor.h"
#include <cmath>
#include <cstring>
#include <iostream>

enum AluFunct : uint16_t
{
    // RV32I
    ADD = 0b000000'00000'00'000,
    SUB = 0b000000'01000'00'000,
    SLL = 0b000000'00000'00'001,
    SLT = 0b000000'00000'00'010,
    SLTU = 0b000000'00000'00'011,
    XOR = 0b000000'00000'00'100,
    SRL = 0b000000'00000'00'101,
    SRA = 0b000000'01000'00'101,
    OR = 0b000000'00000'00'110,
    AND = 0b000000'00000'00'111,

    // RV32M
    MUL = 0b000000'00000'01'000,
    MULH = 0b000000'00000'01'001,
    MULHSU = 0b000000'00000'01'010,
    MULHU = 0b000000'00000'01'011,
    DIV = 0b000000'00000'01'100,
    DIVU = 0b000000'00000'01'101,
    REM = 0b000000'00000'01'110,
    REMU = 0b000000'00000'01'111,
};

enum class LoadFunct : uint8_t
{
    LB = 0b00000'000,
    LH = 0b00000'001,
    LW = 0b00000'010,
    LBU = 0b00000'100,
    LHU = 0b00000'101,
};

enum class StoreFunct : uint8_t
{
    SB = 0b00000'000,
    SH = 0b00000'001,
    SW = 0b00000'010,
};

enum class BranchFunct : uint8_t
{
    BEQ = 0b00000'000,
    BNE = 0b00000'001,
    BLT = 0b00000'100,
    BGE = 0b00000'101,
    BLTU = 0b00000'110,
    BGEU = 0b00000'111,
};

enum class Ecall : Word
{
    EXIT = 10,
    PRINT_STR = 4,
};

static SignedWord sign_extend(Word w, uint8_t len)
{
    constexpr size_t word_len = sizeof(Word) * 8;
    assert(len < word_len);
    SignedWord sign = (SignedWord)1 << (len - 1);
    w = (w << (word_len - len)) >> (word_len - len);
    return (w ^ sign) - sign;
}

Processor::Processor()
{
    for (size_t i = 1; i < REGISTER_FILE_SIZE; i++)
        rfile.write(i, 0);
    pc = 0;
}

Processor::~Processor()
{
}

void Processor::run(Memory& mem)
{
    abort = false;
    while (!abort) {
        if (pc + sizeof(Word) > mem.size()) {
            std::cerr << "PC outside of program memory." << std::endl;
            raise_exception(mem, ExceptionType::FATAL);
            continue;
        }

        Word instruction = fetch(mem);
        DecodedInstruction decoded = decode(instruction);
        execute(decoded, mem);
    }
}

Word Processor::fetch(const Memory& rom)
{
    return rom.get<Word>(pc);
}

template<typename T>
constexpr T extract_bits(T i, uint8_t start, uint8_t end)
{
    assert(start < end);
    return (i >> start) & ((1 << (end - start)) - 1);
}

DecodedInstruction Processor::decode(Word instruction) const
{
    instruction = le32toh(instruction);
    DecodedInstruction decoded;
    decoded.opcode = (Opcode)(instruction & 127);
    switch (decoded.opcode) {
        case Opcode::ALU:
            decoded.r.rd = extract_bits(instruction, 7, 12);
            decoded.r.funct3 = extract_bits(instruction, 12, 15);
            decoded.r.rs1 = extract_bits(instruction, 15, 20);
            decoded.r.rs2 = extract_bits(instruction, 20, 25);
            decoded.r.funct7 = extract_bits(instruction, 25, 32);
            break;

        case Opcode::ALUI:
        case Opcode::ENV:
        case Opcode::LOAD:
        case Opcode::JALR:
            decoded.i.rd = extract_bits(instruction, 7, 12);
            decoded.i.funct3 = extract_bits(instruction, 12, 15);
            decoded.i.rs1 = extract_bits(instruction, 15, 20);
            decoded.i.imm = extract_bits(instruction, 20, 32);
            break;

        case Opcode::STORE:
        case Opcode::BRANCH: {
            uint16_t imm_lower = extract_bits(instruction, 7, 12);
            decoded.s.funct3 = extract_bits(instruction, 12, 15);
            decoded.s.rs1 = extract_bits(instruction, 15, 20);
            decoded.s.rs2 = extract_bits(instruction, 20, 25);
            uint16_t imm_upper = extract_bits(instruction, 25, 32);
            decoded.s.imm = imm_lower | (imm_upper << 5);
            break;
        }

        case Opcode::LUI:
        case Opcode::AUIPC:
        case Opcode::JAL:
            decoded.u.rd = extract_bits(instruction, 7, 12);
            decoded.u.imm = extract_bits(instruction, 12, 32);
            break;
    }
    return decoded;
}

static Word alu(AluFunct funct, Word op1, Word op2)
{
    SignedWord sop1, sop2;
    memcpy(&sop1, &op1, sizeof(Word));
    memcpy(&sop2, &op2, sizeof(Word));
    switch (funct) {
        // RV32I
        case AluFunct::ADD:
            return op1 + op2;
        case AluFunct::SUB:
            return op1 - op2;
        case AluFunct::SLL:
            return op1 << op2;
        case AluFunct::SLT:
            return sop1 < sop2;
        case AluFunct::SLTU:
            return op1 < op2;
        case AluFunct::XOR:
            return op1 ^ op2;
        case AluFunct::SRL:
            return op1 >> (op2 & 31);
        case AluFunct::SRA:
            return sop1 >> (op2 & 31);
        case AluFunct::OR:
            return op1 | op2;
        case AluFunct::AND:
            return op1 & op2;

        // RV32M
        case AluFunct::MUL:
            return op1 * op2;
        case AluFunct::MULH:
            return (Word)(((SignedLongWord)sop1 * (SignedLongWord)sop2) >> (sizeof(Word) * 8));
        case AluFunct::MULHSU:
            return (Word)(((SignedLongWord)sop1 * (LongWord)op2) >> (sizeof(Word) * 8));
        case AluFunct::MULHU:
            return (Word)(((LongWord)op1 * (LongWord)op2) >> (sizeof(Word) * 8));
        case AluFunct::DIV:
            // For division edge cases, see page 44 and 45:
            // https://riscv.org/wp-content/uploads/2019/12/riscv-spec-20191213.pdf
            if (sop2 == 0)
                return -1;
            if (sop1 == (1 << (sizeof(Word) * 8 - 1)) && sop2 == -1)
                return sop1; // Smallest signed integer value
            return sop1 / sop2;
        case AluFunct::DIVU:
            if (op2 == 0)
                // For some reason this returns the dividend in some implementations, even
                // though it should return all ones according to the specification.
                return -1;
            return op1 / op2;
        case AluFunct::REM:
            if (sop2 == 0)
                return sop1;
            if (sop1 == (1 << (sizeof(Word) * 8 - 1)) && sop2 == -1)
                return 0;
            return sop1 % sop2;
        case AluFunct::REMU:
            if (op2 == 0)
                return op1;
            return op1 % op2;

        default:
            throw std::runtime_error("Unknown ALU function.");
    }
}

static Word load(const Memory& mem, LoadFunct funct, Word base, SignedWord offset)
{
    switch (funct) {
        case LoadFunct::LB:
            return (Word)(mem.get<SignedByte>(base + offset));
        case LoadFunct::LH:
            return (Word)(mem.get<SignedHalfWord>(base + offset));
        case LoadFunct::LW:
            return mem.get<Word>(base + offset);
        case LoadFunct::LBU:
            return (Word)mem.get<Byte>(base + offset);
        case LoadFunct::LHU:
            return (Word)mem.get<HalfWord>(base + offset);

        default:
            throw std::runtime_error("Unknown load function.");
    }
}

static void store(Memory& mem, StoreFunct funct, Word base, SignedWord offset, Word value)
{
    switch (funct) {
        case StoreFunct::SB:
            mem.set((Byte)value, base + offset);
            break;
        case StoreFunct::SH:
            mem.set((HalfWord)value, base + offset);
            break;
        case StoreFunct::SW:
            mem.set((Word)value, base + offset);
            break;

        default:
            throw std::runtime_error("Unknown store function.");
    }
}

static bool branch(BranchFunct funct, Word op1, Word op2)
{
    SignedWord sop1, sop2;
    memcpy(&sop1, &op1, sizeof(Word));
    memcpy(&sop2, &op2, sizeof(Word));
    switch (funct) {
        case BranchFunct::BEQ:
            return op1 == op2;
        case BranchFunct::BNE:
            return op1 != op2;
        case BranchFunct::BLT:
            return sop1 < sop2;
        case BranchFunct::BGE:
            return sop1 >= sop2;
        case BranchFunct::BLTU:
            return op1 < op2;
        case BranchFunct::BGEU:
            return op1 >= op2;
        default:
            throw std::runtime_error("Unknown branch function.");
    }
}

void Processor::raise_exception(Memory& mem, ExceptionType except)
{
    switch (except) {
        case ExceptionType::SYSCALL:
            syscall(mem);
            break;
        case ExceptionType::FATAL:
            std::cerr << "Fatal exception occurred at " << pc << std::endl;
            abort = true;
            break;

        default:
            throw std::runtime_error("Unknown exception type.");
    }
}

void Processor::syscall(Memory& mem)
{
    Ecall call = (Ecall)rfile[17];
    Word arg = rfile[10];
    std::cout << (uint32_t)call << std::endl;

    switch (call) {
        case Ecall::EXIT:
            abort = true;
            break;

        case Ecall::PRINT_STR: {
            const char* str = mem.get<const char*>(arg);
            if (strlen(str) + arg >= mem.size())
                throw std::runtime_error("No null terminator in string.");
            printf("%s", str);
            break;
        }

        default:
            throw std::runtime_error("Unsupported system call.");
    }
}

static SignedWord b_offset(DecodedInstruction instr)
{
    Word offset_1_10 = extract_bits(instr.u.imm, 1, 11);
    Word offset_11 = extract_bits(instr.u.imm, 0, 1);
    Word offset_12 = extract_bits(instr.u.imm, 11, 12);
    Word offset = (offset_1_10 << 1) | (offset_11 << 11) | (offset_12 << 12);
    return sign_extend(offset, 13);
}

static SignedWord j_offset(DecodedInstruction instr)
{
    Word offset_12_19 = extract_bits(instr.u.imm, 0, 8);
    Word offset_11 = extract_bits(instr.u.imm, 8, 9);
    Word offset_1_10 = extract_bits(instr.u.imm, 9, 19);
    Word offset_20 = extract_bits(instr.u.imm, 19, 20);
    Word offset = (offset_1_10 << 1) | (offset_11 << 11) | (offset_12_19 << 12) | (offset_20 << 20);
    return sign_extend(offset, 21);
}

void Processor::execute(DecodedInstruction instr, Memory& mem)
{
    bool increment_pc = true;

    switch (instr.opcode) {
        case Opcode::LUI:
            rfile.write(instr.u.rd, instr.u.imm << 12);
            break;

        case Opcode::AUIPC:
            rfile.write(instr.u.rd, pc + (instr.u.imm << 12));
            break;

        case Opcode::ALUI: {
            AluFunct funct;
            Word op2 = sign_extend(instr.i.imm, 12);
            if (instr.i.funct3 == (uint8_t)AluFunct::SRL)
                funct = (AluFunct)((extract_bits(instr.i.imm, 6, 12) << 4) | instr.i.funct3);
            else
                funct = (AluFunct)(instr.i.funct3);

            rfile.write(instr.i.rd, alu(funct, rfile[instr.i.rs1], op2));
            break;
        }
        case Opcode::ALU: {
            AluFunct funct = (AluFunct)(((uint16_t)instr.r.funct7 << 3) | instr.r.funct3);
            rfile.write(instr.r.rd, alu(funct, rfile[instr.r.rs1], rfile[instr.r.rs2]));
            break;
        }
        case Opcode::ENV:
            raise_exception(mem, ExceptionType::SYSCALL);
            break;

        case Opcode::LOAD: {
            LoadFunct funct = (LoadFunct)instr.i.funct3;
            rfile.write(instr.i.rd, load(mem, funct, rfile[instr.i.rs1], sign_extend(instr.i.imm, 12)));
            break;
        }
        case Opcode::STORE: {
            StoreFunct funct = (StoreFunct)instr.s.funct3;
            store(mem, funct, rfile[instr.s.rs1], sign_extend(instr.s.imm, 12), rfile[instr.s.rs2]);
            break;
        }
        case Opcode::JAL:
            rfile.write(instr.u.rd, pc + sizeof(Word));
            pc += j_offset(instr);
            increment_pc = false;
            break;

        case Opcode::JALR: {
            Word temp = pc + sizeof(Word);
            pc = (rfile[instr.i.rs1] + sign_extend(instr.i.imm, 12)) & ~((Word)1);
            rfile.write(instr.i.rd, temp);
            increment_pc = false;
            break;
        }
        case Opcode::BRANCH:
            if (branch((BranchFunct)instr.s.funct3, rfile[instr.s.rs1], rfile[instr.s.rs2])) {
                pc += b_offset(instr);
                increment_pc = false;
            }
            break;

        default:
            std::cerr << "Unknown opcode." << std::endl;
            raise_exception(mem, ExceptionType::FATAL);
            break;
    }

    if (increment_pc)
        pc += sizeof(Word);
}

std::ostream& operator<<(std::ostream& stream, const Processor& proc)
{
    stream << "PC: " << proc.get_pc() << std::endl;
    for (size_t i = 0; i < REGISTER_FILE_SIZE; i++) {
        SignedWord sreg;
        memcpy(&sreg, &proc.registers()[i], sizeof(Word));
        stream << "x" << i << ": " << proc.registers()[i] << " (signed: " << sreg << ")" << std::endl;
    }
    return stream;
}