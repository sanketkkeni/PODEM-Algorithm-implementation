#ifndef CLASSCIRCUIT_H
#define CLASSCIRCUIT_H

#include "ClassGate.h"
#include <assert.h>  // assert
#include <iostream>  // cout
#include <vector>    // vector
#include <sstream>

class Circuit{
 private:
  vector<Gate*> gates;            // Pointers to all gates in the circuit
  vector<Gate*> outputGates;      // Pointers to all gates driving POs
  vector<Gate*> inputGates;       // Pointers to all PIs
  vector<string> outputNames;     // A vector with output names (only used in setup)
  void checkPointerConsistency(); // An internal function to check that the Circuit is setup correctly.

  
 public:
  Circuit();
  void newGate(string name, int ID, int gt);
  Gate* getGate(int i);
  void addOutputName(string n);
  void printAllGates();
  void setupCircuit();
  Gate* findGateByName(string name);
  void setPIValues(vector<char> inputVals);
  vector<int> getPOValues();
  int getNumberPIs();
  int getNumberPOs();
  int getNumberGates();
  void clearGateValues(); 
  vector<Gate*> getPIGates();
  vector<Gate*> getPOGates();
  void clearFaults();
  
};

#endif
