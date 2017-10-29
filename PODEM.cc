// ESE-549 Project 3 Reference Solution
// Peter Milder

/** @file */


// For you to do:
// Part 1: 
//    - Write the getObjective() function
//    - Write the updateDFrontier() function
//    - Write the backtrace() function
//    - Write the podemRecursion() function
// Then your basic PODEM implementation should be finished.
// Test this code carefully.
// Note you can also turn on a "checkTest" mode that will use your 
// simulator to run the test after you generated it and check that it
// correctly detects the faults.
//
// Part 2:
//    - Write the eventDrivenSim() function.
//    - Change the simulation calls in your podemRecursion() function
//      from using the old simFullCircuit() function to the new
//      event-driven simulator.
// Then, your PODEM implementation should run considerably faster
// (probably 8 to 10x faster for large circuits).
// Test everything and then evaluate the speedup.
// A quick way to figure out how long it takes to run something in
// Linux is:
//   time ./atpg ... type other params...


#include <iostream>
#include <fstream> 
#include <vector>
#include <queue>
#include <time.h>
#include <stdio.h>
#include "parse_bench.tab.h"
#include "ClassCircuit.h"
#include "ClassGate.h"
#include <limits>
#include <stdlib.h>
#include <time.h>

using namespace std;

/**  @brief Just for the parser. Don't touch. */
extern "C" int yyparse();

/**  Input file for parser. Don't touch. */
extern FILE *yyin;

/** Our circuit. We declare this external so the parser can use it more easily. */
extern Circuit* myCircuit;

//--------------------------
// Helper functions
void printUsage();
vector<char> constructInputLine(string line);
bool checkTest(Circuit* myCircuit);
string printPIValue(char v);
//--------------------------

//----------------------------
// Functions for logic simulation
void simFullCircuit(Circuit* myCircuit);
void simGateRecursive(Gate* g);
void eventDrivenSim(Circuit* myCircuit, queue<Gate*> q);
char simGate(Gate* g);
char evalGate(vector<char> in, int c, int i);
char EvalXORGate(vector<char> in, int inv);
int LogicNot(int logicVal);
void setValueCheckFault(Gate* g, char gateValue);
//-----------------------------

//----------------------------
// Functions for PODEM:
bool podemRecursion(Circuit* myCircuit);
bool getObjective(Gate* &g, char &v, Circuit* myCircuit);
void updateDFrontier(Circuit* myCircuit);
void backtrace(Gate* &pi, char &piVal, Gate* objGate, char objVal, Circuit* myCircuit);

//--------------------------


///////////////////////////////////////////////////////////
// Global variables
// These are made global to make your life slightly easier.

/** Global variable: a vector of Gate pointers for storing the D-Frontier. */
vector<Gate*> dFrontier;

/** Global variable: holds a pointer to the gate with stuck-at fault on its output location. */
Gate* faultLocation;     

/** Global variable: holds the logic value you will need to activate the stuck-at fault. */
char faultActivationVal;

///////////////////////////////////////////////////////////



/** @brief The main function.
 * 
 * You do not need to change anything in the main function,
 * although you should understand what is happening
 * here.
 */
int main(int argc, char* argv[]) {

  // Check the command line input and usage
  if (argc != 4) {
    printUsage();    
    return 1;
  }
  
  // Parse the bench file and initialize the circuit. (Using C style for our parser.)
  FILE *benchFile = fopen(argv[1], "r");
  if (benchFile == NULL) {
    cout << "ERROR: Cannot read file " << argv[1] << " for input" << endl;
    return 1;
  }
  yyin=benchFile;
  yyparse();
  fclose(benchFile);

  myCircuit->setupCircuit();

  cout << endl;
   
  

  // Setup the output text file
  ofstream outputStream;
  outputStream.open(argv[2]);
  if (!outputStream.is_open()) {
    cout << "ERROR: Cannot open file " << argv[2] << " for output" << endl;
    return 1;
  }
    
  // Open the fault file.
  ifstream faultStream;
  string faultLocStr;
  faultStream.open(argv[3]);
  if (!faultStream.is_open()) {
    cout << "ERROR: Cannot open fault file " << argv[3] << " for input" << endl;
    return 1;
  }
  

  // For each line in our fault file...
  while(getline(faultStream, faultLocStr)) {

    // Clear the old fault
    myCircuit->clearFaults();
      
    string faultTypeStr;
      
    if (!(getline(faultStream, faultTypeStr))) {
      break;
    }
      
    char faultType = atoi(faultTypeStr.c_str());
      
    // set up the fault we are trying to detect
    faultLocation = myCircuit->findGateByName(faultLocStr);      
    faultLocation->set_faultType(faultType);      
    faultActivationVal = (faultType == FAULT_SA0) ? LOGIC_ONE : LOGIC_ZERO;
      
    // set all gate values to X
    for (int i=0; i < myCircuit->getNumberGates(); i++) {
      myCircuit->getGate(i)->setValue(LOGIC_X);
    }

    // initialize the D frontier.
    dFrontier.clear();
      
    // call PODEM recursion function
    bool res = podemRecursion(myCircuit);

    // If we succeed, print the test we found to the output file.
    if (res == true) {
      vector<Gate*> piGates = myCircuit->getPIGates();
      for (int i=0; i < piGates.size(); i++)
        outputStream << printPIValue(piGates[i]->getValue());
      outputStream << endl;
    }

    // If we failed to find a test, print a message to the output file
    else {
      outputStream << "none found" << endl;
    }

    // Lastly, you can use this to test that your PODEM-generated test
    // correctly detects the already-set fault.
    // Of course, this assumes that your simulation code is correct.

    // Don't use this code when you are evaluating the runtime of your
    // ATPG system because it will add extra time.
    if (res == true) {
      if (!checkTest(myCircuit)) {
        cout << "ERROR: PODEM returned true, but generated test does not detect fault on PO." << endl;
        myCircuit->printAllGates();
        assert(false);
      }
    }

    // Just printing to screen to let you monitor progress
    cout << "Fault = " << faultLocation->get_outputName() << " / " << (int)(faultType) << ";";
    if (res == true)
      cout << " test found" << endl;
    else
      cout << " no test found" << endl;
    
  }

  faultStream.close();
  
  // close the output and fault streams
  outputStream.close();

    
  return 0;
}


/////////////////////////////////////////////////////////////////////
// Functions in this section are helper functions.
// You should not need to change these, except if you want
// to enable the checkTest function (which will use your simulator
// to attempt to check the test vector computed by PODEM.)


/** @brief Print usage information (if user provides incorrect input).
 * 
 * You don't need to touch this.
 */
void printUsage() {
  cout << "Usage: ./atpg [bench_file] [output_loc] [fault_file]" << endl << endl;
  cout << "   bench_file:    the target circuit in .bench format" << endl;
  cout << "   output_loc:    location for output file" << endl;
  cout << "   fault_file:    faults to be considered" << endl;
  cout << endl;
  cout << "   The system will generate a test pattern for each fault listed" << endl;
  cout << "   in fault_file and store the result in output_loc." << endl;
  cout << endl;	
}


// You don't need to touch this.
/** @brief Used to parse in the values from the input file.
 * 
 * You don't need to touch this.
 */
vector<char> constructInputLine(string line) {
  
  vector<char> inputVals;
  
  for (int i=0; i<line.size(); i++) {
    if (line[i] == '0') 
      inputVals.push_back(LOGIC_ZERO);
    
    else if (line[i] == '1') 
      inputVals.push_back(LOGIC_ONE);

    else if ((line[i] == 'X') || (line[i] == 'x')) {
      inputVals.push_back(LOGIC_X);
    }
   
    else {
      cout << "ERROR: Do not recognize character " << line[i] << " in line " << i+1 << " of input vector file." << endl;
      assert(false);
      //inputVals.push_back(LOGIC_X);
    }
  }  
  return inputVals;
}

/** @brief Uses your simulator to check validity of your test.
 * 
 * This function gets called after your PODEM algorithm finishes.
 * If you enable this, it will clear the circuit's internal values,
 * and re-simulate the vector PODEM found to test your result.
 
 * This is helpful when you are developing and debugging, but will just
 * slow things down once you know things are correct.
 
 * This function of course assumes that your simulation code 
 * is correct.
*/
bool checkTest(Circuit* myCircuit) {

  // To enable this function, just comment out return true; here,
  // and the function will run.
  // To disable this function, just leave "return true;" here and
  // the system will not do the extra test after generating the vector.
  return true;

  simFullCircuit(myCircuit);

  // look for D or D' on an output
  vector<Gate*> poGates = myCircuit->getPOGates();
  for (int i=0; i<poGates.size(); i++) {
    char v = poGates[i]->getValue();
    if ((v == LOGIC_D) || (v == LOGIC_DBAR)) {
      return true;
    }
  }

  // If we didn't find D or D' on any PO, then our test was not successful.
  return false;

}



/** @brief Prints a PI value. 
 * 
 * This is just a helper function used when storing the final test you computed.
 *  You don't need to run or modify this.
 */
string printPIValue(char v) {
  switch(v) {
  case LOGIC_ZERO: return "0";
  case LOGIC_ONE: return "1";
  case LOGIC_UNSET: return "U";
  case LOGIC_X: return "X";
  case LOGIC_D: return "1";
  case LOGIC_DBAR: return "0";
  }
  return "";
}

// end of helper functions
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Start of functions for circuit simulation.



/** @brief Runs full circuit simulation
 *
 * Full-circuit simulation: set all non-PI gates to LOGIC_UNSET
 * and call the recursive simulate function on all PO gates.
 * Don't change this function unless you want to use your Project 2 code.
 */
void simFullCircuit(Circuit* myCircuit) {
  for (int i=0; i<myCircuit->getNumberGates(); i++) {
    Gate* g = myCircuit->getGate(i);
    if (g->get_gateType() != GATE_PI)
      g->setValue(LOGIC_UNSET);      
  }  
  vector<Gate*> circuitPOs = myCircuit->getPOGates();
  for (int i=0; i < circuitPOs.size(); i++) {
    simGateRecursive(circuitPOs[i]);
  }
}



// Recursive function to find and set the value on Gate* g.
// This function calls simGate and setValueCheckFault. 
// Don't change this function.
/** @brief Recursive function to find and set the value on Gate* g.
 * \param g The gate to simulate.
 * This function prepares Gate* g to to be simulated by recursing
 * on its inputs (if needed).
 * 
 * Then it calls \a simGate(g) to calculate the new value.
 * 
 * Lastly, it will set the Gate's output value based on
 * the calculated value.
 * 
 * \note Do not change this function. 
 */
void simGateRecursive(Gate* g) {

  // If this gate has an already-set value, you are done.
  if (g->getValue() != LOGIC_UNSET)
    return;
  
  // Recursively call this function on this gate's predecessors to
  // ensure that their values are known.
  vector<Gate*> pred = g->get_gateInputs();
  for (int i=0; i<pred.size(); i++) {
    simGateRecursive(pred[i]);
  }
  
  char gateValue = simGate(g);

  // After I have calculated this gate's value, check to see if a fault changes it and set.
  setValueCheckFault(g, gateValue);
}


/** @brief Perform event-driven simulation.
 * \note You will write this function in Part 3. See psuedocode in its comments.
 * 
 * Please see the project handout for a description of what
 * we are doing here and why.

 * This function takes as input the Circuit* and a queue<Gate*>
 * indicating the remaining gates that need to be evaluated.
 */
void eventDrivenSim(Circuit* myCircuit, queue<Gate*> q) {

  // Basic idea: 
  // - Keep a queue (input q) of gates whose output values may potentially change
  // - while there are still gates in the queue: 
  //      - Pop a gate from this queue. Store its currently-set output value.
  //      - Based on the gate's inputs, calculate its new output value. Note that you don't
  //        need to do this from scratch, you can use the simGate() function below.
  //      - Check to see if the new gate output value differs from the old one. If it does
  //        add its fanout gates on the queue  

}



/** @brief Simulate the value of the given Gate.
 *
 * This is a gate simulation function -- it will simulate the gate g
 * with its current input values and return the output value.
 * This function does not deal with the fault. (That comes later.)
 * \note You do not need to change this function.
 *
 */
char simGate(Gate* g) {
  // For convenience, create a vector of the values of this
  // gate's inputs.
  vector<Gate*> pred = g->get_gateInputs();
  vector<char> inputVals;   
  for (int i=0; i<pred.size(); i++) {
    inputVals.push_back(pred[i]->getValue());      
  }

  char gateType = g->get_gateType();
  char gateValue;
  // Now, set the value of this gate based on its logical function and its input values
  switch(gateType) {   
  case GATE_NAND: { gateValue = evalGate(inputVals, 0, 1); break; }
  case GATE_NOR: { gateValue = evalGate(inputVals, 1, 1); break; }
  case GATE_AND: { gateValue = evalGate(inputVals, 0, 0); break; }
  case GATE_OR: { gateValue = evalGate(inputVals, 1, 0); break; }
  case GATE_BUFF: { gateValue = inputVals[0]; break; }
  case GATE_NOT: { gateValue = LogicNot(inputVals[0]); break; }
  case GATE_XOR: { gateValue = EvalXORGate(inputVals, 0); break; }
  case GATE_XNOR: { gateValue = EvalXORGate(inputVals, 1); break; }
  case GATE_FANOUT: {gateValue = inputVals[0]; break; }
  default: { cout << "ERROR: Do not know how to evaluate gate type " << gateType << endl; assert(false);}
  }    

  return gateValue;
}


/** @brief Evaluate a NAND, NOR, AND, or OR gate.
 * \param in The logic value's of this gate's inputs.
 * \param c The controlling value of this gate type (e.g. c==0 for an AND or NAND gate)
 * \param i The inverting value for this gate (e.g. i==0 for AND and i==1 for NAND)
 * \returns The logical value produced by this gate (not including a possible fault on this gate).
 * \note You do not need to change this function.
 */
char evalGate(vector<char> in, int c, int i) {

  // Are any of the inputs of this gate the controlling value?
  bool anyC = find(in.begin(), in.end(), c) != in.end();
  
  // Are any of the inputs of this gate unknown?
  bool anyUnknown = (find(in.begin(), in.end(), LOGIC_X) != in.end());

  int anyD    = find(in.begin(), in.end(), LOGIC_D)    != in.end();
  int anyDBar = find(in.begin(), in.end(), LOGIC_DBAR) != in.end();


  // if any input is c or we have both D and D', then return c^i
  if ((anyC) || (anyD && anyDBar))
    return (i) ? LogicNot(c) : c;
  
  // else if any input is unknown, return unknown
  else if (anyUnknown)
    return LOGIC_X;

  // else if any input is D, return D^i
  else if (anyD)
    return (i) ? LOGIC_DBAR : LOGIC_D;

  // else if any input is D', return D'^i
  else if (anyDBar)
    return (i) ? LOGIC_D : LOGIC_DBAR;

  // else return ~(c^i)
  else
    return LogicNot((i) ? LogicNot(c) : c);
}

/** @brief Evaluate an XOR or XNOR gate.
 * \param in The logic value's of this gate's inputs.
 * \param inv The inverting value for this gate (e.g. i==0 for XOR and i==1 for XNOR)
 * \returns The logical value produced by this gate (not including a possible fault on this gate).
 * \note You do not need to change this function.
 */
char EvalXORGate(vector<char> in, int inv) {

  // if any unknowns, return unknown
  bool anyUnknown = (find(in.begin(), in.end(), LOGIC_X) != in.end());
  if (anyUnknown)
    return LOGIC_X;

  // Otherwise, let's count the numbers of ones and zeros for faulty and fault-free circuits.
  // This is not required for your project, but this will with with XOR and XNOR with > 2 inputs.
  int onesFaultFree = 0;
  int onesFaulty = 0;

  for (int i=0; i<in.size(); i++) {
    switch(in[i]) {
    case LOGIC_ZERO: {break;}
    case LOGIC_ONE: {onesFaultFree++; onesFaulty++; break;}
    case LOGIC_D: {onesFaultFree++; break;}
    case LOGIC_DBAR: {onesFaulty++; break;}
    default: {cout << "ERROR: Do not know how to process logic value " << in[i] << " in Gate::EvalXORGate()" << endl; return LOGIC_X;}
    }
  }
  
  int XORVal;

  if ((onesFaultFree%2 == 0) && (onesFaulty%2 ==0))
    XORVal = LOGIC_ZERO;
  else if ((onesFaultFree%2 == 1) && (onesFaulty%2 ==1))
    XORVal = LOGIC_ONE;
  else if ((onesFaultFree%2 == 1) && (onesFaulty%2 ==0))
    XORVal = LOGIC_D;
  else
    XORVal = LOGIC_DBAR;

  return (inv) ? LogicNot(XORVal) : XORVal;

}


/** @brief Perform a logical NOT operation on a logical value using the LOGIC_* macros
 * \note You will not need to modify this function.
 */
int LogicNot(int logicVal) {
  if (logicVal == LOGIC_ONE)
    return LOGIC_ZERO;
  if (logicVal == LOGIC_ZERO)
    return LOGIC_ONE;
  if (logicVal == LOGIC_D)
    return LOGIC_DBAR;
  if (logicVal == LOGIC_DBAR)
    return LOGIC_D;
  if (logicVal == LOGIC_X)
    return LOGIC_X;
      
  cout << "ERROR: Do not know how to invert " << logicVal << " in LogicNot(int logicVal)" << endl;
  return LOGIC_UNSET;
}

/** @brief Set the value of Gate* g to value gateValue, accounting for any fault on g.
    \note You will not need to modify this.
 */
void setValueCheckFault(Gate* g, char gateValue) {
  if ((g->get_faultType() == FAULT_SA0) && (gateValue == LOGIC_ONE)) 
  	g->setValue(LOGIC_D);
  else if ((g->get_faultType() == FAULT_SA0) && (gateValue == LOGIC_DBAR)) 
  	g->setValue(LOGIC_ZERO);
  else if ((g->get_faultType() == FAULT_SA1) && (gateValue == LOGIC_ZERO)) 
  	g->setValue(LOGIC_DBAR);
  else if ((g->get_faultType() == FAULT_SA1) && (gateValue == LOGIC_D)) 
  	g->setValue(LOGIC_ONE);
  else
  	g->setValue(gateValue);
}

// End of functions for circuit simulation
////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////
// Begin functions for PODEM.

// TODO
/** @brief PODEM recursion.
 *
 * \note For Part 1, you must write this, following the pseudocode in class and in the code's comments.
 */
bool podemRecursion(Circuit* myCircuit) {

  // If D or D' is at an output, then return true
    char val;
	
	vector<Gate*> opGates = myCircuit->getPOGates();
	for (int i=0; i<opGates.size(); i++) {
    val = opGates[i]->getValue();
    if ((val == LOGIC_D) || (val == LOGIC_DBAR)) 
      return true;    
	}

   Gate* g;
   char v;  

  // Call the getObjective function. Store the result in g and v.    
  // If getObjective fails, return false	
  
	bool obj = getObjective(g, v, myCircuit);
	///////////
	if (obj == false ) return false;
	
  Gate* pi;
  char piVal;
  
  // Call the backtrace function. Store the result in pi and piVal.
  
  backtrace(pi, piVal, g, v, myCircuit);
  
  // Set the value of pi to piVal. Use your setValueCheckFault function (see above)
  // to make sure if there is a fault on the PI gate, it correctly gets set.
  
  setValueCheckFault(pi, piVal);
  
  // Now, determine the implications of the input you set by simulating 
  // the circuit by calling simFullCircuit(myCircuit);
  
  simFullCircuit(myCircuit);
   
  
  if (podemRecursion(myCircuit)) return true;
  // If the recursive call fails, set the opposite PI value, simulate, it and recurse.
  // If this recursive call succeeds, return true.
  
  char notpiVal;
  notpiVal= LogicNot(piVal);
  
  setValueCheckFault(pi, notpiVal); 
  
  simFullCircuit(myCircuit);
  if (podemRecursion(myCircuit)) return true;
  
  // If we get to here, neither pi=v nor pi = v' worked. So, set pi to value X and 
  // return false.
  
  setValueCheckFault(pi, LOGIC_X);

  return false;

}

// Find the objective for myCircuit. The objective is stored in g, v.
// 
// TODO Write this function, based on the pseudocode from
// class or your textbook.
/** @brief PODEM objective function.
 *  \param g Use this pointer to store the objective Gate your function picks.
 *  \param v Use this char to store the objective value your function picks.
 *  \returns True if the function is able to determine an objective, and false if it fails.
 * \note For Part 2, you must write this, following the pseudocode in class and the code's comments.
 */

bool getObjective(Gate* &g, char &v, Circuit* myCircuit) {

  // First you will need to check if the fault is activated yet.
  // Note that in the setup above we set up a global variable
  // Gate* faultLocation which represents the gate with the stuck-at
  // fault on its output. Use that when you check if the fault is
  // excited.

  
  // Another note: if the fault is not excited but the fault 
  // location value is not X, then we have failed to activate 
  // the fault. In this case getObjective should fail and Return false.  
	
	if (faultLocation->getValue()== LOGIC_X) {g= faultLocation; v=faultActivationVal; 
		return true;}
	
	if (faultLocation->getValue()== LOGIC_ONE || faultLocation->getValue()== LOGIC_ZERO)
	{return false;} 
	//setValueCheckFault(faultLocation, faultLocation->getValue());

  // If the fault is already activated, then you will need to 
  // use the D-frontier to find an objective.
  // Before you use the D-frontier you should update it by running:
  
  updateDFrontier(myCircuit);

  // This function should update the global D-frontier variable
  // vector<Gate*> dFrontier;
  
  // If the D frontier is empty after update, then getObjective fails
  // and should return false.
	
	if (dFrontier.empty()) return false;
	
	
  // getObjective needs to choose a gate from the D-Frontier.
  // For part 1, pick dFrontier[0] if you want to match my reference outputs.
	Gate* d;	
	d = dFrontier[0];
	
	// Later, a possible optimization is to use the 
  // SCOAP observability metric or other smart methods to choose this carefully.
    

  // Lastly, set the values of g and v based on the
  // gate you chose from the D-Frontier.

	
	vector<Gate*> dinputs = d->get_gateInputs();
	
		for (int i=0; i<dinputs.size(); i++) 
			{
				if (dinputs[i]->getValue()== LOGIC_X)
				{
					g = dinputs[i]; break;
				}
			}
			
	if (d->get_gateType()==GATE_AND || d->get_gateType()==GATE_NAND) v=LOGIC_ONE;
	else if (d->get_gateType()==GATE_OR || d->get_gateType()==GATE_NOR) v=LOGIC_ZERO;
	else if (d->get_gateType()==GATE_XOR || d->get_gateType()==GATE_XNOR) v=LOGIC_ZERO;
	else v=LOGIC_X;
	
	
  return true;

}


// A very simple method to update the D frontier.
// TODO: Write this code based on the pseudocode below.
/** @brief A simple method to compute the set of gates on the D frontier.
 *
 * \note For Part 1, you must write this. The simplest form follows the pseudocode included in the comments.
 */

void updateDFrontier(Circuit* myCircuit) {
  // Procedure:
  //  - clear the dFrontier vector (stored as the global variable dFrontier -- see the top of the file)
  
	dFrontier.clear();
	
  //  - loop over all gates in the circuit; for each gate, check if it should be on D-frontier; if it is,
  //    add it to the dFrontier vector.

	Gate* G;
	int numberofgates = myCircuit->getNumberGates();
	
	for (int i=0; i< numberofgates; i++)
	{
		G = myCircuit->getGate(i);
		if (G->getValue() != LOGIC_X) continue;
		else
		{
			vector<Gate*> Ginputs = G->get_gateInputs(); 
			for (int j=0; j<Ginputs.size(); j++) 
				{ 
					if (Ginputs[j]->getValue()== LOGIC_D || Ginputs[j]->getValue() == LOGIC_DBAR)
					{
						dFrontier.push_back(G); break;
					}
					
				}		
			
		}
	}
  
}


// Backtrace: given objective objGate and objVal, then figure out which input (pi) to set 
// and which value (piVal) to set it to.
// TODO: write this

/** @brief PODEM backtrace function
 * \param pi Output: A Gate pointer to the primary input your backtrace function found.
 * \param piVal Output: The value you want to set that primary input to
 * \param objGate Input: The objective Gate (computed by getObjective)
 * \param objVal Input: the objective value (computed by getObjective)
 * \note Write this function based on the psuedocode from class.
 */
void backtrace(Gate* &pi, char &piVal, Gate* objGate, char objVal, Circuit* myCircuit) {

	pi = objGate;int k1;int cnt=0;
	int num_inversions;
	char gatetype = pi->get_gateType();
	
	if (gatetype == GATE_NOR || gatetype == GATE_NOT || gatetype == GATE_NAND || gatetype == GATE_XNOR)
		num_inversions=1;
	else num_inversions=0;
	
	
	while (pi->get_gateType()!=GATE_PI)
	{ 
		vector<Gate*> gateinputs = pi->get_gateInputs();//
		
		for (k1=0; k1<gateinputs.size(); k1++) 
			{ 
				if (gateinputs[k1]->getValue()== LOGIC_X) {pi=gateinputs[k1]; break;} 
				
			}
			
			gatetype = pi->get_gateType();
			
		if (gatetype == GATE_NOR || gatetype == GATE_NOT || gatetype == GATE_NAND || gatetype == GATE_XNOR)
		{num_inversions++; }
	}
	
	if (num_inversions%2==1) piVal= LogicNot(objVal);
	else piVal = objVal;
	
}


////////////////////////////////////////////////////////////////////////////
// Place any new functions you add here, between these two bars.



////////////////////////////////////////////////////////////////////////////


