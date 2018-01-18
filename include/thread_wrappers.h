// -*- Mode: C++; -*-
//                              File      : thread_wrappers.h
//                              Package   : omniNotify-Library
//                              Created on: 11-Dec-2001
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
//    Wrappers for threads, mutexes, and condition variables.
//
 
////////////////////////////////////////////////////////////////////////////////////
// Summary of Wrapped Functionality
//
// Note: namespace TW_ (Thread Wrappers) is used for all names
//
// Thread Priorities
// -----------------
// The thread priority type:
//   TW_PRIORITY_T
//
// Three priorities are defined; for the moment only NORMAL is used
//    TW_PRIORITY_LOW
//    TW_PRIORITY_NORMAL
//    TW_PRIORITY_HIGH
//
// 3 Main Classes
// --------------
// The three main classes borrow their specs from the omnithread API.  See
// omnithread.h for the full APIs.
//
// class TW_Thread    has the same API as omni_thread
// class TW_Mutex     has the same API as omni_mutex 
// class TW_Condition has the same API as omni_condition
// 
// In addition, class TW_RWMutex is like TW_Mutex
// but provides read-writer synchronization.  
//
// class TW_RWMutex {
//   TW_RWMutex();
//  ~TW_RWMutex();
//   void lock(int readlock);
//   void unlock();
//   void weaken();
//   void acquire(int readlock);
//   void release(void);
// }
//
// Global Functions or Macros
// --------------------------
// void TW_ID()
//   => return id of current thread (type unsigned long)
//
// void TW_EXIT(void* val)
//   => causes the calling thread to terminate
//
// void TW_YIELD()
//   => allows another thread to run.
//
// void TW_SLEEP(unsigned long secs, unsigned long nanosecs = 0);
//   => sleeps for the given time
//
// void TW_GET_TIME(unsigned long* s, unsigned long*  n,
//                  unsigned long  ds = 0, unsigned long  dn = 0)
//     // set s, n (seconds, nanoseconds posix time)
//     // to local clock time plus the specified delta (ds, dn)
//
// ==========================
// CONDITION AND MUTEX MACROS
// ==========================
//
// The following macros incorporate some debugging behavior if
// TW_DEBUG is set.  To allow for useful debug logging, two
// special arguments are present:
//
//      whatfn:  what function is calling the wrapper
//
//      resty:   shorthand name for the type of resource
//               that is protected by the lock
//
// If TW_DEBUG is not set, these extra arguments are simply ignored.
//
// When TW_DEBUG is set, then by default, debug info is output to
// stderr.  Function TW::DbgLog can be used to direct output
// elsewhere.  It takes a string as first arg, and an optional mode as
// second arg.  Examples:
//   TW::DbgLog("stdout");
//   TW::DbgLog("thread_dbg.log");
//   TW::DbgLog("thread_dbg.log", "a+");
// The default file mode is "w".
//
// -------------------------
// Condition Variable Macros
// -------------------------
//
// TW_CV_WAIT(cv, mutex, resty, whatfn);
//   Wait on a condition variable associated with mutex.
//   What not debugging, is equivalent to:
//       cv.wait();
//
// TW_CV_TIMEDWAIT(cv, secs, nanosecs, mutex, resty, whatfn);
//   Timed wait on a condition variable associated with mutex.
//   What not debugging, is equivalent to:
//       cv.timedwait(secs, nanosecs);
//
// TW_CV_WAIT_RW(cv, mutex, readlock, resty, whatfn);
// TW_CV_TIMEDWAIT_RW(cv, secs, nanosecs, mutex, readlock, resty, whatfn);
//    As above, but used for RW mutex.  The readlock argument indicates
//    whether mutex is currently held in read mode (0 means write mode).
//    When not debugging, these macros are equivalent to:
//       cv.wait();
//       cv.timedwait(secs, nanosecs);
//    respectively.
// 
// ------------
// Mutex Macros
// ------------
//
// TW_SCOPE_LOCK(nm, mutex, resty, whatfn);
//   Construct a helper object on the stack with name nm which locks mutex; 
//   on exit from the current scope, unlocks mutex.
// 
// Thus, as an alternative to:
// {
//   mutex.lock();
//   .....
//   mutex.unlock();
// }
//
// One would write, e.g..:
//
// {
//   TW_SCOPE_LOCK(mutex_lock, mutex, "server", "main");
//   ...mutex is locked for rest of scope
// }
//
// TW_SCOPE_RELEASE(nm);
//    Construct a helper object on the stack that unlocks the mutex
//    managed by helper object named nm (as introduced by TW_SCOPE_LOCK)
//    and relocks the mutex on exit from the current scope.
//    Example:
// {
//   TW_SCOPE_LOCK(mutex_lock, mutex, "server", "main");
//   ... mutex is locked
//    {
//      TW_SCOPE_RELEASE(mutex_lock);
//      ... mutex is not locked for rest of inner scope
//     }
//   ... mutex is now locked again
// }
//
// TW_EARLY_RELEASE(nm);
//   Release the mutex lock on the mutex managed by helper object named nm
//   (as introduced by TW_SCOPE_LOCK) now, as opposed to waiting for the
//   end of the scope that contains the TW_SCOPE_LOCK.  When TW_EARLY_RELEASE
//   is used, no action occurs on the exit from the scope containg the
//   TW_SCOPE_LOCK, since the lock has already been released.  Example:
//
// { // introduce outer lock scope
//   TW_SCOPE_LOCK(chanlock, chan_mutex, "channel", "main");
//   ...do something with channel ...
//   { // introduce inner lock scope
//     TW_SCOPE_LOCK(adminlock, admin_mutex, "admin", "main");
//     // now that we have admin lock, release channel lock
//     TW_EARLY_RELEASE(chanlock);
//     ... do something with admin ...
//   } // end inner lock scope (releases admin lock)
// } // end outer lock scope (does nothing, channel lock was released early)
//
//
// TW_SCOPE_LOCK_RW(nm, mutex, readlock, resty, whatfn);
// TW_SCOPE_RELEASE_RW(nm, readlock);
// TW_EARLY_RELEASE_RW(nm);
//
// The above are used for a RW mutx.  For SCOPE_LOCK_RW, readlock
// indicates whether a read lock is acquired (0 means write lock).
// For SCOPE_RELEASE_RW, nm should be the name used when a scope lock
// was acquired on mutex in a containing scope, and readlock indicates
// whether a read lock is acquired on exit from the temporary release
// scope (0 means write lock).
//
// Finally, in some cases one wants to update a boolean tracking lock state.
// The macros TW_SCOPE_LOCK_TRACK and TW_SCOPE_LOCK_RW_TRACK
// are like their non-track counterparts except they take a 'held' location
// that is set to 1 when the lock is held and 0 when it is not.
//
// Example:
//
// CORBA::Boolean chan_held = 0, admin_held = 0;
// { // introduce outer lock scope
//   TW_SCOPE_LOCK_TRACK(chanlock, chan_held, chan_mutex, "channel", WHATFN);
//   ...do something with channel ...
//   { // introduce inner lock scope
//     TW_SCOPE_LOCK_TRACK(adminlock, admin_held, admin_mutex, "admin", WHATFN);
//     // now that we have admin lock, release channel lock
//     TW_EARLY_RELEASE(chanlock);
//     ... do something with admin ...
//     ... here, chan_held should be 0, admin_held should be 1
//   } // end inner lock scope (releases admin lock)
// } // end outer lock scope (does nothing, channel lock was released early)
//
// The RW version of the TRACK macro set held to 0 when not held, to 1 when
// held in read mode, and to 2 when held in write mode.
//
// These tracking macros normally ignore the initial state of the held
// location and simply set its value.  TW_COND_SCOPE_LOCK_TRACK and
// TW_COND_SCOPE_LOCK_RW_TRACK are used for the case where the lock
// may or may not already be held.  If the initial value of held is
// zero, the COND macros behave like the non-cond macros (obtain lock,
// release on scope exit).  If the initial held value is non-zero, the
// COND macros do nothing, they simply leave the lock in its current
// state.  For the RW case, if held is set to 1 (read-locked), then
// the readlock flag must be 1; if held is set to 2 (write-locked),
// the readlock flag must be 0.  In other words, if the lock is
// already held, you must request a read/write mode that is identical
// to the current read/write mode.  An abort occurs if this constraint
// is violated, so be very careful when using COND for the RW case.
// This constraint is neccesary to maintain the following invariant:
// the COND macros leave the lock in the same state on scope exit as
// at the time the macro is used.  To make is simple to enforce this
// invariant, you cannot use SCOPE_RELEASE, EARLY_RELEASE, or
// WEAKEN in combination with one of the COND macros.
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef __THREAD_WRAPPERS_H__
#define __THREAD_WRAPPERS_H__

// More ORBs could be supported.  For now we just support OMNIORB3/4 and MICO
#if !defined(__OMNIORB3__) && !defined(__OMNIORB4__) && !defined(__MICO__)
#  error "One of the following must be defined : __OMNIORB3__, __OMNIORB4__, __MICO__"
stop_compiling
#endif

#if defined(__OMNIORB3__) || defined(__OMNIORB4__)
#include "omnithread_thread_wrappers.h"
#endif

#if defined(__MICO__)
#include "micothread_thread_wrappers.h"
#endif

#include <stdio.h>
#include <stdlib.h>

// NB: Each ORB's thread package include above must define 4 classes:
//   TW_Mutex, TW_RWMutex, TW_Condition, TW_Thread
// It must also define these functions (or macros)
//     TW_ID
//     TW_EXIT
//     TW_YIELD
//     TW_SLEEP
//     TW_GET_TIME
// as described above.  Finally, it must define thread priority type
// TW_PRIORITY_T and the 3 priority values
// TW_PRIORITY_LOW, TW_PRIORITY_NORMAL, and
// TW_PRIORITY_HIGH.
// 
// The rest of the wrapper functionality is not ORB-specific,
// and is implemented here.
//

////////////////////////////////////////////////////////////////////////////////
// MACROS
////////////////////////////////////////////////////////////////////////////////

#ifndef TW_DEBUG
/////////////////////////////////////////////////////
// Non-debug macros
/////////////////////////////////////////////////////

#ifndef __TW_NULL_STMT
#define __TW_NULL_STMT 	do { } while (0)
#endif

#define TW_ABOUT_TO_LOCK(m, r, ty, w, f, l)                 __TW_NULL_STMT
#define TW_JUST_LOCKED(m, r, ty, w, f, l)                   __TW_NULL_STMT
#define TW_JUST_RELOCKED(cv, m, r, ty, w, f, l)             __TW_NULL_STMT
#define TW_ABOUT_TO_WEAKEN(m, ty, w, f, l)                  __TW_NULL_STMT
#define TW_ABOUT_TO_UNLOCK(m, r, ty, w, f, l)               __TW_NULL_STMT
#define TW_ABOUT_TO_EARLY_UNLOCK(m, r, ty, w, f, l)         __TW_NULL_STMT
#define TW_ABOUT_TO_WAIT(cv, m, r, ty, w, f, l)             __TW_NULL_STMT
#define TW_ABOUT_TO_TIMEDWAIT(cv, s, n, m, r, ty, w, f, l)  __TW_NULL_STMT

#define TW_COND_SCOPE_LOCK_TRACK(nm, held, mutex, resty, whatfn) \
    TW_CondMutexLock nm(held, mutex)
#define TW_SCOPE_LOCK_TRACK(nm, held, mutex, resty, whatfn) \
    TW_MutexLock nm(held, mutex)
#define TW_COND_SCOPE_LOCK_RW_TRACK(nm, held, mutex, readlock, resty, whatfn) \
    TW_CondRWMutexLock nm(held, mutex, readlock)
#define TW_SCOPE_LOCK_RW_TRACK(nm, held, mutex, readlock, resty, whatfn) \
    TW_RWMutexLock nm(held, mutex, readlock)

#define TW_SCOPE_RELEASE(nm) \
    TW_TempRelease _tmp_release(nm)
#define TW_SCOPE_RELEASE_RW(nm, readlock) \
    TW_TempRelease _tmp_release(nm, readlock)

#define TW_EARLY_RELEASE(nm) \
    nm.early_release()
#define TW_EARLY_RELEASE_RW(nm) \
    nm.early_release()
#define TW_WEAKEN_RW(nm) \
    nm.weaken()

#define TW_CV_WAIT(cv, mutex, resty, whatfn) \
    cv.wait()
#define TW_CV_TIMEDWAIT(cv, secs, nanosecs, mutex, resty, whatfn) \
    cv.timedwait(secs, nanosecs)
#define TW_CV_WAIT_RW(cv, mutex, readlock, resty, whatfn) \
    cv.wait()
#define TW_CV_TIMEDWAIT_RW(cv, secs, nanosecs, mutex, readlock, resty, whatfn) \
    cv.timedwait(secs, nanosecs)

#else
/////////////////////////////////////////////////////
// Debug macros
/////////////////////////////////////////////////////

#define TW_ABOUT_TO_LOCK(m, r, ty, w, f, l) \
    TW_GetInfo()->about_to_lock(m, r, ty, w, f, l)
#define TW_JUST_LOCKED(m, r, ty, w, f, l) \
    TW_GetInfo()->just_locked(m, r, ty, w, f, l)
#define TW_JUST_RELOCKED(cv, m, r, ty, w, f, l) \
    TW_GetInfo()->just_relocked(cv, m, r, ty, w, f, l)
#define TW_ABOUT_TO_WEAKEN(m, ty, w, f, l) \
    TW_GetInfo()->about_to_weaken(m, ty, w, f, l)
#define TW_ABOUT_TO_UNLOCK(m, r, ty, w, f, l) \
    TW_GetInfo()->about_to_unlock(m, r, ty, w, f, l)
#define TW_ABOUT_TO_EARLY_UNLOCK(m, r, ty, w, f, l) \
    TW_GetInfo()->about_to_early_unlock(m, r, ty, w, f, l)
#define TW_ABOUT_TO_WAIT(cv, m, r, ty, w, f, l) \
    TW_GetInfo()->about_to_wait(cv, m, r, ty, w, f, l)
#define TW_ABOUT_TO_TIMEDWAIT(cv, s, n, m, r, ty, w, f, l) \
    TW_GetInfo()->about_to_timedwait(cv, s, n, m, r, ty, w, f, l)

#define TW_COND_SCOPE_LOCK_TRACK(nm, held, mutex, resty, whatfn) \
    TW_DBG_CondMutexLock nm(held, mutex, resty, whatfn, __FILE__, __LINE__)
#define TW_SCOPE_LOCK_TRACK(nm, held, mutex, resty, whatfn) \
    TW_DBG_MutexLock nm(held, mutex, resty, whatfn, __FILE__, __LINE__)
#define TW_COND_SCOPE_LOCK_RW_TRACK(nm, held, mutex, readlock, resty, whatfn) \
    TW_DBG_CondRWMutexLock nm(held, mutex, readlock, resty, whatfn, __FILE__, __LINE__)
#define TW_SCOPE_LOCK_RW_TRACK(nm, held, mutex, readlock, resty, whatfn) \
    TW_DBG_RWMutexLock nm(held, mutex, readlock, resty, whatfn, __FILE__, __LINE__)

#define TW_SCOPE_RELEASE(nm) \
    TW_DBG_TempRelease _tmp_release(nm, __FILE__, __LINE__)
#define TW_SCOPE_RELEASE_RW(nm, readlock) \
    TW_DBG_TempRelease _tmp_release(nm, readlock, __FILE__, __LINE__)

#define TW_EARLY_RELEASE(nm) \
    nm.early_release(__FILE__, __LINE__)
#define TW_EARLY_RELEASE_RW(nm) \
    nm.early_release(__FILE__, __LINE__)
#define TW_WEAKEN_RW(nm) \
    nm.weaken(__FILE__, __LINE__)

#define TW_CV_WAIT(cv, mutex, resty, whatfn) do { \
    TW_ABOUT_TO_WAIT(&cv, &mutex, 0, resty, whatfn, __FILE__, __LINE__); \
    cv.wait(); \
    TW_JUST_RELOCKED(&cv, &mutex, 0, resty, whatfn, __FILE__, __LINE__); \
  } while (0)

#define TW_CV_TIMEDWAIT(cv, secs, nanosecs, mutex, resty, whatfn) do { \
    TW_ABOUT_TO_TIMEDWAIT(&cv, secs, nanosecs, &mutex, 0, resty, whatfn, __FILE__, __LINE__); \
    cv.timedwait(secs, nanosecs); \
    TW_JUST_RELOCKED(&cv, &mutex, 0, resty, whatfn, __FILE__, __LINE__); \
  } while (0)

#define TW_CV_WAIT_RW(cv, mutex, readlock, resty, whatfn) do { \
    TW_ABOUT_TO_WAIT(&cv, &mutex, readlock, resty, whatfn, __FILE__, __LINE__); \
    cv.wait(); \
    TW_JUST_RELOCKED(&cv, &mutex, readlock, resty, whatfn, __FILE__, __LINE__); \
  } while (0)

#define TW_CV_TIMEDWAIT_RW(cv, secs, nanosecs, mutex, readlock, resty, whatfn) do { \
    TW_ABOUT_TO_TIMEDWAIT(&cv, secs, nanosecs, &mutex, readlock, resty, whatfn, __FILE__, __LINE__); \
    cv.timedwait(secs, nanosecs); \
    TW_JUST_RELOCKED(&cv, &mutex, readlock, resty, whatfn, __FILE__, __LINE__); \
  } while (0)

// ----------------------------
// helper macros for TW_DbgElt
// ----------------------------

#define TW_DbgElt_APPEND(ARG_head, ARG_elt) \
do { \
  TW_DbgElt *_TMP_h = (ARG_head); \
  TW_DbgElt *_TMP_e = (ARG_elt); \
  _TMP_e->_next = _TMP_h; \
  _TMP_e->_prev = _TMP_h->_prev; \
  _TMP_e->_prev->_next = _TMP_e; \
  _TMP_e->_next->_prev = _TMP_e; \
} while (0)

#define TW_DbgElt_DEL1(ARG_elt) \
do { \
  TW_DbgElt *_TMP_e = (ARG_elt); \
  _TMP_e->_next->_prev = _TMP_e->_prev; \
  _TMP_e->_prev->_next = _TMP_e->_next; \
  delete _TMP_e; \
} while (0)

#define TW_DbgElt_POP1(ARG_head) \
do { \
  TW_DbgElt *_TMP_h = (ARG_head); \
  if (_TMP_h->_prev != _TMP_h) { \
    TW_DbgElt_DEL1(_TMP_h->_prev); \
  } \
} while (0)

#define TW_DbgElt_DELALL(ARG_head) \
do { \
  TW_DbgElt *_TMP_h = (ARG_head); \
  while (_TMP_h->_prev != _TMP_h) { \
    TW_DbgElt_DEL1(_TMP_h->_prev); \
  } \
  delete _TMP_h; \
} while (0)

#endif  /* !TW_DEBUG */

/////////////////////////////////////////////////////
// Generic macros
//   These just define non-track cases in terms
//   of track cases, so they work for both debug
//   and non-debug cases.
/////////////////////////////////////////////////////


#define TW_SCOPE_LOCK(nm, mutex, resty, whatfn) \
  int tmp_ ## nm ## _held = 0; \
  TW_SCOPE_LOCK_TRACK(nm, tmp_ ## nm ## _held, mutex, resty, whatfn)

#define TW_SCOPE_LOCK_RW(nm, mutex, readlock, resty, whatfn) \
  int tmp_ ## nm ## _held = 0; \
  TW_SCOPE_LOCK_RW_TRACK(nm, tmp_ ## nm ## _held, mutex, readlock, resty, whatfn)


//////////////////////////////////////////////////////////////////////////////////////////
// HELPER CLASSES
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef TW_DEBUG
/////////////////////////////////////////////////////
// Non-debug helper classes
/////////////////////////////////////////////////////

class TW_CondMutexLock {
  TW_Mutex&      _mutex;
  int&           _held;
  int            _initial_held;
public:
  TW_CondMutexLock(int &held, TW_Mutex& m)
    : _mutex(m), _held(held), _initial_held(held)
  {
    if (!_held) {
      _mutex.lock();
      _held = 1;
    }
  }
  ~TW_CondMutexLock(void)
  {
    // attempt to restore state to _initial_held state
    if (_held && !_initial_held) {
      _mutex.unlock();
      _held = 0;
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  TW_CondMutexLock(const TW_CondMutexLock&);
  TW_CondMutexLock& operator=(const TW_CondMutexLock&);
};

class TW_TempRelease;

class TW_MutexLock {
  friend class TW_TempRelease;
  TW_Mutex&      _mutex;
  int&           _held;
public:
  TW_MutexLock(int &held, TW_Mutex& m)
    : _mutex(m), _held(held)
  {
    _mutex.lock();
    _held = 1;
  }
  ~TW_MutexLock(void)
  {
    if (_held) {
      _mutex.unlock();
      _held = 0;
    }
  }
  void early_release()
  {
    if (_held) {
      _mutex.unlock();
      _held = 0;
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  TW_MutexLock(const TW_MutexLock&);
  TW_MutexLock& operator=(const TW_MutexLock&);
};

class TW_TempRelease {
  TW_MutexLock&  _mlock;
public:
  TW_TempRelease(TW_MutexLock& mlock)
    : _mlock(mlock)
  {
    if (mlock._held) {
      mlock._mutex.unlock();
      mlock._held = 0;
    }
  }
  ~TW_TempRelease(void)
  {
    if (!_mlock._held) {
      _mlock._mutex.lock();
      _mlock._held = 1;
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  TW_TempRelease(const TW_TempRelease&);
  TW_TempRelease& operator=(const TW_TempRelease&);
};

class TW_CondRWMutexLock {
  TW_RWMutex&    _mutex;
  int&           _held;
  int            _initial_held;
public:
  TW_CondRWMutexLock(int &held, TW_RWMutex& m, int readlock) : _mutex(m), _held(held), _initial_held(held)
  {
    int goal_held = (readlock) ? 1 : 2;
    if (_held) {
      if (_held != goal_held) {
	// cannot do a weaken/strengthen or strengthen/weaken
	// across the COND scope so can only support the case where lock is already held
	// in the mode that is requested
	fprintf(stderr,
		"\n*** TW_CondRWMutexLock: when used with lock already held,\n"
		" you MUST request lock mode == current lock mode ***\n");
	abort();
      }
      // else _held is already == goal_held
    } else {
      // lock not held yet
      _mutex.lock(readlock);
      _held = goal_held;
    }
  }
  ~TW_CondRWMutexLock(void)
  {
    // try to restore state to _initial_held state
    if (_held && !_initial_held) {
      _mutex.unlock();
      _held = 0;
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  TW_CondRWMutexLock(const TW_CondRWMutexLock&);
  TW_CondRWMutexLock& operator=(const TW_CondRWMutexLock&);
};

class TW_RWMutexLock {
  friend class TW_RWTempRelease;
  TW_RWMutex&    _mutex;
  int&           _held;
public:
  TW_RWMutexLock(int &held, TW_RWMutex& m, int readlock) : _mutex(m), _held(held)
  {
    _mutex.lock(readlock);
    _held = (readlock) ? 1 : 2;
  }
  ~TW_RWMutexLock(void)
  {
    if (_held) {
      _mutex.unlock();
      _held = 0;
    }
  }
  void early_release()
  {
    if (_held) {
      _mutex.unlock();
      _held = 0;
    }
  }
  void weaken()
  {
    if (_held == 2) {
      _mutex.weaken();
      _held = 1;
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  TW_RWMutexLock(const TW_RWMutexLock&);
  TW_RWMutexLock& operator=(const TW_RWMutexLock&);
};

class TW_RWTempRelease {
  TW_RWMutexLock&  _mlock;
  int                   _readlock;
public:
  TW_RWTempRelease(TW_RWMutexLock& mlock, int readlock)
    : _mlock(mlock), _readlock(readlock)
  {
    if (mlock._held) {
      mlock._mutex.unlock();
      mlock._held = 0;
    }
  }
  ~TW_RWTempRelease(void)
  {
    if (!_mlock._held) {
      _mlock._mutex.lock(_readlock);
      _mlock._held = (_readlock) ? 1 : 2;
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  TW_RWTempRelease(const TW_RWTempRelease&);
  TW_RWTempRelease& operator=(const TW_RWTempRelease&);
};

#else
/////////////////////////////////////////////////////
// Debug helper classes
/////////////////////////////////////////////////////

#define TW_DbgLogger(l) TW_logger l("THREAD_DBG", TW::DbgLog(), 0)
#define TW_DbgLog(stuff) do { TW_DbgLogger(l); l.str << stuff; } while (0)

// A very simple strstream

class TW_strstream {
public:
  TW_strstream();
  ~TW_strstream();

  // *** Methods borrowed from omniORB::logger ***
  TW_strstream& operator<<(char c);
  TW_strstream& operator<<(unsigned char c) { return (*this) << (char)c; }
  TW_strstream& operator<<(signed char c) { return (*this) << (char)c; }
  TW_strstream& operator<<(const char *s);
  TW_strstream& operator<<(const unsigned char *s) {
    return (*this) << (const char*)s;
  }
  TW_strstream& operator<<(const signed char *s) {
    return (*this) << (const char*)s;
  }
  TW_strstream& operator<<(const void *p);
  TW_strstream& operator<<(int n);
  TW_strstream& operator<<(unsigned int n);
  TW_strstream& operator<<(long n);
  TW_strstream& operator<<(unsigned long n);
  TW_strstream& operator<<(short n) {return operator<<((int)n);}
  TW_strstream& operator<<(unsigned short n) {return operator<<((unsigned int)n);}
#ifdef HAS_Cplusplus_Bool
  TW_strstream& operator<<(bool b) { return operator<<((int)b); }
#endif

  void setw(int n) { _width = _p + n; }
  void clear();
  const char* buf() { return (const char*)_buf; }
  char* retn();
  size_t len() { return (size_t)(_p - _buf); }

protected:
  TW_strstream(const TW_strstream&);
  TW_strstream& operator=(const TW_strstream&);

  void reserve(int n) { if ( _end - _p - 1 < n )  more(n); }
  void more(int n);
  void width_fill();

  char*       _buf;
  char*       _width;  // if _width > _p, setw has been called
  char*       _p;      // assert(*_p == '\0')
  char*       _end;    // assert(_p < _end)
  // note: _width can be >= _end
};

class TW_logger {
public:
  TW_logger(const char* prefix, FILE* file, FILE* alt_file, const char* flags = "", const char* srcfile = 0, int srcline = -1);
  ~TW_logger();  // destructor flushes the message

  // str is public instance state
  TW_strstream  str;

  void flush(int do_fflush);
  // output prefix + s to appropriate FILE*; this can be used for another log msg
  void flush_wo_prefix(int do_fflush);
  // same thing except the prefix is not output
  void write2FILE(FILE* outf, int do_fflush);
  // write2FILE writes this's buffer to specified FILE* without clearing it
  void write2FILE_wo_prefix(FILE* outf, int do_fflush);
  // same thing except the prefix is not output

private:
  TW_logger(const TW_logger&);
  TW_logger& operator=(const TW_logger&);

  char*           _prefix_buf;
  FILE*           _file;
  FILE*           _alt_file;
};

class TW {
  static FILE*             _dbg_log;
public:
  static FILE* DbgLog() { return _dbg_log; }
  static FILE* DbgLog(const char* pathnm);
  static FILE* DbgLog(const char* pathnm, const char* mode);
};

class TW_DbgElt;
class TW_DbgInfo;

typedef enum TW_DbgAct_e {
  TW_dummy_act,
  TW_about_to_lock_act,
  TW_just_locked_act,
  TW_about_to_early_unlock_act,
  TW_about_to_weaken_act,
  TW_about_to_unlock_act,
  TW_about_to_wait_act,
  TW_about_to_timedwait_act,
  TW_just_relocked_act
} TW_DbgAct;

class TW_DbgElt {
public:
  TW_DbgElt(TW_DbgAct act, void *mutex, void *cv, int readlock, const char* resty,
	    const char* whatfn, const char* file, int line)
    : _act(act), _mutex(mutex), _cv(cv), _readlock(readlock), _weakened(0), _line(line),
      _prev(this), _next(this)
  {
    _resty     = new char[::strlen(resty)+1];
    ::strcpy(_resty, resty);
    _whatfn    = new char[::strlen(whatfn)+1];
    ::strcpy(_whatfn, whatfn);
    _file      = new char[::strlen(file)+1];
    ::strcpy(_file, file);
  }

  ~TW_DbgElt()
  {
    delete [] _resty;
    delete [] _whatfn;
    delete [] _file;
  }

  static TW_DbgElt* make_dummy()
  {
    return new TW_DbgElt(TW_dummy_act, 0, 0, 0, "", "", "", 0);
  }

private:
  friend class TW_DbgInfo;

  TW_DbgAct        _act;
  void*            _mutex;
  void*            _cv;
  int              _readlock;
  int              _weakened;
  char*            _resty;
  char*            _whatfn;
  char*            _file;
  int              _line;
  // used for chaining:
  TW_DbgElt*       _prev;
  TW_DbgElt*       _next;
};

class TW_DbgInfo {
public:
  inline TW_DbgInfo()
  {
    _head    = TW_DbgElt::make_dummy();
    _length  = 0;
  }
  inline ~TW_DbgInfo()
  {
    if (_head) {
      TW_DbgElt_DELALL(_head);
      _head = 0;
    }
    _length = 0;
  }
  void about_to_lock(void *m, int readlock, const char* resty, const char* whatfn,
		     const char* file, int line);
  void just_locked(void *m, int readlock, const char* resty, const char* whatfn,
		   const char* file, int line);
  void about_to_weaken(void *m, const char* resty, const char* whatfn,
		       const char* file, int line);
  void about_to_early_unlock(void *m, int readlock, const char* resty, const char* whatfn,
			     const char* file, int line);
  void about_to_unlock(void *m, int readlock, const char* resty, const char* whatfn,
		       const char* file, int line);
  void about_to_wait(void *c, void *m, int readlock, const char* resty, const char* whatfn,
		     const char* file, int line);
  void about_to_timedwait(void *c, unsigned long secs, unsigned long nanosecs,
			  void *m, int readlock, const char* resty, const char* whatfn,
			  const char* file, int line);
  void just_relocked(void *c, void *m, int readlock, const char* resty, const char* whatfn,
		     const char* file, int line);

private:
  TW_DbgElt*     _head;
  int                 _length;

  // helpers 
  void _describe(const char* prefix, int fatal = 0);
  void _push1(TW_DbgAct act, void *mutex, void *cv, int readlock, const char* resty,
	      const char* whatfn, const char* file, int line);
  void _pop1();
  void _rem1(TW_DbgElt *e);
};

// --------------------------------------------------------------------------------

void TW_DbgInfo_cleanup(void*);

inline TW_DbgInfo* TW_GetInfo()
{
  TW_Thread* th = TW_Thread::self();
  if (!th->info) {
       th->info = (void*)new TW_DbgInfo();
       th->info_cleanup = TW_DbgInfo_cleanup;
  }
  return (TW_DbgInfo*)th->info;
}

// --------------------------------------------------------------------------------

class TW_DBG_CondMutexLock {
  TW_Mutex&        _mutex;
  int&             _held;
  int              _initial_held;
  const char*      _resty;
  const char*      _whatfn;
  const char*      _file;
  int              _line;

public:
  TW_DBG_CondMutexLock(int &held, TW_Mutex& m, const char* resty, const char* whatfn,
		       const char* file, int line)
    :  _mutex(m), _held(held), _initial_held(held), _resty(resty), _whatfn(whatfn),
       _file(file), _line(line)
  {
    if (!_held) {
      TW_ABOUT_TO_LOCK(&_mutex, 0, _resty, _whatfn, _file, _line);
      _mutex.lock();
      TW_JUST_LOCKED(&_mutex, 0, _resty, _whatfn, _file, _line);
      _held = 1;
    }
  }
  ~TW_DBG_CondMutexLock(void)
  {
    // attempt to restore state to _initial_held state
    if (_held && !_initial_held) {
      TW_ABOUT_TO_UNLOCK(&_mutex, 0, _resty, _whatfn, _file, _line);
      _mutex.unlock();
      _held = 0;
    }
  }
  void early_release(const char* file, int line)
  {
    if (_held) {
      TW_ABOUT_TO_EARLY_UNLOCK(&_mutex, 0, _resty, _whatfn, file, line);
      _mutex.unlock();
      _held = 0;
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  TW_DBG_CondMutexLock(const TW_DBG_CondMutexLock&);
  TW_DBG_CondMutexLock& operator=(const TW_DBG_CondMutexLock&);
};

class TW_DBG_TempRelease;

class TW_DBG_MutexLock {
  friend class TW_DBG_TempRelease;
  TW_Mutex&        _mutex;
  int&             _held;
  const char*      _resty;
  const char*      _whatfn;
  const char*      _file;
  int              _line;

public:
  TW_DBG_MutexLock(int &held, TW_Mutex& m, const char* resty, const char* whatfn,
		   const char* file, int line)
    :  _mutex(m), _held(held), _resty(resty), _whatfn(whatfn),
       _file(file), _line(line)
  {
    TW_ABOUT_TO_LOCK(&_mutex, 0, _resty, _whatfn, _file, _line);
    _mutex.lock();
    TW_JUST_LOCKED(&_mutex, 0, _resty, _whatfn, _file, _line);
    _held = 1;
  }
  ~TW_DBG_MutexLock(void)
  {
    if (_held) {
      TW_ABOUT_TO_UNLOCK(&_mutex, 0, _resty, _whatfn, _file, _line);
      _mutex.unlock();
      _held = 0;
    }
  }
  void early_release(const char* file, int line)
  {
    if (_held) {
      TW_ABOUT_TO_EARLY_UNLOCK(&_mutex, 0, _resty, _whatfn, file, line);
      _mutex.unlock();
      _held = 0;
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  TW_DBG_MutexLock(const TW_DBG_MutexLock&);
  TW_DBG_MutexLock& operator=(const TW_DBG_MutexLock&);
};

class TW_DBG_TempRelease {
  TW_DBG_MutexLock&  _mlock;
  const char*        _file;
  int                _line;
public:
  TW_DBG_TempRelease(TW_DBG_MutexLock& mlock,
		     const char* file, int line)
    : _mlock(mlock), _file(file), _line(line)
  {
    if (mlock._held) {
      TW_ABOUT_TO_UNLOCK(&(mlock._mutex), 0, mlock._resty, mlock._whatfn, file, line);
      mlock._mutex.unlock();
      mlock._held = 0;
    }
  }
  ~TW_DBG_TempRelease(void)
  {
    if (!_mlock._held) {
      TW_ABOUT_TO_LOCK(&(_mlock._mutex), 0, _mlock._resty, _mlock._whatfn, _file, _line);
      _mlock._mutex.lock();
      TW_JUST_LOCKED(&(_mlock._mutex), 0, _mlock._resty, _mlock._whatfn, _file, _line);
      _mlock._held = 1;
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  TW_DBG_TempRelease(const TW_DBG_TempRelease&);
  TW_DBG_TempRelease& operator=(const TW_DBG_TempRelease&);
};

class TW_DBG_CondRWMutexLock {
  TW_RWMutex&      _mutex;
  int&             _held;
  int              _initial_held;
  int              _readlock;
  const char*      _resty;
  const char*      _whatfn;
  const char*      _file;
  int              _line;
public:
  TW_DBG_CondRWMutexLock(int &held, TW_RWMutex& m, int readlock, const char* resty, const char* whatfn,
			 const char* file, int line)
    : _mutex(m), _held(held), _initial_held(held), _readlock(readlock), _resty(resty), _whatfn(whatfn),
      _file(file), _line(line)
  {
    int goal_held = (readlock) ? 1 : 2;
    if (_held) {
      if (_held != goal_held) {
	// cannot do a weaken/strengthen or strengthen/weaken
	// across the COND scope so can only support the case where lock is already held
	// in the mode that is requested
	fprintf(stderr,
		"\n*** TW_DBG_CondRWMutexLock: when used with lock already held,\n"
		" you MUST request lock mode == current lock mode ***\n");
	abort();
      }
      // else _held is already == goal_held
    } else {
      TW_ABOUT_TO_LOCK(&_mutex, _readlock, _resty, _whatfn, _file, _line);
      _mutex.lock(readlock);
      TW_JUST_LOCKED(&_mutex, _readlock, _resty, _whatfn, _file, _line);
      _held = goal_held;
    }
  }
  ~TW_DBG_CondRWMutexLock(void)
  {
    // try to restore state to _initial_held state
    if (_held && !_initial_held) {
      TW_ABOUT_TO_UNLOCK(&_mutex, _readlock, _resty, _whatfn, _file, _line);
      _mutex.unlock();
      _held = 0;
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  TW_DBG_CondRWMutexLock(const TW_DBG_CondRWMutexLock&);
  TW_DBG_CondRWMutexLock& operator=(const TW_DBG_CondRWMutexLock&);
};

class TW_DBG_RWTempRelease;

class TW_DBG_RWMutexLock {
  friend class TW_DBG_RWTempRelease;
  TW_RWMutex&      _mutex;
  int&             _held;
  int              _readlock;
  const char*      _resty;
  const char*      _whatfn;
  const char*      _file;
  int              _line;
public:
  TW_DBG_RWMutexLock(int &held, TW_RWMutex& m, int readlock, const char* resty, const char* whatfn,
		     const char* file, int line)
    : _mutex(m), _held(held), _readlock(readlock), _resty(resty), _whatfn(whatfn),
      _file(file), _line(line)
  {
    TW_ABOUT_TO_LOCK(&_mutex, _readlock, _resty, _whatfn, _file, _line);
    _mutex.lock(readlock);
    TW_JUST_LOCKED(&_mutex, _readlock, _resty, _whatfn, _file, _line);
    _held = (readlock) ? 1 : 2;
  }
  ~TW_DBG_RWMutexLock(void)
  {
    if (_held) {
      TW_ABOUT_TO_UNLOCK(&_mutex, _readlock, _resty, _whatfn, _file, _line);
      _mutex.unlock();
      _held = 0;
    }
  }
  void weaken(const char* file, int line)
  {
    if (_held == 2) {
      TW_ABOUT_TO_WEAKEN(&_mutex, _resty, _whatfn, file, line);
      _mutex.weaken();
      _held = 1;
      _readlock = 1;
      _file = file;
      _line = line;
    }
  }
  void early_release(const char* file, int line)
  {
    if (_held) {
      TW_ABOUT_TO_EARLY_UNLOCK(&_mutex, _readlock, _resty, _whatfn, file, line);
      _mutex.unlock();
      _held = 0;
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  TW_DBG_RWMutexLock(const TW_DBG_RWMutexLock&);
  TW_DBG_RWMutexLock& operator=(const TW_DBG_RWMutexLock&);
};

class TW_DBG_RWTempRelease {
  TW_DBG_RWMutexLock&  _mlock;
  int                       _readlock;
  const char*               _file;
  int                       _line;
public:
  TW_DBG_RWTempRelease(TW_DBG_RWMutexLock& mlock, int readlock,
		       const char* file, int line)
    : _mlock(mlock), _readlock(readlock), _file(file), _line(line)
  {
    if (mlock._held) {
      TW_ABOUT_TO_UNLOCK(&(mlock._mutex), mlock._readlock, mlock._resty, mlock._whatfn, file, line);
      mlock._mutex.unlock();
      mlock._held = 0;
    }
  }
  ~TW_DBG_RWTempRelease(void)
  {
    if (!_mlock._held) {
      TW_ABOUT_TO_LOCK(&(_mlock._mutex), _readlock, _mlock._resty, _mlock._whatfn, _file, _line);
      _mlock._mutex.lock(_readlock);
      TW_JUST_LOCKED(&(_mlock._mutex), _readlock, _mlock._resty, _mlock._whatfn, _file, _line);
      _mlock._held = (_readlock) ? 1 : 2;
      _mlock._readlock = _readlock;
    }
  }
private:
  // dummy copy constructor and operator= to prevent copying
  TW_DBG_RWTempRelease(const TW_DBG_RWTempRelease&);
  TW_DBG_RWTempRelease& operator=(const TW_DBG_RWTempRelease&);
};

#endif  /* !TW_DEBUG */

#endif  /* !__THREAD_WRAPPERS_H__  */
