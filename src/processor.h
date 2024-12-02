#ifndef PROCESSOR_H_INCLUDED
#define PROCESSOR_H_INCLUDED

#include "instruction.h"
#include "memory.h"
#include "rfile.h"
#include <ostream>

enum class ExceptionType
{
    SYSCALL,
    FATAL,
};

class Processor
{
  public:
    Processor();
    ~Processor();

    void run(Memory& mem);

    inline void set_pc(Word pc) { this->pc = pc; }
    inline Word get_pc() const { return pc; }
    inline const RegisterFile& registers() const { return rfile; }

  private:
    Word fetch(const Memory& rom);
    DecodedInstruction decode(Word instruction) const;
    void execute(DecodedInstruction instruction, Memory& mem);

    void raise_exception(Memory& mem, ExceptionType except);
    void syscall(Memory& mem);

    RegisterFile rfile;
    Word pc;
    bool abort;
};

std::ostream& operator<<(std::ostream& stream, const Processor& proc);

#endif