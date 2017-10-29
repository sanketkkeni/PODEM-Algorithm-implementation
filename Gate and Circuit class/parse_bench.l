%{
#include <cstdio>
#include <iostream>
#include <string>
using namespace std;
#define YY_DECL extern "C" int yylex()

#include "parse_bench.tab.h"  // to get the token types that we return

%}
%%
"#"[^\n]*\n     ;
NAND            return NAND;
nand            return NAND;
NOR             return NOR;
nor             return NOR;
AND             return AND;
and             return AND;
OR              return OR;
or              return OR;
XOR             return XOR;
xor             return XOR;
XNOR            return XNOR;
xnor            return XNOR;
DFF             return DFF;
dff             return DFF;
BUFF            return BUFF;
buff            return BUFF;
NOT             return NOT;
not             return NOT;
INPUT           return INPUT;
input           return INPUT;
OUTPUT          return OUTPUT;
output          return OUTPUT;
\(              return LPAREN;
\)              return RPAREN;
[\n]              ;
[ \t\n]         ;
=               { return EQUALS;}
,               return COMMA;
[a-zA-Z0-9._]+   { yylval.s=strdup(yytext); return IDENTIFIER;}
%%
