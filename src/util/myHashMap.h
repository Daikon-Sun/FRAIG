/****************************************************************************
  FileName     [ myHashMap.h ]
  PackageName  [ util ]
  Synopsis     [ Define HashMap and Cache ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2009-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_MAP_H
#define MY_HASH_MAP_H

#include <vector>

using namespace std;

template <class T> class myList;

template <class T>
class myListNode
{
   friend class myList<T>;
   friend class CirMgr;
   //friend class myList<T>::iterator;

   myListNode(const T& d, myListNode<T>* n = 0) : _d(d), _n(n) {}
   ~myListNode() {}

   T _d;
   myListNode<T>* _n;
};

template <class T>
class myList
{
   friend class CirMgr;
public:
   myList() 
   {
      _h = new myListNode<T>(T());
      _h->_n = _h;
      _size = 0;
   }
   ~myList() { clear(); delete _h; }
   /*
   class iterator
   {
      friend class myList;
   public:
      iterator(myListNode<T>* n= 0): _node(n) {}
      iterator(const iterator& i) : _node(i._node) {}
      ~iterator() {} // Should NOT delete _node
      const T& operator * () const { return _node->_data; }
      T& operator * () { return _node->_data; }
      iterator& operator ++ () 
      {  
         _node = _node->_next; 
         return *(this); 
      }
      iterator operator ++ (int) 
      { 
         iterator tmp_it(*this);
         ++(*this);
         return tmp_it; 
      }
      iterator& operator = (const iterator& i) 
      { 
         this->_node = i._node;
         return *(this); 
      }
      bool operator != (const iterator& i) const { return (i._node != this->_node); }
      bool operator == (const iterator& i) const { return (i._node == this->_node); }
   private:
      myListNode<T>* _node;
   };

   iterator begin() const 
   { 
      return iterator(_head->_next);
   }
   iterator end() const 
   { 
      return iterator(_head);
   }*/
   size_t size() const { return _size; }
   void push_front(const T& x) 
   {
      myListNode<T>* _new = new myListNode<T>(x, _h->_n);
      _h->_n = _new;
      ++_size;
   }
   void pop_front() 
   {
      if ( _size == 0 ) return;
      myListNode<T>* n = _h->_n->_n;
      delete _h->_n;
      _h->_n = n;
      --_size;
   }
   bool erase(myListNode<T>*& _d, myListNode<T>*& _p)
   {
      if( _size == 0 ) return false;
      _p->_n = _d->_n;
      delete _d;
      --_size;
      return true;
   }
   void clear() 
   {
      if( _size == 0 ) return;
      myListNode<T>* move = _h->_n;
      do
      {
         myListNode<T>* temp = move;
         move = move->_n;
         delete temp;
      } while ( move != _h );
      _h->_n = _h;
      _size = 0;
   }  
private:
   myListNode<T>*  _h;  // = dummy node if list is empty
   unsigned _size;
};

class HashKey
{
public:
   HashKey() {}
   HashKey(unsigned i0, unsigned i1) : i0(i0), i1(i1) {}

   size_t operator() () const 
   { 
      if(((i0/2)*2!=i0)^((i1*2)/2!=i1)) return i0/2;
      else return i1/2;
   }
   bool operator == (const HashKey& k) const 
   { 
      return (k.i0==i0&&k.i1==i1); 
   }

private:
   size_t i0, i1;
};

template <class HashKey, class HashData>
class HashMap
{
typedef pair<HashKey, HashData> HashNode;

public:
   HashMap() : _numBuckets(0), _buckets(0) {}
   HashMap(size_t b) : _numBuckets(0), _buckets(0) { init(b); }
   ~HashMap() { reset(); }
   
   void init(size_t b) {
      reset(); _numBuckets = b; _buckets = new vector<HashNode>[b]; }
   void reset() {
      _numBuckets = 0;
      if (_buckets) { delete [] _buckets; _buckets = 0; }
   }
   size_t numBuckets() const { return _numBuckets; }

   vector<HashNode>& operator [] (size_t i) { return _buckets[i]; }
   const vector<HashNode>& operator [](size_t i) const { return _buckets[i]; }

   // return true if no valid data
   bool empty() const {
      for (size_t i = 0; i < _numBuckets; ++i)
         if (_buckets[i].size() != 0) return false;
      return true;
   }
   // number of valid data
   size_t size() const {
      size_t s = 0;
      for (size_t i = 0; i < _numBuckets; ++i) s += _buckets[i].size();
      return s;
   }
   // check if k is in the hash...
   // if yes, update n and return true;
   // else return false;
   bool check(const HashKey& k, HashData& n) const {
      size_t b = bucketNum(k);
      for (size_t i = 0, bn = _buckets[b].size(); i < bn; ++i)
         if (_buckets[b][i].first == k) {
            n = _buckets[b][i].second;
            return true;
         }
      return false;
   }

   // return true if inserted successfully (i.e. k is not in the hash)
   // return false is k is already in the hash ==> will not insert
   bool insert(const HashKey& k, HashData& d) {
      size_t b = k();
      for (size_t i = 0, bn = _buckets[b].size(); i < bn; ++i)
         if (_buckets[b][i].first == k)
         {
            d = _buckets[b][i].second;
            return false;
         }
      _buckets[b].push_back(HashNode(k, d));
      return true;
   }

   // return true if inserted successfully (i.e. k is not in the hash)
   // return false is k is already in the hash ==> still do the insertion
   bool replaceInsert(const HashKey& k, const HashData& d) {
      size_t b = bucketNum(k);
      for (size_t i = 0, bn = _buckets[b].size(); i < bn; ++i)
         if (_buckets[b][i].first == k) {
            _buckets[b][i].second = d;
            return false;
         }
      _buckets[b].push_back(HashNode(k, d));
      return true;
   }

   // Need to be sure that k is not in the hash
   void forceInsert(const HashKey& k, const HashData& d) {
      _buckets[bucketNum(k)].push_back(HashNode(k, d)); }

private:
   // Do not add any extra data member
   size_t                   _numBuckets;
   vector<HashNode>*        _buckets;

   size_t bucketNum(const HashKey& k) const { 
      return (k()%_numBuckets); }
};










/*
class CacheKey
{
public:
   CacheKey() {}
   CacheKey(unsigned i0, unsigned i1, bool b0, bool b1) 
      : i0(i0), i1(i1), b0(b0), b1(b1) {}

   size_t operator() () const 
   { 
      return((b0^b1)*aig_num*aig_num 
            +(b0?i1:i0)*aig_num
            +(b0?i0:i1)); 
   }

   bool operator == (const CacheKey& k) const { return (k.i0==i0&&k.i1==i1); }

   static void set_aig_num(unsigned n) { aig_num = n; } 

private:
   size_t i0, i1;
   bool b0, b1;
   static unsigned aig_num;
};

template <class CacheKey, class CacheData>
class Cache
{
typedef pair<CacheKey, CacheData> CacheNode;

public:
   Cache() : _size(0), _cache(0) {}
   Cache(size_t s) : _size(0), _cache(0) { init(s); }
   ~Cache() { reset(); }

   // NO NEED to implement Cache::iterator class

   // TODO: implement these functions
   //
   // Initialize _cache with size s
   void init(size_t s) { reset(); _size = s; _cache = new CacheNode[s]; }
   void reset() {  _size = 0; if (_cache) { delete [] _cache; _cache = 0; } }

   size_t size() const { return _size; }

   CacheNode& operator [] (size_t i) { return _cache[i]; }
   const CacheNode& operator [](size_t i) const { return _cache[i]; }

   // return false if cache miss
   bool read(const CacheKey& k, CacheData& d) const {
      size_t i = k() % _size;
      if (k == _cache[i].first) {
         d = _cache[i].second;
         return true;
      }
      return false;
   }
   // If k is already in the Cache, overwrite the CacheData
   void write(const CacheKey& k, const CacheData& d) {
      size_t i = k() % _size;
      _cache[i].first = k;
      _cache[i].second = d;
   }

private:
   // Do not add any extra data member
   size_t         _size;
   CacheNode*     _cache;
};*/
#endif // MY_HASH_H
