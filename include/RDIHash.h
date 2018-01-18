// -*- Mode: C++; -*-
//                              File      : RDIHash.h
//                              Package   : omniNotify-Library
//                              Created on: 1-Jan-1998
//                              Authors   : gruber&panagos
//
//    Copyright (C) 1998-2003 AT&T Laboratories -- Research
//
//    This file is part of the omniNotify library
//    and is distributed with the omniNotify release.
//
//    The omniNotify library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; if not, write to the Free
//    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//    02111-1307, USA
//
//
// Description:
//    proprietary interface
//
 
#ifndef _RDI_HASH_H_
#define _RDI_HASH_H_

#include "RDIstrstream.h"
#include "RDIHashFuncs.h"

template <class Key, class Val> class RDI_Hash;

/** KEY VALUE PAIR HASH TABLE ENTRY
 * Each hash table entry contains a key, a value, and a pointer to
 * the next hash table entry that is hashed at the same bucket.
 */

template <class Key, class Val> struct RDI_KeyValuePair {
  Key _key;
  Val _val;
  RDI_KeyValuePair* _next;
};

/** A HASH TABLE CURSOR
 * The hash cursor provides  an easy way to traverse a hash table.
 * You should keep in mind that the cursor is *NOT* safe, i.e., if
   * the table changes, the cursor may be in an inconsistent state.
   *
   * Usage scenario:
*
   *     RDI_Hash<Key, Val> h(...);
*     RDI_HashCursor(Key, Val) c;
*	for ( c = h.cursor(); c.is_valid(); c++ ) {
*		const Key& key = c.key();
*		const Val& val = c.val();
*     }
*/

template <class Key, class Val> class RDI_HashCursor {
public:
  typedef RDI_KeyValuePair<Key, Val> KeyValPair;
  typedef RDI_HashCursor<Key, Val>   Cursor;
  typedef RDI_Hash<Key, Val>         Hash;

  RDI_HashCursor() : _hash(0), _node(0), _bnum(0) {;}
  RDI_HashCursor(const Hash* hash) :
    _hash(hash) { hash->frst_node(_node, _bnum); }
  RDI_HashCursor(const Cursor& c) :
    _hash(c._hash), _node(c._node), _bnum(c._bnum) {;}
  ~RDI_HashCursor()	{;}

  Cursor operator= (const Cursor& c)
  { _hash=c._hash; _node=c._node; _bnum=c._bnum; return *this; }

  inline int operator==(const Cursor& c);
  inline int operator!=(const Cursor& c);

  Cursor& operator++()     { _hash->next_node(_node,_bnum); return *this; }
  Cursor  operator++(int)  { Cursor __tmp = *this; ++*this; return __tmp; }

  int is_valid() const	   { return (_hash && _node) ? 1 : 0; }

  // The following assume that the cursor is in a valid state

  const Key& key(void) const		{ return _node->_key; }
  const Val& const_val(void) const	{ return _node->_val; }
  Val&       val(void) const		{ return _node->_val; }

private:
  const Hash*  _hash;
  KeyValPair*  _node;
  unsigned int _bnum;
};

// --------------------------------------------------------- //

template <class Key, class Val> inline 
int RDI_HashCursor<Key, Val>::operator==(const RDI_HashCursor<Key, Val>& c) 
{ return ((_hash == c._hash) && (_node == c._node) && (_bnum == c._bnum)); }

template <class Key, class Val> inline 
int RDI_HashCursor<Key, Val>::operator!=(const RDI_HashCursor<Key, Val>& c) 
{ return ((_hash != c._hash) || (_node != c._node) || (_bnum != c._bnum)); }


/** LINEAR HASH TABLE
 * Currently, hash tables are constructed using the linear hashing
 * approach, which was originally described by Litwin (VLDB 1980).
 *
 *  - hash_func: hashes keys and returns unsigned integer;
 *  - rank_func: compares two keys and returns a negative, 0, or a
 *               positive integer when the first key is <, =, or >
 *               than the second, respectively;
 *  - init_size: size of initial directory for the hash table;
 *  - buck_size: the capacity of each bucket in the hash table.
 * 
 * NOTE: only unique keys are implemented at this time
 */

template <class Key, class Val> class RDI_Hash {
  friend class RDI_HashCursor<Key, Val>;
public:
  typedef RDI_KeyValuePair<Key, Val> KeyValPair;
  typedef RDI_HashCursor<Key, Val>   Cursor;
  typedef RDI_Hash<Key, Val>         Hash;

  inline RDI_Hash(RDI_FuncHash hash_func, RDI_FuncRank rank_func, 
		  unsigned int init_size=32, unsigned int buck_size=10);
  ~RDI_Hash(void)	{ clear(); delete [] _htable; }

  unsigned int length(void) const	{ return _num_pairs; }

  inline int   insert(const Key& key, const Val& val);
  inline int   lookup(const Key& key, Val& val);
  inline int   exists(const Key& key);
  inline void  replace(const Key& key, const Val& val);
  inline void  remove(const Key& key);
  inline void  clear(void);
  inline void  statistics(RDIstrstream& str);

  Cursor cursor(void) const		{ return this; }

private:

  struct BucketNode_t {
    unsigned int _count;	// # <key, value> pairs in bucket
    KeyValPair*  _nodes;	// The list of <key, value> pairs
  };

  RDI_FuncHash  _hash_func;
  RDI_FuncRank  _rank_func;
  unsigned int  _curr_size;
  unsigned int  _next_size;
  unsigned int  _split_idx;
  unsigned int  _curr_mask;
  unsigned int  _next_mask;
  unsigned int  _num_pairs;	// # of <key, value> pairs in table
  unsigned int  _bcapacity;	// Maximum capacity for each bucket
  unsigned int  _num_split;
  BucketNode_t* _htable;

  RDI_Hash();
  RDI_Hash(const RDI_Hash&);
  RDI_Hash& operator= (const RDI_Hash&);

  inline void frst_node(KeyValPair*& node, unsigned int& bnum) const;
  inline void next_node(KeyValPair*& node, unsigned int& bnum) const;

  inline unsigned int pseudo_hash(const Key& key);
  inline KeyValPair*  lookup(const Key& key, 
			     KeyValPair*& prev, unsigned int& bnum);
  inline int split(void);
};

// --------------------------------------------------------- //

template <class Key, class Val> inline
RDI_Hash<Key, Val>::RDI_Hash( RDI_FuncHash hash, RDI_FuncRank rank,
			      unsigned int size, unsigned int capa ) : 
  _hash_func(hash), _rank_func(rank), _split_idx(0), _num_pairs(0)
{
  _curr_size = 1;
  while ( _curr_size < size )	// Size of hash table is a power
    _curr_size <<= 1;	// of 2 that is >= size

  _next_size = _curr_size;
  _next_mask = _curr_mask = _curr_size - 1;
  _bcapacity = (capa == 0) ? 5 : capa;
  _num_split = 0;

  _htable = new BucketNode_t [ _curr_size ];
  if (! _htable) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  for (unsigned int ix=0; ix < _curr_size; ix++) {
    _htable[ix]._count = 0; 
    _htable[ix]._nodes = 0;
  }
}

template <class Key, class Val> inline
int RDI_Hash<Key, Val>::insert(const Key& key, const Val& val)
{
  int done = 0;
  unsigned int bnum = 0, nume = 0, num_splits = 0;
  KeyValPair* node = 0;

  // Since we assume unique keys, check if the key exists already

  if ( lookup(key, node, bnum) != (KeyValPair *) 0 ) {
    return -1;
  }

  while ( ! done ) {
    bnum = pseudo_hash(key);
    nume = _htable[ bnum ]._count;

    // If we have reached the capacity of the bucket, split and
    // try the insertion again - to avoid splitting for ever in
    // the case where the provided hash function is bad,  allow
    // at most 5 splits before exceeding the bucket capacity...

    if ( (nume >= _bcapacity) && (num_splits < 5) ) {
      if ( ! split() ) {
	return -1;
      }
      num_splits += 1;
      continue;
    }

    if ( (node = new KeyValPair) == (KeyValPair *) 0 ) {
      return -1;
    }

    node->_key  = key;
    node->_val  = val;
    node->_next = _htable[ bnum ]._nodes;
    _htable[ bnum ]._nodes = node;
    _htable[ bnum ]._count += 1;
    _num_pairs += 1;
    done = 1;
  }
  return 0; 
}

template <class Key, class Val> inline
int RDI_Hash<Key, Val>::lookup(const Key& key, Val& val)
{
  KeyValPair *curr=0, *prev=0;
  unsigned int  bnum = 0;

  if ( (curr = lookup(key, prev, bnum)) != (KeyValPair *) 0 ) {
    val = curr->_val;
    return 1;
  }
  return 0;
}

template <class Key, class Val> inline
int RDI_Hash<Key, Val>::exists(const Key& key)
{
  KeyValPair *prev=0;
  unsigned int  bnum = 0;

  if ( lookup(key, prev, bnum) != (KeyValPair *) 0 )
    return 1;
  return 0;
}

template <class Key, class Val> inline
void RDI_Hash<Key, Val>::replace(const Key& key, const Val& val)
{
  KeyValPair *curr=0, *prev=0;
  unsigned int  bnum = 0;

  if ( (curr = lookup(key, prev, bnum)) != (KeyValPair *) 0 )
    curr->_val = val;
}

template <class Key, class Val> inline
void RDI_Hash<Key, Val>::remove(const Key& key)
{
  KeyValPair *curr=0, *prev=0;
  unsigned int  bnum = 0;

  if ( (curr = lookup(key, prev, bnum)) != (KeyValPair *) 0 ) {
    if ( prev == (KeyValPair *) 0 ) 
      _htable[ bnum ]._nodes = curr->_next;
    else 
      prev->_next = curr->_next;
    delete curr;
    _htable[ bnum ]._count -= 1;
    _num_pairs -= 1;
  }
}

template <class Key, class Val> inline
void RDI_Hash<Key, Val>::clear()
{
  for (unsigned int indx=0; indx < _next_size; indx++) {
    while ( _htable[indx]._nodes != (KeyValPair *) 0 ) {
      KeyValPair* node = _htable[indx]._nodes;
      _htable[indx]._nodes = node->_next;
      delete node;
    }
    _htable[indx]._count = 0;
  }
  _curr_size = _next_size;
  _curr_mask = _next_mask;
  _split_idx = 0;
  _num_pairs = 0;
}

template <class Key, class Val> inline
void RDI_Hash<Key, Val>::statistics(RDIstrstream& str)
{
  str << "Size " << _curr_size << " [" << _next_size << "] #Keys " << 
    _num_pairs << " Split Index " << _split_idx << " #Split " << 
    _num_split << " Bucket Size " << _bcapacity << '\n';
  for (unsigned int indx = 0; indx < _next_size; indx++) {
    if ( _htable[ indx ]._count != 0 ) {
      str << "\tBucket # ";
      str.setw(5);
      str << indx << " contains " <<
	_htable[ indx ]._count << " keys\n";
    }
  }
}

template <class Key, class Val> inline
void RDI_Hash<Key, Val>::frst_node(RDI_KeyValuePair<Key, Val>*& node,
				   unsigned int& bnum) const
{
  node = 0; bnum = _next_size;
  for (unsigned int indx = 0; indx < _next_size; indx++) {
    if ( _htable[ indx ]._count != 0 ) {
      node = _htable[ indx ]._nodes;
      bnum = indx;
      return;
    }
  }
}

template <class Key, class Val> inline
void RDI_Hash<Key, Val>::next_node(RDI_KeyValuePair<Key, Val>*& node, 
				   unsigned int& bnum) const
{
  if ( node == (KeyValPair *) 0 ) {
    bnum = _next_size;
  } else if ( node->_next != (KeyValPair *) 0 ) {
    node = node->_next;
  } else {
    for (unsigned int indx = bnum + 1; indx < _next_size; indx++) {
      if ( _htable[ indx ]._count != 0 ) {
	node = _htable[ indx ]._nodes;
	bnum = indx;
	return;
      }
    }
    node = 0;
    bnum = _next_size;
  }
}

template <class Key, class Val> inline
unsigned int RDI_Hash<Key, Val>::pseudo_hash(const Key& key)
{
  unsigned int indx = (*_hash_func)((const void *) &key);
  unsigned int bnum = indx & _curr_mask;
  return (bnum < _split_idx) ? (indx & _next_mask) : bnum;
}

template <class Key, class Val> inline
RDI_KeyValuePair<Key, Val>*
RDI_Hash<Key, Val>::lookup(const Key& key,
			   RDI_KeyValuePair<Key, Val>*& prev,
			   unsigned int & bnum)
{
  KeyValPair* curr;

  bnum = pseudo_hash(key);
  prev = 0;
  curr = _htable[ bnum ]._nodes;

  while ( (curr != (KeyValPair *) 0) && (*_rank_func)(&key, &curr->_key) ) {
    prev = curr;
    curr = curr->_next;
  }
  return curr;
}

template <class Key, class Val> inline
int RDI_Hash<Key, Val>::split()
{
  BucketNode_t* htbl = 0;
  KeyValPair*   node = 0;
  KeyValPair*   prev = 0;
  unsigned int  indx = 0;

  // If we have split all existing buckets, we set directory size to
  // '_next_size', hash mask to '_next_mask', and '_split_idx' to 0.
  // (we have already doubled the directory size and, thus, there is
  // no need for memory allocation at this point)

  if ( _split_idx == _curr_size ) {
    _curr_size = _next_size;
    _curr_mask = _next_mask;
    _split_idx = 0;
    return 1;
  }

  // Do we have to allocate a new bucket?  If yes,  we will actually
  // double the size of the directory -- memory resident hash table

  if ( _curr_size == _next_size ) {
    _next_size = _curr_size * 2;
    _next_mask = _next_size - 1;

    if ( (htbl = new BucketNode_t [ _next_size ]) == (BucketNode_t *) 0 ) {
      _next_size = _curr_size;
      _next_mask = _curr_mask;
      return 0;
    }

    for ( indx = 0; indx < _curr_size; indx++ ) {
      htbl[ indx ]._count = _htable[ indx ]._count;
      htbl[ indx ]._nodes = _htable[ indx ]._nodes;
    }
    for ( ; indx < _next_size; indx++ ) {
      htbl[ indx ]._count = 0;
      htbl[ indx ]._nodes = 0;
    }

    delete [] _htable;
    _htable = htbl;
  }

  // Go through the entries in the split bucket  and distribute them
  // between the split bucket and the "newly" allocated one.........

  // RDIErrLog("** Rehash: size " << _curr_size << " [" << _next_size << "]\n");

  node = _htable[ _split_idx ]._nodes;
  while ( node != (KeyValPair *) 0 ) {
    indx = (*_hash_func)(& node->_key) & _next_mask;

    // RDIErrLog(node->_key << " : " << _split_idx << " --> " << indx << '\n');

    if ( indx == _split_idx ) {
      prev = node;
      node = node->_next;
    } else {
      if ( prev == (KeyValPair *) 0 ) 
	_htable[ _split_idx ]._nodes = node->_next;
      else 
	prev->_next = node->_next;
      _htable[ _split_idx ]._count -= 1;

      node->_next = _htable[ indx ]._nodes;
      _htable[ indx ]._nodes = node;
      _htable[ indx ]._count += 1;

      node = prev ? prev->_next : _htable[ _split_idx ]._nodes;
    }
  }

  // RDIErrLog("** Rehash is over\n");

  _num_split += 1;
  _split_idx += 1;

  return 1;
}

#endif
