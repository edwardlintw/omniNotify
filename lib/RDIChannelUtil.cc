// -*- Mode: C++; -*- File : RDIChannelUtil.cc Package :
// omniNotify-Library Created on: 1-Jan-1998 Authors : gruber&panagos
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
//    Implementation of several utility functions used by EventChannel_i
//

#include "thread_wrappers.h"
#include "RDIThreadPriority.h"
#include "RDI.h"
#include "CosNfyUtils.h"
#include "RDIBitmap.h"
#include "CosNotifyChannelAdmin_i.h"

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//                         Various Utility Functions                     //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

const char*
RDI_PRX_TYPE(const CosNA::ProxyType& type)
{
  switch ( type ) {
  case CosNA::PUSH_ANY:        return (const char*)"PUSH_ANY";
  case CosNA::PULL_ANY:        return (const char*)"PULL_ANY";
  case CosNA::PUSH_STRUCTURED: return (const char*)"PUSH_STR";
  case CosNA::PULL_STRUCTURED: return (const char*)"PULL_STR";
  case CosNA::PUSH_SEQUENCE:   return (const char*)"PUSH_SEQ";
  case CosNA::PULL_SEQUENCE:   return (const char*)"PULL_SEQ";
  case CosNA::PUSH_TYPED:      return (const char*)"PUSH_TYP";
  case CosNA::PULL_TYPED:      return (const char*)"PULL_TYP";
  }
  return (const char *)"INVALID";
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-= //
//        CAdminGroupMgr implementation        //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-= //

CAdminGroupMgr::CAdminGroupMgr(unsigned int numGroups,
			     unsigned int numThreds) :
  _oplock(), _groups(0), _length(numGroups),
  _nextgr(0), _numthr(numThreds),
  _rmgrps(numGroups), _rmthrs(numThreds)
{ _groups = new CAdminGroup [_length]; }

CAdminGroupMgr::~CAdminGroupMgr()
{
  if (_groups) {
    delete [] _groups;
    _groups = 0;
  }
}

#undef WHATFN
#define WHATFN "CAdminGroupMgr::insert"
void
CAdminGroupMgr::insert(ConsumerAdmin_i* entry)
{
  unsigned int indx = (unsigned int) (entry->_admin_id() % _length);
  RDIDbgChanLog("Add ConsumerAdmin "<<entry->_admin_id()<<" to group "<<&_groups[indx] << '\n');
  RDI_SAFELIST_SAFE_INSERT(&(_groups[indx]), entry, "cadmingroup", WHATFN);
}

#undef WHATFN
#define WHATFN "CAdminGroupMgr::remove"
void
CAdminGroupMgr::remove(ConsumerAdmin_i* entry)
{
  unsigned int indx = (unsigned int) (entry->_admin_id() % _length);
  RDIDbgChanLog("Rem ConsumerAdmin "<<entry->_admin_id()<<" from group "<<&_groups[indx] << '\n');
  RDI_SAFELIST_SAFE_REMOVE(&(_groups[indx]), entry, "cadmingroup", WHATFN);
}

#undef WHATFN
#define WHATFN "CAdminGroupMgr::remove_all"
void
CAdminGroupMgr::remove_all(ConsumerAdminPtrSeq& seq_out)
{
  int            slen = seq_out.length();
  unsigned int   indx;
  for (indx = 0; indx < _length; indx++) {
    CAdminGroup* list = &(_groups[indx]);
    { // introduce list lock scope
      RDI_SAFELIST_SCOPE_LOCK(list_lock, list, "cadmingroup", WHATFN);
      int len = list->length();
      for (int i = 0; i < len;) {
	ConsumerAdmin_i* admin = list->iter_next();
	if (admin) {
	  i++;
	  seq_out.length(++slen);
	  seq_out[slen-1] = admin;
	}
      }
      list->drain();
    } // end list lock scope
  }
}

#undef WHATFN
#define WHATFN "CAdminGroupMgr::allocate_range"
CORBA::Boolean
CAdminGroupMgr::allocate_range(unsigned int& lo,
			      unsigned int& hi)
{
  unsigned int slice = 0;
  lo = hi = 0;
  TW_SCOPE_LOCK(cadmingroupmgr_lock, _oplock, "cadmingroupmgr", WHATFN);

  if ( _nextgr == _length ) { // Everything has been allocated
    return 0;
  }
  if ( _rmthrs == 0 ) { // Internal error!!!!!
    return 0;
  }
  slice    = _rmgrps / _rmthrs;
  lo       = _nextgr;
  _nextgr += slice;
  hi       = _nextgr - 1;
  _rmgrps -= slice;
  _rmthrs -= 1;
  return 1;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//             Support classes used for event dispatching                //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

EventChannelDispatch::EventChannelDispatch(EventChannel_i*    evnt_channel,
                                           unsigned int       num_athreads,
                                           EventChannelMethod admin_method,
                                           unsigned int       num_pthreads,
                                           EventChannelMethod proxy_method) :
  _num_athreads(num_athreads), _admin_thread(0),
  _num_pthreads(num_pthreads), _proxy_thread(0),
  _evnt_channel(evnt_channel)
{
  unsigned int ix;
  if ( _num_athreads ) {
    _admin_thread = new TW_Thread * [ _num_athreads ];
    RDI_AssertAllocThrowNo(_admin_thread, "Memory allocation failed -- Thread\n");
  }
  for ( ix=0; ix < _num_athreads; ix++) {
    // _admin_thread[ix]=new EventChannelWorker(_evnt_channel, admin_method);
    _admin_thread[ix] = new EventChannelBoundWorker(_evnt_channel, admin_method);
    RDI_AssertAllocThrowNo(_admin_thread[ix], "Failed to create new admin thread\n");
    RDIDbgChanLog("New admin filtering thread: " << _admin_thread[ix]->id() << '\n');
  }
  if ( _num_pthreads ) {
    _proxy_thread = new TW_Thread * [ _num_pthreads ];
    RDI_AssertAllocThrowNo(_proxy_thread, "Memory allocation failed -- Thread\n");
  }
  for ( ix=0; ix < _num_pthreads; ix++) {
    // _proxy_thread[ix]=new EventChannelWorker(_evnt_channel, proxy_method);
    _proxy_thread[ix] = new EventChannelBoundWorker(_evnt_channel, proxy_method);
    RDI_AssertAllocThrowNo(_proxy_thread[ix], "Failed to create new proxy thread\n");
    RDIDbgChanLog("New proxy filtering thread: " << _proxy_thread[ix]->id() << '\n');
  }
  // The following is needed if we create unbound threads to start them
  // for ( ix=0; ix < _num_athreads; ix++)
  //    _admin_thread[ix]->start();
  // for ( ix=0; ix < _num_pthreads; ix++)
  //    _proxy_thread[ix]->start();
}

EventChannelDispatch::~EventChannelDispatch()
{
  // If we have bound threads, we should wait till all of them exit to 
  // reclaim their space -- this is not needed for unbound threads ...
  unsigned int ix;
  for ( ix=0; ix < _num_athreads; ix++) {
    _admin_thread[ix]->join(0);
    _admin_thread[ix] = 0;
  }
  RDIDbgChanLog("\t" << _num_athreads << " admin filtering threads terminated\n");
  for ( ix=0; ix < _num_pthreads; ix++) {
    _proxy_thread[ix]->join(0);
    _proxy_thread[ix] = 0;
  }
  if ( _num_pthreads )
    RDIDbgChanLog("\t"<<_num_pthreads<<" proxy filtering threads terminated\n");
  if ( _admin_thread ) delete [] _admin_thread; _admin_thread = 0;
  if ( _proxy_thread ) delete [] _proxy_thread; _proxy_thread = 0;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//    Thread Pool Implementations for Pull Supplier and Push Consumers   //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

typedef void (RDI_PullSupplier::*PullSupplierMethod)(void);

class RDI_PullWorker : public TW_Thread {
public:
  RDI_PullWorker(RDI_PullSupplier* object, PullSupplierMethod method, 
		 TW_PRIORITY_T priority = RDI_PULL_WORKER_PRIORITY);
  void run(void *);
private:
  RDI_PullSupplier*  _object;
  PullSupplierMethod _method;
  RDI_PullWorker() {;}
};

RDI_PullWorker::RDI_PullWorker(RDI_PullSupplier*     object,
			       PullSupplierMethod    method,
			       TW_PRIORITY_T thprio) :
  TW_Thread(NULL, thprio), _object(object), _method(method) 
{;}

void
RDI_PullWorker::run(void *)
{
  (_object->*_method)();
}

// --------------------------------------------------------------------- //

class RDI_PullBoundWorker : public TW_Thread {
public:
  RDI_PullBoundWorker(RDI_PullSupplier* object, PullSupplierMethod method,
                      TW_PRIORITY_T prio = RDI_PULL_WORKER_PRIORITY);
  void* run_undetached(void *);
private:
  RDI_PullSupplier*  _object;
  PullSupplierMethod _method;
  RDI_PullBoundWorker() {;}
};

RDI_PullBoundWorker::RDI_PullBoundWorker(RDI_PullSupplier*     object, 
					 PullSupplierMethod    method,
                      			 TW_PRIORITY_T thprio) :
  TW_Thread(NULL, thprio), _object(object), _method(method) 
{
  start_undetached();
}

void*
RDI_PullBoundWorker::run_undetached(void *)
{
  (_object->*_method)();
  return 0;
}

// --------------------------------------------------------------------- //

#undef WHATFN
#define WHATFN "RDI_PullSupplier::RDI_PullSupplier"
RDI_PullSupplier::RDI_PullSupplier(unsigned int numThreads, 
				   unsigned int periodMSec) :
  _oplock(), _nonempty(&_oplock), _period(periodMSec),
  _threads(0), _nactive(0),  _ndeleted(0), _terminate(0),
  _entries(0), _last_one(0)
{
  { // introduce lock scope so that release occurs before constructor finishes
    TW_SCOPE_LOCK(pullsup_lock, _oplock, "pullsup", WHATFN);
    if ( numThreads ) {
      _threads = new TW_Thread * [ numThreads ];
      RDI_AssertAllocThrowNo(_threads, "Memory allocation failed -- Thread\n");
    }
    for (unsigned int i = 0; i < numThreads; i++) {
      // _threads[i] = new RDI_PullWorker(this, &RDI_PullSupplier::pull_event);
      _threads[i] = new RDI_PullBoundWorker(this, &RDI_PullSupplier::pull_event);
      RDI_AssertAllocThrowNo(_threads[i], "Failed to create new thread\n");
      // If we use unbound threads, we should start the thread using
      // _threads[i]->start();
      RDIDbgChanLog("New pull supplier thread: " << _threads[i]->id() << '\n');
    }
    _nactive = numThreads;
  } // end scope lock
}

#undef WHATFN
#define WHATFN "RDI_PullSupplier::destroy"
void
RDI_PullSupplier::destroy()
{
  { // introduce lock scope
    TW_SCOPE_LOCK(pullsup_lock, _oplock, "pullsup", WHATFN);
    if (_terminate) {
      RDIDbgChanLog("RDI_PullSupplier::destroy called more than once on same obj -- ignored\n");
      return;
    }
    _terminate = 1;
    _nonempty.broadcast();
  } // end lock scope

  // do not hold lock during join
  for (unsigned int ix = 0; ix < _nactive; ix++ ) {
    _threads[ix]->join(0);
    _threads[ix] = 0;
  }

  // When using unbound threads, we should wait till all of them exit
  // to avoid getting core dumps due to invalid memory accesses. Note
  // that in this case _nactive is decremented by the exiting thread.
  //
  // while ( _nactive && (ix++ < 120) ) {
  // 	_nonempty.broadcast();
  // 	TW_SLEEP(1);
  // }
  // 

  // NOTE: no other threads using this any more, do not need to lock again

  RDIDbgChanLog("\t" << _nactive << " pull supplier threads terminated\n");
  if ( _threads ) delete [] _threads; _threads = 0;
  while ( _entries ) {
    ProxyEntry_t* node = _entries;
    _entries = _entries->_next;
    delete node;
  }
}

// Insert a new entry into the list of ProxyConsumer proxies that are
// handled by the threads of this object. The insertion has no effect
// if this object is being destroyed,  the provided reference is NIL,
// or the proxy does not correspond to a pull supplier.

#undef WHATFN
#define WHATFN "RDI_PullSupplier::insert_proxy"
void
RDI_PullSupplier::insert_proxy(RDIProxyPullConsumer* proxy)
{
  RDIDbgChanLog("PullSupplier::insert_proxy called for proxy " << proxy << '\n');
  { // introduce scope lock
    TW_SCOPE_LOCK(pullsup_lock, _oplock, "pullsup", WHATFN);
    if ( _terminate || (! proxy) ) {
      return;
    }
    ProxyEntry_t* node = new ProxyEntry_t(proxy);
    RDI_AssertAllocThrowNo(node, "Memory allocation failed -- ProxyEntry_t\n");
    node->_next = _entries;
    _entries    = node;
    _nonempty.signal();
  } // end scope lock
  RDIDbgChanLog("PullConsumer proxy " << proxy << " to be monitored\n");
}

// Remove an entry from the list of ProxyConsumer proxies handled by
// the threads of this object.  The deletion has no effect when this
// object is being destroyed, the provided reference is NIL, or such
// entry is not found. Otherwise, the entry is marked as deleted.

#undef WHATFN
#define WHATFN "RDI_PullSupplier::remove_proxy"
void
RDI_PullSupplier::remove_proxy(RDIProxyPullConsumer* proxy)
{
  RDIDbgChanLog("PullSupplier::remove_proxy called for proxy " << proxy << '\n');
  ProxyEntry_t* node = 0;

  { // introduce scope lock
    TW_SCOPE_LOCK(pullsup_lock, _oplock, "pullsup", WHATFN);
    if ( (! proxy) || _terminate ) {
      return;
    }
    for ( node = _entries; node != (ProxyEntry_t *) 0; node = node->_next ) {
      if ( node->_proxy == proxy )
	break;
    }
    if ( node == (ProxyEntry_t *) 0 ) {
      return;
    }
    node->_deled = 1;
    if ( ++_ndeleted > 5 ) {
      _gcollect();
    }
  } // end scope lock
  RDIDbgChanLog("PullConsumer proxy " << proxy << " not monitored\n");
}

// This is the code that gets executed by each created thread for as
// long as the object is not being destroyed.

#undef WHATFN
#define WHATFN "RDI_PullSupplier::pull_event"
void
RDI_PullSupplier::pull_event()
{
  ProxyEntry_t*         node= 0;
  CORBA::Boolean        invalid;
  unsigned long wait_s = 0, wait_n = 0;

  while ( 1 ) {
    { // introduce scope lock
      TW_SCOPE_LOCK(pullsup_lock, _oplock, "pullsup", WHATFN);
      while ( (! _terminate) && (! (node = _next_available(&wait_s, &wait_n))) ) {
	if ( (wait_s > 0) || (wait_n > 0) ) {
	  RDIDbgChanLog("pull pool thread doing timedwait(" << wait_s << "," << wait_n <<")\n");
	  TW_CV_TIMEDWAIT(_nonempty, wait_s, wait_n, _oplock, "pullsup", WHATFN);
	} else {
	  RDIDbgChanLog("pull pool thread doing normal wait\n");
	  TW_CV_WAIT(_nonempty, _oplock, "pullsup", WHATFN);
	}
	wait_s = 0; wait_n = 0;
      }
      if ( _terminate ) {
	// _nactive -= 1;    --> When using unbound threads
	goto pull_event_exit;
      }
      node->_inuse = 1;
    } // end lock scope

    // NOTE: we update 'node' and may update '_ndeleted' in the 
    //       following segment without holding the lock.  While
    //       this is not safe in general, it does not affect us
    //       here since 'node->_deled' and 'node->_inuse' are 1
    //       byte each (atomic assignment) and '_ndeleted' does
    //       not have to be 100% accurate

    node->_proxy->pull_event(invalid);
    if ( invalid ) {
      node->_deled = 1;
      _ndeleted   += 1;
    }
    node->_inuse = 0;
  }

 pull_event_exit:
  TW_EXIT();
}

// Locate an entry that is not being used by another thread and is
// still vaild, i.e., has not been deleted.  If no such entry can be
// found, return NULL

RDI_PullSupplier::ProxyEntry_t*
RDI_PullSupplier::_next_available(unsigned long* wait_s, unsigned long* wait_t)
{
  unsigned int numIter = 1;
  if ( (! _entries) )
    return 0;
  while ( numIter < 3 ) {
    if ( (! _last_one) || (! _last_one->_next) ) {
      numIter += 1;
      _last_one = _entries;
    } else {
      _last_one = _last_one->_next;	
    }
    if ( (! _last_one->_inuse) &&
	 (! _last_one->_deled) &&
	 _last_one->_proxy->is_available(wait_s, wait_t) )
      return _last_one;
  }
  return 0;
}

// Iterate over all entries in the list of ProxyConsumer proxies and
// delete those that marked as deleted and are not being used by any
// of the created threads.

void
RDI_PullSupplier::_gcollect()
{
  ProxyEntry_t* node= _entries;
  ProxyEntry_t* prev= 0;

  while ( node ) {
    if ( node->_deled && (! node->_inuse) ) {
      if ( node == _last_one )
	_last_one = prev;
      if ( (! prev) ) {
	_entries = node->_next;
	delete node;
	node = _entries;
      } else {
	prev->_next = node->_next;
	delete node;
	node = prev->_next;
      }
      _ndeleted -= 1;
    } else {
      prev = node;
      node = node->_next;
    }
  }
}

// --------------------------------------------------------------------- //

typedef void (RDI_NotifyConsumer::*NotifyConsumerMethod)(void);

class RDI_NotifyWorker : public TW_Thread {
public:
  RDI_NotifyWorker(RDI_NotifyConsumer* object, NotifyConsumerMethod method, 
		   TW_PRIORITY_T priority = RDI_NOTIFY_WORKER_PRIORITY);
  void run(void *);
private:
  RDI_NotifyConsumer*  _object;
  NotifyConsumerMethod _method;
  RDI_NotifyWorker() {;}
};

RDI_NotifyWorker::RDI_NotifyWorker(RDI_NotifyConsumer*   object,
				   NotifyConsumerMethod  method,
				   TW_PRIORITY_T thprio) :
  TW_Thread(NULL, thprio), _object(object), _method(method) 
{;}

void
RDI_NotifyWorker::run(void *)
{
  (_object->*_method)();
}

// --------------------------------------------------------------------- //

class RDI_NotifyBoundWorker : public TW_Thread {
public:
  RDI_NotifyBoundWorker(RDI_NotifyConsumer* object,NotifyConsumerMethod method,
		        TW_PRIORITY_T priority = RDI_NOTIFY_WORKER_PRIORITY);
  void* run_undetached(void *);
private:
  RDI_NotifyConsumer*  _object;
  NotifyConsumerMethod _method;
  RDI_NotifyBoundWorker() {;}
};

RDI_NotifyBoundWorker::RDI_NotifyBoundWorker(RDI_NotifyConsumer*   object,
					     NotifyConsumerMethod  method,
				   	     TW_PRIORITY_T thprio) :
  TW_Thread(NULL, thprio), _object(object), _method(method) 
{
  start_undetached();
}

void*
RDI_NotifyBoundWorker::run_undetached(void *)
{
  (_object->*_method)();
  return 0;
}

// --------------------------------------------------------------------- //

typedef void (RDI_ChangePool::*ChangePoolMethod)(void);

class RDI_ChangePoolBoundWorker : public TW_Thread {
public:
  RDI_ChangePoolBoundWorker(RDI_ChangePool* object, ChangePoolMethod method,
			    TW_PRIORITY_T priority = RDI_NOTIFY_WORKER_PRIORITY);
  void* run_undetached(void *);
private:
  RDI_ChangePool*   _object;
  ChangePoolMethod  _method;
  RDI_ChangePoolBoundWorker() {;}
};

RDI_ChangePoolBoundWorker::RDI_ChangePoolBoundWorker(RDI_ChangePool*       object,
						     ChangePoolMethod      method,
						     TW_PRIORITY_T thprio) :
  TW_Thread(NULL, thprio), _object(object), _method(method) 
{
  start_undetached();
}

void*
RDI_ChangePoolBoundWorker::run_undetached(void *)
{
  (_object->*_method)();
  return 0;
}

// --------------------------------------------------------------------- //

#undef WHATFN
#define WHATFN "RDI_NotifyConsumer::RDI_NotifyConsumer"
RDI_NotifyConsumer::RDI_NotifyConsumer(unsigned int numThreads) :
  _oplock(),  _nonempty(&_oplock), _threads(0), 
  _nactive(0), _ndeleted(0), _terminate(0), 
  _entries(0), _last_one(0)
{
  { // introduce lock scope so that release occurs before constructor finishes
    TW_SCOPE_LOCK(notifcon_lock, _oplock, "notifcon", WHATFN);
    if ( numThreads ) {
      _threads = new TW_Thread * [ numThreads ];
      RDI_AssertAllocThrowNo(_threads, "Memory allocation failed -- Thread\n");
    }
    for (unsigned int i = 0; i < numThreads; i++) {
      // _threads[i] = new RDI_NotifyWorker(this, &RDI_NotifyConsumer::notify);
      _threads[i] = new RDI_NotifyBoundWorker(this, &RDI_NotifyConsumer::notify);
      RDI_AssertAllocThrowNo(_threads[i], "Failed to create new thread\n");
      // If we use unbound threads, we should start the thread using
      // _threads[i]->start();
      RDIDbgChanLog("New push consumer thread: " << _threads[i]->id() << '\n');
    }
    _nactive = numThreads;
  } // end lock scope
}

#undef WHATFN
#define WHATFN "RDI_NotifyConsumer::destroy"
void
RDI_NotifyConsumer::destroy()
{
  { // introduce lock scope
    TW_SCOPE_LOCK(notifcon_lock, _oplock, "notifcon", WHATFN);
    if (_terminate) {
      RDIDbgChanLog("RDI_NotifyConsumer::destroy called more than once on same obj -- ignored\n");
      return;
    }
    _terminate = 1;
    _nonempty.broadcast();
  } // end lock scope

  // do not hold lock during join
  for (unsigned int ix = 0; ix < _nactive; ix++ ) {
    _threads[ix]->join(0);
    _threads[ix] = 0;
  }
  // When using unbound threads, we should wait till all of them exit
  // to avoid getting core dumps due to invalid memory accesses. Note
  // that in this case _nactive is decremented by the exiting thread.
  // 
  // while ( _nactive && (ix++ < 120) ) {
  //    _nonempty.broadcast();
  //    TW_SLEEP(1);
  // }
  // 

  // NOTE: no other threads using this any more, do not need to lock again

  RDIDbgChanLog("\t" << _nactive << " push consumer threads terminated\n");
  if ( _threads ) delete [] _threads; _threads = 0;
  while ( _entries ) {
    ProxyEntry_t* node = _entries;
    _entries = _entries->_next;
    delete node;
  }
}

// Insert a new entry into the list of ProxySupplier proxies that are
// handled by the threads of this object. The insertion has no effect
// if this object is being destroyed,  the provided reference is NIL,
// or the proxy does not correspond to a push consumer.

#undef WHATFN
#define WHATFN "RDI_NotifyConsumer::insert_proxy"
void
RDI_NotifyConsumer::insert_proxy(RDIProxyPushSupplier* proxy)
{
  RDIDbgChanLog("NotifyConsumer::insert_proxy called for proxy " << proxy << '\n');
  { // introduce scope lock
    TW_SCOPE_LOCK(notifcon_lock, _oplock, "notifcon", WHATFN);
    if ( _terminate || (! proxy) ) {
      return;
    }
    ProxyEntry_t* node = new ProxyEntry_t(proxy);
    RDI_AssertAllocThrowNo(node, "Memory allocation failed -- ProxyEntry_t\n");
    node->_next = _entries;
    _entries    = node;
    _nonempty.signal();
  } // end lock scope
  RDIDbgChanLog("PushSupplier proxy " << proxy << " to be monitored\n");
}

// Remove an entry from the list of ProxySupplier proxies handled by
// the threads of this object.  The deletion has no effect when this
// object is being destroyed, the provided reference is NIL, or such
// entry is not found. Otherwise, the entry is marked as deleted.

#undef WHATFN
#define WHATFN "RDI_NotifyConsumer::remove_proxy"
void
RDI_NotifyConsumer::remove_proxy(RDIProxyPushSupplier* proxy)
{
  RDIDbgChanLog("NotifyConsumer::remove_proxy called for proxy " << proxy << '\n');
  ProxyEntry_t* node = 0;

  { // introduce scope lock
    TW_SCOPE_LOCK(notifcon_lock, _oplock, "notifcon", WHATFN);
    if ( (! proxy) || _terminate ) {
      return;
    }
    for ( node = _entries; node != (ProxyEntry_t *) 0; node = node->_next ) {
      if ( node->_proxy == proxy )
	break;
    }
    if ( node == (ProxyEntry_t *) 0 ) {
      return;
    }
    node->_deled = 1;
    if ( ++_ndeleted > 5 ) {
      _gcollect();
    }
  } // end scope lock
  RDIDbgChanLog("PushSupplier proxy " << proxy << " not monitored\n");
}

// This is the code that gets executed by each created thread for as
// long as the object is not being destroyed.

#undef WHATFN
#define WHATFN "RDI_NotifyConsumer::notify"
void
RDI_NotifyConsumer::notify()
{
  ProxyEntry_t*  node= 0;
  CORBA::Boolean invalid;
  unsigned long wait_s = 0, wait_n = 0;

  while ( 1 ) {
    { // introduce scope lock
      TW_SCOPE_LOCK(notifcon_lock, _oplock, "notifcon", WHATFN);
      while ( (! _terminate) && (! (node = _next_available(&wait_s, &wait_n))) ) {
	if ( (wait_s > 0) || (wait_n > 0) ) {
	  //	    RDIDbgChanLog("push pool thread doing timedwait(" << wait_s << "," << wait_n <<")\n");
	  TW_CV_TIMEDWAIT(_nonempty, wait_s, wait_n, _oplock, "notifcon", WHATFN);
	} else {
	  //	    RDIDbgChanLog("push pool thread doing normal wait\n");
	  TW_CV_WAIT(_nonempty, _oplock, "notifcon", WHATFN);
	}
	wait_s = 0; wait_n = 0;
      }
      if ( _terminate ) {
	// _nactive -= 1;    --> When using unbound threads
	goto notifcon_notify_exit;
      }
      node->_inuse = 1;
    } // end scope lock

    // NOTE: we update 'node' and may update '_ndeleted' in the
    //       following segment without holding the lock.  While
    //       this is not safe in general, it does not affect us
    //       here since 'node->_deled' and 'node->_inuse' are 1
    //       byte each (atomic assignment) and '_ndeleted' does
    //       not have to be 100% accurate

    if ( (! node->_deled) ) {
      node->_proxy->push_event(invalid);
      if ( invalid ) {
	node->_deled = 1;
	_ndeleted   += 1;
      }
    }
    node->_inuse = 0;
    TW_YIELD();
  }

 notifcon_notify_exit:
  TW_EXIT();
}

// Locate an entry that is not being used by another thread and is
// still vaild, i.e., has not been deleted.  If no such entry can be
// found, return NULL

RDI_NotifyConsumer::ProxyEntry_t*
RDI_NotifyConsumer::_next_available(unsigned long* wait_s, unsigned long* wait_t)
{
  RDIDbgChanLog("NotifyConsumer::_next_available called\n");
  unsigned int numIter = 1;
  if ( (! _entries) ) {
    RDIDbgChanLog("NotifyConsumer::_next_available -- no entries, returning\n");
    return 0;
  }
  while ( numIter < 3 ) {
    if ( (! _last_one) || (! _last_one->_next) ) {
      numIter += 1;
      _last_one = _entries;
    } else {
      _last_one = _last_one->_next;
    }
    if ( (! _last_one->_inuse) && 
	 (! _last_one->_deled) &&
	 _last_one->_proxy->has_events(wait_s, wait_t) ) {
      RDIDbgChanLog("NotifyConsumer::_next_available -- found entry for proxy " << _last_one->_proxy << '\n');
      return _last_one;
    }
  }
  RDIDbgChanLog("NotifyConsumer::_next_available -- did 3 iters with no luck, returning\n");
  return 0;
}

// Iterate over all entries in the list of ProxySupplier proxies and
// delete those that marked as deleted and are not being used by any
// of the created threads.

void
RDI_NotifyConsumer::_gcollect()
{
  ProxyEntry_t* node = _entries;
  ProxyEntry_t* prev = 0;
  unsigned int  dsav = 0;

  while ( node ) {
    if ( (! node->_deled) ) {
      prev = node;
      node = node->_next;
      continue;
    } 
    // Node is marked as being deleted. If the node is in use, 
    // we just update the counter of deleted but still needed
    // nodes. Else, we physically destroy the node
    if ( node->_inuse ) {
      dsav += 1;
      prev  = node;
      node  = node->_next;
    } else {
      if ( node == _last_one )
	_last_one = prev;
      if ( (! prev) ) {
	_entries = node->_next;
	delete node;
	node = _entries;
      } else {
	prev->_next = node->_next;
	delete node;
	node = prev->_next;
      }
    }
  }
  _ndeleted = dsav;
}

// --------------------------------------------------------------------- //

#undef WHATFN
#define WHATFN "RDI_ChangePool::RDI_ChangePool"
RDI_ChangePool::RDI_ChangePool(RDI_ChangeKind kind, unsigned int numThreads) :
  _oplock(), _kind(kind), _nonempty(&_oplock), _threads(0),
  _nproxies(0), _ndeleted(0), _nactive(0), _terminate(0), 
  _c_head(0), _c_tail(0)
{
  { // introduce lock scope so that release occurs before constructor finishes
    TW_SCOPE_LOCK(changepool_lock, _oplock, "changepool", WHATFN);
    if ( numThreads ) {
      _threads = new TW_Thread * [ numThreads ];
      RDI_AssertAllocThrowNo(_threads, "Memory allocation failed -- Thread\n");
    }
    for (unsigned int i = 0; i < numThreads; i++) {
      // _threads[i] = new RDI_ChangePoolWorker(this, &RDI_ChangePool::notify);
      _threads[i] = new RDI_ChangePoolBoundWorker(this, &RDI_ChangePool::notify);
      RDI_AssertAllocThrowNo(_threads[i], "Failed to create new thread\n");
      // If we use unbound threads, we should start the thread using
      // _threads[i]->start();
      RDIDbgChanLog("New ChangePool thread: " << _threads[i]->id() << '\n');
    }
    _nactive = numThreads;
    // _c_tail always has _valid == false (no added/deled defined yet)
    // it is used as a holding pen for proxies that are up-to-date
    _c_head = new CEntry_t();
    RDI_AssertAllocThrowNo(_c_head, "Memory allocation failed -- CEntry_t\n");
    _c_tail = _c_head;
  } // end scope lock
}

#undef WHATFN
#define WHATFN "RDI_ChangePool::destroy"
void
RDI_ChangePool::destroy()
{
  { // introduce lock scope
    TW_SCOPE_LOCK(changepool_lock, _oplock, "changepool", WHATFN);
    if (_terminate) {
      RDIDbgChanLog("RDI_ChangePool::destroy called more than once on same obj -- ignored\n");
      return;
    }
    _terminate = 1;
    _nonempty.broadcast();
  } // end lock scope

  // do not hold lock during join
  for (unsigned int ix = 0; ix < _nactive; ix++ ) {
    _threads[ix]->join(0);
    _threads[ix] = 0;
  }

  // When using unbound threads, we should wait till all of them exit
  // to avoid getting core dumps due to invalid memory accesses. Note
  // that in this case _nactive is decremented by the exiting thread.
  // 
  // while ( _nactive && (ix++ < 120) ) {
  //    _nonempty.broadcast();
  //    TW_SLEEP(1);
  // }
  // 
  // NOTE: no other threads using this any more, do not need to lock again

  RDIDbgChanLog("\t" << _nactive << " ChangePool threads terminated\n");
  if ( _threads ) delete [] _threads; _threads = 0;
  while ( _c_head ) {
    CEntry_t* next_entry = _c_head->_c_next;
    ProxyEntry_t* node = _c_head->_pxy_head;
    while (node) {
      ProxyEntry_t* next_node = node->_pxy_next;
      delete node;
      node = next_node;
    }
    delete _c_head;
    _c_head = next_entry;
  }
  _c_tail = 0;
}

#undef WHATFN
#define WHATFN "RDI_ChangePool::insert_change"
// the channel informs us of a new subscription or offer change
// the change is only recorded if there are some registered proxies that
// must be sent this message
// ** We replace the internal form of *::*   with  ""::"%ALL"
// because the spec seems to say this is required.  Note that the
// replacement is done in a copy of the type sequence that is only used
// by the ChangePool (it would be a disaster to replace *::* in other
// internal data structures because we test for *::* internally)
//
void
RDI_ChangePool::insert_change(RDI_ChangeKind kind,
			      const CosN::EventTypeSeq& added,
			      const CosN::EventTypeSeq& deled)
{
  RDI_Assert(_kind == kind, "Inserting wrong kind of change into a change pool\n");
  RDIDbgChanLog("RDI_ChangePool::insert_change called with added = " << added << " and  deled = " << deled << '\n');
  { // introduce lock scope
    TW_SCOPE_LOCK(changepool_lock, _oplock, "changepool", WHATFN);
    if ( _terminate || (_nproxies == 0) ) {
      // terminate or no non-deleted proxies
      return;
    }
    // make current _c_tail a valid entry
    _c_tail->_valid = 1;
    // sequence<T> operator= performs T::operator= which for String types causes CORBA::string_dup
    _c_tail->_added = added;
    _c_tail->_deled = deled;
    // Replace *::* with ""::"%ALL"
    RDI_EventType::externalize_sequence(_c_tail->_added);
    RDI_EventType::externalize_sequence(_c_tail->_deled);
    // append a new _c_tail for parking proxies that have
    // been sent the most recent change message
    CEntry_t* new_tail = new CEntry_t();
    RDI_AssertAllocThrowNo(new_tail, "Memory allocation failed -- CEntry_t\n");
    _c_tail->_c_skip = new_tail;
    _c_tail->_c_next = new_tail;
    _c_tail = new_tail;
    // let blocked threads know there is a change msg to send
    // to all registered proxies
    _nonempty.broadcast();
  } // end lock scope
}

#undef WHATFN
#define WHATFN "RDI_ChangePool::insert_proxy"
// Put the ChangePool in charge of sending change
// messages to proxy.  Has no effect if pool is being destroyed
// or if the provided reference is NIL.
void
RDI_ChangePool::insert_proxy(RDIProxyConsumer* proxy)
{
  RDIDbgChanLog("ChangePool::insert_proxy called for consumer proxy " << proxy << '\n');
  RDI_Assert(_kind == RDI_ChangeKind_SubscriptionChange, "insert_proxy passed wrong kind of proxy\n");
  if ( (! proxy) ) return;
  { // introduce lock scope
    TW_SCOPE_LOCK(changepool_lock, _oplock, "changepool", WHATFN);
    if ( _terminate ) {
      return;
    }
    ProxyEntry_t* node = new ProxyEntry_t(proxy);
    RDI_AssertAllocThrowNo(node, "Memory allocation failed -- ProxyEntry_t\n");
    // append node to proxy list stored at _c_tail
    if (_c_tail->_pxy_tail) {
      _c_tail->_pxy_tail->_pxy_next = node;
    }
    _c_tail->_pxy_tail = node;
    if (_c_tail->_pxy_head == (ProxyEntry_t*)0) {
      _c_tail->_pxy_head = node;
    }
    node->_pxy_next = 0;
    _nproxies++;
    RDIDbgChanLog("ProxyConsumer proxy " << proxy << " will get subscription_change msgs, _nproxies = " << _nproxies << '\n');
  } // end lock scope
}

#undef WHATFN
#define WHATFN "RDI_ChangePool::insert_proxy"
void
RDI_ChangePool::insert_proxy(RDIProxySupplier* proxy)
{
  RDIDbgChanLog("ChangePool::insert_proxy called for supplier proxy " << proxy << '\n');
  RDI_Assert(_kind == RDI_ChangeKind_OfferChange, "insert_proxy passed wrong kind of proxy\n");
  if ( (! proxy) ) return;
  { // introduce lock scope
    TW_SCOPE_LOCK(changepool_lock, _oplock, "changepool", WHATFN);
    if ( _terminate ) {
      return;
    }
    ProxyEntry_t* node = new ProxyEntry_t(proxy);
    RDI_AssertAllocThrowNo(node, "Memory allocation failed -- ProxyEntry_t\n");
    // append node to proxy list stored at _c_tail
    if (_c_tail->_pxy_tail) {
      _c_tail->_pxy_tail->_pxy_next = node;
    }
    _c_tail->_pxy_tail = node;
    if (_c_tail->_pxy_head == (ProxyEntry_t*)0) {
      _c_tail->_pxy_head = node;
    }
    node->_pxy_next = 0;
    _nproxies++;
    RDIDbgChanLog("ProxySuplier proxy " << proxy << " will get offer_change msgs, _nproxies = " << _nproxies << '\n');
  } // end lock scope
}

#undef WHATFN
#define WHATFN "RDI_ChangePool::remove_proxy"
// ChangePool is no longer in charge of sending change
// messages to proxy.  Has no effect if pool is being destroyed
// or if the provided reference is NIL.
void
RDI_ChangePool::remove_proxy(RDIProxyConsumer* proxy)
{
  RDIDbgChanLog("ChangePool::remove_proxy called for consumer proxy " << proxy << '\n');
  RDI_Assert(_kind == RDI_ChangeKind_SubscriptionChange, "remove_proxy passed wrong kind of proxy\n");
  CEntry_t* entry = 0;
  ProxyEntry_t* node = 0;
  if ( (! proxy) ) return;
  { // introduce lock scope
    TW_SCOPE_LOCK(changepool_lock, _oplock, "changepool", WHATFN);
    if ( _terminate ) {
      return;
    }
    for ( entry = _c_head; entry != (CEntry_t*)0; entry = entry->_c_next) {
      for ( node = entry->_pxy_head; node != (ProxyEntry_t *) 0; node = node->_pxy_next ) {
	if ( (node->_cproxy == proxy) && (! node->_deled) ) {
	  node->_deled = 1;
	  _nproxies--;
	  _ndeleted++;
	  RDIDbgChanLog("ProxyConsumer proxy " << proxy
			<< " will no longer get subscription_change msgs, _nproxies = "
			<< _nproxies << '\n');
	  if (_ndeleted >= 10) {
	    _gcollect();
	  }
	  return;
	}
      }
    }
  } // end lock scope 
}

#undef WHATFN
#define WHATFN "RDI_ChangePool::remove_proxy"
void
RDI_ChangePool::remove_proxy(RDIProxySupplier* proxy)
{
  RDIDbgChanLog("ChangePool::remove_proxy called for supplier proxy " << proxy << '\n');
  RDI_Assert(_kind == RDI_ChangeKind_OfferChange, "remove_proxy passed wrong kind of proxy\n");
  CEntry_t* entry = 0;
  ProxyEntry_t* node = 0;
  if ( (! proxy) ) return;
  { // introduce lock scope
    TW_SCOPE_LOCK(changepool_lock, _oplock, "changepool", WHATFN);
    if ( _terminate ) {
      return;
    }
    for ( entry = _c_head; entry != (CEntry_t*)0; entry = entry->_c_next) {
      for ( node = entry->_pxy_head; node != (ProxyEntry_t *) 0; node = node->_pxy_next ) {
	if ( (node->_sproxy == proxy) && (! node->_deled) ) {
	  node->_deled = 1;
	  _nproxies--;
	  _ndeleted++;
	  RDIDbgChanLog("ProxySupplier proxy " << proxy
			<< " will no longer get offer_change msgs, _nproxies = "
			<< _nproxies << '\n');
	  if (_ndeleted >= 10) {
	    _gcollect();
	  }
	  return;
	}
      }
    }
  } // end lock scope
}

#undef WHATFN
#define WHATFN "RDI_ChangePool::notify"
// This is the code that gets executed by each created thread for as
// long as the object is not being destroyed.
void
RDI_ChangePool::notify()
{
  CORBA::Boolean     invalid;
  ProxyEntry_t*      node        = 0; // next node to work on
  CEntry_t*          found_entry = 0; // entry where node was found

  while ( 1 ) {
    { // introduce lock scope
      TW_SCOPE_LOCK(changepool_lock, _oplock, "changepool", WHATFN);
      while ( (! _terminate) &&
	      ( (_nproxies == 0) || (! (node = _next_available(&found_entry))) ) ) {
	TW_CV_WAIT(_nonempty, _oplock, "changepool", WHATFN);
      }
      if ( _terminate ) {
	// _nactive -= 1;    --> When using unbound threads
	goto changepool_notify_exit;
      }
      // At this point we know _nproxies > 0 and node has been set.
      //
      // _next_available unlinked node from found_entry's proxy list
      // and placed it in a later entry's proxy list (where its next change
      // msg will be processed).  It also set _inuse
      // to 1 so that another thread will not start on node. It also
      // incremented found_entry->_num_inuse by 1; entry's added/deled/c_skip
      // are not allowed to change (or be gc'd) while this # is positive.
      // This thread is in charge of decrementing found_entry->_num_inuse
      // and setting node->_inuse to 0.

    } // end lock scope

    // note we do not hold lock across send_*_change call
    invalid = 0;
    if ( (! node->_deled) ) {
      if (_kind == RDI_ChangeKind_SubscriptionChange) {
	RDIDbgChanLog("Sending subscription_change to ProxyConsumer proxy " << node->_cproxy << '\n');
	invalid = node->_cproxy->send_subscription_change(found_entry->_added, found_entry->_deled);
      } else {
	RDIDbgChanLog("Sending offer_change to ProxySupplier proxy " << node->_sproxy << '\n');
	invalid = node->_sproxy->send_offer_change(found_entry->_added, found_entry->_deled);
      }
    }

    { // introduce lock scope
      TW_SCOPE_LOCK(changepool_lock, _oplock, "changepool", WHATFN);

      found_entry->_num_inuse--;
      node->_inuse = 0;
      if ( invalid ) {
	if ( (! node->_deled) ) {
	  RDIDbgChanLog("send_*_change failed, removing proxy from a CPool\n");
	  node->_deled = 1;
	  _nproxies--;
	  _ndeleted++;
	  if (_kind == RDI_ChangeKind_SubscriptionChange) {
	    RDIDbgChanLog("ProxyConsumer proxy " << node->_cproxy << " will no longer get subscription_change msgs, _nproxies = " << _nproxies << '\n');
	  } else {
	    RDIDbgChanLog("ProxySupplier proxy " << node->_sproxy << " will no longer get offer_change msgs, _nproxies = " << _nproxies << '\n');
	  }
	  if (_ndeleted >= 10) {
	    _gcollect();
	  }
	}
      } else { // give another thread a shot at choosing this valid node
	_nonempty.signal();
      }
    } // end lock scope
    TW_YIELD();
  }

 changepool_notify_exit:
  TW_EXIT();
}

// Locate earliest node that is not being used by another thread and is
// still vaild, i.e., has not been deleted.  If no such node can be
// found, return NULL.  Otherwise, set *found_entry to the SCEntry where 
// the node was found, move the node to its next location
// (where its next change msg will be placed), set node->_inuse
// to 1, and increment (*found_entry)->_num_inuse by 1.
//
// NB ** ASSUMES _oplock is held

RDI_ChangePool::ProxyEntry_t*
RDI_ChangePool::_next_available(CEntry_t** found_entry)
{
  for (CEntry_t* c_entry = _c_head;
       (c_entry != (CEntry_t*)0) && c_entry->_valid;
       c_entry = c_entry->_c_next) {
    ProxyEntry_t* p_entry = c_entry->_pxy_head;
    ProxyEntry_t* prev_node = 0;
    while (p_entry != (ProxyEntry_t *) 0) {
      if ( p_entry->_deled ) {
	RDIDbgChanLog("RDI_ChangePool::_next_available: skipping deleted node\n");
      } else if ( p_entry->_inuse ) {
	RDIDbgChanLog("RDI_ChangePool::_next_available: skipping 'inuse' node\n");
      } else {
	RDIDbgChanLog("RDI_ChangePool::_next_available: found 1 -- unlink+move to _c_skip CEntry \n");
	(*found_entry) = c_entry;
	CEntry_t* skip_entry = c_entry->_c_skip;
	c_entry->_num_inuse++;
	p_entry->_inuse = 1;
	// unlink node
	if (prev_node) {
	  RDIDbgChanLog("Removing proxy " << p_entry << " from prev_node " <<
		   prev_node << " of CEntry " << c_entry << '\n');
	  prev_node->_pxy_next = p_entry->_pxy_next;
	} else {
	  RDIDbgChanLog("Removing proxy " << p_entry << " from head of CEntry " <<
		   c_entry << " proxy chain\n");
	  c_entry->_pxy_head = p_entry->_pxy_next;
	}
	// adjust _pxy_tail if necessary
	if (c_entry->_pxy_tail == p_entry) {
	  RDIDbgChanLog("Adjusting _pxy_tail of CEntry " << c_entry << " new val = " << prev_node << '\n');
	  c_entry->_pxy_tail = prev_node;
	}
	// append node to (*found_entry)->_c_skip proxy list
	RDIDbgChanLog("Adding proxy " << p_entry << " to tail of CEntry " << c_entry << " ->_c_skip "  << skip_entry << " proxy list\n");
	if (skip_entry->_pxy_tail) {
	  skip_entry->_pxy_tail->_pxy_next = p_entry;
	}
	skip_entry->_pxy_tail = p_entry;
	if (skip_entry->_pxy_head == (ProxyEntry_t*)0) {
	  skip_entry->_pxy_head = p_entry;
	}
	p_entry->_pxy_next = 0;
	RDIDbgChanLog("Returning result = " << p_entry << '\n');
	return p_entry; // sucess
      }
      // advance p_entry
      prev_node = p_entry;
      p_entry = p_entry->_pxy_next;
    }
  }
  // failed for now
  RDIDbgChanLog("RDI_ChangePool::_next_available: failed to find a node\n");
  return 0; // failure
}

// ASSUMES _oplock is held!!
void
RDI_ChangePool::_gcollect() {
  RDIDbgChanLog("RDI_ChangPool::_gcollect called with _ndeleted == " << _ndeleted << '\n'); 

  for (CEntry_t* c_ptr = _c_head; c_ptr != (CEntry_t*)0; c_ptr = c_ptr->_c_next) {
    // first GC deleted node(s) sitting at _pxy_head
    ProxyEntry_t* p_ptr = 0;
    while ( (p_ptr = c_ptr->_pxy_head) && (! p_ptr->_inuse) && p_ptr->_deled ) {
      c_ptr->_pxy_head = p_ptr->_pxy_next; // unlinks p_ptr
      RDIDbgChanLog("RDI_ChangePool::_gcollect: discarding deleted proxy entry " << p_ptr << '\n');
      delete p_ptr;
      _ndeleted--;
    }
    if ( (p_ptr = c_ptr->_pxy_head) ) {
      // walk chain at _pxy_head to GC interior nodes
      while (p_ptr->_pxy_next) {
	ProxyEntry_t* prev_node = p_ptr;
	p_ptr = p_ptr->_pxy_next;
	if ( (! p_ptr->_inuse) && p_ptr->_deled ) {
	  prev_node->_pxy_next = p_ptr->_pxy_next; // unlinks p_ptr
	  if (c_ptr->_pxy_tail == p_ptr) { // fix _pxy_tail
	    c_ptr->_pxy_tail = prev_node;
	  }
	  RDIDbgChanLog("RDI_ChangePool::_gcollect: discarding deleted proxy entry " << p_ptr << '\n');
	  delete p_ptr;
	  _ndeleted--;
	  p_ptr = prev_node; // p_ptr must be node in linked list at top of while loop
	}
      }
    } else { // nothing left in proxy chain, fix _pxy_tail
      c_ptr->_pxy_tail = 0;
    }
  }
  // GC any valid CEntry at _c_head with no proxies in its proxy list
  // as long as _num_inuse is zero.
  while ( _c_head->_valid && (! _c_head->_pxy_head) &&
	  _c_head->_c_next && (_c_head->_num_inuse == 0) ) {
    CEntry_t* c_tmp = _c_head;
    _c_head = _c_head->_c_next;
    RDIDbgChanLog("RDI_ChangePool::_gcollect: discarding unnecessary _c_head " << c_tmp << '\n');
    delete c_tmp;
  }
  RDIDbgChanLog("RDI_ChangePool::_gcollect: after GC, _ndeleted == " << _ndeleted << '\n');
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

EventChannelWorker::EventChannelWorker(EventChannel_i*       evchan,
				       EventChannelMethod    method,
				       TW_PRIORITY_T trprio) :
  TW_Thread(NULL, trprio), _channel(evchan), _method(method)
{;}

void
EventChannelWorker::run(void *)
{
  (_channel->*_method)();
}

EventChannelBoundWorker::EventChannelBoundWorker(EventChannel_i*       evchan,
						 EventChannelMethod    method,
						 TW_PRIORITY_T trprio) :
  TW_Thread(NULL, trprio), _channel(evchan), _method(method)
{ start_undetached(); }

void*
EventChannelBoundWorker::run_undetached(void *)
{
  (_channel->*_method)();
  return 0;
}

////////////////////////////////////////
// Logging

RDIstrstream&
operator<<(RDIstrstream& str, const RDI_ProxyState& s)
{
  switch ( s ) {
  case RDI_UnknownState: str << " Unknown State "; break;
  case RDI_NotConnected: str << " Not Connected "; break;
  case RDI_Connected:    str << " Connected     "; break;
  case RDI_Disconnected: str << " Disconnected  "; break;
  case RDI_Exception:    str << " Exception     "; break;
  }
  return str;
}

// Helper for use in FOREACH call below
static void
CAdminGroupMgr_log_cadmin(int idx, int len, ConsumerAdmin_i *admin, void *arg)
{
  RDIstrstream &str = *((RDIstrstream*)arg);
  str << "\tConsumerAdmin_i " << admin << " ID ";
  str.setw(4); str << admin->_admin_id() << " #Proxies " << admin->NumProxies() << '\n';
}

RDIstrstream&
operator<<(RDIstrstream& str, const CAdminGroup& agr) {
  str << "    CAdminGroup: " << (void*)(&agr) << '\n';
  RDI_SAFELIST_SAFE_FOREACH((CAdminGroup*)(&agr), CAdminGroupMgr_log_cadmin, (void*)(&str),
			    "cadmingroup", "operator<<(RDIstrstream &, const CAdminGroup &)");
  return str;
}

#undef WHATFN
#define WHATFN "CAdminGroupMgr::log_output"
RDIstrstream&
CAdminGroupMgr::log_output(RDIstrstream& str)
{
  TW_SCOPE_LOCK(cadmingroupmgr_lock, _oplock, "cadmingroupmgr", WHATFN);
  str << "-------------------\nCAdminGroupMgr\n-------------------\nBrief Listing of CAdmin Groups:\n";
  for ( unsigned int ix=0; ix < _length; ix++ ) {
    str << _groups[ix];
  }
  return str;
}

