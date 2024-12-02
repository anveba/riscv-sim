#include "memory.h"

#include <cassert>
#include <fstream>

Memory::Memory(size_t size)
    : contents(std::vector<uint8_t>(size))
{
}

Memory::Memory(std::vector<uint8_t> contents)
    : contents(contents)
{
}

Memory::~Memory()
{
}

void Memory::load(const char* path, size_t base)
{
    assert(base < contents.size());
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (ifs.fail())
        throw std::runtime_error("Could not open " + std::string(path));

    std::ifstream::pos_type sz = ifs.tellg();

    if (sz == 0)
        return;

    if (base + sz >= contents.size())
        throw std::runtime_error("File contents cannot be contained in memory.");

    ifs.seekg(0, std::ios::beg);
    ifs.read((char*)&contents[0], sz);
}
