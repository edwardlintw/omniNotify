// -*- Mode: C++; -*-
//                              File      : RDIOplocksMacros.h
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
 
#ifndef __RDI_OPLOCKS_MACROS_H__
#define __RDI_OPLCOKS_MACROS_H__

// --------------------------------------------------------------------------------------
//     HELPER MACROS
// --------------------------------------------------------------------------------------

// These macros can be used in a method implementation of any class
// with the following member:
//
//     RDIOplockEntry*   _oplockptr;
//
// This member should be initialized by the constructor using:
//
// RDI_OPLOCK_INIT("admin");  where "cadmin" is an example resty
// (resource type). 
//
// And it should be freed by establishing a bump scope and
// setting dispose_info.  Here is an example usage:
//
// #undef WHATFN
// #define WHATFN "some_class::disconnect_something"
// void some_class::disconnect_something(WRAPPED_IMPLARG_VOID)
// {
//    int held = 0;
//    RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held, WHATFN);
//    if (!held) { RDI_THROW_INV_OBJREF; }
//    ... cleanup work ...
//    // setting dispose_info forces free:
//    RDI_OPLOCK_SET_DISPOSE_INFO(proxy_lock);
// }
//
// The second argument to SCOPE_LOCK is a reference to a boolean to use to
// hold the current lock status for the requested lock.  It is set to 1
// when the lock is held, to 0 when the lock is not held.  
// IMPORTANT: the 'incoming' value of the held location is ignored.
// See the COND_ macros for conditional locking support.
//
// A bumped lock scope is a scope where _oplockptr is locked and
// _oplockptr->inuse has been incremented.  inuse is decremented
// and the lock is released on scope exit.  A bumped lock scope
// should be used whenever there is a subscope that releases
// the lock and/or a wait or timedwait call that releases the lock.
//
// One can use 
//  {
//    int proxy_lock_held = 0;
//    RDI_OPLOCK_SCOPE_LOCK_TRACK(proxy_lock, proxy_lock_held, WHATFN);
//    if (!proxy_lock_held) { RDI_THROW_INV_OBJREF; }
//    ...
//  }
// to establish a scope that holds the lock but does not bump inuse.
//
// All lock-acquire macros assume that if the entry referred to be _oplockptr
// is no longer 'owned' by 'this', or if _oplockptr->disposed is true, then
// the lock should not be acquired.
//
// Within a bump scope, one can introduce a subscope where the lock is
// temporarily released.  Because inuse has been incremented by the bump scope,
// the object may be marked as disposed, but it will not be destroyed until the
// outer bump scope exits.  On exit from the inner scope which releases the
// lock, the lock is reacquired and the following code should check whether
// _oplockptr->disposed is true.  The held location passed to the original scope
// lock should be passed to SCOPE_RELEASE_TRACK if you want to correctly update this
// location, otherwise another valid location can be used, where the value
// before the SCOPE_RELEASE should be 1.  If the held location is 0 after exit
// from the release scope, there was a reacquire failure.
//
// #undef WHATFN
// #define WHATFN "some_class::method"
// void some_class::method(WRAPPED_IMPLARG_VOID)
// {
//    int held = 0;
//    RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held, WHATFN);
//    if (!held) { RDI_THROW_INV_OBJREF; }
//    // use obj here, it is locked and bumped
//    if (test) throw exception; // any exit from this scope will debump and unlock
//    {
//        RDI_OPLOCK_SCOPE_RELEASE_TRACK(held, WHATFN);
//        // obj is unlocked here
//        ...
//    }
//    if (!held) {
//      ...handle reacquire failure here
//    }
//    // lock has been reacquired
//    if (_oplock->disposed) {
//        ... handle disposed case here ... 
//    }
// }
// // debump and lock release occurs on outer scope exit
//
// 
// All macros have a variant that ends in _OTHER that can be used over
// an object 'obj' other than this. 
//
// It is sometimes necessary to release a lock early without exiting the
// scope that was introduced for the lock.  Use RDI_OPLOCK_EARLY_RELEASE
// or RDI_OPLOCK_EARLY_RELEASE_OTHER for this purpose. These macros
// take the lock name that was passed to the SCOPE_LOCK as first argument,
// and they do not need to pass a held location because the location
// remains associated with the SCOPE_LOCK and is updated.
//
//  { // introduce outer lock scope
//    int chan_held = 0, admin_held = 0;
//    RDI_OPLOCK_SCOPE_LOCK_TRACK(channel_lock, chan_held, WHATFN);
//    if (!chan_held) { RDI_THROW_INV_OBJREF; }
//    ... do something with channel ....
//    { // introduce inner lock scope
//      RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(admin_lock, admin_held, admin_ptr, WHATFN);
//      if (!admin_held) { goto handle_admin_lock_error; }
//      ... do something while channel and admin are both locked ... 
//      RDI_OPLOCK_EARLY_RELEASE(channel_lock, WHATFN);
//      ... do something while admin is locked ...
//    } // end inner lock scope (admin is unlocked)
//  } // end outer lock scope (does nothing, channel lock released early)
// handle_admin_lock_error:
//   ... both channel and admin are unlocked here
//
// The following macros:
//    RDI_OPLOCK_COND_SCOPE_LOCK_TRACK
//    RDI_OPLOCK_COND_SCOPE_LOCK_OTHER_TRACK
//    RDI_OPLOCK_BUMP_COND_SCOPE_LOCK_TRACK
//    RDI_OPLOCK_BUMP_COND_SCOPE_LOCK_OTHER_TRACK
//
// are used for the case where a lock may or may not already be held.  The
// current lock status is taken from the incoming value of the held location.
// If the lock is not held, the COND macros behaves exactly like the SCOPE_LOCK
// macros (acquire the lock when called, releases it on scope exit).  If the
// lock is already held, then the COND macros do not change the lock status
// (they simply leave the lock in the locked state, and do not release it on
// scope exit).  The BUMP versions always do a bump/debump pair.  Thus, the lock
// status prior to a COND macro use will be the same as the lock status on scope
// exit.  To make it simple to maintain this invariant, you cannot use
// SCOPE_RELEASE or EARLY_RELEASE in combination with one of the COND macros.
// You also cannot use SET_DISPOSE_INFO in combination with one of the COND macros.
//
// Finally, the non-COND lock-acquire macros have a non-track variant that does
// not take a held location but does take a final argument with exceptional code
// to be executed if lock acquisition fails.  Example:
//
//  {
//    RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
//    ...
//  }
//

////////////////////////////////////////////////////////////////////////////////
// MACROS
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// Initialization / Disposal Macros
//    ** For dispose macros, _oplockptr->oplock MUST be held
///////////////////////////////////////////////////////////////

#define RDI_OPLOCK_INIT(resty) \
  do {_oplockptr = RDIOplocks::alloc_entry(&_oplockptr, &_my_name, resty); \
           RDI_AssertAllocThrowNo(_oplockptr, "Failed to allocate RDIOplockEntry"); } while (0)

#define RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info) \
  do { dispose_info = WRAPPED_IMPL2DISPOSEINFO(this); } while (0)

////////////////////////////////////////////////////////
// Macros that can be used to handle exceptional cases
////////////////////////////////////////////////////////

#define RDI_THROW_INV_OBJREF \
  throw CORBA::INV_OBJREF(0, CORBA::COMPLETED_NO)

#define RDI_THREAD_EXIT(label, flags, flags_nm, desc) \
  do { RDIDbgLog(flags, flags_nm, \
                 desc << " in obj " << (void*)this \
		 << " : thread " << TW_ID() << " exits\n"); \
       goto label; \
  } while (0)

#define RDI_METHOD_RETURN(flags, flags_nm, desc, val) \
  do { RDIDbgLog(flags, flags_nm, desc << " in obj " << (void*)this << " : method return\n"); return val; } while (0)

#define RDI_METHOD_RETURN_NULL(flags, flags_nm, desc) \
  do { RDIDbgLog(flags, flags_nm, desc << " in obj " << (void*)this << " : method return\n"); return; } while (0)

////////////////////////////////////////////////////////////////////////////////////
// DESTROY_CHECK verifies that one of these conditions holds
//    1. _oplockptr is zero (no longer associated with an entry)
//    2. _oplockptr->ptr() is zero (the entry has been freed)
//    3. _oplockptr->ptr() is not equal to this (the entry freed + reassigned)
////////////////////////////////////////////////////////////////////////////////////

#define RDI_OPLOCK_DESTROY_CHECK(desc) \
  do { if ( _oplockptr && _oplockptr->ptr() && (_oplockptr->ptr() == (&_oplockptr) ) ) { \
    RDIDbgForceLog("** Internal error: RDI_OPLOCK_DESTROY_CHECK : " << desc << " " << (void*)this << \
		   " allocated OplockEntry has not been freed properly\n"); \
  } } while (0)

////////////////////////////////////////////////////////////////////////////////////
// CONDITION VARIABLE Macros
//
// The entry has a condition variable associated with oplock, and
// the following macros do the obvious calls on this variable.
// ** For SIGNAL and BROADCAST,
//      _oplockptr->oplock should be held to ensure entry is owned by 'this'
// ** For WAIT and TIMEDWAIT,
//      _oplockptr->oplock must be held, and
//        these macros should be performed in a bumped scope lock.
////////////////////////////////////////////////////////////////////////////////////

#define RDI_OPLOCK_SIGNAL \
  _oplockptr->signal()

#define RDI_OPLOCK_BROADCAST \
  _oplockptr->broadcast()

#ifndef TW_DEBUG
/////////////////////////////////////////////////////
// Non-debug mutex macros
/////////////////////////////////////////////////////

#define RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(nm, held, whatfn) \
  RDI_OplockCondLock nm(held, &_oplockptr)

#define RDI_OPLOCK_BUMP_COND_SCOPE_LOCK_TRACK(nm, held, whatfn) \
  RDI_OplockBumpCondLock nm(held, &_oplockptr)

#define RDI_OPLOCK_SCOPE_LOCK_TRACK(nm, held, whatfn) \
  RDI_OplockLock nm(held, &_oplockptr)

#define RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(nm, held, whatfn) \
  RDI_OplockBumpLock nm(held, &_oplockptr)

#define RDI_OPLOCK_SCOPE_RELEASE_TRACK(held, whatfn) \
  RDI_OplockTempRelease _oplock_release(held, &_oplockptr)

#define RDI_OPLOCK_EARLY_RELEASE(nm, whatfn) \
  nm.early_release()

#define RDI_OPLOCK_COND_SCOPE_LOCK_OTHER_TRACK(nm, held, obj, whatfn) \
  RDI_OplockCondLock nm(held, (obj)->macro_get_oplockptr_ptr())

#define RDI_OPLOCK_BUMP_COND_SCOPE_LOCK_OTHER_TRACK(nm, held, obj, whatfn) \
  RDI_OplockBumpCondLock nm(held, (obj)->macro_get_oplockptr_ptr())

#define RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(nm, held, obj, whatfn) \
  RDI_OplockLock nm(held, (obj)->macro_get_oplockptr_ptr())

#define RDI_OPLOCK_BUMP_SCOPE_LOCK_OTHER_TRACK(nm, held, obj, whatfn) \
  RDI_OplockBumpLock nm(held, (obj)->macro_get_oplockptr_ptr())

#define RDI_OPLOCK_SCOPE_RELEASE_OTHER_TRACK(held, obj, whatfn) \
  RDI_OplockTempRelease _oplock_release(held, (obj)->macro_get_oplockptr_ptr())

#define RDI_OPLOCK_EARLY_RELEASE_OTHER(nm, obj, whatfn) \
  nm.early_release()

#define RDI_OPLOCK_WAIT(whatfn) \
  _oplockptr->wait()

#define RDI_OPLOCK_TIMEDWAIT(s, n, whatfn) \
  _oplockptr->timedwait(s, n)

#define RDI_OPLOCK_INUSEONE_WAIT(whatfn) \
  _oplockptr->inuseone_wait()

#define RDI_OPLOCK_INUSEONE_TIMEDWAIT(s, n, whatfn) \
  _oplockptr->inuseone_timedwait(s, n)

#define RDI_OPLOCK_ALTCV_WAIT(cv, whatfn) \
  _oplockptr->altcv_wait(cv)

#define RDI_OPLOCK_ALTCV_TIMEDWAIT(cv, s, n, whatfn) \
  _oplockptr->altcv_timedwait(cv, s, n)

#else
/////////////////////////////////////////////////////
// Debug mutex macros
/////////////////////////////////////////////////////

#define RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(nm, held, whatfn) \
  RDI_DBG_OplockCondLock nm(held, &_oplockptr, whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_BUMP_COND_SCOPE_LOCK_TRACK(nm, held, whatfn) \
  RDI_DBG_OplockBumpCondLock nm(held, &_oplockptr, whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_SCOPE_LOCK_TRACK(nm, held, whatfn) \
  RDI_DBG_OplockLock nm(held, &_oplockptr, whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(nm, held, whatfn) \
  RDI_DBG_OplockBumpLock nm(held, &_oplockptr, whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_SCOPE_RELEASE_TRACK(held, whatfn) \
  RDI_DBG_OplockTempRelease _oplock_release(held, &_oplockptr, whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_EARLY_RELEASE(nm, whatfn) \
  nm.early_release(whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_COND_SCOPE_LOCK_OTHER_TRACK(nm, held, obj, whatfn) \
  RDI_DBG_OplockCondLock nm(held, (obj)->macro_get_oplockptr_ptr(), whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_BUMP_COND_SCOPE_LOCK_OTHER_TRACK(nm, held, obj, whatfn) \
  RDI_DBG_OplockBumpCondLock nm(held, (obj)->macro_get_oplockptr_ptr(), whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(nm, held, obj, whatfn) \
  RDI_DBG_OplockLock nm(held, (obj)->macro_get_oplockptr_ptr(), whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_BUMP_SCOPE_LOCK_OTHER_TRACK(nm, held, obj, whatfn) \
  RDI_DBG_OplockBumpLock nm(held, (obj)->macro_get_oplockptr_ptr(), whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_SCOPE_RELEASE_OTHER_TRACK(held, obj, whatfn) \
  RDI_DBG_OplockTempRelease _oplock_release(held, (obj)->macro_get_oplockptr_ptr(), whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_EARLY_RELEASE_OTHER(nm, obj, whatfn) \
  nm.early_release(whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_WAIT(whatfn) \
  _oplockptr->dbg_wait(whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_TIMEDWAIT(s, n, whatfn) \
  _oplockptr->dbg_timedwait(s, n, whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_INUSEONE_WAIT(whatfn) \
  _oplockptr->dbg_inuseone_wait(whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_INUSEONE_TIMEDWAIT(s, n, whatfn) \
  _oplockptr->dbg_inuseone_timedwait(s, n, whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_ALTCV_WAIT(cv, whatfn) \
  _oplockptr->dbg_altcv_wait(cv, whatfn, __FILE__, __LINE__)

#define RDI_OPLOCK_ALTCV_TIMEDWAIT(cv, s, n, whatfn) \
  _oplockptr->dbg_altcv_timedwait(cv, s, n, whatfn, __FILE__, __LINE__)

#endif  /* !TW_DEBUG */

/////////////////////////////////////////////////////
// Generic macros
//   These just define non-track cases in terms
//   of track cases, so they work for both debug
//   and non-debug cases.
/////////////////////////////////////////////////////

#define RDI_OPLOCK_SCOPE_LOCK(nm, whatfn, ex_code) \
  int tmp_ ## nm ## _held = 0; \
  RDI_OPLOCK_SCOPE_LOCK_TRACK(nm, tmp_ ## nm ## _held, whatfn); \
  if (! tmp_ ## nm ## _held) { ex_code; }

#define RDI_OPLOCK_BUMP_SCOPE_LOCK(nm, whatfn, ex_code) \
  int tmp_ ## nm ## _held = 0; \
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(nm, tmp_ ## nm ## _held, whatfn); \
  if (! tmp_ ## nm ## _held) { ex_code; }

#define RDI_OPLOCK_SCOPE_LOCK_OTHER(nm, obj, whatfn, ex_code) \
  int tmp_ ## nm ## _held = 0; \
  RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(nm, tmp_ ## nm ## _held, obj, whatfn); \
  if (! tmp_ ## nm ## _held) { ex_code; }

#define RDI_OPLOCK_BUMP_SCOPE_LOCK_OTHER(nm, obj, whatfn, ex_code) \
  int tmp_ ## nm ## _held = 0; \
  RDI_OPLOCK_BUMP_SCOPE_LOCK_OTHER_TRACK(nm, tmp_ ## nm ## _held, obj, whatfn); \
  if (! tmp_ ## nm ## _held) { ex_code; }


//////////////////////////////////////////////////////////////////////////////////////////
// HELPER CLASSES
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef TW_DEBUG
/////////////////////////////////////////////////////
// Non-debug helper classes
/////////////////////////////////////////////////////

// conditional scope lock, cannot be used for disposal
class RDI_OplockCondLock {
  RDIOplockEntry*   _oplockptr;
  RDIOplockEntry**  _oplockptr_ptr;
  int&              _held;
  int               _initial_held;
public:
  RDI_OplockCondLock(int& held, RDIOplockEntry **ptr_ptr)
    : _oplockptr(*ptr_ptr), _oplockptr_ptr(ptr_ptr), _held(held), _initial_held(held)
  {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { /* we are in trouble */
      _held = 0;
    } else {
      if (!_held && _oplockptr->acquire(ptr_ptr)) {
	_held = 1;
      }
    }
  }
  ~RDI_OplockCondLock(void) {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { /* we are in trouble */
      _held = 0;
    } else {
      // attempt to restore state to _initial_held state
      if (_held && !_initial_held) {
	_oplockptr->release();
	_held = 0;
      }
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  RDI_OplockCondLock(const RDI_OplockCondLock&);
  RDI_OplockCondLock& operator=(const RDI_OplockCondLock&);
};

// always bump, conditional scope lock, cannot be used for disposal
class RDI_OplockBumpCondLock {
  RDIOplockEntry*   _oplockptr;
  RDIOplockEntry**  _oplockptr_ptr;
  int&              _held;
  int               _initial_held;
public:
  RDI_OplockBumpCondLock(int& held, RDIOplockEntry **ptr_ptr)
    : _oplockptr(*ptr_ptr), _oplockptr_ptr(ptr_ptr), _held(held), _initial_held(held)
  {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { /* we are in trouble */
      _held = 0;
    } else {
      if (_held) {
	_oplockptr->bump();
      } else if (_oplockptr->acquire(ptr_ptr)) {
	_held = 1;
	_oplockptr->bump();
      }
    }
  }
  ~RDI_OplockBumpCondLock(void) {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { /* we are in trouble */
      _held = 0;
    } else {
      if (_held) {
	_oplockptr->debump();
	// attempt to restore state to _initial_held state
	if (!_initial_held) {
	  _oplockptr->release();
	  _held = 0;
	}
      }    
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  RDI_OplockBumpCondLock(const RDI_OplockBumpCondLock&);
  RDI_OplockBumpCondLock& operator=(const RDI_OplockBumpCondLock&);
};

class RDI_OplockLock {
  RDIOplockEntry*   _oplockptr;
  RDIOplockEntry**  _oplockptr_ptr;
  int&              _held;
public:
  // set this field to WRAPPED_IMPL2DISPOSEINFO(this) to cause disposal
  // on exit from scope or on early release
  WRAPPED_DISPOSEINFO_PTR dispose_info;

  RDI_OplockLock(int& held, RDIOplockEntry** ptr_ptr)
    : _oplockptr(*ptr_ptr), _oplockptr_ptr(ptr_ptr), _held(held), dispose_info(0)
  {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { /* we are in trouble */
      _held = 0;
    } else {
      _held = (_oplockptr->acquire(ptr_ptr)) ? 1 : 0;
    }
  }
  ~RDI_OplockLock(void) {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { /* we are in trouble */
      _held = 0;
    } else {
      if (_held) {
	if (dispose_info) {
	  RDIOplocks::free_entry(_oplockptr, _oplockptr_ptr, dispose_info); \
	} else {
	  _oplockptr->release();
	}
	_held = 0;
      }
    }
  }
  void early_release() {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { /* we are in trouble */
      _held = 0;
    } else {
      if (_held) {
	if (dispose_info) {
	  RDIOplocks::free_entry(_oplockptr, _oplockptr_ptr, dispose_info); \
	} else {
	  _oplockptr->release();
	}
	_held = 0;
      }
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  RDI_OplockLock(const RDI_OplockLock&);
  RDI_OplockLock& operator=(const RDI_OplockLock&);
};

class RDI_OplockBumpLock {
  RDIOplockEntry*   _oplockptr;
  RDIOplockEntry**  _oplockptr_ptr;
  int&              _held;
public:
  // set this field to WRAPPED_IMPL2DISPOSEINFO(this) to cause disposal
  // on exit from scope or on early release:
  WRAPPED_DISPOSEINFO_PTR dispose_info;

  RDI_OplockBumpLock(int& held, RDIOplockEntry** ptr_ptr)
    : _oplockptr(*ptr_ptr), _oplockptr_ptr(ptr_ptr), _held(held), dispose_info(0)
  {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { /* we are in trouble */
    _held = 0; 
    } else {
      _held = 0; 
      if (_oplockptr->acquire(ptr_ptr)) {
	_held = 1;
	_oplockptr->bump();
      }
    }
  }
  ~RDI_OplockBumpLock(void) {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { /* we are in trouble */
      _held = 0;
    } else {
      if (_held) {
	_oplockptr->debump();
	if (dispose_info) {
	  RDIOplocks::free_entry(_oplockptr, _oplockptr_ptr, dispose_info); \
	} else {
	  _oplockptr->release();
	}
	_held = 0;
      }
    }
  }
  void early_release() {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { /* we are in trouble */
      _held = 0;
    } else {
      if (_held) {
	_oplockptr->debump();
	if (dispose_info) {
	  RDIOplocks::free_entry(_oplockptr, _oplockptr_ptr, dispose_info); \
	} else {
	  _oplockptr->release();
	}
	_held = 0;
      }
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  RDI_OplockBumpLock(const RDI_OplockBumpLock&);
  RDI_OplockBumpLock& operator=(const RDI_OplockBumpLock&);
};

class RDI_OplockTempRelease {
  RDIOplockEntry*    _oplockptr;
  RDIOplockEntry**   _oplockptr_ptr;
  int&               _held;
public:
  RDI_OplockTempRelease(int& held, RDIOplockEntry** ptr_ptr) :
    _oplockptr(*ptr_ptr), _oplockptr_ptr(ptr_ptr), _held(held) {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { // we are in trouble
      _held = 0;
    } else {
      _oplockptr->release();
      _held = 0;
    }
  }
  ~RDI_OplockTempRelease(void) {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { // we are in trouble
      _held = 0;
    } else {
      _held = (_oplockptr->reacquire(_oplockptr_ptr)) ? 1 : 0;
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  RDI_OplockTempRelease(const RDI_OplockTempRelease&);
  RDI_OplockTempRelease& operator=(const RDI_OplockTempRelease&);
};

#else
/////////////////////////////////////////////////////
// Debug helper classes
/////////////////////////////////////////////////////

#define RDI_WARN_OPLOCKPTR_INVALID(fnIN, oplockptrIN, whatfnIN, fileIN, lineIN) \
  TW_DbgLog("XXX_WARNING!!! Invalid Oplockptr Accessed by " << fnIN << " oplockptr " << (void*)oplockptrIN \
        << " in " << whatfnIN << " " << fileIN << ":" << lineIN << "\n")

// conditional scope lock, cannot be used for disposal
class RDI_DBG_OplockCondLock {
  RDIOplockEntry*   _oplockptr;
  RDIOplockEntry**  _oplockptr_ptr;
  int&              _held;
  int               _initial_held;
  const char*       _whatfn;
  const char*       _file;
  int               _line;
public:
  RDI_DBG_OplockCondLock(int& held, RDIOplockEntry** ptr_ptr,
			 const char* whatfn, const char* file, int line) :
    _oplockptr(*ptr_ptr), _oplockptr_ptr(ptr_ptr), _held(held), _initial_held(held),
    _whatfn(whatfn), _file(file), _line(line)
  {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { // we are in trouble
      RDI_WARN_OPLOCKPTR_INVALID("RDI_DBG_OplockCondLock::RDI_DBG_OplockCondLock", _oplockptr, _whatfn, _file, _line);
      _held = 0;
    } else {
      if (!_held) {
	TW_ABOUT_TO_LOCK(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
	if (_oplockptr->acquire(ptr_ptr)) {
	  TW_JUST_LOCKED(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
	  _held = 1;
	}
      }
    }
  }
  ~RDI_DBG_OplockCondLock(void) {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { // we are in trouble
      RDI_WARN_OPLOCKPTR_INVALID("RDI_DBG_OplockCondLock::~RDI_DBG_OplockCondLock", _oplockptr, _whatfn, _file, _line);
      _held = 0;
    } else {
      // attempt to restore lock state to _intial_held state
      if (_held && !_initial_held) {
	TW_ABOUT_TO_UNLOCK(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
	_oplockptr->release();
	_held = 0;
      }
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  RDI_DBG_OplockCondLock(const RDI_DBG_OplockCondLock&);
  RDI_DBG_OplockCondLock& operator=(const RDI_DBG_OplockCondLock&);
};

// always bump, conditional scope lock, cannot be used for disposal
class RDI_DBG_OplockBumpCondLock {
  RDIOplockEntry*   _oplockptr;
  RDIOplockEntry**  _oplockptr_ptr;
  int&              _held;
  int               _initial_held;
  const char*       _whatfn;
  const char*       _file;
  int               _line;
public:
  RDI_DBG_OplockBumpCondLock(int& held, RDIOplockEntry** ptr_ptr,
			     const char* whatfn, const char* file, int line) :
    _oplockptr(*ptr_ptr), _oplockptr_ptr(ptr_ptr), _held(held), _initial_held(held),
    _whatfn(whatfn), _file(file), _line(line)
  {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { // we are in trouble
      RDI_WARN_OPLOCKPTR_INVALID("RDI_DBG_OplockBumpCondLock::RDI_DBG_OplockBumpCondLock", _oplockptr, _whatfn, _file, _line);
      _held = 0;
    } else {
      if (_held) {
	_oplockptr->bump();
      } else {
	TW_ABOUT_TO_LOCK(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
	if (_oplockptr->acquire(ptr_ptr)) {
	  TW_JUST_LOCKED(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
	  _held = 1;
	  _oplockptr->bump();
	}
      }
    }
  }
  ~RDI_DBG_OplockBumpCondLock(void) {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { // we are in trouble
      RDI_WARN_OPLOCKPTR_INVALID("RDI_DBG_OplockBumpCondLock::~RDI_DBG_OplockBumpCondLock", _oplockptr, _whatfn, _file, _line);
      _held = 0;
    } else {
      if (_held) {
	_oplockptr->debump();
	// attempt to restore lock state to _intial_held state
	if (!_initial_held) {
	  TW_ABOUT_TO_UNLOCK(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
	  _oplockptr->release();
	  _held = 0;
	}
      }
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  RDI_DBG_OplockBumpCondLock(const RDI_DBG_OplockBumpCondLock&);
  RDI_DBG_OplockBumpCondLock& operator=(const RDI_DBG_OplockBumpCondLock&);
};

class RDI_DBG_OplockLock {
  RDIOplockEntry*   _oplockptr;
  RDIOplockEntry**  _oplockptr_ptr;
  int&              _held;
  const char*       _whatfn;
  const char*       _file;
  int               _line;
public:
  // set this field to WRAPPED_IMPL2DISPOSEINFO(this) to cause disposal
  // on exit from scope or on early release
  WRAPPED_DISPOSEINFO_PTR dispose_info;

  RDI_DBG_OplockLock(int& held, RDIOplockEntry** ptr_ptr,
		     const char* whatfn, const char* file, int line) :
    _oplockptr(*ptr_ptr), _oplockptr_ptr(ptr_ptr), _held(held),
    _whatfn(whatfn), _file(file), _line(line),
    dispose_info(0) {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { // we are in trouble
      RDI_WARN_OPLOCKPTR_INVALID("RDI_DBG_OplockLock::RDI_DBG_OplockLock", _oplockptr, _whatfn, _file, _line);
      _held = 0;
    } else {
      _held = 0;
      if (_oplockptr) {
	TW_ABOUT_TO_LOCK(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
	if (_oplockptr->acquire(ptr_ptr)) {
	  TW_JUST_LOCKED(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
	  _held = 1;
	}
      }
    }
  }
  ~RDI_DBG_OplockLock(void) {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { // we are in trouble
      RDI_WARN_OPLOCKPTR_INVALID("RDI_DBG_OplockLock::~RDI_DBG_OplockLock", _oplockptr, _whatfn, _file, _line);
      _held = 0;
    } else {
      if (_held) {
	TW_ABOUT_TO_UNLOCK(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
	if (dispose_info) {
	  RDIOplocks::free_entry(_oplockptr, _oplockptr_ptr, dispose_info); \
	} else {
	  _oplockptr->release();
	}
	_held = 0;
      }
    }
  }
  void early_release() {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { // we are in trouble
      RDI_WARN_OPLOCKPTR_INVALID("RDI_DBG_OplockLock::early_release", _oplockptr, _whatfn, _file, _line);
      _held = 0;
    } else {
      if (_held) {
	TW_ABOUT_TO_EARLY_UNLOCK(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
	if (dispose_info) {
	  RDIOplocks::free_entry(_oplockptr, _oplockptr_ptr, dispose_info); \
	} else {
	  _oplockptr->release();
	}
	_held = 0;
      }
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  RDI_DBG_OplockLock(const RDI_DBG_OplockLock&);
  RDI_DBG_OplockLock& operator=(const RDI_DBG_OplockLock&);
};

class RDI_DBG_OplockBumpLock {
  RDIOplockEntry*   _oplockptr;
  RDIOplockEntry**  _oplockptr_ptr;
  int&              _held;
  const char*       _whatfn;
  const char*       _file;
  int               _line;
public:
  // set this field to WRAPPED_IMPL2DISPOSEINFO(this) to cause disposal
  // on exit from scope or on early release:
  WRAPPED_DISPOSEINFO_PTR dispose_info;

  RDI_DBG_OplockBumpLock(int& held, RDIOplockEntry** ptr_ptr,
			 const char* whatfn, const char* file, int line) :
    _oplockptr(*ptr_ptr), _oplockptr_ptr(ptr_ptr), _held(held),
    _whatfn(whatfn), _file(file), _line(line),
    dispose_info(0)
  {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { // we are in trouble
      RDI_WARN_OPLOCKPTR_INVALID("RDI_DBG_OplockBumpLock::RDI_DBG_OplockBumpLock", _oplockptr, _whatfn, _file, _line);
    _held = 0;
    } else {
      _held = 0;
      if (_oplockptr) {
	TW_ABOUT_TO_LOCK(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
	if (_oplockptr->acquire(ptr_ptr)) {
	  TW_JUST_LOCKED(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
	  _held = 1;
	  _oplockptr->bump();
	}
      }
    }
  }
  ~RDI_DBG_OplockBumpLock(void) {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { // we are in trouble
      RDI_WARN_OPLOCKPTR_INVALID("RDI_DBG_OplockBumpLock::~RDI_DBG_OplockBumpLock", _oplockptr, _whatfn, _file, _line);
      _held = 0;
    } else {
      if (_held) {
	TW_ABOUT_TO_UNLOCK(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
	_oplockptr->debump();
	if (dispose_info) {
	  RDIOplocks::free_entry(_oplockptr, _oplockptr_ptr, dispose_info); \
	} else {
	  _oplockptr->release();
	}
	_held = 0;
      }
    }
  }
  void early_release() {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { // we are in trouble
      RDI_WARN_OPLOCKPTR_INVALID("RDI_DBG_OplockBumpLock::early_release", _oplockptr, _whatfn, _file, _line);
      _held = 0;
    } else {
      if (_held) {
	TW_ABOUT_TO_EARLY_UNLOCK(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
	_oplockptr->debump();
	if (dispose_info) {
	  RDIOplocks::free_entry(_oplockptr, _oplockptr_ptr, dispose_info); \
	} else {
	  _oplockptr->release();
	}
	_held = 0;
      }
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  RDI_DBG_OplockBumpLock(const RDI_DBG_OplockBumpLock&);
  RDI_DBG_OplockBumpLock& operator=(const RDI_DBG_OplockBumpLock&);
};

class RDI_DBG_OplockTempRelease {
  RDIOplockEntry*    _oplockptr;
  RDIOplockEntry**   _oplockptr_ptr;
  int&               _held;
  const char*        _whatfn;
  const char*        _file;
  int                _line;
public:
  RDI_DBG_OplockTempRelease(int& held, RDIOplockEntry** ptr_ptr,
			    const char* whatfn, const char* file, int line) :
    _oplockptr(*ptr_ptr), _oplockptr_ptr(ptr_ptr), _held(held),
    _whatfn(whatfn), _file(file), _line(line)
  {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { // we are in trouble
      RDI_WARN_OPLOCKPTR_INVALID("RDI_DBG_OplockTempRelease::RDI_DBG_OplockTempRelease", _oplockptr, _whatfn, _file, _line);
      _held = 0;
    } else {
      TW_ABOUT_TO_UNLOCK(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
      _oplockptr->release();
      _held = 0;
    }
  }
  ~RDI_DBG_OplockTempRelease(void) {
    if (RDI_OPLOCKPTR_INVALID(_oplockptr)) { // we are in trouble
      RDI_WARN_OPLOCKPTR_INVALID("RDI_DBG_OplockTempRelease::~RDI_DBG_OplockTempRelease", _oplockptr, _whatfn, _file, _line);
      _held = 0;
    } else {
      TW_ABOUT_TO_LOCK(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
      _held = (_oplockptr->reacquire(_oplockptr_ptr)) ? 1 : 0;
      if (_held) {
	TW_JUST_LOCKED(_oplockptr->macro_get_mutex_ptr(), 0, _oplockptr->macro_get_resty(), _whatfn, _file, _line);
      }
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  RDI_DBG_OplockTempRelease(const RDI_DBG_OplockTempRelease&);
  RDI_DBG_OplockTempRelease& operator=(const RDI_DBG_OplockTempRelease&);
};

#endif  /*  !TW_DEBUG  */

#endif  /*  !__RDI_OPLOCKS_MACROS_H__  */
