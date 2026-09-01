# Կեռասի շարահյուսությունը

```
Program = [ NewLines ] { Subroutine NewLines }.

Subroutine = 'SUB' IDENT [ '(' ParameterList ')' ] [ 'AS' TypeName ] Sequence 'END' 'SUB'.

ParameterList = Parameter { ',' Parameter }.
Parameter     = IDENT [ '[' [ Expression ] ']' ] 'AS' TypeName.

TypeName = 'REAL' | 'TEXT' | 'BOOL'.

Sequence  = NewLines { Statement NewLines }.

Statement = Let | Dim | If | While | For | Call.

Let = 'LET' IDENT [ '[' Expression ']' ] '=' Expression.

Dim = 'DIM' IDENT [ '[' Expression ']' ] 'AS' TypeName.

If = 'IF' Expression 'THEN' Sequence { 'ELSEIF' Expression 'THEN' Sequence } [ 'ELSE' Sequence ] 'END' 'IF'.

While = 'WHILE' Expression Sequence 'END' 'WHILE'.

For = 'FOR' IDENT '=' Expression 'TO' Expression [ 'STEP' ['+' | '-'] NUMBER ] Sequence 'END' 'FOR'.

Call = 'CALL' IDENT [ ExpressionList ].

ExpressionList = Expression { ',' Expression }.

Expression = Disjunction.

Disjunction = Conjunction { 'OR' Conjunction }.

Conjunction = Comparison { 'AND' Comparison }.

Comparison = Addition [ ('=' | '<>' | '>' | '>=' | '<' | '<=') Addition ].

Addition = Multiplication { ('+' | '-' | '&') Multiplication }.

Multiplication = Power { ('*' | '/' | 'MOD' | '\') Power }.

Power = Unary [ '^' Power ].

Unary = { ('+' | '-' | 'NOT') } Subscript.

Subscript = Factor { '[' Expression ']' }.

Factor = 'TRUE' | 'FALSE' | NUMBER | STRING
       | IDENT [ '(' [ ExpressionList ] ')' ]
       | Grouped.

Grouped = '(' Expression ')'.
```
