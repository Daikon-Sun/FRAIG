/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/
#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <iomanip>
#include <string>
#include <vector>
#include <iostream>
#include "cirDef.h"
#include "sat.h"

using namespace std;

string INTtoSTRING(unsigned);

const unsigned long long one = 1;

void inverse(unsigned&,unsigned long long&);

class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
// TODO: Define your own data members and member functions, or classes
class CirGate
{
   public:
   CirGate(unsigned i, unsigned ln) : 
      state(0), own_id(i), line_num(ln),var(0), lRef(0) {}

   virtual ~CirGate() {}

   virtual void printGate() const = 0;
   virtual void reportGate()const = 0;
   virtual void coutGate(int, int, bool) const = 0;
   virtual string getTypeStr() const = 0;
   
   virtual int rtn_iid(unsigned) const { return -1; }
   virtual int rtn_riid(unsigned) const { return -1; }
   virtual unsigned rtn_oid(unsigned) const { return -1; }
   virtual void set_oid(unsigned) {};
   virtual void set_iid(unsigned, unsigned) {};
   virtual void add_id(unsigned) {};
   virtual unsigned oid_size() const { return 0; }
   virtual bool is_inverted(unsigned) const = 0;

   virtual bool check_float_1() const { return false; }
   virtual bool check_float_2() const { return false; }
   
   virtual bool has_zero() const { return false; }
   virtual bool has_one() const { return false; }
   virtual bool is_same() const { return false; }
   virtual bool is_opp() const { return false; }

   string indent(int) const;
   void bool_state() const
   {
      cout << "= Value: ";
      for(int k = 31; k>=0; --k)
         cout << (bool)((1<<k)&state) << ((k%4==0&&k)?"_":"");
      cout << " =\n";
   }
   
   virtual void set_symbol(string) {}
   virtual string rtn_symbol () const { return ""; }
   
   bool is_marked() { return (lRef == gRef); }
   void set_mark() { lRef = gRef; }

   unsigned rtn_own_id() const { return own_id; }
   unsigned getLineNo() const { return line_num;}

   virtual void set_state(unsigned long long& i,unsigned& num) 
   { state = i; }
   virtual void set_state(unsigned long long&,unsigned long long&,unsigned&) {}

   unsigned long long rtn_state() const { return state; }
   unsigned long long rtn_cstate(bool b,unsigned num) const
   {
      if( !b ) 
         return (state>((unsigned long long)(-1)/2)?~state:state);
      if( (state&one)==0 ) return state;
      if( num == 64 ) return ~state;
      unsigned long long s = state;
      for(unsigned short i = 0; i<num; ++i)
         s ^= (one<<i);
      return s;
   }
   unsigned long long rtn_cstate(unsigned& num,bool b) const
   {
      if(b) return state;
      else
      {
         unsigned long long s = state;
         for(unsigned short i = 0; i<num;++i)
            s ^= (one<<i);
         return s;
      }
   }

   void set_var(Var v) {var = v;}
   Var rtn_var() { return var;}

   static void set_gRef() { ++gRef; }
   static unsigned top_level;

   virtual bool isAig() const { return false; }

   void reportFanin(int) const;
   void my_reportFanin(int) const;
   void reportFanout(int) const;
   void my_reportFanout(int) const;

   unsigned long long state;
   protected:
   unsigned own_id;
   unsigned line_num;
   //unsigned long long state;
   Var var;

   private:
   static unsigned gRef;
   unsigned lRef;
};

class And : public CirGate
{
   public:
   And(unsigned own_id, int i1, int i2, unsigned ln)
      : CirGate(own_id, ln)
   { iid[0] = i1; iid[1] = i2; }
   ~And() {}

   void printGate() const;
   void reportGate() const
   {
      string tmp = "= AIG(" + INTtoSTRING(own_id) +")";
      tmp += ", line " + INTtoSTRING(getLineNo());
      cout << "==================================================" << endl
           << setw(49) << left << tmp << "=" << endl;
      bool_state();
      cout << "==================================================" << endl;
   }
   void coutGate(int level, int top_level, bool inv) const
   {
      cout << indent(top_level-level) 
           << (inv?"!":"") << "AIG " << own_id;
   }
   string getTypeStr() const { return "AIG"; }

   int rtn_iid(unsigned s) const { return iid[s]/2; }
   int rtn_riid(unsigned s) const { return iid[s]; }
   unsigned rtn_oid(unsigned s) const { return oid[s]; }
   void set_oid(unsigned i) { oid.erase(oid.begin()+i); }
   void set_iid(unsigned i, unsigned n) { iid[i] = n; }
   void add_id(unsigned i) { oid.push_back(i); }
   unsigned oid_size() const { return oid.size(); }
   bool is_inverted(unsigned i) const{ return (iid[i]/2)*2 != iid[i]; }

   bool check_float_1 () const;
   bool check_float_2 () const { return ( oid.size() == 0); }

   void set_state(unsigned long long& i, unsigned long long& j,unsigned& num)
   {
      if( num == 64 )
      state = (((iid[0]/2)*2 != iid[0])?(~i):(i))&
              (((iid[1]/2)*2 != iid[1])?(~j):(j));
      else
      {
         unsigned long long I = i, J=j;
         if( (iid[0]/2)*2 != iid[0] )
            inverse(num,I); 
         if( (iid[1]/2)*2 != iid[1] )
            inverse(num,J); 
         state = I&J;
      }
   }

   bool has_zero() const { return (iid[0]==0||iid[1]==0); }
   bool has_one() const { return (iid[0]==1||iid[1]==1); }
   bool is_same() const { return (iid[0] == iid[1]); }
   bool is_opp() const {return (iid[0]/2==iid[1]/2&&iid[0]!= iid[1]); }

   virtual bool isAig() const { return true; }

   private:
   unsigned iid[2];
   vector<unsigned> oid;
};

class PI : public CirGate
{
   public:
   PI(unsigned id, unsigned ln) : CirGate(id, ln),symbol("") {}
   ~PI() {}

   void printGate() const
   {
      if( own_id ) cout << "PI  " << own_id;
      else cout << "CONST0";
      if( symbol.size() != 0 ) cout << " (" << symbol << ")";
   }
   void reportGate() const
   {
      string tmp = (own_id?"= PI(":"= CONST(") + INTtoSTRING(own_id) +")";
      if( symbol.size() != 0 ) tmp += ("\"" + symbol + "\"");
      tmp += ", line " + INTtoSTRING(getLineNo());
      cout << "==================================================" << endl
           << setw(49) << left << tmp << "=" << endl;
      bool_state();
      cout << "==================================================" << endl;
   }
   void coutGate(int level, int top_level, bool inv) const
   {
      cout << indent(top_level-level) << (inv?"!":"");
      if( own_id ) cout<< "PI " << own_id;
      else cout << "CONST 0";
   }
   string getTypeStr() const { return (own_id?"PI":"CONST0"); }

   unsigned rtn_oid(unsigned s) const { return oid[s]; }
   void set_oid(unsigned i) { oid.erase(oid.begin()+i); }
   void add_id(unsigned i) { oid.push_back(i); }
   unsigned oid_size() const { return oid.size(); }
   bool is_inverted(unsigned i ) const { return false; }

   bool check_float_2 () const { return ( oid.empty() ); }
   
   void set_symbol(string str) { symbol = str; }
   string rtn_symbol() const { return symbol; }

   
   private:
   vector<unsigned> oid;
   string symbol;
};

class PO : public CirGate
{
   public:
   PO(unsigned own_id,unsigned i,unsigned ln):CirGate(own_id,ln),symbol("") 
   { iid = i; }
   ~PO() {}
   void printGate() const;
   void reportGate() const
   {
      string tmp = "= PO(" + INTtoSTRING(own_id) +")";
      if( symbol.size() != 0 ) tmp += ("\"" + symbol + "\"");
      tmp += ", line " + INTtoSTRING(getLineNo());
      cout << "==================================================" << endl
           << setw(49) << left << tmp << "=" << endl;
      bool_state();
      cout << "==================================================" << endl;
   }
   void PO_bool_state(unsigned) const;
   void coutGate(int level, int top_level, bool inv) const
   {
      cout << indent(top_level-level) 
           << (inv?"!":"") << "PO " << own_id;
   }
   string getTypeStr() const { return "PO"; }

   int rtn_iid(unsigned s) const 
   { 
      if( s == 0 ) return iid/2;
      else return -1;
   }
   int rtn_riid(unsigned s) const
   {
      if( s == 0 ) return iid;
      else return -1;
   }
   void set_iid(unsigned i, unsigned n) { if( i < 1 ) iid = n; }
   bool is_inverted(unsigned i) const { return ((iid/2)*2 != iid); }

   bool check_float_1 () const;

   void set_state(unsigned long long& i,unsigned& num) 
   { 
      if( (iid/2)*2 != iid )
      {
         unsigned long long I = i;
         inverse(num,I);
         state = I;
      }
      else state = i;
   }
   void set_symbol(string str) { symbol = str; }
   string rtn_symbol() const { return symbol; }

   private:
   unsigned iid;
   string symbol;
};

#endif // CIR_GATE_H
