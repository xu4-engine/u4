#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <stdint.h>
#include <string.h>
#include <vector>

typedef uint16_t symbol_t;

class SymbolTable {
public:
    SymbolTable() {}
    const char* name(symbol_t symbol) const;
    symbol_t intern(const char* name, size_t len);
    void internSymbols(symbol_t* table, size_t count, const char* names);

    symbol_t intern(const char* name) {
        return intern(name, strlen(name));
    }

private:
    struct Entry {
        uint32_t hash;
        uint32_t stringOffset;
    };

    std::vector<Entry> index;
    std::vector<char> stringStore;
};

#endif // SYMBOLTABLE_H
