grammar ifcc;

axiom : prog EOF ;

// La fonction principale, ici on autorise plusieurs affectations et un return.
prog : 'int' 'main' '(' ')' '{' (assignment)* return_stmt '}' ;

// Affectation d'une variable (déclarée en tant qu'entier uniquement dans cet exemple).
assignment : 'int' ID ('=' expr)? ';' ;

// La règle expr a été étendue pour supporter les appels de fonction.
expr : expr ('*' | '/') expr   # MulDiv
     | expr ('+' | '-') expr   # AddSub
     | callExpr                # CallExpression
     | '(' expr ')'            # Parens
     | operand                 # OperandExpr
     ;

// Règle pour reconnaître un appel de fonction : 
// Un identifiant suivi d'une parenthèse ouvrante, une liste d'expressions séparées par des virgules (optionnelle) et une parenthèse fermante.
callExpr : ID '(' (expr (',' expr)*)? ')' ;

// Un opérande peut être une constante, un littéral de caractère ou un identifiant.
operand : CONST | CHAR_LITERAL | ID ;

// Retour de fonction
return_stmt : RETURN expr ';' ;

// Lexique
RETURN : 'return' ;
CONST  : [0-9]+ ;
// Définition d'un littéral de caractère (gère également les échappements simples).
CHAR_LITERAL : '\'' ( ~('\'' | '\\') | '\\' . ) '\'' ;
ID     : [a-zA-Z][a-zA-Z0-9]* ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n]+ -> skip ;
