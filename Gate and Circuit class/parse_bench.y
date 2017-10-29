%{
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include "parse_bench.tab.h"
#include "ClassCircuit.h"
#include "ClassGate.h"

using namespace std;

Circuit* myCircuit = new Circuit;
 
int gate_index=0, gate_ID_val=0;
string temp, temp1;
int i=0;
 
 // stuff from flex that bison needs to know about:
 extern "C" int yylex();
 extern "C" int yyparse();
 
 void yyerror(const char *str) { fprintf(stderr,"error: %s\n", str); } 
 int yywrap() { return 1; }
 
 void process(string);

%}

%token INPUT OUTPUT LPAREN RPAREN EQUALS COMMA
%token NOR AND OR XOR XNOR BUFF NOT DFF NAND

%union 
{
    int gatetype;
    char *s;
}

%token <s> IDENTIFIER;
%type <s> id_list;
%type <gatetype> GATE;
%%

lines: /* empty */
      | lines line
    ;

line:
    input_line | output_line | assign_line;

input_line:
    INPUT LPAREN IDENTIFIER RPAREN {

    myCircuit->newGate($3, gate_ID_val, GATE_PI);
	gate_ID_val=gate_ID_val+1;
	gate_index=gate_index+1;
   };

output_line:
    OUTPUT LPAREN IDENTIFIER RPAREN {
	myCircuit->addOutputName($3);
   };

assign_line:
    IDENTIFIER EQUALS GATE LPAREN id_list RPAREN {
	int gateID = gate_ID_val;
	int gateType = $3;
	string assign_out=$1;

    myCircuit->newGate($1, gateID, gateType);
	process(temp);
	gate_index=gate_index+1;
	gate_ID_val=gate_ID_val+1;
	temp="";
    };

id_list: IDENTIFIER {
  temp=temp.append($1);
  temp=temp.append(",");
}
| IDENTIFIER COMMA id_list 
{
  temp1=temp;
  temp="";
  temp=temp.append($1);		
  temp=temp.append(",");
  temp.append(temp1);
};

GATE:
       NAND {$$=GATE_NAND; }
     | NOR {$$=GATE_NOR; }
     | AND {$$=GATE_AND; }
     | OR {$$=GATE_OR; }
     | XOR {$$=GATE_XOR; }
     | XNOR {$$=GATE_XNOR; }
//     | DFF {$$=GATE_DFF; }
     | BUFF {$$=GATE_BUFF; }
     | NOT {$$=GATE_NOT; }
;
%%

void process(string id_list) {  
  string input;
  input = id_list;
  
  string temp_string="";
  
  for(int i=0; i<input.length(); i++) { // Stores input names of each gate in a vector     
    if(input[i]!=',') {
      stringstream ss;
      ss << input[i];
      string temp ;
      ss >> temp;
      temp_string=temp_string.append(temp);
    }
    else if(input[i]==',') {           	    
      myCircuit->getGate(gate_index)->set_gateInputName(temp_string);
      temp_string.clear();
    }
  }	
}
