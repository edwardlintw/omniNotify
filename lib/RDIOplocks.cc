// -*- Mode: C++; -*-
//                              File      : RDIOplocks.cc
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
//    Implementation of RDIOplocks
//
 
#include "thread_wrappers.h"
#include "RDI.h"
#include "RDIOplocks.h"
#include "CosNfyUtils.h"

#define RDI_100_MS_AS_NS 100000000

#if 0
#define DESCRIBE_INUSE_BUMP \
  do { if (_owner_name) { RDIDbgForceLog("*** INUSE BUMP: " << *(_owner_name) << " inuse now " << _inuse << '\n'); } } while (0)
#define DESCRIBE_INUSE_DEBUMP \
  do { if (_owner_name) { RDIDbgForceLog("*** INUSE DEBUMP: " << *(_owner_name) << " inuse now " << _inuse << '\n'); } } while (0)
#else
#define DESCRIBE_INUSE_BUMP    do { } while(0)
#define DESCRIBE_INUSE_DEBUMP  do { } while(0)
#endif

// ---------------------- RDIOplockEntry -------------------------------

CORBA::Boolean
RDIOplockEntry::acquire(RDIOplockEntry** ptr)
{
  if ((ptr != _ptr) || _disposed) {
    return 0;
  }
  _oplock.lock();
  if ((ptr != _ptr) || _disposed) {
    _oplock.unlock();
    return 0;
  }
  return 1;
}

CORBA::Boolean
RDIOplockEntry::reacquire(RDIOplockEntry** ptr)
{
  if (ptr != _ptr) return 0;
  _oplock.lock();
  if (ptr != _ptr) {
    _oplock.unlock();
    return 0;
  }
  return 1;
}

void
RDIOplockEntry::wait()
{
  CORBA::Boolean do_bump_debump = 1;
  if (_disposed) {
    do_bump_debump = 0;
    RDIDbgForceLog("** Internal error: RDIOplockEntry " << (void*)this <<
		   " calling wait after disposed is true\n");
  }
  if (do_bump_debump) { _inuse++; DESCRIBE_INUSE_BUMP; } 
  _waitvar.wait();
  if (do_bump_debump) { _inuse--; DESCRIBE_INUSE_DEBUMP; } 
}

void
RDIOplockEntry::timedwait(unsigned long s, unsigned long n)
{
  CORBA::Boolean do_bump_debump = 1;
  if (_disposed) {
    do_bump_debump = 0;
    RDIDbgForceLog("** Internal error: RDIOplockEntry " << (void*)this <<
		   " calling timedwait after disposed is true\n");
  }
  if (do_bump_debump) { _inuse++; DESCRIBE_INUSE_BUMP; } 
  _waitvar.timedwait(s,n);
  if (do_bump_debump) { _inuse--; DESCRIBE_INUSE_DEBUMP; } 
}

void
RDIOplockEntry::altcv_wait(TW_Condition& cv)
{
  CORBA::Boolean do_bump_debump = 1;
  if (_disposed) {
    do_bump_debump = 0;
    RDIDbgForceLog("** Internal error: RDIOplockEntry " << (void*)this <<
		   " calling altcv_wait after disposed is true\n");
  }
  if (do_bump_debump) { _inuse++; DESCRIBE_INUSE_BUMP; } 
  cv.wait();
  if (do_bump_debump) { _inuse--; DESCRIBE_INUSE_DEBUMP; } 
}

void
RDIOplockEntry::altcv_timedwait(TW_Condition& cv, unsigned long s, unsigned long n)
{
  CORBA::Boolean do_bump_debump = 1;
  if (_disposed) {
    do_bump_debump = 0;
    RDIDbgForceLog("** Internal error: RDIOplockEntry " << (void*)this <<
		   " calling altcv_timedwait after disposed is true\n");
  }
  if (do_bump_debump) { _inuse++; DESCRIBE_INUSE_BUMP; } 
  cv.timedwait(s,n);
  if (do_bump_debump) { _inuse--; DESCRIBE_INUSE_DEBUMP; } 
}

#ifdef TW_DEBUG
void
RDIOplockEntry::dbg_wait(const char *whatfn, const char *file, int line)
{
  CORBA::Boolean do_bump_debump = 1;
  if (_disposed) {
    do_bump_debump = 0;
    RDIDbgForceLog("** Internal error: RDIOplockEntry " << (void*)this <<
		   " calling wait after disposed is true\n");
  }
  if (do_bump_debump) { _inuse++; DESCRIBE_INUSE_BUMP; }
  TW_ABOUT_TO_WAIT(&_waitvar, &_oplock, 0, _resty, whatfn, file, line); \
  _waitvar.wait();
  TW_JUST_RELOCKED(&_waitvar, &_oplock, 0, _resty, whatfn, file, line); \
  if (do_bump_debump) { _inuse--; DESCRIBE_INUSE_DEBUMP; } 
}

void
RDIOplockEntry::dbg_timedwait(unsigned long s, unsigned long n, const char *whatfn, const char *file, int line)
{
  CORBA::Boolean do_bump_debump = 1;
  if (_disposed) {
    do_bump_debump = 0;
    RDIDbgForceLog("** Internal error: RDIOplockEntry " << (void*)this <<
		   " calling timedwait after disposed is true\n");
  }
  if (do_bump_debump) { _inuse++; DESCRIBE_INUSE_BUMP; } 
  TW_ABOUT_TO_TIMEDWAIT(&_waitvar, s, n, &_oplock, 0, _resty, whatfn, file, line); \
  _waitvar.timedwait(s,n);
  TW_JUST_RELOCKED(&_waitvar, &_oplock, 0, _resty, whatfn, file, line); \
  if (do_bump_debump) { _inuse--; DESCRIBE_INUSE_DEBUMP; } 
}

void
RDIOplockEntry::dbg_altcv_wait(TW_Condition& cv, const char *whatfn, const char *file, int line)
{
  CORBA::Boolean do_bump_debump = 1;
  if (_disposed) {
    do_bump_debump = 0;
    RDIDbgForceLog("** Internal error: RDIOplockEntry " << (void*)this <<
		   " calling altcv_wait after disposed is true\n");
  }
  if (do_bump_debump) { _inuse++; DESCRIBE_INUSE_BUMP; }
  TW_ABOUT_TO_WAIT(&cv, &_oplock, 0, _resty, whatfn, file, line); \
  cv.wait();
  TW_JUST_RELOCKED(&cv, &_oplock, 0, _resty, whatfn, file, line); \
  if (do_bump_debump) { _inuse--; DESCRIBE_INUSE_DEBUMP; } 
}

void
RDIOplockEntry::dbg_altcv_timedwait(TW_Condition& cv, unsigned long s, unsigned long n, const char *whatfn, const char *file, int line)
{
  CORBA::Boolean do_bump_debump = 1;
  if (_disposed) {
    do_bump_debump = 0;
    RDIDbgForceLog("** Internal error: RDIOplockEntry " << (void*)this <<
		   " calling altcv_timedwait after disposed is true\n");
  }
  if (do_bump_debump) { _inuse++; DESCRIBE_INUSE_BUMP; } 
  TW_ABOUT_TO_TIMEDWAIT(&cv, s, n, &_oplock, 0, _resty, whatfn, file, line); \
  cv.timedwait(s,n);
  TW_JUST_RELOCKED(&cv, &_oplock, 0, _resty, whatfn, file, line); \
  if (do_bump_debump) { _inuse--; DESCRIBE_INUSE_DEBUMP; } 
}

void
RDIOplockEntry::dbg_inuseone_wait(const char *whatfn, const char *file, int line)
{
  TW_ABOUT_TO_WAIT(&_inuseone, &_oplock, 0, _resty, whatfn, file, line); \
  _inuseone.wait();
  TW_JUST_RELOCKED(&_inuseone, &_oplock, 0, _resty, whatfn, file, line); \
}

void
RDIOplockEntry::dbg_inuseone_timedwait(unsigned long s, unsigned long n,
				       const char *whatfn, const char *file, int line)
{
  TW_ABOUT_TO_TIMEDWAIT(&_inuseone, s, n, &_oplock, 0, _resty, whatfn, file, line); \
  _inuseone.timedwait(s,n);
  TW_JUST_RELOCKED(&_inuseone, &_oplock, 0, _resty, whatfn, file, line); \
}
#endif  /* TW_DEBUG */

void
RDIOplockEntry::bump()
{
  if (_disposed) {
    RDIDbgForceLog("** Internal error: RDIOplockEntry " << (void*)this <<
		   " bumping inuse after disposed is true\n");
  }
  _inuse++; DESCRIBE_INUSE_BUMP; 
}

void
RDIOplockEntry::debump()
{
  if (_inuse == 0) {
    RDIDbgForceLog("** Internal error: RDIOplockEntry " << (void*)this <<
		   " debumping zero inuse -- ignored\n");
  } else {
    _inuse--; DESCRIBE_INUSE_DEBUMP;
    if (_inuse == 1) {
      _inuseone.broadcast();
    }
  }
}

// -------------- helper routines -----------------

// remove this from chain -- becomes singleton chain
void
RDIOplockEntry::_remove()
{
  _prev->_next = _next;
  _next->_prev = _prev;
  _next = this;
  _prev = this;
}

// prepend this in front of b (b is in chain, this is not)
void
RDIOplockEntry::_prepend(RDIOplockEntry* b)
{
  _next = b;
  _prev = b->_prev;
  _prev->_next = this;
  b->_prev = this;
}

// append this after b (b is in chain, this is not)
void
RDIOplockEntry::_append(RDIOplockEntry* b)
{
  _prev = b;
  _next = b->_next;
  _next->_prev = this;
  b->_next = this;
}

// ---------------------- RDIOplocks -----------------------------------

// private static class variables

TW_Mutex         RDIOplocks::_oplock;
RDIOplockEntry*  RDIOplocks::_freelist = new RDIOplockEntry();
CORBA::Boolean   RDIOplocks::_shutdown = 0;
#ifndef NDEBUG
RDIOplocksDbgMap RDIOplocks::_dbg_ptrs(RDI_VoidStarHash, RDI_VoidStarRank);
#endif

// cleanup returns 1 if there are entries in use that could not be discarded, else 0
CORBA::ULong
RDIOplocks::cleanup()
{
  CORBA::ULong res = 0;
  _oplock.lock();
  if (_freelist == 0) { 
    _oplock.unlock();
    return res;
  }
  // discard entries with ptr == 0 and inuse == 0
  // or with disposed == 1 and inuse == 0
  RDIOplockEntry* nxt_nxt = _freelist;
  for (RDIOplockEntry* nxt = _freelist->_next; nxt != _freelist; nxt = nxt_nxt) {
    nxt_nxt = nxt->_next; 
    if ((nxt->_inuse == 0) && ((nxt->_ptr == 0) || nxt->_disposed)) {
      nxt->_remove(); // removes from freelist
      delete nxt;
    } else {
      res++;
#if 0
      if (nxt->_disposed && nxt->_owner_name) {
	RDIDbgForceLog("XXX_REMOVE_THIS: RDIOplocks::cleanup waiting for destroyed obj " <<
		       *(nxt->_owner_name) << " to finish " << nxt->_inuse << " outstanding invocations\n");
      } else if (nxt->_owner_name) {
	RDIDbgForceLog("XXX_REMOVE_THIS: UNEXPECTED: free list has oplock of non-destroyed obj " <<
		       *(nxt->_owner_name) << " with " << nxt->_inuse << " outstanding invocations\n");
      }
#endif
    }
  }
  _oplock.unlock();
  return res;
}

void
RDIOplocks::shutdown()
{
  _oplock.lock();
  if (_shutdown) { // already called
    _oplock.unlock();
    return;
  }
  _shutdown = 1;
  _oplock.unlock();
  CORBA::ULong inuse = cleanup();
  _oplock.lock();
  if (inuse == 0) { // can discard freelist
    if (_freelist) {
      delete _freelist;
    }
    _freelist = 0;
  } else { // fast shutdown requires leaving some allocated storage in freelist
    //    RDIDbgForceLog("XXX_REMOVE_THIS: Leaving " << inuse << " inuse oplock entries in freelist\n");
  }
  _oplock.unlock();
}

RDIOplockEntry*
RDIOplocks::alloc_entry(RDIOplockEntry** optr, AttN::NameSeq* oname, const char *resty)
{
  if (optr == 0) {
    RDIDbgForceLog("** Internal error: RDIOplocks::alloc_entry called with null ptr\n");
    return 0;
  }
  _oplock.lock();
  if (_shutdown) {
    _oplock.unlock();
    return 0;
  }
  RDIOplockEntry* nxt = 0;
  // first try to re-use an entry on the free list 
  for (nxt = _freelist->_next; nxt != _freelist; nxt = nxt->_next) {
    if (nxt->_ptr == 0) {
      nxt->_oplock.lock();
      if ((nxt->_ptr != 0) || (nxt->_inuse != 0) || (nxt->_disposed)) {
	RDIDbgForceLog("** Internal error: RDIOplocks::alloc_entry : " <<
		       "once ptr is zero, ptr, inuse, and disposed " <<
		       "are supposed to remain zero until entry is reallocated\n");
	nxt->_oplock.unlock();
	continue;
      }
      nxt->_remove(); // removes from freelist
      nxt->_dispose_info = WRAPPED_DISPOSEINFO_NIL;
      nxt->_inuse = 0;
      nxt->_disposed = 0;
      nxt->_ptr = optr;
      nxt->_owner_name = oname;
      nxt->_resty = resty;
      _oplock.unlock();
      nxt->_oplock.unlock();
#ifndef NDEBUG
      _dbg_ptrs.insert(nxt, 1);
#endif
      return nxt;
    }
    if (nxt->_disposed && (nxt->_inuse == 0)) {
      nxt->_oplock.lock();
      if (!nxt->_disposed || (nxt->_inuse != 0)) {
	RDIDbgForceLog("** Internal error: RDIOplocks::alloc_entry : " <<
		       "disposed changed from 1 to 0, or inuse from 0 to 1, unexpectedly!" <<
		       "Once disposed is set, it is supposed to remain set " <<
		       "and once inuse is zero, it is supposed to remain zero until dispose action occurs\n");
	nxt->_oplock.unlock();
	continue;
      }
      nxt->_remove(); // removes from freelist
      if (!nxt->_ptr) {
	RDIDbgForceLog("** Internal error: RDIOplocks::alloc_entry : " <<
		       "ptr not set!  Once disposed is set, ptr is supposed to remain set " <<
		       "until dispose action occurs\n");
      } else {
	*(nxt->_ptr) = 0; // _oplockptr of (about to be previous) owner set to zero
#ifndef NDEBUG
	_dbg_ptrs.remove(nxt);
#endif
      }
      if (!WRAPPED_DISPOSEINFO_IS_NIL(nxt->_dispose_info)) {
	WRAPPED_DISPOSE_INFO(nxt->_dispose_info);
      }
      nxt->_dispose_info = WRAPPED_DISPOSEINFO_NIL;
      nxt->_inuse = 0;
      nxt->_disposed = 0;
      nxt->_ptr = optr;
      nxt->_owner_name = oname;
      nxt->_resty = resty;
      _oplock.unlock();
      nxt->_oplock.unlock();
#ifndef NDEBUG
      _dbg_ptrs.insert(nxt, 1);
#endif
      return nxt;
    }
    // nxt is 'in use' -- skip
  }
  // none of the entries on the free list are available at the moment
  // so allocate a new entry
  nxt = new RDIOplockEntry();
  nxt->_oplock.lock();
  nxt->_dispose_info = WRAPPED_DISPOSEINFO_NIL;
  nxt->_inuse = 0;
  nxt->_disposed = 0;
  nxt->_ptr = optr;
  nxt->_owner_name = oname;
  nxt->_resty = resty;
  _oplock.unlock();
  nxt->_oplock.unlock();
#ifndef NDEBUG
  _dbg_ptrs.insert(nxt, 1);
#endif
  return nxt;
}

// ASSUMES: e->_oplock has been locked
void
RDIOplocks::free_entry(RDIOplockEntry* e, RDIOplockEntry** optr, WRAPPED_DISPOSEINFO_PTR dinfo)
{
  if (e->_inuse && e->_owner_name) {
    // RDIDbgForceLog("*** XXX_REMOVE_THIS: " << *(e->_owner_name) << " : RDIOplocks::free_entry called with e->_inuse = " << e->_inuse << '\n');
  }
  if (e->_ptr != optr) {
    RDIDbgForceLog("*** Internal error: RDIOplocks::free_entry called with optr != e->_ptr\n");
    return;
  }
  if (!e->_ptr) {
    RDIDbgForceLog("*** Internal error: RDIOplocks::free_entry called with null e->_ptr\n");
    return;
  }
  if (e->_disposed) {  // it is OUR job to set e->_disposed
    RDIDbgForceLog("*** Internal error: RDIOplocks::free_entry should not be " <<
		   "called with e->_disposed set\n");
    e->_disposed = 0;
  }
  if ((e->_next != e) || (e->_prev != e)) {
    RDIDbgForceLog("*** Internal error: RDIOplocks::free_entry called with an entry " <<
		   "that appears to already be on the free list\n");
    e->_remove();
  }
  // append e to free list (by prepending in front of dummy head elt)
  e->_prepend(_freelist);
  if (e->_inuse == 0) { // can dispose immediately
    *(e->_ptr) = 0; // _oplockptr of owner set to zero
#ifndef NDEBUG
    _dbg_ptrs.remove(e);
#endif
    if (!WRAPPED_DISPOSEINFO_IS_NIL(dinfo)) {
      WRAPPED_DISPOSE_INFO(dinfo);
    }
    e->_ptr = 0;
    e->_owner_name = 0;
  } else { // defer disposal
    e->_disposed = 1;
    e->_dispose_info = dinfo;
  }
  e->_oplock.unlock();
}


