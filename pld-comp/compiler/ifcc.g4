grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' (declaration | assignment)* return_stmt '}' ;

declaration : ('int' | 'char') ID (',' ID)* ';' ;

assignment : ('int' | 'char') ID '=' expr (',' ID '=' expr)* ';' ;

expr : expr ('*' | '/') expr   # MulDiv
     | expr ('+' | '-') expr   # AddSub
     | '(' expr ')'            # Parens
     | operand                 # OperandExpr
     ;

operand : CONST | CHAR | ID ;

return_stmt : RETURN expr ';' ;

// Lexique
RETURN : 'return' ;
CONST  : [0-9]+ ;
CHAR   : '\'' . '\'' ;
ID     : [a-zA-Z][a-zA-Z0-9]* ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n]+ -> skip ;
