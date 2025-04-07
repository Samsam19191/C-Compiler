#ifndef DEFFONCTION_H
#define DEFFONCTION_H

#include <string>

class DefFonction
{
public:
    DefFonction(const std::string &name) : funcName(name) {}
    std::string getName() const { return funcName; }

private:
    std::string funcName;
};

#endif // DEFFONCTION_H
