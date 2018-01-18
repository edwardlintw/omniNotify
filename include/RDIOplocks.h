// -*- Mode: C++; -*-
//                              File      : RDIOplocks.h
//                              Package   : omniNotify-Library
//                              Created on: 6-Nov-2000
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
 
#ifndef _RDI_OPLOCKS_H_
#define _RDI_OPLOCKS_H_

#include "RDIsysdep.h"
#include "thread_wrappers.h"
#include "corba_wrappers.h"

// ----------------------------------------------------------- //
// Portable support for deallocating objects that might be
// "in use" at the time a deallocate request occurs.
// ----------------------------------------------------------- // 

// See RDIOplocksMacros.h for useful macros related to RDIOplocks

// ----------------------------------------------------------- //
// RDIOplockEntry 
// ----------------------------------------------------------- //

// INVARIANTS:
//    ptr is NULL => inuse is zero and will remain zero until entry is re-assigned
//    inuse > 0 => ptr is non-NULL
//    disposed is 1, inuse > 0
//            => inuse should not be incremented anymore, only decremented
// TRANSITIONS:
//    inuse goes from 2 to 1
//            => this causes an inuseone.broadcast()
//    disposed is 1, inuse goes from 1 to 0
//            => this triggers a dispose action which causes a transition
//               to uninitialized state where ptr, inuse, disposed are all zero

class RDIOplocks;
class RDIOplockEntry;

#ifndef NDEBUG
#include "RDIHash.h"
typedef RDI_Hash<RDIOplockEntry*, int> RDIOplocksDbgMap;
#endif

class RDIOplockEntry {
public:
  RDIOplockEntry() : _oplock(), _waitvar(&_oplock), _inuseone(&_oplock),
		     _inuse(0), _ptr(0), _disposed(0), _owner_name(0), _resty(0)
  {
    _prev = this;
    _next = this;
    _dispose_info = WRAPPED_DISPOSEINFO_NIL;
  }

  // add_condition associates a new condition variable with _oplock
  // caller is responsible for deleting when it is no longer needed
  TW_Condition* add_condition() { return new TW_Condition(&_oplock); }

  // * acquire only acquires lock if _ptr and ptr are equal and _disposed is false
  // * reacquire only acquires lock if _ptr and ptr are equal
  ///   (each returns true if the lock is acquired, else false) 
  // * release releases the lock (must be held)
  CORBA::Boolean acquire(RDIOplockEntry** ptr);
  CORBA::Boolean reacquire(RDIOplockEntry** ptr);
  void release()         { _oplock.unlock(); }   
  // these methods should only be called while holding oplock
  void bump();
  void debump();
  void signal()          { _waitvar.signal(); } 
  void broadcast()       { _waitvar.broadcast(); }

  void wait();
  void timedwait(unsigned long s, unsigned long n);
  void altcv_wait(TW_Condition& cv);
  void altcv_timedwait(TW_Condition& cv, unsigned long s, unsigned long n);

  void inuseone_wait()  {_inuseone.wait(); }
  void inuseone_timedwait(unsigned long s, unsigned long n)
                         {_inuseone.timedwait(s,n); }

  // these state access methods should be called while holding oplock 
  // if accurate values are required.
  CORBA::UShort  inuse()    { return _inuse; }
  CORBA::Boolean disposed() { return _disposed; }
  RDIOplockEntry** ptr()    { return _ptr; }         

#ifdef TW_DEBUG
  // debug versions of wait and timedwait:
  void dbg_wait(const char *whatfn, const char *file, int line);
  void dbg_timedwait(unsigned long s, unsigned long n, const char *whatfn, const char *file, int line);
  void dbg_altcv_wait(TW_Condition &cv, const char *whatfn, const char *file, int line);
  void dbg_altcv_timedwait(TW_Condition &cv, unsigned long s, unsigned long n, const char *whatfn, const char *file, int line);
  void dbg_inuseone_wait(const char *whatfn, const char *file, int line);
  void dbg_inuseone_timedwait(unsigned long s, unsigned long n, const char *whatfn, const char *file, int line);
#endif

  // N.B. only special macros should call these methods
  TW_Mutex* macro_get_mutex_ptr()    			{ return &_oplock; }
  const char* macro_get_resty()                         { return _resty; } 

private:
  friend class RDIOplocks;
  TW_Mutex                 _oplock;
  TW_Condition             _waitvar;
  TW_Condition             _inuseone;
  CORBA::UShort            _inuse;
  RDIOplockEntry**         _ptr;
  CORBA::Boolean           _disposed;
  WRAPPED_DISPOSEINFO_VAR  _dispose_info;
  AttN::NameSeq*           _owner_name;
  const char*              _resty;

  // for chaining entry on a doubly-linked list
  RDIOplockEntry*          _prev;
  RDIOplockEntry*          _next;

  // remove this from list (becomes singleton list)
  void _remove();
  // prepend this in front of b (b is in chain, this is not)
  void _prepend(RDIOplockEntry* b);
  // append this after b (b is in chain, this is not)
  void _append(RDIOplockEntry* b);


};

// ----------------------------------------------------------- //
//  RDIOplocks
// ----------------------------------------------------------- //

class RDIOplocks {
public:
  static RDIOplockEntry* alloc_entry(RDIOplockEntry** optr, AttN::NameSeq* oname, const char *resty);
  static void            free_entry(RDIOplockEntry* e, RDIOplockEntry** optr, WRAPPED_DISPOSEINFO_PTR dinfo);

  // * alloc_entry returns an entry whose ptr is
  //   set to optr (normally '(&_oplockptr)' is used).
  //   (the entry is not locked)

  // * free_entry should be called while holding e's oplock
  //   and it should only be called by the object whose ptr
  //   is stored in ptr -- we pass in optr to allow for a sanity check.
  //   A side-effect is that e's oplock is released.
  //   Another side-effect: if a non-nil dinfo is passed to free_entry,
  //   then RDIOplocks is in charge of disposal.  If inuse is zero disposal
  //   happens immediately, otherwise it is deferred; disposed is set to 1.
  //   N.B.: If disposal happens immediately, *optr is set to zero
  //   (i.e., _oplockptr is set to zero).  If it is deferred, _oplockptr
  //   remains set until the deferred disposal takes place.
  //
  //   Once disposal has occurred, ptr is changed either
  //   to zero or to some other value, thus an entry's user can compare
  //   ptr to '(&_oplockptr)' to ensure the entry still 'belongs' to it. 

  static CORBA::ULong cleanup();
  static void shutdown();

  // * cleanup returns the # of entries that could not be discarded (inuse > 0)

  // * shutdown disables further allocations and calls cleanup until
  //   there are no more entries, then it deletes the dummy entry.
  //   shutdown is only useful if there is a need
  //   to delete all allocated storage, e.g., for memory leak checking.

#ifndef NDEBUG
  inline static int ptr_exists(RDIOplockEntry* ptr) { return _dbg_ptrs.exists(ptr); }
#endif

private:
  static _nfy_attr TW_Mutex          _oplock;
  static _nfy_attr RDIOplockEntry*   _freelist;
  static _nfy_attr CORBA::Boolean    _shutdown;
#ifndef NDEBUG
  static _nfy_attr RDIOplocksDbgMap  _dbg_ptrs;
#endif
};

#ifndef NDEBUG
#define RDI_OPLOCKPTR_INVALID(ptr) (!RDIOplocks::ptr_exists(ptr))
#else
#define RDI_OPLOCKPTR_INVALID(ptr) (!ptr)
#endif

#endif
