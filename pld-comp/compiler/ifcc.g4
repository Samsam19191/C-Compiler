grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' (statement)* return_stmt '}' ;

statement : assignment | declaration ; // Ajouter ici les autres types de statements (if, while, etc.)

declaration : ('int' | 'char') ID (',' ID)* ';' ;

assignment : ('int' | 'char')? ID '=' expr (',' ID '=' expr)* ';' ;

expr : expr ('*' | '/') expr   # MulDiv
     | expr ('+' | '-') expr   # AddSub
     | '(' expr ')'            # Parens
     | operand                 # OperandExpr
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