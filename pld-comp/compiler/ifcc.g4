grammar ifcc;

// =======================================================
// Règles principales (Point d'entrée)
// =======================================================
prog
    : (functionDefinition)+ EOF
    ;

// =======================================================
// Définition de fonctions et paramètres
// =======================================================
functionDefinition
    : type Identifier '(' parameterList? ')' block
    ;

parameterList
    : parameter (',' parameter)*
    ;

parameter
    : type Identifier
    ;

// =======================================================
// Bloc d'instructions et instructions simples
// =======================================================
block
    : '{' statement* '}'
    ;

statement
    : declaration ';'
    | assignment ';'
    | return_stmt ';'
    | if_stmt
    | while_stmt
    | funcCall ';'
    | block
    ;

// =======================================================
// Déclarations et affectations
// =======================================================
declaration
    : type Identifier (',' Identifier)* ('=' expr (',' expr)*)?
    ;

assignment
    : Identifier '=' expr
    ;

// =======================================================
// Retour de fonction
// =======================================================
return_stmt
    : 'return' expr
    ;

// =======================================================
// Instructions de contrôle
// =======================================================
if_stmt
    : 'if' '(' expr ')' statement ('else' statement)?
    ;

while_stmt
    : 'while' '(' expr ')' statement
    ;

// =======================================================
// Expressions et appels de fonction
// =======================================================
expr
    : expr op=('*'|'/') expr     # MulDiv
    | expr op=('+'|'-') expr     # AddSub
    | '(' expr ')'               # Parens
    | funcCall                   # FuncCallExpr
    | operand                    # OperandExpr
    ;

funcCall
    : Identifier '(' (expr (',' expr)*)? ')'
    ;

operand
    : Identifier
    | CONSTINT
    | CONSTCHAR
    ;

// =======================================================
// Types et règles lexicales
// =======================================================
type
    : 'int'
    | 'char'
    | 'void'
    ;

CONSTINT: [0-9]+;
CONSTCHAR: '\'' . '\'';

Identifier: [a-zA-Z_][a-zA-Z0-9_]*;

WS: [ \t\r\n]+ -> skip;
LINE_COMMENT: '//' ~[\r\n]* -> skip;
BLOCK_COMMENT: '/*' .*? '*/' -> skip;
