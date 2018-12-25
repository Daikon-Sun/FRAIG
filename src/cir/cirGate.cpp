/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

extern CirMgr *cirMgr;

void inverse(unsigned& num, unsigned long long& s)
{
   for(unsigned short i = 0; i<num; ++i)
      s ^= (one<<i);
}

string INTtoSTRING(unsigned i)
{
   string str = "";
   do
   {
      str.insert(0, 1,(char)(i%10+48));
      i = (i-i%10)/10;
   } 
   while(i>0);
   return str;
}

string CirGate::indent(int t) const
{
   string str = "";
   for(int i = 0; i<t; ++i)
      str += "  ";
   return str;
}

unsigned CirGate::gRef = 0;
unsigned CirGate::top_level = 0;

void And::printGate() const
{
   cout << "AIG " << own_id << " " 
        <<(cirMgr->valid_id(iid[0]/2)?"":"*")
        <<(is_inverted(0)?"!":"")<<iid[0]/2<<" " 
        <<(cirMgr->valid_id(iid[1]/2)?"":"*")
        <<(is_inverted(1)?"!":"")<<iid[1]/2;
}
bool And::check_float_1 () const 
{
   return !( cirMgr->valid_id(iid[0]/2) &&
             cirMgr->valid_id(iid[1]/2) );
}
void PO::printGate() const
{
   cout << "PO  " << own_id << " " 
        << (is_inverted(0)?"!":"") 
        << (cirMgr->valid_id(iid/2)?"":"*") << iid/2;
   if( symbol.size() != 0 ) cout << " (" << symbol << ")";
}
bool PO::check_float_1 () const
{
   return !cirMgr->valid_id(iid/2);
}
/**************************************/
/*   class CirGate member functions   */
/**************************************/
void CirGate::reportFanin(int level) const
{
   assert (level >= 0);
   CirGate::set_gRef();
   top_level = level;
   coutGate(level, top_level, false);
   my_reportFanin(level);
   cout << endl;
}
void CirGate::my_reportFanin(int level) const
{
   for(unsigned i = 0 ; i<2; ++i)
   {
      int id = rtn_iid(i);
      if( id == -1 ) continue;
      if( cirMgr->valid_id(id) )
      {
         bool done = false;
         bool inverse = (is_inverted(i) ? true : false );
         CirGate* child = cirMgr->rtn_gate(id);
         cout << endl;
         child->coutGate(level-1, top_level, inverse);
         if( child->is_marked() && child->rtn_iid(0) != -1 )
         {
            done = true;
            cout << " (*)";
         }
         child->set_mark();
         if( level && !done ) 
            child->my_reportFanin(level-1);
      }
      else if ( id > 0 )
         cout << endl << indent(top_level-level+1) <<  "UNDEF " << id;
   } 
}
void CirGate::reportFanout(int level) const
{
   assert (level >= 0);
   CirGate::set_gRef();
   top_level = level;
   coutGate(level, top_level, false);
   cout << endl;
   if(level) my_reportFanout(level);
}
void CirGate::my_reportFanout(int level) const
{
   for(unsigned i = 0, n = oid_size(); i < n; ++i )
   {
      int id = rtn_oid(i);
      bool inverse = false;
      CirGate* parent = cirMgr->rtn_gate(id);
      if((parent->rtn_iid(0)==(int)rtn_own_id() && parent->is_inverted(0))||
         (parent->rtn_iid(1)==(int)rtn_own_id() && parent->is_inverted(1)) ) 
         inverse = true;
      parent->coutGate(level-1, top_level, inverse);
      if( parent->is_marked() && level>1 && 
          parent->oid_size() > 0 )
         cout << " (*)" << endl;
      else
      {
         cout << endl;
         if( level>1 )
         {
            parent->set_mark();
            parent->my_reportFanout(level-1);
         }
      }
   }
}
      


   



