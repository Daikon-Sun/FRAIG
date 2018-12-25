/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"

extern CirMgr *cirMgr;

class CirMgr
{
//typedef vector< vector<unsigned> >* vvp;
typedef myList< myList<unsigned>* > mmu;
typedef myListNode< myList<unsigned>* >* mnmup;
//typedef mmu::iterator mmuit;
typedef myList<unsigned> mu;
typedef myListNode< unsigned >* mnup;
//typedef mu::iterator muit;

public:
   CirMgr() : gate_list(0), line_list(0), simt(0) {}
   ~CirMgr();

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const
   {
      if( gid < gate_list.size() && valid_id(gid) )
         return gate_list[gid];
      else return 0;
   }
   bool valid_id(unsigned id) const
   {
      return( (size_t)gate_list[id] > para[0]+para[3] );
   }
   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit optimization
   void sweep();
   void optimize();

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }

   // Member functions about fraig
   void strash();
   void printFEC() const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs() const;
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;

   CirGate* rtn_gate(unsigned i) const { return gate_list[i]; }
   CirGate* rtn_line(unsigned i) const { return line_list[i]; }
   unsigned rtn_topo(unsigned i) const { return topo[i];}

private:
   ofstream  *_simLog;
   unsigned  para[5];
   GateList  gate_list;
   CirGate** line_list;
   mmu* simt;
   unsigned b_size;
   vector<unsigned> addr,topo;
   vector<unsigned long long> patts;
   vector<unsigned>* vp;

   void dfsT( CirGate* ) const;
   void dfsT( CirGate*, GateList& ) const;
   void dfsT_for_AIG( CirGate*, GateList& ) const;
   void dfsT_for_Opt( CirGate*& );
   void dfsT_for_strash(CirGate*, HashMap<HashKey, unsigned>& );
   void dfsT_for_sim(CirGate*&,bool&);
   void dfsT_for_SIM(CirGate*&,bool&);
   void dfsT_for_PI(CirGate*&,GateList&);
   void dfsT_for_fraig(CirGate*&,vector<unsigned long long>&);
   void dfsT_for_order(CirGate*&,vector<unsigned>&);
   void mySim(bool&);
   bool init_simt(bool&);
   unsigned renew_simt();
   void refresh_simt(unsigned);
   unsigned cal_fail_limit(unsigned);
   int parse_patt(ifstream&);
   void logging();
   void parseModel(SatSolver&);
   void merge(CirGate*&,CirGate*&,bool&);
   void solveCNF(SatSolver&,vector<unsigned>&);
   
   void same_or_one( bool, CirGate*&, bool);
   void opp_or_zero( bool, CirGate*&, bool);

   unsigned rdirec(CirGate*&, CirGate*&) const;
   unsigned ldirec(CirGate*&, CirGate*&) const;
};

#endif // CIR_MGR_H
