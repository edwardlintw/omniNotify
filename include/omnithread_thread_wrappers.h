// -*- Mode: C++; -*-
//                              File      : omniorb_thread_wrappers.h
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
//    Wrappers for threads, mutexes, condition variables.
//
 
#ifndef __omniorb_thread_wrappers_h__
#define __omniorb_thread_wrappers_h__

#include <omnithread.h>
// #include "RDIstrstream.h" XXX_OBSOLETE?

// Some classes can be direct mappings:

#define TW_Mutex      omni_mutex 
#define TW_Condition  omni_condition
#define TW_Thread     omni_thread

/** TW_RWMutex
  *
  * In many cases, we want to support concurrent read accesses and
  * exclusive write access to a given object.  The following class
  * provides such an interface  using the OMNI mutex and condition
  * variables.
  */

class TW_RWMutex {
public:
  TW_RWMutex() : _lock(), _gate(&_lock), _wait(0), _read(0) {;}
  ~TW_RWMutex()  {;}

  inline void lock(int toread);
  inline void unlock();
  inline void weaken();
         void acquire(int toread)	{ lock(toread); }
         void release(void)		{ unlock(); }

  // XXX_OBSOLETE?
  //  RDIstrstream& log_output(RDIstrstream& str) const {
  //    return str << "wait " << (_wait ? "T " : "F ") << "read " << _read; 
  //  }

private:
  TW_Mutex        _lock;
  TW_Condition    _gate;	
  int             _wait;		// TRUE when writer active
  unsigned int    _read;		// # of concurrent readers

  TW_RWMutex(const TW_RWMutex&);
  TW_RWMutex& operator=(const TW_RWMutex&);
};

// ------------------------------------------------------------- //

inline void TW_RWMutex::lock(int toread)
{
  _lock.lock();
  // If a writer is already present, we have to wait.  The same is
  // true when we are the only writer and there are active readers
  while ( _wait || ((toread == 0) && (_read != 0)) )
	_gate.wait();
  if ( toread )
	_read += 1;
  else 
	_wait  = 1;
  _lock.unlock();
}

inline void TW_RWMutex::unlock()
{
  _lock.lock();
  if ( _wait )
	_wait = 0;
  else  
	_read -= 1;
  if ( _read == 0 ) 
	_gate.signal();
  _lock.unlock();
}

inline void TW_RWMutex::weaken()
{
  _lock.lock();
  // We should be a writer wanting to replace our exclusive lock
  // with a shared lock
  if ( _wait ) {
	_wait  = 0;
	_read += 1;
	_gate.signal();		// Wake up any blocked threads
  }
  _lock.unlock();
}

////////////////////////////////////////
// Global Functions or Macros

inline unsigned long TW_ID() {
  return (unsigned long)(omni_thread::self()->id());
}

#define TW_EXIT     omni_thread::exit
#define TW_YIELD    omni_thread::yield
#define TW_SLEEP    omni_thread::sleep
#define TW_GET_TIME omni_thread::get_time

////////////////////////////////////////
// Thread Priorities

#define TW_PRIORITY_T       omni_thread::priority_t
#define TW_PRIORITY_LOW     omni_thread::PRIORITY_LOW
#define TW_PRIORITY_NORMAL  omni_thread::PRIORITY_NORMAL
#define TW_PRIORITY_HIGH    omni_thread::PRIORITY_HIGH

#endif  /* __omniorb_thread_wrappers_h__  */
