grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' (assignment)* return_stmt '}' ;

assignment : 'int' ID '=' expr ';' ;

expr : expr ('*' | '/') expr   # MulDiv
     | expr ('+' | '-') expr   # AddSub
     | '(' expr ')'            # Parens
     | operand                 # OperandExpr
     ;

operand : CONST | ID ;

return_stmt : RETURN expr ';' ;

// Lexique
RETURN : 'return' ;
CONST  : [0-9]+ ;
ID     : [a-zA-Z][a-zA-Z0-9]* ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n]+ -> skip ;