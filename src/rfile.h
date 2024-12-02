#ifndef RFILE_H_INCLUDED
#define RFILE_H_INCLUDED

#include "memory.h"

constexpr size_t REGISTER_FILE_SIZE = 32;

class RegisterFile
{
  public:
    RegisterFile()
    {
        rfile[0] = 0;
    }

    inline const Word& operator[](size_t i) const
    {
        assert(i < REGISTER_FILE_SIZE);
        assert(rfile[0] == 0);
        return rfile[i];
    }

    inline void write(size_t i, Word value)
    {
        assert(i < REGISTER_FILE_SIZE);
        if (i != 0)
            rfile[i] = value;
    }

    void to_file_le32(const char* path) const;

  private:
    Word rfile[REGISTER_FILE_SIZE];
};

#endif