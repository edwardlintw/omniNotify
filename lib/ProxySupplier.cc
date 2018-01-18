//-*- Mode: C++; -*-
//                              File      : ProxySupplier.cc
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
//    Implementation of Supplier proxies
//
 
#include "thread_wrappers.h"
#include "RDI.h"
#include "RDIStringDefs.h"
#include "RDIEvent.h"
#include "RDIOplocks.h"
#include "RDIOplocksMacros.h"

#include "CosNfyUtils.h"
#include "CosNotifyShorthands.h"
#include "CosNotifyChannelAdmin_i.h"
#include "RDITypeMap.h"

extern const char* RDI_PRX_TYPE(const CosNA::ProxyType& type);

// ------------------------------------------------------------- //
// Support classes needed for implementing  the thread-per-proxy //
// notification approach                                         //
// ------------------------------------------------------------- //

typedef void (ProxyPushSupplier_i::*AnyPushSupplierMethod)(void);

class AnyProxyWorker : public TW_Thread {
public:
  AnyProxyWorker(ProxyPushSupplier_i* p, AnyPushSupplierMethod m) 
    : TW_Thread(NULL,PRIORITY_NORMAL), _proxy(p), _method(m) {;}
  void run(void *)      { (_proxy->*_method)(); }
private:
  ProxyPushSupplier_i*  _proxy;
  AnyPushSupplierMethod _method;
  AnyProxyWorker()  {;}
};

class AnyProxyBoundWorker : public TW_Thread {
public:
  AnyProxyBoundWorker(ProxyPushSupplier_i* p, AnyPushSupplierMethod m) 
    : TW_Thread(NULL,PRIORITY_NORMAL), _proxy(p), _method(m) 
  { start_undetached();   }
  void* run_undetached(void *)	{ (_proxy->*_method)(); return 0; }
private:
  ProxyPushSupplier_i*  _proxy;
  AnyPushSupplierMethod _method;
  AnyProxyBoundWorker()  {;}
};

typedef void (StructuredProxyPushSupplier_i::*StrPushSupplierMethod)(void);

class StrProxyWorker : public TW_Thread {
public:
  StrProxyWorker(StructuredProxyPushSupplier_i* p, StrPushSupplierMethod m) 
    : TW_Thread(NULL,PRIORITY_NORMAL), _proxy(p), _method(m) {;}
  void run(void *)      { (_proxy->*_method)(); }
private:
  StructuredProxyPushSupplier_i* _proxy;
  StrPushSupplierMethod          _method;
  StrProxyWorker()  {;}
};

class StrProxyBoundWorker : public TW_Thread {
public:
  StrProxyBoundWorker(StructuredProxyPushSupplier_i* p, StrPushSupplierMethod m)
    : TW_Thread(NULL,PRIORITY_NORMAL), _proxy(p), _method(m) 
  { start_undetached();   }
  void* run_undetached(void *)  { (_proxy->*_method)(); return 0; }
private:
  StructuredProxyPushSupplier_i* _proxy;
  StrPushSupplierMethod          _method;
  StrProxyBoundWorker()  {;}
};

typedef void (SequenceProxyPushSupplier_i::*SeqPushSupplierMethod)(void);

class SeqProxyWorker : public TW_Thread {
public:
  SeqProxyWorker(SequenceProxyPushSupplier_i* p, SeqPushSupplierMethod m) 
    : TW_Thread(NULL,PRIORITY_NORMAL), _proxy(p), _method(m) {;}
  void run(void *)      { (_proxy->*_method)(); }
private:
  SequenceProxyPushSupplier_i* _proxy;
  SeqPushSupplierMethod        _method;
  SeqProxyWorker()  {;}
};

class SeqProxyBoundWorker : public TW_Thread {
public:
  SeqProxyBoundWorker(SequenceProxyPushSupplier_i* p, SeqPushSupplierMethod m) 
    : TW_Thread(NULL,PRIORITY_NORMAL), _proxy(p), _method(m) 
  { start_undetached();   }
  void* run_undetached(void *)  { (_proxy->*_method)(); return 0; }
private:
  SequenceProxyPushSupplier_i* _proxy;
  SeqPushSupplierMethod        _method;
  SeqProxyBoundWorker()  {;}
};

// ------------------------------------------------------------- //
// RDIProxySupplier                                              //
// ------------------------------------------------------------- //

RDIProxySupplier::RDIProxySupplier(const char*            resty,
				   const char*            fa_helper_resty,
				   ConsumerAdmin_i*       admin, 
				   EventChannel_i*        chann,
				   const RDI_ObjectKind   otype,
				   const CosNA::ProxyType prtype,
				   const CosNA::ProxyID&  prxid) : 
  RDINotifySubscribe(),
  _oplockptr(0), _my_name(admin->L_my_name()),
  _fa_helper(fa_helper_resty), _channel(chann), _myadmin(admin), 
  _otype(otype), _prxtype(prtype), _pserial(prxid), _nevents(0),
  _pxstate(RDI_NotConnected), _active(0),
  _qosprop(0), _oc_off(1), _rqstypes(), _ntfqueue()
{
  RDI_OPLOCK_INIT(resty);
  char buf[20];
  sprintf(buf, "proxy%ld", prxid);
  _my_name.length(_my_name.length()+1);
  _my_name[_my_name.length()-1] = (const char*)buf;
  _qosprop = new RDI_NotifQoS(_myadmin->qos_properties());
  RDI_AssertAllocThrowNo(_qosprop, "Memory allocation failure - RDI_NotifQoS\n");
  _oc_subscriber = CosNC::NotifyPublish::_nil();
  _rqstypes.length(0);
  _pfilter  = CosNF::MappingFilter::_nil();
  _lfilter  = CosNF::MappingFilter::_nil();
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
}

RDIProxySupplier::~RDIProxySupplier()
{
  // RDIDbgForceLog("XXX_REMOVE ~RDIProxySupplier called for " << RDI_PRX_TYPE(_prxtype) << " proxy " << _pserial << '\n');
  RDI_OPLOCK_DESTROY_CHECK("RDIProxySupplier");
}

// -------- ** Routines that implement public CORBA interfaces ** -------------

#undef WHATFN
#define WHATFN "RDIProxySupplier::MyType"
CosNA::ProxyType
RDIProxySupplier::MyType()
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNA::ProxyType res = _prxtype;
  return res;
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::MyID"
CosNA::ProxyID
RDIProxySupplier::MyID()
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNA::ProxyID res = _pserial; 
  return res;
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::MyAdmin"
CosNA::ConsumerAdmin_ptr
RDIProxySupplier::MyAdmin()
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNA::ConsumerAdmin_ptr res = WRAPPED_IMPL2OREF(CosNA::ConsumerAdmin, _myadmin);
  return res;
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::priority_filter"
CosNF::MappingFilter_ptr
RDIProxySupplier::priority_filter()
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNF::MappingFilter_ptr res = CosNF::MappingFilter::_nil();
  if ( !CORBA::is_nil(_pfilter) ) {
    res =  CosNF::MappingFilter::_duplicate(_pfilter);
  }
  return res;
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::priority_filter"
void
RDIProxySupplier::priority_filter(CosNF::MappingFilter_ptr prio_filter)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  if ( CORBA::is_nil(prio_filter) ) {
    throw CORBA::BAD_PARAM(0, CORBA::COMPLETED_NO);
  }
  _pfilter = CosNF::MappingFilter::_duplicate(prio_filter);
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::lifetime_filter"
CosNF::MappingFilter_ptr
RDIProxySupplier::lifetime_filter()
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNF::MappingFilter_ptr res = CosNF::MappingFilter::_nil();
  if ( !CORBA::is_nil(_lfilter) ) {
    res = CosNF::MappingFilter::_duplicate(_lfilter);
  }
  return res;
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::lifetime_filter"
void
RDIProxySupplier::lifetime_filter(CosNF::MappingFilter_ptr life_filter)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  if ( CORBA::is_nil(life_filter) ) {
    throw CORBA::BAD_PARAM(0, CORBA::COMPLETED_NO);
  }
  _lfilter = CosNF::MappingFilter::_duplicate(life_filter); 
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::obtain_offered_types"
CosN::EventTypeSeq* 
RDIProxySupplier::obtain_offered_types(CosNA::ObtainInfoMode mode)
{
  // to achieve atomicity for CosNA::ALL_NOW_UPDATES_ON we must
  // ask _channel to do this operation so that lock acquisition
  // order will be _channel followed by this + _channel->ochange_pool
  // (via call to _enable_updates or _disable_updates)
  CosN::EventTypeSeq* res;

  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  { // introduce unlock scope
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
    res = _channel->pxy_obtain_offered_types(this, mode);
  } // end unlock scope
  if (!held.sproxy) {
    RDI_Fatal("RDIProxySupplier::obtain_offered_types [**unexpected REACQUIRE failure**]\n");
  }
  return res;
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::validate_event_qos"
void
RDIProxySupplier::validate_event_qos(const CosN::QoSProperties& r_qos,
				     CosN_NamedPropertyRangeSeq_outarg rseq)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
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
#define WHATFN "RDIProxySupplier::get_qos"
CosN::QoSProperties*
RDIProxySupplier::get_qos()
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosN::QoSProperties* res = _qosprop->get_qos(_otype);
  return res;
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::_qos_changed"
void
RDIProxySupplier::_qos_changed(RDI_LocksHeld& held)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate != RDI_Disconnected) { 
    _ntfqueue.qos_changed(_qosprop);
  }
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::set_qos"
void
RDIProxySupplier::set_qos(const CosN::QoSProperties& r_qos)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
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
  _qos_changed(held);
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::validate_qos"
void
RDIProxySupplier::validate_qos(const CosN::QoSProperties& r_qos,
			       CosN_NamedPropertyRangeSeq_outarg rseq)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
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


// helper routine called by each add_event impl
// assumes oplock has been acquired; does not release it
// return TRUE if event added, FALSE if not added
//    (e.g., because state is not RDI_Connected)
#undef WHATFN
#define WHATFN "RDIProxySupplier::_add_event"
CORBA::Boolean
RDIProxySupplier::_add_event(RDI_StructuredEvent* entry)
{
  if ( (_pxstate == RDI_Connected) && entry ) {
    // Check if the 'MaxEventsPerConsumer' threshold has been reached.
    // In this case, we discard events based on 'DiscardPolicy'
    CORBA::ULong msize = (CORBA::ULong)_qosprop->maxEventsPerConsumer();
    while ( msize && (_ntfqueue.length() >= msize) ) {
      RDI_StructuredEvent* evnt = _ntfqueue.remove_sec_head(); // uses discard policy
      if (RDIRptTst(RDIRptDrops)) {
	RDIRptLogger(l, RDIRptDrops_nm);
	const char * dname = evnt->get_domain_name();
	if (dname == (const char*)0) dname = "\"\"";
	const char * tname = evnt->get_type_name();
	if (tname == (const char*)0) tname = "\"\"";
	l.str << "Dropped event with type " << dname << "::" << tname
	      << "\n\tfrom proxy queue for consumer ID " << _pserial
	      << "\n\tqueue len " << _ntfqueue.length() << " >= MaxEventsPerConsumer "
	      << msize << "\n";
      }
      RDI_SEVENT_DECR_REF_COUNTER(evnt, WHATFN);
    }
    RDI_SEVENT_INCR_REF_COUNTER(entry, WHATFN);
    _ntfqueue.insert(entry);
    return 1;
  }
  return 0;
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::add_filter"
CosNF::FilterID
RDIProxySupplier::add_filter(CosNF::Filter_ptr filter)
{
  CosNF::FilterID  res;
  RDI_LocksHeld    held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(outer_proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif

  // Above bump lock is obtained to make sure this sproxy is not disposed, to do
  // any initial checking, and to get the inuse count bumped so that it is safe
  // to access the _channel pointer after we do a release (it will remain
  // valid).  Now we release it and then obtain locks in this order:
  //     channel -> typemap -> sproxy

  { // introduce scope to temporarily release bump lock
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
    { // introduce channel lock scope
      RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(chan_lock, held.channel, _channel, WHATFN);
      if (!held.channel || _channel->shutting_down()) { RDI_THROW_INV_OBJREF; }
      { // introduce typemap lock scope
	RDI_CHAN_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(typemap_w_lock, held.typemap, _channel, WHATFN);
	if (!held.typemap) { RDI_THROW_INV_OBJREF; }
	{ // introduce inner sproxy lock scope
	  RDI_OPLOCK_SCOPE_LOCK_TRACK(inner_proxy_lock, held.sproxy, WHATFN);
	  if (!held.sproxy) { RDI_THROW_INV_OBJREF; }
	  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }

	  // FINALLY we can do some real work.
	  // If we have registered interest in specific event types, using
	  // 'subscription_change()', we have to cancel it at this point..
	  if ( _rqstypes.length() != 0 ) {
	    CosN::EventTypeSeq added; added.length(0);
	    (void) _channel->update_mapping(held, added, _rqstypes, this, 0);
	    _rqstypes.length(0);
	  }
	  res = _fa_helper.add_filter_i(held, filter, (RDINotifySubscribe_ptr) this, 1); // 1 => DO want propagate_schange callbacks

	} // end inner sproxy lock scope
      } // end typemap lock scope
    } // end channel lock scope
  } // end temporary release scope
  return res;
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::remove_filter"
void
RDIProxySupplier::remove_filter(CosNF::FilterID fltrID)
{
  RDI_LocksHeld    held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(outer_proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif

  // Above bump lock is obtained to make sure this sproxy is not disposed, to do
  // any initial checking, and to get the inuse count bumped so that it is safe
  // to access the _channel pointer after we do a release (it will remain
  // valid).  Now we release it and then obtain locks in this order:
  //     channel -> typemap -> sproxy

  { // introduce scope to temporarily release bump lock
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
    { // introduce channel lock scope
      RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(chan_lock, held.channel, _channel, WHATFN);
      if (!held.channel || _channel->shutting_down()) { RDI_THROW_INV_OBJREF; }
      { // introduce typemap lock scope
	RDI_CHAN_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(typemap_w_lock, held.typemap, _channel, WHATFN);
	if (!held.typemap) { RDI_THROW_INV_OBJREF; }
	{ // introduce inner sproxy lock scope
	  RDI_OPLOCK_SCOPE_LOCK_TRACK(inner_proxy_lock, held.sproxy, WHATFN);
	  if (!held.sproxy) { RDI_THROW_INV_OBJREF; }
	  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }

	  // FINALLY we can do some real work.
	  _fa_helper.remove_filter(held, fltrID, (RDINotifySubscribe_ptr) this);  // this => DO want final propagate_schange callback

	} // end inner sproxy lock scope
      } // end typemap lock scope
    } // end channel lock scope
  } // end temporary release scope
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::remove_all_filters"
void
RDIProxySupplier::remove_all_filters()
{
  RDI_LocksHeld    held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(outer_proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif

  // Above bump lock is obtained to make sure this sproxy is not disposed, to do
  // any initial checking, and to get the inuse count bumped so that it is safe
  // to access the _channel pointer after we do a release (it will remain
  // valid).  Now we release it and then obtain locks in this order:
  //     channel -> typemap -> sproxy

  { // introduce scope to temporarily release bump lock
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
    { // introduce channel lock scope
      RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(chan_lock, held.channel, _channel, WHATFN);
      if (!held.channel || _channel->shutting_down()) { RDI_THROW_INV_OBJREF; }
      { // introduce typemap lock scope
	RDI_CHAN_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(typemap_w_lock, held.typemap, _channel, WHATFN);
	if (!held.typemap) { RDI_THROW_INV_OBJREF; }
	{ // introduce inner sproxy lock scope
	  RDI_OPLOCK_SCOPE_LOCK_TRACK(inner_proxy_lock, held.sproxy, WHATFN);
	  if (!held.sproxy) { RDI_THROW_INV_OBJREF; }
	  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }

	  // FINALLY we can do some real work.
	  _fa_helper.remove_all_filters(held, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callbacks

	} // end inner sproxy lock scope
      } // end typemap lock scope
    } // end channel lock scope
  } // end temporary release scope
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::get_filter"
CosNF::Filter_ptr
RDIProxySupplier::get_filter(CosNF::FilterID fltrID)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNF::Filter_ptr res = _fa_helper.get_filter(fltrID);
  return CosNF::Filter::_duplicate(res);
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::get_all_filters"
CosNF::FilterIDSeq*
RDIProxySupplier::get_all_filters()
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNF::FilterIDSeq* res = _fa_helper.get_all_filters();
  return res;
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::subscription_change"
void
RDIProxySupplier::subscription_change(const CosN::EventTypeSeq& added,
				      const CosN::EventTypeSeq& deled)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(outer_proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; }
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif

  // The consumer can invoke this operation as long as it does not
  // use filter objects -- CORBA Notification Specification 2.6.4.
  if ( _fa_helper.has_filters() ) {
    throw CORBA::BAD_OPERATION(0, CORBA::COMPLETED_NO);
  }

  CosN::EventTypeSeq added_copy = added;
  CosN::EventTypeSeq deled_copy = deled;
  CosN::EventTypeSeq nadded;
  CosN::EventTypeSeq ndeled;
  nadded.length(0);
  ndeled.length(0);
  CORBA::Boolean i_am_consumer = 1;
  CORBA::ULong   ix = 0;
  if ( ! RDI_EventType::valid_sequence(added_copy, ix, i_am_consumer) ) {
    throw CosNC::InvalidEventType(added[ix]);
  }
  if ( ! RDI_EventType::valid_sequence(deled_copy, ix, i_am_consumer) ) {
    throw CosNC::InvalidEventType(deled[ix]);
  }

  // Above bump lock is obtained to make sure this sproxy is not disposed, to do
  // any initial checking, and to get the inuse count bumped so that it is safe
  // to access the _channel pointer after we do a release (it will remain
  // valid).  Now we release it and then obtain locks in this order:
  //     channel -> typemap -> sproxy

  { // introduce scope to temporarily release bump lock
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
    { // introduce channel lock scope
      RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(chan_lock, held.channel, _channel, WHATFN);
      if (!held.channel || _channel->shutting_down()) { RDI_THROW_INV_OBJREF; }
      { // introduce typemap lock scope
	RDI_CHAN_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(typemap_w_lock, held.typemap, _channel, WHATFN);
	if (!held.typemap) { RDI_THROW_INV_OBJREF; }
	{ // introduce inner sproxy lock scope
	  RDI_OPLOCK_SCOPE_LOCK_TRACK(inner_proxy_lock, held.sproxy, WHATFN);
	  if (!held.sproxy) { RDI_THROW_INV_OBJREF; }
	  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; }

	  // FINALLY we can do some real work.
	  // If this is not the first time this operation gets invoked, we
	  // need to compute the actual added and deleted event types that
	  // will be used to update the TypeMap
	  if ( _rqstypes.length() ) {
	    RDI_EventType::compute_diff(_rqstypes, added_copy, deled_copy, nadded, ndeled);
	    if ( nadded.length() || ndeled.length() ) {
	      (void) _channel->update_mapping(held, nadded, ndeled, this, 0);
	      RDI_EventType::update(_rqstypes, added_copy, deled_copy);
	    }
	  } else {
	    ndeled.length(0);
	    _rqstypes = added_copy; 
	    RDI_EventType::update(_rqstypes, added_copy, deled_copy);
	    (void) _channel->update_mapping(held, _rqstypes, ndeled, this, 0);
	  }

	} // end inner sproxy lock scope
      } // end typemap lock scope
    } // end channel lock scope
  } // end temporary release scope
}

AttN::IactSeq*
RDIProxySupplier::children(CORBA::Boolean only_cleanup_candidates)
{
  AttN::IactSeq* ren = new AttN::IactSeq;
  if ( ren == (AttN::IactSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  ren->length(0);
  return ren;
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::safe_cleanup"
CORBA::Boolean
RDIProxySupplier::safe_cleanup()
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; } 
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  if (_pxstate == RDI_Connected) { return 0; } // do not destroy connected proxy
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
  return 1;
}

// ------------------ ** Local-only routines **  --------------------------

#ifndef NO_OBJ_GC
#undef WHATFN
#define WHATFN "RDIProxySupplier::obj_gc"
CORBA::Boolean
RDIProxySupplier::obj_gc(RDI_TimeT curtime, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.cproxy, WHATFN);
  if (!held.cproxy) { return 0; }
  if (_pxstate == RDI_Disconnected) { return 0; }
  if ( (deadConProxy && _pxstate == RDI_Connected && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadConProxy)) ||
       (deadOtherProxy && _pxstate != RDI_Connected && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadOtherProxy)) ) {
#ifndef NDEBUG
    if (_pxstate == RDI_Connected) {
      RDIDbgSPxyLog("GC destroys connected " << RDI_PRX_TYPE(_prxtype) << " proxy " << _pserial
		    << " curtime = " << curtime.fmt_local()
		    << " last_use = " << _last_use.fmt_local()
		    << "(diff = " << RDI_TIMET_DIFF_IN_SECS(_last_use, curtime)
		    << ") DeadConProxyInterval = " << deadConProxy << '\n');
    } else {
      RDIDbgSPxyLog("GC destroys non-connected " << RDI_PRX_TYPE(_prxtype) << " proxy " << _pserial
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
#define WHATFN "RDIProxySupplier::_enable_updates"
void
RDIProxySupplier::_enable_updates()
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return);
  RDI_ChangePool* cocp = _channel->ochange_pool();
  if ( (_pxstate != RDI_Connected) || CORBA::is_nil(_oc_subscriber) || ! cocp ) {
    // _oc_off status is meaningless without connected _nc_consumer and a valid _ochange_pool
    _oc_off = 0;
    return;
  }
  if (!_oc_off) {
    // make offer_change msgs reflect only subsequent updates
    // by removing 'this' before re-inserting it
    cocp->remove_proxy(this);
  }
  _oc_off = 0;
  cocp->insert_proxy(this);
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::_disable_updates"
void
RDIProxySupplier::_disable_updates()
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return);
  RDI_ChangePool* cocp = _channel->ochange_pool();
  if ( (_pxstate != RDI_Connected) || CORBA::is_nil(_oc_subscriber) || ! cocp ) {
    // _oc_off status is meaningless without connected _nc_consumer and a valid _ochange_pool
    _oc_off = 1;
    return;
  }
  if (!_oc_off) {
    _oc_off = 1;
    cocp->remove_proxy(this);
  }
}

// RDINotifSupplier methods:
void
RDIProxySupplier::propagate_schange(RDI_LocksHeld&             held,
				    const CosN::EventTypeSeq&  added,
				    const CosN::EventTypeSeq&  deled,
				    Filter_i*                  filter)
{
  (void) _channel->update_mapping(held, added, deled, this, filter);
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::filter_destroy_i"
void
RDIProxySupplier::filter_destroy_i(Filter_i* filter)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return);
  _fa_helper.rem_filter_i(filter);
}

// --------------------- ** Protected methods ** ------------------------------------- 

#undef WHATFN
#define WHATFN "RDIProxySupplier::disconnect_client_and_dispose"
void
RDIProxySupplier::disconnect_client_and_dispose(RDI_LocksHeld&  held,
						CORBA::Boolean  remove_from_admin)
{
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { return; }
  _disconnect_client_and_dispose(held, remove_from_admin, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "RDIProxySupplier::send_offer_change"
// If proxy has a connected CosNotify-style consumer, _oc_subscriber will be non-null.
// Attempt to send it an offer_change msg.
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
RDIProxySupplier::send_offer_change(const CosN::EventTypeSeq& added,
				    const CosN::EventTypeSeq& deled)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { return 1; }

  CORBA::Boolean res = 0; // OK
  if (_oc_off) {
    RDIDbgSPxyLog("send_offer_change called on proxy with _oc_off true\n");
    res = 1;
  } else if ( (_pxstate == RDI_Connected) && ! CORBA::is_nil(_oc_subscriber) ) {
    // do not hold OPLOCK across offer_change call
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
      try {
	_oc_subscriber->offer_change(added, deled);
#ifndef NO_OBJ_GC
	_last_use.set_curtime();
#endif
      } catch(CORBA::NO_IMPLEMENT& ex) {
	res = 1; // do not try any more of these
      } catch (...) {
	RDIDbgSPxyLog("RDIProxySupplier -- caught exception other than CORBA::NO_IMPLEMENT " <<
		      "when sending an offer_change; could retry if this is a soft failure " <<
		      "but for now, giving up immediately\n");
	res = 1;
      }
    } // end unlock scope
    if (!held.sproxy) {
      RDI_Fatal("RDIProxySupplier::send_offer_change [**unexpected REACQUIRE failure**]\n");
    }
  } else {
    RDIDbgSPxyLog("RDIProxySupplier -- oc_subscriber not available => did not send offer_change\n");
    res = 1;
  }
  return res;
}

// Assumes lock is held
#undef WHATFN
#define WHATFN "RDIProxySupplier::_clear_ntfqueue"
void
RDIProxySupplier::_clear_ntfqueue()
{
  // Remove all events from _ntfqueue
  RDI_StructuredEvent* event = 0;
  while ( (event = _ntfqueue.remove_pri_head()) ) {
    RDI_SEVENT_DECR_REF_COUNTER(event, WHATFN);
  }
}

// ------------------------------------------------------------- //
// ProxyPullSupplier_i                                           //
// ------------------------------------------------------------- //

ProxyPullSupplier_i::ProxyPullSupplier_i(ConsumerAdmin_i* admin, 
					 EventChannel_i*  chann,
					 const CosNA::ProxyID&  prxid) : 
  RDIProxySupplier("ProxyPullSupplier", "ProxyPullSupplier_fa_helper",
		   admin, chann, RDI_S_AnyPRX, CosNA::PULL_ANY, prxid)
{
  _consumer = CosEvC::PullConsumer::_nil();
  _nc_consumer = CosNC::PullConsumer::_nil();
  _ntfqueue.qos_changed(_qosprop);
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

ProxyPullSupplier_i::~ProxyPullSupplier_i()
{
  RDI_OPLOCK_DESTROY_CHECK("ProxyPullSupplier_i");
}

#undef WHATFN
#define WHATFN "ProxyPullSupplier_i::pull"
CORBA::Any*
ProxyPullSupplier_i::pull( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_BUMP_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  CORBA::ULong qsize = 0;
  while ( (_pxstate == RDI_Connected) && ((qsize = _ntfqueue.length()) == 0) ) {
    RDI_OPLOCK_WAIT(WHATFN);
  }
  if ( _pxstate != RDI_Connected ) {
    throw CosEvC::Disconnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif

  RDI_StructuredEvent* event = _ntfqueue.remove_pri_head();
  const CosN::StructuredEvent& cosev = event->get_cos_event();
  _nevents += 1;

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

  _channel->incr_num_notifications(--qsize);
  return res;
}

#undef WHATFN
#define WHATFN "ProxyPullSupplier_i::try_pull"
CORBA::Any* 
ProxyPullSupplier_i::try_pull(CORBA::Boolean& has_event WRAPPED_IMPLARG)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_Connected ) {
    throw CosEvC::Disconnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CORBA::ULong qsize = _ntfqueue.length();
  if ( qsize == 0 ) {
    has_event = 0;
    return new CORBA::Any();
  }
  RDI_StructuredEvent* event = _ntfqueue.remove_pri_head();
  const CosN::StructuredEvent&      cosev = event->get_cos_event();
  _nevents += 1;
  has_event = 1;

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

  _channel->incr_num_notifications(--qsize);
  return res;
}
  
#undef WHATFN
#define WHATFN "ProxyPullSupplier_i::connect_any_pull_consumer"
void
ProxyPullSupplier_i::connect_any_pull_consumer(CosEvC::PullConsumer_ptr consumer WRAPPED_IMPLARG)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_NotConnected ) {
    throw CosEvCA::AlreadyConnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  // According to the CORBA Notification Service specification, pull
  // consumers can provide a _nil reference [2.6.1].
  if ( ! CORBA::is_nil(consumer) ) {
    _consumer = CosEvC::PullConsumer::_duplicate(consumer);
    // If consumer is a CosNC::PullConsumer, set _nc_consumer
    _nc_consumer = CosNC::PullConsumer::_narrow(consumer); // implicit duplicate
    if (! CORBA::is_nil(_nc_consumer)) {
      _oc_subscriber = CosNC::NotifyPublish::_narrow(_nc_consumer); // implicit duplicate
      if (CORBA::is_nil(_oc_subscriber)) {
	RDIDbgSPxyLog("** UNEXPECTED: CosNC::PullConsumer could not be narrowed to CosNC::NotifyPublish\n");
      }
      // if enabled, register this proxy for offer_change msgs
      RDI_ChangePool* cocp = _channel->ochange_pool();
      if ( ! _oc_off && cocp ) {
	cocp->insert_proxy(this);
      }
    }
  }
  _pxstate = RDI_Connected;
  _active  = 1;
}

#undef WHATFN
#define WHATFN "ProxyPullSupplier_i::add_event"
void
ProxyPullSupplier_i::add_event(RDI_StructuredEvent* entry)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_add_event(entry)) {
    RDI_OPLOCK_SIGNAL;
  }
}

#undef WHATFN
#define WHATFN "ProxyPullSupplier_i::disconnect_pull_supplier"
void
ProxyPullSupplier_i::disconnect_pull_supplier( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; } 
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "ProxyPullSupplier_i::_disconnect_client_and_dispose"
// *** lock must be held, held.sproxy set 
// Effect: initiate impl dispose
void
ProxyPullSupplier_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
						    CORBA::Boolean            remove_from_admin,
						    WRAPPED_DISPOSEINFO_PTR&  dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgSPxyLog("** Internal error -- ProxyPullSupplier_i::_disconnect_client_and_dispose called twice on same proxy!\n");
    return;
  }
  // unregister this proxy -- no more offer_change msgs
  RDI_ChangePool* cocp = _channel->ochange_pool();
  if ( ! _oc_off && (_pxstate == RDI_Connected) && ! CORBA::is_nil(_nc_consumer) && cocp ) {
    cocp->remove_proxy(this);
  }
  _pxstate = RDI_Disconnected; // acts as guard -- only one thread does the following
  // Wait for inuse to drop to one.  This ensures that any outstanding blocked pull call
  // or any other call that bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
      _myadmin->remove_proxy(held, this);
    } // end unlock scope
    if (!held.sproxy) {
      RDI_Fatal("ProxyPullSupplier_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }

  // To do a typemap update (via remove_all_filters or update_mapping),
  // we need to release our lock and then obtain locks in this order:
  //   channel -> typemap -> sproxy

  if (held.channel && held.typemap) {
    // lucky us, we do not have to play with locks
    if ( _rqstypes.length() == 0 ) {
      _fa_helper.remove_all_filters(held, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callbacks
    } else {
      CosN::EventTypeSeq added; added.length(0);
      (void) _channel->update_mapping(held, added, _rqstypes, this, 0);
    }
  } else {
    // scope to temporarily release bump lock
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
    { // introduce channel lock scope
      RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(chan_lock, held.channel, _channel, WHATFN);
      if (!held.channel || _channel->shutting_down()) { goto skip_update; }
      { // introduce typemap lock scope
	RDI_CHAN_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(typemap_w_lock, held.typemap, _channel, WHATFN);
	if (!held.typemap) { goto skip_update; }
	{ // introduce inner sproxy lock scope
	  RDI_OPLOCK_SCOPE_LOCK_TRACK(inner_proxy_lock, held.sproxy, WHATFN);
	  if (!held.sproxy) { goto skip_update; }

	  // FINALLY we can do some real work.
	  if ( _rqstypes.length() == 0 ) {
	    _fa_helper.remove_all_filters(held, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callbacks
	  } else {
	    CosN::EventTypeSeq added; added.length(0);
	    (void) _channel->update_mapping(held, added, _rqstypes, this, 0);
	  }

	} // end inner sproxy lock scope
      } // end typemap lock scope
    } // end channel lock scope
  } // end temporary release scope

 skip_update:
  _consumer = CosEvC::PullConsumer::_nil();
  _nc_consumer = CosNC::PullConsumer::_nil();
  _pfilter = CosNF::MappingFilter::_nil();
  _lfilter = CosNF::MappingFilter::_nil();
  if ( _qosprop ) {
    delete _qosprop; _qosprop = 0;
  }
  _clear_ntfqueue(); // Remove all events
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

// ------------------------------------------------------------- //
// StructuredProxyPullSupplier_i                                 //
// ------------------------------------------------------------- //

StructuredProxyPullSupplier_i::StructuredProxyPullSupplier_i(
							     ConsumerAdmin_i* admin, 
							     EventChannel_i*  chann,
							     const CosNA::ProxyID&  prxid) : 
  RDIProxySupplier("StructuredProxyPullSupplier", "StructuredProxyPullSupplier_fa_helper",
		   admin, chann, RDI_S_StrPRX, CosNA::PULL_STRUCTURED, prxid)
{
  _consumer = CosNC::StructuredPullConsumer::_nil();
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

StructuredProxyPullSupplier_i::~StructuredProxyPullSupplier_i()
{
  RDI_OPLOCK_DESTROY_CHECK("StructuredProxyPullSupplier_i");
}

#undef WHATFN
#define WHATFN "StructuredProxyPullSupplier_i::pull_structured_event"
CosN::StructuredEvent* 
StructuredProxyPullSupplier_i::pull_structured_event( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_BUMP_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  CosN::StructuredEvent* cosse = 0;
  RDI_StructuredEvent*  entry = 0;

  CORBA::ULong qsize = 0;
  while ( (_pxstate == RDI_Connected) && ((qsize = _ntfqueue.length()) == 0) ) {
    RDI_OPLOCK_WAIT(WHATFN);
  }
  if ( _pxstate != RDI_Connected ) {
    throw CosEvC::Disconnected();
  } 
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif

  entry = _ntfqueue.remove_pri_head();
  _nevents += 1;

  // Given that we need to hold entry lock to decrement ref counter,
  // may as well hold it across cosse construction
  // (accounts for !WRAPPED_ANY_THREADSAFE case)
  { // introduce event lock scope
    RDI_SEVENT_SCOPE_LOCK(event_lock, entry, WHATFN);
    cosse = new CosN::StructuredEvent(entry->get_cos_event());
    entry->decr_ref_counter_lock_held();
  } // end event lock scope

  _channel->incr_num_notifications(--qsize);
  return cosse;
}

#undef WHATFN
#define WHATFN "StructuredProxyPullSupplier_i::try_pull_structured_event"
CosN::StructuredEvent*
StructuredProxyPullSupplier_i::try_pull_structured_event(CORBA::Boolean& has_event   WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_Connected ) {
    throw CosEvC::Disconnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CORBA::ULong qsize = _ntfqueue.length();
  if ( qsize == 0 ) {
    has_event = 0;
    return new CosN::StructuredEvent();
  }
  CosN::StructuredEvent*    cosse = 0; 
  RDI_StructuredEvent* entry = _ntfqueue.remove_pri_head();
  _nevents += 1;
  has_event = 1;

  // Given that we need to hold entry lock to decrement ref counter,
  // may as well hold it across cosse construction
  // (accounts for !WRAPPED_ANY_THREADSAFE case)
  { // introduce event lock scope
    RDI_SEVENT_SCOPE_LOCK(event_lock, entry, WHATFN);
    cosse = new CosN::StructuredEvent(entry->get_cos_event());
    entry->decr_ref_counter_lock_held();
  } // end event lock scope

  _channel->incr_num_notifications(--qsize);
  return cosse;
}	

#undef WHATFN
#define WHATFN "StructuredProxyPullSupplier_i::connect_structured_pull_consumer"
void
StructuredProxyPullSupplier_i::connect_structured_pull_consumer(CosNC::StructuredPullConsumer_ptr consumer
                                                                 WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_NotConnected ) {
    throw CosEvCA::AlreadyConnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  // According to the CORBA Notification Service specification, pull
  // consumers can provide a _nil reference [2.6.1] ................
  if ( ! CORBA::is_nil(consumer) ) {
    _consumer = CosNC::StructuredPullConsumer::_duplicate(consumer);
    _oc_subscriber = CosNC::NotifyPublish::_narrow(_consumer); // implicit duplicate
    if (CORBA::is_nil(_oc_subscriber)) {
      RDIDbgSPxyLog("** UNEXPECTED: CosNC::StructuredPullConsumer could not be narrowed to CosNC::NotifyPublish\n");
    }
    // if enabled, register this proxy for offer_change msgs
    RDI_ChangePool* cocp = _channel->ochange_pool();
    if ( ! _oc_off && cocp ) {
      cocp->insert_proxy(this);
    }
  }
  _pxstate  = RDI_Connected;
  _active  = 1;
}

#undef WHATFN
#define WHATFN "StructuredProxyPullSupplier_i::add_event"
void
StructuredProxyPullSupplier_i::add_event(RDI_StructuredEvent* entry)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_add_event(entry)) {
    RDI_OPLOCK_SIGNAL;
  }
}

#undef WHATFN
#define WHATFN "StructuredProxyPullSupplier_i::disconnect_structured_pull_supplier"
void
StructuredProxyPullSupplier_i::disconnect_structured_pull_supplier( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; } 
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "StructuredProxyPullSupplier_i::_disconnect_client_and_dispose"
// *** lock must be held, held.sproxy set
// Effect: initiate impl dispose
void
StructuredProxyPullSupplier_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
							      CORBA::Boolean            remove_from_admin,
							      WRAPPED_DISPOSEINFO_PTR&  dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgSPxyLog("** Internal error -- StructuredProxyPullSupplier_i::_disconnect_client_and_dispose " <<
		  "called twice on same proxy!\n");
    return;
  }
  // unregister this proxy -- no more offer_change msgs
  RDI_ChangePool* cocp = _channel->ochange_pool();
  if ( ! _oc_off && (_pxstate == RDI_Connected) && cocp ) {
    cocp->remove_proxy(this);
  }
  _pxstate = RDI_Disconnected; // acts as guard -- only one thread does the following
  // Wait for inuse to drop to one.  This ensures that any outstanding blocked pull call
  // or any other call that bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
      _myadmin->remove_proxy(held, this);
    } // end unlock scope
    if (!held.sproxy) {
      RDI_Fatal("StructuredProxyPullSupplier_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }

  // To do a typemap update (via remove_all_filters or update_mapping),
  // we need to release our lock and then obtain locks in this order:
  //   channel -> typemap -> sproxy

  if (held.channel && held.typemap) {
    // lucky us, we do not have to play with locks
    if ( _rqstypes.length() == 0 ) { 
      _fa_helper.remove_all_filters(held, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callbacks
    } else {
      CosN::EventTypeSeq added; added.length(0);
      (void) _channel->update_mapping(held, added, _rqstypes, this, 0);
    }
  } else { // introduce scope to temporarily release bump lock
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
    { // introduce channel lock scope
      RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(chan_lock, held.channel, _channel, WHATFN);
      if (!held.channel || _channel->shutting_down()) { goto skip_update; }
      { // introduce typemap lock scope
	RDI_CHAN_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(typemap_w_lock, held.typemap, _channel, WHATFN);
	if (!held.typemap) { goto skip_update; }
	{ // introduce inner sproxy lock scope
	  RDI_OPLOCK_SCOPE_LOCK_TRACK(inner_proxy_lock, held.sproxy, WHATFN);
	  if (!held.sproxy) { goto skip_update; }

	  // FINALLY we can do some real work.
	  if ( _rqstypes.length() == 0 ) { 
	    _fa_helper.remove_all_filters(held, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callbacks
	  } else {
	    CosN::EventTypeSeq added; added.length(0);
	    (void) _channel->update_mapping(held, added, _rqstypes, this, 0);
	  }

	} // end inner sproxy lock scope
      } // end typemap lock scope
    } // end channel lock scope
  } // end temporary release scope

 skip_update:
  _consumer = CosNC::StructuredPullConsumer::_nil();
  _pfilter = CosNF::MappingFilter::_nil();
  _lfilter = CosNF::MappingFilter::_nil();
  if ( _qosprop ) {
    delete _qosprop; _qosprop = 0;
  }
  _clear_ntfqueue(); // Remove all events
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

// ------------------------------------------------------------- //
// SequenceProxyPullSupplier_i                                   //
// ------------------------------------------------------------- //

SequenceProxyPullSupplier_i::SequenceProxyPullSupplier_i(
							 ConsumerAdmin_i* admin, 
							 EventChannel_i*  chann,
							 const CosNA::ProxyID&  prxid) : 
  RDIProxySupplier("SequenceProxyPullSupplier", "SequenceProxyPullSupplier_fa_helper",
		   admin, chann, RDI_S_SeqPRX, CosNA::PULL_SEQUENCE, prxid)
{
  unsigned long pacing_interval_s, pacing_interval_n;
  _consumer = CosNC::SequencePullConsumer::_nil();

  // If pacingInterval is non-zero, set initial timeout.
  // Note that the interval typically expires before any events are
  // placed in the notification queue of this proxy, which means
  // the first batch normally contains just a single event.
  // The only way to fix this would be to initialize the pacing
  // interval on the very first event insertion.

  _qosprop->pacingInterval_s_n(pacing_interval_s, pacing_interval_n);

  if (pacing_interval_s || pacing_interval_n) { // update timeout
    TW_GET_TIME(&_timeout_s, &_timeout_n, pacing_interval_s, pacing_interval_n);
  } else {
    _timeout_s = 0; _timeout_n = 0;
  }
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

SequenceProxyPullSupplier_i::~SequenceProxyPullSupplier_i()
{
  RDI_OPLOCK_DESTROY_CHECK("SequenceProxyPullSupplier_i");
}

#undef WHATFN
#define WHATFN "SequenceProxyPullSupplier_i::pull_structured_events"
CosN::EventBatch* 
SequenceProxyPullSupplier_i::pull_structured_events(CORBA::Long maxnum   WRAPPED_IMPLARG)
{
  RDI_OPLOCK_BUMP_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_Connected ) {
    throw CosEvC::Disconnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  if (maxnum < 0) {
    maxnum = 0; // we take 0 to mean use the channel's maximumBatchSize
  }
  CosN::EventBatch* notif = 0;
  RDI_StructuredEvent* event = 0;
  // pacing interval : both 0 means always wait for full batch size
  unsigned long pacing_interval_s, pacing_interval_n;
  unsigned long time_s, time_n;
  CORBA::Long bsize, qsize, actsize;

  while ( 1 ) {
    // must recompute these here because they can change across a wait/timedwait
    qsize = _ntfqueue.length();
    _qosprop->pacingInterval_s_n(pacing_interval_s, pacing_interval_n);
    if (pacing_interval_s || pacing_interval_n) {
      // if timeout not set, recompute timeout
      if ((_timeout_s == 0) && (_timeout_n == 0)) {
	TW_GET_TIME(&_timeout_s, &_timeout_n, pacing_interval_s, pacing_interval_n);
      }
    } else {
      _timeout_s = 0; _timeout_n = 0;
    }
    bsize = _qosprop->maximumBatchSize();
    if (maxnum && (bsize > maxnum))
      bsize = maxnum;
    // breaking from inner loop requires either a sufficient batch size or a
    // pacing interval timeout (and state must be RDI_Connected and _active)
    if ((_pxstate!=RDI_NotConnected) && (_pxstate!=RDI_Connected)) {
      break; // also break on exceptional state
    }
    if ((_pxstate==RDI_Connected) && _active) {
      if ( qsize >= bsize ) {
	break;
      }
      if ( qsize && (pacing_interval_s || pacing_interval_n) ) {
	TW_GET_TIME(&time_s, &time_n);
	if ( (time_s > _timeout_s) || ((time_s == _timeout_s) && (time_n > _timeout_n)) ) {
	  // pacing interval has passed so go ahead and send a smaller batch
	  break;
	}
      }
    }
    // must wait
    if ( (_pxstate==RDI_Connected) && _active && qsize &&
	 (pacing_interval_s || pacing_interval_n) ) {
      //      RDIDbgSPxyLog("sequence proxy doing timedwait(" << _timeout_s << "," << _timeout_n <<")\n");
      RDI_OPLOCK_TIMEDWAIT(_timeout_s, _timeout_n, WHATFN);
    } else {
      //      RDIDbgSPxyLog("sequence proxy doing normal wait\n");
      RDI_OPLOCK_WAIT(WHATFN);
    }
  }
  if ( _pxstate != RDI_Connected ) {
    throw CosEvC::Disconnected();
  }
  actsize = (qsize < bsize) ? qsize : bsize;
  if ( ! (notif = new CosN::EventBatch(actsize)) ) {
    RDIDbgSPxyLog("Memory allocation failed -- CosN::EventBatch\n");
    return 0;
  }
  notif->length(actsize);
  for (CORBA::Long i = 0; i < actsize; i++) {
    event = _ntfqueue.remove_pri_head();
    // Given that we need to hold event lock to decrement ref counter,
    // may as well hold it across copy
    // (accounts for !WRAPPED_ANY_THREADSAFE case)
    { // introduce event lock scope
      RDI_SEVENT_SCOPE_LOCK(event_lock, event, WHATFN);
      (*notif)[i] = event->get_cos_event();
      event->decr_ref_counter_lock_held();
    } // end event lock scope
    _nevents += 1;
    _channel->incr_num_notifications(--qsize);
  }
  // returning a valid batch => next call to pull_structured_events should use a new timeout:
  if (pacing_interval_s || pacing_interval_n) {
    TW_GET_TIME(&_timeout_s, &_timeout_n, pacing_interval_s, pacing_interval_n);
  } else {
    _timeout_s = 0; _timeout_n = 0;
  }
  return notif;
}
  
#undef WHATFN
#define WHATFN "SequenceProxyPullSupplier_i::try_pull_structured_events"
CosN::EventBatch* 
SequenceProxyPullSupplier_i::try_pull_structured_events(CORBA::Long maxnum,
							CORBA::Boolean& has_event WRAPPED_IMPLARG)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  CosN::EventBatch* notif = 0;
  RDI_StructuredEvent* event = 0;

  if ( _pxstate != RDI_Connected ) {
    throw CosEvC::Disconnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif

  if (maxnum < 0)
    maxnum = 0; // we take 0 to mean use the channel's maximumBatchSize
  if ( (maxnum == 0) || (maxnum > _qosprop->maximumBatchSize()) )
    maxnum = _qosprop->maximumBatchSize();
  CORBA::Long qsize = _ntfqueue.length();

  if ( ! (notif = new CosN::EventBatch) ) {
    RDIDbgSPxyLog("Memory allocation failed -- CosN::EventBatch\n");
    return 0;
  }

  if ( qsize == 0 ) {
    has_event = 0;
    notif->length(0);
  } else {
    CORBA::Long bsize = ((maxnum < qsize) ? maxnum : qsize);
    notif->length(bsize);
    for (CORBA::Long i = 0; i < bsize; i++) {
      event = _ntfqueue.remove_pri_head();
      // Given that we need to hold event lock to decrement ref counter,
      // may as well hold it across copy
      // (accounts for !WRAPPED_ANY_THREADSAFE case)
      { // introduce event lock scope
	RDI_SEVENT_SCOPE_LOCK(event_lock, event, WHATFN);
	(*notif)[i] = event->get_cos_event();
	event->decr_ref_counter_lock_held();
      } // end event lock scope
      _nevents += 1;
      _channel->incr_num_notifications(--qsize);
    }
    has_event = 1;
  }
  return notif;
}

#undef WHATFN
#define WHATFN "SequenceProxyPullSupplier_i::connect_sequence_pull_consumer"
void
SequenceProxyPullSupplier_i::connect_sequence_pull_consumer(CosNC::SequencePullConsumer_ptr consumer
                                                             WRAPPED_IMPLARG)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _pxstate != RDI_NotConnected ) {
    throw CosEvCA::AlreadyConnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  // According to the CORBA Notification Service specification, pull
  // consumers can provide a _nil reference [2.6.1] ................
  if ( ! CORBA::is_nil(consumer) ) {
    _consumer = CosNC::SequencePullConsumer::_duplicate(consumer);
    _oc_subscriber = CosNC::NotifyPublish::_narrow(_consumer); // implicit duplicate
    if (CORBA::is_nil(_oc_subscriber)) {
      RDIDbgSPxyLog("** UNEXPECTED: CosNC::SequencePullConsumer could not be narrowed to CosNC::NotifyPublish\n");
    }
    // if enabled, register this proxy for offer_change msgs
    RDI_ChangePool* cocp = _channel->ochange_pool();
    if ( ! _oc_off && cocp ) {
      cocp->insert_proxy(this);
    }
  }
  _pxstate  = RDI_Connected;
  _active  = 1;
}

#undef WHATFN
#define WHATFN "SequenceProxyPullSupplier_i::add_event"
void
SequenceProxyPullSupplier_i::add_event(RDI_StructuredEvent* entry)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_add_event(entry)) {
    RDI_OPLOCK_SIGNAL;
  }
}

#undef WHATFN
#define WHATFN "SequenceProxyPullSupplier_i::disconnect_sequence_pull_supplier"
void
SequenceProxyPullSupplier_i::disconnect_sequence_pull_supplier( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; } 
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "SequenceProxyPullSupplier_i::_disconnect_client_and_dispose"
// *** lock must be held, held.sproxy set
// Effect: initiate impl dispose
void
SequenceProxyPullSupplier_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
							    CORBA::Boolean            remove_from_admin,
							    WRAPPED_DISPOSEINFO_PTR&  dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgSPxyLog("** Internal error -- SequenceProxyPullSupplier_i::_disconnect_client_and_dispose called twice on same proxy!\n");
    return;
  }
  // unregister this proxy -- no more offer_change msgs
  RDI_ChangePool* cocp = _channel->ochange_pool();
  if ( ! _oc_off && (_pxstate == RDI_Connected) && cocp ) {
    cocp->remove_proxy(this);
  }
  _pxstate = RDI_Disconnected; // acts as guard -- only one thread does the following
  // Wait for inuse to drop to one.  This ensures that any outstanding blocked pull call
  // or any other call that bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
      _myadmin->remove_proxy(held, this);
    } // end unlock scope
    if (!held.sproxy) {
      RDI_Fatal("SequenceProxyPullSupplier_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }

  // To do a typemap update (via remove_all_filters or update_mapping),
  // we need to release our lock and then obtain locks in this order:
  //   channel -> typemap -> sproxy

  if (held.channel && held.typemap) {
    // lucky us, we do not have to play with locks
    if ( _rqstypes.length() == 0 ) { 
      _fa_helper.remove_all_filters(held, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callbacks
    } else {
      CosN::EventTypeSeq added; added.length(0);
      (void) _channel->update_mapping(held, added, _rqstypes, this, 0);
    }
  } else { // introduce scope to temporarily release bump lock
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
    { // introduce channel lock scope
      RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(chan_lock, held.channel, _channel, WHATFN);
      if (!held.channel || _channel->shutting_down()) { goto skip_update; }
      { // introduce typemap lock scope
	RDI_CHAN_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(typemap_w_lock, held.typemap, _channel, WHATFN);
	if (!held.typemap) { goto skip_update; }
	{ // introduce inner sproxy lock scope
	  RDI_OPLOCK_SCOPE_LOCK_TRACK(inner_proxy_lock, held.sproxy, WHATFN);
	  if (!held.sproxy) { goto skip_update; }

	  // FINALLY we can do some real work.
	  if ( _rqstypes.length() == 0 ) { 
	    _fa_helper.remove_all_filters(held, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callbacks
	  } else {
	    CosN::EventTypeSeq added; added.length(0);
	    (void) _channel->update_mapping(held, added, _rqstypes, this, 0);
	  }

	} // end inner sproxy lock scope
      } // end typemap lock scope
    } // end channel lock scope
  } // end temporary release scope

 skip_update:
  _consumer = CosNC::SequencePullConsumer::_nil();
  _pfilter = CosNF::MappingFilter::_nil();
  _lfilter = CosNF::MappingFilter::_nil();
  if ( _qosprop ) {
    delete _qosprop; _qosprop = 0;
  }
  _clear_ntfqueue(); // Remove all events
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

// ------------------------------------------------------------- //
// ProxyPushSupplier_i                                           //
// ------------------------------------------------------------- //

ProxyPushSupplier_i::ProxyPushSupplier_i(ConsumerAdmin_i* admin, 
					 EventChannel_i*  chann,
					 const CosNA::ProxyID&  prxid) : 
  RDIProxySupplier("ProxyPushSupplier", "ProxyPushSupplier_fa_helper", 
		   admin, chann, RDI_S_AnyPRX, CosNA::PUSH_ANY, prxid),
  _worker(0)
{
  _consumer = CosEvC::PushConsumer::_nil();

  // When the number of push threads allocated at the channel level
  // is 0, each proxy uses its own thread for pushing events 
  if ( _channel->push_threads() == 0 ) {
    // _worker = new AnyProxyWorker(this, &ProxyPushSupplier_i::_push_event);
    _worker = new AnyProxyBoundWorker(this, &ProxyPushSupplier_i::_push_event);
    RDI_AssertAllocThrowNo(_worker, "Memory allocation failed -- Thread\n");
    // If we use unbound threads, we should start the thread using
    // _worker->start();
    RDIDbgSPxyLog("Push thread for proxy " << (void*)this << " -- " << _worker->id());
  }
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}
 
ProxyPushSupplier_i::~ProxyPushSupplier_i()
{
  RDI_OPLOCK_DESTROY_CHECK("ProxyPushSupplier_i");
}

#undef WHATFN
#define WHATFN "ProxyPushSupplier_i::connect_any_push_consumer"
void
ProxyPushSupplier_i::connect_any_push_consumer(CosEvC::PushConsumer_ptr consumer WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  CosEvC::PushConsumer_var evcons;
  if ( CORBA::is_nil(consumer) ) {
    throw CORBA::BAD_PARAM(0, CORBA::COMPLETED_NO);
  }
  if ( _pxstate != RDI_NotConnected ) {
    throw CosEvCA::AlreadyConnected();
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _active   = 1;
  _pxstate  = RDI_Connected;
  _consumer = CosEvC::PushConsumer::_duplicate(consumer);
  // If consumer is a CosNC::PushConsumer, set _nc_consumer
  _nc_consumer = CosNC::PushConsumer::_narrow(consumer); // implicit duplicate
  if (! CORBA::is_nil(_nc_consumer)) {
    _oc_subscriber = CosNC::NotifyPublish::_narrow(_nc_consumer); // implicit duplicate
    if (CORBA::is_nil(_oc_subscriber)) {
      RDIDbgSPxyLog("** UNEXPECTED: CosNC::PushConsumer could not be narrowed to CosNC::NotifyPublish\n");
    }
    // if enabled, register this proxy for offer_change msgs
    RDI_ChangePool* cocp = _channel->ochange_pool();
    if ( ! _oc_off && cocp ) {
      cocp->insert_proxy(this);
    }
  }
}

#undef WHATFN
#define WHATFN "ProxyPushSupplier_i::suspend_connection"
void
ProxyPushSupplier_i::suspend_connection( WRAPPED_IMPLARG_VOID )
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
#define WHATFN "ProxyPushSupplier_i::resume_connection"
void
ProxyPushSupplier_i::resume_connection( WRAPPED_IMPLARG_VOID )
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
  RDI_NotifyConsumer* cpc = _channel->push_consumer();
  if ( cpc ) {
    cpc->signal_push_threads();
  }
}

#undef WHATFN
#define WHATFN "ProxyPushSupplier_i::has_events"
CORBA::Boolean
ProxyPushSupplier_i::has_events(unsigned long* wait_s, unsigned long* wait_t)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return 0);
  if ( !_oplockptr || (_pxstate != RDI_Connected) || !_active || !_ntfqueue.length() ) {
    return 0;
  }
  return 1;
}

#undef WHATFN
#define WHATFN "ProxyPushSupplier_i::push_event"
void
ProxyPushSupplier_i::push_event(CORBA::Boolean& invalid)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; }

  CORBA::Boolean outcall_worked;
  RDI_StructuredEvent* event = 0;
  unsigned int         qsize = 0;

  invalid = 0;
  if ( (_pxstate == RDI_Disconnected) || (_pxstate == RDI_Exception) ) {
    // tell caller no more pushes (by setting invalid to 1)
    invalid = 1;
    return;
  }
  if ( (_pxstate != RDI_Connected) || !_active || !_ntfqueue.length() ) {
    return;
  }
  event = _ntfqueue.remove_pri_head();
  qsize = _ntfqueue.length();
  _nevents += 1;

  // Do not hold OPLOCK across push
  outcall_worked = 0;
  { // introduce unlock scope
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);

    try {
#ifndef WRAPPED_ANY_THREADSAFE
      const CosN::StructuredEvent& sev = event->get_cos_event();
      if ( RDI_STR_EQ(event->get_type_name(), "%ANY") ) {
	// Need to copy remainder_of_body to produce thread-safe marshalling without
	// holding event lock.  An efficient form of copy is used here (shares buffer).
	// Need to hold event lock across copy.
	RDI_SEVENT_PROT_UNPROT(event, WHATFN,
			       CORBA::Any safe(sev.remainder_of_body.type(), (void*)(sev.remainder_of_body.value())),
			       _consumer->push(safe));
      } else {
	// Construction of any_event here avoids need to worry about thread-safe marshalling.
	// Need to hold event lock across construction.
	CORBA::Any any_event;
	{ // introduce event lock scope
	  RDI_SEVENT_SCOPE_LOCK(event_lock, event, WHATFN);
	  any_event <<= sev;
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
      RDIDbgSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Invalid object reference\n");
    } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
      RDIDbgSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Consumer object does not exist\n");
    } catch ( CORBA::COMM_FAILURE& e ) {
      RDIDbgSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Communication Failure\n");
    } catch (...) {
      RDIDbgSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Exception while pushing event to consumer\n");
    }
    RDI_SEVENT_DECR_REF_COUNTER(event, WHATFN);
  } // end unlock scope
  if (!held.sproxy) {
    RDI_Fatal("ProxyPushSupplier_i::push_event [**unexpected REACQUIRE failure**]\n");
  }

  if (_pxstate != RDI_Connected) {
    // disconnected/disposed during push -- do nothing else with my state
  } else {
    if (outcall_worked) {
      _channel->incr_num_notifications(qsize);
    } else { // exception
      // unregister this proxy -- no more subscription_change msgs
      RDI_ChangePool* cocp = _channel->ochange_pool();
      if ( ! _oc_off && ! CORBA::is_nil(_nc_consumer) && cocp ) {
	cocp->remove_proxy(this);
      }
      _pxstate = RDI_Exception;
      _clear_ntfqueue();
      invalid = 1;
    }
  }
}

#undef WHATFN
#define WHATFN "ProxyPushSupplier_i::_push_event"
void
ProxyPushSupplier_i::_push_event()
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; }

  CORBA::Boolean       outcall_worked;
  RDI_StructuredEvent* event = 0;
  unsigned int         qsize = 0;
  CORBA::Boolean       do_yield = 0;


  while ( 1 ) {
    do_yield = 1;
    while ( (_pxstate==RDI_NotConnected) ||
	    ((_pxstate==RDI_Connected)&&(!_active || !_ntfqueue.length())) ) {
      do_yield = 0;
      RDI_OPLOCK_WAIT(WHATFN);
    }
    if ( _pxstate != RDI_Connected ) {
      RDI_THREAD_EXIT(push_exit, RDIDbgSPxy, RDIDbgSPxy_nm, "push thread\n");
    }
    event = _ntfqueue.remove_pri_head();
    qsize = _ntfqueue.length();
    _nevents += 1;

    // Do not hold OPLOCK across push
    outcall_worked = 0;
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);

      // if we did not wait, yield here (wait or yield each time through outer loop)
      if (do_yield) {
	TW_YIELD();
      }

      try {
#ifndef WRAPPED_ANY_THREADSAFE
	const CosN::StructuredEvent& sev = event->get_cos_event();
	if ( RDI_STR_EQ(event->get_type_name(), "%ANY") ) {
	  // Need to copy remainder_of_body to produce thread-safe marshalling without
	  // holding event lock.  An efficient form of copy is used here (shares buffer).
	  // Need to hold event lock across copy.
	  RDI_SEVENT_PROT_UNPROT(event, WHATFN,
				 CORBA::Any safe(sev.remainder_of_body.type(), (void*)(sev.remainder_of_body.value())),
				 _consumer->push(safe));
	} else {
	  // Construction of any_event here avoids need to worry about thread-safe marshalling.
	  // Need to hold event lock across construction.
	  CORBA::Any any_event;
	  { // introduce event lock scope
	    RDI_SEVENT_SCOPE_LOCK(event_lock, event, WHATFN);
	    any_event <<= sev;
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
	RDIDbgSPxyLog("Push thread for proxy " << (void*)this << " - Invalid object reference\n");
      } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
	RDIDbgSPxyLog("Push thread for proxy " << (void*)this << " - Consumer object does not exist\n");
      } catch ( CORBA::COMM_FAILURE& e ) {
	RDIDbgSPxyLog("Push thread for proxy " << (void*)this << " - Communication failure while pushing event to consumer\n");
      } catch (...) {
	RDIDbgSPxyLog("Push thread for proxy " << (void*)this << " - Exception while pushing event to consumer\n");
      }
      RDI_SEVENT_DECR_REF_COUNTER(event, WHATFN);
    } // end unlock scope
    if (!held.sproxy) {
      RDI_Fatal("ProxyPushSupplier_i::_push_event push thread [**unexpected REACQUIRE failure**]\n");
    }

    if (_pxstate != RDI_Connected) {
      // disconnected/disposed during push -- do nothing else with my state
    } else {
      if (outcall_worked) {
	_channel->incr_num_notifications(qsize);
      } else { // exception
	// unregister this proxy -- no more subscription_change msgs
	RDI_ChangePool* cocp = _channel->ochange_pool();
	if ( ! _oc_off && ! CORBA::is_nil(_nc_consumer) && cocp ) {
	  cocp->remove_proxy(this);
	}
	_clear_ntfqueue();
	_pxstate = RDI_Exception;
      }
    }
  }

 push_exit:
  TW_EXIT();
}

#undef WHATFN
#define WHATFN "ProxyPushSupplier_i::add_event"
void
ProxyPushSupplier_i::add_event(RDI_StructuredEvent* entry)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_add_event(entry)) {
    if ( _worker ) {
      RDI_OPLOCK_SIGNAL;
    }
    RDI_NotifyConsumer* cpc = _channel->push_consumer();
    if ( cpc ) {
      cpc->signal_push_threads();
    }
  }
}

#undef WHATFN
#define WHATFN "ProxyPushSupplier_i::disconnect_push_supplier"
void
ProxyPushSupplier_i::disconnect_push_supplier( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; } 
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "ProxyPushSupplier_i::_disconnect_client_and_dispose"
// *** lock must be held, held.sproxy set
// Effect: initiate impl dispose
void
ProxyPushSupplier_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
						    CORBA::Boolean            remove_from_admin,
						    WRAPPED_DISPOSEINFO_PTR&  dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgSPxyLog("** Internal error -- ProxyPushSupplier_i::_disconnect_client_and_dispose called twice on same proxy!\n");
    return;
  }
  // unregister this proxy -- no more offer_change msgs
  RDI_ChangePool* cocp = _channel->ochange_pool();
  if ( ! _oc_off && (_pxstate == RDI_Connected) && ! CORBA::is_nil(_nc_consumer) && cocp ) {
    cocp->remove_proxy(this);
  }
  _pxstate = RDI_Disconnected; // acts as guard; following only entered by 1 thread
  // Wait for inuse to drop to one.  This ensures that any call that
  // bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
      _myadmin->remove_proxy(held, this);
    } // end unlock scope
    if (!held.sproxy) {
      RDI_Fatal("ProxyPushSupplier_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }

  // To do a typemap update (via remove_all_filters or update_mapping),
  // we need to release our lock and then obtain locks in this order:
  //   channel -> typemap -> sproxy

  if (held.channel && held.typemap) {
    // lucky us, we do not have to play with locks
    if ( _rqstypes.length() == 0 ) {
      // RDIDbgForceLog("XXX_REMOVE " << WHATFN << " calling _fa_helper.remove_all_filters");
      _fa_helper.remove_all_filters(held, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callbacks
    } else {
      CosN::EventTypeSeq added; added.length(0);
      (void) _channel->update_mapping(held, added, _rqstypes, this, 0);
    }
  } else { // introduce scope to temporarily release bump lock
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
    { // introduce channel lock scope
      RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(chan_lock, held.channel, _channel, WHATFN);
      if (!held.channel || _channel->shutting_down()) { goto skip_update; }
      { // introduce typemap lock scope
	RDI_CHAN_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(typemap_w_lock, held.typemap, _channel, WHATFN);
	if (!held.typemap) { goto skip_update; }
	{ // introduce inner sproxy lock scope
	  RDI_OPLOCK_SCOPE_LOCK_TRACK(inner_proxy_lock, held.sproxy, WHATFN);
	  if (!held.sproxy) { goto skip_update; }

	  // FINALLY we can do some real work.
	  if ( _rqstypes.length() == 0 ) { 
	    // RDIDbgForceLog("XXX_REMOVE " << WHATFN << " calling _fa_helper.remove_all_filters");
	    _fa_helper.remove_all_filters(held, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callbacks
	  } else {
	    CosN::EventTypeSeq added; added.length(0);
	    (void) _channel->update_mapping(held, added, _rqstypes, this, 0);
	  }

	} // end inner sproxy lock scope
      } // end typemap lock scope
    } // end channel lock scope
  } // end temporary release scope
  
 skip_update:
  _active  = 0;
  _consumer = CosEvC::PushConsumer::_nil();
  _nc_consumer = CosNC::PushConsumer::_nil();
  _pfilter = CosNF::MappingFilter::_nil();
  _lfilter = CosNF::MappingFilter::_nil();
  if ( _qosprop ) {
    delete _qosprop; _qosprop = 0;
  }
  _clear_ntfqueue(); // Remove all events
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

// ------------------------------------------------------------- //
// StructuredProxyPushSupplier_i                                 //
// ------------------------------------------------------------- //

StructuredProxyPushSupplier_i::StructuredProxyPushSupplier_i(
							     ConsumerAdmin_i* admin, 
							     EventChannel_i*  chann,
							     const CosNA::ProxyID&  prxid) : 
  RDIProxySupplier("StructuredProxyPushSupplier", "StructuredProxyPushSupplier_fa_helper",
		   admin, chann, RDI_S_StrPRX, CosNA::PUSH_STRUCTURED, prxid),
  _worker(0)
{
  _consumer = CosNC::StructuredPushConsumer::_nil();
  // When the number of push threads allocated at the channel level is
  // 0, each proxy uses its own thread to push events to its consumer.
  if (  _channel->push_threads() == 0 ) {
    // _worker = new StrProxyWorker(this,&StructuredProxyPushSupplier_i::_push_event);
    _worker = new StrProxyBoundWorker(this,&StructuredProxyPushSupplier_i::_push_event);
    // If we use unbound threads, we should start the thread using
    // _worker->start();
    RDIDbgSPxyLog("PushSupplier Proxy Thread " << _worker->id());
  }
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

StructuredProxyPushSupplier_i::~StructuredProxyPushSupplier_i()
{
  RDI_OPLOCK_DESTROY_CHECK("StructuredProxyPushSupplier_i");
}

#undef WHATFN
#define WHATFN "StructuredProxyPushSupplier_i::connect_structured_push_consumer"
void
StructuredProxyPushSupplier_i::connect_structured_push_consumer(CosNC::StructuredPushConsumer_ptr consumer
                                                                 WRAPPED_IMPLARG)
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
  _active  = 1;
  _pxstate = RDI_Connected;
  _consumer= CosNC::StructuredPushConsumer::_duplicate(consumer);
  _oc_subscriber = CosNC::NotifyPublish::_narrow(_consumer); // implicit duplicate
  if (CORBA::is_nil(_oc_subscriber)) {
    RDIDbgSPxyLog("** UNEXPECTED: CosNC::StructuredPushConsumer could not be narrowed to CosNC::NotifyPublish\n");
  }
  // if enabled, register this proxy for offer_change msgs
  RDI_ChangePool* cocp = _channel->ochange_pool();
  if ( ! _oc_off && cocp ) {
    cocp->insert_proxy(this);
  }
}

#undef WHATFN
#define WHATFN "StructuredProxyPushSupplier_i::suspend_connection"
void
StructuredProxyPushSupplier_i::suspend_connection( WRAPPED_IMPLARG_VOID )
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
#define WHATFN "StructuredProxyPushSupplier_i::resume_connection"
void
StructuredProxyPushSupplier_i::resume_connection( WRAPPED_IMPLARG_VOID )
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
  if ( _worker ) {
    RDI_OPLOCK_SIGNAL;
  }
  RDI_NotifyConsumer* cpc = _channel->push_consumer();
  if ( cpc ) {
    cpc->signal_push_threads();
  }
}

#undef WHATFN
#define WHATFN "StructuredProxyPushSupplier_i::has_events"
CORBA::Boolean
StructuredProxyPushSupplier_i::has_events(unsigned long* wait_s, unsigned long* wait_t)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return 0);
  if ( !_oplockptr || (_pxstate != RDI_Connected) || !_active || !_ntfqueue.length() ) {
    return 0;
  }
  return 1;
}

#undef WHATFN
#define WHATFN "StructuredProxyPushSupplier_i::push_event"
void
StructuredProxyPushSupplier_i::push_event(CORBA::Boolean& invalid)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { return; }

  CORBA::Boolean       outcall_worked;
  RDI_StructuredEvent* event = 0;;
  unsigned int         qsize = 0;

  invalid = 0;
  if ( (_pxstate == RDI_Disconnected) || (_pxstate == RDI_Exception) ) {
    // tell caller no more pushes (by setting invalid to 1)
    invalid = 1;
    return;
  }
  if ( (_pxstate != RDI_Connected) || !_active || !_ntfqueue.length() ) {
    return;
  }
  event = _ntfqueue.remove_pri_head();
  qsize = _ntfqueue.length();
  _nevents += 1;

  // Do not hold OPLOCK across push
  outcall_worked = 0;
  { // introduce unlock scope
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);

    try {
#ifndef WRAPPED_ANY_THREADSAFE
      // Need to copy event->get_cos_event() to produce thread-safe marshalling.
      // XXX_TODO need a copy that shares the internal CORBA::Any buffers.
      // Need to hold event lock across copy.
      RDI_SEVENT_PROT_UNPROT(event, WHATFN,
			     CosN::StructuredEvent safe(event->get_cos_event()),
			     _consumer->push_structured_event(safe));
#else
      // Can marshall event safely wihtout event lock.
      _consumer->push_structured_event(event->get_cos_event());
#endif
      outcall_worked = 1;
#ifndef NO_OBJ_GC
      _last_use.set_curtime();
#endif
    } catch ( CORBA::INV_OBJREF& e ) {
      RDIDbgSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Invalid object reference\n");
    } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
      RDIDbgSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Consumer object does not exist\n");
    } catch ( CORBA::COMM_FAILURE& e ) {
      RDIDbgSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Communication Failure\n");
    } catch (...) {
      RDIDbgSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Exception while pushing event to consumer\n");
    }
    RDI_SEVENT_DECR_REF_COUNTER(event, WHATFN);
  } // end unlock scope
  if (!held.sproxy) {
    RDI_Fatal("StructuredProxyPushSupplier_i::push_event [**unexpected REACQUIRE failure**]\n");
  }
  if (_pxstate != RDI_Connected) {
    // disconnected/disposed during push -- do nothing else with my state
  } else {
    if (outcall_worked) {
      _channel->incr_num_notifications(qsize);
    } else { // exception
      // unregister this proxy -- no more subscription_change msgs
      RDI_ChangePool* cocp = _channel->ochange_pool();
      if ( ! _oc_off && cocp ) {
	cocp->remove_proxy(this);
      }
      _clear_ntfqueue();
      _pxstate = RDI_Exception;
      invalid = 1;
    }
  }
}

#undef WHATFN
#define WHATFN "StructuredProxyPushSupplier_i::_push_event"
void
StructuredProxyPushSupplier_i::_push_event()
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { return; }

  CORBA::Boolean       outcall_worked;
  RDI_StructuredEvent* event = 0;
  unsigned int         qsize = 0;
  CORBA::Boolean       do_yield = 0;

  while ( 1 ) {
    do_yield = 1;
    while ( (_pxstate==RDI_NotConnected) ||
	    ((_pxstate==RDI_Connected)&&(!_active || !_ntfqueue.length())) ) {
      do_yield = 0;
      RDI_OPLOCK_WAIT(WHATFN);
    }
    if ( _pxstate != RDI_Connected ) {
      RDI_THREAD_EXIT(struct_push_exit, RDIDbgSPxy, RDIDbgSPxy_nm, "push thread\n");
    }
    event = _ntfqueue.remove_pri_head();
    qsize = _ntfqueue.length();
    _nevents += 1;

    // Do not hold OPLOCK across push
    outcall_worked = 0;
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);

      // if we did not wait, yield here (wait or yield each time through outer loop)
      if (do_yield) {
	TW_YIELD();
      }

      try {
#ifndef WRAPPED_ANY_THREADSAFE
	// Need to copy event->get_cos_event() to produce thread-safe marshalling.
	// XXX_TODO need a copy that shares the internal CORBA::Any buffers.
	// Need to hold event lock across copy.
	RDI_SEVENT_PROT_UNPROT(event, WHATFN,
			       CosN::StructuredEvent safe(event->get_cos_event()),
			       _consumer->push_structured_event(safe));
#else
	// Can marshall event safely without event lock.
	_consumer->push_structured_event(event->get_cos_event());
#endif
	outcall_worked = 1;
#ifndef NO_OBJ_GC
	_last_use.set_curtime();
#endif
      } catch ( CORBA::INV_OBJREF& e ) {
	RDIDbgSPxyLog("Push thread for proxy " << (void*)this << " - Invalid object reference\n");
      } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
	RDIDbgSPxyLog("Push thread for proxy " << (void*)this << " - Consumer object does not exist\n");
      } catch ( CORBA::COMM_FAILURE& e ) {
	RDIDbgSPxyLog("Push thread for proxy " << (void*)this << " - Communication failure while pushing event to consumer\n");
      } catch (...) {
	RDIDbgSPxyLog("Push thread for proxy " << (void*)this << " - Exception while pushing event to consumer\n");
      }
      RDI_SEVENT_DECR_REF_COUNTER(event, WHATFN);
    } // end unlock scope
    if (!held.sproxy) {
      RDI_Fatal("StructuredProxyPushSupplier_i::_push_event push thread [**unexpected REACQUIRE failure**]\n");
    }

    if (_pxstate != RDI_Connected) {
      // disconnected/disposed during push -- do nothing else with my state
    } else {
      if (outcall_worked) {
	_channel->incr_num_notifications(qsize);
      } else { // exception
	// unregister this proxy -- no more subscription_change msgs
	RDI_ChangePool* cocp = _channel->ochange_pool();
	if ( ! _oc_off && cocp ) {
	  cocp->remove_proxy(this);
	}
	_pxstate = RDI_Exception;
	_clear_ntfqueue();
      }
    }
  }

 struct_push_exit:
  TW_EXIT();
}

#undef WHATFN
#define WHATFN "StructuredProxyPushSupplier_i::add_event"
void
StructuredProxyPushSupplier_i::add_event(RDI_StructuredEvent* entry)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_add_event(entry)) {
    if ( _worker ) {
      RDI_OPLOCK_SIGNAL;
    }
    RDI_NotifyConsumer* cpc = _channel->push_consumer();
    if ( cpc ) {
      cpc->signal_push_threads();
    }
  }
}

#undef WHATFN
#define WHATFN "StructuredProxyPushSupplier_i::disconnect_structured_push_supplier"
void
StructuredProxyPushSupplier_i::disconnect_structured_push_supplier( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; } 
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "StructuredProxyPushSupplier_i::_disconnect_client_and_dispose"
// *** lock must be held, held.sproxy set
// Effect: initiate impl dispose
void
StructuredProxyPushSupplier_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
							      CORBA::Boolean            remove_from_admin,
							      WRAPPED_DISPOSEINFO_PTR&  dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgSPxyLog("** Internal error -- StructuredProxyPushSupplier_i::_disconnect_client_and_dispose called twice on same proxy!\n");
    return;
  }
  // unregister this proxy -- no more offer_change msgs
  RDI_ChangePool* cocp = _channel->ochange_pool();
  if ( ! _oc_off && (_pxstate == RDI_Connected) && cocp ) {
    cocp->remove_proxy(this);
  }
  _pxstate = RDI_Disconnected; // acts as guard; following only entered by 1 thread
  // Wait for inuse to drop to one.  This ensures that any call that
  // bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
      _myadmin->remove_proxy(held, this);
    } // end unlock scope
    if (!held.sproxy) {
      RDI_Fatal("StructuredProxyPushSupplier_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }

  // To do a typemap update (via remove_all_filters or update_mapping),
  // we need to release our lock and then obtain locks in this order:
  //   channel -> typemap -> sproxy

  if (held.channel && held.typemap) {
    // lucky us, we do not have to play with locks
    if ( _rqstypes.length() == 0 ) { 
      _fa_helper.remove_all_filters(held, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callbacks
    } else {
      CosN::EventTypeSeq added; added.length(0);
      (void) _channel->update_mapping(held, added, _rqstypes, this, 0);
    }
  } else { // introduce scope to temporarily release bump lock
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
    { // introduce channel lock scope
      RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(chan_lock, held.channel, _channel, WHATFN);
      if (!held.channel || _channel->shutting_down()) { goto skip_update; }
      { // introduce typemap lock scope
	RDI_CHAN_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(typemap_w_lock, held.typemap, _channel, WHATFN);
	if (!held.typemap) { goto skip_update; }
	{ // introduce inner sproxy lock scope
	  RDI_OPLOCK_SCOPE_LOCK_TRACK(inner_proxy_lock, held.sproxy, WHATFN);
	  if (!held.sproxy) { goto skip_update; }

	  // FINALLY we can do some real work.
	  if ( _rqstypes.length() == 0 ) { 
	    _fa_helper.remove_all_filters(held, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callbacks
	  } else {
	    CosN::EventTypeSeq added; added.length(0);
	    (void) _channel->update_mapping(held, added, _rqstypes, this, 0);
	  }

	} // end inner sproxy lock scope
      } // end typemap lock scope
    } // end channel lock scope
  } // end temporary release scope

 skip_update:
  _active  = 0;
  _consumer =  CosNC::StructuredPushConsumer::_nil();
  _pfilter = CosNF::MappingFilter::_nil();
  _lfilter = CosNF::MappingFilter::_nil();
  if ( _qosprop ) {
    delete _qosprop; _qosprop = 0;
  }
  _clear_ntfqueue(); // Remove all events
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

// ------------------------------------------------------------- //
// SequenceProxyPushSupplier_i                                   //
// ------------------------------------------------------------- //

SequenceProxyPushSupplier_i::SequenceProxyPushSupplier_i(
							 ConsumerAdmin_i* admin, 
							 EventChannel_i*  chann,
							 const CosNA::ProxyID&  prxid) : 
  RDIProxySupplier("SequenceProxyPushSupplier", "SequenceProxyPushSupplier_fa_helper", 
		   admin, chann, RDI_S_SeqPRX, CosNA::PUSH_SEQUENCE, prxid),
  _worker(0)
{
  unsigned long pacing_interval_s, pacing_interval_n;
  _consumer = CosNC::SequencePushConsumer::_nil();

  // If pacingInterval is non-zero, set initial timeout.
  // Note that the interval typically expires before any events are
  // placed in the notification queue of this proxy, which means
  // the first batch normally contains just a single event.
  // The only way to fix this would be to initialize the pacing
  // interval on the very first event insertion.
  _qosprop->pacingInterval_s_n(pacing_interval_s, pacing_interval_n);
  if (pacing_interval_s || pacing_interval_n) { // update timeout
    TW_GET_TIME(&_timeout_s, &_timeout_n, pacing_interval_s, pacing_interval_n);
  } else {
    _timeout_s = 0; _timeout_n = 0;
  }

  // When the number of push threads allocated at the channel level is
  // 0, each proxy uses its own thread to push events to its consumer.
  if (  _channel->push_threads() == 0 ) {
    // _worker = new SeqProxyWorker(this,&SequenceProxyPushSupplier_i::_push_event);
    _worker = new SeqProxyBoundWorker(this,&SequenceProxyPushSupplier_i::_push_event);
    // If we use unbound threads, we should start the thread using
    // _worker->start();
    RDIDbgSPxyLog("PushSupplier Proxy Thread " << _worker->id());
  }
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

SequenceProxyPushSupplier_i::~SequenceProxyPushSupplier_i()
{
  RDI_OPLOCK_DESTROY_CHECK("SequenceProxyPushSupplier_i");
}

#undef WHATFN
#define WHATFN "SequenceProxyPushSupplier_i::connect_sequence_push_consumer"
void
SequenceProxyPushSupplier_i::connect_sequence_push_consumer(CosNC::SequencePushConsumer_ptr consumer
                                                             WRAPPED_IMPLARG)
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
  _active   = 1;
  _pxstate  = RDI_Connected;
  _consumer = CosNC::SequencePushConsumer::_duplicate(consumer);
  _oc_subscriber = CosNC::NotifyPublish::_narrow(_consumer); // implicit duplicate
  if (CORBA::is_nil(_oc_subscriber)) {
    RDIDbgSPxyLog("** UNEXPECTED: CosNC::SequencePushConsumer could not be narrowed to CosNC::NotifyPublish\n");
  }
  // if enabled, register this proxy for offer_change msgs
  RDI_ChangePool* cocp = _channel->ochange_pool();
  if ( ! _oc_off && cocp ) {
    cocp->insert_proxy(this);
  }
}

#undef WHATFN
#define WHATFN "SequenceProxyPushSupplier_i::suspend_connection"
void
SequenceProxyPushSupplier_i::suspend_connection( WRAPPED_IMPLARG_VOID )
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
#define WHATFN "SequenceProxyPushSupplier_i::resume_connection"
void
SequenceProxyPushSupplier_i::resume_connection( WRAPPED_IMPLARG_VOID )
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
  if ( _worker ) { 
    RDI_OPLOCK_SIGNAL;
  }
  RDI_NotifyConsumer* cpc = _channel->push_consumer();
  if ( cpc ) {
    cpc->signal_push_threads();
  }
}

#undef WHATFN
#define WHATFN "SequenceProxyPushSupplier_i::has_events"
// has_events determines whether a push should occur.
// in this case the requirements are that either the max batch size
// has been reached or there is at least 1 event and pacing interval reached
CORBA::Boolean
SequenceProxyPushSupplier_i::has_events(unsigned long* wait_s, unsigned long* wait_n)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return 0);

  CORBA::Long qsize = _ntfqueue.length();
  if ( (_pxstate != RDI_Connected) || (!_active) || (qsize == 0) ) {
    return 0;
  }

  CORBA::Long bsize = _qosprop->maximumBatchSize();

  if (qsize >= bsize) {
    return 1;
  }

  // pacing interval : both 0 means always wait for full batch size
  unsigned long pacing_interval_s, pacing_interval_n;
  unsigned long time_s, time_n;
  _qosprop->pacingInterval_s_n(pacing_interval_s, pacing_interval_n);
  if ( (pacing_interval_s == 0) && (pacing_interval_n == 0) ) {
    _timeout_s = 0; _timeout_n = 0;
    return 0; // no pacing interval
  }

  TW_GET_TIME(&time_s, &time_n);

  if ( (_timeout_s == 0) && (_timeout_n == 0) ) {
    // proxy has not established timeout yet : set it now
    TW_GET_TIME(&_timeout_s, &_timeout_n, pacing_interval_s, pacing_interval_n);
  }

  if ( (time_s > _timeout_s) || ((time_s == _timeout_s) && (time_n > _timeout_n)) ) {
    return 1; // pacing interval timeout occurred for this proxy
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
#define WHATFN "SequenceProxyPushSupplier_i::push_event"
// REQUIREMENT: A thread from push pool only calls this method
// if has_events() is true and it thinks state is connected and active.
// (For safety we verify that these conditions actually hold.)
void
SequenceProxyPushSupplier_i::push_event(CORBA::Boolean& invalid)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { return; }

  CORBA::Boolean outcall_worked;
  invalid = 0;
  CORBA::Long qsize = _ntfqueue.length();
  if ( (_pxstate == RDI_Disconnected) || (_pxstate == RDI_Exception) ) {
    // tell caller no more pushes (by setting invalid to 1)
    invalid = 1;
    return;
  }
  if ( (_pxstate != RDI_Connected) || (!_active) || (qsize == 0) ) {
    return;
  }

  // pacing interval : both 0 means always wait for full batch size
  unsigned long pacing_interval_s, pacing_interval_n;
  unsigned long time_s, time_n;
  _qosprop->pacingInterval_s_n(pacing_interval_s, pacing_interval_n);
  CORBA::Long   i, bsize = _qosprop->maximumBatchSize();
  RDI_StructuredEvent** event = 0;
  CORBA::Long actsize;

  if ( (qsize < bsize) && (pacing_interval_s == 0) && (pacing_interval_n == 0) ) {
    return;
  }
  // if queue size < bsize, verify that a timeout really occurred
  if ( qsize < bsize) { // one/both of pacing_interval_s/_n must be non-zero
    TW_GET_TIME(&time_s, &time_n);
    if ((_timeout_s == 0) && (_timeout_n == 0)) { // timeout not set; set it now
      TW_GET_TIME(&_timeout_s, &_timeout_n, pacing_interval_s, pacing_interval_n);
    } 
    if ( (time_s < _timeout_s) || ((time_s == _timeout_s) && (time_n < _timeout_n)) ) {
      RDIDbgSPxyLog("** INTERNAL ERROR: SequenceProxyPushSupplier_i::push_event called too soon\n");
      return;
    }
  }

  actsize = (qsize < bsize) ? qsize : bsize;
  CosN::EventBatch notif(actsize);
  notif.length(actsize);

  event = new RDI_StructuredEvent * [actsize];
  RDI_AssertAllocThrowNo(event, "Memory allocation failed -- RDI_StructuredEvent\n");

  for ( i = 0; i < actsize; i++ ) {
    event[i] = _ntfqueue.remove_pri_head();
#ifndef WRAPPED_ANY_THREADSAFE
    // Need to copy the events to produce thread-safe marshalling.
    // XXX_TODO need a copy that shares the internal CORBA::Any buffers.
    // Need to hold event lock across copy.
    { // introduce event lock scope
      RDI_SEVENT_SCOPE_LOCK(event_lock, event[i], WHATFN);
      notif[i] = event[i]->get_cos_event();
    } // end event lock scope
#else
    // XXX_TODO would rather not copy here, but not sure how to avoid it.
    // XXX_TODO if going to copy, need a copy that shares the internal CORBA::Any buffers.
    notif[i] = event[i]->get_cos_event();
#endif
  }
  _nevents += actsize;
  // update timeout before releasing OPLOCK -- this means we do the update
  // before doing the push (seems OK)
  if (pacing_interval_s || pacing_interval_n) {
    TW_GET_TIME(&_timeout_s, &_timeout_n, pacing_interval_s, pacing_interval_n);
  } else {
    _timeout_s = 0; _timeout_n = 0;
  }

  // Do not hold OPLOCK across push
  outcall_worked = 0;
  { // introduce unlock scope
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);

    try {
      _consumer->push_structured_events(notif);
      outcall_worked = 1;
#ifndef NO_OBJ_GC
      _last_use.set_curtime();
#endif
    } catch ( CORBA::INV_OBJREF& e ) {
      RDIDbgSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Invalid object reference\n");
    } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
      RDIDbgSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Consumer object does not exist\n");
    } catch ( CORBA::COMM_FAILURE& e ) {
      RDIDbgSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Communication Failure\n");
    } catch (...) {
      RDIDbgSPxyLog("Push pool calling push_event for proxy " << (void*)this << " - Exception while pushing event to consumer\n");
    }
  } // end unlock scope
  if (!held.sproxy) {
    RDI_Fatal("SequenceProxyPushSupplier_i::push_event [**unexpected REACQUIRE failure**]\n");
  }
  for ( i = 0; i < actsize; i++ ) {
    RDI_SEVENT_DECR_REF_COUNTER(event[i], WHATFN);
    event[i] = 0;
  }
  delete [] event;

  if (_pxstate != RDI_Connected) {
    // disconnected/disposed during push -- do nothing else with my state
  } else {
    if (outcall_worked) {
      for ( i = 0; i < actsize; i++ ) {
	_channel->incr_num_notifications(--qsize);
      }
    } else { // exception
      // unregister this proxy -- no more subscription_change msgs
      RDI_ChangePool* cocp = _channel->ochange_pool();
      if ( ! _oc_off && cocp ) {
	cocp->remove_proxy(this);
      }
      _pxstate = RDI_Exception;
      _clear_ntfqueue();
      invalid = 1;
    }
  }
}

#undef WHATFN
#define WHATFN "SequenceProxyPushSupplier_i::_push_event"
void
SequenceProxyPushSupplier_i::_push_event()
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { return; }

  CORBA::Boolean outcall_worked;
  // pacing interval : both 0 means always wait for full batch size
  unsigned long pacing_interval_s, pacing_interval_n;
  unsigned long time_s, time_n;
  CORBA::Boolean update_timeout = 0, do_yield = 0;

  // get initial qospr and bsize, setup initial notif and event based on bsize
  CORBA::Long  i, bsize = _qosprop->maximumBatchSize();

  CosN::EventBatch notif(bsize);
  RDI_StructuredEvent** event = new RDI_StructuredEvent*[bsize];
  RDI_AssertAllocThrowNo(event, "Memory allocation failed -- RDI_StructuredEvent\n");
  CORBA::Long eventsize = bsize;
  CORBA::Long qsize, actsize;

  while ( 1 ) {
    do_yield = 1;
    while ( 1 ) {
      // must recompute these here because they can change across a wait/timedwait
      qsize = _ntfqueue.length();
      _qosprop->pacingInterval_s_n(pacing_interval_s, pacing_interval_n);
      if (pacing_interval_s || pacing_interval_n) {
	// if timeout not set, or if update_timeout true due to push, recompute timeout
	if ( update_timeout || ((_timeout_s == 0) && (_timeout_n == 0)) ) {
	  TW_GET_TIME(&_timeout_s, &_timeout_n, pacing_interval_s, pacing_interval_n);
	}
      } else {
	_timeout_s = 0; _timeout_n = 0;
      }
      update_timeout = 0;
      bsize = _qosprop->maximumBatchSize();
      if (bsize > eventsize) { // need a larger array
	delete [] event;
	event = new RDI_StructuredEvent*[bsize];
	RDI_AssertAllocThrowNo(event, "Memory allocation failed -- RDI_StructuredEvent\n");
	eventsize = bsize;
      }
      // breaking from inner loop requires either a sufficient batch size or a
      // pacing interval timeout (and state must be RDI_Connected and _active)
      if ((_pxstate!=RDI_NotConnected) && (_pxstate!=RDI_Connected)) {
	break; // also break on exceptional state
      }
      if ((_pxstate==RDI_Connected) && _active) {
	if ( qsize >= bsize )
	  break;
	if ( qsize && (pacing_interval_s || pacing_interval_n) ) {
	  TW_GET_TIME(&time_s, &time_n);
	  if ( (time_s > _timeout_s) || ((time_s == _timeout_s) && (time_n > _timeout_n)) ) {
	    // pacing interval has passed so go ahead and send a smaller batch
	    break;
	  }
	}
      }
      // must wait
      if ( qsize && (pacing_interval_s || pacing_interval_n) ) {
	//	RDIDbgSPxyLog("sequence proxy doing timedwait(" << _timeout_s << "," << _timeout_n <<")\n");
	do_yield = 0;
	RDI_OPLOCK_TIMEDWAIT(_timeout_s, _timeout_n, WHATFN);
      } else {
	//	RDIDbgSPxyLog("sequence proxy doing normal wait\n");
	do_yield = 0;
	RDI_OPLOCK_WAIT(WHATFN);
      }
    }
    if ( _pxstate != RDI_Connected ) {
      delete [] event;
      RDI_THREAD_EXIT(seq_push_exit, RDIDbgSPxy, RDIDbgSPxy_nm, "push thread\n");
    }
    actsize = (qsize < bsize) ? qsize : bsize;
    notif.length(actsize);
    for ( i = 0; i < actsize; i++ ) {
      event[i] = _ntfqueue.remove_pri_head();
#ifndef WRAPPED_ANY_THREADSAFE
      // Need to copy the events to produce thread-safe marshalling.
      // XXX_TODO need a copy that shares the internal CORBA::Any buffers.
      // Need to hold event lock across copy.
      { // introduce event lock scope
	RDI_SEVENT_SCOPE_LOCK(event_lock, event[i], WHATFN);
	notif[i] = event[i]->get_cos_event();
      } // end event lock scope
#else
      // XXX_TODO would rather not copy here, but not sure how to avoid it.
      // XXX_TODO if going to copy, need a copy that shares the internal CORBA::Any buffers.
      notif[i] = event[i]->get_cos_event();
#endif
    }
    _nevents += actsize;

    // Do not hold OPLOCK across push
    outcall_worked = 0;
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
    
      // if we did not wait, yield here (wait or yield each time through outer loop)
      if (do_yield) {
	TW_YIELD();
      }

      try {
	_consumer->push_structured_events(notif);
	outcall_worked = 1;
#ifndef NO_OBJ_GC
	_last_use.set_curtime();
#endif
      } catch ( CORBA::INV_OBJREF& e ) {
	RDIDbgSPxyLog("Push thread for proxy " << (void*)this << " - Invalid object reference\n");
      } catch ( CORBA::OBJECT_NOT_EXIST& e ) {
	RDIDbgSPxyLog("Push thread for proxy " << (void*)this << " - Consumer object does not exist\n");
      } catch ( CORBA::COMM_FAILURE& e ) {
	RDIDbgSPxyLog("Push thread for proxy " << (void*)this << " - Communication Failure\n");
      } catch (...) {
	RDIDbgSPxyLog("Push thread for proxy " << (void*)this << " - Exception while pushing event to consumer\n");
      }
    } // end unlock scope
    if (!held.sproxy) {
      RDI_Fatal("SequenceProxyPushSupplier_i::_push_event [**unexpected REACQUIRE failure**]\n");
    }

    update_timeout = 1; // force update of timeout at top of loop

    for ( i = 0; i < actsize; i++ ) {
      RDI_SEVENT_DECR_REF_COUNTER(event[i], WHATFN);
      event[i] = 0;
    }

    if (_pxstate != RDI_Connected) {
      // disconnected/disposed during push -- do nothing else with my state
    } else {
      if (outcall_worked) {
	for ( i = 0; i < actsize; i++ ) {
	  _channel->incr_num_notifications(--qsize);
	}
      } else { // exception
	// unregister this proxy -- no more subscription_change msgs
	RDI_ChangePool* cocp = _channel->ochange_pool();
	if ( ! _oc_off && cocp ) {
	  cocp->remove_proxy(this);
	}
	_pxstate = RDI_Exception;
	_clear_ntfqueue();
      }
    }
  }

 seq_push_exit:
  TW_EXIT();
}

#undef WHATFN
#define WHATFN "SequenceProxyPushSupplier_i::add_event"
void
SequenceProxyPushSupplier_i::add_event(RDI_StructuredEvent* entry)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_add_event(entry)) {
    if ( _worker ) { 
      RDI_OPLOCK_SIGNAL;
    }
    RDI_NotifyConsumer* cpc = _channel->push_consumer();
    if ( cpc ) {
      cpc->signal_push_threads();
    }
  }
}

#undef WHATFN
#define WHATFN "SequenceProxyPushSupplier_i::disconnect_sequence_push_supplier"
void
SequenceProxyPushSupplier_i::disconnect_sequence_push_supplier( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(proxy_lock, held.sproxy, WHATFN);
  if (!held.sproxy) { RDI_THROW_INV_OBJREF; } 
  if (_pxstate == RDI_Disconnected) { RDI_THROW_INV_OBJREF; } // already in process of being disposed
  _disconnect_client_and_dispose(held, 1, proxy_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "SequenceProxyPushSupplier_i::_disconnect_client_and_dispose"
// *** lock must be held, held.sproxy set
// Effect: initiate impl dispose
void
SequenceProxyPushSupplier_i::_disconnect_client_and_dispose(RDI_LocksHeld&            held,
							    CORBA::Boolean            remove_from_admin,
							    WRAPPED_DISPOSEINFO_PTR&  dispose_info)
{
  if (_pxstate == RDI_Disconnected) {
    RDIDbgSPxyLog("** Internal error -- SequenceProxyPushSupplier_i::_disconnect_client_and_dispose called twice on same proxy!\n");
    return;
  }
  // unregister this proxy -- no more offer_change msgs
  RDI_ChangePool* cocp = _channel->ochange_pool();
  if ( ! _oc_off && (_pxstate == RDI_Connected) && cocp ) {
    cocp->remove_proxy(this);
  }
  _pxstate = RDI_Disconnected; // acts as guard; following only entered by 1 thread
  // Wait for inuse to drop to one.  This ensures that any call that
  // bumps inuse and releases the lock has a chance to complete cleanly.
  while (_oplockptr->inuse() > 1) {
    RDI_OPLOCK_BROADCAST;
    RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
  }
  if (remove_from_admin) {
    // do not hold OPLOCK across upcalls involving this
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
      _myadmin->remove_proxy(held, this);
    } // end unlock scope
    if (!held.sproxy) {
      RDI_Fatal("SequenceProxyPushSupplier_i::_disconnect_client_and_dispose [**unexpected REACQUIRE failure**]\n");
    }
  }

  // To do a typemap update (via remove_all_filters or update_mapping),
  // we need to release our lock and then obtain locks in this order:
  //   channel -> typemap -> sproxy

  if (held.channel && held.typemap) {
    // lucky us, we do not have to play with locks
    if ( _rqstypes.length() == 0 ) { 
      _fa_helper.remove_all_filters(held, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callbacks
    } else {
      CosN::EventTypeSeq added; added.length(0);
      (void) _channel->update_mapping(held, added, _rqstypes, this, 0);
    }
  } else   { // introduce scope to temporarily release bump lock
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sproxy, WHATFN);
    { // introduce channel lock scope
      RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(chan_lock, held.channel, _channel, WHATFN);
      if (!held.channel || _channel->shutting_down()) { goto skip_update; }
      { // introduce typemap lock scope
	RDI_CHAN_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(typemap_w_lock, held.typemap, _channel, WHATFN);
	if (!held.typemap) { goto skip_update; }
	{ // introduce inner sproxy lock scope
	  RDI_OPLOCK_SCOPE_LOCK_TRACK(inner_proxy_lock, held.sproxy, WHATFN);
	  if (!held.sproxy) { goto skip_update; }

	  // FINALLY we can do some real work.
	  if ( _rqstypes.length() == 0 ) { 
	    _fa_helper.remove_all_filters(held, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callbacks
	  } else {
	    CosN::EventTypeSeq added; added.length(0);
	    (void) _channel->update_mapping(held, added, _rqstypes, this, 0);
	  }

	} // end inner sproxy lock scope
      } // end typemap lock scope
    } // end channel lock scope
  } // end temporary release scope

 skip_update:
  _active  = 0;
  _consumer =  CosNC::SequencePushConsumer::_nil();
  _pfilter = CosNF::MappingFilter::_nil();
  _lfilter = CosNF::MappingFilter::_nil();
  if ( _qosprop ) {
    delete _qosprop; _qosprop = 0;
  }
  _clear_ntfqueue(); // Remove all events
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

////////////////////////////////////////
// Logging

RDIstrstream&
ProxyPullSupplier_i::log_output(RDIstrstream& str) const
{
  str << (void*)this << " -- " << RDI_PRX_TYPE(_prxtype);
  if ( CORBA::is_nil(_nc_consumer) ) str << " CosEventComm Consumer";
  str << _pxstate;
  str << " QSize "<< _ntfqueue.length()<< " #Pull "<< _nevents;
  if ( _rqstypes.length() != 0 ) {
    for (CORBA::ULong ix = 0; ix < _rqstypes.length(); ix++) {
      str << "\n\t" << _rqstypes[ix].domain_name; 
      str << "::" << _rqstypes[ix].type_name;
    }
  }
  return str;
}

RDIstrstream&
StructuredProxyPullSupplier_i::log_output(RDIstrstream& str) const
{
  str << (void*)this  << " -- "  << RDI_PRX_TYPE(_prxtype) << " ID ";
  str.setw(3); str << _pserial;
  if ( ! CORBA::is_nil(_pfilter ) ) str << " PFilter " << (void*)_pfilter;
  if ( ! CORBA::is_nil(_lfilter ) ) str << " LFilter " << (void*)_lfilter;
  str << _pxstate;
  str << " QSize "<< _ntfqueue.length()<< " #Pull "<< _nevents;
  if ( _rqstypes.length() != 0 ) {
    for (CORBA::ULong ix = 0; ix < _rqstypes.length(); ix++) {
      str << "\n\t" << _rqstypes[ix].domain_name; 
      str << "::" << _rqstypes[ix].type_name;
    }
  }
  return str;
}

RDIstrstream&
SequenceProxyPullSupplier_i::log_output(RDIstrstream& str) const
{
  str << (void*)this  << " -- "  << RDI_PRX_TYPE(_prxtype) << " ID ";
  str.setw(3); str << _pserial;
  if ( ! CORBA::is_nil(_pfilter) ) str << " PFilter " << (void*)_pfilter;
  if ( ! CORBA::is_nil(_lfilter) ) str << " LFilter " << (void*)_lfilter;
  str << _pxstate;
  str << " QSize "<< _ntfqueue.length()<< " #Pull "<< _nevents;
  if ( _rqstypes.length() != 0 ) {
    for (CORBA::ULong ix = 0; ix < _rqstypes.length(); ix++) {
      str << "\n\t" << _rqstypes[ix].domain_name; 
      str << "::" << _rqstypes[ix].type_name;
    }
  }
  return str;
}

RDIstrstream&
ProxyPushSupplier_i::log_output(RDIstrstream& str) const
{
  str << (void*)this << " -- " << RDI_PRX_TYPE(_prxtype) << " ID ";
  str.setw(3); str << _pserial;
  if ( CORBA::is_nil(_nc_consumer) )   str << " CosEventComm Consumer";
  if ( ! CORBA::is_nil(_pfilter) ) str << " PFilter " << (void*)_pfilter;
  if ( ! CORBA::is_nil(_lfilter) ) str << " LFilter " << (void*)_lfilter;
  str << _pxstate;
  str << "QSize "<< _ntfqueue.length() << " #Push "<< _nevents;
  if ( _rqstypes.length() != 0 ) {
    for (CORBA::ULong ix = 0; ix < _rqstypes.length(); ix++) {
      str << "\n\t" << _rqstypes[ix].domain_name;
      str << "::" << _rqstypes[ix].type_name;
    }
  }
  return str;
}

RDIstrstream&
StructuredProxyPushSupplier_i::log_output(RDIstrstream& str) const
{
  str << (void*)this << " -- " << RDI_PRX_TYPE(_prxtype) << " ID ";
  str.setw(3); str << _pserial;
  if ( ! CORBA::is_nil(_pfilter) ) str << " PFilter " << (void*)_pfilter;
  if ( ! CORBA::is_nil(_lfilter) ) str << " LFilter " << (void*)_lfilter;
  str << _pxstate;
  str << "QSize "<< _ntfqueue.length() << " #Push "<< _nevents;
  if ( _rqstypes.length() != 0 ) {
    for (CORBA::ULong ix = 0; ix < _rqstypes.length(); ix++) {
      str << "\n\t" << _rqstypes[ix].domain_name; 
      str << "::" << _rqstypes[ix].type_name;
    }
  }
  return str;
}

RDIstrstream&
SequenceProxyPushSupplier_i::log_output(RDIstrstream& str) const
{
  str << (void*)this << " -- " << RDI_PRX_TYPE(_prxtype) << " ID ";
  str.setw(3); str << _pserial;
  if ( ! CORBA::is_nil(_pfilter) ) str << " PFilter " << (void*)_pfilter;
  if ( ! CORBA::is_nil(_lfilter) ) str << " LFilter " << (void*)_lfilter;
  str << _pxstate;
  str << "QSize "<< _ntfqueue.length() << " #Push "<< _nevents;
  if ( _rqstypes.length() != 0 ) {
    for (CORBA::ULong ix = 0; ix < _rqstypes.length(); ix++) {
      str << "\n\t" << _rqstypes[ix].domain_name; 
      str << "::" << _rqstypes[ix].type_name;
    }
  }
  return str;
}

////////////////////////////////////////
// Interactive

#undef WHATFN
#define WHATFN "RDIProxySupplier::my_name"
AttN::NameSeq*
RDIProxySupplier::my_name( WRAPPED_IMPLARG_VOID )
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
#define WHATFN "RDIProxySupplier::child_names"
AttN::NameSeq*
RDIProxySupplier::child_names( WRAPPED_IMPLARG_VOID )
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
#define WHATFN "RDIProxySupplier::out_info_filters"
void
RDIProxySupplier::out_info_filters(RDIstrstream& str)
{
  RDI_OPLOCK_SCOPE_LOCK(proxy_lock, WHATFN, return);
  str << "----------------------------------------------------------------------\n";
  str << "Proxy Filters attached to " << _my_name << '\n';
  str << "----------------------------------------------------------------------\n";
  _fa_helper.out_info_filters(str);
}

void
RDIProxySupplier::out_config(RDIstrstream& str)
{
  str << "----------------------------------------------------------------------\n";
  str << "NotifQos settings for " << _my_name << '\n';
  str << "----------------------------------------------------------------------\n";
  _qosprop->log_output(str);
  str << '\n';
}

CORBA::Boolean
RDIProxySupplier::do_set_command(RDIstrstream& str, RDIParseCmd& p)
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
RDIProxySupplier::out_commands(RDIstrstream& str)
{
  str << "omniNotify ProxySupplier commands:\n"
      << "  up                : change target to proxy's admin\n"
      << "  debug             : show debugging information for this proxy\n"
      << "  config            : show config information for this proxy\n"
      << "  info filters      : show filters attached to this proxy\n"
      << "  set <name1> <value1> [ <name2> <value2> ... ]\n"
      << "                    : set NotifQoS property name1 to value1, name2 to value2, etc.\n";
}

char*
RDIProxySupplier::do_command(const char* cmnd, CORBA::Boolean& success,
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
    next_target = WRAPPED_IMPL2OREF(AttN::ConsumerAdmin, _myadmin);
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

