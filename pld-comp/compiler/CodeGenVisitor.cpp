#include "CodeGenVisitor.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

// Génère le code assembleur pour la fonction main.
antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext* ctx) {
  std::cout << ".globl main\n";
  std::cout << " main:\n";
  std::cout << "    pushq %rbp\n";
  std::cout << "    movq %rsp, %rbp\n";

  // Visite toutes les affectations du corps de main.
  for (auto assignment : ctx->assignment()) {
    visit(assignment);
  }

  // Traite l'instruction return.
  this->visit(ctx->return_stmt());

  std::cout << "    popq %rbp\n";
  std::cout << "    ret\n";

  return 0;
}

// Génère le code pour l'instruction return.
// L'expression est évaluée et son résultat doit se retrouver dans %eax.
antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext* ctx) {
  visit(ctx->expr());
  return 0;
}

// Génère le code pour une affectation.
// On récupère le nom de la variable, on évalue l'expression (résultat dans %eax)
// puis on stocke %eax dans la zone mémoire associée (offset récupéré dans symbolTable).
antlrcpp::Any CodeGenVisitor::visitAssignment(ifccParser::AssignmentContext* ctx) {
  std::string varName = ctx->ID()->getText();

  if (symbolTable.find(varName) == symbolTable.end()) {
    std::cerr << "Error: Undefined variable '" << varName
              << "' during code generation.\n";
    exit(1);
  }

  visit(ctx->expr());
  
  // symbolTable[varName] est de type int (offset)
  std::cout << "    movl %eax, " << symbolTable[varName] << "(%rbp)\n";

  return 0;
}

// Génère le code pour un opérande, qui peut être une constante ou un identifiant.
antlrcpp::Any CodeGenVisitor::visitOperand(ifccParser::OperandContext* ctx) {
    if (ctx->CONST()) {
        int value = std::stoi(ctx->CONST()->getText());
        std::cout << "    movl $" << value << ", %eax\n";
    }
    else if (ctx->CHAR_LITERAL()) {
        // Par exemple, si ctx->CHAR_LITERAL()->getText() renvoie "'A'"
        std::string literal = ctx->CHAR_LITERAL()->getText();
        // Supposons que le littéral est au format simple 'X'
        // Pour une solution plus complète, il faudrait gérer les échappements.
        char c = literal[1]; // On suppose que le littéral est bien formé (le deuxième caractère est le contenu)
        int value = static_cast<int>(c);
        std::cout << "    movl $" << value << ", %eax\n";
    }
    else if (ctx->ID()) {
        std::string varName = ctx->ID()->getText();
        if (symbolTable.find(varName) == symbolTable.end()) {
            std::cerr << "Error: Undefined variable '" << varName
                      << "' during code generation.\n";
            exit(1);
        }
        std::cout << "    movl " << symbolTable[varName] << "(%rbp), %eax\n";
    }
    return 0;
}

// Génère le code pour les opérations de multiplication ou de division.
antlrcpp::Any CodeGenVisitor::visitMulDiv(ifccParser::MulDivContext* ctx) {
  // Évalue l'opérande droit et sauvegarde le résultat.
  visit(ctx->expr(1));
  std::cout << "    pushq %rax\n";

  // Évalue l'opérande gauche.
  visit(ctx->expr(0));
  std::cout << "    popq %rcx\n";

  // Effectue la multiplication ou la division selon l'opérateur.
  if (ctx->getText().find("*") != std::string::npos) {
    std::cout << "    imul %rcx, %rax\n";
  } else {
    std::cout << "    cqo\n"; // Extension de %rax pour la division signée.
    std::cout << "    idiv %rcx\n";
  }

  return 0;
}

// Génère le code pour les opérations d'addition ou de soustraction.
antlrcpp::Any CodeGenVisitor::visitAddSub(ifccParser::AddSubContext* ctx) {
  visit(ctx->expr(0));
  std::cout << "    pushq %rax\n";
  visit(ctx->expr(1));
  std::cout << "    popq %rcx\n";

  if (ctx->getText().find("+") != std::string::npos) {
    std::cout << "    add %rcx, %rax\n";
  } else {
    std::cout << "    sub %rcx, %rax\n";
  }

  return 0;
}

// Traite les expressions entre parenthèses.
antlrcpp::Any CodeGenVisitor::visitParens(ifccParser::ParensContext* ctx) {
  return visit(ctx->expr());
}

// Délègue le traitement à l'opérande.
antlrcpp::Any CodeGenVisitor::visitOperandExpr(ifccParser::OperandExprContext* ctx) {
  return visit(ctx->operand());
}

// Nouvelle méthode pour gérer les appels de fonction.
// Elle évalue chaque argument, place le résultat dans le registre approprié (selon l'ABI System V AMD64)
// et émet l'instruction 'call' avec le nom de la fonction appelée.
antlrcpp::Any CodeGenVisitor::visitCallExpr(ifccParser::CallExprContext* ctx) {
  // Récupère le nom de la fonction appelée (par exemple "putchar" ou "getchar").
  std::string funcName = ctx->ID()->getText();

  // Registres pour les 6 premiers arguments.
  std::vector<std::string> argRegs = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};

  // Nombre d'arguments dans l'appel.
  int argCount = ctx->expr().size();
  if (argCount > 6) {
    std::cerr << "Error: More than 6 arguments are not supported.\n";
    exit(1);
  }

  // Pour chaque argument, évalue l'expression et place le résultat (supposé être dans %eax) dans le registre adéquat.
  for (int i = 0; i < argCount; i++) {
    visit(ctx->expr(i));
    std::cout << "    movl %eax, " << argRegs[i] << "\n";
  }

  // Émet l'instruction call pour appeler la fonction.
  std::cout << "    call " << funcName << "\n";
  return 0;
}
