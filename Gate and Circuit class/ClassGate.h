#ifndef CLASSGATES_H
#define CLASSGATES_H

#include <iostream>  // cout
#include <vector>    // vector
#include <algorithm> // min_element, find
#include <assert.h>  // assert
#include <numeric>   // accumulate
#include <limits.h>  // INT_MAX
#include <sstream>
using namespace std;

// Marcos for gate types
#define GATE_NAND 0
#define GATE_NOR 1
#define GATE_AND 2
#define GATE_OR 3
#define GATE_XOR 4
#define GATE_XNOR 5
#define GATE_BUFF 6
#define GATE_NOT 7
#define GATE_PI 9
#define GATE_FANOUT 10

// Macros for logic values
#define LOGIC_UNSET -1
#define LOGIC_ZERO   0
#define LOGIC_ONE    1
#define LOGIC_D      2 
#define LOGIC_DBAR   3
#define LOGIC_X      4 

// Macros for fault types
#define NOFAULT   -1
#define FAULT_SA0 0
#define FAULT_SA1 1

class Gate{

 private:
  int gateID;	             // A unique ID for this gate.
  char gateType;             // Gate type (macros above: GATE_NAND, etc.)
  vector<Gate*> gateInputs;  // Stores the pointers to all the gates that this gate's inputs connect to.
  vector<Gate*> gateOutputs; // Stores the pointers to all the gates that this gate's output connects to.
  string outputName;         // The name of the output of this gate

  char gateValue;            // The logic value of this gate's output (using macros above: LOGIC_ZERO, etc.

  string printLogicVal(int val);
  vector<string> inputName;  // A list of the names of the inputs to this gate.

  char faultType;            // Type of fault on this gate's output (NOFAULT, FAULT_SA0, FAULT_SA1)

 public:
  Gate(string name, int ID, int gt);
  
  char get_gateType();

  vector<Gate*> get_gateOutputs();
  void set_gateOutput(Gate* x);
  void replace_gateOutput(Gate* oldGate, Gate* newGate);
 
  vector<Gate*> get_gateInputs();
  void set_gateInput(Gate* x);
  void replace_gateInput(Gate* oldGate, Gate* newGate);
  
  string get_outputName();

  void printGateInfo();
  string gateTypeName();

  void setValue(int val);
  char getValue();
  string printValue();

  void set_gateInputName(string n);
  vector<string> get_gateInputNames();

  int getGateInputNumber(Gate *g);

  void set_faultType(char f);
  char get_faultType();

};

#endif
