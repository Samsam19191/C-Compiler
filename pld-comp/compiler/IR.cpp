#include "IR.h"
#include "Type.h"
#include <sstream>
#include <cassert>
#include <cctype>
#include <map>
#include <iostream>
#include <unordered_map>

using namespace std;

////////////////////
// IRInstr methods

IRInstr::IRInstr(BasicBlock *bb_, Operation op, Type t, vector<string> params)
    : bb(bb_), op(op), t(t), params(params)
{
}

void IRInstr::gen_asm(ostream &o)
{
    switch (op)
    {
    case ldconst:
        // ldconst : paramètres [dest, constant]
        o << "    movl $" << params[1] << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    case copy:
        // copy : paramètres [dest, src]
        // Pour éviter un transfert mémoire->mémoire, on charge d'abord dans %eax.
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    case add:
        // add : paramètres [dest, op1, op2]
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    addl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    case sub:
        // sub : paramètres [dest, op1, op2]
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    subl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    case mul:
        // mul : paramètres [dest, op1, op2]
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    imull " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    case rmem:
        // rmem : lecture mémoire, paramètres [dest, address]
        o << "    movl (" << bb->cfg->IR_reg_to_asm(params[1]) << "), %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    case wmem:
        // wmem : écriture mémoire, paramètres [address, src]
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    movl %eax, (" << bb->cfg->IR_reg_to_asm(params[0]) << ")\n";
        break;
    case call:
    {
        // call : paramètres [label, dest, arg1, arg2, ...]
        int numArgs = params.size() - 2;
        for (int i = numArgs; i >= 1; i--)
        {
            o << "    pushl " << bb->cfg->IR_reg_to_asm(params[i + 1]) << "\n";
        }
        o << "    call " << params[0] << "\n";
        if (numArgs > 0)
            o << "    addl $" << (numArgs * 4) << ", %esp\n";
        if (!params[1].empty())
            o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
        break;
    }
    case cmp_eq:
        // cmp_eq : paramètres [dest, op1, op2]
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    cmpl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    sete %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    case cmp_lt:
        // cmp_lt : paramètres [dest, op1, op2]
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    cmpl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    setl %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    case cmp_le:
        // cmp_le : paramètres [dest, op1, op2]
        o << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "    cmpl " << bb->cfg->IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "    setle %al\n";
        o << "    movzbl %al, %eax\n";
        o << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
        break;
    default:
        o << "    # Opération IR non gérée\n";
        break;
    }
}

////////////////////
// BasicBlock methods

BasicBlock::BasicBlock(CFG *cfg, string entry_label)
    : cfg(cfg), label(entry_label), exit_true(nullptr), exit_false(nullptr), test_var_name("")
{
}

void BasicBlock::add_IRInstr(IRInstr::Operation op, Type t, vector<string> params)
{
    IRInstr *instr = new IRInstr(this, op, t, params);
    instrs.push_back(instr);
}

void BasicBlock::gen_asm(ostream &o)
{
    o << label << ":\n";
    for (IRInstr *instr : instrs)
    {
        instr->gen_asm(o);
    }
    if (exit_true == nullptr)
    {
        cfg->gen_asm_epilogue(o);
    }
    else if (exit_false == nullptr)
    {
        o << "    jmp " << exit_true->label << "\n";
    }
    else
    {
        o << "    cmpl $0, " << cfg->IR_reg_to_asm(test_var_name) << "\n";
        o << "    je " << exit_false->label << "\n";
        o << "    jmp " << exit_true->label << "\n";
    }
}

////////////////////
// CFG methods

CFG::CFG(DefFonction *ast)
    : ast(ast), nextFreeSymbolIndex(0), nextBBnumber(0), current_bb(nullptr)
{
}

void CFG::add_bb(BasicBlock *bb)
{
    bbs.push_back(bb);
}

string CFG::new_BB_name()
{
    ostringstream oss;
    oss << "BB" << nextBBnumber++;
    return oss.str();
}

string CFG::IR_reg_to_asm(string reg)
{
    // Si 'reg' est une constante numérique, renvoie "$<constante>"
    if (!reg.empty() && (isdigit(reg[0]) || (reg[0] == '-' && reg.size() > 1 && isdigit(reg[1]))))
    {
        return "$" + reg;
    }
    auto it = SymbolIndex.find(reg);
    if (it != SymbolIndex.end())
    {
        ostringstream oss;
        oss << it->second << "(%rbp)";
        return oss.str();
    }
    else
    {
        cerr << "Erreur : variable '" << reg << "' introuvable dans la table des symboles.\n";
        return reg;
    }
}

void CFG::gen_asm(ostream &o)
{
    gen_asm_prologue(o);
    for (BasicBlock *bb : bbs)
    {
        bb->gen_asm(o);
    }
}

void CFG::gen_asm_prologue(ostream &o)
{
    o << "    .globl " << ast->getName() << "\n";
    o << ast->getName() << ":\n";
    o << "    pushq %rbp\n";
    o << "    movq %rsp, %rbp\n";
}

void CFG::gen_asm_epilogue(ostream &o)
{
    o << "    popq %rbp\n";
    o << "    ret\n";
}

void CFG::setSymbolTable(const unordered_map<string, int> &st)
{
    SymbolIndex.clear();
    for (const auto &p : st)
    {
        SymbolIndex[p.first] = p.second;
    }
}

void CFG::add_to_symbol_table(std::string name, Type t)
{
    // Enregistre le type dans SymbolType
    SymbolType[name] = t;
    // Si la variable n'est pas déjà dans SymbolIndex, on lui attribue un offset
    if (SymbolIndex.find(name) == SymbolIndex.end())
    {
        nextFreeSymbolIndex++;
        SymbolIndex[name] = -(nextFreeSymbolIndex * 4);
    }
}

string CFG::create_new_tempvar(Type t)
{
    ostringstream oss;
    oss << "t" << nextFreeSymbolIndex++;
    string temp = oss.str();
    // Calcul de l'offset : ici, on suppose 4 octets par variable
    SymbolIndex[temp] = -(nextFreeSymbolIndex * 4);
    return temp;
}

int CFG::get_var_index(string name)
{
    auto it = SymbolIndex.find(name);
    if (it != SymbolIndex.end())
        return it->second;
    else
    {
        cerr << "Erreur : variable '" << name << "' introuvable dans la table des symboles.\n";
        return 0;
    }
}

Type CFG::get_var_type(string name)
{
    return SymbolType.find(name) != SymbolType.end() ? SymbolType[name] : Type::INT;
}
