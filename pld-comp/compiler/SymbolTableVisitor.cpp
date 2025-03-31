#include "SymbolTableVisitor.h"
#include <cstdlib>

// Visite d'une définition de fonction
antlrcpp::Any SymbolTableVisitor::visitFunctionDefinition(ifccParser::FunctionDefinitionContext *ctx) {
    std::string funcName = ctx->Identifier()->getText();
//    std::cout << "[DEBUG] Traitement de la fonction : " << funcName << "\n";

    // Réinitialisation de la table des symboles pour la fonction en cours
    currentSymbolTable.clear();
    currentInitializedVariables.clear();
    currentOffset = 0;

    // Traitement des paramètres (s'ils existent)
    if (ctx->parameterList()) {
        for (auto paramCtx : ctx->parameterList()->parameter()) {
            std::string paramName = paramCtx->Identifier()->getText();
            if (currentSymbolTable.find(paramName) != currentSymbolTable.end()) {
                std::cerr << "Error: Paramètre '" << paramName
                          << "' déclaré plusieurs fois dans la fonction " << funcName << ".\n";
                exit(1);
            }

            // Chaque paramètre occupe 4 octets
            currentOffset -= 4;
            currentSymbolTable[paramName] = currentOffset;
//            std::cout << "[DEBUG] Paramètre trouvé : " << paramName<< " avec offset " << currentOffset << "\n";
            // Les paramètres sont considérés comme initialisés dès leur entrée
            currentInitializedVariables.insert(paramName);
        }
    }

    // Visite du corps de la fonction (bloc)
    visit(ctx->block());

    // Optionnel : vérification des variables déclarées mais jamais utilisées
    checkUnusedVariables();

    // Sauvegarder la table des symboles de la fonction dans la table globale
    FunctionSymbolTable fst;
    fst.symbols = currentSymbolTable;
    fst.initializedVariables = currentInitializedVariables;
    functionSymbolTables[funcName] = fst;

    return 0;
}

// Visite d'un bloc d'instructions
antlrcpp::Any SymbolTableVisitor::visitBlock(ifccParser::BlockContext *ctx) {
    for (auto stmt : ctx->statement()) {
        visit(stmt);
    }
    return 0;
}

// Visite d'une déclaration de variable
antlrcpp::Any SymbolTableVisitor::visitDeclaration(ifccParser::DeclarationContext *ctx) {
    std::string varType = ctx->type()->getText();
    // Remplacer ctx->ID() par ctx->Identifier() (la grammaire utilise Identifier)
    std::vector<antlr4::tree::TerminalNode*> varNames = ctx->Identifier();
    std::vector<ifccParser::ExprContext*> exprs = ctx->expr();
    for (size_t i = 0; i < varNames.size(); i++) {
        std::string varName = varNames[i]->getText();
        if (currentSymbolTable.find(varName) != currentSymbolTable.end()) {
            std::cerr << "Error: Variable '" << varName
                      << "' est déclarée plusieurs fois.\n";
            exit(1);
        }
        // Pour 'char' on décrémente de 1, sinon (int) de 4
        if (varType == "char")
            currentOffset -= 1;
        else
            currentOffset -= 4;
        currentSymbolTable[varName] = currentOffset;
//        std::cout << "[DEBUG] Variable trouvée : " << varName<< " avec offset " << currentOffset << "\n";
        // Si une initialisation est présente, on visite l'expression et on marque la variable comme initialisée
        if (i < exprs.size()) {
            visit(exprs[i]);
            currentInitializedVariables.insert(varName);
        }
    }
    return 0;
}

// Visite d'une affectation (exemple : a = expr)
antlrcpp::Any SymbolTableVisitor::visitAssignment(ifccParser::AssignmentContext *ctx) {
    std::string varName = ctx->Identifier()->getText();
    if (currentSymbolTable.find(varName) == currentSymbolTable.end()) {
        std::cerr << "Error: Variable '" << varName
                  << "' assignée avant déclaration.\n";
        exit(1);
    }
    visit(ctx->expr());
    // Marquer la variable comme initialisée
    currentInitializedVariables.insert(varName);
    return 0;
}

// Visite d'une instruction return
antlrcpp::Any SymbolTableVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
    visit(ctx->expr());
    return 0;
}

// Visite d'une instruction if
antlrcpp::Any SymbolTableVisitor::visitIf_stmt(ifccParser::If_stmtContext *ctx) {
    visit(ctx->expr());
    visit(ctx->statement(0));
    if (ctx->statement().size() > 1) {
        visit(ctx->statement(1));
    }
    return 0;
}

// Visite d'une instruction while
antlrcpp::Any SymbolTableVisitor::visitWhile_stmt(ifccParser::While_stmtContext *ctx) {
    visit(ctx->expr());
    visit(ctx->statement());
    return 0;
}

// Visite d'une expression contenant un opérande
antlrcpp::Any SymbolTableVisitor::visitOperandExpr(ifccParser::OperandExprContext *ctx) {
    visit(ctx->operand());
    return 0;
}

// Visite d'un opérande
antlrcpp::Any SymbolTableVisitor::visitOperand(ifccParser::OperandContext *ctx) {
    if (ctx->Identifier()) {
        std::string varName = ctx->Identifier()->getText();
        if (currentSymbolTable.find(varName) == currentSymbolTable.end()) {
            std::cerr << "Error: Variable '" << varName
                      << "' utilisée avant déclaration.\n";
            exit(1);
        }
        // Ici, vous pouvez ajouter une logique pour marquer la variable comme utilisée
        // (par exemple, en ajoutant varName dans un ensemble currentUsedVariables)
    }
    return 0;
}

// Visite d'un appel de fonction
antlrcpp::Any SymbolTableVisitor::visitFuncCall(ifccParser::FuncCallContext *ctx) {
    // On accepte tous les appels de fonction (qu'ils soient intégrés ou définis par l'utilisateur)
    std::string funcName = ctx->Identifier()->getText();
    // Visite de chacun des arguments
    for (auto arg : ctx->expr()) {
        visit(arg);
    }
    return 0;
}

// Vérification finale des variables déclarées mais jamais utilisées
// (Dans cette version, la gestion des variables non utilisées n'est pas détaillée, mais vous pouvez ajouter un ensemble pour suivre les variables utilisées.)
void SymbolTableVisitor::checkUnusedVariables() {
    // Exemple : vous pouvez afficher un warning pour chaque variable déclarée qui n'a pas été utilisée.
    // Pour cela, il faudrait disposer d'un ensemble (currentUsedVariables) en plus de currentSymbolTable.
    // Ici, nous laissons cette fonction vide ou avec une simple notification.
}
