grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' (assignment)* return_stmt '}' ;

assignment : 'int' ID ('=' expr)? ';' ;

expr : expr ('*' | '/') expr   # MulDiv
     | expr ('+' | '-') expr   # AddSub
     | funcCall                # CallFunction
     | '(' expr ')'            # Parens
     | operand                 # OperandExpr
     ;

funcCall : ID '(' (expr (',' expr)*)? ')' ;

operand : CONST | CHAR | ID ;

return_stmt : RETURN expr ';' ;

RETURN : 'return' ;
CONST  : [0-9]+ ;
CHAR : '\'' ( ~('\'' | '\\') | '\\' . ) '\'' ;
ID     : [a-zA-Z][a-zA-Z0-9]* ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n]+ -> skip ;
