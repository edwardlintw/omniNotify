// -*- Mode: C++; -*-
//                              File      : CosEventProxy.cc
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
//    Implementation of CORBA Event Service Proxies
//

#include "thread_wrappers.h"
#include "CosNotifyShorthands.h"
#include "CosNfyUtils.h"
#include "RDI.h"
#include "RDIStringDefs.h"
#include "CosEventChannelAdmin_i.h"
#include "CosNotifyChannelAdmin_i.h"
#include "RDIOplocksMacros.h"

// ------------------------------------------------------------ //
// EventProxyPushConsumer_i                                     //
// ------------------------------------------------------------ //

EventProxyPushConsumer_i::EventProxyPushConsumer_i(SupplierAdmin_i* admin, 
					 	   EventChannel_i*  evchn,
						   const CosNA::ProxyID&  prxid) :
  _oplockptr(0), _my_name(admin->L_my_name()),
  _channel(evchn), _myadmin(admin), 
  _nevents(0), _pxstate(RDI_NotConnected), _pserial(prxid)
{
  RDI_OPLOCK_INIT("EventProxyPushConsumer");
  char buf[20];
  sprintf(buf, "proxy%ld", prxid);
  _my_name.length(_my_name.length()+1);
  _my_name[_my_name.length()-1] = (const char*)buf;
  _supplier = CosEvC::PushSupplier::_nil();
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

EventProxyPushConsumer_i::~EventProxyPushConsumer_i()
{
  // RDIDbgForceLog("XXX_REMOVE  ~EventProxyPushConsumer_i called for proxy " << _pserial << '\n');
  RDI_OPLOCK_DESTROY_CHECK("EventProxyPushConsumer_i");
}

#undef WHATFN
#define WHATFN "EventProxyPushConsumer_i::connect_push_supplier"
void
EventProxyPushConsumer_i::connect_push_supplier
(CosEvC::PushSupplier_ptr supplier WRAPPED_IMPLARG)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_NotConnected ) {
    throw CosEvCA::AlreadyConnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _pxstate  = RDI_Connected;
  if ( ! CORBA::is_nil(supplier) ) {
    _supplier = CosEvC::PushSupplier::_duplicate(supplier);
  }
  RDIDbgCosCPxyLog("CosEvent push supplier connected to " << (void*)this << '\n');
}

#undef WHATFN
#define WHATFN "EventProxyPushConsumer_i::push"
void
EventProxyPushConsumer_i::push(const CORBA::Any& data WRAPPED_IMPLARG)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_Connected ) {
    throw CosEvC::Disconnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  if ( _channel->new_any_event(data) ) {
    throw CORBA::IMP_LIMIT(0, CORBA::COMPLETED_NO);
  }
  _nevents += 1;
}

#undef WHATFN
#define WHATFN "EventProxyPushConsumer_i::disconnect_push_consumer"
void
EventProxyPushConsumer_i::disconnect_push_consumer(WRAPPED_IMPLARG_VOID)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { RDI_THROW_INV_OBJREF; }
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "EventProxyPushConsumer_i::disconnect_client_and_dispose"
void
EventProxyPushConsumer_i::disconnect_client_and_dispose(RDI_LocksHeld&  held,
							CORBA::Boolean  remove_proxy_from_admin)
{
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { return; }
  _disconnect_client_and_dispose(held, remove_proxy_from_admin, proxy_lock.dispose_info);
}

#ifndef NO_OBJ_GC
#undef WHATFN
#define WHATFN "EventProxyPushConsumer_i::obj_gc"
CORBA::Boolean
EventProxyPushConsumer_i::obj_gc(RDI_TimeT curtime, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { return 0; }
  if (_pxstate == RDI_Disconnected) { return 0; }
  if ( (deadConProxy && _pxstate == RDI_Connected && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadConProxy)) ||
       (deadOtherProxy && _pxstate != RDI_Connected && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadOtherProxy)) ) {
#ifndef NDEBUG
    if (_pxstate == RDI_Connected) {
      RDIDbgCosCPxyLog("GC destroys connected EventProxyPushConsumer_i proxy " << _pserial
		       << " curtime = " << curtime.fmt_local()
		       << " last_use = " << _last_use.fmt_local()
		       << "(diff = " << RDI_TIMET_DIFF_IN_SECS(_last_use, curtime)
		       << ") DeadConProxyInterval = " << deadConProxy << '\n');
    } else {
      RDIDbgCosCPxyLog("GC destroys non-connected EventProxyPushConsumer_i proxy " << _pserial
		       << " curtime = " << curtime.fmt_local()
		       << " last_use = " << _last_use.fmt_local()
		       << "(diff = " << RDI_TIMET_DIFF_IN_SECS(_last_use, curtime)
		       << ") DeadOtherProxyInterval = " << deadOtherProxy << '\n');
    }
#endif
    _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
    return 1;
  }
  return 0;
}
#endif

#undef WHATFN
#define WHATFN "EventProxyPushConsumer_i::_disconnect_client_and_dispose"
// *** caller must have bumped scope lock, held.cproxy set
// Effect: initiate impl dispose
void
EventProxyPushConsumer_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
							 CORBA::Boolean            remove_proxy_from_admin,
							 WRAPPED_DISPOSEINFO_PTR&  dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgCosCPxyLog("EventProxyPushConsumer_i::disconnect_client_and_dispose called twice on same proxy!\n");
    return;
  }
  _pxstate = RDI_Disconnected; // acts as guard -- only one thread does the following
  // Wait for inuse to drop to one.  This ensures that any call that
  // bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_proxy_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    {
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);
      // this is unlocked for rest of scope
      _myadmin->remove_proxy(held, this);
    }
    if (!held.cproxy) {
      RDI_Fatal("EventProxyPushConsumer_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }
  _supplier = CosEvC::PushSupplier::_nil();
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

// ------------------------------------------------------------ //
// EventProxyPullConsumer_i                                     //
// ------------------------------------------------------------ //

/** We use unbound threads here for we assume that in the majority
  * of configuations a thread pool will be used for pulling events
  * from suppliers and pushing events to consumers.
  */

class EventPullWorker : public TW_Thread {
public:
  typedef void (EventProxyPullConsumer_i::*Method)(void);
  EventPullWorker(EventProxyPullConsumer_i* p, Method m) :
    TW_Thread(NULL,PRIORITY_NORMAL), _proxy(p), _method(m) {;}
  void run(void *)      { (_proxy->*_method)(); }
private:
  EventProxyPullConsumer_i* _proxy;
  Method                    _method;
  EventPullWorker()  {;}
};

// ------------------------------------------------------------ //

EventProxyPullConsumer_i::EventProxyPullConsumer_i(SupplierAdmin_i* admin, 
                                                   EventChannel_i*  evchn,
						   const CosNA::ProxyID&  prxid) :
  _oplockptr(0), _my_name(admin->L_my_name()),
  _channel(evchn), _myadmin(admin), 
  _pworker(0), _thrdone(0), _nevents(0), 
  _pxstate(RDI_NotConnected), _pserial(prxid),_timeout_s(0), _timeout_n(0)
{
  RDI_OPLOCK_INIT("EventProxyPullConsumer");
  char buf[20];
  sprintf(buf, "proxy%ld", prxid);
  _my_name.length(_my_name.length()+1);
  _my_name[_my_name.length()-1] = (const char*)buf;
  _supplier = CosEvC::PullSupplier::_nil();
  // When the number of pull threads allocated at the channel level is
  // 0, each proxy uses its own thread to pull events from its supplier
  if ( _channel->pull_threads() == 0 ) {
    _pworker = new EventPullWorker(this, &EventProxyPullConsumer_i::_pull);
    RDI_AssertAllocThrowNo(_pworker, "Memory allocation failed -- Thread\n");
    _pworker->start();
    _thrdone = 0;
    RDIDbgCosCPxyLog("Pull thread for proxy " << (void*)this << " -- " << _pworker->id() << '\n');
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

EventProxyPullConsumer_i::~EventProxyPullConsumer_i()
{
  // RDIDbgForceLog("XXX_REMOVE  ~EventProxyPullConsumer_i called for " << _pserial << '\n');
  RDI_OPLOCK_DESTROY_CHECK("EventProxyPullConsumer_i");
}

#undef WHATFN
#define WHATFN "EventProxyPullConsumer_i::connect_pull_supplier"
void
EventProxyPullConsumer_i::connect_pull_supplier(CosEvC::PullSupplier_ptr supplier WRAPPED_IMPLARG)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( CORBA::is_nil(supplier) ) {
    throw CORBA::BAD_PARAM(0, CORBA::COMPLETED_NO);
  }
  if ( _pxstate != RDI_NotConnected ) {
    throw CosEvCA::AlreadyConnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _pxstate  = RDI_Connected;
  _supplier = CosEvC::PullSupplier::_duplicate(supplier);
  if ( _pworker ) {
    RDI_OPLOCK_SIGNAL;
  }
  RDI_PullSupplier* cps = _channel->pull_supplier();
  if ( cps ) {
    cps->signal_pull_threads();
  }
  RDIDbgCosCPxyLog("CosEvent pull supplier connected to " << (void*)this << '\n');
}

#undef WHATFN
#define WHATFN "EventProxyPullConsumer_i::is_available"
// is_available determines whether a pull should occur.
// in this case the requirement is that either the pull interval is zero
// or an interval timeout has occurred
CORBA::Boolean
EventProxyPullConsumer_i::is_available(unsigned long* wait_s, unsigned long* wait_n)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return 0);
  if ( _pxstate != RDI_Connected ) {
    return 0;
  }

  unsigned long pull_interval_s, pull_interval_n;
  unsigned long time_s, time_n;

  _channel->pull_period_s_n(pull_interval_s, pull_interval_n);
  if ( (pull_interval_s == 0) && (pull_interval_n == 0) ) {
    _timeout_s = 0; _timeout_n = 0;
    return 1; // pull as fast as possible
  }

  TW_GET_TIME(&time_s, &time_n);

  if ( (_timeout_s == 0) && (_timeout_n == 0) ) {
    // proxy has not established timeout yet : set it now
    TW_GET_TIME(&_timeout_s, &_timeout_n, pull_interval_s, pull_interval_n);
  }
  if ( (time_s > _timeout_s) || ((time_s == _timeout_s) && (time_n > _timeout_n)) ) {
    return 1; // interval timeout occurred for this proxy
  }
  // update wait_s/_n to reflect the future timeout point of this proxy
  if ( ( ((*wait_s) == 0) && ((*wait_n) == 0)                   ) ||
       ( ((*wait_s) > _timeout_s)                               ) ||
       ( ((*wait_s) == _timeout_s) && ((*wait_n) > _timeout_n)) ) {
    (*wait_s) = _timeout_s;
    (*wait_n) = _timeout_n;
  }
  return 0;
}

#undef WHATFN
#define WHATFN "EventProxyPullConsumer_i::pull_event"
// REQUIREMENT: A thread from pull pool only calls this method
// if is_available() is true and it thinks state is connected
// (For safety we verify that these conditions actually hold.)
void
EventProxyPullConsumer_i::pull_event(CORBA::Boolean& invalid)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { return; }
  if ( _pxstate != RDI_Connected ) {
    return;
  }

  unsigned long pull_interval_s, pull_interval_n;
  unsigned long time_s, time_n;
  _channel->pull_period_s_n(pull_interval_s, pull_interval_n);

  CORBA::Any*     event = 0;
  CORBA::Boolean  hasev = 0;
  invalid = 0;

  // sanity check: if pull interval is positive, we should only be called if a timeout occurred
  if (pull_interval_s || pull_interval_n) {
    TW_GET_TIME(&time_s, &time_n);
    if ((_timeout_s == 0) && (_timeout_n == 0)) { // timeout not set; set it now
      TW_GET_TIME(&_timeout_s, &_timeout_n, pull_interval_s, pull_interval_n);
    } 
    if ( (time_s < _timeout_s) || ((time_s == _timeout_s) && (time_n < _timeout_n)) ) {
      RDIDbgCosCPxyLog("** INTERNAL ERROR: ProxyPullConsumer_i::pull_event called too soon\n");
      return;
    }
  }

  // update timeout before releasing OPLOCK -- this means we do the update
  // before doing the try_pull (seems OK)
  if (pull_interval_s || pull_interval_n) {
    TW_GET_TIME(&_timeout_s, &_timeout_n, pull_interval_s, pull_interval_n);
  } else {
    _timeout_s = 0; _timeout_n = 0;
  }


  CORBA::Boolean outcall_worked = 0;
  // do not hold OPLOCK across try_pull call
  {
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);
    // this is unlocked for rest of scope
    try {
      event = _supplier->try_pull(hasev);
      outcall_worked = 1;
#ifndef NO_OBJ_GC
      _last_use.set_curtime();
#endif
    } catch ( CORBA::INV_OBJREF& e ) {
      RDIDbgCosCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Invalid object reference\n");
    } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
      RDIDbgCosCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Consumer object does not exist\n");
    } catch ( CORBA::COMM_FAILURE& e ) {
      RDIDbgCosCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Communication Failure\n");
    } catch (...) {
      RDIDbgCosCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Exception while pulling event from consumer\n");
    }
  }
  if (!held.cproxy) {
    RDI_Fatal("EventProxyPullConsumer_i::pull_event [**unexpected REACQUIRE failure**]\n");
  }
  // add new event to channel, but only if we are still connected
  if ( (_pxstate == RDI_Connected) && outcall_worked && hasev && event ) {
    _nevents += 1;
    _channel->new_any_event(*event);
  }
  // new_any_event copies event, so we delete regardless
  if ( event ) {
    delete event; event = 0;
  }

  if (_pxstate != RDI_Connected) {
    // disconnected/disposed during try_pull -- do nothing else with my state
  } else {
    if (!outcall_worked) { // exception
      _pxstate = RDI_Exception;
      invalid  = 1;
    }
  }
}

#undef WHATFN
#define WHATFN "EventProxyPullConsumer_i::_pull"
void
EventProxyPullConsumer_i::_pull()
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { return; }

  CORBA::Any*    event = 0;
  CORBA::Boolean hasev = 0, update_timeout = 0;
  CORBA::Boolean outcall_worked;
  CORBA::Boolean do_yield = 0;
  unsigned long pull_interval_s, pull_interval_n;
  unsigned long time_s, time_n;

  // invariant: oplock is held at top of loop
  while ( 1 ) {
    do_yield = 1;
    while ( 1 ) {
      // must recompute these here because they can change across a wait/timedwait
      _channel->pull_period_s_n(pull_interval_s, pull_interval_n);
      if (pull_interval_s || pull_interval_n) {
	// if timeout not set, or if update_timeout true due to pull, recompute timeout
	if ( update_timeout || ((_timeout_s == 0) && (_timeout_n == 0)) ) {
	  TW_GET_TIME(&_timeout_s, &_timeout_n, pull_interval_s, pull_interval_n);
	}
      } else {
	_timeout_s = 0; _timeout_n = 0;
      }
      update_timeout = 0;
      // breaking from inner loop requires a pull interval timeout (if set)
      // and state must be RDI_Connected
      if ((_pxstate!=RDI_NotConnected) && (_pxstate!=RDI_Connected)) {
	break; // also break on exceptional state
      }
      if (_pxstate==RDI_Connected) {
	if ((_timeout_s == 0) && (_timeout_n == 0)) {
	  break; // pulling as fast as possible so pull now
	}
	TW_GET_TIME(&time_s, &time_n);
	if ( (time_s > _timeout_s) || ((time_s == _timeout_s) && (time_n > _timeout_n)) ) {
	  break; // pull interval has passed so pull now
	}
	do_yield = 0;
	RDI_OPLOCK_TIMEDWAIT(_timeout_s, _timeout_n, WHATFN); // must wait for pull interval
      } else {
	do_yield = 0;
	RDI_OPLOCK_WAIT(WHATFN); // must wait for _pxstate to change
      }
    }
    if ( _pxstate != RDI_Connected ) {
      RDI_THREAD_EXIT(pull_exit, RDIDbgCosCPxy, RDIDbgCosCPxy_nm, "pull thread\n");
    }

    // do not hold OPLOCK across try_pull call
    outcall_worked = 0;
    {
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);
      // this is unlocked for rest of scope

      // if we did not wait, yield here (wait or yield each time through outer loop)
      if (do_yield) {
	TW_YIELD();
      }
      try {
	event = _supplier->try_pull(hasev);
	outcall_worked = 1;
#ifndef NO_OBJ_GC
	_last_use.set_curtime();
#endif
      } catch ( CORBA::INV_OBJREF& e ) {
	RDIDbgCosCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Invalid object reference\n");
      } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
	RDIDbgCosCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Consumer object does not exist\n");
      } catch ( CORBA::COMM_FAILURE& e ) {
	RDIDbgCosCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Communication Failure\n");
      } catch (...) {
	RDIDbgCosCPxyLog("Pull pool calling pull_event for proxy " << (void*)this <<
			 " - Exception while pulling event from consumer\n");
      }
    } // end of lock release scope
    if (!held.cproxy) {
      RDI_Fatal("EventProxyPullConsumer_i::_pull [**unexpected REACQUIRE failure**]\n");
    }

    update_timeout = 1; // force update of timeout at top of loop

    // add new event to channel, but only if we are still connected
    if ( (_pxstate == RDI_Connected) && outcall_worked && hasev && event ) {
      _nevents += 1;
      _channel->new_any_event(*event);
    }
    // new_any_event copies event so we can delete it regardless
    if ( event ) {
      delete event; event = 0;
    }

    if (_pxstate != RDI_Connected) {
      // disconnected/disposed during try_pull -- do nothing else with my state
    } else {
      if (!outcall_worked) { // exception
	_pxstate = RDI_Exception;
      }
    }
  }

 pull_exit:
  TW_EXIT();
}

#undef WHATFN
#define WHATFN "EventProxyPullConsumer_i::disconnect_pull_consumer"
void
EventProxyPullConsumer_i::disconnect_pull_consumer(WRAPPED_IMPLARG_VOID)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { RDI_THROW_INV_OBJREF; }
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "EventProxyPullConsumer_i::disconnect_client_and_dispose"
void
EventProxyPullConsumer_i::disconnect_client_and_dispose(RDI_LocksHeld&  held,
							CORBA::Boolean remove_proxy_from_admin)
{
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { return; }
  _disconnect_client_and_dispose(held, remove_proxy_from_admin, proxy_lock.dispose_info);
}

#ifndef NO_OBJ_GC
#undef WHATFN
#define WHATFN "EventProxyPullConsumer_i::obj_gc"
CORBA::Boolean
EventProxyPullConsumer_i::obj_gc(RDI_TimeT curtime, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { return 0; }
  if (_pxstate == RDI_Disconnected) { return 0; }
  if ( (deadConProxy && _pxstate == RDI_Connected && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadConProxy)) ||
       (deadOtherProxy && _pxstate != RDI_Connected && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadOtherProxy)) ) {
#ifndef NDEBUG
    if (_pxstate == RDI_Connected) {
      RDIDbgCosCPxyLog("GC destroys connected EventProxyPullConsumer_i proxy " << _pserial
		       << " curtime = " << curtime.fmt_local()
		       << " last_use = " << _last_use.fmt_local()
		       << "(diff = " << RDI_TIMET_DIFF_IN_SECS(_last_use, curtime)
		       << ") DeadConProxyInterval = " << deadConProxy << '\n');
    } else {
      RDIDbgCosCPxyLog("GC destroys non-connected EventProxyPullConsumer_i proxy " << _pserial
		       << " curtime = " << curtime.fmt_local()
		       << " last_use = " << _last_use.fmt_local()
		       << "(diff = " << RDI_TIMET_DIFF_IN_SECS(_last_use, curtime)
		       << ") DeadOtherProxyInterval = " << deadOtherProxy << '\n');
    }
#endif
    _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
    return 1;
  }
  return 0;
}
#endif

#undef WHATFN
#define WHATFN "EventProxyPullConsumer_i::_disconnect_client_and_dispose"
// *** caller must have bumped scope lock, held.cproxy set
// Effect: initiate impl dispose
void
EventProxyPullConsumer_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
							 CORBA::Boolean            remove_proxy_from_admin,
							 WRAPPED_DISPOSEINFO_PTR&  dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgCosCPxyLog("EventProxyPullConsumer_i::disconnect_client_and_dispose called twice on same proxy!\n");
    return;
  }
  _pxstate = RDI_Disconnected; // acts as guard; following only entered by 1 thread
  // Wait for inuse to drop to one.  This ensures that any call that
  // bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_proxy_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    {
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);
      // this is unlocked for rest of scope
      _myadmin->remove_proxy(held, this);
    }
    if (!held.cproxy) {
      RDI_Fatal("EventProxyPullConsumer_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }
  _supplier = CosEvC::PullSupplier::_nil();
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

// ------------------------------------------------------------ //
// EventProxyPushSupplier_i                                     //
// ------------------------------------------------------------ //

/** We use unbound threads here for we assume that in the majority
  * of configuations a thread pool will be used for pulling events
  * from suppliers and pushing events to consumers.
  */

class EventPushWorker : public TW_Thread {
public:
  typedef void (EventProxyPushSupplier_i::*Method)(void);
  EventPushWorker(EventProxyPushSupplier_i* p, Method m) :
    TW_Thread(NULL,PRIORITY_NORMAL), _proxy(p), _method(m) {;}
  void run(void *)      { (_proxy->*_method)(); }
private:
  EventProxyPushSupplier_i* _proxy;
  Method                    _method;
  EventPushWorker()  {;}
};

// ------------------------------------------------------------ //

EventProxyPushSupplier_i::EventProxyPushSupplier_i(ConsumerAdmin_i* admin, 
						   EventChannel_i*  evchn,
						   const CosNA::ProxyID&  prxid) :
  _oplockptr(0), _my_name(admin->L_my_name()),
  _channel(evchn), _myadmin(admin), 
  _pworker(0),  _thrdone(0), _nevents(0), 
  _pxstate(RDI_NotConnected), _pserial(prxid), _cnfqueue()
{
  RDI_OPLOCK_INIT("EventProxyPushSupplier");
  char buf[20];
  sprintf(buf, "proxy%ld", prxid);
  _my_name.length(_my_name.length()+1);
  _my_name[_my_name.length()-1] = (const char*)buf;
  _consumer = CosEvC::PushConsumer::_nil();
  // When the number of push threads allocated at the channel level is
  // 0, each proxy uses its own thread to pull events from its supplier
  if ( _channel->push_threads() == 0 ) {
    _pworker = new EventPushWorker(this, &EventProxyPushSupplier_i::_push);
    RDI_AssertAllocThrowNo(_pworker, "Memory allocation failed -- Thread\n");
    _pworker->start();
    _thrdone = 0;
    RDIDbgCosSPxyLog("Pull thread for proxy " << (void*)this << " -- " << _pworker->id() << '\n');
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

EventProxyPushSupplier_i::~EventProxyPushSupplier_i()
{
  // RDIDbgForceLog("XXX_REMOVE  ~EventProxyPushSupplier_i called for proxy " << _pserial << '\n');
  RDI_OPLOCK_DESTROY_CHECK("EventProxyPushSupplier_i");
}

#undef WHATFN
#define WHATFN "EventProxyPushSupplier_i::connect_push_consumer"
void
EventProxyPushSupplier_i::connect_push_consumer(CosEvC::PushConsumer_ptr consumer WRAPPED_IMPLARG)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( CORBA::is_nil(consumer) ) {
    throw CORBA::BAD_PARAM(0, CORBA::COMPLETED_NO);
  }
  if ( _pxstate != RDI_NotConnected ) {
    throw CosEvCA::AlreadyConnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _pxstate  = RDI_Connected;
  _consumer = CosEvC::PushConsumer::_duplicate(consumer);
  if ( _pworker ) {
    RDI_OPLOCK_SIGNAL;
  }
  RDI_NotifyConsumer* cpc = _channel->push_consumer();
  if ( cpc ) {
    cpc->signal_push_threads();
  }
  RDIDbgCosSPxyLog("CosEvent push consumer connected to " << (void*)this << '\n');
}

#undef WHATFN
#define WHATFN "EventProxyPushSupplier_i::add_event"
void
EventProxyPushSupplier_i::add_event(RDI_StructuredEvent* event)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return);
  if ( (_pxstate == RDI_Connected) && event ) {
    RDI_SEVENT_INCR_REF_COUNTER(event, WHATFN);
    _cnfqueue.insert_tail(event);
    if ( _pworker ) {
      RDI_OPLOCK_SIGNAL;
    }
    RDI_NotifyConsumer* cpc = _channel->push_consumer();
    if ( cpc ) {
      cpc->signal_push_threads();
    }
  }
}

#undef WHATFN
#define WHATFN "EventProxyPushSupplier_i::push_event"
void
EventProxyPushSupplier_i::push_event(CORBA::Boolean& invalid)
{
  RDI_StructuredEvent* event = 0;
  unsigned int         qsize = 0;
  RDI_LocksHeld        held  = { 0 }; 

  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { return; }


  invalid = 0;
  if ( (_pxstate != RDI_Connected) || (_cnfqueue.length() == 0) ) {
    return;
  }
  event = _cnfqueue.get_head();
  qsize = _cnfqueue.length() - 1;
  _cnfqueue.remove_head();
  _nevents += 1;

  CORBA::Boolean outcall_worked = 0;
  // do not hold OPLOCK across push call
  {
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);

    try {
#ifndef WRAPPED_ANY_THREADSAFE
      const CosN::StructuredEvent& cosev = event->get_cos_event();
      if ( RDI_STR_EQ(event->get_type_name(), "%ANY") ) {
	// Need to copy remainder_of_body to produce thread-safe marshalling without
	// holding event lock.  An efficient form of copy is used here (shares buffer).
	// Need to hold event lock across copy.
	RDI_SEVENT_PROT_UNPROT(event, WHATFN,
			       CORBA::Any safe(cosev.remainder_of_body.type(), (void*)(cosev.remainder_of_body.value())),
			       _consumer->push(safe));
      } else {
	// Construction of any_event here avoids need to worry about thread-safe marshalling.
	// Need to hold event lock across construction.
	CORBA::Any any_event;
	{ // introduce event lock scope
	  RDI_SEVENT_SCOPE_LOCK(event, WHATFN);
	  any_event <<= cosev;
	} // end event lock scope
	_consumer->push(any_event);
      }
#else
      if ( RDI_STR_EQ(event->get_type_name(), "%ANY") ) {
	// Can marshall remainder of body safely without event lock
	_consumer->push(event->get_cos_event().remainder_of_body);
      } else {
	// Can construct any_event safely without event lock
	CORBA::Any any_event;
	any_event <<= event->get_cos_event();
	_consumer->push(any_event);
      }
#endif
      outcall_worked = 1;
#ifndef NO_OBJ_GC
      _last_use.set_curtime();
#endif
    } catch ( CORBA::INV_OBJREF& e ) {
      RDIDbgCosSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Invalid object reference\n");
    } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
      RDIDbgCosSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Consumer object does not exist\n");
    } catch ( CORBA::COMM_FAILURE& e ) {
      RDIDbgCosSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Communication Failure\n");
    } catch (...) {
      RDIDbgCosSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Exception while pushing event to consumer\n");
    }
    RDI_SEVENT_DECR_REF_COUNTER(event, WHATFN);
  } // end of lock release scope
  if (!held.sproxy) {
    RDI_Fatal("EventProxyPushSupplier_i::push_event [**unexpected REACQUIRE failure**]\n");
  }

  if (_pxstate != RDI_Connected) {
    // disconnected/disposed during push -- do nothing else with my state
  } else {
    if (outcall_worked) {
      _channel->incr_num_notifications(qsize);
    } else { // exception
      _pxstate = RDI_Exception;
      _clear_cnfqueue(); // Remove all events
      invalid  = 1;
    }
  }
}

#undef WHATFN
#define WHATFN "EventProxyPushSupplier_i::_push"
void
EventProxyPushSupplier_i::_push()
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { return; }

  RDI_StructuredEvent* event = 0;
  unsigned int         qsize = 0;
  CORBA::Boolean       do_yield = 0;
  CORBA::Boolean       outcall_worked;

  // invariant : OPLOCK held at top of loop
  while ( 1 ) {
    do_yield = 1;
    while ( ((_pxstate == RDI_Connected) && (_cnfqueue.length() == 0)) ||
	    (_pxstate == RDI_NotConnected) ) {
      do_yield = 0;
      RDI_OPLOCK_WAIT(WHATFN);
    }
    if ( _pxstate != RDI_Connected ) {
      RDI_THREAD_EXIT(push_exit, RDIDbgCosSPxy, RDIDbgCosCPxy_nm, "push thread\n");
    }
    event = _cnfqueue.get_head();
    qsize = _cnfqueue.length() - 1;
    _cnfqueue.remove_head();
    _nevents += 1;

    // do not hold OPLOCK across push call
    outcall_worked = 0;
    {
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);

      // if we did not wait, yield here (wait or yield each time through outer loop)
      if (do_yield) {
	TW_YIELD();
      }

      try {
#ifndef WRAPPED_ANY_THREADSAFE
	const CosN::StructuredEvent& cosev = event->get_cos_event();
	if ( RDI_STR_EQ(event->get_type_name(), "%ANY") ) {
	  // Need to copy remainder_of_body to produce thread-safe marshalling without
	  // holding event lock.  An efficient form of copy is used here (shares buffer).
	  // Need to hold event lock across copy.
	  RDI_SEVENT_PROT_UNPROT(event, WHATFN,
				 CORBA::Any safe(cosev.remainder_of_body.type(), (void*)(cosev.remainder_of_body.value())),
				 _consumer->push(safe));
	} else {
	  // Construction of any_event here avoids need to worry about thread-safe marshalling.
	  // Need to hold event lock across construction.
	  CORBA::Any any_event;
	  { // introduce event lock scope
	    RDI_SEVENT_SCOPE_LOCK(event, WHATFN);
	    any_event <<= cosev;
	  } // end event lock scope
	  _consumer->push(any_event);
	}
#else
	if ( RDI_STR_EQ(event->get_type_name(), "%ANY") ) {
	  // Can marshall remainder of body safely without event lock
	  _consumer->push(event->get_cos_event().remainder_of_body);
	} else {
	  // Can construct any_event safely without event lock
	  CORBA::Any any_event;
	  any_event <<= event->get_cos_event();
	  _consumer->push(any_event);
	}
#endif
	outcall_worked = 1;
#ifndef NO_OBJ_GC
	_last_use.set_curtime();
#endif
      } catch ( CORBA::INV_OBJREF& e ) {
	RDIDbgCosSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Invalid object reference\n");
      } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
	RDIDbgCosSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Consumer object does not exist\n");
      } catch ( CORBA::COMM_FAILURE& e ) {
	RDIDbgCosSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Communication Failure\n");
      } catch (...) {
	RDIDbgCosSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Exception while pushing event to consumer\n");
      }
      RDI_SEVENT_DECR_REF_COUNTER(event, WHATFN);
    } // end of lock release scope
    if (!held.sproxy) {
      RDI_Fatal("EventProxyPushSupplier_i::_push [**unexpected REACQUIRE failure**]\n");
    }

    if (_pxstate != RDI_Connected) {
      // disconnected/disposed during push -- do nothing else with my state
    } else {
      if (outcall_worked) {
	_channel->incr_num_notifications(qsize);
      } else { // exception
	_pxstate = RDI_Exception;
	_clear_cnfqueue(); // Remove all events
      }
    }
  }

 push_exit:
  TW_EXIT();
}

#undef WHATFN
#define WHATFN "EventProxyPushSupplier_i::disconnect_push_supplier"
void
EventProxyPushSupplier_i::disconnect_push_supplier(WRAPPED_IMPLARG_VOID)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; }
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "EventProxyPushSupplier_i::disconnect_client_and_dispose"
void
EventProxyPushSupplier_i::disconnect_client_and_dispose(RDI_LocksHeld&  held,
							CORBA::Boolean  remove_proxy_from_admin)
{
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { return; }
  _disconnect_client_and_dispose(held, remove_proxy_from_admin, proxy_lock.dispose_info);
}

#ifndef NO_OBJ_GC
#undef WHATFN
#define WHATFN "EventProxyPushSuplier_i::obj_gc"
CORBA::Boolean
EventProxyPushSupplier_i::obj_gc(RDI_TimeT curtime, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { return 0; }
  if (_pxstate == RDI_Disconnected) { return 0; }
  if ( (deadConProxy && _pxstate == RDI_Connected && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadConProxy)) ||
       (deadOtherProxy && _pxstate != RDI_Connected && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadOtherProxy)) ) {
#ifndef NDEBUG
    if (_pxstate == RDI_Connected) {
      RDIDbgCosSPxyLog("GC destroys connected EventProxyPushSupplier_i proxy " << _pserial
		       << " curtime = " << curtime.fmt_local()
		       << " last_use = " << _last_use.fmt_local()
		       << "(diff = " << RDI_TIMET_DIFF_IN_SECS(_last_use, curtime)
		       << ") DeadConProxyInterval = " << deadConProxy << '\n');
    } else {
      RDIDbgCosSPxyLog("GC destroys non-connected EventProxyPushSupplier_i proxy " << _pserial
		       << " curtime = " << curtime.fmt_local()
		       << " last_use = " << _last_use.fmt_local()
		       << "(diff = " << RDI_TIMET_DIFF_IN_SECS(_last_use, curtime)
		       << ") DeadOtherProxyInterval = " << deadOtherProxy << '\n');
    }
#endif
    _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
    return 1;
  }
  return 0;
}
#endif

#undef WHATFN
#define WHATFN "EventProxyPushSupplier_i::_disconnect_client_and_dispose"
// *** caller must have bumped scope lock, held.sproxy set
// Effect: initiate impl dispose
void
EventProxyPushSupplier_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
							 CORBA::Boolean            remove_proxy_from_admin,
							 WRAPPED_DISPOSEINFO_PTR&  dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgCosSPxyLog("EventProxyPushSupplier_i::disconnect_client_and_dispose called twice on same proxy!\n");
    return;
  }
  _pxstate = RDI_Disconnected; // acts as guard; following only entered by 1 thread
  // Wait for inuse to drop to one.  This ensures that any call that
  // bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_proxy_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    {
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
      // this is unlocked for rest of scope
      _myadmin->remove_proxy(held, this);
    }
    if (!held.sproxy) {
      RDI_Fatal("EventProxyPushSupplier_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }
  _consumer = CosEvC::PushConsumer::_nil();
  _clear_cnfqueue(); // Remove all events
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

// Assumes lock is held
#undef WHATFN
#define WHATFN "EventProxyPushSupplier_i::_clear_cnfqueue"
void EventProxyPushSupplier_i::_clear_cnfqueue()
{
  // Remove all events
  RDI_StructuredEvent* event = 0;
  while ( (event = _cnfqueue.get_head()) ) {
    _cnfqueue.remove_head();
    RDI_SEVENT_DECR_REF_COUNTER(event, WHATFN);
  }
}

// ------------------------------------------------------------ //
// EventProxyPullSupplier_i                                     //
// ------------------------------------------------------------ //

EventProxyPullSupplier_i::EventProxyPullSupplier_i(ConsumerAdmin_i* admin, 
						   EventChannel_i*  evchn,
						   const CosNA::ProxyID&  prxid) :
  _oplockptr(0), _my_name(admin->L_my_name()),
  _channel(evchn), _myadmin(admin), _nevents(0), 
  _pxstate(RDI_NotConnected), _pserial(prxid), _cnfqueue()
{
  RDI_OPLOCK_INIT("EventProxyPullSupplier");
  char buf[20];
  sprintf(buf, "proxy%ld", prxid);
  _my_name.length(_my_name.length()+1);
  _my_name[_my_name.length()-1] = (const char*)buf;
  _consumer = CosEvC::PullConsumer::_nil();
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

EventProxyPullSupplier_i:: ~EventProxyPullSupplier_i()
{
  // RDIDbgForceLog("XXX_REMOVE  ~EventProxyPullSupplier_i called for proxy " << _pserial << '\n');
  RDI_OPLOCK_DESTROY_CHECK("EventProxyPullSupplier_i");
}

#undef WHATFN
#define WHATFN "EventProxyPullSupplier_i::connect_pull_consumer"
void
EventProxyPullSupplier_i::connect_pull_consumer(CosEvC::PullConsumer_ptr consumer WRAPPED_IMPLARG)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_NotConnected ) {
    throw CosEvCA::AlreadyConnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _pxstate  = RDI_Connected;
  if ( ! CORBA::is_nil(consumer) ) {
    _consumer = CosEvC::PullConsumer::_duplicate(consumer);
  }
  RDIDbgCosSPxyLog("CosEvent pull consumer connected to " << (void*)this << '\n');
}

#undef WHATFN
#define WHATFN "EventProxyPullSupplier_i::add_event"
void
EventProxyPullSupplier_i::add_event(RDI_StructuredEvent* event)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return);
  if ( (_pxstate == RDI_Connected) && event ) {
    RDI_SEVENT_INCR_REF_COUNTER(event, WHATFN);
    _cnfqueue.insert_tail(event);
    RDI_OPLOCK_SIGNAL;
  }
}

#undef WHATFN
#define WHATFN "EventProxyPullSupplier_i::pull"
CORBA::Any*
EventProxyPullSupplier_i::pull(WRAPPED_IMPLARG_VOID)
{
  RDI_OPLOCK_BUMP_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate == RDI_NotConnected ) {
    throw CosEvC::Disconnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CORBA::ULong qsize = 0;
  while ( (_pxstate == RDI_Connected) && ((qsize = _cnfqueue.length()) == 0) ) {
    RDI_OPLOCK_WAIT(WHATFN);
  }
  if ( _pxstate != RDI_Connected ) {
    throw CosEvC::Disconnected();
  }
  RDI_StructuredEvent* event = _cnfqueue.get_head();
  const CosN::StructuredEvent& cosev = event->get_cos_event();
  _cnfqueue.remove_head();
  _nevents += 1;
  RDIDbgCosSPxyLog("pull event " << event << " from queue of " << (void*)this << '\n');

  // Given that we need to hold event lock to decrement ref counter,
  // may as well hold it across res construction
  // (accounts for !WRAPPED_ANY_THREADSAFE case)
  CORBA::Any* res = 0;
  { // introduce event lock scope
    RDI_SEVENT_SCOPE_LOCK(event_lock, event, WHATFN);
    if ( RDI_STR_EQ(event->get_type_name(), "%ANY") ) {
      res = new CORBA::Any(cosev.remainder_of_body);
    } else {
      res = new CORBA::Any;
      (*res) <<= cosev;
    }
    event->decr_ref_counter_lock_held();
  } // end event lock scope

  _channel->incr_num_notifications(qsize);
  return res;
}

#undef WHATFN
#define WHATFN "EventProxyPullSupplier_i::try_pull"
CORBA::Any*
EventProxyPullSupplier_i::try_pull(CORBA::Boolean& has_event WRAPPED_IMPLARG)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_Connected ) {
    throw CosEvC::Disconnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CORBA::ULong qsize = _cnfqueue.length();
  if (qsize == 0 ) {
    has_event = 0;
    return new CORBA::Any();
  }
  RDI_StructuredEvent* event = _cnfqueue.get_head();
  const CosN::StructuredEvent& cosev = event->get_cos_event();
  _cnfqueue.remove_head();
  _nevents += 1;
  has_event = 1;
  RDIDbgCosSPxyLog("try_pull event " << event << " from queue of " << (void*)this << '\n');

  // Given that we need to hold event lock to decrement ref counter,
  // may as well hold it across res construction
  // (accounts for !WRAPPED_ANY_THREADSAFE case)
  CORBA::Any* res = 0;
  { // introduce event lock scope
    RDI_SEVENT_SCOPE_LOCK(event_lock, event, WHATFN);
    if ( RDI_STR_EQ(event->get_type_name(), "%ANY") ) {
      res = new CORBA::Any(cosev.remainder_of_body);
    } else {
      res = new CORBA::Any;
      (*res) <<= cosev;
    }
    event->decr_ref_counter_lock_held();
  } // end event lock scope

  _channel->incr_num_notifications(qsize);
  return res;
}

#undef WHATFN
#define WHATFN "EventProxyPullSupplier_i::disconnect_pull_supplier"
void
EventProxyPullSupplier_i::disconnect_pull_supplier(WRAPPED_IMPLARG_VOID)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; }
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "EventProxyPullSupplier_i::disconnect_client_and_dispose"
void
EventProxyPullSupplier_i::disconnect_client_and_dispose(RDI_LocksHeld&  held,
							CORBA::Boolean  remove_proxy_from_admin)
{
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { return; }
  _disconnect_client_and_dispose(held, remove_proxy_from_admin, proxy_lock.dispose_info);
}

#ifndef NO_OBJ_GC
#undef WHATFN
#define WHATFN "EventProxyPullSupplier_i::obj_gc"
CORBA::Boolean
EventProxyPullSupplier_i::obj_gc(RDI_TimeT curtime, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { return 0; }
  if (_pxstate == RDI_Disconnected) { return 0; }
  if ( (deadConProxy && _pxstate == RDI_Connected && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadConProxy)) ||
       (deadOtherProxy && _pxstate != RDI_Connected && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadOtherProxy)) ) {
#ifndef NDEBUG
    if (_pxstate == RDI_Connected) {
      RDIDbgCosSPxyLog("GC destroys connected EventProxyPullSupplier_i proxy " << _pserial
		       << " curtime = " << curtime.fmt_local()
		       << " last_use = " << _last_use.fmt_local()
		       << "(diff = " << RDI_TIMET_DIFF_IN_SECS(_last_use, curtime)
		       << ") DeadConProxyInterval = " << deadConProxy << '\n');
    } else {
      RDIDbgCosSPxyLog("GC destroys non-connected EventProxyPullSupplier_i proxy " << _pserial
		       << " curtime = " << curtime.fmt_local()
		       << " last_use = " << _last_use.fmt_local()
		       << "(diff = " << RDI_TIMET_DIFF_IN_SECS(_last_use, curtime)
		       << ") DeadOtherProxyInterval = " << deadOtherProxy << '\n');
    }
#endif
    _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
    return 1;
  }
  return 0;
}
#endif

#undef WHATFN
#define WHATFN "EventProxyPullSupplier_i::_disconnect_client_and_dispose"
// *** caller must have bumped scope lock, held.sproxy set
// Effect: initiate impl dispose
void
EventProxyPullSupplier_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
							 CORBA::Boolean            remove_proxy_from_admin,
							 WRAPPED_DISPOSEINFO_PTR&  dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgCosSPxyLog("EventProxyPullSupplier_i::disconnect_client_and_dispose called twice on same proxy!\n");
    return;
  }
  _pxstate = RDI_Disconnected; // acts as guard -- only one thread does the following
  // Wait for inuse to drop to one.  This ensures that any outstanding blocked pull call
  // or any other call that bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_proxy_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    {
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
      // this is unlocked for rest of scope
      _myadmin->remove_proxy(held, this);
    }
    if (!held.sproxy) {
      RDI_Fatal("EventProxyPullSupplier_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }
  _consumer = CosEvC::PullConsumer::_nil();
  _clear_cnfqueue(); // Remove all events
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

// Assumes lock is held
#undef WHATFN
#define WHATFN "EventProxyPullSupplier_i::_clear_cnfqueue"
void EventProxyPullSupplier_i::_clear_cnfqueue()
{
  // Remove all events
  RDI_StructuredEvent* event = 0;
  while ( (event = _cnfqueue.get_head()) ) {
    _cnfqueue.remove_head();
    RDI_SEVENT_DECR_REF_COUNTER(event, WHATFN);
  }
}

////////////////////////////////////////
// Logging

RDIstrstream& EventProxyPushConsumer_i::log_output(RDIstrstream& str) const
{
  str << (void*)this << " CosEvent ProxyPushConsumer";
  str << _pxstate;
  return str << " #Push "<< _nevents;
}

RDIstrstream& EventProxyPullConsumer_i::log_output(RDIstrstream& str) const
{
  str << (void*)this << " CosEvent ProxyPullConsumer";
  str << _pxstate;
  return str << "#Push "<< _nevents;
}

RDIstrstream& EventProxyPushSupplier_i::log_output(RDIstrstream& str) const
{
  str << (void*)this << " CosEvent ProxyPushSupplier";
  str << _pxstate;
  return str << "QSize "<< _cnfqueue.length() << " #Push "<< _nevents;
}

RDIstrstream& EventProxyPullSupplier_i::log_output(RDIstrstream& str) const
{
  str << (void*)this << " CosEvent ProxyPullSupplier";
  str << _pxstate;
  return str << "QSize "<< _cnfqueue.length() << " #Push "<< _nevents;
}

////////////////////////////////////////
// Interactive

AttN::IactSeq* EventProxyPushConsumer_i::children(CORBA::Boolean only_cleanup_candidates)
{
  AttN::IactSeq* ren = new AttN::IactSeq;
  if ( ren == (AttN::IactSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  ren->length(0);
  return ren;
}

#undef WHATFN
#define WHATFN "EventProxyPushConsumer_i::safe_cleanup"
CORBA::Boolean
EventProxyPushConsumer_i::safe_cleanup()
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { RDI_THROW_INV_OBJREF; } 
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  if (_pxstate == RDI_Connected) { return 0; } // do not destroy connected proxy
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
  return 1;
}

#undef WHATFN
#define WHATFN "EventProxyPushConsumer_i::my_name"
AttN::NameSeq*
EventProxyPushConsumer_i::my_name( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  AttN::NameSeq* res = new AttN::NameSeq(_my_name);
  if ( res == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  return res;
}

#undef WHATFN
#define WHATFN "EventProxyPushConsumer_i::child_names"
AttN::NameSeq*
EventProxyPushConsumer_i::child_names( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  AttN::NameSeq* names = new AttN::NameSeq;
  if ( names == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  names->length(0);
  return names;
}

void EventProxyPushConsumer_i::out_commands(RDIstrstream& str)
{
  str << "omniNotify CosEvent ProxyPushConsumer commands:\n"
      << "  up                : change target to proxy's admin\n"
      << "  debug             : show debugging information for this proxy\n";
}

char*
EventProxyPushConsumer_i::do_command(const char* cmnd, CORBA::Boolean& success,
				     CORBA::Boolean& target_changed,
				     AttN_Interactive_outarg next_target  WRAPPED_IMPLARG )
{
  { // make sure the object is valid, update _last_use
    RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
    _last_use.set_curtime();
#endif
  } // end lock scope

  RDIParseCmd p(cmnd);
  success = 1;
  target_changed = 0;
  if (p.argc == 0) { return CORBA_STRING_DUP("");  }

  RDIstrstream str;
  if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "help")) {
    out_commands(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "debug")) {
    str << "----------------------------------------------------------------------\n";
    str << "Debug info for " << _my_name << '\n';
    str << "----------------------------------------------------------------------\n";
    log_output(str);
    str << '\n';
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "up")) {
    target_changed = 1;
    next_target = WRAPPED_IMPL2OREF(AttN::SupplierAdmin, _myadmin);
    str << "\nomniNotify: new target ==> " << _my_name[_my_name.length()-2] << '\n';
  } else {
    str << "Invalid command: " << cmnd << "\n";
    success = 0;
  }
  RDIRptInteractiveLog(_my_name << " received command: " << cmnd << "\nResult:\n" << str.buf());
  // this is the only safe way to return a string?
  return CORBA_STRING_DUP(str.buf());
}

AttN::IactSeq*
EventProxyPullConsumer_i::children(CORBA::Boolean only_cleanup_candidates)
{
  AttN::IactSeq* ren = new AttN::IactSeq;
  if ( ren == (AttN::IactSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  ren->length(0);
  return ren;
}

#undef WHATFN
#define WHATFN "EventProxyPullConsumer_i::safe_cleanup"
CORBA::Boolean
EventProxyPullConsumer_i::safe_cleanup()
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { RDI_THROW_INV_OBJREF; } 
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  if (_pxstate == RDI_Connected) { return 0; } // do not destroy connected proxy
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
  return 1;
}

#undef WHATFN
#define WHATFN "EventProxyPullConsumer_i::my_name"
AttN::NameSeq*
EventProxyPullConsumer_i::my_name( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  AttN::NameSeq* res = new AttN::NameSeq(_my_name);
  if ( res == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  return res;
}

#undef WHATFN
#define WHATFN "EventProxyPullConsumer_i::child_names"
AttN::NameSeq*
EventProxyPullConsumer_i::child_names( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  AttN::NameSeq* names = new AttN::NameSeq;
  if ( names == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  names->length(0);
  return names;
}

void
EventProxyPullConsumer_i::out_commands(RDIstrstream& str)
{
  str << "omniNotify CosEvent ProxyPullConsumer commands:\n"
      << "  up                : change target to proxy's admin\n"
      << "  debug             : show debugging information for this proxy\n";
}

char*
EventProxyPullConsumer_i::do_command(const char* cmnd, CORBA::Boolean& success,
				     CORBA::Boolean& target_changed,
				     AttN_Interactive_outarg next_target  WRAPPED_IMPLARG )
{
  { // make sure the object is valid, update _last_use
    RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
    _last_use.set_curtime();
#endif
  } // end lock scope

  RDIParseCmd p(cmnd);
  success = 1;
  target_changed = 0;
  if (p.argc == 0) { return CORBA_STRING_DUP("");  }

  RDIstrstream str;
  if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "help")) {
    out_commands(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "debug")) {
    str << "----------------------------------------------------------------------\n";
    str << "Debug info for " << _my_name << '\n';
    str << "----------------------------------------------------------------------\n";
    log_output(str);
    str << '\n';
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "up")) {
    target_changed = 1;
    next_target = WRAPPED_IMPL2OREF(AttN::SupplierAdmin, _myadmin);
    str << "\nomniNotify: new target ==> " << _my_name[_my_name.length()-2] << '\n';
  } else {
    str << "Invalid command: " << cmnd << "\n";
    success = 0;
  }
  RDIRptInteractiveLog(_my_name << " received command: " << cmnd << "\nResult:\n" << str.buf());
  // this is the only safe way to return a string?
  return CORBA_STRING_DUP(str.buf());
}

AttN::IactSeq*
EventProxyPushSupplier_i::children(CORBA::Boolean only_cleanup_candidates)
{
  AttN::IactSeq* ren = new AttN::IactSeq;
  if ( ren == (AttN::IactSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  ren->length(0);
  return ren;
}

#undef WHATFN
#define WHATFN "EventProxyPushSupplier_i::safe_cleanup"
CORBA::Boolean
EventProxyPushSupplier_i::safe_cleanup()
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; } 
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  if (_pxstate == RDI_Connected) { return 0; } // do not destroy connected proxy
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
  return 1;
}

#undef WHATFN
#define WHATFN "EventProxyPushSupplier_i::my_name"
AttN::NameSeq*
EventProxyPushSupplier_i::my_name( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  AttN::NameSeq* res = new AttN::NameSeq(_my_name);
  if ( res == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  return res;
}

#undef WHATFN
#define WHATFN "EventProxyPushSupplier_i::child_names"
AttN::NameSeq*
EventProxyPushSupplier_i::child_names( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  AttN::NameSeq* names = new AttN::NameSeq;
  if ( names == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  names->length(0);
  return names;
}

void
EventProxyPushSupplier_i::out_commands(RDIstrstream& str)
{
  str << "omniNotify CosEvent ProxyPushSupplier commands:\n"
      << "  up                : change target to proxy's admin\n"
      << "  debug             : show debugging information for this proxy\n";
}

char*
EventProxyPushSupplier_i::do_command(const char* cmnd, CORBA::Boolean& success,
				     CORBA::Boolean& target_changed,
				     AttN_Interactive_outarg next_target  WRAPPED_IMPLARG )
{
  { // make sure the object is valid, update _last_use
    RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
    _last_use.set_curtime();
#endif
  } // end lock scope

  RDIParseCmd p(cmnd);
  success = 1;
  target_changed = 0;
  if (p.argc == 0) { return CORBA_STRING_DUP("");  }

  RDIstrstream str;
  if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "help")) {
    out_commands(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "debug")) {
    str << "----------------------------------------------------------------------\n";
    str << "Debug info for " << _my_name << '\n';
    str << "----------------------------------------------------------------------\n";
    log_output(str);
    str << '\n';
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "up")) {
    target_changed = 1;
    next_target = WRAPPED_IMPL2OREF(AttN::ConsumerAdmin, _myadmin);
    str << "\nomniNotify: new target ==> " << _my_name[_my_name.length()-2] << '\n';
  } else {
    str << "Invalid command: " << cmnd << "\n";
    success = 0;
  }
  RDIRptInteractiveLog(_my_name << " received command: " << cmnd << "\nResult:\n" << str.buf());
  // this is the only safe way to return a string?
  return CORBA_STRING_DUP(str.buf());
}

AttN::IactSeq*
EventProxyPullSupplier_i::children(CORBA::Boolean only_cleanup_candidates)
{
  AttN::IactSeq* ren = new AttN::IactSeq;
  if ( ren == (AttN::IactSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  ren->length(0);
  return ren;
}

#undef WHATFN
#define WHATFN "EventProxyPullSupplier_i::safe_cleanup"
CORBA::Boolean
EventProxyPullSupplier_i::safe_cleanup()
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; } 
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  if (_pxstate == RDI_Connected) { return 0; } // do not destroy connected proxy
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
  return 1;
}

#undef WHATFN
#define WHATFN "EventProxyPullSupplier_i::my_name"
AttN::NameSeq*
EventProxyPullSupplier_i::my_name( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  AttN::NameSeq* res = new AttN::NameSeq(_my_name);
  if ( res == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  return res;
}

#undef WHATFN
#define WHATFN "EventProxyPullSupplier_i::child_names"
AttN::NameSeq*
EventProxyPullSupplier_i::child_names( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  AttN::NameSeq* names = new AttN::NameSeq;
  if ( names == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  names->length(0);
  return names;
}

void
EventProxyPullSupplier_i::out_commands(RDIstrstream& str)
{
  str << "omniNotify CosEvent ProxyPullSupplier commands:\n"
      << "  up                : change target to proxy's admin\n"
      << "  debug             : show debugging information for this proxy\n";
}

char*
EventProxyPullSupplier_i::do_command(const char* cmnd, CORBA::Boolean& success,
				     CORBA::Boolean& target_changed,
				     AttN_Interactive_outarg next_target  WRAPPED_IMPLARG )
{
  { // make sure the object is valid, update _last_use
    RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
    _last_use.set_curtime();
#endif
  } // end lock scope

  RDIParseCmd p(cmnd);
  success = 1;
  target_changed = 0;
  if (p.argc == 0) { return CORBA_STRING_DUP("");  }

  RDIstrstream str;
  if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "help")) {
    out_commands(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "debug")) {
    str << "----------------------------------------------------------------------\n";
    str << "Debug info for " << _my_name << '\n';
    str << "----------------------------------------------------------------------\n";
    log_output(str);
    str << '\n';
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "up")) {
    target_changed = 1;
    next_target = WRAPPED_IMPL2OREF(AttN::ConsumerAdmin, _myadmin);
    str << "\nomniNotify: new target ==> " << _my_name[_my_name.length()-2] << '\n';
  } else {
    str << "Invalid command: " << cmnd << "\n";
    success = 0;
  }
  RDIRptInteractiveLog(_my_name << " received command: " << cmnd << "\nResult:\n" << str.buf());
  // this is the only safe way to return a string?
  return CORBA_STRING_DUP(str.buf());
}

