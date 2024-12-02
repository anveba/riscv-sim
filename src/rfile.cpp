#include "rfile.h"
#include <fstream>

void RegisterFile::to_file_le32(const char* path) const
{
    std::ofstream fs(path, std::ios::out | std::ios::binary);
    if (fs.fail())
        throw std::runtime_error("Could not open " + std::string(path));
    Word le[REGISTER_FILE_SIZE];
    for (size_t i = 0; i < REGISTER_FILE_SIZE; i++)
        le[i] = htole32(rfile[i]);
    fs.write((const char*)le, sizeof(le));
    fs.close();
}