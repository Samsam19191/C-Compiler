#include "CodeGenVisitor.h"

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx)
{
  std::cout << ".globl main\n";
  std::cout << " main: \n";
  std::cout << "    pushq %rbp \n";
  std::cout << "    movq %rsp, %rbp \n";

  // Vsiit statements
  for (auto statement : ctx->statement())
  {
    std::cerr << "[DEBUG] Generating code for statement: "
              << statement->getText() << "\n";
    visit(statement);
  }

  std::cerr << "[DEBUG] Generating code for return statement: "
            << ctx->return_stmt()->getText() << "\n";
  this->visit(ctx->return_stmt());

  std::cout << "    popq %rbp\n";
  std::cout << "    ret\n";

  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitStatement(ifccParser::StatementContext *ctx)
{
  if (ctx->declaration())
  {
    return visit(ctx->declaration());
  }
  else if (ctx->assignment())
  {
    return visit(ctx->assignment());
  }

  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
  // visit(ctx->operand());
  visit(ctx->expr());
  // int retval = stoi(ctx->CONST()->getText());

  // std::cout << "    movl $" << retval << ", %eax\n";

  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitAssignment(ifccParser::AssignmentContext *ctx)
{
  const std::vector<antlr4::tree::TerminalNode *> &varNames = ctx->ID();
  const std::vector<ifccParser::ExprContext *> &exprs = ctx->expr();

  for (int i = 0; i < varNames.size(); i++)
  {
    std::string varName = varNames[i]->getText();
    ifccParser::ExprContext *expr = exprs[i];

    if (symbolTable.find(varName) == symbolTable.end())
    {
      std::cerr << "Error: Undefined variable '" << varName
                << "' during code generation.\n";
      exit(1);
    }

    visit(expr);
    initializedVariables.insert(varName);
    std::cout << "    movl %eax, " << symbolTable[varName] << "(%rbp)\n";
  }

  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitDeclaration(ifccParser::DeclarationContext *ctx)
{
  std::vector<ifccParser::ExprContext *> exprs = ctx->expr();
  if (exprs.size() > 0)
  {
    std::vector<antlr4::tree::TerminalNode *> varNames = ctx->ID();
    for (int i = 0; i < varNames.size(); i++)
    {
      std::string varName = varNames[i]->getText();
      ifccParser::ExprContext *expr = exprs[i];

      if (symbolTable.find(varName) == symbolTable.end())
      {
        std::cerr << "Error: Undefined variable '" << varName
                  << "' during code generation.\n";
        exit(1);
      }

      visit(expr);
      std::cout << "    movl %eax, " << symbolTable[varName] << "(%rbp)\n";
    }
  }
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitOperand(ifccParser::OperandContext *ctx)
{
  if (ctx->CONSTINT())
  {
    int value = std::stoi(ctx->CONSTINT()->getText());
    std::cerr << "[DEBUG] Loading constant " << value << "\n";
    std::cout << "    movl $" << value << ", %eax\n";
  }
  else if (ctx->ID())
  {
    std::string varName = ctx->ID()->getText();
    std::cerr << "[DEBUG] Loading variable '" << varName << "' from offset "
              << symbolTable[varName] << "\n";

    if (symbolTable.find(varName) == symbolTable.end() ||
        initializedVariables.find(varName) == initializedVariables.end())
    {
      std::cerr << "Error: Undefined or Uninitialized variable '" << varName
                << "' during code generation.\n";
      exit(1);
    }
    std::cout << "    movl " << symbolTable[varName] << "(%rbp), %eax\n";
  }
  else if (ctx->CONSTCHAR())
  {
    char value = ctx->CONSTCHAR()->getText()[1];
    std::cout << "    movl $" << (int)value << ", %eax\n";
  }
  else if (ctx->CONSTCHAR())
  {
    char value = ctx->CONSTCHAR()->getText()[1];
    std::cout << "    movl $" << (int)value << ", %eax\n";
  }
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitBitOps(ifccParser::BitOpsContext *ctx)
{
  // Evaluate the left operand and store the result in %eax
  visit(ctx->expr(0));
  // Save %eax in %edx
  std::cout << "    movl %eax, %edx\n";
  // Evaluate the right operand; result is in %eax
  visit(ctx->expr(1));
  std::string op = ctx->op->getText();

  if (op == "&")
  {
    std::cout << "    andl %edx, %eax\n";
  }
  else if (op == "|")
  {
    std::cout << "    orl %edx, %eax\n";
  }
  else if (op == "^")
  {
    std::cout << "    xorl %edx, %eax\n";
  }

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitMulDiv(ifccParser::MulDivContext *ctx)
{
  std::string op = ctx->op->getText();

  if (op == "*")
  {
    // Multiplication : a * b
    visit(ctx->expr(0));                   // Évalue a (résultat dans %eax)
    std::cout << "    movl %eax, %edx\n";  // Sauvegarde a dans %edx
    visit(ctx->expr(1));                   // Évalue b (résultat dans %eax)
    std::cout << "    imull %edx, %eax\n"; // %eax = a * b
  }
  else if (op == "/")
  {
    // Division : a / b
    visit(ctx->expr(1));                  // Évalue b (diviseur)
    std::cout << "    movl %eax, %ecx\n"; // Sauvegarde b dans %ecx
    visit(ctx->expr(0));                  // Évalue a (dividende)
    std::cout << "    cdq\n";             // Extension de signe dans %edx
    std::cout << "    idivl %ecx\n";      // Quotient dans %eax, reste dans %edx
  }
  else if (op == "%")
  {
    // Modulo : a % b
    visit(ctx->expr(1));                  // Évalue b (diviseur)
    std::cout << "    movl %eax, %ecx\n"; // Sauvegarde b dans %ecx
    visit(ctx->expr(0));                  // Évalue a (dividende)
    std::cout << "    cdq\n";             // Extension de signe dans %edx
    std::cout << "    idivl %ecx\n";      // Effectue la division
    std::cout << "    movl %edx, %eax\n"; // Récupère le reste dans %eax
  }
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitAddSub(ifccParser::AddSubContext *ctx)
{
  std::string op = ctx->op->getText(); // Récupère l'opérateur du nœud courant

  if (op == "+")
  {
    // Addition : a + b
    visit(ctx->expr(0));                  // Évalue a, résultat dans %eax
    std::cout << "    movl %eax, %edx\n"; // Sauvegarde a dans %edx
    visit(ctx->expr(1));                  // Évalue b, résultat dans %eax
    std::cout << "    addl %edx, %eax\n"; // %eax = a + b
  }
  else // op est "-"
  {
    // Soustraction : a - b
    visit(ctx->expr(0));                  // Évalue a, résultat dans %eax
    std::cout << "    movl %eax, %edx\n"; // Sauvegarde a dans %edx
    visit(ctx->expr(1));                  // Évalue b, résultat dans %eax
    std::cout << "    movl %eax, %ecx\n"; // Sauvegarde b dans %ecx
    std::cout << "    movl %edx, %eax\n"; // Restaure a dans %eax
    std::cout << "    subl %ecx, %eax\n"; // %eax = a - b
  }
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitParens(ifccParser::ParensContext *ctx)
{
  std::cerr << "[DEBUG] Generating code for parenthesized expression: "
            << ctx->getText() << "\n";
  return visit(ctx->expr());
}

antlrcpp::Any
CodeGenVisitor::visitOperandExpr(ifccParser::OperandExprContext *ctx)
{
  std::cerr << "[DEBUG] Generating code for operand expression: "
            << ctx->getText() << "\n";
  return visit(ctx->operand());
}
