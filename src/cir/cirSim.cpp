/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

#include <ctime>
#include <cstdlib>
#include <cctype>

using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/
unsigned num = 0;
bool cstate_comp_rand( unsigned i1, unsigned i2)
{
   return( cirMgr->rtn_gate(i1/2)->rtn_cstate(false,64) <
           cirMgr->rtn_gate(i2/2)->rtn_cstate(false,64) );
}
bool cstate_comp_file(unsigned i1, unsigned i2)
{
   return( cirMgr->rtn_gate(i1/2)->rtn_cstate(true,num) <
           cirMgr->rtn_gate(i2/2)->rtn_cstate(true,num) );
}
/*void inverse(unsigned& num, unsigned long long& s)
{
   for(unsigned short i = 0; i<num; ++i)
      s ^= (one<<i);
   //for(int i = 63;i>=0; --i)
   //   cout << (bool)(s&(one<<i));
}*/
/**************************************/
/*   Static varaibles and functions   */
/**************************************/
/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void CirMgr::randomSim()
{
   num = 64;
   bool rand_or_file = false;
   if( para[4] == 0 ) return;
   if( simt == 0 )
   {
      srand(time(0));
      if( !init_simt(rand_or_file) ) 
         return;
   }
   mySim(rand_or_file);
}
void CirMgr::fileSim(ifstream& patternFile)
{
   bool rand_or_file = true;
   unsigned patt_num = 0;
   int flag = 0;
   patts.resize(para[1]);
   while( flag == 0 )
   {
      flag = parse_patt(patternFile);
      if( flag == -1 ) break;
      if( flag == 0 ) 
      {
         patt_num += 64;
         num = 64;
      }
      else 
      {
         patt_num += flag;
         num = flag;
      }
      if( simt == 0 )
      {
         if( !init_simt(rand_or_file) ) return;
      }
      else 
         mySim(rand_or_file);
   }
   cout << patt_num << " patterns simulated.\n";
}
/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
void CirMgr::logging()
{
   vector<unsigned long long> pi, po;
   for(unsigned i = 2;i<para[1]+2;++i)
      pi.push_back(line_list[i]->state);
   for(unsigned i = para[0]+1;i<=para[0]+para[3];++i)
      po.push_back(gate_list[i]->state);
   for(unsigned k = 0; k<num; ++k)
   {
      for(unsigned i = 0; i<para[1]; ++i)
         (*_simLog) << (bool)((one<<k)&pi[i]);
      (*_simLog) << ' ';
      for(unsigned i = 1; i<=para[3]; ++i)
         (*_simLog) << (bool)((one<<k)&po[i-1]);
      (*_simLog) << '\n';
   }
}
void CirMgr::dfsT_for_SIM(CirGate*& _move, bool& rand_or_file)
{
   if( _move->is_marked() ) return;
   for( unsigned i = 0; i<2; ++i)
   {
      int id = _move->rtn_iid(i);
      if( id != -1&& valid_id(id)&& !(gate_list[id]->is_marked()))
         dfsT_for_SIM(gate_list[id],rand_or_file);
   }   
   _move->set_mark();
   int i0 = _move->rtn_iid(0), i1 = _move->rtn_iid(1);
   if( i0 == -1 ) 
   {
      if( _move->rtn_own_id() == 0 ) return;
      if( !rand_or_file )
      {
         unsigned long long s = rand();
         s <<= 32;
         s += rand();
         return _move->set_state(s,num);
      }
      else
         _move->set_state(patts[_move->getLineNo()-2],num);
   }
   else if (i1==-1&&_move->getTypeStr().compare("PO")==0&&valid_id(i0))
      return _move->set_state(gate_list[i0]->state,num);
   else if ( valid_id(i0) && valid_id(i1) )
      return _move->set_state(gate_list[i0]->state,gate_list[i1]->state,num);
}
void CirMgr::dfsT_for_sim (CirGate*& _move ,bool& rand_or_file)
{
   for( unsigned i = 0; i<2; ++i)
   {
      int id = _move->rtn_iid(i);
      if( id != -1&& valid_id(id)&& !(gate_list[id]->is_marked()))
      {
         gate_list[id]->set_mark();
         dfsT_for_sim(gate_list[id],rand_or_file);
      }
   }   
   int i0 = _move->rtn_iid(0), i1 = _move->rtn_iid(1);
   if( i0 == -1 )
   {
      if( _move->rtn_own_id() == 0 ) return;
      else if( !rand_or_file )
      {
         unsigned long long s = rand();
         s <<= 32;
         s += rand();
         return _move->set_state(s,num);
      }
      else
         return _move->set_state(patts[_move->getLineNo()-2],num);
   }
   else if (i1==-1&&_move->getTypeStr().compare("PO")==0&&valid_id(i0))
      return _move->set_state(gate_list[i0]->state,num);
   else if ( valid_id(i0) && valid_id(i1) )
      _move->set_state(gate_list[i0]->state,gate_list[i1]->state,num);
   else return;
   bool inv = false;//, done = false;
   unsigned long long s = _move->state;
   if(!rand_or_file&&s>((unsigned long long)(-1)/2))
   { 
      s = ~s;
      inv = true;
   }
   else if ( rand_or_file&&((s&one)==1) )
   {
      inverse(num,s);
      inv = true;
   }
   unsigned key=s%b_size,id=_move->rtn_own_id()*2+(inv?1:0);
   vp[key].push_back(id);
}
bool CirMgr::init_simt(bool& rand_or_file)
{
   unsigned temp = para[4];
   b_size = 1;
   while( temp > 0 )
   {  
      temp >>= 1;
      b_size <<= 1;
   }
   simt = new mmu[b_size];
   CirGate::set_gRef();
   addr.resize(para[0]+1);
   vp = new vector<unsigned>[b_size];
   vp[0].push_back(0);
   for( unsigned i = 1+para[0] ; i <= para[3]+para[0] ; ++i )
      dfsT_for_sim( gate_list[i], rand_or_file );
   for(unsigned i = 0; i<b_size; ++i)
   {
      if( vp[i].size() < 2 ) continue;
      if( rand_or_file )
         std::sort(vp[i].begin(),vp[i].end(),cstate_comp_file);
      else 
         std::sort(vp[i].begin(),vp[i].end(),cstate_comp_rand);
      for(unsigned j = 0, n = vp[i].size(); j<n-1;)
      {
         unsigned m = 0;
         if(gate_list[vp[i][j]/2]->rtn_cstate(rand_or_file,num)==
            gate_list[vp[i][j+1]/2]->rtn_cstate(rand_or_file,num) )
         {
            do
            {
               ++m; ++j;
               if( j == n-1 ) break;
            } 
            while
            (gate_list[vp[i][j]/2]->rtn_cstate(rand_or_file,num)==
             gate_list[vp[i][j+1]/2]->rtn_cstate(rand_or_file,num) );
         }
         if( m > 0 )
         {
            simt[i].push_front( new mu );
            for(unsigned k = 0; k<=m; ++k)
               (simt[i]._h)->_n->_d->push_front(vp[i][j-k]);
         }
         ++j;
      }
   }
   if( renew_simt() == 0 ) { delete [] simt; simt = 0; return false; }
   if( _simLog ) logging();
   delete [] vp;
   return true;
}
void CirMgr::mySim(bool& rand_or_file)
{
   unsigned patt_num = 0;
   unsigned fail_limit = 1;
   if( !rand_or_file )
      fail_limit = cal_fail_limit(b_size);
   unsigned fail_times = 0;
   while( fail_times < fail_limit )
   {
      bool changed = false;
      CirGate::set_gRef();
      if( _simLog == 0 )
      {
         for(unsigned i = 0; i<b_size; ++i)
         {
            if( simt[i].size() == 0 ) continue;
            for(mnmup j = (simt[i]._h)->_n; j!=simt[i]._h; j=j->_n)
               for(mnup k=j->_d->_h->_n;k!=j->_d->_h;k=k->_n)
                  dfsT_for_SIM( gate_list[k->_d/2] , rand_or_file);
         }
      }
      else
      {
         for( unsigned i = 1+para[0] ; i <= para[3]+para[0] ; ++i )
            dfsT_for_SIM( gate_list[i] ,rand_or_file);
         logging();
      }
      patt_num+=64;
      for(unsigned i = 0; i<b_size; ++i)
      {
         if( simt[i].size() == 0 ) continue;
         for(mnmup It=simt[i]._h,IT=It->_n;IT!=simt[i]._h;)
         {
            bool added = false;
            unsigned long long first_s=gate_list[IT->_d->_h->_n->_d/2]->rtn_cstate(rand_or_file,num);
            for(mnup it=IT->_d->_h,iT=it->_n;iT!=IT->_d->_h;)
            {
               unsigned long long s = gate_list[iT->_d/2]->rtn_cstate(rand_or_file,num);
               if( s != first_s )
               {
                  changed = true;
                  bool inv = false, done = false;
                  if(!rand_or_file&&s>((unsigned long long)(-1)/2))
                  { 
                     s = ~s;
                     inv = true;
                  }
                  else if ( rand_or_file&&((s&one)==1) )
                  {
                     inverse(num,s);
                     inv = true;
                  }
                  unsigned key = s%b_size;
                  unsigned id = (iT->_d/2)*2+(inv?1:0);
                  if( simt[key].size() > 0 )
                  {
                     for(mnmup it2=(simt[key]._h)->_n;it2!=simt[key]._h;it2=it2->_n)
                     {
                        if(addr[it2->_d->_h->_n->_d/2]==addr[iT->_d/2] &&
                        gate_list[it2->_d->_h->_n->_d/2]->rtn_cstate(rand_or_file,num)==s)
                        {
                           it2->_d->push_front(id);
                           done = true; break;
                        }
                     }
                  }
                  if( !done )
                  {
                     simt[key].push_front( new mu );
                     (simt[key]._h)->_n->_d->push_front(id);
                  }
                  mnup tmp = iT;
                  iT = iT->_n;
                  IT->_d->erase(tmp,it);
               }
               else { it=it->_n;iT=iT->_n; }
            }
            if( !added ) { It=It->_n;IT=IT->_n; }
         }
      }
      if( !changed ) { ++fail_times; continue; }
      unsigned pn = renew_simt();
      if( pn == 0 ) { delete [] simt; break; }
      if( pn*30 < b_size ) 
      {
         refresh_simt(pn);
         fail_limit *= 2;
         //fail_limit /= 2;
      }
      if( rand_or_file ) break;
   }
   if( !rand_or_file )
      cout << "MAX_FAILS = " << fail_limit << "\n"
           << patt_num << " patterns simulated.\n";
}
unsigned CirMgr::renew_simt()
{
   unsigned pn = 0;
   for(unsigned i = 0; i<b_size; ++i)
   {
      if( simt[i].size() == 0 ) continue;
      for(mnmup It=simt[i]._h,IT=It->_n;IT!=simt[i]._h;)
      {
         unsigned m = IT->_d->size();
         if( m == 1 ) 
         {
            delete IT->_d;
            simt[i].erase(IT,It);
            IT = It->_n;
         }
         else if ( m > 1 ) 
         {
            for(mnup it=IT->_d->_h->_n;it!=IT->_d->_h;it=it->_n)
               addr[it->_d/2] = IT->_d->_h->_n->_d;
            ++pn; IT=IT->_n; It=It->_n;
         }
      }
   }
   return pn;
}
void CirMgr::refresh_simt(unsigned s)
{
   mmu* temp_simt = new mmu[s*10];
   for(unsigned i = 0, p = 0; i<b_size; ++i)
   {
      if( simt[i].size() > 0 )
         for(mnmup IT=(simt[i]._h)->_n;IT!=simt[i]._h;IT=IT->_n)
            temp_simt[p++].push_front( IT->_d );
   }
   b_size = s*10;
   mmu* temp_rtn = simt;
   simt = temp_simt;
   delete [] temp_rtn;
}
unsigned CirMgr::cal_fail_limit(unsigned i)
{
   if( i < 64 ) return (i/16+1);
   else if ( i<   160) return (i/29);
   else if ( i<   400) return (i/52);
   else if ( i<  1000) return (i/93);
   else if ( i<  2500) return (i/168);
   else if ( i<  6250) return (i/302);
   else if ( i< 15625) return (i/544);
   else if ( i< 39063) return (i/980);
   else if ( i< 97656) return (i/1763);
   else if ( i<244141) return (i/3174);
   else if ( i<610352) return (i/5713);
   else return 110;
}
int CirMgr::parse_patt(ifstream& IF)
{
   for(unsigned i = 0; i<para[1]; ++i)
      patts[i] = 0;
   for(int i = 0;i<64; ++i)
   {
      if( isspace(IF.peek()) )
      {
         do
         {
            IF.get();
         } while (isspace(IF.peek()) );
      }
      for(int j = 0; j<(int)para[1]; ++j)
      {
         short x = IF.get();
         if( x/2 != 24 )
         {
            cout << "\nError: Pattern(";
            for( ; j>=0 ; --j )
               IF.unget();
            for( j = 0 ; j < (int)para[1]; ++j)
               cout << (char)IF.get();
            cout << ") contains a no-0/1 character('" 
                 << (char)x << "').\n";
            return -1;
         }
         else if ( x == 49 )
            patts[j] += (one<<i);
      }
      if( isspace(IF.peek()) )
      {
         do
         {
            IF.get();
         } while (isspace(IF.peek()) );
      }
      else 
      {
         unsigned j = 0;
         cout << "Error: Pattern(";
         for( ; j<para[1] ; ++j )
            IF.unget();
         for( j=0; IF.peek()/2 == 24 ;++j)
            cout << (char)IF.get();
         cout << ") length(" << j << ") does not match the number of inputs(" << para[1] << ") in a circuit!!\n";
         return -1;
      }
      if( IF.peek() == EOF ) return i+1;
   }
   return 0;
}
