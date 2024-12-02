#include "processor.h"
#include <iostream>

constexpr size_t MEM_SIZE = 1 << 24;

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cout << "Expected two arguments.\nUsage: riscv-sim INFILE OUTFILE" << std::endl;
        return 1;
    }

    Memory mem(MEM_SIZE);
    mem.load(argv[1], 0);
    Processor proc;
    proc.set_pc(0);
    proc.run(mem);

    std::cout << proc << std::endl;
    proc.registers().to_file_le32(argv[2]);
}