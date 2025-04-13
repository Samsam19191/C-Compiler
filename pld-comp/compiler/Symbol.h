#pragma once
#include "Type.h"
#include <string>

using namespace std;

struct Symbol
{
    // Memory offset (in bytes)
    int offset;
    // Wheter or not this variable has been used
    bool used;
    // Wheter the variable was initialized
    bool initialized;
    // In which line the symbol was declared
    int line;
    // The type of the symbol
    Type type;
    // The lexeme corresponding to this symbol
    string lexeme;

    Symbol(Type type, const string &lexeme)
        : type(type), lexeme(lexeme), used(false), initialized(false), offset(0), line(1) {}

    Symbol(Type type, const string &lexeme, int line)
        : type(type), lexeme(lexeme), used(false), initialized(false), offset(0), line(line) {}
};