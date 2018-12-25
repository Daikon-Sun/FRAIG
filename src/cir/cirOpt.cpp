/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void CirMgr::sweep()
{
   CirGate::set_gRef();
   for( unsigned i = 1+para[0] ; i <= para[0]+para[3] ; ++i ) 
      dfsT( gate_list[i] ) ;
   for( unsigned i = 1, n = gate_list.size()-para[3]; i<n; ++i)
   {
      if( valid_id(i) && !gate_list[i]->is_marked() &&
          gate_list[i]->getTypeStr().compare("AIG") == 0 )
      {
         cout << "Sweeping: AIG("
              << gate_list[i]->rtn_own_id() << ")"
              << " removed..." << endl;
         --para[4];
         for(unsigned j = 0; j<2; ++j)
         {
            unsigned id = gate_list[i]->rtn_iid(j); //child id
            if( valid_id(id) )
            {
               for(unsigned k = 0, n = gate_list[id]->oid_size(); k<n; ++k)
                  if( gate_list[id]->rtn_oid(k) == gate_list[i]->rtn_own_id())
                  {
                     gate_list[id]->set_oid(k);
                     break;
                  }
            }
         }
         delete gate_list[i];
         gate_list[i] = 0;
      }
      else if ( !valid_id(i) && (size_t)gate_list[i] &&   
                !gate_list[(size_t)gate_list[i]]->is_marked() ) 
         cout << "Sweeping: UNDEF(" << i << ") removed..." << endl;
   }
}
// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
   CirGate::set_gRef();
   for( unsigned i = 1+para[0] ; i <= para[3]+para[0] ; ++i )
      dfsT_for_Opt( gate_list[i] );
}
void CirMgr::dfsT_for_Opt( CirGate*& _move )
{
   for( unsigned i = 0; i<2; ++i)
   {
      int id = _move->rtn_iid(i);
      if( id != -1&& valid_id(id)&& !(gate_list[id]->is_marked()))
      {
         gate_list[id]->set_mark();
         dfsT_for_Opt(gate_list[id]);
      }
   } 
   if( _move->has_zero() ) 
      opp_or_zero(true, _move, (_move->rtn_iid(1) == 0));
   else if ( _move->is_opp() )
   {
      if(valid_id(_move->rtn_iid(0))&&valid_id(_move->rtn_iid(1)))
         opp_or_zero(false, _move,false); 
   }
   else if( _move->is_same() )
   {
      if(valid_id(_move->rtn_iid(0))&&valid_id(_move->rtn_iid(1)))
         same_or_one(false,_move, false);
   }
   else if ( _move->has_one() )
      same_or_one(true, _move, (_move->rtn_iid(1) == 0));
}
/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
void CirMgr::same_or_one(bool b, CirGate*& _move, bool p)
{
   bool inv = false;
   unsigned own_id = _move->rtn_own_id();
   unsigned cid = _move->rtn_iid(!p);
   if( _move->is_inverted(!p) ) inv = true;
   cout << "Simplifying: " << cid << " merging " 
        << (inv?"!":"") << own_id << "...\n";
   gate_list[cid]->set_oid(rdirec(_move,gate_list[cid]));
   gate_list[b?0:cid]->set_oid(rdirec(_move,gate_list[b?0:cid]));
   for(unsigned i = 0, m=_move->oid_size(); i<m; ++i)
   {
      CirGate* par = gate_list[_move->rtn_oid(i)];
      unsigned pos = ldirec(par,_move);
      par->set_iid(pos,(par->is_inverted(pos)^inv)?(cid*2+1):(cid*2));
      gate_list[cid]->add_id(par->rtn_own_id());
   }
   delete gate_list[own_id];
   gate_list[own_id] = 0;
   --para[4];
}
void CirMgr::opp_or_zero(bool b, CirGate*& _move, bool p)
{
   unsigned own_id = _move->rtn_own_id();
   unsigned cid = _move->rtn_iid(!p);
   cout << "Simplifying: 0 merging " << own_id << "...\n";
   gate_list[cid]->set_oid(rdirec(_move, gate_list[cid]));
   gate_list[b?0:cid]->set_oid(rdirec(_move, gate_list[b?0:cid]));
   for(unsigned i = 0, m=_move->oid_size(); i<m; ++i)
   {
      CirGate* par = gate_list[_move->rtn_oid(i)];
      unsigned pos = ldirec(par,_move);
      par->set_iid(pos,par->is_inverted(pos)?(1):(0));
      gate_list[0]->add_id(par->rtn_own_id());
   }
   delete gate_list[own_id];
   gate_list[own_id] = 0;
   --para[4];
}
unsigned CirMgr::rdirec(CirGate*& parent, CirGate*& child) const
{
   for(unsigned i = 0, n = child->oid_size(); i<n; ++i)
      if( (size_t)child->rtn_oid(i) == parent->rtn_own_id() )
         return i;
   return 0;
}
unsigned CirMgr::ldirec(CirGate*& parent, CirGate*& child) const
{
   if( (size_t)parent->rtn_iid(0) == child->rtn_own_id() )
      return 0;
   else 
      return 1;
}
