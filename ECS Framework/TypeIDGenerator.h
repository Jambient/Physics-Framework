#ifndef TYPEINDEXGENERATOR_H
#define TYPEINDEXGENERATOR_H

#include <cstddef>

class TypeIndexGenerator {
public:
    template <typename T>
    static size_t GetTypeIndex() {
        static const size_t typeIndex = m_nextTypeIndex++;
        return typeIndex;
    }

private:
    inline static size_t m_nextTypeIndex = 0; // Shared counter for generating unique IDs
};

#endif // TYPEINDEXGENERATOR_H