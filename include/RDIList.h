// -*- Mode: C++; -*-
//                              File      : RDIList.h
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
 
#ifndef _RDI_LIST_H_
#define _RDI_LIST_H_

#include "RDIUtil.h"

template <class ListEntry> class RDI_List;

/** A LIST CURSOR
  * The list cursor provides an easy way to traverse a list.  You should
  * keep in mind that the cursor is NOT safe, i.e., if the list changes,
  * the cursor may be in an inconsistent state.
  *
  * Usage scenarios:
  *
  *	RDI_List<ListEntry> l(...);
  *	RDI_ListCursor<ListEntry> c(&l);
  *	// Forward scanning of the list
  *	for (unsigned int indx = 0; indx < l.length(); i++, c++)
  *		const ListEntry& entry = *c;
  *	// Backward scanning of the list
  *	c.reverse();
  *	for (unsigned int indx = 0; indx < l.length(); i++, c--)
  *		const ListEntry& entry = *c;
  */

template <class ListEntry> class RDI_ListCursor {
	friend class RDI_List<ListEntry>;
public:
  typedef RDI_ListCursor<ListEntry> Cursor;
  typedef RDI_List<ListEntry>       List;

  RDI_ListCursor(void) : _list(0), _indx(0)	{;}
  RDI_ListCursor(const List* list) : _list(list), _indx(list->_list_head) {;}
  RDI_ListCursor(const Cursor& cur) : _list(cur._list),  _indx(cur._indx) {;}
  ~RDI_ListCursor() {;}

  void reset()		{ _indx = _list->_list_head; }
  void reverse()	{ _indx = _list->_list_tail; }
  void clear()		{ _list = 0; _indx = 0; }

  Cursor operator= (const Cursor& cursor)
		{ _list = cursor._list; _indx = cursor._indx; return *this; }

  int operator==(const Cursor& cur)
                { return ((_list == cur._list) && (_indx == cur._indx)); }
  int operator!=(const Cursor& cur)
                { return ((_list != cur._list) || (_indx != cur._indx)); }

  Cursor& operator++()	   { _list->next_index(_indx); return *this; }
  Cursor operator++(int)   { Cursor __tmp = *this; ++*this; return __tmp; }

  Cursor& operator--()	   { _list->prev_index(_indx); return *this;  }
  Cursor operator--(int)   { Cursor __tmp = *this; --*this; return __tmp; }
 
  const ListEntry& operator*() const { return _list->entry(_indx); }
private:
  const List*  _list;
  unsigned int _indx;
};


/** A LIST CONTAINING HOMOGENEOUS ELEMENTS
  * The list is implemented as an array.  One can limit the maximum size
  * of the list to a specific number of elements.  When 'max_size' is 0,
  * only the available memory limits the maximum size of the list.
  *
  * NOTE: the class 'ListEntry' should provide a default constructor and
  *       have the assignment operator defined.
  */

template <class ListEntry> class RDI_List {
	friend class RDI_ListCursor<ListEntry>;
public:
  typedef RDI_ListCursor<ListEntry> Cursor;
  typedef RDI_List<ListEntry>       List;

  inline RDI_List(unsigned int init_size=4, 
	  	  unsigned int incr_size=4, unsigned int maxm_size=0);
  inline RDI_List(const List& list);
  inline ~RDI_List();

  inline int  insert_head(const ListEntry& entry);
  inline int  insert_tail(const ListEntry& entry);
  inline void remove_head();
  inline void remove_tail();

  // Remove the entry pointed by the cursor -- after the operation
  // is completed, the  cursor is invalidated

  inline void remove(const Cursor& position);

  // Return the entry at the head/tail -- entry is NOT removed

  inline ListEntry get_head();
  inline ListEntry get_tail();

  unsigned int length() const		{ return _num_items; } 
  unsigned int max_size() const		{ return _maxm_size; }

  void set_inc_size(unsigned int size)	{ _incr_size = size; }
  void set_max_size(unsigned int size)	{ _maxm_size = size; }

  Cursor cursor() const			{ return this; }

  void drain()				{ _num_items=_list_head=_list_tail=0; }
private:
  unsigned int _num_items;
  unsigned int _curr_size;
  unsigned int _incr_size;
  unsigned int _maxm_size;
  unsigned int _list_head;
  unsigned int _list_tail;
  ListEntry*   _entry;

  const ListEntry& entry(unsigned int index) const { return _entry[index]; }

  // Increment the size of the list, if the maximum has not been hit

  inline int resize();

  // Given a VALID list index, compute the next/previous VALID index

  inline void next_index(unsigned int &index) const;
  inline void prev_index(unsigned int &index) const;
};

// --------------------------------------------------------- //

template <class ListEntry> inline
RDI_List<ListEntry>::RDI_List(unsigned int init_size, 
			      unsigned int incr_size, unsigned int maxm_size) :
		_num_items(0), _curr_size(0), _incr_size(incr_size),
		_maxm_size(maxm_size), _list_head(0), _list_tail(0)
{ _curr_size = (maxm_size && (init_size > maxm_size)) ? maxm_size : init_size;
  _entry = new ListEntry[_curr_size]; 
}

template <class ListEntry> inline
RDI_List<ListEntry>::RDI_List(const RDI_List<ListEntry>& list) :
		_num_items(list._num_items), _curr_size(list._curr_size),
		_incr_size(list._incr_size), _maxm_size(list._maxm_size),
		_list_head(list._list_head), _list_tail(list._list_tail)
{ _entry = new ListEntry[_curr_size];
  for (unsigned int i = _list_head; i < _list_head + _num_items; i++)
	_entry[ i % _curr_size ] = list._entry[ i % _curr_size ];
}

template <class ListEntry> inline
RDI_List<ListEntry>::~RDI_List()
{ drain(); delete [] _entry; }

template <class ListEntry> inline
int RDI_List<ListEntry>::insert_head(const ListEntry& entry)
{
  if ( (_num_items == _curr_size) && (resize() == -1) )
	return -1;
  if ( _num_items == 0 ) {
	_list_head =_list_tail = 0;
  } else {
  	_list_head = (_list_head == 0) ? (_curr_size - 1) : (_list_head - 1);
  }
  _entry[_list_head] = entry;
  _num_items += 1;
  return 0;
}

template <class ListEntry> inline
int RDI_List<ListEntry>::insert_tail(const ListEntry& entry)
{
  if ( (_num_items == _curr_size) && (resize() == -1) )
	return -1;
  if ( _num_items == 0 ) {
	_list_head =_list_tail = 0;
  } else {
  	_list_tail = (_list_tail == (_curr_size - 1)) ? 0 : (_list_tail + 1);
  }
  _entry[_list_tail] = entry;
  _num_items += 1;
  return 0;
}

template <class ListEntry> inline
void RDI_List<ListEntry>::remove_head()
{
  if ( _num_items == 0 )
	return;
  _list_head = (_list_head == (_curr_size - 1)) ? 0 : (_list_head + 1);
  _num_items -= 1;
}

template <class ListEntry> inline
void RDI_List<ListEntry>::remove_tail()
{
  if ( _num_items == 0 )
	return;
  _list_tail = (_list_tail == 0) ? (_curr_size - 1) : (_list_tail - 1);
  _num_items -= 1;
}

template <class ListEntry> inline
void RDI_List<ListEntry>::remove(const Cursor& cursor)
{
  unsigned int nl, nr, i, indx = cursor._indx;

  if ( cursor._list != this ) 
	return;
  if ((_list_head > _list_tail) && (indx < _list_head) && (indx > _list_tail))
        return;
  if ((_list_head < _list_tail) && ((indx < _list_head) || (indx > _list_tail)))
        return;

  _num_items -= 1;

  if ( _num_items == 0 ) {      // No entries left in the list
	_list_head = 0;
	_list_tail = 0;
	return;
  } else if ( indx == _list_head ) {
	_list_head = (_list_head + 1) % _curr_size;
  } else if ( indx == _list_tail ) {
	_list_tail = (_list_tail + _curr_size - 1) % _curr_size;
  } else {
  	nl = (indx > _list_head) ? (indx - _list_head) :
				   (_curr_size + indx - _list_head);
	nr = (indx < _list_tail) ? (_list_tail - indx) :
				   (_curr_size + _list_tail - indx);

	if ( nr < nl ) {
		for (i=0; i < nr; i++) {
			_entry[(indx+i) % _curr_size] = 
					_entry[(indx+i+1) % _curr_size];
		}
		_list_tail = (_list_tail+_curr_size-1) % _curr_size;
	} else {
		for (i=0; i < nl; i++) {
			_entry[(indx+_curr_size-i) % _curr_size] =
				_entry[(indx+_curr_size-i-1) % _curr_size];

		}
		_list_head = (_list_head + 1) % _curr_size;
	}
  }
}

template <class ListEntry> inline
ListEntry RDI_List<ListEntry>::get_head()
{
  if ( _num_items == 0 )
	return ListEntry();
  return _entry[_list_head];
}

template <class ListEntry> inline
ListEntry RDI_List<ListEntry>::get_tail()
{
  if ( _num_items == 0 )
	 return ListEntry();
  return _entry[_list_tail];
}

template <class ListEntry> inline
int RDI_List<ListEntry>::resize()
{
  unsigned int new_size = _curr_size + _incr_size;
  ListEntry* tmp_buff = 0;

  if ( _maxm_size && (_curr_size == _maxm_size) )
	return -1;
  if ( _maxm_size && (new_size > _maxm_size) )
	new_size = _maxm_size;
  if ( (tmp_buff = new ListEntry[new_size]) == (ListEntry *) 0 )  
	return -1;
  for (unsigned int i=0; i < _num_items; i++)
	tmp_buff[i] = _entry[(_list_head + i) % _curr_size];
  _curr_size = new_size;
  _list_head = 0;
  _list_tail = _num_items - 1;
  delete [] _entry;
  _entry = tmp_buff;
  return 0;
}

template <class ListEntry> inline
void RDI_List<ListEntry>::next_index(unsigned int &idx) const
{ idx = (idx==_list_tail) ? _list_head : ((idx + 1) % _curr_size); }

template <class ListEntry> inline
void RDI_List<ListEntry>::prev_index(unsigned int &idx) const
{ idx = (idx==_list_head) ? _list_tail : (idx ? (idx - 1) : (_curr_size - 1)); }

#endif
