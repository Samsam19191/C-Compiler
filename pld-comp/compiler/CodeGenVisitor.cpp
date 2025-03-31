#include "CodeGenVisitor.h"
#include <cstdlib>
#include <string>
#include <vector>

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext* ctx) {
    // Parcourt toutes les définitions de fonctions du programme.
    for (auto funcDef : ctx->functionDefinition()) {
        // Récupère le nom de la fonction.
        std::string funcName = funcDef->Identifier()->getText();
        currentFunctionName = funcName;

        // Vérifie que la table des symboles globale contient bien une entrée pour cette fonction.
        if (globalFuncTables.find(funcName) == globalFuncTables.end()) {
            std::cerr << "Error: Global symbol table for function " << funcName << " not found.\n";
            exit(1);
        }

        // Génère le code assembleur pour cette fonction.
        // La méthode visitFunctionDefinition (définie dans CodeGenVisitor) utilisera
        // la table des symboles locale pour générer le prologue, la copie des paramètres,
        // le corps et l'épilogue de la fonction.
        FunctionSymbolTable fst = globalFuncTables[funcName];
        setSymbolTable(fst.symbols);
        setInitializedVariables(fst.initializedVariables);
        visit(funcDef);

        // Optionnel : affiche une séparation entre les fonctions pour clarifier la sortie.
        std::cout << "\n";
    }
    return 0;
}


//
// Fonction : visitFunctionDefinition
// -------------------------
// Génére le code assembleur pour une définition de fonction :
// - Affichage du label de la fonction
// - Prologue (push rbp, mov rsp, rbp)
// - Copie des paramètres depuis les registres (les six premiers) vers la zone de la pile, en utilisant les offsets du symbol table
// - Réservation d'espace local (calculé à partir du minimum d'offset)
// - Appel du corps de la fonction (le bloc)
// - Épilogue (restauration de la pile et retour)
//
antlrcpp::Any CodeGenVisitor::visitFunctionDefinition(ifccParser::FunctionDefinitionContext *ctx) {
    std::string funcName = ctx->Identifier()->getText();
    std::cout << ".globl " << funcName << "\n";
    std::cout << funcName << ":\n";

    // Prologue standard
    std::cout << "    pushq %rbp\n";
    std::cout << "    movq %rsp, %rbp\n";

    // Copie des paramètres depuis les registres vers la pile.
    // Les paramètres sont enregistrés dans la table (injection par setSymbolTable() depuis SymbolTableVisitor).
    std::vector<std::string> paramRegisters = {"%edi", "%esi", "%rdx", "%rcx", "%r8", "%r9"};
    if (ctx->parameterList()) {
        int index = 0;
        for (auto paramCtx : ctx->parameterList()->parameter()) {
            std::string paramName = paramCtx->Identifier()->getText();
            // Vérifier que le paramètre existe dans la table locale.
            if (symbolTable.find(paramName) == symbolTable.end()) {
                std::cerr << "Error: Paramètre '" << paramName << "' non trouvé dans la table des symboles pour la fonction " << funcName << ".\n";
                exit(1);
            }
            if (index < paramRegisters.size()) {
                std::cout << "    movl " << paramRegisters[index] << ", " << symbolTable[paramName] << "(%rbp)\n";
            } else {
                std::cerr << "Error: Trop de paramètres dans la fonction '" << funcName << "'.\n";
                exit(1);
            }
            index++;
        }
    }

    // Calcul de l'espace local à réserver :
    // Parcours de la table pour trouver la plus grande taille (les offsets sont négatifs).
    int localSize = 0;
    for (auto &entry : symbolTable) {
        if (entry.second < localSize)
            localSize = entry.second;
    }
    localSize = -localSize; // conversion en taille positive
    int adjustedSize = ((localSize + 15) / 16) * 16; // alignement sur 16 octets
    if (adjustedSize > 0)
        std::cout << "    subq $" << adjustedSize << ", %rsp\n";

    // Génération du code pour le corps de la fonction.
    visit(ctx->block());

    // Épilogue
    std::cout << funcName << "_exit:\n";
    std::cout << "    movq %rbp, %rsp\n";
    std::cout << "    popq %rbp\n";
    std::cout << "    ret\n";

    return 0;
}

//
// Fonction : visitBlock
// -------------------------
// Parcourt chaque instruction du bloc
//
antlrcpp::Any CodeGenVisitor::visitBlock(ifccParser::BlockContext *ctx) {
    for (auto stmt : ctx->statement()) {
        visit(stmt);
    }
    return 0;
}

//
// Fonction : visitDeclaration
// -------------------------
// Pour chaque déclaration avec initialisation, évalue l'expression et copie le résultat (%eax)
// dans l'emplacement associé dans la pile (via symbolTable).
//
antlrcpp::Any CodeGenVisitor::visitDeclaration(ifccParser::DeclarationContext *ctx) {
    std::vector<antlr4::tree::TerminalNode*> varNames = ctx->Identifier();
    std::vector<ifccParser::ExprContext*> exprs = ctx->expr();
    for (size_t i = 0; i < varNames.size(); i++) {
        std::string varName = varNames[i]->getText();
        if (i < exprs.size()) {
            visit(exprs[i]); // Le résultat se trouve dans %eax
            std::cout << "    movl %eax, " << symbolTable[varName] << "(%rbp)\n";
        }
    }
    return 0;
}

//
// Fonction : visitAssignment
// -------------------------
// Évalue l'expression et stocke le résultat dans la variable correspondante.
//
antlrcpp::Any CodeGenVisitor::visitAssignment(ifccParser::AssignmentContext *ctx) {
    std::string varName = ctx->Identifier()->getText();
    if (symbolTable.find(varName) == symbolTable.end()) {
        std::cerr << "Error: Variable '" << varName << "' non définie lors de l'affectation.\n";
        exit(1);
    }
    visit(ctx->expr());
    std::cout << "    movl %eax, " << symbolTable[varName] << "(%rbp)\n";
    return 0;
}

//
// Fonction : visitReturn_stmt
// -------------------------
// Évalue l'expression de retour et saute vers l'étiquette d'épilogue.
// (On suppose ici que le résultat de l'expression se trouve dans %eax).
//
antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
    visit(ctx->expr());
    std::cout << "    jmp " << currentFunctionName << "_exit" << "\n";
    return 0;
}

//
// Fonction : visitIf_stmt
// -------------------------
// Génère du code pour une structure conditionnelle if/else en créant des labels uniques.
//
antlrcpp::Any CodeGenVisitor::visitIf_stmt(ifccParser::If_stmtContext *ctx) {
    static int ifCounter = 0;
    int labelNum = ifCounter++;
    std::string elseLabel = "else_" + std::to_string(labelNum);
    std::string endLabel = "endif_" + std::to_string(labelNum);

    visit(ctx->expr());
    std::cout << "    cmpl $0, %eax\n";
    std::cout << "    je " << elseLabel << "\n";

    // Branche then
    visit(ctx->statement(0));
    std::cout << "    jmp " << endLabel << "\n";

    // Branche else (si présente)
    std::cout << elseLabel << ":\n";
    if (ctx->statement().size() > 1)
        visit(ctx->statement(1));

    std::cout << endLabel << ":\n";
    return 0;
}

//
// Fonction : visitWhile_stmt
// -------------------------
// Génère un label de début et de fin pour la boucle while et insère un saut inconditionnel à la fin de chaque itération.
//
antlrcpp::Any CodeGenVisitor::visitWhile_stmt(ifccParser::While_stmtContext *ctx) {
    static int whileCounter = 0;
    int labelNum = whileCounter++;
    std::string startLabel = "while_start_" + std::to_string(labelNum);
    std::string endLabel = "while_end_" + std::to_string(labelNum);

    std::cout << startLabel << ":\n";
    visit(ctx->expr());
    std::cout << "    cmpl $0, %eax\n";
    std::cout << "    je " << endLabel << "\n";

    visit(ctx->statement());
    std::cout << "    jmp " << startLabel << "\n";
    std::cout << endLabel << ":\n";
    return 0;
}

//
// Fonction : visitAddSub
// -------------------------
// Évalue les expressions gauche et droite et effectue l'addition ou la soustraction.
//
antlrcpp::Any CodeGenVisitor::visitAddSub(ifccParser::AddSubContext *ctx) {
    std::string op = ctx->op->getText();
    visit(ctx->expr(0)); // résultat dans %eax
    std::cout << "    pushq %rax\n";
    visit(ctx->expr(1)); // résultat dans %eax
    std::cout << "    movl %eax, %ebx\n";
    std::cout << "    popq %rax\n";
    if (op == "+")
        std::cout << "    addl %ebx, %eax\n";
    else
        std::cout << "    subl %ebx, %eax\n";
    return 0;
}

//
// Fonction : visitMulDiv
// -------------------------
// Évalue les deux opérandes et effectue la multiplication ou la division.
//
antlrcpp::Any CodeGenVisitor::visitMulDiv(ifccParser::MulDivContext *ctx) {
    std::string op = ctx->op->getText();
    visit(ctx->expr(0)); // résultat dans %eax
    std::cout << "    pushq %rax\n";
    visit(ctx->expr(1)); // résultat dans %eax
    std::cout << "    movl %eax, %ebx\n";
    std::cout << "    popq %rax\n";
    if (op == "*")
        std::cout << "    imull %ebx, %eax\n";
    else {
        std::cout << "    cdq\n";
        std::cout << "    idivl %ebx\n";
    }
    return 0;
}

//
// Fonction : visitParens
// -------------------------
// Pour une expression parenthésée, on se contente de visiter l'expression intérieure.
//
antlrcpp::Any CodeGenVisitor::visitParens(ifccParser::ParensContext *ctx) {
    return visit(ctx->expr());
}

//
// Fonction : visitOperandExpr
// -------------------------
// Simple redirection vers l'opérande.
//
antlrcpp::Any CodeGenVisitor::visitOperandExpr(ifccParser::OperandExprContext *ctx) {
    return visit(ctx->operand());
}

//
// Fonction : visitOperand
// -------------------------
// Si l'opérande est une constante ou un identifiant, on génère le code adéquat.
// Pour un identifiant, on vérifie qu'il a été initialisé et on charge sa valeur depuis la pile.
//
antlrcpp::Any CodeGenVisitor::visitOperand(ifccParser::OperandContext *ctx) {
    if (ctx->CONSTINT()) {
        int value = std::stoi(ctx->CONSTINT()->getText());
        std::cout << "    movl $" << value << ", %eax\n";
    } else if (ctx->CONSTCHAR()) {
        std::string literal = ctx->CONSTCHAR()->getText();
        char c = literal[1];
        std::cout << "    movl $" << static_cast<int>(c) << ", %eax\n";
    } else if (ctx->Identifier()) {
        std::string varName = ctx->Identifier()->getText();
        if (symbolTable.find(varName) == symbolTable.end() ||
            initializedVariables.find(varName) == initializedVariables.end()) {
            std::cerr << "Error: Variable '" << varName << "' utilisée avant initialisation.\n";
            exit(1);
        }
        std::cout << "    movl " << symbolTable[varName] << "(%rbp), %eax\n";
    }
    return 0;
}


//
// Fonction : visitFuncCall
// -------------------------
// Évalue les arguments (en respectant l'ordre) et les déplace dans les registres (%rdi, %rsi, …),
// puis effectue l'appel de la fonction.
//
antlrcpp::Any CodeGenVisitor::visitFuncCall(ifccParser::FuncCallContext *ctx) {
    std::string funcName = ctx->Identifier()->getText();
    std::vector<std::string> registers = {"%edi", "%esi", "%rdx", "%rcx", "%r8", "%r9"};
    int argIndex = 0;
    for (auto arg : ctx->expr()) {
        visit(arg); // résultat dans %eax
        if (argIndex < registers.size())
            std::cout << "    movl %eax, " << registers[argIndex] << "\n";
        else {
            std::cerr << "Error: Trop d'arguments dans l'appel de fonction '" << funcName << "'.\n";
            exit(1);
        }
        argIndex++;
    }
    std::cout << "    call " << funcName << "\n";
    return 0;
}
