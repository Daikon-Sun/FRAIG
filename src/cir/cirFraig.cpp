/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/
#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"
#include <algorithm>

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions
//HashMap
/*******************************/
/*   Global variable and enum  */
/*******************************/
unsigned max_n = 6;
const unsigned limit_to_sim = 4;
bool cstate_comp(unsigned i0, unsigned i1)
{
      return
      (cirMgr->rtn_gate(i0/2)->rtn_cstate(max_n,((i0/2)*2==i0))<
      cirMgr->rtn_gate(i1/2)->rtn_cstate(max_n,((i1/2)*2==i1)));
}
bool topo_comp(unsigned i0, unsigned i1)
{
   return (cirMgr->rtn_topo(i0) < cirMgr->rtn_topo(i1));
}
/**************************************/
/*   Static varaibles and functions   */
/**************************************/
/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
//unsigned CacheKey::aig_num = 0;
void CirMgr::strash()
{
   CirGate::set_gRef();
   HashMap<HashKey,unsigned> hash(para[0]+1);
   for( unsigned i = 1+para[0] ; i <= para[3]+para[0] ; ++i )
      dfsT_for_strash( gate_list[i], hash );
}
void CirMgr::fraig()
{
   if( simt == 0 ) return;
   SatSolver solver;
   solver.initialize();
   for(unsigned i = 0; i<=para[0];++i)
   {
      if( valid_id(i) )
         gate_list[i]->set_var(solver.newVar());
   }
   parseModel(solver);
   for(unsigned i = 0; i<b_size; ++i)
   {
      if( simt[i].size() == 0 ) continue;
      else
      {
         assert( simt[i]._h->_n!=simt[i]._h);
         for(mnmup j=simt[i]._h,J=j->_n;J!=simt[i]._h;)
         {
            int n = J->_d->size();
            vector<unsigned> group(n,0);
            assert( J->_d->size() != 0 );
            for(mnup k=J->_d->_h->_n;k!=J->_d->_h;k=k->_n)
            {  
               group[n-1] = k->_d;
               --n;
            }
            delete J->_d;
            simt[i].erase(J,j);
            J = j->_n;
            solveCNF(solver,group);
         }
         i = 0;
      }
   }
   if( simt ) 
   {
      for(unsigned i = 0; i<b_size; ++i)
      {
         if( simt[i].size() == 0 ) continue;
         for(mnmup IT=(simt[i]._h)->_n;IT!=simt[i]._h;IT=IT->_n)
            delete IT->_d;
      }
      delete [] simt;
      simt = 0;
   }
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
void CirMgr::solveCNF(SatSolver& solver,vector<unsigned>& group)
{
   unsigned n = group.size();
   bool need_sim = (n>limit_to_sim);
   vector<unsigned long long> patts;
   GateList PIs;
   if( need_sim )
   {
      CirGate::set_gRef();
      for(unsigned l = 0; l<n; ++l)
         dfsT_for_PI(gate_list[group[l]/2],PIs);
      patts.resize(para[1],0);
   }
   unsigned num_patt=0;
   for(unsigned l = 0; l<n; ++l)
   {
      unsigned x = group[l];
      for(unsigned p = l+1; p<n;)
      {
         unsigned y = group[p];
         Var test = solver.newVar();
         solver.addXorCNF(test,
                         gate_list[x/2]->rtn_var(),((x/2)*2!=x),
                         gate_list[y/2]->rtn_var(),((y/2)*2!=y));
         solver.assumeRelease();
         solver.assumeProperty(gate_list[0]->rtn_var(),false);
         solver.assumeProperty(test,true);
         bool result = solver.assumpSolve();
         if( !result )
         {
            bool t = (topo[x/2]>topo[y/2]);
            if( t ) { unsigned tmp = x;x=y;y=tmp; }
            bool b = (((x/2)*2==x)^((y/2)*2==y));
            cout << "Fraig: " << x/2 << " merging " 
                 << (b?"!":"")<< y/2 << "...\n";
            --para[4];
            merge(gate_list[x/2],gate_list[y/2],b);
            if( !t )
               group.erase(group.begin()+p);
            else 
            {
               group.erase(group.begin()+l);
               x = group[l];
               p = l+1;
            }
            --n;
         }
         else
         {
            if( need_sim )
            {
               for(unsigned t = 0,r=PIs.size();t<r;++t)
               {
                  unsigned short dat=solver.getValue(line_list[PIs[t]->getLineNo()]->rtn_var());
                  if( dat )
                    patts[PIs[t]->getLineNo()-2]+=(dat<<num_patt);
               }
               ++num_patt;
               if( num_patt == max_n )
               {
                  CirGate::set_gRef();
                  for(unsigned u = 0; u<n; ++u)
                     dfsT_for_fraig(gate_list[group[u]/2],patts);
                  std::sort(group.begin(),group.end(),cstate_comp);
                  for(unsigned j = 0; j<n-1;)
                  {
                     unsigned m = 0;
                     if(gate_list[group[j]/2]->rtn_cstate(max_n,((group[j]/2)*2==group[j]))==
                        gate_list[group[j+1]/2]->rtn_cstate(max_n,((group[j+1]/2)*2==group[j+1])))
                     {
                        do
                        {
                           ++m; ++j;
                           if( j == n-1 ) break;
                        } 
                        while
                        (gate_list[group[j]/2]->rtn_cstate(max_n,((group[j]/2)*2==group[j]))==
                         gate_list[group[j+1]/2]->rtn_cstate(max_n,((group[j]/2)*2==group[j+1])));
                     }
                     if( m > 0 )
                     {
                        bool done = false;
                        for(unsigned z = 0; z<b_size; ++z)
                        {
                           if(simt[z].size() == 0)
                           {
                              done = true;
                              simt[z].push_front(new mu);
                              for(unsigned k = 0; k<=m; ++k)
                                 simt[z]._h->_n->_d->push_front(group[j-k]);
                              break;
                           }
                        }
                        assert( done == true );
                        if( !done )
                        {
                           simt[b_size/2].push_front(new mu);
                           for(unsigned k = 0; k<=m; ++k)
                              simt[b_size/2]._h->_n->_d->push_front(group[j-k]);
                        }
                     }
                     ++j;
                  }
                  return;
               }
            }
            ++p;
         }
      }
   }
}
void CirMgr::parseModel(SatSolver& s)
{
   GateList gl;
   CirGate::set_gRef();
   for(unsigned i = para[0]+1;i<=para[0]+para[3];++i)
      dfsT_for_AIG(gate_list[i],gl);
   for(unsigned i = 0, n = gl.size();i<n;++i)
   {
      s.addAigCNF(gl[i]->rtn_var(),
                  gate_list[gl[i]->rtn_iid(0)]->rtn_var(),
                  gl[i]->is_inverted(0),
                  gate_list[gl[i]->rtn_iid(1)]->rtn_var(),
                  gl[i]->is_inverted(1));
   }
}
void CirMgr::merge(CirGate*& stay, CirGate*& leave,bool& b)
{
   for(unsigned i = 0,n=leave->oid_size(); i<n; ++i)
   {
      unsigned p_id = leave->rtn_oid(i);
      stay->add_id(p_id);
      bool d = ldirec(gate_list[p_id],leave);
      gate_list[p_id]->set_iid(d,stay->rtn_own_id()*2+(gate_list[p_id]->is_inverted(d)^b?1:0));
   }
   unsigned i0 = leave->rtn_iid(0), i1 = leave->rtn_iid(1);
   if( valid_id(i0) )
      gate_list[i0]->set_oid(rdirec(leave,gate_list[i0]));
   if( valid_id(i1) )
      gate_list[i1]->set_oid(rdirec(leave,gate_list[i1]));
   delete leave;
   leave = 0;
}
void CirMgr::dfsT_for_PI(CirGate*& _move, GateList& dfsT_list)
{
   for( unsigned i = 0; i<2; ++i)
   {
      int id = _move->rtn_iid(i);
      if( id != -1&& valid_id(id)&& !(gate_list[id]->is_marked()))
      {
         gate_list[id]->set_mark();
         dfsT_for_PI(gate_list[id],dfsT_list);
      }
   } 
   if( _move->getTypeStr().compare("PI") == 0 )
      dfsT_list.push_back(_move);
}
void CirMgr::dfsT_for_fraig(CirGate*& _move, vector<unsigned long long>& patts)
{
   if( _move->is_marked() ) return;
   for( unsigned i = 0; i<2; ++i)
   {
      int id = _move->rtn_iid(i);
      if( id != -1&& valid_id(id)&& !(gate_list[id]->is_marked()))
         dfsT_for_fraig(gate_list[id],patts);
   }   
   _move->set_mark();
   int i0 = _move->rtn_iid(0), i1 = _move->rtn_iid(1);
   if( _move->rtn_own_id() == 0 ) return;
   else if( i0 == -1 ) 
      _move->set_state(patts[_move->getLineNo()-2],max_n);
   else if (i1==-1&&_move->getTypeStr().compare("PO")==0&&valid_id(i0))
      return _move->set_state(gate_list[i0]->state,max_n);
   else if ( valid_id(i0) && valid_id(i1) )
      return _move->set_state(gate_list[i0]->state,gate_list[i1]->state,max_n);
}
void 
CirMgr::dfsT_for_strash(CirGate* _move,HashMap<HashKey,unsigned>& hash)
{
   for( unsigned i = 0; i<2; ++i)
   {
      int id = _move->rtn_iid(i);
      if( id != -1&& valid_id(id)&& !(gate_list[id]->is_marked()))
      {
         gate_list[id]->set_mark();
         dfsT_for_strash(gate_list[id], hash);
      }
   }   
   int i0 = _move->rtn_riid(0), i1 = _move->rtn_riid(1);
   if( i0 == -1 || i1 == -1 ) return;
   if( i0 > i1 )
   {
      i0 = i1;
      i1 = _move->rtn_riid(0);
   }
   unsigned o_id = _move->rtn_own_id();
   unsigned gid = o_id;
   if(!hash.insert(HashKey(i0,i1),gid))
   {
      cout << "Strashing: " << gid << " merging " << o_id << "...\n";
      --para[4];
      for(unsigned i = 0, n = _move->oid_size(); i<n; ++i)
      {
         unsigned p_id = _move->rtn_oid(i);
         gate_list[gid]->add_id(p_id);
         CirGate* p = gate_list[p_id];
         bool d = ldirec(p,gate_list[o_id]);
         p->set_iid(d,gid*2+(p->is_inverted(d)?1:0));
      }
      if( valid_id(i0/2) )
         gate_list[i0/2]->set_oid(rdirec(_move,gate_list[i0/2]));
      else if ( gate_list[i0/2] )
         gate_list[i0/2] = (CirGate*)(size_t)gid;
      if( valid_id(i1/2) )
         gate_list[i1/2]->set_oid(rdirec(_move,gate_list[i1/2]));
      else if ( gate_list[i1/2] )
         gate_list[i1/2] = (CirGate*)(size_t)gid;
      delete gate_list[o_id];
      gate_list[o_id] = 0;
   }
}
