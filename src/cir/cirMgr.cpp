/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

bool comp_line( CirGate* c1, CirGate* c2)
{
   if( c1 == 0 || c2 == 0 ) return false;
   else
      return (c1->getLineNo() <= c2->getLineNo() );
}
bool comp_id( CirGate* c1, CirGate* c2)
{
   if( c1 == 0 || c2 == 0 ) return false;
   else
      return (c1->rtn_own_id() <= c2->rtn_own_id() );
}
/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};
/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine const (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
bool CirMgr::readCircuit(const string& fileName)
{
   ifstream aiger(fileName.c_str());
   string head = "";

   aiger >> head;
   for(unsigned i = 0; i<5; ++i)
      aiger >> para[i];
   aiger.get(); //get the \n after header
   
   line_list = new CirGate*[para[1]+para[3]+para[4]+2];
   line_list[0] = line_list[1] = 0;
   gate_list.resize(para[0]+para[3]+1);
   gate_list[0] = new PI(0,0);
   unsigned sum = 0;
   for(unsigned i = 0; i<para[1]; ++i)
   {
      aiger >> sum;
      gate_list[sum/2] = new PI(sum/2, i+2);
      line_list[i+2] = gate_list[sum/2];
   }
   for(unsigned i = 0; i<para[3]; ++i)
   {
      aiger >> sum;
      gate_list[para[0]+i+1]=new PO(para[0]+i+1,sum,i+para[1]+2);
      line_list[i+para[1]+2] = gate_list[i+para[0]+1];
   }
   for(unsigned i = 0; i<para[4]; ++i)
   {
      unsigned in[2];
      aiger >> sum >> in[0] >> in[1];
      gate_list[sum/2]= new And(sum/2,in[0],in[1],i+para[1]+para[3]+2);
      line_list[i+para[1]+para[3]+2] = gate_list[sum/2];
   }
   for(unsigned i = 1; i<para[0]+1; ++i)
   {
      if( valid_id(i) && 
          gate_list[i]->getTypeStr().compare("AIG")==0 )
      {
         for(unsigned k = 0; k<2; ++k)
         {
            unsigned id = gate_list[i]->rtn_iid(k);
            if( valid_id(id) )
               gate_list[id]->add_id(i);
            else
               gate_list[id] = (CirGate*)(size_t)i;
         }
      }
   }
   for(unsigned i = para[0]+1; i<para[3]+para[0]+1; ++i)
   {
      if( valid_id(i) )
      {
         unsigned id = gate_list[i]->rtn_iid(0);
         if( valid_id(id) )
            gate_list[id]->add_id(i);
         else
            gate_list[id] = (CirGate*)(size_t)i;
      }
   }
   aiger.get();
   if( aiger.peek() != EOF && aiger.peek() != 'c')
   {
      for(unsigned j = 0; j<para[1]+para[3]; ++j)
      {
         string str;
         unsigned sum;
 
         bool do_i = false;
         if( aiger.peek() == 'i' ) do_i = true;
         aiger.get();
         aiger >> sum;
         aiger >> str;
         if( do_i )
            line_list[sum+2]->set_symbol(str);
         else
            gate_list[para[0]+sum+1]->set_symbol(str);
         aiger.get();
         if( aiger.peek() == EOF || aiger.peek() == 'c' )  break;
      }
   }
   colNo = 0;
   topo.resize(para[0]+para[3]+1);
   CirGate::set_gRef();
   for( unsigned i = 1+para[0] ; i <= para[3]+para[0] ; ++i )
   {
      dfsT_for_order( gate_list[i],topo ) ;
      topo[gate_list[i]->rtn_own_id()] = colNo++;
   }
   topo[0] = 0;
   colNo = 0;
   /*if( aiger.peek() != EOF )
   {
      aiger.get(); // get c
      aiger.get(); // get \n
      unsigned beg = aiger.tellg();
      aiger.seekg(0, aiger.end);
      unsigned end = aiger.tellg();
      buf_length = end-beg;
      aiger.seekg(beg);
      buf = new char [buf_length];
      aiger.read(buf, buf_length);
   }*/
   return true;
}
/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
void CirMgr::printSummary() const
{
   cout << endl;
   cout << "Circuit Statistics" << endl;
   cout << "==================" << endl;
   cout << "  PI" << setw(12) << right << para[1] << endl;
   cout << "  PO" << setw(12) << right << para[3]  << endl;
   cout << "  AIG" << setw(11) << right << para[4] << endl;
   cout << "------------------" << endl;
   cout << "  Total" << setw(9) << right << para[1]+para[3]+para[4] << endl;
}
void CirMgr::printNetlist() const
{
   cout << endl;
   GateList dfsT_list;
   CirGate::set_gRef();
   lineNo = 0;
   for( unsigned i = 1+para[0] ; i <= para[3]+para[0] ; ++i )
      dfsT( gate_list[i], dfsT_list) ;
   for(unsigned i = 0; i<dfsT_list.size(); ++i)
   {
      cout << "[" << lineNo << "] ";
      dfsT_list[i]->printGate();
      cout << endl;
      ++lineNo;
   }
   lineNo = 0;
}
void CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   for(unsigned i = 2 ; i<para[1]+2; ++i)
      cout << ' ' << line_list[i]->rtn_own_id();
   cout << endl;
}
void CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   for(unsigned i = para[1]+2 ; i<para[1]+para[3]+2; ++i)
      cout << " " << line_list[i]->rtn_own_id();
   cout << endl;
}
void CirMgr::printFloatGates() const
{
   GateList floatG1, floatG2;
   for(unsigned i = 1; i<=para[0]+para[3]; ++i)
      if( valid_id(i) && gate_list[i]->check_float_1())
         floatG1.push_back(gate_list[i]);
   if( floatG1.size() > 0 )
   {
      cout << "Gates with floating fanin(s):";
      for(unsigned i = 0; i<floatG1.size(); ++i)
         cout << " " << floatG1[i]->rtn_own_id();
      cout << endl;
   }
   for(unsigned i = 1; i<=para[0]+para[3]; ++i)
      if( valid_id(i) && gate_list[i]->check_float_2())
         floatG2.push_back(gate_list[i]);
   if( floatG2.size() > 0 )
   {
      cout << "Gates defined but not used  :";
      for(unsigned i = 0; i<floatG2.size(); ++i)
         cout << " " << floatG2[i]->rtn_own_id();
      cout << endl;
   }
}
void CirMgr::printFECPairs() const
{
   if( simt == 0 ) return;
   unsigned pair_num = 0;
   for(unsigned i = 0; i<b_size; ++i)
   {
      if( simt[i].size() == 0 ) continue;
      for(mnmup j=(simt[i]._h)->_n;j!=simt[i]._h;j=j->_n)
      {
         cout << "[" << pair_num++ << "]";
         vector<unsigned> group;
         for(mnup k=j->_d->_h->_n;k!=j->_d->_h;k=k->_n)
            group.push_back(k->_d);
         std::sort(group.begin(),group.end());
         bool inv = ((group[0]/2)*2!=group[0]);
         for(unsigned k = 0, n = group.size(); k<n; ++k)
            cout<<" "<<((((group[k]/2)*2!=group[k])^inv)?"!":"")
                <<(group[k]/2);
         cout << endl;
      }
   }
}
void CirMgr::writeAag(ostream& outfile) const
{
   GateList dfsT_list;
   CirGate::set_gRef();
   for( unsigned i = 1 ; i <= para[3] ; ++i )
      dfsT_for_AIG( gate_list[i+para[0]], dfsT_list) ;
   outfile << "aag " << para[0] << " " << para[1] << " " << para[2];
   outfile << " " << para[3] << " " << dfsT_list.size() << "\n";
   for( unsigned i = 2 ; i<para[1]+2; ++i )
      outfile << line_list[i]->rtn_own_id()*2 << "\n";
      
   for( unsigned i = 1+para[0] ; i<=para[3]+para[0]; ++i)
      outfile << gate_list[i]->rtn_riid(0) << "\n";
   for(unsigned i = 0 ; i<dfsT_list.size(); ++i)
   {
      outfile << dfsT_list[i]->rtn_own_id()*2 << " "
      << dfsT_list[i]->rtn_riid(0) << " "
      << dfsT_list[i]->rtn_riid(1) << endl;
   }
   for( unsigned i = 1 ; i<=para[0] ; ++i )
   {
      if( valid_id(i) && gate_list[i]->rtn_symbol().size() > 0 )
         outfile<<"i"<<gate_list[i]->rtn_own_id()-1
                <<" "<< gate_list[i]->rtn_symbol() <<"\n";
   }
   for( unsigned i = para[0]+1; i < para[0]+para[3]+1; ++i)
   {
      if ( gate_list[i]->rtn_symbol().size() > 0 )
         outfile<<"o"<<gate_list[i]->rtn_own_id()-para[0]-1
                <<" "<< gate_list[i]->rtn_symbol() << "\n";
   }
   //outfile<<"c\nAAG output by Chung-Yang (Ric) Huang\n";
   /*if( buf_length )
   {
      outfile << "c\n";
      outfile.write(buf,buf_length);;
   }*/
}
void CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
   GateList dfsT_list,PI,AND;
   CirGate::set_gRef();
   dfsT( g, dfsT_list );
   unsigned m = 0;
   for(unsigned t =0, n = dfsT_list.size();t<n; ++t)
   {
      if( dfsT_list[t]->rtn_own_id() > m )m=dfsT_list[t]->rtn_own_id();
      if( dfsT_list[t]->rtn_own_id() == 0 ) continue;
      else if (dfsT_list[t]->getTypeStr().compare("PI") == 0 )
         PI.push_back(dfsT_list[t]);
      else if (dfsT_list[t]->getTypeStr().compare("AIG")== 0 )
         AND.push_back(dfsT_list[t]);
   }
   unsigned n_pi = PI.size(), n_and = AND.size();
   std::sort(PI.begin(),PI.end(),comp_id);
   outfile<<"aag "<<m<<" "<<n_pi<<" 0 1 "<< n_and <<"\n";
   for(unsigned t = 0;t<n_pi;++t)
      outfile << PI[t]->rtn_own_id()*2 << "\n";
   outfile << g->rtn_own_id()*2 << "\n";
   for(unsigned t = 0;t<n_and;++t)
      outfile << AND[t]->rtn_own_id()*2 << " "
              << AND[t]->rtn_riid(0) << " "
              << AND[t]->rtn_riid(1) << "\n";
   outfile << "o0 " << g->rtn_own_id() << "\n";
}
CirMgr::~CirMgr()
{
   colNo = lineNo = 0;
   for(unsigned i = 0; i<gate_list.size(); ++i)
      if( valid_id(i) )
         delete gate_list[i];
   GateList tmp;
   gate_list.swap(tmp);
   if( line_list ) delete [] line_list;
   if( simt ) 
   {
      for(unsigned i = 0; i<b_size; ++i)
      {
         if( simt[i].size() == 0 ) continue;
         for(mnmup IT=(simt[i]._h)->_n;IT!=simt[i]._h;IT=IT->_n)
            delete IT->_d;
      }
      delete [] simt;
   }
}
void CirMgr::dfsT( CirGate* _move ) const
{
   for( unsigned i = 0; i<2; ++i)
   {
      int id = _move->rtn_iid(i);
      if( id != -1&& valid_id(id)&& !(gate_list[id]->is_marked()))
      {
         gate_list[id]->set_mark();
         dfsT(gate_list[id]);
      }
   } 
}
void CirMgr::dfsT( CirGate* _move, GateList& dfsT_list ) const
{
   for( unsigned i = 0; i<2; ++i)
   {
      int id = _move->rtn_iid(i);
      if( id != -1&& valid_id(id)&& !(gate_list[id]->is_marked()))
      {
         gate_list[id]->set_mark();
         dfsT(gate_list[id],dfsT_list);
      }
   } 
   dfsT_list.push_back(_move);
}
void CirMgr::dfsT_for_order(CirGate*& _move,vector<unsigned>& topo )
{
   for( unsigned i = 0; i<2; ++i)
   {
      int id = _move->rtn_iid(i);
      if( id != -1&& valid_id(id)&& !(gate_list[id]->is_marked()))
      {
         gate_list[id]->set_mark();
         dfsT_for_order(gate_list[id],topo);
      }
   } 
   topo[_move->rtn_own_id()] = colNo++;
}
void CirMgr::dfsT_for_AIG( CirGate* _move, GateList& dfsT_list ) const
{
   for( unsigned i = 0; i<2; ++i)
   {
      int id = _move->rtn_iid(i);
      if( id != -1&& valid_id(id)&& !(gate_list[id]->is_marked()))
      {
         gate_list[id]->set_mark();
         dfsT_for_AIG(gate_list[id],dfsT_list);
      }
   } 
   if( _move->getTypeStr().compare("AIG") == 0 )
      dfsT_list.push_back(_move);
}
