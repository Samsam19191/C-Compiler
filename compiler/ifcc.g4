grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' (assignment)* return_stmt '}' ;

assignment : 'int' ID '=' operand ';' ;

operand : CONST | ID ;

return_stmt : RETURN operand ';' ;

// Lexique
RETURN : 'return' ;
CONST  : [0-9]+ ;
ID     : [a-zA-Z][a-zA-Z0-9]* ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n]+ -> skip ;
