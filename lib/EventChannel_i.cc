// -*- Mode: C++; -*-
//                              File      : EventChannel_i.cc
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
//    Implementation of EventChannel_i
//

#include "thread_wrappers.h"
#include "corba_wrappers.h"
#include "RDI.h"
#include "RDICatchMacros.h"
#include "RDIStringDefs.h"
#include "RDINotifServer.h"
#include "RDITypeMap.h"
#include "RDIOplocksMacros.h"
#include "CosNotifyChannelAdmin_i.h"
#include "CosNfyUtils.h"
#include "RDIInteractive.h"

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//                             EventChannel_i                            //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

EventChannel_i::EventChannel_i(EventChannelFactory_i*        cfactory,
			       FilterFactory_i*              ffactory,
			       const CosN::QoSProperties&    init_qos,
			       const CosN::AdminProperties&  init_adm,
			       RDI_ServerQoS*                s_qos,
			       const CosNA::ChannelID&       myserial) :
  _oplockptr(0), 
#ifndef NO_OBJ_GC
  _gcollector(0), _gc_wait(0), _gc_exit(0),  _gcisactive(0),
#endif
  _my_name(cfactory->L_my_name()), _my_channel_factory(cfactory), 
  _def_consumer_admin(0), _def_supplier_admin(0), _def_filter_factory(ffactory),
  _type_map(0), _admin_qos(), _server_qos(s_qos), _serial(myserial), _admin_serial(1), 
  _num_consadmin(0), _num_suppadmin(0), _num_consumers(0), 
  _num_suppliers(0), _proxy_lock(), 
  _proxy_empty(&_proxy_lock), _rdi_dispatch(0), 
  _shutmedown(0), 
  _reporter(0),   _rep_wait(), _rep_exit(0), _repisactive(0), _repInterval(0),
  _events(0), _qos_lock(), _stats_lock(), _performance_timer(), 
  _proxy_events(8192,1024), _admin_group(0),
  _supl_admin(RDI_CorbaULongHash, RDI_CorbaULongRank), 
  _cons_admin(RDI_CorbaULongHash, RDI_CorbaULongRank),
  _evtypes(RDI_EventType::hash, RDI_EventType::rank),
  _pull_supplier(0), _push_consumer(0),
  _ochange_pool(0), _schange_pool(0)
{
  RDI_OPLOCK_INIT("channel");
  _rep_wait = _oplockptr->add_condition();
  _rep_exit = _oplockptr->add_condition();
  char buf[20];
  sprintf(buf, "chan%ld", (long)myserial);
  _my_name.length(_my_name.length()+1);
  _my_name[_my_name.length()-1] = (const char*)buf;

  CosNA::InterFilterGroupOperator ifop = CosNA::OR_OP;

  // Make sure that the _server_qos and _admin_qos preperties are valid

  RDI_Assert(_server_qos->numAdminGroups, "Zero value for numAdminGroups\n");
  RDI_Assert(_server_qos->numAdminThreads, "Zero value for numAdminThreads\n");

  _qosprop = new RDI_NotifQoS((RDI_NotifQoS*)0);
  RDI_AssertAllocThrowNo(_qosprop, "Memory allocation failure - RDI_NotifQoS\n");

  _qosprop->set_qos(init_qos);		// The factory validated them
  _admin_qos.from_admin(init_adm); 	// The factory validated them

  // Create the type mapping object to be used for matching
  _type_map = new RDI_TypeMap(this, 256);
  RDI_AssertAllocThrowNo(_type_map, "Memory allocation failure - RDI_TypeMap\n");

  // Create the manager for the queue of announced events
  _events = new RDI_EventQueue(_server_qos->numAdminThreads,
			       _server_qos->queueGCPeriod,
			       _admin_qos.maxQueueLength,
			       _admin_qos.rejectNewEvents,
			       _qosprop->discardPolicy());
  RDI_AssertAllocThrowNo(_events, "Memory allocation failure - RDI_EventQueue\n");

  // Create the thread pools for pull consumers and push suppliers
  _pull_supplier = (_server_qos->numPullThreads == 0) ? 0 :
    (new RDI_PullSupplier(_server_qos->numPullThreads, 
			  _server_qos->pullEventPeriod));
  _push_consumer = (_server_qos->numPushThreads == 0) ? 0 :
    (new RDI_NotifyConsumer(_server_qos->numPushThreads));
  // Create the thread pools for sending subscription_change and offer_change msgs
  _ochange_pool =  (_server_qos->numOChangeThreads == 0) ? 0 :
    (new RDI_ChangePool(RDI_ChangeKind_OfferChange,
			_server_qos->numOChangeThreads)); 
  _schange_pool =  (_server_qos->numSChangeThreads == 0) ? 0 :
    (new RDI_ChangePool(RDI_ChangeKind_SubscriptionChange,
			_server_qos->numSChangeThreads)); 

  // Initialize all statistics-related structures

  _thread_stats = new RDI_ThStat[RDI_TH_ARRAY_SZ];
  RDI_AssertAllocThrowNo(_thread_stats, "Memory allocation failed - RDI_ThStat object\n");
  for (unsigned int id = 0; id < RDI_TH_ARRAY_SZ; id++) {
    _thread_stats[id]._num_rdi_match     = 0;
    _thread_stats[id]._num_rvm_eval      = 0;
    _thread_stats[id]._num_announcements = 0;
    _thread_stats[id]._num_notifications = 0;
    _thread_stats[id]._qsize_acum        = 0;
    _thread_stats[id]._qsize_ctr         = 0;
  }

  _prev_num_rdi_match = 0;       _prev_num_rvm_eval = 0;
  _prev_num_announcements = 0;   _prev_num_notifications = 0;
  _gq_acm    = 0; _gq_ctr = 0;   _pq_acm = 0; _pq_ctr = 0;
  _cum_msecs = 0; _second_delta = 0;
  _gq_sleep_nanosecs = 0;
  _prev_avg_nq_sz = 0;
  _performance_timer.start();
  // the first time interval will be longer than others due to setup,
  // connection, etc.,  so it should be ignored  when computing stats
  _stat_update_counter = 0;
  _stat_delta_target   = RDI_STATS_DELTA_INCREMENT;

  // Create the ConsumerAdmin_i group manager
  _admin_group = new CAdminGroupMgr(_server_qos->numAdminGroups, 
				    _server_qos->numAdminThreads);

  // Create the default consumer and supplier admin objects and 
  // insert them into the corresponding hash tables
  _def_consumer_admin = new ConsumerAdmin_i(this, ifop, 0);
  _cons_admin.insert(0, _def_consumer_admin);
  _num_consadmin += 1;
  _admin_group->insert(_def_consumer_admin);
  _def_supplier_admin = new SupplierAdmin_i(this, ifop, 0);
  _supl_admin.insert(0, _def_supplier_admin);
  _num_suppadmin += 1;

  // Create the threads responsible for dispatching incoming events,
  // garbage collecting  the queue of announced events,  and pulling 
  // events from pull suppliers

  _rdi_dispatch = new EventChannelDispatch(this, 
					   _server_qos->numAdminThreads, &EventChannel_i::admin_dispatch, 
					   _server_qos->numProxyThreads, &EventChannel_i::proxy_dispatch);

  _reporter   = new EventChannelWorker(this, &EventChannel_i::periodic_report);
  RDI_AssertAllocThrowNo(_reporter, "Memory allocation failed -- Thread\n");
  RDIDbgChanLog("report thread created -- ID " << _reporter->id() << '\n');
  _reporter->start();
  _repisactive = 1;
#ifndef NO_OBJ_GC
  _objGCPeriod = _server_qos->objectGCPeriod;
  _gc_wait     = _oplockptr->add_condition();
  _gc_exit     = _oplockptr->add_condition();
  _gcollector  = new EventChannelWorker(this, &EventChannel_i::gcollect);
  RDI_AssertAllocThrowNo(_gcollector, "Memory allocation failed -- Thread\n");
  RDIDbgChanLog("channel object GC thread for channel " << _serial << " created -- ID " << _gcollector->id() << '\n');
  _gcollector->start();
  _gcisactive = 1;
  _last_use.set_curtime();
#endif
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
  _my_oref = WRAPPED_IMPL2OREF(AttN::EventChannel, this);
}

EventChannel_i::~EventChannel_i()
{
  // RDIDbgForceLog("XXX_REMOVE  ~EventChannel_i called for chan " << _serial << '\n');
  RDI_OPLOCK_DESTROY_CHECK("EventChannel_i");
}

#undef WHATFN
#define WHATFN "EventChannel_i::for_consumers"
CosEvCA::ConsumerAdmin_ptr 
EventChannel_i::for_consumers( WRAPPED_IMPLARG_VOID )
{ 
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosEvCA::ConsumerAdmin_ptr res = WRAPPED_IMPL2OREF(CosEvCA::ConsumerAdmin, _def_consumer_admin);
  return res;
}

#undef WHATFN
#define WHATFN "EventChannel_i::for_suppliers"
CosEvCA::SupplierAdmin_ptr 
EventChannel_i::for_suppliers( WRAPPED_IMPLARG_VOID )
{ 
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosEvCA::SupplierAdmin_ptr res = WRAPPED_IMPL2OREF(CosEvCA::SupplierAdmin, _def_supplier_admin); 
  return res;
}

#undef WHATFN
#define WHATFN "EventChannel_i::MyFactory"
CosNA::EventChannelFactory_ptr 
EventChannel_i::MyFactory( WRAPPED_IMPLARG_VOID )
{ 
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNA::EventChannelFactory_ptr res = WRAPPED_IMPL2OREF(CosNA::EventChannelFactory, _my_channel_factory); 
  return res;
}

#undef WHATFN
#define WHATFN "EventChannel_i::default_consumer_admin"
CosNA::ConsumerAdmin_ptr 
EventChannel_i::default_consumer_admin( WRAPPED_IMPLARG_VOID )
{ 
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNA::ConsumerAdmin_ptr res = WRAPPED_IMPL2OREF(CosNA::ConsumerAdmin, _def_consumer_admin); 
  return res;
}

#undef WHATFN
#define WHATFN "EventChannel_i::default_supplier_admin"
CosNA::SupplierAdmin_ptr 
EventChannel_i::default_supplier_admin( WRAPPED_IMPLARG_VOID )
{ 
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNA::SupplierAdmin_ptr res = WRAPPED_IMPL2OREF(CosNA::SupplierAdmin, _def_supplier_admin); 
  return res;
}

#undef WHATFN
#define WHATFN "EventChannel_i::default_filter_factory"
CosNF::FilterFactory_ptr 
EventChannel_i::default_filter_factory( WRAPPED_IMPLARG_VOID )
{ 
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosNF::FilterFactory_ptr res = WRAPPED_IMPL2OREF(CosNF::FilterFactory, _def_filter_factory); 
  return res;
}

#undef WHATFN
#define WHATFN "EventChannel_i::new_for_consumers"
CosNA::ConsumerAdmin_ptr 
EventChannel_i::new_for_consumers(CosNA::InterFilterGroupOperator op, 
				  CosNA::AdminID& id WRAPPED_IMPLARG)
{ 
  ConsumerAdmin_i* adm = 0;
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  id = _admin_serial;
  RDIDbgChanLog("Creating ConsumerAdmin -- Channel " << _serial << '\n');
  if ( ! (adm = new ConsumerAdmin_i(this, op, id)) ) {
    RDIDbgChanLog("\tfailed to create ConsumerAdmin object\n");
    return CosNA::ConsumerAdmin::_nil();
  }
  if ( _cons_admin.insert(id, adm) ) {
    RDIDbgChanLog("\tfailed to register created ConsumerAdmin object\n");
    adm->disconnect_clients_and_dispose(1); // fast destroy
    return CosNA::ConsumerAdmin::_nil();
  }
  _admin_group->insert(adm);
  _admin_serial  += 1;
  _num_consadmin += 1;
  RDIDbgChanLog("\tcreated ConsumerAdmin [" << id << "] -- Channel " << _serial << '\n');
  return WRAPPED_IMPL2OREF(CosNA::ConsumerAdmin, adm);
}

#undef WHATFN
#define WHATFN "EventChannel_i::new_for_suppliers"
CosNA::SupplierAdmin_ptr 
EventChannel_i::new_for_suppliers(CosNA::InterFilterGroupOperator op, 
				  CosNA::AdminID& id WRAPPED_IMPLARG)
{ 
  SupplierAdmin_i* adm = 0;
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  id = _admin_serial;
  if ( !(adm=new SupplierAdmin_i(this,op,id)) || _supl_admin.insert(id,adm) ) {
    if ( adm ) {
      adm->disconnect_clients_and_dispose(1); // fast destroy
    }
    return CosNA::SupplierAdmin::_nil();
  }
  _admin_serial  += 1;
  _num_suppadmin += 1;
  RDIDbgChanLog("Created SupplierAdmin [" << id << "] -- Channel " << _serial << '\n');
  return WRAPPED_IMPL2OREF(CosNA::SupplierAdmin, adm);
}

#undef WHATFN
#define WHATFN "EventChannel_i::get_consumeradmin"
CosNA::ConsumerAdmin_ptr 
EventChannel_i::get_consumeradmin(CosNA::AdminID id WRAPPED_IMPLARG)
{ 
  RDI_LocksHeld held = { 0 };
  ConsumerAdmin_i* admin = 0;
  if ( id == (CORBA::Long)-999 ) {  // XXX HACK
    RDIRptChanStatsLog("\nXXX dump_stats hack\n\n");
    dump_stats(held, 1);
    return CosNA::ConsumerAdmin::_nil();
  }
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  if ( _cons_admin.lookup(id, admin) ) {
    return WRAPPED_IMPL2OREF(CosNA::ConsumerAdmin, admin);
  }
  throw CosNA::AdminNotFound();
}

#undef WHATFN
#define WHATFN "EventChannel_i::get_supplieradmin"
CosNA::SupplierAdmin_ptr 
EventChannel_i::get_supplieradmin(CosNA::AdminID id WRAPPED_IMPLARG)
{ 
  SupplierAdmin_i* admin = 0;
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  if ( _supl_admin.lookup(id, admin) ) {
    return WRAPPED_IMPL2OREF(CosNA::SupplierAdmin, admin);
  }
  throw CosNA::AdminNotFound();
}

#undef WHATFN
#define WHATFN "EventChannel_i::get_all_consumeradmins"
CosNA::AdminIDSeq*
EventChannel_i::get_all_consumeradmins( WRAPPED_IMPLARG_VOID )
{ 
  RDI_HashCursor<CosNA::AdminID, ConsumerAdmin_i *> c;
  unsigned int idx = 0;
  CosNA::AdminIDSeq* aseq = new CosNA::AdminIDSeq;
  RDI_AssertAllocThrowNo(aseq, "Alloc failed for CosNA::AdminIDSeq\n");
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  aseq->length(_cons_admin.length());
  for (c = _cons_admin.cursor(); c.is_valid(); ++c) {
    (*aseq)[idx++] = c.key();
  }
  return aseq;
}

#undef WHATFN
#define WHATFN "EventChannel_i::get_all_supplieradmins"
CosNA::AdminIDSeq*
EventChannel_i::get_all_supplieradmins( WRAPPED_IMPLARG_VOID )
{ 
  RDI_HashCursor<CosNA::AdminID, SupplierAdmin_i *> c;
  unsigned int idx = 0;
  CosNA::AdminIDSeq* aseq = new CosNA::AdminIDSeq;
  RDI_AssertAllocThrowNo(aseq, "Alloc failed for CosNA::AdminIDSeq\n");
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  aseq->length(_supl_admin.length());
  for (c = _supl_admin.cursor(); c.is_valid(); ++c) {
    (*aseq)[idx++] = c.key();
  }
  return aseq;
}

#undef WHATFN
#define WHATFN "EventChannel_i::get_qos"
CosN::QoSProperties*
EventChannel_i::get_qos( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosN::QoSProperties* res = _qosprop->get_qos(RDI_ECHANNEL);
  return res;
}

#undef WHATFN
#define WHATFN "EventChannel_i::set_qos"
void
EventChannel_i::set_qos(const CosN::QoSProperties& r_qos  WRAPPED_IMPLARG )
{
  if (r_qos.length() == 0) return;
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(channel_lock, held.channel, WHATFN);
  if (!held.channel) { RDI_THROW_INV_OBJREF; }
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosN::PropertyErrorSeq eseq;
  CosN::NamedPropertyRangeSeq rseq;
  CORBA::Boolean  subobjs = 1;	// Channel, once created, always has default admins

  { // introduce qos lock scope
    TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
    if (! RDI_NotifQoS::validate(r_qos, *_qosprop, RDI_ECHANNEL, eseq, rseq, subobjs)) {
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
    _events->qos_changed(_server_qos->queueGCPeriod,
			 _admin_qos.maxQueueLength,
			 _admin_qos.rejectNewEvents,
			 _qosprop->discardPolicy());
    // let admins know about the change
    RDI_HashCursor<CosNA::AdminID, ConsumerAdmin_i *> ccur;
    for (ccur = _cons_admin.cursor(); ccur.is_valid(); ccur++) {
      ccur.val()->_qos_changed(held);
    }
  } // end qos lock scope
}

#undef WHATFN
#define WHATFN "EventChannel_i::validate_qos"
void
EventChannel_i::validate_qos(const CosN::QoSProperties& r_qos,
			     CosN_NamedPropertyRangeSeq_outarg rseq
			     WRAPPED_IMPLARG )
{
  rseq = new CosNotification::NamedPropertyRangeSeq();
  RDI_AssertAllocThrowNo((CosNotification::NamedPropertyRangeSeq*)rseq,
			 "Memory allocation failed - NamedPropertyRangeSeq\n");

  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosN::PropertyErrorSeq eseq;
  CORBA::Boolean  subobjs = 1;	// Channel, once created, always has default admins
  { // introduce qos lock scope
    TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
    if (! RDI_NotifQoS::validate(r_qos, *_qosprop, RDI_ECHANNEL, eseq, *rseq, subobjs)) {
      throw CosN::UnsupportedQoS(eseq);
    }
  } // end qos lock scope
}

#undef WHATFN
#define WHATFN "EventChannel_i::get_admin"
CosN::AdminProperties* 
EventChannel_i::get_admin( WRAPPED_IMPLARG_VOID )
{
  CosN::AdminProperties* res;
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  { // introduce qos lock scope
    TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
    res = _admin_qos.to_admin();
  } // end qos lock scope
  return res;
}

#undef WHATFN
#define WHATFN "EventChannel_i::set_admin"
void
EventChannel_i::set_admin(const CosN::AdminProperties& qos  WRAPPED_IMPLARG )
{
  if (qos.length() == 0) return;
  CosN::PropertyErrorSeq error;
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  RDIstrstream str;
  { // introduce qos lock scope
    TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
    if ( ! _admin_qos.validate(str, qos, error) ) {
      if (str.len() > 0) {
	RDIRptForceLog(str.buf());
      }
      throw CosN::UnsupportedAdmin(error);
    }
    _admin_qos.from_admin(qos);
    if (RDIRptAdminQoS) {
      RDIRptLogger(l, RDIRptAdminQoS_nm);
      l.str << _my_name << ": AdminQoS param(s) modified as follows\n";
      for (unsigned int i = 0; i < qos.length(); i++) {
	l.str << "  " << qos[i].name << " set to "; RDI_pp_any(l.str, qos[i].value); l.str << '\n';
      }
      l.str << '\n';
    }
    _events->qos_changed(_server_qos->queueGCPeriod,
			 _admin_qos.maxQueueLength,
			 _admin_qos.rejectNewEvents,
			 _qosprop->discardPolicy());
  } // end qos lock scope
}

#undef WHATFN
#define WHATFN "EventChannel_i::obtain_offered_types"
CosN::EventTypeSeq*
EventChannel_i::obtain_offered_types( WRAPPED_IMPLARG_VOID )
{
  CosN::EventTypeSeq* etypeseq = new CosN::EventTypeSeq;
  RDI_AssertAllocThrowNo(etypeseq, "Memory allocation failed -- EventTypeSeq\n");

  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  RDI_HashCursor<CosN::EventType, CORBA::ULong> curs;
  CORBA::ULong    indx = 0;
  etypeseq->length(_evtypes.length());
  for ( indx = 0, curs = _evtypes.cursor(); curs.is_valid(); ++indx, ++curs ) {
    (*etypeseq)[indx] = curs.key();
  }
  return etypeseq;
}

#undef WHATFN
#define WHATFN "EventChannel_i::obtain_subscription_types"
CosN::EventTypeSeq*
EventChannel_i::obtain_subscription_types( WRAPPED_IMPLARG_VOID )
{ 
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  CosN::EventTypeSeq* res = _type_map->obtain_subscription_types(); 
  return res;
}

#undef WHATFN
#define WHATFN "EventChannel_i::obtain_stats"
AttN::ChannelStats
EventChannel_i::obtain_stats( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  AttN::ChannelStats stats;
  stats.num_announcements = 0;
  stats.num_notifications = 0;
  {
    TW_SCOPE_LOCK(_chan_stats_lock, _stats_lock, "chan_stats", WHATFN);
    for (unsigned int id = 0; id < RDI_TH_ARRAY_SZ; id++) {
      TW_SCOPE_LOCK(_thread_stats_lock, _thread_stats[id]._lock, "thread_stats", WHATFN);
      stats.num_announcements += _thread_stats[id]._num_announcements;
      stats.num_notifications += _thread_stats[id]._num_notifications;
    }
  }
  return stats;
}

#undef WHATFN
#define WHATFN "EventChannel_i::destroy"
void
EventChannel_i::destroy( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(channel_lock, held.channel, WHATFN);
  if (!held.channel) { RDI_THROW_INV_OBJREF; }
  if (_shutmedown) {
    RDIDbgChanLog("EventChannel_i::destroy: channel " << _serial << " destroyed more than once\n");
    return;
  }
  _shutmedown = 1; // acts as guard -- only one thread executes the following
  RDIDbgChanLog("Event Channel " << _serial << " is being destroyed\n");

#ifndef NO_OBJ_GC
  // step 1
  RDIDbgChanLog("** [step 1a] signal object GC thread for channel " << _serial << " and wait for exit\n");
  while ( _gcisactive ) {
    _gc_wait->broadcast();
    RDI_OPLOCK_ALTCV_WAIT((*_gc_exit), WHATFN);
  }
  // no longer need these condition vars
  delete _gc_wait;   _gc_wait = 0;
  delete _gc_exit;   _gc_exit = 0;
#else
  RDIDbgChanLog("** [step 1a] skipped - no object GC thread\n");
#endif
  RDIDbgChanLog("** [step 1b] signal report thread for channel " << _serial << " and wait for exit\n");
  while ( _repisactive ) {
    _rep_wait->broadcast();
    RDI_OPLOCK_ALTCV_WAIT((*_rep_exit), WHATFN);
  }
  _my_channel_factory->remove_channel(_serial);
  // no longer need these condition vars
  delete _rep_wait;  _rep_wait = 0;
  delete _rep_exit;  _rep_exit = 0;

  // step 2
  RDIDbgChanLog("** [step 2] call set_finished on event queue, which waits for any\n");
  RDIDbgChanLog("     threads blocked waiting for an event to unblock with a NULL event\n");
  if ( _events ) _events->set_finished(1);

  // step 3
  RDIDbgChanLog("** [step 3] destroy all the thread pools for channel " << _serial << '\n');
  // We have to notify any blocked threads waiting for proxy-level
  // filter evalaution about the shut down
  _proxy_empty.broadcast();
  RDIDbgChanLog("** [step 3-a] destroy filter evaluation thread pools for channel " << _serial << '\n');
  if ( _rdi_dispatch ) delete _rdi_dispatch; _rdi_dispatch = 0;
  RDIDbgChanLog("** [step 3-b] destroy event push thread pool for channel " << _serial << '\n');
  if ( _push_consumer ) delete _push_consumer; _push_consumer = 0;
  RDIDbgChanLog("** [step 3-c] destroy event pull thread pool for channel " << _serial << '\n');
  if ( _pull_supplier ) delete _pull_supplier; _pull_supplier = 0;
  RDIDbgChanLog("** [step 3-d] destroy offer_change thread pool for channel " << _serial << '\n');
  if ( _ochange_pool ) delete _ochange_pool; _ochange_pool = 0;
  RDIDbgChanLog("** [step 3-e] destroy subscription_change thread pool for channel " << _serial << '\n');
  if ( _schange_pool ) delete _schange_pool; _schange_pool = 0;

  // step 4
  RDIDbgChanLog("** [step 4] destroy administrative groups for channel " << _serial << '\n');
  if ( _admin_group ) delete _admin_group; _admin_group = 0;

  // step 5
  RDIDbgChanLog("** [step 5] Destroy all consumer and supplier objects that have been\n");
  RDIDbgChanLog("     created so far. Their destruction will disconnect any connected\n");
  RDIDbgChanLog("     clients and destroy all created proxies.\n");
  RDIDbgChanLog("** [step 5-a] destroy all SupplierAdmin objects for channel " << _serial << '\n');
  // _supl_admin and _cons_admin cannot be modified once _shutmedown is set, so
  // although the for loops below release channel lock, the cursors are safe.
  RDI_HashCursor<CosNA::AdminID, SupplierAdmin_i *> scur;
  RDI_HashCursor<CosNA::AdminID, ConsumerAdmin_i *> ccur;
  for (scur = _supl_admin.cursor(); scur.is_valid(); scur++) {
    // do not hold channel lock across this call
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.channel, WHATFN);
      scur.val()->disconnect_clients_and_dispose(0); // not a fast destroy
    } // end unlock scope
    if (!held.channel) {
      RDI_Fatal("EventChannel_i::destroy [**unexpected REACQUIRE failure**]\n");
    }
  }
  _supl_admin.clear();
  _def_supplier_admin = 0;
  RDIDbgChanLog("** [step 5-b] destroy all ConsumerAdmin objects for channel " << _serial << '\n');
  for (ccur = _cons_admin.cursor(); ccur.is_valid(); ccur++) {
    // do not hold channel lock across this call
    { // introduce unlock scope
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.channel, WHATFN);
      ccur.val()->disconnect_clients_and_dispose(0); // not a fast destroy
    } // end unlock scope
    if (!held.channel) {
      RDI_Fatal("EventChannel_i::destroy [**unexpected REACQUIRE failure**]\n");
    }
  }
  _cons_admin.clear();
  _def_consumer_admin = 0;

  // step 6
  RDIDbgChanLog("** [step 6] destroy global Event Queue for channel " << _serial << '\n');
  if ( _events ) delete _events; _events = 0;

  // step 7
  RDIDbgChanLog("** [step 7] destroy event type map object for channel " << _serial << '\n');
  if ( _type_map ) {
    delete _type_map;
    _type_map = 0;
  }
  if ( _qosprop ) {
    delete _qosprop; _qosprop  = 0;
  }

  // step 8
  RDIDbgChanLog("** [step 8] destroy thread stats objects for channel " << _serial << '\n');
  if ( _thread_stats )  delete [] _thread_stats; _thread_stats = 0;

  // done
  RDIDbgChanLog("DONE destroying Event Channel channel " << _serial << '\n');
  RDI_OPLOCK_SET_DISPOSE_INFO(channel_lock.dispose_info);
}

// -------------------------- ** Local-only ** -----------------------------------------------

#undef WHATFN
#define WHATFN "EventChannel_i::server_qos_changed"
void
EventChannel_i::server_qos_changed() 
{
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, return);
  if (_shutmedown) { return; }
  { // introduce qos lock scope
    TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
    _events->qos_changed(_server_qos->queueGCPeriod,
			 _admin_qos.maxQueueLength,
			 _admin_qos.rejectNewEvents,
			 _qosprop->discardPolicy());
    if ( _pull_supplier ) {
      _pull_supplier->set_pull_period(_server_qos->pullEventPeriod);
    }
    if (_repInterval != _server_qos->reportingInterval) {
      _rep_wait->broadcast(); // wake report thread loop
    }
#ifndef NO_OBJ_GC
    if (_objGCPeriod != _server_qos->objectGCPeriod) {
      _gc_wait->broadcast(); // wake obj gc thread loop
    }
#endif
  } // end qos lock scope
}

#undef WHATFN
#define WHATFN "EventChannel_i::unregister_1"
void
EventChannel_i::unregister(RDI_LocksHeld&   held,
			   SupplierAdmin_i* suplAdmin)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(channel_lock, held.channel, WHATFN);
  if (!held.channel) { return; }
  if (_shutmedown)   { return; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  if ( _supl_admin.exists( suplAdmin->_admin_id() ) ) {
    _supl_admin.remove( suplAdmin->_admin_id() );
    _num_suppadmin -= 1;
  } else {
    RDIDbgChanLog("Invalid SupplierAdmin_i ["<<suplAdmin->_admin_id()<<"] given\n");
  }
}

#undef WHATFN
#define WHATFN "EventChannel_i::unregister_2"
void
EventChannel_i::unregister(RDI_LocksHeld&   held,
			   ConsumerAdmin_i* consAdmin)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(channel_lock, held.channel, WHATFN);
  if (!held.channel) { return; }
  if (_shutmedown)   { return; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _admin_group->remove(consAdmin);
  if ( _cons_admin.exists( consAdmin->_admin_id() ) ) {
    _cons_admin.remove( consAdmin->_admin_id() );
    _num_consadmin -= 1;
  } else {
    RDIDbgChanLog("Invalid ConsumerAdmin_i ["<<consAdmin->_admin_id()<<"] given\n");
  }
}

// ************************************************************* //
// Increment/Decrement the number of consumer/suppliers that are //
// connected to the channel at any particular point in time      //
// ************************************************************* //

#undef WHATFN
#define WHATFN "EventChannel_i::incr_consumers"
CORBA::Boolean
EventChannel_i::incr_consumers()
{
  TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
  if (_shutmedown) { return 0; }
  if ( _admin_qos.maxConsumers && 
       ((CORBA::Long)_num_consumers >= _admin_qos.maxConsumers)) {
    return 0;
  }
  _num_consumers += 1;
  return 1;
}

#undef WHATFN
#define WHATFN "EventChannel_i::decr_consumers"
void
EventChannel_i::decr_consumers()
{
  TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
  if (_shutmedown) { return; }
  _num_consumers -= 1;
}

#undef WHATFN
#define WHATFN "EventChannel_i::incr_suppliers"
CORBA::Boolean
EventChannel_i::incr_suppliers()
{
  TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
  if (_shutmedown) { return 0; }
  if (_admin_qos.maxSuppliers && ((CORBA::Long)_num_suppliers >= _admin_qos.maxSuppliers)) {
    return 0;
  }
  _num_suppliers += 1;
  return 1;
}

#undef WHATFN
#define WHATFN "EventChannel_i::decr_suppliers"
void
EventChannel_i::decr_suppliers()
{ 
  TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
  if (_shutmedown) { return; }
  _num_suppliers -= 1;
}
// ************************************************************* //
// A new structured event is announced to the channel via one of //
// the proxies. Insert the event in the queue of events and then //
// signal the dispatcher thread, given that the queue was empty. //
// ************************************************************* //

#define RDI_NQ_SIZE_CHECK \
if (_gq_sleep_nanosecs) TW_SLEEP(0, _gq_sleep_nanosecs)

int
EventChannel_i::_new_structured_event(RDI_StructuredEvent* sevnt)
{
  if (_shutmedown) {
    return -1;
  }
  if ( _events->insert(sevnt) ) {
    return -1;
  }
  incr_num_announcements();
  RDI_NQ_SIZE_CHECK;
  return 0;
}

int
EventChannel_i::new_structured_event(RDI_StructuredEvent* sevnt)
{
  if (_new_structured_event(sevnt)) { // sevnt not inserted
    return -1;
  }
  return 0;
}

int
EventChannel_i::new_any_event(const CORBA::Any& event)
{
  RDI_StructuredEvent* sevnt = new RDI_StructuredEvent(event);
  RDI_AssertAllocThrowNo(sevnt, "Memory allocation failure -- RDI_StructuredEvent\n");
  if (_new_structured_event(sevnt)) { // svnt not inserted
    delete sevnt;
    return -1;
  }
  return 0;
}

int
EventChannel_i::new_structured_event(const CosN::StructuredEvent& evnt)
{
  RDI_StructuredEvent* sevnt = new RDI_StructuredEvent(evnt);
  RDI_AssertAllocThrowNo(sevnt, "Memory allocation failure -- RDI_StructuredEvent\n");
  if (_new_structured_event(sevnt)) { // svnt not inserted
    delete sevnt;
    return -1;
  }
  return 0;
}

// ************************************************************* //
// Update the current event type registrations for ConsumerAdmin //
// and ProxySupplier objects                                     //
// ************************************************************* //

#undef WHATFN
#define WHATFN "EventChannel_i::update_mapping_1"
CORBA::Boolean
EventChannel_i::update_mapping(RDI_LocksHeld&             held,
			       const CosN::EventTypeSeq&  added,
			       const CosN::EventTypeSeq&  deled,
			       ConsumerAdmin_i*           admin,
			       Filter_i*                  filter)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(channel_lock, held.channel, WHATFN);
  if (!held.channel) { return 0; }
  if (_shutmedown) { return 0; } // 0 means update failure
  CORBA::Boolean res = _type_map->update(held, added, deled, admin, filter);
  return res;
}

#undef WHATFN
#define WHATFN "EventChannel_i::update_mapping_2"
CORBA::Boolean
EventChannel_i::update_mapping(RDI_LocksHeld&             held,
			       const CosN::EventTypeSeq&  added,
			       const CosN::EventTypeSeq&  deled,
			       RDIProxySupplier*          proxy,
			       Filter_i*                  filter)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(channel_lock, held.channel, WHATFN);
  if (!held.channel) { return 0; }
  if (_shutmedown) { return 0; } // 0 means update failure
  CORBA::Boolean res = _type_map->update(held, added, deled, proxy, filter);
  return res;
}

// ************************************************************* //
// Update the hash table that keeps information about the event  //
// types supplied by suppliers.  When a new entry is created or  //
// an existing entry is deleted, insert the delta into the       //
// _ochange_pool, causing _offer_change msgs to be sent          //
// ************************************************************* //

#undef WHATFN
#define WHATFN "EventChannel_i::propagate_ochange"
void
EventChannel_i::propagate_ochange(RDI_LocksHeld&             held,
				  const CosN::EventTypeSeq&  added,
				  const CosN::EventTypeSeq&  deled)
{
  CORBA::ULong ix = 0, sz = 0, vl = 0;
  CosN::EventTypeSeq new_added;
  CosN::EventTypeSeq old_deled;

  {
    // introduce lock scope
    RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(channel_lock, held.channel, WHATFN);
    if (!held.channel) { return; }
    if (_shutmedown) { return; }

    new_added.length(0);
    old_deled.length(0);

    for (sz = 0, ix = 0; ix < added.length(); ix++) {
      RDIDbgChanLog("+ " << added[ix].domain_name << "::" << added[ix].type_name << '\n');
      if ( _evtypes.lookup(added[ix], vl) ) {
	vl += 1; _evtypes.replace(added[ix], vl);
      } else {
	vl  = 1; _evtypes.insert(added[ix], vl);
	new_added.length(sz + 1);
	new_added[sz].domain_name = added[ix].domain_name;
	new_added[sz++].type_name = added[ix].type_name;
      }
    }
    for (sz = 0, ix = 0; ix < deled.length(); ix++) {
      RDIDbgChanLog("- " << deled[ix].domain_name << "::" << deled[ix].type_name << '\n');
      if ( _evtypes.lookup(deled[ix], vl) ) {
	if ( vl == 1 ) {
	  _evtypes.remove(deled[ix]);
	  old_deled.length(sz + 1);
	  old_deled[sz].domain_name = deled[ix].domain_name;
	  old_deled[sz++].type_name = deled[ix].type_name;
	} else {
	  vl -= 1; _evtypes.replace(deled[ix], vl);
	}
      } else {
	RDIDbgChanLog("Invalid " << deled[ix].domain_name << "::" << deled[ix].type_name << '\n');
      }
    }

    // If any new event types were added or existing event types were
    // deleted, cause offer_change messages to be sent
    if ( new_added.length() || old_deled.length() ) {
      if (_ochange_pool) {
	_ochange_pool->insert_change(RDI_ChangeKind_OfferChange, new_added, old_deled);
      }
    }

  } // end lock scope
}

#undef WHATFN
#define WHATFN "EventChannel_i::pxy_obtain_offered_types"
CosN::EventTypeSeq* 
EventChannel_i::pxy_obtain_offered_types(RDIProxySupplier* pxy, CosNA::ObtainInfoMode mode)
{
  RDI_HashCursor<CosN::EventType, CORBA::ULong> curs;
  CosN::EventTypeSeq* etypeseq = new CosN::EventTypeSeq;
  RDI_AssertAllocThrowNo(etypeseq, "Memory allocation failed -- EventTypeSeq\n");
  CORBA::ULong    indx = 0;
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
  if ( (mode == CosNA::NONE_NOW_UPDATES_OFF) || (mode == CosNA::NONE_NOW_UPDATES_ON) ) {
    etypeseq->length(0);
  } else {
    // fill in etypeseq
    etypeseq->length(_evtypes.length());
    for ( indx = 0, curs = _evtypes.cursor(); curs.is_valid(); ++indx, ++curs ) {
      (*etypeseq)[indx] = curs.key();
    }
  }
  if ( (mode == CosNA::NONE_NOW_UPDATES_OFF) || (mode == CosNA::ALL_NOW_UPDATES_OFF) ) {
    pxy->_disable_updates();
  } else {
    pxy->_enable_updates();
  }
  return etypeseq;
}

// ************************************************************* //
// There have been some changes in the event types referenced in //
// consumers filters. Insert the delta into the _schange_pool,   //
// causing _subscription_change msgs to be sent                  //
// [This method is invoked by the RDI_TypeMap module]            //
// ************************************************************* //

#undef WHATFN
#define WHATFN "EventChannel_i::propagate_schange"
void
EventChannel_i::propagate_schange(RDI_LocksHeld&             held,
				  const CosN::EventTypeSeq&  added,
				  const CosN::EventTypeSeq&  deled)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(channel_lock, held.channel, WHATFN);
  if (!held.channel) { return; }
  if ( _shutmedown ) {
    return;
  }
  if (_schange_pool) {
    _schange_pool->insert_change(RDI_ChangeKind_SubscriptionChange, added, deled);
  }
}

#undef WHATFN
#define WHATFN "EventChannel_i::pxy_obtain_subscription_types"
CosN::EventTypeSeq*
EventChannel_i::pxy_obtain_subscription_types(RDIProxyConsumer* pxy, CosNA::ObtainInfoMode mode)
{
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  // if channel is shutting down, proxies can be treated as 'invalid'
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
  CosN::EventTypeSeq* res = _type_map->pxy_obtain_subscription_types(pxy, mode);
  return res;
}

// ************************************************************* //
// The following is executed by threads that carry out filtering //
// on behalf of ConsumerAdmin objects.                           //
// ************************************************************* //

// Since the code does not acquire _oplock, it must check
// _shutmedown at certain places to ensure it does not use
// channel state which may have become invalid due to an ongoing
// channel destroy.

#undef WHATFN
#define WHATFN "EventChannel_i::admin_dispatch"
void
EventChannel_i::admin_dispatch()
{
  RDI_LocksHeld         held  = { 0 };
  CORBA::Boolean        matched;
  CORBA::Boolean        adone = 0;
  RDI_StructuredEvent*  pevnt = 0;
  RDI_StructuredEvent*  sevnt = 0;
  ConsumerAdmin_i*      admin = 0;
  CAdminGroup*          group = 0;
  RDI_FilterState_t     fstate;
  ProxyDispatch_t       pxdis;
  unsigned int          ix, mylo = 0, myhi = 0;

  unsigned long tid = TW_ID();

  adone = _admin_group->allocate_range(mylo, myhi);
  RDI_Assert(adone, "Failed to allocated group range to work on\n");
  RDIDbgChanLog("Thread " << tid << " L: " << mylo << " H: " << myhi << '\n');

  while ( 1 ) {
    // Wait until an event becomes available. Since we are blocked 
    // waiting for an event, if we get a NULL event back we should
    // be in the process of terminating execution.  
    sevnt = 0;
    if (_shutmedown) {
      RDIDbgChanLog("   - ADispatch thread " << tid << " for channel " << _serial << " exits\n");
      goto admin_dispatch_exit;
    }
    sevnt = _events->next_event(pevnt, 1);
    if (_shutmedown) {
      RDIDbgChanLog("   - ADispatch thread " << tid << " for channel " << _serial << " exits\n");
      goto admin_dispatch_exit;
    }
    if (sevnt == (RDI_StructuredEvent *)0) {
      RDI_Fatal("Internal error -- NULL event\n");
    }
    sevnt->set_state(RDI_StructuredEvent::DISPATCHED);
    pevnt = sevnt;

    for ( ix = mylo; ix <= myhi; ix++ ) {
      if (_shutmedown) {
	RDIDbgChanLog("   - ADispatch thread " << tid << " for channel " << _serial << " exits\n");
	goto admin_dispatch_exit;
      }
      group = (*_admin_group)[ix];
      while (1) {
	if (_shutmedown) {
	  RDIDbgChanLog("   - ADispatch thread " << tid << " for channel " << _serial << " exits\n");
	  goto admin_dispatch_exit;
	}
	TW_YIELD();
	{ // introduce typemap read lock scope
	  // need to acquire typemap lock before acquiring admin lock
	  RDI_TYPEMAP_READ_SCOPE_LOCK_TRACK(typemap_lock, held.typemap, _type_map, WHATFN);

	  { // introduce admin group lock scope
	    RDI_SAFELIST_SCOPE_LOCK(admin_group_lock, group, "cadmingroup", WHATFN);
	    // lock acquire may have taken some time, check _shutmedown again
	    if (_shutmedown) {
	      RDIDbgChanLog("   - ADispatch thread " << tid << " for channel " << _serial << " exits\n");
	      goto admin_dispatch_exit;
	    }
	    // get next admin from group
	    if (!(admin = group->iter_next())) {
	      break; // on to next admin group
	    }

	    { // introduce admin lock scope
	      RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(admin_lock, held.cadmin, admin, WHATFN);
	      if (!held.cadmin) {
		RDIDbgForceLog("** Internal error: Unexpected failure to grab admin lock\n");
		continue; // on to next admin
	      }
	      if ( admin->NumProxies() == 0 ) {
		continue; // on to next admin
	      }

	      // Release lock on admin group here; we do not need it now that we have safely obtained
	      // AND locked the next admin for the group
	      RDI_SAFELIST_EARLY_RELEASE(admin_group_lock);

	      // Dispatch the event to all proxies based on the CORBA Event
	      // Service -- no filtering is performed in this case
	      try {
		admin->dispatch_event(sevnt);
	      } catch (...) {
		RDI_Fatal("** [XXX_REMOVE try] Internal error: Unexpected exception raised by admin->dispatch_event(held, sevnt)\n");
	      }

	      try {
		matched = this->match_event(admin, sevnt, fstate);
	      } catch (...) {
		RDI_Fatal("** [XXX_REMOVE try] Internal error: Unexpected exception raised by this->match_event(held, admin, sevnt, fstate)\n");
		matched = 0;
	      }
	      if (matched) {
		if (_server_qos->numProxyThreads) {
		  goto use_proxy_threads;
		}
		// No proxy threads, so this thread dispatches event to all
		// proxies of this ConsumerAdmin object; we do not need
		// to increment the reference counter of the event here
		// since it is <> 1 due to the 'next_event()' semantics
		try {
		  admin->dispatch_event(sevnt, fstate, _type_map);
		} catch (...) {
		  RDI_Fatal("** [XXX_REMOVE try] Internal error: Unexpected exception raised by admin->dispatch_event(held, sevnt, fstate, _type_map)\n");
		}
	      }
	    } // end admin lock scope
	  } // end admin group lock scope [lock was released early, above]
	} // end typemap read lock scope

	continue; // on to next admin

      use_proxy_threads:
	// matched at admin level and there are proxy threads, so queue a pxdis
	// note that admin lock is no longer held
	if (_shutmedown) {
	  RDIDbgChanLog("   - ADispatch thread " << tid << " for channel " << _serial << " exits\n");
	  goto admin_dispatch_exit;
	}
	pxdis._event = sevnt;
	pxdis._admin = admin;
	pxdis._state = fstate;
	RDI_SEVENT_INCR_REF_COUNTER(sevnt, WHATFN);
	{ // introduce proxy_lock lock scope
	  TW_SCOPE_LOCK(chan_proxy_lock, _proxy_lock, "chan_proxy_lock", WHATFN);
	  _proxy_events.insert_tail(pxdis);
	  _proxy_empty.broadcast();
	} // end proxy_lock lock scope

	// (on to next admin)

      } // end admin loop
    } // end admin group loop 
  } // end outer loop

 admin_dispatch_exit:
  TW_EXIT();
}

// ************************************************************* //
// The following is executed by threads that carry out filtering //
// on behalf of ProxySupplier objects.                           //
// ************************************************************* //

#undef WHATFN
#define WHATFN "EventChannel_i::proxy_dispatch"
void
EventChannel_i::proxy_dispatch()
{
  ProxyDispatch_t pxdis;
  RDI_LocksHeld   held  = { 0 };
  unsigned int    numev = 0;
  unsigned long   tid   = TW_ID();

  while ( 1 ) {
    { // introduce proxy_lock lock scope
      TW_SCOPE_LOCK(chan_proxy_lock, _proxy_lock, "chan_proxy_lock", WHATFN);
      while ( ! _shutmedown && ! _proxy_events.length() ) {
	numev = 0;
	TW_CV_WAIT(_proxy_empty, _proxy_lock, "chan_proxy_lock", WHATFN);
      }
      if ( _shutmedown ) { // Channel is being destroyed
	RDIDbgChanLog("   - PDispatch thread " << tid << " for channel " << _serial << " exits\n");
	goto proxy_dispatch_exit;
      }
      pxdis = _proxy_events.get_head();
      _proxy_events.remove_head();

      // Before releasing the lock on the proxy queue,  we need to 
      // grab the lock on the admin to prevent another thread from
      // dispatching an event to the same admin out of order......

      { // introduce admin lock scope 
	RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(admin_lock, held.cadmin, pxdis._admin, WHATFN);
	if (!held.cadmin) {
	  RDIDbgForceLog("** Internal error: Unexpected failure to grab admin lock\n");
	} else {
	  // early release of lock on proxy queue
	  TW_EARLY_RELEASE(chan_proxy_lock);
	  pxdis._admin->dispatch_event(pxdis._event, pxdis._state, _type_map);
	}
      } // end admin lock scope
    } // end proxy_lock lock scope
    // Decrement reference counter -- was incremented when event
    // was inserted into the proxy queue
    RDI_SEVENT_DECR_REF_COUNTER(pxdis._event, WHATFN);
    pxdis._event = 0; pxdis._admin = 0;
    if ( ++numev >= 10 ) {
      numev = 0;
      TW_YIELD();
    }
  }

 proxy_dispatch_exit:
  TW_EXIT();
}

// ************************************************************* //
// Does one of the filters, if any,  of a ConsumerAdmin_i object //
// match a structured event having the provided type?            //
//								 //
// NOTE: typemap --> cadmin locks acquired/released              //
//       by the caller of this method                            //
// ************************************************************* //

#undef WHATFN
#define WHATFN "EventChannel_i::match_event"
CORBA::Boolean
EventChannel_i::match_event(ConsumerAdmin_i*      admin,
			    RDI_StructuredEvent*  event,
			    RDI_FilterState_t&    fstate)
{
  RDI_TypeMap::FList_t flist;
  RDI_TypeMap::FNode_t* fnode = 0;

  if ( ! admin->has_filters() ) {	// No filters -- success
    fstate = NoFilters;
    return  1;
  }

  const char* dname = event->get_domain_name();
  const char* tname = event->get_type_name();

  _type_map->lookup(dname, tname, admin, flist);

  if ( ! flist._star_star && ! flist._domn_star && 
       ! flist._star_type && ! flist._domn_type ) {
    if ( admin->_admin_operator() == CosNA::OR_OP ) {
      fstate = OrFailed;
      return 1;
    }
    return 0;
  }

  // When consumers use 'subscription_change()' on an admin to register 
  // interest in specific event types, we have 'has_filters()' return true
  // so that we execute the filter evaluation logic. However, there are no
  // filters registered and, hence, we need the '!fnode->_fltr' below...

  for ( fnode=flist._star_star; fnode; fnode=fnode->_next ) {
    if (!fnode->_fltr || fnode->_fltr->rdi_match(event, this)) goto match_found;
  }
  for ( fnode=flist._domn_star; fnode; fnode=fnode->_next ) {
    if (!fnode->_fltr || fnode->_fltr->rdi_match(event, this)) goto match_found;
  }
  for ( fnode=flist._star_type; fnode; fnode=fnode->_next ) {
    if (!fnode->_fltr || fnode->_fltr->rdi_match(event, this)) goto match_found;
  }
  for ( fnode=flist._domn_type; fnode; fnode=fnode->_next ) {
    if (!fnode->_fltr || fnode->_fltr->rdi_match(event, this)) goto match_found;
  }

  // no match
  if ( admin->_admin_operator() == CosNA::OR_OP ) {
    fstate = OrFailed;
    return 1;
  }
  return 0;

 match_found:
  fstate = (admin->_admin_operator() == CosNA::OR_OP) ? OrMatch : AndMatch;
  return 1; 
}

// ************************************************************* //

#undef WHATFN
#define WHATFN "EventChannel_i::gcollect"

#ifndef NO_OBJ_GC
void
EventChannel_i::gcollect()
{
  RDI_HashCursor<CosNA::AdminID, ConsumerAdmin_i *> ccur;
  RDI_HashCursor<CosNA::AdminID, SupplierAdmin_i *> scur;
  ConsumerAdminPtrSeq                               clst;
  SupplierAdminPtrSeq                               slst;
  unsigned int                                      i, num_cadmins, num_sadmins;
  CosNA_ConsumerAdminPtrSeq                         creflst;
  CosNA_SupplierAdminPtrSeq                         sreflst;

  RDI_LocksHeld    held = { 0 };
  ConsumerAdmin_i* cadmin = 0;
  SupplierAdmin_i* sadmin = 0;
  unsigned long    tid = TW_ID();
  int              gc_me;
  RDI_TimeT        curtime;

  gc_me = 0;
  while ( 1 ) {
    unsigned long secs, nanosecs;

    { // introduce bumped lock scope
      RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(channel_lock, held.channel, WHATFN);
      if (!held.channel) {
	RDIDbgForceLog("   - GC thread " << tid << " for channel " << _serial << " exits ABNORMALLY: ** unexpected acquire failure **\n");
	_gcisactive = 0;
	_gc_exit->broadcast();
	goto gcollect_exit;
      }
      if ( _shutmedown ) {
	RDIDbgChanLog("   - GC thread " << tid << " for channel " << _serial << " exits\n");
	_gcisactive = 0;
	_gc_exit->broadcast();
	goto gcollect_exit;
      }
      { // introduce qos lock scope
	TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
	_objGCPeriod = _server_qos->objectGCPeriod;
      } // end qos lock scope
    redo_wait:
      CORBA::ULong gcPeriod = _objGCPeriod;
      if (gcPeriod == 0) {
	gcPeriod = 60 * 60 * 24 * 365; // 1 year
      }
      TW_GET_TIME(&secs, &nanosecs, gcPeriod, 0);
      RDI_OPLOCK_ALTCV_TIMEDWAIT((*_gc_wait), secs, nanosecs, WHATFN);
      if ( _shutmedown ) {
	RDIDbgChanLog("   - GC thread " << tid << " for channel " << _serial << " exits\n");
	_gcisactive = 0;
	_gc_exit->broadcast();
	goto gcollect_exit;
      }

      CORBA::ULong deadChan, deadAdmin, deadConProxy, deadOtherProxy;
      // if we were woken due to a change in _server_qos->objectGCPeriod and the period
      // just got longer (or was set to 0), redo the wait before attempting GC
      { // introduce qos lock scope
	TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
	if (_server_qos->objectGCPeriod == 0 ||
	    _server_qos->objectGCPeriod > _objGCPeriod) {
	  _objGCPeriod = _server_qos->objectGCPeriod;
	  goto redo_wait;
	}
	deadChan       = _server_qos->deadChanInterval;
	deadAdmin      = _server_qos->deadAdminInterval;
	deadConProxy   = _server_qos->deadConProxyInterval;
	deadOtherProxy = _server_qos->deadOtherProxyInterval;
      } // end qos lock scope

      // build up temp lists of admins (use them in temp lock release scope)
      num_cadmins = _cons_admin.length();
      clst.length(num_cadmins);
      creflst.length(num_cadmins);
      for (i = 0, ccur = _cons_admin.cursor(); i < num_cadmins; ++i, ++ccur) {
	cadmin = ccur.val();
	clst[i] = cadmin;
	// bump refcount of cadmin so that its storage cannot be deallocated before we call obj_gc on it
	creflst[i] = WRAPPED_IMPL2OREF(AttN::ConsumerAdmin, cadmin);
      }
      num_sadmins = _supl_admin.length();
      slst.length(num_sadmins);
      sreflst.length(num_sadmins);
      for (i = 0, scur = _supl_admin.cursor(); i < num_sadmins; ++i, ++scur) {
	sadmin = scur.val();
	slst[i] = sadmin;
	// bump refcount of sadmin so that its storage cannot be deallocated before we call obj_gc on it
	sreflst[i] = WRAPPED_IMPL2OREF(AttN::SupplierAdmin, sadmin);
      }

      { // temp lock release scope
	RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.channel, WHATFN);

	curtime.set_curtime();
	// get the admins to do object gc
	for (i = 0; i < num_cadmins; i++) {
	  clst[i]->obj_gc(curtime, deadAdmin, deadConProxy, deadOtherProxy);
	  CORBA::release(creflst[i]); // undo the refcount bump performed above
	}
	clst.length(0);
	for (i = 0; i < num_sadmins; i++) {
	  slst[i]->obj_gc(curtime, deadAdmin, deadConProxy, deadOtherProxy);
	  CORBA::release(sreflst[i]); // undo the refcount bump performed above
	}
	clst.length(0);
      } // end temp lock release scope

      if (!held.channel) {
	RDI_Fatal("EventChannel_i::gcollect [**unexpected REACQUIRE failure**]\n");
      }
      if ( _shutmedown ) {
	RDIDbgChanLog("   - GC thread " << tid << " for channel " << _serial << " exits\n");
	_gcisactive = 0;
	_gc_exit->broadcast();
	goto gcollect_exit;
      }

      if (_serial != 0 && _num_suppadmin == 1 && _num_consadmin == 1) {
	// This is not the default channel and only has default admins;
	// check whether this channel can be gc'd
	if ( deadChan && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadChan) &&
	     (!_def_consumer_admin || _def_consumer_admin->gc_able(curtime, deadAdmin)) &&
	     (!_def_supplier_admin || _def_supplier_admin->gc_able(curtime, deadAdmin)) ) {
	  gc_me = 1;// OK, obj gc destroys this channel
	  RDIDbgChanLog("GC destroys chan " << _serial
			<< " curtime = " << curtime.fmt_local()
			<< " last_use = " << _last_use.fmt_local()
			<< "(diff = " << RDI_TIMET_DIFF_IN_SECS(_last_use, curtime)
			<< ") DeadChanInterval = " << deadChan
			<< "\n   - GC thread " << tid << " for channel " << _serial << " exits\n");
	  _gcisactive = 0;
	  _gc_exit->broadcast();
	  goto gcollect_exit;
	}
      }
    } // end lock scope
  } // end while loop

 gcollect_exit:
  if (gc_me) {
    destroy();
  }
  TW_EXIT();
}
#endif

#undef WHATFN
#define WHATFN "EventChannel_i::periodic_report"
void
EventChannel_i::periodic_report()
{
  RDI_LocksHeld held = { 0 };
  unsigned long tid = TW_ID();

  while ( 1 ) {
    unsigned long s, n;
    { // introduce bumped lock scope 
      RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(channel_lock, held.channel, WHATFN);
      if (!held.channel) {
	RDIDbgForceLog("   - Report thread " << tid << " for channel " << _serial << " exits ABNORMALLY: ** unexpected acquire failure **\n");
	_repisactive = 0;
	_rep_exit->broadcast();
	goto periodic_report_exit;
      }
      if ( _shutmedown) {
	RDIDbgChanLog("   - Report thread " << tid << " for channel " << _serial << " exits\n");
	_repisactive = 0;
	_rep_exit->broadcast();
	goto periodic_report_exit;
      }
      { // introduce qos lock scope
	TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
	_repInterval = _server_qos->reportingInterval;
      } // end qos lock scope
      CORBA::ULong rep_interval = _repInterval;
      if (rep_interval == 0) {
	rep_interval = 60 * 60 * 24 * 365; // 1 year
      }
      TW_GET_TIME(&s, &n, rep_interval, 0);
      RDI_OPLOCK_ALTCV_TIMEDWAIT((*_rep_wait), s, n, WHATFN);
      if ( _shutmedown ) {
	RDIDbgChanLog("   - Report thread " << tid << " for channel " << _serial << " exits\n");
	_repisactive = 0;
	_rep_exit->broadcast();
	goto periodic_report_exit;
      }

      // Lock is held; do 1 report
      { // introduce qos lock scope
	TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
	if (_server_qos->reportingInterval > 0) {
	  RDIRptLogger1(l);
	  l.str << "\n======================================================================"
		<< "\n  The following report generated by Report thread " << tid
		<< "\n    (last ReportingInterval = " << _repInterval << " secs)";
	  if (_repInterval != _server_qos->reportingInterval) {
	    l.str << "\n    (next ReportingInterval = " << _server_qos->reportingInterval << " secs)";
	  }
	  _rpt_stats(l.str);
	}
      } // end qos lock scope
    } // end lock scope
    TW_YIELD();
  }

 periodic_report_exit:
  TW_EXIT();
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//                         EventChannelFactory_i                         //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

EventChannelFactory_i::EventChannelFactory_i(FilterFactory_i*    ffactory,
					     const RDI_NotifQoS& defqos,
					     const RDI_AdminQoS& defadm,
					     RDI_ServerQoS*      s_qos) :
  _oplockptr(0), _ffactory(ffactory), _serial(0), _defqos(defqos), 
  _defadm(defadm), _server_qos(s_qos), _channel(RDI_CorbaULongHash, RDI_CorbaULongRank)
{
  RDI_OPLOCK_INIT("chanfact");
  _my_name.length(2);
  _my_name[0] = (const char*)"server";
  _my_name[1] = (const char*)"chanfact";
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
  _my_oref = WRAPPED_IMPL2OREF(AttN::EventChannelFactory, this);
}

EventChannelFactory_i::~EventChannelFactory_i()
{
  RDI_OPLOCK_DESTROY_CHECK("EventChannelFactory_i");
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::cleanup_and_dispose"
void
EventChannelFactory_i::cleanup_and_dispose()
{
  RDI_OPLOCK_BUMP_SCOPE_LOCK(chanfact_lock, WHATFN, return);
  if (_disposed) {
    RDIDbgFactLog("EventChannelFactory_i::cleanup_and_dispose() called more than once\n");
    return;
  }
  _disposed = 1; // acts as guard: the following is executed by only one thread 
  _channel.clear();
  RDI_OPLOCK_SET_DISPOSE_INFO(chanfact_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::create_channel"
CosNA::EventChannel_ptr
EventChannelFactory_i::create_channel(const CosN::QoSProperties&   qosP,
                                      const CosN::AdminProperties& admP,
                                      CosNA::ChannelID& myID  WRAPPED_IMPLARG)
{
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  CosN::NamedPropertyRangeSeq pran;
  CosN::PropertyErrorSeq      perr;
  CORBA::Boolean  subobjs = 0;	// Before creation, channel has no subobjs

  CosNA::EventChannel_ptr res = CosNA::EventChannel::_nil();
  EventChannel_i* chan = 0;

  // Validate the requested QoS based on the '_defqos' settings
  if ( ! RDI_NotifQoS::validate(qosP, _defqos, RDI_ECHANNEL, perr, pran, subobjs) ) {
    for (CORBA::ULong ix = 0; ix < perr.length(); ix++) {
      RDIDbgFactLog("Invalid QoS: " << (const char*)(perr[ix].name) << '\n');
    }
    throw CosN::UnsupportedQoS(perr);
  }
  // Validate the requested Admin QoS based on the '_defadm' settings
  RDIstrstream str;
  if ( ! _defadm.validate(str, admP, perr) ) {
    if (str.len() > 0) {
      RDIRptForceLog(str.buf());
    }
    for (CORBA::ULong ix = 0; ix < perr.length(); ix++) {
      RDIDbgFactLog("Invalid AdminQoS: " << (const char*)(perr[ix].name) << '\n');
    }
    throw CosN::UnsupportedAdmin(perr);
  }

  // Construct new channel using defaults
  chan = _create_channel(myID);
  if (chan != 0) {
    // Override defaults with requested notif qos settings
    try {
      chan->set_qos(qosP);
    } catch (CosN::UnsupportedQoS& eseq) {
      _channel.remove(chan->MyID());
      WRAPPED_DISPOSE_IMPL(chan);
      chan = 0;
      throw CosN::UnsupportedQoS(eseq);
    }
    // Override defaults with requested admin settings
    try {
      chan->set_admin(admP);
    } catch (CosN::UnsupportedAdmin& eseq) {
      _channel.remove(chan->MyID());
      WRAPPED_DISPOSE_IMPL(chan);
      chan = 0;
      throw CosN::UnsupportedAdmin(eseq);
    }
    res = WRAPPED_IMPL2OREF(CosNA::EventChannel, chan);
  }
  return res;
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::get_all_channels"
CosNA::ChannelIDSeq* 
EventChannelFactory_i::get_all_channels( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  unsigned int index = 0;
  CosNA::ChannelIDSeq* idseq = new CosNA::ChannelIDSeq;
  if ( idseq == (CosNA::ChannelIDSeq *) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  RDI_HashCursor<CosNA::ChannelID, EventChannel_i*> cursor;
  idseq->length( _channel.length() );
  for ( cursor = _channel.cursor(); cursor.is_valid(); index++, ++cursor ) {
    (*idseq)[index] = cursor.key();
  }
  return idseq;
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::get_event_channel"
CosNA::EventChannel_ptr 
EventChannelFactory_i::get_event_channel(CosNA::ChannelID id   WRAPPED_IMPLARG)
{
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  EventChannel_i* channel = 0;
  CORBA::Boolean found = _channel.lookup(id, channel);
  if (!found) {
    throw CosNA::ChannelNotFound();
  }
  return WRAPPED_IMPL2OREF(CosNA::EventChannel, channel);
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::get_qos"
CosN::QoSProperties*
EventChannelFactory_i::get_qos( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  CosN::QoSProperties* res = _defqos.get_qos(RDI_ECHANNEL);
  return res;
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::set_qos"
void
EventChannelFactory_i::set_qos(const CosN::QoSProperties& r_qos  WRAPPED_IMPLARG )
{
  if (r_qos.length() == 0) return;
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  CosN::PropertyErrorSeq eseq;
  CosN::NamedPropertyRangeSeq rseq;
  CORBA::Boolean  subobjs = 0;	// Before creation, channel has no subobjs

  if (! RDI_NotifQoS::validate(r_qos, _defqos, RDI_ECHANNEL, eseq, rseq, subobjs)) {
    throw CosN::UnsupportedQoS(eseq);
  }
  _defqos.set_qos(r_qos);
  if (RDIRptNotifQoS) {
    RDIRptLogger(l, RDIRptNotifQoS_nm);
    l.str << _my_name << ": Default NotifQoS param(s) modified as follows\n";
    for (unsigned int i = 0; i < r_qos.length(); i++) {
      l.str << "  " << r_qos[i].name << " set to "; RDI_pp_any(l.str, r_qos[i].value); l.str << '\n';
    }
    l.str << '\n';
  }
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::validate_qos"
void
EventChannelFactory_i::validate_qos(const CosN::QoSProperties& r_qos,
				    CosN_NamedPropertyRangeSeq_outarg rseq
				    WRAPPED_IMPLARG )
{
  rseq = new CosNotification::NamedPropertyRangeSeq();
  RDI_AssertAllocThrowNo((CosNotification::NamedPropertyRangeSeq*)rseq,
			 "Memory allocation failed - NamedPropertyRangeSeq\n");

  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, RDI_THROW_INV_OBJREF);

  CosN::PropertyErrorSeq eseq;
  CORBA::Boolean  subobjs = 0;	// Before creation, channel has no subobjs

  if (! RDI_NotifQoS::validate(r_qos, _defqos, RDI_ECHANNEL, eseq, *rseq, subobjs)) {
    throw CosN::UnsupportedQoS(eseq);
  }
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::get_admin"
CosN::AdminProperties* 
EventChannelFactory_i::get_admin( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  CosN::AdminProperties* res = _defadm.to_admin();
  return res;
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::set_admin"
void
EventChannelFactory_i::set_admin(const CosN::AdminProperties& qos  WRAPPED_IMPLARG )
{
  if (qos.length() == 0) return;
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  CosN::PropertyErrorSeq error;
  RDIstrstream str;
  if ( ! _defadm.validate(str, qos, error) ) {
    if (str.len() > 0) {
      RDIRptForceLog(str.buf());
    }
    throw CosN::UnsupportedAdmin(error);
  }
  _defadm.from_admin(qos);
  if (RDIRptAdminQoS) {
    RDIRptLogger(l, RDIRptAdminQoS_nm);
    l.str << _my_name << ": Default AdminQoS param(s) modified as follows\n";
    for (unsigned int i = 0; i < qos.length(); i++) {
      l.str << "  " << qos[i].name << " set to "; RDI_pp_any(l.str, qos[i].value); l.str << '\n';
    }
    l.str << '\n';
  }
}

// -------------------- ** Local-only Methods ** --------------------------------

// NB:oplock must be held
EventChannel_i* 
EventChannelFactory_i::_create_channel(CosNA::ChannelID& myID)
{
  CosN::QoSProperties*   qosP = 0; 
  CosN::AdminProperties* admP = 0;
  EventChannel_i* chan = 0;

  qosP = _defqos.get_qos(RDI_ECHANNEL);
  admP = _defadm.to_admin();
  myID = _serial++;
  chan = new EventChannel_i(this, _ffactory, *qosP, *admP, _server_qos, myID);
  delete qosP;
  delete admP;
  if ( chan && _channel.insert(myID, chan) ) {
    WRAPPED_DISPOSE_IMPL(chan);
    chan = 0;
  }
  return chan;
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::create_channel"
EventChannel_i* 
EventChannelFactory_i::create_channel(CosNA::ChannelID& myID)
{
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, return 0);
  EventChannel_i* chan = _create_channel(myID);
  return chan;
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::remove_channel"
void
EventChannelFactory_i::remove_channel(CosNA::ChannelID id)
{
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, return);
  _channel.remove(id);
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::server_qos_changed"
void
EventChannelFactory_i::server_qos_changed() 
{
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, return);
  RDI_HashCursor<CosNA::ChannelID, EventChannel_i*> curs;
  for ( curs = _channel.cursor(); curs.is_valid(); ++curs ) {
    curs.val()->server_qos_changed();
  }
}


#undef WHATFN
#define WHATFN "EventChannelFactory_i::out_all_stats"
void
EventChannelFactory_i::out_all_stats(RDIstrstream& str)
{
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, return);
  str << "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
  str << "Stats for All Channels\n";
  str << "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
  if (_channel.length() == 0) {
    str << "  (no channels)\n";
  } else {
    RDI_HashCursor<CosNA::ChannelID, EventChannel_i*> curs;
    for ( curs = _channel.cursor(); curs.is_valid(); ++curs ) {
      curs.val()->out_stats(str);
    }
  }
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::out_all_debug_info"
void
EventChannelFactory_i::out_all_debug_info(RDIstrstream& str)
{
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, return);
  str << "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
  str << "Debug Info for All Channels\n";
  str << "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
  if (_channel.length() == 0) {
    str << "  (no channels)\n";
  } else {
    RDI_HashCursor<CosNA::ChannelID, EventChannel_i*> curs;
    for ( curs = _channel.cursor(); curs.is_valid(); ++curs ) {
      curs.val()->out_heading(str);
      curs.val()->out_debug_info(str);
    }
  }
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::out_default_config"
void
EventChannelFactory_i::out_default_config(RDIstrstream& str)
{
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, return);
  str << "======================================================================\n";
  str << "Channel Factory Default Configuration\n";
  str << "  (these settings are the defaults for any future channel creation)\n";
  str << "======================================================================\n";
  str << "NotifQoS Properties:\n" << _defqos << '\n';
  str << "AdminQoS Properties:\n" << _defadm << '\n';
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::out_all_config"
void
EventChannelFactory_i::out_all_config(RDIstrstream& str)
{
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, return);
  str << "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
  str << "Configuration for All Channels\n";
  str << "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
  if (_channel.length() == 0) {
    str << "  (no channels)\n";
  } else {
    RDI_HashCursor<CosNA::ChannelID, EventChannel_i*> curs;
    for ( curs = _channel.cursor(); curs.is_valid(); ++curs ) {
      curs.val()->out_heading(str);
      curs.val()->out_config(str);
    }
  }
}

void
EventChannelFactory_i::cleanup_all(RDIstrstream& str, CORBA::Boolean admins, CORBA::Boolean proxies)
{
  RDIInteractive::cleanup_channels(str, _my_oref, admins, proxies);
}

////////////////////////////////////////
// Logging

RDIstrstream&
EventChannel_i::log_output(RDIstrstream& str, CORBA::Boolean show_events)
{
  out_heading(str);
  out_debug_info(str);
  return str;
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::log_output"
RDIstrstream&
EventChannelFactory_i::log_output(RDIstrstream& str)
{
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, return str);
  str << "Event Channel Factory with " << _channel.length() << " channels\n";
  RDI_HashCursor<CosNA::ChannelID, EventChannel_i*> curs;
  for ( curs = _channel.cursor(); curs.is_valid(); ++curs ) {
    curs.val()->log_output(str, 0);
  }
  return str;
}

// ************************************************************* //
//                   dump_stats                                  //
// ************************************************************* //

// unit is nanosecs
#define RDI_1_US_AS_NS        1000
#define RDI_pt01_MS_AS_NS    10000
#define RDI_pt1_MS_AS_NS    100000
#define RDI_pt25_MS_AS_NS   250000
#define RDI_pt5_MS_AS_NS    500000
#define RDI_1_MS_AS_NS     1000000
#define RDI_2_MS_AS_NS     2000000
#define RDI_4_MS_AS_NS     4000000
#define RDI_100_MS_AS_NS 100000000

#undef WHATFN
#define WHATFN "EventChannel_i::dump_stats"
void
EventChannel_i::dump_stats(RDI_LocksHeld& held, CORBA::Boolean force) 
{
  TW_COND_SCOPE_LOCK_TRACK(_chan_stats_lock, held.chan_stats, _stats_lock, "chan_stats", WHATFN);
  if (force) {
    RDIRptLogger1(l);
    _rpt_stats(l.str);
    l.flush();
  } else if (RDIRptChanStats) {
    RDIRptLogger(l, RDIRptChanStats_nm);
    _rpt_stats(l.str);
    l.flush();
  }
}

// called by dump_stats and out_stats
#undef WHATFN
#define WHATFN "EventChannel_i::_rpt_stats"
void
EventChannel_i::_rpt_stats(RDIstrstream& str)
{
  unsigned int msecs = 0;
  int thread_e = 0, thread_n = 0, thread_a = 0, thread_m = 0;
  CORBA::ULong num_e = 0, num_n = 0, num_a = 0, num_m = 0;
  CORBA::ULong nq_acm = 0, nq_ctr = 0;
  CORBA::ULong delta_e, delta_n, delta_a, delta_m;
  double avg_nq_sz = -1, avg_gq_sz = -1, avg_pq_sz = -1;

  for (unsigned int id = 0; id < RDI_TH_ARRAY_SZ; id++) {
    TW_SCOPE_LOCK(_thread_stats_lock, _thread_stats[id]._lock, "thread_stats", WHATFN);
    num_n += _thread_stats[id]._num_notifications;
    num_a += _thread_stats[id]._num_announcements;
    num_m += _thread_stats[id]._num_rdi_match;
    num_e += _thread_stats[id]._num_rvm_eval;
    nq_acm += _thread_stats[id]._qsize_acum;
    nq_ctr += _thread_stats[id]._qsize_ctr;
    _thread_stats[id]._qsize_acum = 0;
    _thread_stats[id]._qsize_ctr = 0;
    if (_thread_stats[id]._num_notifications > 0) thread_n++;
    if (_thread_stats[id]._num_announcements > 0) thread_a++;
    if (_thread_stats[id]._num_rdi_match > 0)     thread_m++;
    if (_thread_stats[id]._num_rvm_eval > 0)      thread_e++;
  }

  _performance_timer.stop();

  if ( nq_ctr ) {
    avg_nq_sz = (double)nq_acm/(double)nq_ctr;

    if ( (avg_nq_sz > 40) && (avg_nq_sz > _prev_avg_nq_sz) && 
         (_gq_sleep_nanosecs < RDI_100_MS_AS_NS)) {
      _gq_sleep_nanosecs += 
	(unsigned long) (RDI_pt01_MS_AS_NS * (avg_nq_sz - _prev_avg_nq_sz));
      if (_gq_sleep_nanosecs > RDI_100_MS_AS_NS)
	_gq_sleep_nanosecs = RDI_100_MS_AS_NS;
    } else if ( (avg_nq_sz < 10) && (avg_nq_sz <= _prev_avg_nq_sz) && 
		(_gq_sleep_nanosecs > 0) ) {
      unsigned long deltns = 
	(unsigned long) (RDI_pt01_MS_AS_NS * (_prev_avg_nq_sz - avg_nq_sz));
      if ( !deltns || deltns >= _gq_sleep_nanosecs) {
	_gq_sleep_nanosecs = 0;
      } else {
	_gq_sleep_nanosecs -= deltns;
      }
    }
    _prev_avg_nq_sz = avg_nq_sz;
  }
  if (_gq_ctr) {
    avg_gq_sz = (double)_gq_acm/(double)_gq_ctr;
  }
  if (_pq_ctr) {
    avg_pq_sz = (double)_pq_acm/(double)_pq_ctr;
  }
  _gq_acm = 0;  _gq_ctr = 0;
  _pq_acm = 0;  _pq_ctr = 0;

  delta_n = num_n - _prev_num_notifications;
  delta_a = num_a - _prev_num_announcements;
  delta_e = num_e - _prev_num_rvm_eval;
  delta_m = num_m - _prev_num_rdi_match;

  _prev_num_notifications = num_n;
  _prev_num_announcements = num_a;
  _prev_num_rvm_eval      = num_e;
  _prev_num_rdi_match     = num_m;

  msecs = _performance_timer.millisecs();
  if (_cum_msecs == 0) {
    _cum_msecs = msecs;
    _second_delta = 1;
  } else if (_second_delta) {
    _cum_msecs = msecs * 2;
    _second_delta = 0;
  } else {
    _cum_msecs += msecs;
  }

  str << "\n======================================================================";
  str << "\n  omniNotify delta stats report for Channel " << _serial;
  str << "\n======================================================================";
  str << "\nqueue sizes: global " << avg_gq_sz <<
    " proxy " << avg_pq_sz << " notif " << avg_nq_sz << '\n';
  str << "\tDELTA: #announ " << delta_a << " #notifs " << delta_n << 
    " #rdi_match " << delta_m << " #rvm_evals " << delta_e << '\n' <<
    "\t       time " << msecs << " msecs : " << 
    (double)delta_a/((double) msecs/1000) << " a/sec " << 
    (double)delta_n/((double) msecs/1000) << " n/sec " << 
    (double)(delta_a+delta_n)/((double) msecs/1000) << " a+n/sec" <<
    '\n';
  str << "\tCUM:   #announ " << num_a << " " << "#notifs " << num_n <<
    " #rdi_match " << num_m << " " << "#rvm_evals " << num_e << 
    '\n' << "\t       time " << (unsigned long)_cum_msecs <<
    " msecs : " <<
    (double)num_a/((double) _cum_msecs/1000) << " a/sec " << 
    (double)num_n/((double) _cum_msecs/1000) << " n/sec  " << 
    (double)(num_a+num_n)/((double) _cum_msecs/1000) << " a+n/sec" <<
    '\n';
  str << "\nNUM_THREAD_ID_BUCKETS " << "for_announ " << thread_a << " " << "for_notif " << thread_n << " " << "rdi_match " << thread_m << " " << "rvm_evals " << thread_e << '\n';
  str << "SAMPLED_QUEUE_SIZES" << " glob_arriv_Q " << avg_gq_sz << " proxy_work_Q " << avg_pq_sz << " avg_notif_Q  " << avg_nq_sz << "\n\n";

  _performance_timer.start();
}

////////////////////////////////////////
// Interactive

#undef WHATFN
#define WHATFN "EventChannel_i::my_name"
AttN::NameSeq*
EventChannel_i::my_name( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
  AttN::NameSeq* res = new AttN::NameSeq(_my_name);
  if ( res == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  return res;
}

#undef WHATFN
#define WHATFN "EventChannel_i::child_names"
AttN::NameSeq*
EventChannel_i::child_names( WRAPPED_IMPLARG_VOID )
{
  char buf[20];
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_shutmedown) { RDI_THROW_INV_OBJREF; }
  AttN::NameSeq* names = new AttN::NameSeq;
  if ( names == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  names->length(_cons_admin.length() + _supl_admin.length());
  RDI_HashCursor<CosNA::AdminID, ConsumerAdmin_i *> c;
  unsigned int idx = 0;
  for (c = _cons_admin.cursor(); c.is_valid(); ++c) {
    sprintf(buf, "cadmin%ld", (long)c.key());
    (*names)[idx++] = (const char*)buf;
  }
  RDI_HashCursor<CosNA::AdminID, SupplierAdmin_i *> s;
  for (s = _supl_admin.cursor(); s.is_valid(); ++s) {
    sprintf(buf, "sadmin%ld", (long)s.key());
    (*names)[idx++] = (const char*)buf;
  }
  return names;
}

void
EventChannel_i::out_heading(RDIstrstream& str)
{
  str << "======================================================================\n";
  str << "Event Channel " << _my_name << '\n';
  str << "======================================================================\n";
}

#undef WHATFN
#define WHATFN "EventChannel_i::out_stats"
void
EventChannel_i::out_stats(RDIstrstream& str)
{
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, return);
  if (_shutmedown) {
    str << "[channel is shutting down]\n";
    return;
  }
  _rpt_stats(str);
}

#undef WHATFN
#define WHATFN "EventChannel_i::out_debug_info"
void
EventChannel_i::out_debug_info(RDIstrstream& str, CORBA::Boolean show_events)
{
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, return);
  if (_shutmedown) {
    str << "[channel is shutting down]\n";
    return;
  }
  RDI_HashCursor<CosNA::AdminID, SupplierAdmin_i *> suplcur;
  RDI_HashCursor<CosNA::AdminID, ConsumerAdmin_i *> conscur;
  { // introduce qos lock scope
    TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
    str << *(_qosprop) << "\n\n";
    str << _admin_qos << '\n';
  } // end qos lock scope
  _events->out_debug_info(str, show_events);
  str << '\n' << *(_admin_group) << '\n';
  for ( suplcur = _supl_admin.cursor(); suplcur.is_valid(); ++suplcur) {
    str << *suplcur.val() << '\n';
  }
  for ( conscur = _cons_admin.cursor(); conscur.is_valid(); ++conscur) {
    str << *conscur.val() << '\n';
  }
  str << *(_type_map);
}

#undef WHATFN
#define WHATFN "EventChannel_i::out_config"
void
EventChannel_i::out_config(RDIstrstream& str)
{
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, return);
  if (_shutmedown) {
    str << "[channel is shutting down]\n";
    return;
  }
  { // introduce qos lock scope
    TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
    str << "NotifQoS Properties:\n" << *(_qosprop) << '\n';
    str << "AdminQoS Properties:\n" << _admin_qos << '\n';
  } // end qos lock scope
}

#undef WHATFN
#define WHATFN "EventChannel_i::out_info_filters"
void
EventChannel_i::out_info_filters(RDIstrstream& str, CORBA::Boolean admins, CORBA::Boolean proxies)
{
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, return);
  if (_shutmedown) {
    str << "[channel is shutting down]\n";
    return;
  }
  str << "======================================================================\n";
  if (admins && proxies) {str << "All ";} else if (admins) {str << "Admin ";} else {str << "Proxy ";}
  str << "Filters attached to Event Channel " << _my_name << '\n';
  str << "======================================================================\n";
  RDI_HashCursor<CosNA::AdminID, SupplierAdmin_i *> suplcur;
  RDI_HashCursor<CosNA::AdminID, ConsumerAdmin_i *> conscur;
  for ( suplcur = _supl_admin.cursor(); suplcur.is_valid(); ++suplcur) {
    suplcur.val()->out_info_filters(str, admins, proxies);
  }
  for ( conscur = _cons_admin.cursor(); conscur.is_valid(); ++conscur) {
    conscur.val()->out_info_filters(str, admins, proxies);
  }
}

// _children: fills in 'ren'
// NB: REQUIRES oplock is held, _shutmedown has been checked
void
EventChannel_i::_children(AttN::IactSeq& ren, CORBA::Boolean only_cleanup_candidates)
{
  // All admins are cleanup candidates
  ren.length(_supl_admin.length() + _cons_admin.length());
  RDI_HashCursor<CosNA::AdminID, SupplierAdmin_i *> suplcur;
  RDI_HashCursor<CosNA::AdminID, ConsumerAdmin_i *> conscur;
  unsigned int idx = 0;
  for ( suplcur = _supl_admin.cursor(); suplcur.is_valid(); ++suplcur) {
    ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, suplcur.val());
  }
  for ( conscur = _cons_admin.cursor(); conscur.is_valid(); ++conscur) {
    ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, conscur.val());
  }
}

#undef WHATFN
#define WHATFN "EventChannel_i::children"
AttN::IactSeq*
EventChannel_i::children(CORBA::Boolean only_cleanup_candidates  WRAPPED_IMPLARG )
{
  // All admins are cleanup candidates
  RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::IactSeq* ren = new AttN::IactSeq;
  if ( ren == (AttN::IactSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  if (_shutmedown) {
    ren->length(0);
    return ren;
  }
  _children(*ren, only_cleanup_candidates);
  return ren;
}


#undef WHATFN
#define WHATFN "EventChannel_i::do_set_command"
CORBA::Boolean
EventChannel_i::do_set_command(RDIstrstream& str, RDIParseCmd& p)
{
  CosN::QoSProperties    n_qos;
  CosN::AdminProperties  a_qos;
  AttN::ServerProperties s_qos;
  CORBA::Boolean success = 1, n_success = 0, a_success = 0;
  success = RDI_AllQoS::parse_set_command(str, p, RDI_ECHANNEL, n_qos, a_qos, s_qos);
  if (success && n_qos.length() > 0) {
    try {
      set_qos(n_qos);
      n_success = 1;
    } catch (CosN::UnsupportedQoS& e) {
      str << "\nThe following NotifQoS Property Settings are invalid:\n";
      RDI_describe_prop_errors(str, e.qos_err);
      str << '\n';
      success = 0;
    } catch (CORBA::INV_OBJREF) { throw; }
    if (n_success) {
      str << '\n';
      for (unsigned int i = 0; i < n_qos.length(); i++) {
	str << n_qos[i].name << " set to "; RDI_pp_any(str, n_qos[i].value); str << '\n';
      }
    }
  }
  if (success && a_qos.length() > 0) {
    CORBA::Boolean aqos_valid;
    CosN::PropertyErrorSeq eseq;
    { // introduce lock scope
      RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, RDI_THROW_INV_OBJREF);
      if (_shutmedown) { RDI_THROW_INV_OBJREF; }
      { // introduce qos lock scope
	TW_SCOPE_LOCK(chan_qos_lock, _qos_lock, "chan_qos", WHATFN);
	aqos_valid = _admin_qos.validate(str, a_qos, eseq);
      } // end qos lock scope
    } // end lock scope
    if (aqos_valid) {
      try {
	set_admin(a_qos);
	a_success = 1;
      } catch (CosN::UnsupportedAdmin& e) {
	str << "\nThe following AdminQoS Property Settings are invalid:\n";
	RDI_describe_prop_errors(str, e.admin_err);
	str << '\n';
	success = 0;
      } catch (CORBA::INV_OBJREF) { throw; }
      if (a_success) {
	str << '\n';
	for (unsigned int i = 0; i < n_qos.length(); i++) {
	  str << n_qos[i].name << " set to "; RDI_pp_any(str, n_qos[i].value); str << '\n';
	}
      }
    } else {
      str << "\nThe following AdminQoS Property Settings are invalid:\n";
      RDI_describe_prop_errors(str, eseq);
      str << '\n';
      success = 0;
    }
  }
  if (a_success || n_success) {
    str << "\nSome properties updated successfully.  Current settings:\n\n";
    out_heading(str);
    out_config(str);
  }
  return success;
}

#undef WHATFN
#define WHATFN "EventChannel_i::do_go_command"
CORBA::Boolean
EventChannel_i::do_go_command(RDIstrstream& str, RDIParseCmd& p,
			      CORBA::Boolean& target_changed,
			      AttN_Interactive_outarg next_target)
{
  CORBA::Boolean success = 1;
  AttN::Interactive_ptr targ1 = AttN::Interactive::_nil();
  AttN::Interactive_ptr targ2 = AttN::Interactive::_nil();
  CORBA::Boolean targ1_set = 0, targ2_set = 0;

  char* go_targ = CORBA_STRING_DUP(p.argv[1]);
  char* rest_go_targ = RDI_STRCHR(go_targ, '.');
  if (rest_go_targ) {
    *rest_go_targ = '\0';
    rest_go_targ++;
  }
  CORBA::Long id = RDIParseId("sadmin", go_targ);

  { // introduce lock scope
    RDI_OPLOCK_SCOPE_LOCK(channel_lock, WHATFN, return 0);
    if (_shutmedown) { return 0; }
    if (id >= 0) {
      SupplierAdmin_i* sadmin = 0;
      if ( _supl_admin.lookup((CosNA::AdminID)id, sadmin) ) {
	targ1 = WRAPPED_IMPL2OREF(AttN::SupplierAdmin, sadmin);
	targ1_set = 1;
	str << "\nomniNotify: new target ==> " << go_targ << '\n';
      }
    } else {
      id = RDIParseId("cadmin", p.argv[1]);
      if (id >= 0) {
	ConsumerAdmin_i* cadmin = 0;
	if ( _cons_admin.lookup((CosNA::AdminID)id, cadmin) ) {
	  targ1 = WRAPPED_IMPL2OREF(AttN::ConsumerAdmin, cadmin);
	  targ1_set = 1;
	  str << "\nomniNotify: new target ==> " << go_targ << '\n';
	}
      }
    }
  } // end lock scope
  if (targ1_set == 0) {
    str << "Invalid target " << p.argv[1] << " : " <<  go_targ << " is not an admin name\n";
    str << "  (Use 'children' for list of valid admin names)\n";
    success = 0;
  } else if (rest_go_targ && (RDI_STRLEN(rest_go_targ) > 0)) {
    CORBA::String_var cmdres;
    char* newcmd = CORBA_STRING_ALLOC(4 + RDI_STRLEN(rest_go_targ));
    sprintf(newcmd, "go %s", rest_go_targ);
    CORBA::Boolean docmd_prob = 0;
    try {
      cmdres = targ1->do_command(newcmd, success, targ2_set, targ2);
    } CATCH_INVOKE_PROBLEM(docmd_prob);
    CORBA_STRING_FREE(newcmd);
    if (docmd_prob) {
      str << "The target " << rest_go_targ << " is not available\n";
    } else {
      str << cmdres.in();
    }
  }
  CORBA_STRING_FREE(go_targ);
  if (targ2_set) {
    CORBA::release(targ1);
    next_target = targ2;
    target_changed = 1;
  } else if (targ1_set) {
    next_target = targ1;
    target_changed = 1;
  }
  return success;
}

void
EventChannel_i::cleanup(RDIstrstream& str, CORBA::Boolean admins, CORBA::Boolean proxies)
{
  RDIInteractive::cleanup_channel(str, _my_oref, admins, proxies);
}

CORBA::Boolean
EventChannel_i::safe_cleanup( WRAPPED_IMPLARG_VOID )
{
  return 0; // not destroyed
}

void
EventChannel_i::out_commands(RDIstrstream& str)
{
  str << "omniNotify EventChannel commands:\n"
      << "  up                : change target to chanfact\n"
      << "  go <admin_name>   : change target to a specific admin\n"
      << "                        ('children' lists admin names)\n"
      << "  debug             : show this channel's debugging information\n"
      << "  config            : show this channel's configuration\n"
      << "  stats             : show this channel's statistics\n"
      << "  info filters      : show filters attached to this channel's admins and proxies\n"
      << "  info afilters     : show filters attached to this channel's admins\n"
      << "  info pfilters     : show filters attached to this channel's proxies\n"
      << "  cleanup proxies   : destroy all proxies of this channel that are not connected\n"
      << "  cleanup admins    : destroy all admins  of this channel that have no connected proxies\n"
      << "  cleanup both      : combines 'cleanup proxies' and 'cleanup admins'\n"
      << "  set <name1> <value1> [ <name2> <value2> ... ]\n"
      << "                    : set NotifQoS or AdminQoS property name1 to value1, name2 to value2, etc.\n"
      << "                      (use 'config' to see all NotifQoS and AdminQoS properties)\n";
}

char*
EventChannel_i::do_command(const char* cmnd, CORBA::Boolean& success,
			   CORBA::Boolean& target_changed,
			   AttN_Interactive_outarg next_target  WRAPPED_IMPLARG )
{
  RDIParseCmd p(cmnd);
  success = 1;
  target_changed = 0;
  if (p.argc == 0) { return CORBA_STRING_DUP("");  }

  RDIstrstream str;
  if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "help")) {
    out_commands(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "_test_")) {
    if (_server_qos) {
      str << "inacclo = " << _server_qos->localClockInaccLo << '\n';
      str << "inacchi = " << _server_qos->localClockInaccHi << '\n';
      str << "tdf     = " << _server_qos->localClockTDF << '\n';
    } else {
      str << "** Error obtaining ServerQoS ** \n";
    }
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "debug")) {
    out_heading(str);
    out_debug_info(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "config")) {
    out_heading(str);
    out_config(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "stats")) {
    out_stats(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "up")) {
    target_changed = 1;
    next_target = WRAPPED_IMPL2OREF(AttN::EventChannelFactory, _my_channel_factory);
    str << "\nomniNotify: new target ==> chanfact\n";
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "info") && RDI_STR_EQ_I(p.argv[1], "filters")) {
    out_info_filters(str, 1, 1);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "info") && RDI_STR_EQ_I(p.argv[1], "afilters")) {
    out_info_filters(str, 1, 0);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "info") && RDI_STR_EQ_I(p.argv[1], "pfilters")) {
    out_info_filters(str, 0, 1);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "cleanup") && RDI_STR_EQ_I(p.argv[1], "proxies")) {
    cleanup(str, 0, 1);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "cleanup") && RDI_STR_EQ_I(p.argv[1], "admins")) {
    cleanup(str, 1, 0);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "cleanup") && RDI_STR_EQ_I(p.argv[1], "both")) {
    cleanup(str, 1, 1);
  } else if ((p.argc >= 1) && RDI_STR_EQ_I(p.argv[0], "set")) {
    success = do_set_command(str, p);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "go")) {
    success = do_go_command(str, p, target_changed, next_target);
  } else {
    str << "Invalid command: " << cmnd << "\n";
    success = 0;
  }
  RDIRptInteractiveLog(_my_name << " received command: " << cmnd << "\nResult:\n" << str.buf());
  // this is the only safe way to return a string?
  return CORBA_STRING_DUP(str.buf());
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::my_name"
AttN::NameSeq*
EventChannelFactory_i::my_name( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::NameSeq* res = new AttN::NameSeq(_my_name);
  if ( res == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  return res;
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::child_names"
AttN::NameSeq*
EventChannelFactory_i::child_names( WRAPPED_IMPLARG_VOID )
{
  char buf[20];
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::NameSeq* names = new AttN::NameSeq;
  if ( names == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  names->length(_channel.length());
  unsigned int index = 0;
  RDI_HashCursor<CosNA::ChannelID, EventChannel_i*> cursor;
  for ( cursor = _channel.cursor(); cursor.is_valid(); index++, ++cursor ) {
    sprintf(buf, "chan%ld", (long)cursor.key());
    (*names)[index] = (const char*)buf;
  }
  return names;
}

void
EventChannelFactory_i::_children(AttN::IactSeq& ren, CORBA::Boolean only_cleanup_candidates)
{
  // All channels are cleanup candidates
  ren.length(_channel.length());
  unsigned int idx = 0;
  RDI_HashCursor<CosNA::ChannelID, EventChannel_i*> cursor;
  for ( cursor = _channel.cursor(); cursor.is_valid(); ++cursor ) {
    ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, cursor.val());
  }
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::children"
AttN::IactSeq*
EventChannelFactory_i::children(CORBA::Boolean only_cleanup_candidates  WRAPPED_IMPLARG )
{
  // All channels are cleanup candidates
  RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::IactSeq* ren = new AttN::IactSeq;
  if ( ren == (AttN::IactSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  _children(*ren, only_cleanup_candidates);
  return ren;
}

CORBA::Boolean
EventChannelFactory_i::safe_cleanup( WRAPPED_IMPLARG_VOID )
{
  return 0; // not destroyed
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::do_set_command"
CORBA::Boolean
EventChannelFactory_i::do_set_command(RDIstrstream& str, RDIParseCmd& p)
{
  CosN::QoSProperties    n_qos;
  CosN::AdminProperties  a_qos;
  AttN::ServerProperties s_qos;
  CORBA::Boolean success = 1, n_success = 0, a_success = 0;
  success = RDI_AllQoS::parse_set_command(str, p, RDI_ECHANNEL, n_qos, a_qos, s_qos);
  if (success && n_qos.length() > 0) {
    try {
      set_qos(n_qos);
      n_success = 1;
    } catch (CosN::UnsupportedQoS& e) {
      str << "\nThe following NotifQoS Property Settings are invalid:\n";
      RDI_describe_prop_errors(str, e.qos_err);
      str << '\n';
      success = 0;
    } catch (CORBA::INV_OBJREF) { throw; }
    if (n_success) {
      str << '\n';
      for (unsigned int i = 0; i < n_qos.length(); i++) {
	str << n_qos[i].name << " set to "; RDI_pp_any(str, n_qos[i].value); str << '\n';
      }
    }
  }
  if (success && a_qos.length() > 0) {
    CosN::PropertyErrorSeq eseq;
    CORBA::Boolean aqos_valid;
    { // introduce lock scope
      RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
      // this is locked for rest of scope
      aqos_valid = _defadm.validate(str, a_qos, eseq);
    } // end lock scope
    if (aqos_valid) {
      try {
	set_admin(a_qos);
	a_success = 1;
      } catch (CosN::UnsupportedAdmin& e) {
	str << "\nThe following AdminQoS Property Settings are invalid:\n";
	RDI_describe_prop_errors(str, e.admin_err);
	str << '\n';
	success = 0;
      } catch (CORBA::INV_OBJREF) { throw; }
      if (a_success) {
	str << '\n';
	for (unsigned int i = 0; i < n_qos.length(); i++) {
	  str << n_qos[i].name << " set to "; RDI_pp_any(str, n_qos[i].value); str << '\n';
	}
      }
    } else {
      str << "\nThe following AdminQoS Property Settings are invalid:\n";
      RDI_describe_prop_errors(str, eseq);
      str << '\n';
      success = 0;
    }
  }
  if (a_success || n_success) {
    str << "\nSome properties updated successfully.  Current settings:\n\n";
    out_default_config(str);
  }
  return success;
}

#undef WHATFN
#define WHATFN "EventChannelFactory_i::do_go_command"
CORBA::Boolean
EventChannelFactory_i::do_go_command(RDIstrstream& str, RDIParseCmd& p,
				     CORBA::Boolean& target_changed,
				     AttN_Interactive_outarg next_target)
{
  CORBA::Boolean success = 1;
  AttN::Interactive_ptr targ1 = AttN::Interactive::_nil();
  AttN::Interactive_ptr targ2 = AttN::Interactive::_nil();
  CORBA::Boolean targ1_set = 0, targ2_set = 0;

  char* go_targ = CORBA_STRING_DUP(p.argv[1]);
  char* rest_go_targ = RDI_STRCHR(go_targ, '.');
  if (rest_go_targ) {
    *rest_go_targ = '\0';
    rest_go_targ++;
  }
  CORBA::Long id = RDIParseId("chan", go_targ);

  { // introduce lock scope
    RDI_OPLOCK_SCOPE_LOCK(chanfact_lock, WHATFN, return 0);
    if (id >= 0) {
      EventChannel_i* channel = 0;
      if ( _channel.lookup((CosNA::ChannelID)id, channel) ) {
	targ1 = WRAPPED_IMPL2OREF(AttN::EventChannel, channel);
	targ1_set = 1;
	str << "\nomniNotify: new target ==> " << go_targ << '\n';
      }
    }
  } // end lock scope
  if (targ1_set == 0) {
    str << "Invalid target " << p.argv[1] << " : " << go_targ << " is not a channel name\n";
    str << "  (Use 'children' for list of valid channel names)\n";
    success = 0;
  } else if (rest_go_targ && (RDI_STRLEN(rest_go_targ) > 0)) {
    CORBA::String_var cmdres;
    char* newcmd = CORBA_STRING_ALLOC(4 + RDI_STRLEN(rest_go_targ));
    sprintf(newcmd, "go %s", rest_go_targ);
    CORBA::Boolean docmd_prob = 0;
    try {
      cmdres = targ1->do_command(newcmd, success, targ2_set, targ2);
    } CATCH_INVOKE_PROBLEM(docmd_prob);
    CORBA_STRING_FREE(newcmd);
    if (docmd_prob) {
      str << "The target " << rest_go_targ << " is not available\n";
    } else {
      str << cmdres.in();
    }
  }
  CORBA_STRING_FREE(go_targ);
  if (targ2_set) {
    CORBA::release(targ1);
    next_target = targ2;
    target_changed = 1;
  } else if (targ1_set) {
    next_target = targ1;
    target_changed = 1;
  }
  return success;
}

void
EventChannelFactory_i::out_commands(RDIstrstream& str)
{
  str << "omniNotify EventChannelFactory commands:\n"
      << "  up                : change target to server\n"
      << "  go <chan_name>    : change target to a specific channel\n"
      << "                        ('children' lists channel names)\n"
      << "  debug             : show all channel debugging information\n"
      << "  config default    : show default channel configuration\n"
      << "  config chans      : show all channel configurations\n"
      << "  stats             : show all channel statistics\n"
      << "  cleanup proxies   : destroy all proxies of all channels that are not connected\n"
      << "  cleanup admins    : destroy all admins  of all channels that have no connected proxies\n"
      << "  cleanup both      : combines 'cleanup proxies' and 'cleanup admins'\n"
      << "  set <name1> <value1> [ <name2> <value2> ... ]\n"
      << "                    : set Default NotifQoS or AdminQoS property name1 to value1, name2 to value2, etc.\n";
}

char*
EventChannelFactory_i::do_command(const char* cmnd, CORBA::Boolean& success,
				  CORBA::Boolean& target_changed,
				  AttN_Interactive_outarg next_target  WRAPPED_IMPLARG )
{
  RDIParseCmd p(cmnd);
  success = 1;
  target_changed = 0;
  if (p.argc == 0) { return CORBA_STRING_DUP("");  }

  RDIstrstream str;
  RDINotifServer* server = RDI::get_server_i();
  if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "help")) {
    out_commands(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "stats")) {
    out_all_stats(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "debug")) {
    out_all_debug_info(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "config")) {
    out_default_config(str);
    out_all_config(str);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "config") && RDI_STR_EQ_I(p.argv[1], "default")) {
    out_default_config(str);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "config") && RDI_STR_EQ_I(p.argv[1], "chans")) {
    out_all_config(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "up")) {
    target_changed = 1;
    next_target = RDI::get_server();
    str << "\nomniNotify: new target ==> server\n";
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "go")) {
    success = do_go_command(str, p, target_changed, next_target);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "cleanup") && RDI_STR_EQ_I(p.argv[1], "proxies")) {
    cleanup_all(str, 0, 1);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "cleanup") && RDI_STR_EQ_I(p.argv[1], "admins")) {
    cleanup_all(str, 1, 0);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "cleanup") && RDI_STR_EQ_I(p.argv[1], "both")) {
    cleanup_all(str, 1, 1);
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

