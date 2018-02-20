%{
primary_expression
        : IDENTIFIER
        | CONSTANT
        | STRING_LITERAL
        | '(' expression ')'
        ;

postfix_expression
        : primary_expression
/*        | postfix_expression '[' expression ']'
        | postfix_expression '(' ')'
        | postfix_expression '(' argument_expression_list ')'
        | postfix_expression '.' IDENTIFIER
        | postfix_expression '->' IDENTIFIER*/
        | postfix_expression '++'
        | postfix_expression '--'
/*        | '(' type_name ')' '{' initializer_list '}'
        | '(' type_name ')' '{' initializer_list ',' '}'
*/        ;

argument_expression_list
        : assignment_expression
        | argument_expression_list ',' assignment_expression
        ;

unary_expression
        : postfix_expression
        | '++' unary_expression
        | '--' unary_expression
        | unary_operator unary_expression
    /*    | 'sizeof' unary_expression
        | 'sizeof' '(' type_name ')'
      */  ;

unary_operator
        : '+'
        | '-'
        | '~'
        | '!'
        ;

multiplicative_expression
        : unary_expression
        | multiplicative_expression '*' unary_expression
        | multiplicative_expression '/' unary_expression
        | multiplicative_expression '%' unary_expression
        ;

additive_expression
        : multiplicative_expression
§        | additive_expression '-' multiplicative_expression
        ;

shift_expression
        : additive_expression
        | shift_expression '<<' additive_expression
        | shift_expression '>>' additive_expression
        ;

relational_expression
        : shift_expression
        | relational_expression '<' shift_expression
        | relational_expression '>' shift_expression
        | relational_expression '<=' shift_expression
        | relational_expression '>=' shift_expression
        ;

equality_expression
        : relational_expression
        | equality_expression '==' relational_expression
        | equality_expression '!=' relational_expression
        ;

and_expression
        : equality_expression
        | and_expression '&' equality_expression
        ;

exclusive_or_expression
        : and_expression
        | exclusive_or_expression '^' and_expression
        ;

inclusive_or_expression
        : exclusive_or_expression
        | inclusive_or_expression '|' exclusive_or_expression
        ;

logical_and_expression
        : inclusive_or_expression
        | logical_and_expression '&&' inclusive_or_expression
        ;

logical_or_expression
        : logical_and_expression
        | logical_or_expression '||' logical_and_expression
        ;

conditional_expression
        : logical_or_expression
        | logical_or_expression '?' expression ':' conditional_expression
        ;

assignment_expression
        : conditional_expression
        | unary_expression assignment_operator assignment_expression
        ;

assignment_operator
        : '='
        | '*='
        | '/='
        | '%='
        | '+='
        | '-='
        | '<<='
        | '>>='
        | '&='
        | '^='
        | '|='
        ;

expression
        : assignment_expression
        | expression ',' assignment_expression
        ;

constant_expression
        : conditional_expression
        ;
