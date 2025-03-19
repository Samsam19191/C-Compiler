#include "CodeGenVisitorV2.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

//
// Note : Dans cette version, nous ne produisons plus directement du code assembleur via std::cout.
// Au lieu de cela, nous construisons une IR en ajoutant des instructions dans le CFG (cf. IR.h/IR.cpp).
// La génération finale de l'assembleur se fera par un appel ultérieur à cfg->gen_asm(std::cout);
//

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext* ctx) {
  // Ici, on suppose que cfg (le CFG) a déjà été créé et configuré dans main.cpp.
  // On laisse la génération du prologue et de l'épilogue au CFG.
  for (auto assignment : ctx->assignment()) {
    visit(assignment);
  }
  this->visit(ctx->return_stmt());
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext* ctx) {
  // On évalue l'expression de retour.
  visit(ctx->expr());
  // On pourrait ajouter ici une instruction IR pour marquer le return.
  // Par exemple : cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {"!retval", "%eax"});
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitAssignment(ifccParser::AssignmentContext* ctx) {
  std::string varName = ctx->ID()->getText();
  if (symbolTable.find(varName) == symbolTable.end()) {
    std::cerr << "Error: Undefined variable '" << varName
              << "' during code generation.\n";
    exit(1);
  }
  // On évalue l'expression de droite
  visit(ctx->expr());
  
  // Au lieu d'émettre directement "movl %eax, offset(%rbp)",
  // on ajoute une instruction IR de type "copy" qui copie le contenu de %eax dans la variable.
  cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {varName, "%eax"});
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitOperand(ifccParser::OperandContext* ctx) {
  if (ctx->CONST()) {
    int value = std::stoi(ctx->CONST()->getText());
    // Ajoute une instruction IR pour charger une constante dans %eax.
    cfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT, {"%eax", std::to_string(value)});
  }
  else if (ctx->CHAR()) {
    std::string literal = ctx->CHAR()->getText();
    char c = literal[1];
    int value = static_cast<int>(c);
    cfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT, {"%eax", std::to_string(value)});
  }
  else if (ctx->ID()) {
    std::string varName = ctx->ID()->getText();
    if (symbolTable.find(varName) == symbolTable.end()) {
      std::cerr << "Error: Undefined variable '" << varName
                << "' during code generation.\n";
      exit(1);
    }
    // Ajoute une instruction IR pour charger la valeur de la variable dans %eax.
    cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {"%eax", varName});
  }
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitMulDiv(ifccParser::MulDivContext* ctx) {
  // Évaluation de l'opérande de droite.
  visit(ctx->expr(1));
  // Sauvegarde du résultat dans une variable temporaire.
  std::string temp = cfg->create_new_tempvar(Type::INT);
  cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {temp, "%eax"});
  
  // Évaluation de l'opérande de gauche.
  visit(ctx->expr(0));
  
  if (ctx->getText().find("*") != std::string::npos) {
    // Ajoute une instruction IR pour effectuer une multiplication.
    cfg->current_bb->add_IRInstr(IRInstr::mul, Type::INT, {"%eax", "%eax", temp});
  } else {
    // Pour la division, on suppose ici que l'on ajoute un op "div".
    // Si ce n'est pas défini dans IRInstr, vous pouvez le simuler par un appel à une fonction de runtime.
    cfg->current_bb->add_IRInstr(IRInstr::call, Type::INT, {"div", "%eax", temp});
  }
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitAddSub(ifccParser::AddSubContext* ctx) {
  // Évaluation de l'opérande gauche.
  visit(ctx->expr(0));
  // Sauvegarde du résultat dans une variable temporaire.
  std::string temp = cfg->create_new_tempvar(Type::INT);
  cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {temp, "%eax"});
  
  // Évaluation de l'opérande droite.
  visit(ctx->expr(1));
  
  if (ctx->getText().find("+") != std::string::npos) {
    // Ajoute une instruction IR pour additionner temp et %eax, stockant le résultat dans %eax.
    cfg->current_bb->add_IRInstr(IRInstr::add, Type::INT, {"%eax", temp, "%eax"});
  } else {
    // Pour la soustraction, soustrait temp de %eax.
    cfg->current_bb->add_IRInstr(IRInstr::sub, Type::INT, {"%eax", temp, "%eax"});
  }
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitParens(ifccParser::ParensContext* ctx) {
  return visit(ctx->expr());
}

antlrcpp::Any CodeGenVisitor::visitOperandExpr(ifccParser::OperandExprContext* ctx) {
  return visit(ctx->operand());
}

antlrcpp::Any CodeGenVisitor::visitFuncCall(ifccParser::FuncCallContext *ctx) {
  std::string functionName = ctx->ID()->getText();
  int argIndex = 0;
  std::vector<std::string> argRegs; // Stocke les registres utilisés pour les arguments
  std::vector<std::string> registers = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
  
  for (auto expr : ctx->expr()) {
    visit(expr);
    if (argIndex < registers.size()) {
      std::string reg = registers[argIndex];
      // Copie le résultat (%eax) dans le registre d'argument.
      cfg->current_bb->add_IRInstr(IRInstr::copy, Type::INT, {reg, "%eax"});
      argRegs.push_back(reg);
    } else {
      std::cerr << "Error: Too many arguments for function '" << functionName << "'.\n";
      exit(1);
    }
    argIndex++;
  }
  
  // Construit les paramètres pour l'instruction d'appel :
  // le nom de la fonction, la destination (%eax pour la valeur de retour) et les registres d'argument.
  std::vector<std::string> callParams;
  callParams.push_back(functionName);
  callParams.push_back("%eax"); // Destination pour le retour
  for (auto reg : argRegs) {
    callParams.push_back(reg);
  }
  cfg->current_bb->add_IRInstr(IRInstr::call, Type::INT, callParams);
  return 0;
}
