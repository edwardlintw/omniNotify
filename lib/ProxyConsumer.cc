// -*- Mode: C++; -*-
//                              File      : ProxyConsumer.cc
//                              Package   : omniNotify-Library
//                              Created on: 18-Oct-2000
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
//    Implementation of Consumer proxies
//

#include "thread_wrappers.h"
#include "CosNotifyShorthands.h"
#include "RDI.h"
#include "RDIStringDefs.h"
#include "CosNfyUtils.h"
#include "CosNotifyChannelAdmin_i.h"
#include "RDIOplocksMacros.h"

extern const char* RDI_PRX_TYPE(const CosNA::ProxyType& type);

// ------------------------------------------------------------- //
// Support classes needed for implementing  the thread-per-proxy //
// notification approach                                         //
// ------------------------------------------------------------- //

class AnyPullWorker : public TW_Thread {
public:
  typedef void (ProxyPullConsumer_i::*Method)(void);
  AnyPullWorker(ProxyPullConsumer_i* p, Method m) :
    TW_Thread(NULL,PRIORITY_NORMAL), _proxy(p), _method(m) {;}
  void run(void *)      { (_proxy->*_method)(); }
private:
  ProxyPullConsumer_i* _proxy;
  Method               _method;
  AnyPullWorker()  {;}
};

class StrPullWorker : public TW_Thread {
public:
  typedef void (StructuredProxyPullConsumer_i::*Method)(void);
  StrPullWorker(StructuredProxyPullConsumer_i* p, Method m) :
    TW_Thread(NULL,PRIORITY_NORMAL), _proxy(p), _method(m) {;}
  void run(void *)      { (_proxy->*_method)(); }
private:
  StructuredProxyPullConsumer_i* _proxy;
  Method                         _method;
  StrPullWorker()  {;}
};

class SeqPullWorker : public TW_Thread {
public:
  typedef void (SequenceProxyPullConsumer_i::*Method)(void);
  SeqPullWorker(SequenceProxyPullConsumer_i* p, Method m) :
    TW_Thread(NULL,PRIORITY_NORMAL), _proxy(p), _method(m) {;}
  void run(void *)      { (_proxy->*_method)(); }
private:
  SequenceProxyPullConsumer_i* _proxy;
  Method                       _method;
  SeqPullWorker()  {;}
};

// ------------------------------------------------------------- //
// RDIProxyConsumer                                              //
// ------------------------------------------------------------- //

RDIProxyConsumer::RDIProxyConsumer(const char*            resty,
				   const char*            fa_helper_resty,
				   SupplierAdmin_i*       admin,
				   EventChannel_i*        chann,
				   const RDI_ObjectKind   otype,
				   const CosNA::ProxyType prtype,
				   const CosNA::ProxyID&  prxid) : 
  RDINotifySubscribe(),
  _oplockptr(0), _my_name(admin->L_my_name()),
  _fa_helper(fa_helper_resty), _channel(chann), _myadmin(admin),
  _otype(otype), _prxtype(prtype), _pserial(prxid), _nevents(0),
  _pxstate(RDI_NotConnected), _active(0), 
  _qosprop(0), _sc_off(0),
  _evtypes(RDI_EventType::hash, RDI_EventType::rank)
{
  RDI_OPLOCK_INIT(resty);
  char buf[20];
  sprintf(buf, "proxy%ld", prxid);
  _my_name.length(_my_name.length()+1);
  _my_name[_my_name.length()-1] = (const char*)buf;
  _qosprop = new RDI_NotifQoS(_myadmin->qos_properties());
  RDI_AssertAllocThrowNo(_qosprop, "Memory allocation failure - RDI_NotifQoS\n");
  _sc_subscriber = CosNC::NotifySubscribe::_nil();
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
}

RDIProxyConsumer::~RDIProxyConsumer()
{
  // RDIDbgForceLog("XXX_REMOVE  ~RDIProxyConsumer called for " << RDI_PRX_TYPE(_prxtype) << " proxy " << _pserial << '\n');
  RDI_OPLOCK_DESTROY_CHECK("RDIProxyConsumer");
}

// ----------- ** Methods that implement public CORBA interfaces ** ---------------

#undef WHATFN
#define WHATFN "RDIProxyConsumer::MyType"
CosNA::ProxyType
RDIProxyConsumer::MyType()
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // in process of being disposed
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNA::ProxyType res = _prxtype;
  return res;
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::MyID"
CosNA::ProxyID
RDIProxyConsumer::MyID()
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // in process of being disposed
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNA::ProxyID res = _pserial;
  return res;
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::MyAdmin"
CosNA::SupplierAdmin_ptr
RDIProxyConsumer::MyAdmin()
{ 
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // in process of being disposed
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNA::SupplierAdmin_ptr res = WRAPPED_IMPL2OREF(CosNA::SupplierAdmin, _myadmin);
  return res;
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::obtain_subscription_types"
CosN::EventTypeSeq*
RDIProxyConsumer::obtain_subscription_types(CosNA::ObtainInfoMode mode)
{
  // to achieve atomicity for CosNA::ALL_NOW_UPDATES_ON we must
  // ask _channel to do this operation so that lock acquisition
  // order will be _chanel (actually _type_map)
  // followed by this + _channel->schange_pool
  // (via call to _enable_updates or _disable_updates)

  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // in process of being disposed
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosN::EventTypeSeq* res = 0;
  { // introduce unlock scope
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);
    res = _channel->pxy_obtain_subscription_types(this, mode);
  } // end unlock scope
  if (!held.cproxy) {
    RDI_Fatal("RDIProxyConsumer::obtain_subscription_types [**unexpected REACQUIRE failure**]\n");
  }
  return res;
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::validate_event_qos"
void
RDIProxyConsumer::validate_event_qos(const CosN::QoSProperties& r_qos,
				     CosN_NamedPropertyRangeSeq_outarg rseq)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // in process of being disposed
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosN::PropertyErrorSeq eseq;
  CORBA::Boolean  subobjs = 0;

  rseq = new CosNotification::NamedPropertyRangeSeq();
  RDI_AssertAllocThrowNo((CosNotification::NamedPropertyRangeSeq*)rseq,
			 "Memory allocation failed - NamedPropertyRangeSeq\n");
  if (!RDI_NotifQoS::validate(r_qos, *_qosprop, RDI_EMESSAGE, eseq, *rseq, subobjs)) {
    throw CosN::UnsupportedQoS(eseq);
  }
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::get_qos"
CosN::QoSProperties*
RDIProxyConsumer::get_qos()
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // in process of being disposed
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosN::QoSProperties* res = _qosprop->get_qos(_otype);
  return res;
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::set_qos"
void
RDIProxyConsumer::set_qos(const CosN::QoSProperties& r_qos)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // in process of being disposed
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  if (r_qos.length() == 0) return;

  CosN::PropertyErrorSeq eseq;
  CosN::NamedPropertyRangeSeq rseq;
  CORBA::Boolean  subobjs = 0;
  
  if (! RDI_NotifQoS::validate(r_qos, *_qosprop, _otype, eseq, rseq, subobjs)) {
    throw CosN::UnsupportedQoS(eseq);
  }
  _qosprop->set_qos(r_qos); 
  if (RDIRptNotifQoS) {
    RDIRptLogger(l, RDIRptNotifQoS_nm);
    l.str << _my_name << ": NotifQoS param(s) modified as follows\n";
    for (unsigned int i = 0; i < r_qos.length(); i++) {
      l.str << "  " << r_qos[i].name << " set to "; RDI_pp_any(l.str, r_qos[i].value); l.str << '\n';
    }
    l.str << '\n';
  }
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::validate_qos"
void
RDIProxyConsumer::validate_qos(const CosN::QoSProperties& r_qos,
			       CosN_NamedPropertyRangeSeq_outarg rseq)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // in process of being disposed
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosN::PropertyErrorSeq eseq;
  CORBA::Boolean  subobjs = 0;

  rseq = new CosNotification::NamedPropertyRangeSeq();
  RDI_AssertAllocThrowNo((CosNotification::NamedPropertyRangeSeq*)rseq,
			 "Memory allocation failed - NamedPropertyRangeSeq\n");
  if (!RDI_NotifQoS::validate(r_qos, *_qosprop, _otype, eseq, *rseq, subobjs)) {
    throw CosN::UnsupportedQoS(eseq);
  }
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::add_filter"
CosNF::FilterID
RDIProxyConsumer::add_filter(CosNF::Filter_ptr filter)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // in process of being disposed
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  return _fa_helper.add_filter_i(held, filter, (RDINotifySubscribe_ptr) this, 0); // 0 => do NOT want propagate_schange callbacks
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::remove_filter"
void
RDIProxyConsumer::remove_filter(CosNF::FilterID fltrID)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  _fa_helper.remove_filter(held, fltrID, 0);
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::remove_all_filters"
void
RDIProxyConsumer::remove_all_filters()
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  _fa_helper.remove_all_filters(held, 0); // 0 => do NOT want propagate_schange callback
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::get_filter"
CosNF::Filter_ptr
RDIProxyConsumer::get_filter(CosNF::FilterID fltrID)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // in process of being disposed
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNF::Filter_ptr res = _fa_helper.get_filter(fltrID);
  return CosNF::Filter::_duplicate(res);
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::get_all_filters"
CosNF::FilterIDSeq*
RDIProxyConsumer::get_all_filters()
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // in process of being disposed
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNF::FilterIDSeq* res = _fa_helper.get_all_filters();
  return res;
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::offer_change"
// unlike admin and channel levels, multiple additions of some event type boo::bar
// are canceled by a single deletion of boo::bar (assumption is only a single
// external entity is responsible for calling offer_change on a proxy)
void
RDIProxyConsumer::offer_change(const CosN::EventTypeSeq& added, const CosN::EventTypeSeq& deled)
{
  RDI_LocksHeld held        = { 0 };
  SupplierAdmin_i* myadmin  = _myadmin;
  CosN::EventTypeSeq new_added;
  CosN::EventTypeSeq old_deled;

  { // introduce lock scope
    RDI_OPLOCK_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
    if (!held.cproxy) { RDI_THROW_INV_OBJREF; }
    if ((_pxstate == RDI_Disconnected) || (_pxstate == RDI_Exception)) {
      // all offers permanently revoked in these 2 states
      RDI_THROW_INV_OBJREF;
    }
#ifndef NO_OBJ_GC
    _last_use.set_curtime();
#endif
    CosN::EventTypeSeq added_copy = added;
    CosN::EventTypeSeq deled_copy = deled;
    CORBA::ULong ix = 0, sz = 0, vl = 0;
    if ( ! RDI_EventType::valid_sequence(added_copy, ix) ) {
      throw CosNC::InvalidEventType(added[ix]);
    }
    if ( ! RDI_EventType::valid_sequence(deled_copy, ix) ) {
      throw CosNC::InvalidEventType(deled[ix]);
    }
    new_added.length(0);
    old_deled.length(0);
    for (sz = 0, ix = 0; ix < added_copy.length(); ix++) {
      if ( _evtypes.lookup(added_copy[ix], vl) ) {
	// FORMERLY: vl += 1; _evtypes.replace(added_copy[ix], vl);
	// NOW: do not increase count beyond 1
      } else {
	vl  = 1; _evtypes.insert(added_copy[ix], vl);
	new_added.length(sz + 1);
	new_added[sz].domain_name = added_copy[ix].domain_name;
	new_added[sz++].type_name = added_copy[ix].type_name;
      }
    }
    for (sz = 0, ix = 0; ix < deled_copy.length(); ix++) {
      if ( _evtypes.lookup(deled_copy[ix], vl) ) {
	if ( vl == 1 ) {
	  _evtypes.remove(deled_copy[ix]);
	  old_deled.length(sz + 1);
	  old_deled[sz].domain_name = deled_copy[ix].domain_name;
	  old_deled[sz++].type_name = deled_copy[ix].type_name;
	} else {
	  vl -= 1; _evtypes.replace(deled_copy[ix], vl);
	} 
      } else {
	RDIDbgCPxyLog("Invalid " << (const char*)deled_copy[ix].domain_name << "::" << (const char*)deled_copy[ix].type_name);
      }
    }
  } // end lock scope
  // If any new event types were added or existing event types were
  // deleted, notify the parent administrative object
  if ( new_added.length() || old_deled.length() ) {
    myadmin->propagate_ochange(held, new_added, old_deled);
  }
}

AttN::IactSeq*
RDIProxyConsumer::children(CORBA::Boolean only_cleanup_candidates)
{
  AttN::IactSeq* ren = new AttN::IactSeq;
  if ( ren == (AttN::IactSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  ren->length(0);
  return ren;
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::safe_cleanup"
CORBA::Boolean
RDIProxyConsumer::safe_cleanup()
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  if (_pxstate == RDI_Connected) { return 0; } // do not destroy connected proxy
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
  return 1;
}

// RDINotifSupplier methods:
void
RDIProxyConsumer::propagate_schange(RDI_LocksHeld&             held,
				    const CosN::EventTypeSeq&  added,
				    const CosN::EventTypeSeq&  deled,
				    Filter_i*                  filter)
{
  RDI_Fatal("RDIProxyConsumer::propagate_schange: did not register, should not get here");
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::filter_destroy_i"
void
RDIProxyConsumer::filter_destroy_i(Filter_i* filter)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return);
  _fa_helper.rem_filter_i(filter);
}

// -------------------- ** Local-only Methods ** -----------------------------------

#ifndef NO_OBJ_GC
#undef WHATFN
#define WHATFN "RDIProxyConsumer::obj_gc"
CORBA::Boolean
RDIProxyConsumer::obj_gc(RDI_TimeT curtime, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { return 0; }
  if (_pxstate == RDI_Disconnected) { return 0; }
  if ( (deadConProxy && _pxstate == RDI_Connected && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadConProxy)) ||
       (deadOtherProxy && _pxstate != RDI_Connected && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadOtherProxy)) ) {
#ifndef NDEBUG
    if (_pxstate == RDI_Connected) {
      RDIDbgCPxyLog("GC destroys connected " << RDI_PRX_TYPE(_prxtype) << " proxy " << _pserial
		    << " curtime = " << curtime.fmt_local()
		    << " last_use = " << _last_use.fmt_local()
		    << "(diff = " << RDI_TIMET_DIFF_IN_SECS(_last_use, curtime)
		    << ") DeadConProxyInterval = " << deadConProxy << '\n');
    } else {
      RDIDbgCPxyLog("GC destroys non-connected " << RDI_PRX_TYPE(_prxtype) << " proxy " << _pserial
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

// When pxstate changes to Exception or Disconnected, no further events
// will be supplied, and offer_change calls will be ignored,
// so we must revoke any outstanding offers.
//
// ASSUMES a bumped oplock is held
//
#undef WHATFN
#define WHATFN "RDIProxyConsumer::_revoke_offers"
void
RDIProxyConsumer::_revoke_offers(RDI_LocksHeld &held)
{
  if ( _evtypes.length() ) {
    CosN::EventTypeSeq added, deled;
    added.length(0);
    deled.length( _evtypes.length() );
    RDI_HashCursor<CosN::EventType, CORBA::ULong> curs;
    CORBA::ULong ix=0;
    for ( ix=0, curs = _evtypes.cursor(); curs.is_valid(); ++ix, ++curs ) {
      deled[ix] = curs.key();
    }
    // Need to release lock across this call
    {
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);
      _myadmin->propagate_ochange(held, added, deled);
    }
  }
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::_enable_updates"
void
RDIProxyConsumer::_enable_updates()
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return);
  RDI_ChangePool* cscp = _channel->schange_pool();
  if ( (_pxstate != RDI_Connected) || CORBA::is_nil(_sc_subscriber) || ! cscp ) {
    // _sc_off status is meaningless without connected _nc_supplier and a valid _schange_pool
    _sc_off = 0;
    return;
  }
  if (!_sc_off) {
    // make subscription_change msgs reflect only subsequent updates
    // by removing 'this' before re-inserting it
    cscp->remove_proxy(this);
  }
  _sc_off = 0;
  cscp->insert_proxy(this);
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::_disable_updates"
void
RDIProxyConsumer::_disable_updates()
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return);
  RDI_ChangePool* cscp = _channel->schange_pool();
  if ( (_pxstate != RDI_Connected) || CORBA::is_nil(_sc_subscriber) || ! cscp ) {
    // _sc_off status is meaningless without connected _nc_supplier and a valid _schange_pool
    _sc_off = 1;
    return;
  }
  if (!_sc_off) {
    _sc_off = 1;
    cscp->remove_proxy(this);
  }
}

// Return true only if there are admin-level filters and at least one matches event
// ** N.B.: oplock should be held during call to this helper routine
CORBA::Boolean
RDIProxyConsumer::_match_event_admin_level(const CORBA::Any& event)
{
  RDIDbgCPxyLog("XXX _match_event_admin_level(any) called\n");
  return (_myadmin->has_filters() && _myadmin->match_event(event));
}

// Return true only if there are proxy-level filters and at least one matches event
// ** N.B.: oplock should be held during call to this helper routine
CORBA::Boolean
RDIProxyConsumer::_match_event_proxy_level(const CORBA::Any& event)
{
  RDIDbgCPxyLog("XXX _match_event_proxy_level(any) called\n");
  CORBA::Boolean matched = 0;
  if ( _fa_helper.has_filters() ) {
    CosNF::FilterIDSeq* fseq = _fa_helper.get_all_filters();
    CosNF::Filter_ptr filter;
    Filter_i* rdfilter = 0;
    for (CORBA::ULong ix = 0; ix < fseq->length(); ix++) {
      filter = _fa_helper.get_filter((*fseq)[ix]);
      rdfilter = Filter_i::Filter2Filter_i(filter);
      if (rdfilter) // XXX
	RDIDbgCPxyLog("XXX RDIProxyConsumer::_match_event_proxy_level calling rdfilter->match_chan\n");
      else // XXX
	RDIDbgCPxyLog("XXX RDIProxyConsumer::_match_event_proxy_level calling filter->match\n");
      if ( (rdfilter && rdfilter->match_chan(event, _channel)) ||
	   (!rdfilter && filter->match(event)) ) {
	matched = 1;
	break;
      }
    }
    delete fseq;
  }
  return matched;
}

// Return true if event should be added to the channel
// ** N.B.: oplock should be held during call to this helper routine  
CORBA::Boolean
RDIProxyConsumer::_match_event(const CORBA::Any& event)
{
  // no filters at all => matched 
  if ( (! _fa_helper.has_filters()) && (! _myadmin->has_filters()) ) {
    return 1;
  }
  // proxy or admin or both have filters
  if (!_fa_helper.has_filters()) {
    // only admin has filters -- _myadmin->_admin_operator() does not matter
    return _match_event_admin_level(event);
  } else if (!_myadmin->has_filters()) {
    // only proxy has filters -- _myadmin->_admin_operator() does not matter
    return _match_event_proxy_level(event);
  } else {
    // both proxy and admin have filters -- _myadmin->_admin_operator() matters
    if (_myadmin->_admin_operator() == CosNA::AND_OP) {
      return (_match_event_proxy_level(event) &&  _match_event_admin_level(event));
    } else {
      return (_match_event_proxy_level(event) ||  _match_event_admin_level(event));
    }
  }
}

// Return true only if there are admin-level filters and at least one matches event
// ** N.B.: oplock should be held during call to this helper routine
CORBA::Boolean
RDIProxyConsumer::_match_event_admin_level(const CosN::StructuredEvent* event, RDI_StructuredEvent* sevnt)
{
  RDIDbgCPxyLog("XXX _match_event_admin_level(event, sevnt) called\n");
  return (_myadmin->has_filters() && _myadmin->match_event(event, sevnt));
}

// Return true only if there are proxy-level filters and at least one matches event
// ** N.B.: oplock should be held during call to this helper routine
CORBA::Boolean
RDIProxyConsumer::_match_event_proxy_level(const CosN::StructuredEvent* event, RDI_StructuredEvent* sevnt)
{
  RDIDbgCPxyLog("XXX _match_event_proxy_level(event, sevnt) called\n");
  CORBA::Boolean matched = 0;
  if ( _fa_helper.has_filters() ) {
    CosNF::FilterIDSeq* filterseq = _fa_helper.get_all_filters();
    CosNF::Filter_ptr filter;
    Filter_i* rdfilter = 0;
    for (CORBA::ULong ix = 0; ix < filterseq->length(); ix++) {
      filter = _fa_helper.get_filter((*filterseq)[ix]);
      rdfilter = Filter_i::Filter2Filter_i(filter);
      if (rdfilter) // XXX
	RDIDbgCPxyLog("XXX RDIProxyConsumer::_match_event_proxy_level calling rdfilter->rdi_match\n");
      else // XXX
	RDIDbgCPxyLog("XXX RDIProxyConsumer::_match_event_proxy_level calling filter->match_structured\n");
      if ( (rdfilter && rdfilter->rdi_match(sevnt, _channel)) ||
	   (!rdfilter && filter->match_structured(*event)) ) {
	matched = 1;
	break;
      }
    }
    delete filterseq;
  }
  return matched;
}

// Return true if event should be added to the channel
// ** N.B.: oplock should be held during call to this helper routine
CORBA::Boolean
RDIProxyConsumer::_match_event(const CosN::StructuredEvent* event, RDI_StructuredEvent* sevnt)
{
  // no filters at all => matched 
  if ( (! _fa_helper.has_filters()) && (! _myadmin->has_filters()) ) {
    return 1;
  }
  // proxy or admin or both have filters
  if (!_fa_helper.has_filters()) {
    // only admin has filters -- _myadmin->_admin_operator() does not matter
    return _match_event_admin_level(event, sevnt);
  } else if (!_myadmin->has_filters()) {
    // only proxy has filters -- _myadmin->_admin_operator() does not matter
    return _match_event_proxy_level(event, sevnt);
  } else {
    // both proxy and admin have filters -- _myadmin->_admin_operator() matters
    if (_myadmin->_admin_operator() == CosNA::AND_OP) {
      return (_match_event_proxy_level(event, sevnt) &&  _match_event_admin_level(event, sevnt));
    } else {
      return (_match_event_proxy_level(event, sevnt) ||  _match_event_admin_level(event, sevnt));
    }
  }
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::disconnect_client_and_dispose"
void
RDIProxyConsumer::disconnect_client_and_dispose(RDI_LocksHeld&  held,
						CORBA::Boolean  remove_from_admin)
{
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { return; }
  _disconnect_client_and_dispose(held, remove_from_admin, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "RDIProxyConsumer::send_subscription_change"
// If proxy has a connected CosNotify-style supplier, _sc_subscriber will be non-null.
// Attempt to send it a subscription_change msg.
//
// If the message cannot be delivered, or if CORBA::NO_IMPLEMENT or another
// exception is returned, return value TRUE
// (indicating the proxy should not be sent any further offer_change msgs), otherwise FALSE.
//
// XXX On a transient failure, the channel could retry one or more times before
// XXX giving up and returning value TRUE.  At the moment, channel gives up immediately.
//
// XXX ** On hard failure, change _pxstate ??? **
//
CORBA::Boolean
RDIProxyConsumer::send_subscription_change(const CosN::EventTypeSeq& added,
					   const CosN::EventTypeSeq& deled)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { return 1; }

  CORBA::Boolean res = 0; // OK
  if (_sc_off) {
    RDIDbgCPxyLog("send_subscription_change called on proxy with _sc_off true\n");
    res = 1;
  } else if ( (_pxstate == RDI_Connected) && ! CORBA::is_nil(_sc_subscriber) ) {
    // do not hold OPLOCK across subscription_change call
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);
      try {
	_sc_subscriber->subscription_change(added, deled);
#ifndef NO_OBJ_GC
	_last_use.set_curtime();
#endif
      } catch(CORBA::NO_IMPLEMENT& ex) {
	res = 1; // do not try any more of these
      } catch (...) {
	RDIDbgCPxyLog("RDIProxyConsumer -- caught exception other than CORBA::NO_IMPLEMENT " <<
		      "when sending a subscription_change; could retry if this is a soft failure " <<
		      "but for now, giving up immediately\n");
	res = 1;
      }
    } // end unlock scope
    if (!held.cproxy) {
      RDI_Fatal("RDIProxyConsumer::send_subscription_change [**unexpected REACQUIRE failure**]\n");
    }
  } else {
    RDIDbgCPxyLog("RDIProxyConsumer -- sc_subscriber not available => did not send subscription_change\n");
    res = 1;
  }
  return res;
}

void
RDIProxyConsumer::_report_reject_se(const char* cproxy_descr, CosNA::ProxyID consumer_id,
				    const CosN::StructuredEvent& evnt)
{
  if (!_channel->shutting_down()) {
    RDIRptRejectsLog("Rejecting structured event:\n" << evnt << 
		     "\n\tfrom supplier connected to " << cproxy_descr << " proxy with id " << consumer_id <<
		     "\n\tAnnounced-events queue hit limit MaxQueueLength\n");
  }
}

void
RDIProxyConsumer::_report_reject_any(const char* cproxy_descr, CosNA::ProxyID consumer_id,
				     const CORBA::Any& evnt)
{
  if (!_channel->shutting_down()) {
    RDIRptRejectsLog("Rejecting event of type CORBA::Any:\n" << evnt <<
		     "\n\tfrom supplier connected to " << cproxy_descr << " proxy with id " << consumer_id <<
		     "\n\tAnnounced-events queue hit limit MaxQueueLength\n");
  }
}

#ifndef NREPORT
#define REPORT_REJECT_SE(descr, id, event)  _report_reject_se (descr, id, event)
#define REPORT_REJECT_ANY(descr, id, event) _report_reject_any(descr, id, event)
#else
#define REPORT_REJECT_SE(descr, id, event)  __RDI_NULL_STMT
#define REPORT_REJECT_ANY(descr, id, event) __RDI_NULL_STMT
#endif

// ------------------------------------------------------------- //
// ProxyPullConsumer_i                                           //
// ------------------------------------------------------------- //

ProxyPullConsumer_i::ProxyPullConsumer_i(SupplierAdmin_i* admin,
					 EventChannel_i*  chann,
					 const CosNA::ProxyID&  prxid) : 
  RDIProxyConsumer("ProxyPullConsumer", "ProxyPullConsumer_fa_helper",
		   admin, chann, RDI_C_AnyPRX, CosNA::PULL_ANY, prxid),
  _worker(0), _thrdone(0), _timeout_s(0), _timeout_n(0)
{
  _supplier = CosEvC::PullSupplier::_nil();
  _nc_supplier = CosNC::PullSupplier::_nil();
  // When the number of pull threads allocated at the channel level is
  // 0, each proxy uses its own thread to pull events from its supplier
  if ( _channel->pull_threads() == 0 ) {
    _worker = new AnyPullWorker(this, &ProxyPullConsumer_i::_pull_event);
    RDI_AssertAllocThrowNo(_worker, "Memory allocation failed -- Thread\n");
    _worker->start();
    _thrdone = 0;
    RDIDbgCPxyLog("Pull thread for proxy " << (void*)this << " -- " << _worker->id());
  }
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

ProxyPullConsumer_i::~ProxyPullConsumer_i()
{
  RDI_OPLOCK_DESTROY_CHECK("ProxyPullConsumer_i");
}

#undef WHATFN
#define WHATFN "ProxyPullConsumer_i::connect_any_pull_supplier"
void
ProxyPullConsumer_i::connect_any_pull_supplier(CosEvC::PullSupplier_ptr supplier WRAPPED_IMPLARG)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  CosEvC::PullSupplier_var evsupl;
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
  _active   = 1;
  _supplier = CosEvC::PullSupplier::_duplicate(supplier);
  // If supplier is a CosNC::PullSupplier, set _nc_supplier
  _nc_supplier = CosNC::PullSupplier::_narrow(supplier); // implicit duplicate

  if (! CORBA::is_nil(_nc_supplier)) {
    _sc_subscriber = CosNC::NotifySubscribe::_narrow(_nc_supplier); // implicit duplicate
    if (CORBA::is_nil(_sc_subscriber)) {
      RDIDbgCPxyLog("** UNEXPECTED: CosNC::PullSupplier could not be narrowed to CosNC::NotifySubscribe\n");
    }
    // if enabled, register this proxy for subscription_change msgs
    RDI_ChangePool* cscp = _channel->schange_pool();
    if ( ! _sc_off && cscp ) {
      cscp->insert_proxy(this);
    }
  }
  if ( _worker ) {    // Notify worker thread
    RDI_OPLOCK_SIGNAL;
  }
  RDI_PullSupplier* cps = _channel->pull_supplier();
  if ( cps ) {
    cps->signal_pull_threads();
  }
}

#undef WHATFN
#define WHATFN "ProxyPullConsumer_i::suspend_connection"
void
ProxyPullConsumer_i::suspend_connection(WRAPPED_IMPLARG_VOID)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_Connected ) {
    throw CosNA::NotConnected();
  }
  if ( ! _active ) {
    throw CosNA::ConnectionAlreadyInactive();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _active = 0;
}

#undef WHATFN
#define WHATFN "ProxyPullConsumer_i::resume_connection"
void
ProxyPullConsumer_i::resume_connection(WRAPPED_IMPLARG_VOID)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_Connected ) {
    throw CosNA::NotConnected();
  }
  if ( _active ) {
    throw CosNA::ConnectionAlreadyActive();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _active = 1;
  if ( _worker ) {	// Notify worker thread
    RDI_OPLOCK_SIGNAL;
  }
  RDI_PullSupplier* cps = _channel->pull_supplier();
  if ( cps ) {
    cps->signal_pull_threads();
  }
}

#undef WHATFN
#define WHATFN "ProxyPullConsumer_i::is_available"
// is_available determines whether a pull should occur.
// in this case the requirement is that either the pull interval is zero
// or an interval timeout has occurred
CORBA::Boolean
ProxyPullConsumer_i::is_available(unsigned long* wait_s, unsigned long* wait_n)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return 0);
  unsigned long pull_interval_s, pull_interval_n;
  unsigned long time_s, time_n;

  if ( (_pxstate != RDI_Connected) || !_active ) {
    return 0;
  }

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
#define WHATFN "ProxyPullConsumer_i::pull_event"
// REQUIREMENT: A thread from pull pool only calls this method
// if is_available() is true and it thinks state is connected and active.
// (For safety we verify that these conditions actually hold.)
void
ProxyPullConsumer_i::pull_event(CORBA::Boolean& invalid)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { return; }

  CORBA::Boolean outcall_worked;
  unsigned long pull_interval_s, pull_interval_n;
  unsigned long time_s, time_n;

  _channel->pull_period_s_n(pull_interval_s, pull_interval_n);

  CORBA::Boolean hasev = 0;
  CORBA::Any*    event = 0;

  invalid = 0;
  if ( (_pxstate == RDI_Disconnected) || (_pxstate == RDI_Exception) ) {
    // tell caller no more pulls (by setting invalid to 1)
    invalid = 1;
    return;
  }
  if ( (_pxstate != RDI_Connected) || !_active ) {
    return;
  }

  // sanity check: if pull interval is positive, we should only be called if a timeout occurred
  if (pull_interval_s || pull_interval_n) {
    TW_GET_TIME(&time_s, &time_n);
    if ((_timeout_s == 0) && (_timeout_n == 0)) { // timeout not set; set it now
      TW_GET_TIME(&_timeout_s, &_timeout_n, pull_interval_s, pull_interval_n);
    } 
    if ( (time_s < _timeout_s) || ((time_s == _timeout_s) && (time_n < _timeout_n)) ) {
      RDIDbgCPxyLog("** INTERNAL ERROR: ProxyPullConsumer_i::pull_event called too soon\n");
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

  // Do not hold OPLOCK across try_pull
  outcall_worked = 0;
  { // introduce unlock scope
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);
    try {
      event = _supplier->try_pull(hasev);
      outcall_worked = 1;
#ifndef NO_OBJ_GC
      _last_use.set_curtime();
#endif
    } catch ( CORBA::INV_OBJREF& e ) {
      RDIDbgCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Invalid object reference\n");
    } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
      RDIDbgCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Supplier object does not exist\n");
    } catch ( CORBA::COMM_FAILURE& e ) {
      RDIDbgCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Communication Failure\n");
    } catch (...) {
      RDIDbgCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Exception while pulling event from supplier\n");
    }
  } // end unlock scope
  if (!held.cproxy) {
    RDI_Fatal("ProxyPullConsumer_i::pull_event [**unexpected REACQUIRE failure**]\n");
  }
  // add new event to channel, but only if we are still connected
  if ( (_pxstate == RDI_Connected) && outcall_worked && hasev && event) {
    _nevents++; 
    if ( _match_event(*event) ) {
      if (_channel->new_any_event(*event)) {
	REPORT_REJECT_ANY("ProxyPullConsumer", _pserial, *event);
      }
    }
  }
  // new_any_event copies event so can always delete it
  if ( event ) {
    delete event; event = 0;
  }

  if (_pxstate != RDI_Connected) {
    // disconnected/disposed during try_pull -- do nothing else with my state
  } else {
    if (!outcall_worked) { // exception
      // unregister this proxy -- no more subscription_change msgs
      RDI_ChangePool* cscp = _channel->schange_pool();
      if ( ! _sc_off && ! CORBA::is_nil(_nc_supplier) && cscp ) {
	cscp->remove_proxy(this);
      }
      _pxstate = RDI_Exception;
      _revoke_offers(held);
      invalid  = 1;
    }
  }
}

#undef WHATFN
#define WHATFN "ProxyPullConsumer_i::_pull_event"
void
ProxyPullConsumer_i::_pull_event()
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { return; }

  CORBA::Boolean outcall_worked;
  CORBA::Boolean hasev = 0, update_timeout = 0, do_yield = 0;
  CORBA::Any*    event = 0;
  unsigned long  pull_interval_s, pull_interval_n;
  unsigned long  time_s, time_n;

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
      // and state must be RDI_Connected and _active
      if ((_pxstate != RDI_NotConnected) && (_pxstate != RDI_Connected)) {
	break; // also break on exceptional state
      }
      if ((_pxstate == RDI_Connected) && _active) {
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
	RDI_OPLOCK_WAIT(WHATFN); // must wait _pxstate or _active to change
      }
    }
    if ( _pxstate != RDI_Connected ) {
      RDI_THREAD_EXIT(pull_exit, RDIDbgCPxy, RDIDbgCPxy_nm, "pull thread\n");
    }

    // Do not hold OPLOCK across try_pull
    outcall_worked = 0;
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);

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
	RDIDbgCPxyLog("Pull thread for proxy " << (void*)this << " - Invalid object reference\n");
      } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
	RDIDbgCPxyLog("Pull thread for proxy " << (void*)this << " - Supplier object does not exist\n");
      } catch ( CORBA::COMM_FAILURE& e ) {
	RDIDbgCPxyLog("Pull thread for proxy " << (void*)this << " - Communication Failure\n");
      } catch (...) {
	RDIDbgCPxyLog("Pull thread for proxy " << (void*)this << " - Exception while pulling event from supplier\n");
      }
    } // end unlock scope
    if (!held.cproxy) {
      RDI_Fatal("ProxyPullConsumer_i::_pull_event [**unexpected REACQUIRE failure**]\n");
    }
    update_timeout = 1; // force update of timeout at top of loop

    // add new event to channel, but only if we are still connected
    if ( (_pxstate == RDI_Connected) && outcall_worked && hasev && event ) {
      _nevents += 1;
      if ( _match_event(*event) ) {
	if (_channel->new_any_event(*event)) {
	  REPORT_REJECT_ANY("ProxyPullConsumer", _pserial, *event);
	}
      }
    }
    // new_any_event copies event so we can delete it regardless
    if ( event ) {
      delete event; event = 0;
    }

    if (_pxstate != RDI_Connected) {
      // disconnected/disposed during try_pull -- do nothing else with my state
    } else {
      if (!outcall_worked) { // exception
	// unregister this proxy -- no more subscription_change msgs
	RDI_ChangePool* cscp = _channel->schange_pool();
	if ( ! _sc_off && ! CORBA::is_nil(_nc_supplier) && cscp ) {
	  cscp->remove_proxy(this);
	}
	_pxstate = RDI_Exception;
	_revoke_offers(held);
      } 
    }
  }

 pull_exit:
  TW_EXIT();
}

#undef WHATFN
#define WHATFN "ProxyPullConsumer_i::disconnect_pull_consumer"
void
ProxyPullConsumer_i::disconnect_pull_consumer( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "ProxyPullConsumer_i::_disconnect_client_and_dispose"
// *** caller must have bumped scope lock, held.cproxy set
// Effect: initiate impl dispose
void
ProxyPullConsumer_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
						    CORBA::Boolean            remove_from_admin,
						    WRAPPED_DISPOSEINFO_PTR&  dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgCPxyLog("** ProxyPullConsumer_i::_disconnect_client_and_dispose called twice on same proxy!\n");
    return;
  }
  RDI_ChangePool* cscp = _channel->schange_pool();
  if ( ! _sc_off && (_pxstate == RDI_Connected) && ! CORBA::is_nil(_nc_supplier) && cscp ) {
    cscp->remove_proxy(this);
  }
  _pxstate = RDI_Disconnected; // acts as guard; following only entered by 1 thread
  // Wait for inuse to drop to one.  This ensures that any call that
  // bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_from_admin) {
    _revoke_offers(held);
  } // else channel is being destroyed so do not bother
  if (remove_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);
      _myadmin->remove_proxy(held, this);
    } // end unlock scope
    if (!held.cproxy) {
      RDI_Fatal("ProxyPullConsumer_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }
  _active  = 0;
  _fa_helper.remove_all_filters(held, 0); // 0 => do not want propagate_schange callback
  _supplier = CosEvC::PullSupplier::_nil();
  _nc_supplier = CosNC::PullSupplier::_nil();
  if ( _qosprop ) {
    delete _qosprop; _qosprop = 0;
  }
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

// ------------------------------------------------------------- //
// StructuredProxyPullConsumer_i                                 //
// ------------------------------------------------------------- //

StructuredProxyPullConsumer_i::StructuredProxyPullConsumer_i(
							     SupplierAdmin_i* admin,	
							     EventChannel_i*  chann,
							     const CosNA::ProxyID&  prxid) : 
  RDIProxyConsumer("StructuredProxyPullConsumer", "StructuredProxyPullConsumer_fa_helper",
		   admin, chann, RDI_C_StrPRX, CosNA::PULL_STRUCTURED, prxid),
  _worker(0), _thrdone(0), _timeout_s(0), _timeout_n(0)
{
  _supplier = CosNC::StructuredPullSupplier::_nil();
  // When the number of pull threads allocated at the channel level is
  // 0, each proxy uses its own thread to pull events from its supplier
  if ( _channel->pull_threads() == 0 ) {
    _worker = new StrPullWorker(this, &StructuredProxyPullConsumer_i::_pull_event);
    RDI_AssertAllocThrowNo(_worker, "Memory allocation failed -- Thread\n");
    _worker->start();
    _thrdone = 0;
    RDIDbgCPxyLog("Pull thread for proxy " << (void*)this << " -- " << _worker->id());
  }
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

StructuredProxyPullConsumer_i::~StructuredProxyPullConsumer_i()
{
  RDI_OPLOCK_DESTROY_CHECK("StructuredProxyPullConsumer_i");
}

#undef WHATFN
#define WHATFN "StructuredProxyPullConsumer_i::connect_structured_pull_supplier"
void
StructuredProxyPullConsumer_i::connect_structured_pull_supplier(CosNC::StructuredPullSupplier_ptr supplier
                                                                 WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( CORBA::is_nil(supplier)) {
    throw CORBA::BAD_PARAM(0, CORBA::COMPLETED_NO);
  }
  if ( _pxstate != RDI_NotConnected ) {
    throw CosEvCA::AlreadyConnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _supplier = CosNC::StructuredPullSupplier::_duplicate(supplier);
  _sc_subscriber = CosNC::NotifySubscribe::_narrow(_supplier); // implicit duplicate
  if (CORBA::is_nil(_sc_subscriber)) {
    RDIDbgCPxyLog("** UNEXPECTED: CosNC::StructuredPullSupplier could not be narrowed to CosNC::NotifySubscribe\n");
  }
  _pxstate  = RDI_Connected;
  _active   = 1;

  // if enabled, register this proxy for subscription_change msgs
  RDI_ChangePool* cscp = _channel->schange_pool();
  if ( ! _sc_off && cscp ) {
    cscp->insert_proxy(this);
  }
  if ( _worker ) {    // Notify worker thread
    RDI_OPLOCK_SIGNAL;
  }
  RDI_PullSupplier* cps = _channel->pull_supplier();
  if ( cps ) {
    cps->signal_pull_threads();
  }
}

#undef WHATFN
#define WHATFN "StructuredProxyPullConsumer_i::suspend_connection"
void
StructuredProxyPullConsumer_i::suspend_connection( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_Connected ) {
    throw CosNA::NotConnected();
  }
  if ( ! _active ) {
    throw CosNA::ConnectionAlreadyInactive();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _active = 0;
}

#undef WHATFN
#define WHATFN "StructuredProxyPullConsumer_i::resume_connection"
void
StructuredProxyPullConsumer_i::resume_connection( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_Connected ) {
    throw CosNA::NotConnected();
  }
  if ( _active ) {
    throw CosNA::ConnectionAlreadyActive();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _active = 1;
  if ( _worker ) {	// Notify worker thread
    RDI_OPLOCK_SIGNAL;
  }
  RDI_PullSupplier* cps = _channel->pull_supplier();
  if ( cps ) {
    cps->signal_pull_threads();
  }
}

#undef WHATFN
#define WHATFN "StructuredProxyPullConsumer_i::is_available"
// is_available determines whether a pull should occur.
// in this case the requirement is that either the pull interval is zero
// or an interval timeout has occurred
CORBA::Boolean
StructuredProxyPullConsumer_i::is_available(unsigned long* wait_s, unsigned long* wait_n)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return 0);
  unsigned long pull_interval_s, pull_interval_n;
  unsigned long time_s, time_n;

  if ( (_pxstate != RDI_Connected) || ! _active ) {
    return 0;
  }

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
#define WHATFN "StructuredProxyPullConsumer_i::pull_event"
// REQUIREMENT: A thread from pull pool only calls this method
// if is_available() is true and it thinks state is connected and active.
// (For safety we verify that these conditions actually hold.)
void
StructuredProxyPullConsumer_i::pull_event(CORBA::Boolean& invalid)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { return; }

  CORBA::Boolean outcall_worked;
  unsigned long pull_interval_s, pull_interval_n;
  unsigned long time_s, time_n;
  _channel->pull_period_s_n(pull_interval_s, pull_interval_n);

  CORBA::Boolean hasev = 0, out_of_space = 0, matched = 0;
  CosN::StructuredEvent* event = 0;
  RDI_StructuredEvent* sevnt = 0;

  invalid = 0;
  if ( (_pxstate == RDI_Disconnected) || (_pxstate == RDI_Exception) ) {
    // tell caller no more pulls (by setting invalid to 1)
    invalid = 1;
    return;
  }
  if ( (_pxstate != RDI_Connected) || ! _active ) {
    return;
  }

  // sanity check: if pull interval is positive, we should only be called if a timeout occurred
  if (pull_interval_s || pull_interval_n) {
    TW_GET_TIME(&time_s, &time_n);
    if ((_timeout_s == 0) && (_timeout_n == 0)) { // timeout not set; set it now
      TW_GET_TIME(&_timeout_s, &_timeout_n, pull_interval_s, pull_interval_n);
    } 
    if ( (time_s < _timeout_s) || ((time_s == _timeout_s) && (time_n < _timeout_n)) ) {
      RDIDbgCPxyLog("** INTERNAL ERROR: StructuredProxyPullConsumer_i::pull_event called too soon\n");
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

  // Do not hold OPLOCK across try_pull
  outcall_worked = 0;
  { // introduce unlock scope
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);

    try {
      RDIDbgCPxyLog("StructuredProxyPullConsumer_i::pull_event doing try_pull_structured_event\n");
      event = _supplier->try_pull_structured_event(hasev);
      RDIDbgCPxyLog("StructuredProxyPullConsumer_i::RETURN FROM pull_event doing try_pull_structured_event\n");
      outcall_worked = 1;
#ifndef NO_OBJ_GC
      _last_use.set_curtime();
#endif
    } catch ( CORBA::INV_OBJREF& e ) {
      RDIDbgCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Invalid object reference\n");
    } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
      RDIDbgCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Supplier object does not exist\n");
    } catch ( CORBA::COMM_FAILURE& e ) {
      RDIDbgCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Communication Failure\n");
    } catch (...) {
      RDIDbgCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Exception while pulling event from supplier\n");
    }
  } // end unlock scope
  if (!held.cproxy) {
    RDI_Fatal("StructuredProxyPullConsumer_i::pull_event [**unexpected REACQUIRE failure**]\n");
  }
  // add new event to channel, but only if we are still connected
  out_of_space = 0;
  if ( (_pxstate == RDI_Connected) && outcall_worked && hasev && event ) {
    _nevents++; 
    sevnt = new RDI_StructuredEvent(*event);
    RDI_AssertAllocThrowNo(sevnt, "Memory allocation failure -- RDI_StructuredEvent\n");
    if ( (matched = _match_event(event, sevnt)) ) {
      out_of_space = _channel->new_structured_event(sevnt);
      if (out_of_space) {
	REPORT_REJECT_SE("StructuredProxyPullConsumer", _pserial, *event);
      }
    }
    // Do not delete sevnt unless it was not added to channel
    if (sevnt && (!matched || out_of_space)) {
      delete sevnt;
    }
    sevnt = 0;
  }
  // Creation of sevnt copies event, so delete it regardless
  if ( event ) {
    delete event; event = 0;
  }

  if (_pxstate != RDI_Connected) {
    // disconnected/disposed during try_pull -- do nothing else with my state
  } else {
    if (!outcall_worked) { // exception
      // unregister this proxy -- no more subscription_change msgs
      RDI_ChangePool* cscp = _channel->schange_pool();
      if ( ! _sc_off && cscp ) {
	cscp->remove_proxy(this);
      }
      _pxstate = RDI_Exception;
      _revoke_offers(held);
      invalid  = 1;
    }
  }
}

#undef WHATFN
#define WHATFN "StructuredProxyPullConsumer_i::_pull_event"
void
StructuredProxyPullConsumer_i::_pull_event()
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { return; }

  CORBA::Boolean outcall_worked; 
  CORBA::Boolean hasev = 0, update_timeout = 0, do_yield = 0;
  CORBA::Boolean out_of_space = 0, matched = 0;
  CosN::StructuredEvent* event = 0;
  RDI_StructuredEvent* sevnt = 0;
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
      // and state must be RDI_Connected and _active
      if ((_pxstate != RDI_NotConnected) && (_pxstate != RDI_Connected)) {
	break; // also break on exceptional state
      }
      if ((_pxstate == RDI_Connected) && _active) {
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
	RDI_OPLOCK_WAIT(WHATFN); // must wait _pxstate or _active to change
      }
    }
    if ( _pxstate != RDI_Connected ) {
      RDI_THREAD_EXIT(struct_pull_exit, RDIDbgCPxy, RDIDbgCPxy_nm, "pull thread\n");
    }

    // Do not hold OPLOCK across try_pull
    outcall_worked = 0;
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);

      // if we did not wait, yield here (wait or yield each time through outer loop)
      if (do_yield) {
	TW_YIELD();
      }

      try {
	event = _supplier->try_pull_structured_event(hasev);
	outcall_worked = 1;
#ifndef NO_OBJ_GC
	_last_use.set_curtime();
#endif
      } catch ( CORBA::INV_OBJREF& e ) {
	RDIDbgCPxyLog("Pull thread for proxy " << (void*)this << " - Invalid object reference\n");
      } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
	RDIDbgCPxyLog("Pull thread for proxy " << (void*)this << " - Supplier object does not exist\n");
      } catch ( CORBA::COMM_FAILURE& e ) {
	RDIDbgCPxyLog("Pull thread for proxy " << (void*)this << " - Communication Failure\n");
      } catch (...) {
	RDIDbgCPxyLog("Pull thread for proxy " << (void*)this << " - Exception while pulling event from supplier\n");
      }
    } // end unlock scope
    if (!held.cproxy) {
      RDI_Fatal("StructuredProxyPullConsumer_i::_pull_event [**unexpected REACQUIRE failure**]\n");
    }
    update_timeout = 1; // force update of timeout at top of loop

    // add new event to channel, but only if we are still connected
    out_of_space = 0;
    if ( (_pxstate == RDI_Connected) && outcall_worked && hasev && event ) {
      _nevents++; 
      sevnt = new RDI_StructuredEvent(*event);
      RDI_AssertAllocThrowNo(sevnt, "Memory allocation failure -- RDI_StructuredEvent\n");
      if ( (matched = _match_event(event, sevnt)) ) {
	out_of_space = _channel->new_structured_event(sevnt);
	if (out_of_space) {
	  REPORT_REJECT_SE("StructuredProxyPullConsumer", _pserial, *event);
	}
      }
      // Do not delete sevnt unless it was not added to channel
      if (sevnt && (!matched || out_of_space)) {
	delete sevnt;
      }
      sevnt = 0;
    }
    // Creation of sevnt copies event, so delete it regardless
    if ( event ) {
      delete event; event = 0;
    }

    if (_pxstate != RDI_Connected) {
      // disconnected/disposed during try_pull -- do nothing else with my state
    } else {
      if (!outcall_worked) { // exception
	// unregister this proxy -- no more subscription_change msgs
	RDI_ChangePool* cscp = _channel->schange_pool();
	if ( ! _sc_off && cscp ) {
	  cscp->remove_proxy(this);
	}
	_pxstate = RDI_Exception;
	_revoke_offers(held);
      }
    }
  }

 struct_pull_exit:
  TW_EXIT();
}

#undef WHATFN
#define WHATFN "StructuredProxyPullConsumer_i::disconnect_structured_pull_consumer"
void
StructuredProxyPullConsumer_i::disconnect_structured_pull_consumer( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "StructuredProxyPullConsumer_i::_disconnect_client_and_dispose"
// *** caller must have bumped scope lock, held.cproxy set
// Effect: initiate impl dispose
void
StructuredProxyPullConsumer_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
							      CORBA::Boolean            remove_from_admin,
							      WRAPPED_DISPOSEINFO_PTR&  dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgCPxyLog("** StructuredProxyPullConsumer_i::_disconnect_client_and_dispose called twice on same proxy!\n");
    return;
  }
  // unregister this proxy -- no more subscription_change msgs
  RDI_ChangePool* cscp = _channel->schange_pool();
  if ( ! _sc_off && (_pxstate == RDI_Connected) && cscp ) {
    cscp->remove_proxy(this);
  }
  _pxstate = RDI_Disconnected; // acts as guard; following only entered by 1 thread
  // Wait for inuse to drop to one.  This ensures that any call that
  // bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_from_admin) {
    _revoke_offers(held);
  } // else channel is being destroyed so don't bother
  if (remove_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);
      _myadmin->remove_proxy(held, this);
    } // end unlock scope
    if (!held.cproxy) {
      RDI_Fatal("StructuredProxyPullConsumer_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }
  _active  = 0;
  _fa_helper.remove_all_filters(held, 0); // 0 => do not want propagate_schange callback
  _supplier = CosNC::StructuredPullSupplier::_nil();
  if ( _qosprop ) {
    delete _qosprop; _qosprop = 0;
  }
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

// ------------------------------------------------------------- //
// SequenceProxyPullConsumer_i                                   //
// ------------------------------------------------------------- //

SequenceProxyPullConsumer_i::SequenceProxyPullConsumer_i(
							 SupplierAdmin_i* admin,
							 EventChannel_i*  chann,
							 const CosNA::ProxyID&  prxid) : 
  RDIProxyConsumer("SequenceProxyPullConsumer", "SequenceProxyPullConsumer_fa_helper",
		   admin, chann, RDI_C_SeqPRX, CosNA::PULL_SEQUENCE, prxid),
  _worker(0), _thrdone(0), _timeout_s(0), _timeout_n(0)
{
  _supplier = CosNC::SequencePullSupplier::_nil();
  // When the number of pull threads allocated at the channel level is
  // 0, each proxy uses its own thread to pull events from its supplier
  if ( _channel->pull_threads() == 0 ) {
    _worker = new SeqPullWorker(this, &SequenceProxyPullConsumer_i::_pull_event);
    RDI_AssertAllocThrowNo(_worker, "Memory allocation failed -- Thread\n");
    _worker->start();
    _thrdone = 0;
    RDIDbgCPxyLog("Pull thread for proxy " << (void*)this << " -- " << _worker->id());
  }
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

SequenceProxyPullConsumer_i::~SequenceProxyPullConsumer_i()
{
  RDI_OPLOCK_DESTROY_CHECK("SequenceProxyPullConsumer_i");
}

#undef WHATFN
#define WHATFN "SequenceProxyPullConsumer_i::connect_sequence_pull_supplier"
void
SequenceProxyPullConsumer_i::connect_sequence_pull_supplier(CosNC::SequencePullSupplier_ptr supplier
                                                             WRAPPED_IMPLARG )
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
  _supplier = CosNC::SequencePullSupplier::_duplicate(supplier);
  _sc_subscriber = CosNC::NotifySubscribe::_narrow(_supplier); // implicit duplicate
  if (CORBA::is_nil(_sc_subscriber)) {
    RDIDbgCPxyLog("** UNEXPECTED: CosNC::SequencePullSupplier could not be narrowed to CosNC::NotifySubscribe\n");
  }
  _pxstate  = RDI_Connected;
  _active   = 1;

  // if enabled, register this proxy for subscription_change msgs
  RDI_ChangePool* cscp = _channel->schange_pool();
  if ( ! _sc_off && cscp ) {
    cscp->insert_proxy(this);
  }
  if ( _worker ) {	// Notify worker thread
    RDI_OPLOCK_SIGNAL;
  }
  RDI_PullSupplier* cps = _channel->pull_supplier();
  if ( cps ) {
    cps->signal_pull_threads();
  }
}

#undef WHATFN
#define WHATFN "SequenceProxyPullConsumer_i::suspend_connection"
void
SequenceProxyPullConsumer_i::suspend_connection( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_Connected ) {
    throw CosNA::NotConnected();
  }
  if ( ! _active ) {
    throw CosNA::ConnectionAlreadyInactive();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _active = 0;
}

#undef WHATFN
#define WHATFN "SequenceProxyPullConsumer_i::resume_connection"
void
SequenceProxyPullConsumer_i::resume_connection( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_Connected ) {
    throw CosNA::NotConnected();
  }
  if ( _active ) {
    throw CosNA::ConnectionAlreadyActive();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _active = 1;
  if ( _worker ) {    // Notify worker thread
    RDI_OPLOCK_SIGNAL;
  }
  RDI_PullSupplier* cps = _channel->pull_supplier();
  if ( cps ) {
    cps->signal_pull_threads();
  }
}

#undef WHATFN
#define WHATFN "SequenceProxyPullConsumer_i::is_available"
// is_available determines whether a pull should occur.
// in this case the requirement is that either the pull interval is zero
// or an interval timeout has occurred
CORBA::Boolean
SequenceProxyPullConsumer_i::is_available(unsigned long* wait_s, unsigned long* wait_n)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return 0);
  unsigned long pull_interval_s, pull_interval_n;
  unsigned long time_s, time_n;

  if ( (_pxstate != RDI_Connected) || ! _active ) {
    return 0;
  }

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
#define WHATFN "SequenceProxyPullConsumer_i::pull_event"
// REQUIREMENT: A thread from pull pool only calls this method
// if is_available() is true and it thinks state is connected and active.
// (For safety we verify that these conditions actually hold.)
void
SequenceProxyPullConsumer_i::pull_event(CORBA::Boolean& invalid)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { return; }

  CORBA::Boolean outcall_worked, hasev;
  unsigned long pull_interval_s, pull_interval_n;
  unsigned long time_s, time_n;
  _channel->pull_period_s_n(pull_interval_s, pull_interval_n);

  CORBA::Boolean out_of_space = 0, matched = 0;
  CORBA::ULong   ex = 0;
  CosN::EventBatch* events = 0;
  RDI_StructuredEvent* sevnt = 0;

  invalid = 0;
  if ( (_pxstate == RDI_Disconnected) || (_pxstate == RDI_Exception) ) {
    // tell caller no more pulls (by setting invalid to 1)
    invalid = 1;
    return;
  }
  if ( (_pxstate != RDI_Connected) || ! _active ) {
    return;
  }

  // sanity check: if pull interval is positive, we should only be called if a timeout occurred
  if (pull_interval_s || pull_interval_n) {
    TW_GET_TIME(&time_s, &time_n);
    if ((_timeout_s == 0) && (_timeout_n == 0)) { // timeout not set; set it now
      TW_GET_TIME(&_timeout_s, &_timeout_n, pull_interval_s, pull_interval_n);
    } 
    if ( (time_s < _timeout_s) || ((time_s == _timeout_s) && (time_n < _timeout_n)) ) {
      RDIDbgCPxyLog("** INTERNAL ERROR: ProxyPullConsumer_i::pull_event called too soon\n");
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

  // determine max batch size
  CORBA::ULong mxnum = _qosprop->maximumBatchSize();
  if (mxnum == 0)
    mxnum = 5;

  // Do not hold OPLOCK across try_pull
  outcall_worked = 0;
  hasev = 0;
  { // introduce unlock scope
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);

    try {
      events = _supplier->try_pull_structured_events(mxnum, hasev);
      outcall_worked = 1;
#ifndef NO_OBJ_GC
      _last_use.set_curtime();
#endif
    } catch ( CORBA::INV_OBJREF& e ) {
      RDIDbgCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Invalid object reference\n");
    } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
      RDIDbgCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Supplier object does not exist\n");
    } catch ( CORBA::COMM_FAILURE& e ) {
      RDIDbgCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Communication Failure\n");
    } catch (...) {
      RDIDbgCPxyLog("Pull pool calling pull_event for proxy " << (void*)this << " - Exception while pulling event from supplier\n");
    }
  } // end unlock scope
  if (!held.cproxy) {
    RDI_Fatal("SequenceProxyPullConsumer_i::pull_event [**unexpected REACQUIRE failure**]\n");
  }

  // add new event to channel, but only if we are still connected
  out_of_space = 0;
  if ( (_pxstate == RDI_Connected) && outcall_worked && hasev && events ) {
    _nevents += events->length();
    for (ex = 0; (out_of_space == 0) && (ex < events->length()); ex++) {
      sevnt = new RDI_StructuredEvent((*events)[ex]);
      RDI_AssertAllocThrowNo(sevnt,"Memory allocation failure -- RDI_StructuredEvent\n");
      if ( (matched = _match_event(&(*events)[ex], sevnt)) ) {
	out_of_space = _channel->new_structured_event(sevnt);
	if (out_of_space) {
	  REPORT_REJECT_SE("SequenceProxyPullConsumer", _pserial, (*events)[ex]);
	}
      }
      // Do not delete sevnt unless it was not added to channel
      if (sevnt && (!matched || out_of_space)) {
	delete sevnt;
      }
      sevnt = 0;
    }
  }
  // events can all be deleted because each was either copied or not added
  if ( events ) {
    delete events; events = 0;
  }

  if (_pxstate != RDI_Connected) {
    // disconnected/disposed during try_pull -- do nothing else with my state
  } else {
    if (!outcall_worked) { // exception
      // unregister this proxy -- no more subscription_change msgs
      RDI_ChangePool* cscp = _channel->schange_pool();
      if ( ! _sc_off && cscp ) {
	cscp->remove_proxy(this);
      }
      _pxstate = RDI_Exception;
      _revoke_offers(held);
      invalid  = 1;
    }
  }
}

#undef WHATFN
#define WHATFN "SequenceProxyPullConsumer_i::_pull_event"
void
SequenceProxyPullConsumer_i::_pull_event()
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) return;

  CORBA::Boolean outcall_worked, hasev;
  CORBA::Boolean update_timeout = 0, do_yield = 0, matched = 0;
  CORBA::Boolean out_of_space = 0;
  CORBA::ULong ex = 0;
  CosN::EventBatch* events = 0;
  RDI_StructuredEvent* sevnt = 0;

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
      // and state must be RDI_Connected and _active
      if ((_pxstate != RDI_NotConnected) && (_pxstate != RDI_Connected)) {
	break; // also break on exceptional state
      }
      if ((_pxstate == RDI_Connected) && _active) {
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
	RDI_OPLOCK_WAIT(WHATFN); // must wait _pxstate or _active to change
      }
    }
    if ( _pxstate != RDI_Connected ) {
      RDI_THREAD_EXIT(seq_pull_exit, RDIDbgCPxy, RDIDbgCPxy_nm, "pull thread\n");
    }

    // determine max batch size
    CORBA::ULong mxnum = _qosprop->maximumBatchSize();
    if (mxnum == 0)
      mxnum = 5;

    // Do not hold OPLOCK across try_pull
    outcall_worked = 0;
    hasev = 0;
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);

      // if we did not wait, yield here (wait or yield each time through outer loop)
      if (do_yield) {
	TW_YIELD();
      }

      try {
	events = _supplier->try_pull_structured_events(mxnum, hasev);
	outcall_worked = 1;
#ifndef NO_OBJ_GC
	_last_use.set_curtime();
#endif
      } catch ( CORBA::INV_OBJREF& e ) {
	RDIDbgCPxyLog("Pull thread for proxy " << (void*)this << " - Invalid object reference\n");
      } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
	RDIDbgCPxyLog("Pull thread for proxy " << (void*)this << " - Supplier object does not exist\n");
      } catch ( CORBA::COMM_FAILURE& e ) {
	RDIDbgCPxyLog("Pull thread for proxy " << (void*)this << " - Communication Failure\n");
      } catch (...) {
	RDIDbgCPxyLog("Pull thread for proxy " << (void*)this << " - Exception while pulling event from supplier\n");
      }
    } // end unlock scope
    if (!held.cproxy) {
      RDI_Fatal("SequenceProxyPullConsumer_i::_pull_event [**unexpected REACQUIRE failure**]\n");
    }

    update_timeout = 1; // force update of timeout at top of loop

    // add new event to channel, but only if we are still connected
    out_of_space = 0;
    if ( (_pxstate == RDI_Connected) && outcall_worked && hasev && events ) {
      _nevents += events->length();
      for (ex = 0; (out_of_space == 0) && (ex < events->length()); ex++) {
	sevnt = new RDI_StructuredEvent((*events)[ex]);
	RDI_AssertAllocThrowNo(sevnt,"Memory allocation failure -- RDI_StructuredEvent\n");
	if ( (matched = _match_event(&(*events)[ex], sevnt)) ) {
	  out_of_space = _channel->new_structured_event(sevnt);
	  if (out_of_space) {
	    REPORT_REJECT_SE("SequenceProxyPullConsumer", _pserial, (*events)[ex]);
	  }
	}
	// Do not delete sevnt unless it was not added to channel
	if (sevnt && (!matched || out_of_space)) {
	  delete sevnt;
	}
	sevnt = 0;
      }
    }
    // events can all be deleted because each was either copied or not added
    if ( events ) {
      delete events; events = 0;
    }

    if (_pxstate != RDI_Connected) {
      // disconnected/disposed during try_pull -- do nothing else with my state
    } else {
      if (!outcall_worked) { // exception
	// unregister this proxy -- no more subscription_change msgs
	RDI_ChangePool* cscp = _channel->schange_pool();
	if ( ! _sc_off && cscp ) {
	  cscp->remove_proxy(this);
	}
	_pxstate = RDI_Exception;
	_revoke_offers(held);
      }
    }
  }

 seq_pull_exit:
  TW_EXIT();
}

#undef WHATFN
#define WHATFN "SequenceProxyPullConsumer_i::disconnect_sequence_pull_consumer"
void
SequenceProxyPullConsumer_i::disconnect_sequence_pull_consumer( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "SequenceProxyPullConsumer_i::_disconnect_client_and_dispose"
// *** caller must have bumped scope lock, held.cproxy set
// Effect: initiate impl dispose
void
SequenceProxyPullConsumer_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
							    CORBA::Boolean remove_from_admin,
							    WRAPPED_DISPOSEINFO_PTR& dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgCPxyLog("** SequenceProxyPullConsumer_i::_disconnect_client_and_dispose called twice on same proxy!\n");
    return;
  }
  // unregister this proxy -- no more subscription_change msgs
  RDI_ChangePool* cscp = _channel->schange_pool();
  if ( ! _sc_off && (_pxstate == RDI_Connected) && cscp ) {
    cscp->remove_proxy(this);
  }

  _pxstate = RDI_Disconnected; // acts as guard; following only entered by 1 thread
  // Wait for inuse to drop to one.  This ensures that any call that
  // bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_from_admin) {
    _revoke_offers(held);
  } // else channel is being destroyed so don't bother
  if (remove_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);
      _myadmin->remove_proxy(held, this);
    } // end unlock scope
    if (!held.cproxy) {
      RDI_Fatal("SequenceProxyPullConsumer_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }
  _active  = 0;
  _fa_helper.remove_all_filters(held, 0); // 0 => do not want propagate_schange callback
  _supplier = CosNC::SequencePullSupplier::_nil();
  if ( _qosprop ) {
    delete _qosprop; _qosprop = 0;
  }
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

// ------------------------------------------------------------- //
// ProxyPushConsumer_i                                           //
// ------------------------------------------------------------- //

ProxyPushConsumer_i::ProxyPushConsumer_i(SupplierAdmin_i* admin,
					 EventChannel_i*  evchn,
					 const CosNA::ProxyID&  prxid) : 
  RDIProxyConsumer("ProxyPushConsumer", "ProxyPushConsumer_fa_helper",
		   admin, evchn, RDI_C_AnyPRX, CosNA::PUSH_ANY, prxid)
{
  _supplier = CosEvC::PushSupplier::_nil();
  _nc_supplier = CosNC::PushSupplier::_nil();
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

ProxyPushConsumer_i::~ProxyPushConsumer_i()
{
  RDI_OPLOCK_DESTROY_CHECK("ProxyPushConsumer_i");
}

#undef WHATFN
#define WHATFN "ProxyPushConsumer_i::push"
void
ProxyPushConsumer_i::push(const CORBA::Any& event WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_Connected ) {
    throw CosEvC::Disconnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _nevents += 1;
  CORBA::Boolean matched = _match_event(event);
  if ( matched ) {
    if (_channel->new_any_event(event) ) { // event not added due to limits
      REPORT_REJECT_ANY("ProxyPushConsumer", _pserial, event);
      throw CORBA::IMP_LIMIT(0, CORBA::COMPLETED_NO);
    }
  }
}

#undef WHATFN
#define WHATFN "ProxyPushConsumer_i::connect_any_push_supplier"
void
ProxyPushConsumer_i::connect_any_push_supplier(CosEvC::PushSupplier_ptr supplier WRAPPED_IMPLARG)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( CORBA::is_nil(supplier) ) {
    throw CORBA::BAD_PARAM(0, CORBA::COMPLETED_NO);
  }
  if ( (_pxstate != RDI_NotConnected) || ! CORBA::is_nil(_supplier) ) {
    throw CosEvCA::AlreadyConnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _pxstate = RDI_Connected;
  _active  = 1;
  _supplier = CosEvC::PushSupplier::_duplicate(supplier);
  // If supplier is a CosNC::PushSupplier, set _nc_supplier
  _nc_supplier = CosNC::PushSupplier::_narrow(supplier); // implicit duplicate

  if (! CORBA::is_nil(_nc_supplier)) {
    _sc_subscriber = CosNC::NotifySubscribe::_narrow(_nc_supplier); // implicit duplicate
    if (CORBA::is_nil(_sc_subscriber)) {
      RDIDbgCPxyLog("** UNEXPECTED: CosNC::PushSupplier could not be narrowed to CosNC::NotifySubscribe\n");
    }
    // if enabled, register this proxy for subscription_change msgs
    RDI_ChangePool* cscp = _channel->schange_pool();
    if ( ! _sc_off && cscp ) {
      cscp->insert_proxy(this);
    }
  }
}

#undef WHATFN
#define WHATFN "ProxyPushConsumer_i::disconnect_push_consumer"
void
ProxyPushConsumer_i::disconnect_push_consumer( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "ProxyPushConsumer_i::_disconnect_client_and_dispose"
// *** caller must have bumped scope lock, held.cproxy set
// Effect: initiate impl dispose
void
ProxyPushConsumer_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
						    CORBA::Boolean remove_from_admin,
						    WRAPPED_DISPOSEINFO_PTR& dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgCPxyLog("** ProxyPushConsumer_i::_disconnect_client_and_dispose called twice on same proxy!\n");
    return;
  }
  // unregister this proxy -- no more subscription_change msgs
  RDI_ChangePool* cscp = _channel->schange_pool();
  if ( ! _sc_off && (_pxstate == RDI_Connected) && ! CORBA::is_nil(_nc_supplier) && cscp ) {
    cscp->remove_proxy(this);
  }
  _pxstate = RDI_Disconnected; // acts as guard -- only one thread does the following
  // Wait for inuse to drop to one.  This ensures that any call that
  // bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_from_admin) {
    _revoke_offers(held);
  } // else channel is being destroyed so don't bother
  if (remove_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);
      _myadmin->remove_proxy(held, this);
    } // end unlock scope
    if (!held.cproxy) {
      RDI_Fatal("ProxyPushConsumer_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }
  _fa_helper.remove_all_filters(held, 0); // 0 => do not want propagate_schange callback
  _supplier = CosEvC::PushSupplier::_nil();
  _nc_supplier = CosNC::PushSupplier::_nil();
  if ( _qosprop ) {
    delete _qosprop; _qosprop = 0;
  }
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

// ------------------------------------------------------------- //
// StructuredProxyPushConsumer_i                                 //
// ------------------------------------------------------------- //

StructuredProxyPushConsumer_i::StructuredProxyPushConsumer_i(
							     SupplierAdmin_i* admin, 	
							     EventChannel_i*  evchn,
							     const CosNA::ProxyID&  prxid) :
  RDIProxyConsumer("StructuredProxyPushConsumer", "StructuredProxyPushConsumer_fa_helper",
		   admin, evchn, RDI_C_StrPRX, CosNA::PUSH_STRUCTURED, prxid)
{
  _supplier = CosNC::StructuredPushSupplier::_nil();
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

StructuredProxyPushConsumer_i::~StructuredProxyPushConsumer_i()
{
  RDI_OPLOCK_DESTROY_CHECK("StructuredProxyPushConsumer_i");
}

#undef WHATFN
#define WHATFN "StructuredProxyPushConsumer_i::push_structured_event"
void
StructuredProxyPushConsumer_i::push_structured_event(const CosN::StructuredEvent& event
                                                      WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  CORBA::Boolean matched = 0, out_of_space = 0; 
  if ( _pxstate != RDI_Connected ) {
    throw CosEvC::Disconnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _nevents += 1;
  RDI_StructuredEvent* sevnt = new RDI_StructuredEvent(event);
  RDI_AssertAllocThrowNo(sevnt, "Memory allocation failure -- RDI_StructuredEvent\n");
  if ( (matched = _match_event(&event, sevnt)) ) {
    out_of_space = _channel->new_structured_event(sevnt);
    if (out_of_space) {
      REPORT_REJECT_SE("StructuredProxyPushConsumer", _pserial, event);
    }
  }
  // Do not delete sevnt unless it was not added to channel
  if (sevnt && (!matched || out_of_space)) {
    delete sevnt;
  }
  sevnt = 0;
  if ( out_of_space ) {
    throw CORBA::IMP_LIMIT(0, CORBA::COMPLETED_NO);
  }
}

#undef WHATFN
#define WHATFN "StructuredProxyPushConsumer_i::connect_structured_push_supplier"
void
StructuredProxyPushConsumer_i::connect_structured_push_supplier(CosNC::StructuredPushSupplier_ptr supplier
                                                                 WRAPPED_IMPLARG)
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
  _pxstate = RDI_Connected;
  _active  = 1;
  _supplier= CosNC::StructuredPushSupplier::_duplicate(supplier);
  _sc_subscriber = CosNC::NotifySubscribe::_narrow(_supplier); // implicit duplicate
  if (CORBA::is_nil(_sc_subscriber)) {
    RDIDbgCPxyLog("** UNEXPECTED: CosNC::StructuredPushSupplier could not be narrowed to CosNC::NotifySubscribe\n");
  }
  // if enabled, register this proxy for subscription_change msgs
  RDI_ChangePool* cscp = _channel->schange_pool();
  if ( ! _sc_off && cscp ) {
    cscp->insert_proxy(this);
  }
}

#undef WHATFN
#define WHATFN "StructuredProxyPushConsumer_i::disconnect_structured_push_consumer"
void
StructuredProxyPushConsumer_i::disconnect_structured_push_consumer( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "StructuredProxyPushConsumer_i::_disconnect_client_and_dispose"
// *** caller must have bumped scope lock, held.cproxy set
// Effect: initiate impl dispose
void
StructuredProxyPushConsumer_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
							      CORBA::Boolean remove_from_admin,
							      WRAPPED_DISPOSEINFO_PTR& dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgCPxyLog("** StructuredProxyPushConsumer_i::_disconnect_client_and_dispose called twice on same proxy!\n");
    return;
  }
  // unregister this proxy -- no more subscription_change msgs
  RDI_ChangePool* cscp = _channel->schange_pool();
  if ( ! _sc_off && (_pxstate == RDI_Connected) && cscp ) {
    cscp->remove_proxy(this);
  }
  _pxstate = RDI_Disconnected; // acts as guard -- only one thread does the following
  // Wait for inuse to drop to one.  This ensures that any call that
  // bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_from_admin) {
    _revoke_offers(held);
  } // else channel is being destroyed so don't bother
  if (remove_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);
      _myadmin->remove_proxy(held, this);
    } // end unlock scope
    if (!held.cproxy) {
      RDI_Fatal("StructuredProxyPushConsumer_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }
  _fa_helper.remove_all_filters(held, 0); // 0 => do not want propagate_schange callback
  _supplier = CosNC::StructuredPushSupplier::_nil();
  if ( _qosprop ) {
    delete _qosprop; _qosprop = 0;
  }
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

// ------------------------------------------------------------- //
// SequenceProxyPushConsumer_i                                   //
// ------------------------------------------------------------- //

SequenceProxyPushConsumer_i::SequenceProxyPushConsumer_i(SupplierAdmin_i* admin,
							 EventChannel_i*  evchn,
							 const CosNA::ProxyID&  prxid) :
  RDIProxyConsumer("SequenceProxyPushConsumer", "SequenceProxyPushConsumer_fa_helper",
		   admin, evchn, RDI_C_SeqPRX, CosNA::PUSH_SEQUENCE, prxid)
{
  _supplier = CosNC::SequencePushSupplier::_nil();
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

SequenceProxyPushConsumer_i::~SequenceProxyPushConsumer_i()
{
  RDI_OPLOCK_DESTROY_CHECK("SequenceProxyPushConsumer_i");
}

#undef WHATFN
#define WHATFN "SequenceProxyPushConsumer_i::push_structured_events"
void 
SequenceProxyPushConsumer_i::push_structured_events(const CosN::EventBatch& events WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  CORBA::ULong ex = 0;
  CORBA::Boolean matched = 0, out_of_space = 0;
  RDI_StructuredEvent* sevnt = 0;
  if ( _pxstate != RDI_Connected ) {
    throw CosEvC::Disconnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _nevents += events.length();
  out_of_space = 0;
  for (ex = 0; (out_of_space == 0) && (ex < events.length()); ex++) {
    sevnt = new RDI_StructuredEvent(events[ex]);
    RDI_AssertAllocThrowNo(sevnt,"Memory allocation failure -- RDI_StructuredEvent\n");
    if ( (matched = _match_event(&events[ex], sevnt)) ) {
      out_of_space = _channel->new_structured_event(sevnt);
      if (out_of_space) {
	REPORT_REJECT_SE("SequenceProxyPushConsumer", _pserial, events[ex]);
      }
    }
    // Do not delete sevnt unless it was not added to channel
    if (sevnt && (!matched || out_of_space)) {
      delete sevnt;
    }
    sevnt = 0;
  }
  if ( out_of_space ) {
    throw CORBA::IMP_LIMIT(0, CORBA::COMPLETED_NO);
  }
}

#undef WHATFN
#define WHATFN "SequenceProxyPushConsumer_i::connect_sequence_push_supplier"
void
SequenceProxyPushConsumer_i::connect_sequence_push_supplier(CosNC::SequencePushSupplier_ptr supplier
                                                             WRAPPED_IMPLARG)
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
  _pxstate = RDI_Connected;
  _active  = 1;
  _supplier = CosNC::SequencePushSupplier::_duplicate(supplier);
  _sc_subscriber = CosNC::NotifySubscribe::_narrow(_supplier); // implicit duplicate
  if (CORBA::is_nil(_sc_subscriber)) {
    RDIDbgCPxyLog("** UNEXPECTED: CosNC::SequencePushSupplier could not be narrowed to CosNC::NotifySubscribe\n");
  }
  // if enabled, register this proxy for subscription_change msgs
  RDI_ChangePool* cscp = _channel->schange_pool();
  if ( ! _sc_off && cscp ) {
    cscp->insert_proxy(this);
  }
}

#undef WHATFN
#define WHATFN "SequenceProxyPushConsumer_i::disconnect_sequence_push_consumer"
void
SequenceProxyPushConsumer_i::disconnect_sequence_push_consumer( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "SequenceProxyPushConsumer_i::_disconnect_client_and_dispose"
// *** caller must have bumped scope lock, held.cproxy set
// Effect: initiate impl dispose
void
SequenceProxyPushConsumer_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
							    CORBA::Boolean remove_from_admin,
							    WRAPPED_DISPOSEINFO_PTR& dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgCPxyLog("** SequenceProxyPushConsumer_i::_disconnect_client_and_dispose called twice on same proxy!\n");
    return;
  }
  // unregister this proxy -- no more subscription_change msgs
  RDI_ChangePool* cscp = _channel->schange_pool();
  if ( ! _sc_off && (_pxstate == RDI_Connected) && cscp ) {
    cscp->remove_proxy(this);
  }
  _pxstate = RDI_Disconnected; // acts as guard -- only one thread does the following
  // Wait for inuse to drop to one.  This ensures that any call that
  // bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_from_admin) {
    _revoke_offers(held);
  } // else channel is being destroyed so don't bother
  if (remove_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cproxy, WHATFN);
      _myadmin->remove_proxy(held, this);
    } // end unlock scope
    if (!held.cproxy) {
      RDI_Fatal("SequenceProxyPushConsumer_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }
  _fa_helper.remove_all_filters(held, 0); // 0 => do not want propagate_schange callback
  _supplier = CosNC::SequencePushSupplier::_nil();
  if ( _qosprop ) {
    delete _qosprop; _qosprop = 0;
  }
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

////////////////////////////////////////
// Logging

RDIstrstream&
ProxyPullConsumer_i::log_output(RDIstrstream& str) const
{
  str << (void*)this << " -- " << RDI_PRX_TYPE(_prxtype) << " ID ";
  str.setw(3); str << _pserial;
  if ( CORBA::is_nil(_nc_supplier) ) str << " CosEventComm Consumer";
  str << _pxstate;
  str << (_active ? "Active " : "Suspended ");
  return str << " #Pull " << _nevents;
}

RDIstrstream&
StructuredProxyPullConsumer_i::log_output(RDIstrstream& str) const
{
  str << (void*)this << " -- " << RDI_PRX_TYPE(_prxtype) << " ID ";
  str.setw(3); str << _pserial;
  str << _pxstate;
  str << (_active ? "Active " : "Suspended ");
  return str << " #Pull " << _nevents;
}

RDIstrstream&
SequenceProxyPullConsumer_i::log_output(RDIstrstream& str) const
{
  str << (void*)this << " -- " << RDI_PRX_TYPE(_prxtype) << " ID ";
  str.setw(3); str << _pserial;
  str << _pxstate;
  str << (_active ? "Active " : "Suspended ");
  return str << " #Pull " << _nevents;
}

RDIstrstream&
ProxyPushConsumer_i::log_output(RDIstrstream& str) const
{
  str << (void*)this << " -- " << RDI_PRX_TYPE(_prxtype);
  if ( CORBA::is_nil(_nc_supplier ) ) str << " CosEventComm Supplier";
  str << _pxstate;
  return str << " #Events " << _nevents;
}

RDIstrstream&
StructuredProxyPushConsumer_i::log_output(RDIstrstream& str) const
{
  str << (void*)this << " -- "  << RDI_PRX_TYPE(_prxtype) << " ID ";
  str.setw(3); str << _pserial;
  str << _pxstate;
  return str << " #Events " << _nevents;
}

RDIstrstream&
SequenceProxyPushConsumer_i::log_output(RDIstrstream& str) const
{
  str << (void*)this  << " -- "  << RDI_PRX_TYPE(_prxtype) << " ID ";
  str.setw(3); str << _pserial;
  str << _pxstate;
  return str << " #Events " << _nevents;
}

////////////////////////////////////////
// Interactive

#undef WHATFN
#define WHATFN "RDIProxyConsumer::my_name"
AttN::NameSeq*
RDIProxyConsumer::my_name( WRAPPED_IMPLARG_VOID )
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
#define WHATFN "RDIProxyConsumer::child_names"
AttN::NameSeq*
RDIProxyConsumer::child_names( WRAPPED_IMPLARG_VOID )
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

#undef WHATFN
#define WHATFN "RDIProxyConsumer::out_info_filters"
void
RDIProxyConsumer::out_info_filters(RDIstrstream& str)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return);
  str << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n";
  str << "Proxy Filters attached to " << _my_name << '\n';
  str << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n";
  _fa_helper.out_info_filters(str);
}

void
RDIProxyConsumer::out_config(RDIstrstream& str)
{
  str << "----------------------------------------------------------------------\n";
  str << "NotifQoS settings for " << _my_name << '\n';
  str << "----------------------------------------------------------------------\n";
  _qosprop->log_output(str);
  str << '\n';
}

CORBA::Boolean
RDIProxyConsumer::do_set_command(RDIstrstream& str, RDIParseCmd& p)
{
  CosN::QoSProperties    n_qos;
  CosN::AdminProperties  a_qos;
  AttN::ServerProperties s_qos;
  CORBA::Boolean success = 1;
  success = RDI_AllQoS::parse_set_command(str, p, _otype, n_qos, a_qos, s_qos);
  if (success && n_qos.length() > 0) {
    try {
      set_qos(n_qos);
    } catch (CosN::UnsupportedQoS& e) {
      str << "\nThe following NotifQoS Property Settings are invalid:\n";
      RDI_describe_prop_errors(str, e.qos_err);
      str << '\n';
      success = 0;
    } catch (CORBA::INV_OBJREF) { throw; }
    if (success) {
      str << '\n';
      for (unsigned int i = 0; i < n_qos.length(); i++) {
	str << n_qos[i].name << " set to "; RDI_pp_any(str, n_qos[i].value); str << '\n';
      }
      str << "\nSome properties updated successfully.  Current settings:\n\n";
      out_config(str);
    }
  }
  return success;
}

void
RDIProxyConsumer::out_commands(RDIstrstream& str)
{
  str << "omniNotify ProxyConsumer commands:\n"
      << "  up                : change target to proxy's admin\n"
      << "  debug             : show debugging information for this proxy\n"
      << "  config            : show config information for this proxy\n"
      << "  info filters      : show filters attached to this proxy\n"
      << "  set <name1> <value1> [ <name2> <value2> ... ]\n"
      << "                    : set NotifQoS property name1 to value1, name2 to value2, etc.\n";
}

char*
RDIProxyConsumer::do_command(const char* cmnd, CORBA::Boolean& success,
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
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "config")) {
    out_config(str);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "info") && RDI_STR_EQ_I(p.argv[1], "filters")) {
    out_info_filters(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "up")) {
    target_changed = 1;
    next_target = WRAPPED_IMPL2OREF(AttN::SupplierAdmin, _myadmin);
    str << "\nomniNotify: new target ==> " << _my_name[_my_name.length()-2] << '\n';
  } else if ((p.argc >= 1) && RDI_STR_EQ_I(p.argv[0], "set")) {
    success = do_set_command(str, p);
  } else {
    str << "Invalid command: " << cmnd << "\n";
    success = 0;
  }
  RDIRptInteractiveLog(_my_name << " received command: " << cmnd << "\nResult:\n" << str.buf());
  // this is the only safe way to return a string?
  return CORBA_STRING_DUP(str.buf());
}
