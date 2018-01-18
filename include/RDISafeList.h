// -*- Mode: C++; -*-
//                              File      : RDISafeList.h
//                              Package   : omniNotify-Library
//                              Created on: 3-Sep-2003
//                              Authors   : gruber
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
 
#ifndef __RDI_SAFELIST_H__
#define __RDI_SAFELIST_H__

#include "thread_wrappers.h"
#include "corba_wrappers.h"

template <class SafeListEntry> class RDI_SafeList;

// This class implements a trivial list of pointers to SafeListEntry
// together with built-in circular iterator functionality.  The iterator
// wraps around, thus producing an infinite sequence of pointers.
// Each time it 'wraps' it produces one null value.  If there are no
// elements, it produces null values each time it is called.
// (There is only ONE built-in iterator.)
//
// To use the methods in a thread-safe manner for more
// than one method call, use the following approach:
//
// { // introduce list lock scope
//    RDI_SAFELIST_SCOPE_LOCK(list_lock, list, "color_list", WHATFN);
//    int len = list->length();
//    for (int i = 0; i < len; ) {
//       SafeListEntry* entry = list->iter_next();
//       if (entry) {
//          i++;
//          ... do something with entry ...
//       }
//    }
// } // end list lock scope
//
// Note: the above code also demonstrates how to iterate over all elements 
// without changing the 'current' position of the built-in iterator, since
// the iterator's end position will be the same as its start position.
// Of course a better way to do this is to use the foreach method.

// To use a single method call in thread-safe manner, use the following
// macros which will grab and release the lock for you:
//
// RDI_SAFELIST_SAFE_DRAIN(list, "color_list", WHATFN);
// RDI_SAFELIST_SAFE_INSERT(list, color_ptr, "color_list", WHATFN);
// RDI_SAFELIST_SAFE_REMOVE(list, color_ptr, "color_list", WHATFN);
// RDI_SAFELIST_SAFE_ITER_NEXT(list, color_ptr, "color_list", WHATFN); // color_ptr is a var that gets the return val
// RDI_SAFELIST_SAFE_LENGTH(list, len, "color_list", WHATFN);          // len is an int var that gets the return val
// RDI_SAFELIST_SAFE_FOREACH(list, fn, arg, "color_list", WHATFN);     // apply fn(elt, arg) to each elt in list

template <class SafeListEntry> class RDI_SafeList {
public:
  typedef RDI_SafeList<SafeListEntry>       SafeList;
  typedef void (*EntryFn)(int idx, int len, SafeListEntry *entry, void *arg);
  WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(SafeListEntry*, PtrSeq);

  inline RDI_SafeList() : _mutex() { drain(); }
  inline ~RDI_SafeList() { drain(); }

  inline void drain()    { _elts.length(0); _next_idx = 0; _deliver_end_marker = 0; }

  inline void insert(SafeListEntry* entry) {
    int len = _elts.length();
    _elts.length(len+1);
    _elts[len] = entry;
    if (_deliver_end_marker) {
      // _next_idx is zero and we were about to mark the end of an iteration;
      // instead, allow new last elt to be part of the current iteration
      _next_idx = len;
      _deliver_end_marker = 0;
    }
  }

  inline void remove(SafeListEntry* entry) {
    int len = _elts.length();
    if (!len) return;
    int prev_idx = _next_idx + len - 1;
    int term_idx =  prev_idx + len;
    // start with prev_idx, entry may have been discovered using iter
    for (int i = prev_idx; i < term_idx; i++) {
      if (_elts[i % len] == entry) {
	_remove_idx(i % len);
	return;
      }
    }
  }

  // iter_next returns 0 if there are no elts
  // and also at the end of each full interation
  inline SafeListEntry* iter_next() {
    if (_deliver_end_marker) {
      _deliver_end_marker = 0;
      return 0;
    }
    int len = _elts.length();
    if (!len) return 0;
    int cur_idx = _next_idx;
    _next_idx = (_next_idx + 1) % len;
    if (_next_idx == 0) {
      // following call should deliver end marker
      _deliver_end_marker = 1;
    }
    return _elts[cur_idx];
  }

  inline int length() const {
    return _elts.length();
  }

  inline void foreach(EntryFn fn, void *arg) {
    int len = _elts.length();
    for (int i = 0; i < len; i++) {
      fn(i, len, _elts[i], arg);
    }
  }

  // NB should only be used by macro below
  inline TW_Mutex& macro_get_mutex() { return _mutex; }

private:
  // dummy copy constructor and operator= to prevent copying
  RDI_SafeList(const SafeList&);
  SafeList& operator=(const SafeList&);
  // state
  TW_Mutex       _mutex;
  PtrSeq         _elts;
  int            _next_idx;
  int            _deliver_end_marker;

  inline void _remove_idx(int idx)
  {
    int max_idx = _elts.length() - 1;
    if (idx > max_idx) return; // should not happen
    if (_next_idx > idx) {
      // Slide _next_idx down so that it will still ref the same pointer
      // (pters at indices > idx are moved down 1)
      _next_idx--;
    } else if (_next_idx == max_idx) {
      // Since _next_idx <= idx, this is the case where _next_idx == idx == max_idx.
      // Last elt is going away, so make _next_idx refer to first elt
      // (this works even for case of _next_idx == idx == max_idx == 0).
      _next_idx = 0;
      if (max_idx) {
	// This deletion means we have ended an iteration.
	// Since there are still some elts (new length > 0)
	// we need to deliver an end marker
	_deliver_end_marker = 1;
      }
    }
    for (int i = idx; i < max_idx; i++) {
      _elts[i] = _elts[i+1];
    }
    _elts.length(max_idx);
  }
};

#define RDI_SAFELIST_SCOPE_LOCK(nm, listp, resty, whatfn) \
    TW_SCOPE_LOCK(nm, (listp)->macro_get_mutex(), resty, whatfn)

#define RDI_SAFELIST_EARLY_RELEASE(nm) \
    TW_EARLY_RELEASE(nm)

#define RDI_SAFELIST_SAFE_DRAIN(list, resty, whatfn) \
do { \
  TW_SCOPE_LOCK(list_lock, (list)->macro_get_mutex(), resty, whatfn); \
  (list)->drain(); \
} while (0)

#define RDI_SAFELIST_SAFE_INSERT(list, color_ptr, resty, whatfn) \
do { \
  TW_SCOPE_LOCK(list_lock, (list)->macro_get_mutex(), resty, whatfn); \
  (list)->insert(color_ptr); \
} while (0)

#define RDI_SAFELIST_SAFE_REMOVE(list, color_ptr, resty, whatfn) \
do { \
  TW_SCOPE_LOCK(list_lock, (list)->macro_get_mutex(), resty, whatfn); \
  (list)->remove(color_ptr); \
} while (0)

#define RDI_SAFELIST_SAFE_RESET_ITER(list, resty, whatfn) \
do { \
  TW_SCOPE_LOCK(list_lock, (list)->macro_get_mutex(), resty, whatfn); \
  (list)->reset_iter(); \
} while (0)

#define RDI_SAFELIST_SAFE_CUR(list, color_ptr, resty, whatfn) \
do { \
  TW_SCOPE_LOCK(list_lock, (list)->macro_get_mutex(), resty, whatfn); \
  color_ptr = (list)->cur(); \
} while (0)

#define RDI_SAFELIST_SAFE_REMOVE_CUR(list, resty, whatfn) \
do { \
  TW_SCOPE_LOCK(list_lock, (list)->macro_get_mutex(), resty, whatfn); \
  (list)->remove_cur(); \
} while (0)

#define RDI_SAFELIST_SAFE_ITER_NEXT(list, elt_ptr_var, resty, whatfn) \
do { \
  TW_SCOPE_LOCK(list_lock, (list)->macro_get_mutex(), resty, whatfn); \
  elt_ptr_var = (list)->iter_next(); \
} while (0)

#define RDI_SAFELIST_SAFE_LENGTH(list, len, resty, whatfn) \
do { \
  TW_SCOPE_LOCK(list_lock, (list)->macro_get_mutex(), resty, whatfn); \
  len = (list)->length(); \
} while (0)

#define RDI_SAFELIST_SAFE_FOREACH(list, fn, arg, resty, whatfn) \
do { \
  TW_SCOPE_LOCK(list_lock, (list)->macro_get_mutex(), resty, whatfn); \
  (list)->foreach(fn, arg); \
} while (0)

#endif  /* !__RDI_SAFELIST_H__ */

