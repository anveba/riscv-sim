#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include <cassert>
#include <cstdint>
#include <vector>

typedef uint64_t LongWord;
typedef int64_t SignedLongWord;
typedef uint32_t Word;
typedef int32_t SignedWord;
typedef uint16_t HalfWord;
typedef int16_t SignedHalfWord;
typedef uint8_t Byte;
typedef int8_t SignedByte;

class Memory
{
  public:
    Memory(size_t size);
    ~Memory();

    void load(const char* path, size_t base);

    inline size_t size() const { return contents.size(); }

    template<typename T>
    inline T get(size_t i) const
    {
        assert(i + sizeof(T) < size());
        return *(T*)(start_ptr() + i);
    }

    template<typename T>
    inline void set(T value, size_t i)
    {
        assert(i + sizeof(T) < size());
        *(T*)(start_ptr() + i) = value;
    }

  private:
    std::vector<uint8_t> contents;

    Memory(std::vector<uint8_t> contents);
    inline const uint8_t* start_ptr() const { return &contents[0]; }
};

#endif