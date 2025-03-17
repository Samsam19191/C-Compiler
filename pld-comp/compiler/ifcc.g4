grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' (statement)* return_stmt '}' ;

statement : assignment | declaration ; // Ajouter ici les autres types de statements (if, while, etc.)

declaration : type ID (('=' expr)? (',' ID ('=' expr)?)*)? ';' ;

assignment : ID '=' expr (',' ID '=' expr)* ';' ;

type : 'int' | 'char' ;

expr : expr op=('&' | '|' | '^') expr  # BitOps
     | expr op=('*' | '/' | '%') expr   # MulDiv
     | expr op=('+' | '-') expr         # AddSub
     | '(' expr ')'                     # Parens
     | operand                          # OperandExpr
     ;


operand : CONSTINT | CONSTCHAR | ID ;

return_stmt : RETURN expr ';' ;

// Lexique
RETURN : 'return' ;
CONSTINT  : [0-9]+ ;
CONSTCHAR : '\'' ( ~['\\] | '\\' . ) '\'' ;
ID     : [a-zA-Z][a-zA-Z0-9]* ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n]+ -> skip ;