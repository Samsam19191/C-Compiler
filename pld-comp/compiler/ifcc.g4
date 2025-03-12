grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' (statement)* return_stmt '}' ;

statement : assignment | declaration ; // Ajouter ici les autres types de statements (if, while, etc.)

declaration : type ID (('=' expr)? (',' ID ('=' expr)?)*)? ';' ;

assignment : ID '=' expr (',' ID '=' expr)* ';' ;

type : 'int' | 'char' ;

expr : expr op=('*' | '/') expr   # MulDiv
     | expr op=('+' | '-') expr   # AddSub
     | funcCall                # CallFunction
     | '(' expr ')'            # Parens
     | operand                 # OperandExpr
     ;

funcCall : ID '(' (expr (',' expr)*)? ')' ;

operand : CONSTINT | CONSTCHAR | ID ;

// Retour de fonction
return_stmt : RETURN expr ';' ;

RETURN : 'return' ;
CONSTINT  : [0-9]+ ;
CONSTCHAR : '\'' ( ~['\\] | '\\' . ) '\'' ;
ID     : [a-zA-Z][a-zA-Z0-9]* ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n]+ -> skip ;
