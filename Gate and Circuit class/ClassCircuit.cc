
 /** \class Circuit
 * \brief A circuit contains primary inputs and outputs, and a number of interconnected gates.
 *
 * The API for interacting with a Circuit includes functions to access the gates in the
 * system. Functions you may use in your project include \a getPIGates() and \a getPOGates(), which
 * return the inputs and outputs of the circuit. Also useful are \a setPIValues(), which the reference
 * code uses to set the values of the inputs of the circuit, and \a clearGateValues() which it uses
 * to clear the logical values after each simulation iteration.
 *
 * An important thing to understand: we will use a special Gate type called a PI to represent
 * the circuit's primary inputs. This isn't really a logic gate, it's just a way of 
 * making it easy to access the inputs. So, when you call \a getPIGates() you will get a vector of
 * pointers to the special PI gates.
 * 
 * We don't use a special structure for the primary outputs (POs); instead we simply maintain a 
 * list of which gates drive the output values. So when you call \a getPOGates() you will get
 * a vector of pointers to the normal gates that drive the outputs.
 * 
 * Lastly, note that there are a number of functions here that are only used when the initial 
 * representation of the circuit is constructed. (This is done for you by the yyparse() function 
 * (see main.cc). These functions are ones that you will never have to manipulate yourself, and 
 * they are marked with a note that indicate this.
 */

#include "ClassCircuit.h"

/** \brief Construct a new circuit */
Circuit::Circuit() {}

/** \brief Add a new gate to the circuit
 *  \param name a string providing the output name for the gate
 *  \param ID a unique ID number for the gate (primarily used while parsing the input source file)
 *  \param gt the type of gate, using the GATE_* marcos defined in ClassGate.h
 *  \note This function should only need to be run by the parser.
 */
void Circuit::newGate(string name, int ID, int gt) {
    Gate* g = new Gate(name, ID, gt);
    gates.push_back(g);

    if (gt == GATE_PI)
      inputGates.push_back(g);
}

/** \brief Get pointer to gate \a i from the circuit
 *  \param i gate number
 *  \return Pointer to Gate \a i
 */
Gate* Circuit::getGate(int i) {
  if (i >= gates.size()) {
    cout << "ERROR: Requested gate out of bounds" << endl;
    assert(false);
  }
  return gates[i];
}

/** \brief Record the name of a primary output of this circuit.
 *  \param n Name of the output signal
 *  \note This function should only need to be run by the parser.
 */
void Circuit::addOutputName(string n) {
  outputNames.push_back(n);
}

/** \brief Print the circuit
 */
void Circuit::printAllGates() {
  cout << "Inputs: ";
  for (int i=0; i<inputGates.size(); i++)
    cout << inputGates[i]->get_outputName() << " ";
  cout << endl;

  cout << "Outputs: ";
  for (int i=0; i<outputGates.size(); i++)
    cout << outputGates[i]->get_outputName() << " ";
  cout << endl;
  
  for (int i=0; i<gates.size(); i++) {
    gates[i]->printGateInfo();
  }

}  

/** \brief Returns a pointer to the Gate in this circuit with output name \a name.
 *  \param name A string containing the name of the gate requested
 *  \return Pointer to the Gate with output given by \a name
 *  Will fail an assertion if multiple gates with that name are found, or if none are.
 *  \note This function should probably only need to be called by the parser.
 */
Gate* Circuit::findGateByName(string name) {
	Gate* res;
	bool found = false;

	for (int i=0; i<gates.size(); i++) {
		if (gates[i]->get_outputName().compare(name) == 0) {
			res = gates[i];
			assert(!found);
			found = true;
		}
  	}
  
  if (!found)
  	cout << "ERROR: Cannot find: " << name << endl;
  	
  assert(found);
  return res;
  
}

/** \brief Sets up the circuit data structures after parsing is complete.
 *  Run this once after parsing, before using the data structure.
 *  The handout \a main.cc code already does this; you do not need to add it yourself.
 */
void Circuit::setupCircuit() {

  // set-up the vector of output gates based on their pre-stored names
  for (int i=0; i<outputNames.size(); i++) {
    outputGates.push_back(findGateByName(outputNames[i]));
  }

  // set input and output pointers of each gate
  for (int i=0; i<gates.size(); i++) {
    Gate* g = gates[i];
    vector<string> names = g->get_gateInputNames();
    for (int j=0; j<names.size(); j++) {
      string n = names[j];
      Gate* inGate = findGateByName(n);
      inGate->set_gateOutput(g);
      g->set_gateInput(inGate);
    }
  }
  

  // In order for the fault simulator to consider fanout stems and
  // branches as different faults, here we find all the gates that have
  // fanout > 1, and add special FANOUT gates.
  // After this, the only gates that have fanout > 1 will have GATE_FANOUT gates
  // as their only output gates. 
  // 
  // Example: A = AND(B, C)
  //          D = OR(A, E)
  //          F = OR(A, G)
  //
  //          Here we see that A has fanout of two. Our function here will change it to:
  //          A = AND(B, C)
  //          D = OR(A_0, E)
  //          F = OR(A_1, G)
  //          A_0 = FANOUT(A)
  //          A_1 = FANOUT(A)	
  //
  // Given this new type of gate, now any possible single-stuck-at gate fault site
  // is a gate output.
  for (int i=0; i<gates.size(); i++) {
  	Gate* g = gates[i];
  	vector<Gate*> go = g->get_gateOutputs();
  	
  	if ((g->get_gateType() != GATE_FANOUT) && (go.size() > 1)) {  		
  		for (int j=0; j<go.size(); j++) {	
  		
  			// Before running: 
  			//    g --> go[j]
  			// After:
  			//    g --> newG --> go[j], where newG is a gate of type FANOUT
  		
  			ostringstream ss;
  			ss << g->get_outputName() << "_" << (int)j;
			newGate(ss.str(), gates.size(), GATE_FANOUT);  		
			Gate* newFanoutGate = gates.back();
			
			// change g's output[j] to point to newFanoutGate
			g->replace_gateOutput(go[j], newFanoutGate);
			
			// set newFanoutGate's input to point to g
			newFanoutGate->set_gateInput(g);
			
			// change go[j]'s appropriate input to point to newFanoutGate
			go[j]->replace_gateInput(g, newFanoutGate);
			
			// set newFanoutGate's output to point to go[j]
			newFanoutGate->set_gateOutput(go[j]);
			
			
		}
  	}
  }
  
  checkPointerConsistency();

}

/** \brief Initializes the values of the PIs of the circuit.
 *  \param inputVals the desired input values (using LOGIC_* macros).
 */
void Circuit::setPIValues(vector<char> inputVals) {
  if (inputVals.size() != inputGates.size()) {
    cout << "ERROR: Incorrect number of input values: " << inputVals.size() << " vs " << inputGates.size() << endl;
    assert(false);
  }

  for (int i=0; i<inputVals.size(); i++) {
  
  	if ((inputGates[i]->get_faultType() == FAULT_SA0) && (inputVals[i] == LOGIC_ONE)) 
  		inputGates[i]->setValue(LOGIC_D);
	else if ((inputGates[i]->get_faultType() == FAULT_SA0) && (inputVals[i] == LOGIC_DBAR)) 
	  	inputGates[i]->setValue(LOGIC_ZERO);
	else if ((inputGates[i]->get_faultType() == FAULT_SA1) && (inputVals[i] == LOGIC_ZERO)) 
	  	inputGates[i]->setValue(LOGIC_DBAR);
	else if ((inputGates[i]->get_faultType() == FAULT_SA1) && (inputVals[i] == LOGIC_D)) 
	  	inputGates[i]->setValue(LOGIC_ONE);
	else
	  	inputGates[i]->setValue(inputVals[i]);
  }

}

/** \brief Returns the output values on each PO of this circuit.
 */
vector<int> Circuit::getPOValues() { 
  vector<int> outVals;
  for (int i=0; i < outputGates.size(); i++) 
    outVals.push_back(outputGates[i]->getValue());

  return outVals;
}

/** \brief Get the number of PIs of the circuit.
 *  \return The number of PIs of the circuit. */
int Circuit::getNumberPIs() { return inputGates.size(); }

/** \brief Get the number of POs of the circuit.
 *  \return The number of POs of the circuit. */
int Circuit::getNumberPOs() { return outputGates.size(); }

/** \brief Get the number of gates of the circuit.
 *  \return The number of gates of the circuit.
 */
int Circuit::getNumberGates() { return gates.size(); }

/** \brief Clears the value of each gate in the circuit (to LOGIC_UNSET) */
void Circuit::clearGateValues() {
  for (int i=0; i<gates.size(); i++) {
    gates[i]->setValue(LOGIC_UNSET);
  }
    
}

/** \brief Returns the PI (input) gates. (The PIs of the circuit).
    \return a \a vector<Gate*> of the circuit's PIs */
vector<Gate*> Circuit::getPIGates() { return inputGates; }

/** \brief Returns the PO (output) gates. (The gates which drive the POs of the circuit.)
    \return a \a vector<Gate*> of the circuit's POs */
vector<Gate*> Circuit::getPOGates() { return outputGates; }

/** \brief Private function for Circuit to check input and output pointers
 *   for all gates are set consistently. Just used in setting up circuit.
 */ 
void Circuit::checkPointerConsistency() {
  
  for (int i=0; i<gates.size(); i++) {
    Gate* g = gates[i];
    
    // every gate in g's input list must have g in its output list
    vector<Gate*> gi = g->get_gateInputs();
    for (int j=0; j < gi.size(); j++) {
      vector<Gate*> gi_go = gi[j]->get_gateOutputs();
      assert(find(gi_go.begin(), gi_go.end(), g) != gi_go.end());
    }
    
    // every gate in g's output list must have g in its input list
    vector<Gate*> go = g->get_gateOutputs();
    for (int j=0; j < go.size(); j++) {
      vector<Gate*> go_gi = go[j]->get_gateInputs();
      assert(find(go_gi.begin(), go_gi.end(), g) != go_gi.end());
    }	
  }	
  
  // check that fanout goes to FANOUT gates only
  for (int i=0; i<gates.size(); i++) {
    Gate* g = gates[i];
    vector<Gate*> go = g->get_gateOutputs();
    
    if (go.size() > 1) {
      for (int j=0; j<go.size(); j++) {
	assert(go[j]->get_gateType() == GATE_FANOUT);
      }
    }
  }
}

/** \brief Clears faults on all gates 
 *  \note We run this function as part of the setup code; you should not need to run this.
*/
void Circuit::clearFaults() {
  for (int i=0; i<gates.size(); i++) {
    gates[i]->set_faultType(NOFAULT);
  }
}
