// -*- Mode: C++; -*-
//                              File      : ChannelAdmin_i.cc
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
//    Implementation of ConsumerAdmin_i and SupplierAdmin_i
//

#include "corba_wrappers.h"
#include "RDICatchMacros.h"
#include "RDIStringDefs.h"
#include "RDI.h"
#include "CosNotifyShorthands.h"
#include "CosNfyUtils.h"
#include "CosNotification_i.h"
#include "RDITypeMap.h"
#include "RDIOplocksMacros.h"
#include "CosEventChannelAdmin_i.h"
#include "CosNotifyChannelAdmin_i.h"
#include "RDIInteractive.h"

// ------------------------------------------------------------- //
// ConsumerAdmin_i implementation                                //
// ------------------------------------------------------------- //

ConsumerAdmin_i::ConsumerAdmin_i(EventChannel_i* channel,
				 CosNA::InterFilterGroupOperator op,
				 const CosNA::AdminID& serial) :
  RDINotifySubscribe(),
  _oplockptr(0), _my_name(channel->L_my_name()), _disposed(0),
  _fa_helper("cadmin_fa_helper"), _channel(channel), _qosprop(0),
  _serial(serial), _and_or_oper(op), _rqstypes(), _prx_serial(1),
  _num_proxies(0), _cosevent_push(), _cosevent_pull(),
  _prx_any_push(RDI_CorbaULongHash, RDI_CorbaULongRank),
  _prx_any_pull(RDI_CorbaULongHash, RDI_CorbaULongRank),
  _prx_struc_push(RDI_CorbaULongHash, RDI_CorbaULongRank),
  _prx_struc_pull(RDI_CorbaULongHash, RDI_CorbaULongRank),
  _prx_batch_push(RDI_CorbaULongHash, RDI_CorbaULongRank),
  _prx_batch_pull(RDI_CorbaULongHash, RDI_CorbaULongRank)
{
  RDI_OPLOCK_INIT("cadmin");
  _qosprop = new RDI_NotifQoS(_channel->qos_properties());
  RDI_AssertAllocThrowNo(_qosprop, "Memory allocation failure - RDI_NotifQoS\n");
  _rqstypes.length(0);
  _prio_filter = CosNF::MappingFilter::_nil();
  _life_filter = CosNF::MappingFilter::_nil();
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  char buf[20];
  sprintf(buf, "cadmin%ld", (long)serial);
  _my_name.length(_my_name.length()+1);
  _my_name[_my_name.length()-1] = (const char*)buf;
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
  _my_oref = WRAPPED_IMPL2OREF(AttN::ConsumerAdmin, this);
}

ConsumerAdmin_i::~ConsumerAdmin_i() {
  // RDIDbgForceLog("XXX_REMOVE  ~ConsumerAdmin_i() called for cadmin " << _serial << '\n');
  RDI_OPLOCK_DESTROY_CHECK("ConsumerAdmin_i");
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::MyID"
CosNA::AdminID
ConsumerAdmin_i::MyID( WRAPPED_IMPLARG_VOID ) 
{
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosNA::AdminID res = _serial;
  return res;
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::MyChannel"
CosNA::EventChannel_ptr
ConsumerAdmin_i::MyChannel( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosNA::EventChannel_ptr res = WRAPPED_IMPL2OREF(CosNA::EventChannel, _channel);
  return res;
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::MyOperator"
CosNA::InterFilterGroupOperator
ConsumerAdmin_i::MyOperator( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosNA::InterFilterGroupOperator res = _and_or_oper;
  return res;
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::priority_filter_0arg"
CosNF::MappingFilter_ptr
ConsumerAdmin_i::priority_filter( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosNF::MappingFilter_ptr res = CosNF::MappingFilter::_nil();
  if ( !CORBA::is_nil(_prio_filter) ) {
    res = CosNF::MappingFilter::_duplicate(_prio_filter);
  }
  return res;
}
 
#undef WHATFN
#define WHATFN "ConsumerAdmin_i::priority_filter_1arg"
void
ConsumerAdmin_i::priority_filter(CosNF::MappingFilter_ptr prio_filter   
				 WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  if ( CORBA::is_nil(prio_filter) ) {
    throw CORBA::BAD_PARAM(0, CORBA::COMPLETED_NO);
  }
  _prio_filter = CosNF::MappingFilter::_duplicate(prio_filter);
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::lifetime_filter_0arg"
CosNF::MappingFilter_ptr
ConsumerAdmin_i::lifetime_filter( WRAPPED_IMPLARG_VOID )
{ 
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosNF::MappingFilter_ptr res = CosNF::MappingFilter::_nil();
  if ( ! CORBA::is_nil(_life_filter) ) {
    res = CosNF::MappingFilter::_duplicate(_life_filter); 
  }
  return res;
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::lifetime_filter_1arg"
void
ConsumerAdmin_i::lifetime_filter(CosNF::MappingFilter_ptr life_filter   
				 WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  if ( CORBA::is_nil(life_filter) ) {
    throw CORBA::BAD_PARAM(0, CORBA::COMPLETED_NO);
  }
  _life_filter = CosNF::MappingFilter::_duplicate(life_filter);
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::obtain_push_supplier"
CosEvCA::ProxyPushSupplier_ptr 
ConsumerAdmin_i::obtain_push_supplier( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosEvCA::ProxyPushSupplier_ptr res = 
    CosEvCA::ProxyPushSupplier::_nil();
  EventProxyPushSupplier_i* prx=0;
  
  RDIDbgCAdmLog("CosEvent ProxyPushSupplier creation requested\n");
  if ( ! _channel->incr_consumers() ) {
    // return null res
  } else if ( ! (prx = new EventProxyPushSupplier_i(this, _channel, _prx_serial)) ) {
    _channel->decr_consumers();
    // return null res
  } else if ( _cosevent_push.insert_tail(prx) != 0 ) {
    _channel->decr_consumers();
    prx->disconnect_client_and_dispose(held, 0);
    // return null res
  } else { // return valid res
    _num_proxies++;
    _prx_serial++;
    RDI_NotifyConsumer* cpc = _channel->push_consumer();
    if ( cpc ) {        // Register Push Proxy
      cpc->insert_proxy(prx);
    }
    RDIDbgCAdmLog("CosEvent ProxyPushSupplier creation completed\n");
    res = WRAPPED_IMPL2OREF(CosEvCA::ProxyPushSupplier, prx);
  }
  return res;
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::obtain_pull_supplier"
CosEvCA::ProxyPullSupplier_ptr
ConsumerAdmin_i::obtain_pull_supplier( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosEvCA::ProxyPullSupplier_ptr res = 
    CosEvCA::ProxyPullSupplier::_nil();
  EventProxyPullSupplier_i* prx=0;
  RDIDbgCAdmLog("CosEvent ProxyPullSupplier creation requested\n");
  if ( ! _channel->incr_consumers() ) {
    // return null res
  } else if ( ! (prx = new EventProxyPullSupplier_i(this, _channel, _prx_serial)) ) {
    _channel->decr_consumers();
    // return null res
  } else if ( _cosevent_pull.insert_tail(prx) != 0 ) {
    _channel->decr_consumers();
    prx->disconnect_client_and_dispose(held, 0);
    // return null res
  } else { // return valid res
    _prx_serial++;
    _num_proxies++;
    RDIDbgCAdmLog("CosEvent ProxyPullSupplier creation completed\n");
    res = WRAPPED_IMPL2OREF(CosEvCA::ProxyPullSupplier, prx);
  }
  return res;
}

// called externally by consumers
#undef WHATFN
#define WHATFN "ConsumerAdmin_i::subscription_change"
void
ConsumerAdmin_i::subscription_change(const CosN::EventTypeSeq& added,
				     const CosN::EventTypeSeq& deled   
				     WRAPPED_IMPLARG )
{
  RDI_LocksHeld held = { 0 };

  RDI_OPLOCK_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin)     { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed

  // A consumer can invoke this operation as long as filters have
  // not been placed at this admin
  if ( _fa_helper.has_filters() ) {
    throw CORBA::BAD_OPERATION(0, CORBA::COMPLETED_NO);
  }

  CosN::EventTypeSeq added_copy = added;
  CosN::EventTypeSeq deled_copy = deled;
  CosN::EventTypeSeq nadded;
  CosN::EventTypeSeq ndeled;
  nadded.length(0);
  ndeled.length(0);
  CORBA::ULong   ix = 0;
  if ( ! RDI_EventType::valid_sequence(added_copy, ix) ) {
    throw CosNC::InvalidEventType(added[ix]);
  }
  if ( ! RDI_EventType::valid_sequence(deled_copy, ix) ) {
    throw CosNC::InvalidEventType(deled[ix]);
  }
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
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::pull_suppliers"
CosNA::ProxyIDSeq*
ConsumerAdmin_i::pull_suppliers( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  RDI_HashCursor<CosNA::ProxyID, ProxyPullSupplier_i *>           ac;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullSupplier_i *> sc;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullSupplier_i * >  bc;
  unsigned long num;
  CosNA::ProxyIDSeq*  seq = new CosNA::ProxyIDSeq();
  if ( ! seq ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_MAYBE);
  }
  num = _prx_any_pull.length() +
    _prx_struc_pull.length() + _prx_batch_pull.length();
  seq->length(num);
  for ( num=0, ac = _prx_any_pull.cursor(); ac.is_valid(); ++ac, ++num ) {
    (*seq)[num] = ac.key();
  }
  for ( sc = _prx_struc_pull.cursor(); sc.is_valid(); ++sc, ++num ) {
    (*seq)[num] = sc.key();
  }
  for ( bc = _prx_batch_pull.cursor(); bc.is_valid(); ++bc, ++num ) {
    (*seq)[num] = bc.key();
  }
  return seq;
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::push_suppliers"
CosNA::ProxyIDSeq*
ConsumerAdmin_i::push_suppliers( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  RDI_HashCursor<CosNA::ProxyID, ProxyPushSupplier_i *>           ac;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushSupplier_i *> sc;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushSupplier_i * >  bc;
  unsigned long num;
  CosNA::ProxyIDSeq*  seq = new CosNA::ProxyIDSeq();
  if ( ! seq ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_MAYBE);
  }
  num = _prx_any_push.length() +
    _prx_struc_push.length() + _prx_batch_push.length();
  seq->length(num);
  for ( num=0, ac = _prx_any_push.cursor(); ac.is_valid(); ++ac, ++num ) {
    (*seq)[num] = ac.key();
  }
  for ( sc = _prx_struc_push.cursor(); sc.is_valid(); ++sc, ++num ) {
    (*seq)[num] = sc.key();        
  }
  for ( bc = _prx_batch_push.cursor(); bc.is_valid(); ++bc, ++num ) {
    (*seq)[num] = bc.key();
  }
  return seq;
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::get_proxy_supplier"
CosNA::ProxySupplier_ptr 
ConsumerAdmin_i::get_proxy_supplier(CosNA::ProxyID proxy_id WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  ProxyPushSupplier_i*           apush=0;
  ProxyPullSupplier_i*           apull=0;
  StructuredProxyPushSupplier_i* spush=0;
  StructuredProxyPullSupplier_i* spull=0;
  SequenceProxyPushSupplier_i*   bpush=0;
  SequenceProxyPullSupplier_i*   bpull=0;

  CosNA::ProxySupplier_ptr res = CosNA::ProxySupplier::_nil();

  if ( proxy_id <= _prx_serial ) {
    if ( _prx_any_push.lookup(proxy_id, apush) ) {
      res = WRAPPED_IMPL2OREF(CosNA::ProxySupplier, apush);
    } else if ( _prx_any_pull.lookup(proxy_id, apull) ) {
      res = WRAPPED_IMPL2OREF(CosNA::ProxySupplier, apull);
    } else if ( _prx_struc_push.lookup(proxy_id, spush) ) {
      res = WRAPPED_IMPL2OREF(CosNA::ProxySupplier, spush);
    } else if ( _prx_struc_pull.lookup(proxy_id, spull) ) {
      res = WRAPPED_IMPL2OREF(CosNA::ProxySupplier, spull);
    } else if ( _prx_batch_push.lookup(proxy_id, bpush) ) {
      res = WRAPPED_IMPL2OREF(CosNA::ProxySupplier, bpush);
    } else if ( _prx_batch_pull.lookup(proxy_id, bpull) ) {
      res = WRAPPED_IMPL2OREF(CosNA::ProxySupplier, bpull);
    }
  }
  if (CORBA::is_nil(res)) {
    throw CosNA::ProxyNotFound();
  }
  return res;
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::obtain_notification_pull_supplier"
CosNA::ProxySupplier_ptr 
ConsumerAdmin_i::obtain_notification_pull_supplier(CosNA::ClientType ctype, 
						   CosNA::ProxyID&   proxy_id
						   WRAPPED_IMPLARG )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed

  // Check with  the channel if we are about to exceed the maximum
  // number of consumers supported by the channel
  RDIDbgCAdmLog("Creation of a new ProxySupplier -- type " << ctype << '\n');
  if ( ! _channel->incr_consumers() ) {
    CosNA::AdminLimit limit;
    RDIDbgCAdmLog("Consumer limit has been reached creation failed\n");
    limit.name    = (const char *) "MaxConsumers";
    limit.value <<= _channel->max_consumers();
    throw CosNA::AdminLimitExceeded(limit);
  }

  CosNA::ProxySupplier_ptr res = CosNA::ProxySupplier::_nil();
  if ( ctype == CosNA::ANY_EVENT ) {
    ProxyPullSupplier_i* prx = 0;
    if ( ! (prx = new ProxyPullSupplier_i(this, _channel, _prx_serial)) ) {
      _channel->decr_consumers();
    } else {
      proxy_id = _prx_serial++;
      if ( _prx_any_pull.insert(proxy_id, prx) != 0 ) {
	prx->disconnect_client_and_dispose(held, 0);
	_channel->decr_consumers(); 
      } else {
	_num_proxies += 1;
	res = WRAPPED_IMPL2OREF(CosNA::ProxySupplier, prx);
      }
    }
  } else if ( ctype == CosNA::STRUCTURED_EVENT ) {
    StructuredProxyPullSupplier_i* prx = 
      new StructuredProxyPullSupplier_i(this,_channel,_prx_serial);
    if ( ! prx ) {
      _channel->decr_consumers();
    } else {
      proxy_id = _prx_serial++;
      if ( _prx_struc_pull.insert(proxy_id, prx) != 0 ) {
	prx->disconnect_client_and_dispose(held, 0);
	_channel->decr_consumers();
      } else {
	_num_proxies += 1;
	res = WRAPPED_IMPL2OREF(CosNA::ProxySupplier, prx);
      }
    }
  } else if ( ctype == CosNA::SEQUENCE_EVENT ) {
    SequenceProxyPullSupplier_i* prx =
      new SequenceProxyPullSupplier_i(this,_channel,_prx_serial);
    if ( ! prx ) {
      _channel->decr_consumers();
    } else {
      proxy_id = _prx_serial++;
      if ( _prx_batch_pull.insert(proxy_id, prx) != 0 ) {
	prx->disconnect_client_and_dispose(held, 0);
	_channel->decr_consumers();
      } else {
	_num_proxies += 1;
	res = WRAPPED_IMPL2OREF(CosNA::ProxySupplier, prx);
      }
    }
  }
  return res;
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::obtain_notification_push_supplier"
CosNA::ProxySupplier_ptr 
ConsumerAdmin_i::obtain_notification_push_supplier(CosNA::ClientType ctype, 
						   CosNA::ProxyID&   proxy_id
						   WRAPPED_IMPLARG )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed

  // Check with  the channel if we are about to exceed the maximum
  // number of consumers supported by the channel
  RDIDbgCAdmLog("Creation of a new ProxySupplier -- type " << ctype << '\n');
  if ( ! _channel->incr_consumers() ) {
    CosNA::AdminLimit limit;
    RDIDbgCAdmLog("Consumer limit has been reached creation failed\n");
    limit.name    = (const char *) "MaxConsumers";
    limit.value <<= _channel->max_consumers();
    throw CosNA::AdminLimitExceeded(limit);
  }

  CosNA::ProxySupplier_ptr res = CosNA::ProxySupplier::_nil();
  if ( ctype == CosNA::ANY_EVENT ) {
    ProxyPushSupplier_i* prx = 0;
    if ( ! (prx = new ProxyPushSupplier_i(this, _channel, _prx_serial)) ) {
      _channel->decr_consumers();
    } else {
      proxy_id = _prx_serial++;
      if ( _prx_any_push.insert(proxy_id, prx) != 0 ) {
	prx->disconnect_client_and_dispose(held, 0);
	_channel->decr_consumers(); 
      } else {
	RDI_NotifyConsumer* cpc = _channel->push_consumer();
	if ( cpc ) {    // Register Push Proxy
	  cpc->insert_proxy(prx);
	}
	_num_proxies += 1;
	res = WRAPPED_IMPL2OREF(CosNA::ProxySupplier, prx);
      }
    }
  } else if ( ctype == CosNA::STRUCTURED_EVENT ) {
    StructuredProxyPushSupplier_i* prx = 
      new StructuredProxyPushSupplier_i(this,_channel,_prx_serial);
    if ( ! prx ) {
      RDIDbgCAdmLog("\tfailed to create proxy\n");
      _channel->decr_consumers();
    } else {
      proxy_id = _prx_serial++;
      if ( _prx_struc_push.insert(proxy_id, prx) != 0 ) {
	RDIDbgCAdmLog("\tfailed to register proxy with id " << proxy_id << '\n');
	prx->disconnect_client_and_dispose(held, 0);
	_channel->decr_consumers();
      } else {
	RDI_NotifyConsumer* cpc = _channel->push_consumer();
	if ( cpc ) {    // Register Push Proxy
	  cpc->insert_proxy(prx);
	}
	_num_proxies += 1;
	res = WRAPPED_IMPL2OREF(CosNA::ProxySupplier, prx);
      }
    }
  } else if ( ctype == CosNA::SEQUENCE_EVENT ) {
    SequenceProxyPushSupplier_i* prx = 
      new SequenceProxyPushSupplier_i(this,_channel,_prx_serial);
    if ( ! prx ) {
      _channel->decr_consumers();
    } else {
      proxy_id = _prx_serial++;
      if ( _prx_batch_push.insert(proxy_id, prx) != 0 ) {
	prx->disconnect_client_and_dispose(held, 0);
	_channel->decr_consumers();
      } else { 
	RDI_NotifyConsumer* cpc = _channel->push_consumer();
	if ( cpc ) {    // Register Push Proxy
	  cpc->insert_proxy(prx);
	}
	_num_proxies += 1;
	res = WRAPPED_IMPL2OREF(CosNA::ProxySupplier, prx);
      }
    }
  }
  return res;
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::safe_cleanup"
CORBA::Boolean
ConsumerAdmin_i::safe_cleanup( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { RDI_THROW_INV_OBJREF; }
  if ( _serial == 0 ) { // Default ConsumerAdmin not a cleanup candidate
    return 0;
  }
  if (_num_proxies > 0) { // Admin with proxies not a cleanup candidate
    return 0;
  }
  _disconnect_clients_and_dispose(held, 0, 1, cadmin_lock.dispose_info); // arg 1 means update channel
  return 1;
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::destroy"
void
ConsumerAdmin_i::destroy( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { RDI_THROW_INV_OBJREF; }
  if ( _serial == 0 ) {
    RDIDbgCAdmLog("Default ConsumerAdmin cannot be destroyed!\n");
    return;
  }
  _disconnect_clients_and_dispose(held, 0, 1, cadmin_lock.dispose_info); // arg 1 means update channel
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::get_qos"
CosN::QoSProperties*
ConsumerAdmin_i::get_qos( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  //RDI_NotifQoS*   qosp = qos_properties();
  //RDI_Assert(qosp, "Event Channel should have RDI_NotifQoS set\n");
  //CosN::QoSProperties* res = qosp->get_qos(RDI_C_ADMIN);
  CosN::QoSProperties* res = _qosprop->get_qos(RDI_C_ADMIN);
  return res;
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::set_qos"
void
ConsumerAdmin_i::set_qos(const CosN::QoSProperties& r_qos  WRAPPED_IMPLARG )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  if (r_qos.length() == 0) return;
  CosN::PropertyErrorSeq eseq;
  CosN::NamedPropertyRangeSeq rseq;
  CORBA::Boolean  subobjs = (_num_proxies ? 1 : 0);
  
  if (! RDI_NotifQoS::validate(r_qos, *_qosprop, RDI_C_ADMIN, eseq, rseq, subobjs)) {
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
#define WHATFN "ConsumerAdmin_i::validate_qos"
void
ConsumerAdmin_i::validate_qos(const CosN::QoSProperties& r_qos,
			      CosN_NamedPropertyRangeSeq_outarg rseq
			      WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosN::PropertyErrorSeq eseq;
  CORBA::Boolean  subobjs = (_num_proxies ? 1 : 0);

  rseq = new CosNotification::NamedPropertyRangeSeq();
  RDI_AssertAllocThrowNo((CosNotification::NamedPropertyRangeSeq*)rseq,
			 "Memory allocation failed - NamedPropertyRangeSeq\n");
  if (! RDI_NotifQoS::validate(r_qos, *_qosprop, RDI_C_ADMIN, eseq, *rseq, subobjs)) {
    throw CosN::UnsupportedQoS(eseq);
  }
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::add_filter"
CosNF::FilterID
ConsumerAdmin_i::add_filter(CosNF::Filter_ptr fltr WRAPPED_IMPLARG )
{
  CosNF::FilterID  res;
  RDI_LocksHeld    held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(outer_cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed

  // Above bump lock is obtained to make sure this cadmin is not disposed, to do
  // any initial checking, and to get the inuse count bumped so that it is safe
  // to access the _channel pointer after we do a release (it will remain
  // valid).  Now we release it and then obtain locks in this order:
  //     channel -> typemap -> cadmin

  { // introduce scope to temporarily release bump lock
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cadmin, WHATFN);
    { // introduce channel lock scope
      RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(chan_lock, held.channel, _channel, WHATFN);
      if (!held.channel || _channel->shutting_down()) { RDI_THROW_INV_OBJREF; }
      { // introduce typemap lock scope
	RDI_CHAN_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(typemap_w_lock, held.typemap, _channel, WHATFN);
	if (!held.typemap) { RDI_THROW_INV_OBJREF; }
	{ // introduce inner cadmin lock scope
	  RDI_OPLOCK_SCOPE_LOCK_TRACK(inner_cadmin_lock, held.cadmin, WHATFN);
	  if (!held.cadmin) { RDI_THROW_INV_OBJREF; }
	  if (_disposed) { RDI_THROW_INV_OBJREF; }

	  // FINALLY we can do some real work.
	  // If we have registered interest in specific event types, using
	  // 'subscription_change()', we have to cancel it at this point..
	  if ( _rqstypes.length() != 0 ) {
	    CosN::EventTypeSeq added; added.length(0);
	    (void) _channel->update_mapping(held, added, _rqstypes, this, 0);
	    _rqstypes.length(0);
	  }
	  res = _fa_helper.add_filter_i(held, fltr, (RDINotifySubscribe_ptr) this, 1); // 1 => DO want propagate_schange callbacks

	} // end inner cadmin lock scope
      } // end typemap lock scope
    } // end channel lock scope
  } // end temporary release scope
  return res;
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::remove_filter"
void
ConsumerAdmin_i::remove_filter(CosNF::FilterID fltrID WRAPPED_IMPLARG )
{
  RDI_LocksHeld    held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(outer_cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed

  // Above bump lock is obtained to make sure this cadmin is not disposed, to do
  // any initial checking, and to get the inuse count bumped so that it is safe
  // to access the _channel pointer after we do a release (it will remain
  // valid).  Now we release it and then obtain locks in this order:
  //     channel -> typemap -> cadmin

  { // introduce scope to temporarily release bump lock
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cadmin, WHATFN);
    { // introduce channel lock scope
      RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(chan_lock, held.channel, _channel, WHATFN);
      if (!held.channel || _channel->shutting_down()) { RDI_THROW_INV_OBJREF; }
      { // introduce typemap lock scope
	RDI_CHAN_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(typemap_w_lock, held.typemap, _channel, WHATFN);
	if (!held.typemap) { RDI_THROW_INV_OBJREF; }
	{ // introduce inner cadmin lock scope
	  RDI_OPLOCK_SCOPE_LOCK_TRACK(inner_cadmin_lock, held.cadmin, WHATFN);
	  if (!held.cadmin) { RDI_THROW_INV_OBJREF; }
	  if (_disposed) { RDI_THROW_INV_OBJREF; }

	  // FINALLY we can do some real work.
	  _fa_helper.remove_filter(held, fltrID, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callback

	} // end inner cadmin lock scope
      } // end typemap lock scope
    } // end channel lock scope
  } // end temporary release scope
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::remove_all_filters"
void
ConsumerAdmin_i::remove_all_filters( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld    held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(outer_cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed

  // Above bump lock is obtained to make sure this cadmin is not disposed, to do
  // any initial checking, and to get the inuse count bumped so that it is safe
  // to access the _channel pointer after we do a release (it will remain
  // valid).  Now we release it and then obtain locks in this order:
  //     channel -> typemap -> cadmin

  { // introduce scope to temporarily release bump lock
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cadmin, WHATFN);
    { // introduce channel lock scope
      RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(chan_lock, held.channel, _channel, WHATFN);
      if (!held.channel || _channel->shutting_down()) { RDI_THROW_INV_OBJREF; }
      { // introduce typemap lock scope
	RDI_CHAN_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(typemap_w_lock, held.typemap, _channel, WHATFN);
	if (!held.typemap) { RDI_THROW_INV_OBJREF; }
	{ // introduce inner cadmin lock scope
	  RDI_OPLOCK_SCOPE_LOCK_TRACK(inner_cadmin_lock, held.cadmin, WHATFN);
	  if (!held.cadmin) { RDI_THROW_INV_OBJREF; }
	  if (_disposed) { RDI_THROW_INV_OBJREF; }

	  // FINALLY we can do some real work.
	  _fa_helper.remove_all_filters(held, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callbacks

	} // end inner cadmin lock scope
      } // end typemap lock scope
    } // end channel lock scope
  } // end temporary release scope
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::get_filter"
CosNF::Filter_ptr
ConsumerAdmin_i::get_filter(CosNF::FilterID fltrID WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosNF::Filter_ptr res = _fa_helper.get_filter(fltrID);
  return CosNF::Filter::_duplicate(res);
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::get_all_filters"
CosNF::FilterIDSeq*
ConsumerAdmin_i::get_all_filters( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosNF::FilterIDSeq* res = _fa_helper.get_all_filters();
  return res;
}

// -------------------- ** Local-only routines ** ----------------------------

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::remove_proxy_1"
void
ConsumerAdmin_i::remove_proxy(RDI_LocksHeld& held, EventProxyPushSupplier_i* prx)
{
  RDI_ListCursor<EventProxyPushSupplier_i *> lc;
  unsigned int    indx = 0;
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { return; }

  for ( lc = _cosevent_push.cursor(); indx < _cosevent_push.length(); indx++ ) {
    EventProxyPushSupplier_i* proxy = *lc;
    if ( proxy == prx ) {
      _cosevent_push.remove(lc);
      _removed_push_proxy(prx);
      break;
    }
    ++lc;
  }
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::remove_proxy_2"
void
ConsumerAdmin_i::remove_proxy(RDI_LocksHeld& held, EventProxyPullSupplier_i* prx)
{
  RDI_ListCursor<EventProxyPullSupplier_i *> lc;
  unsigned int    indx = 0;
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { return; }

  for ( lc = _cosevent_pull.cursor(); indx < _cosevent_pull.length(); indx++ ) {
    EventProxyPullSupplier_i* proxy = *lc;
    if (  proxy == prx ) {
      _cosevent_pull.remove(lc);
      _removed_pull_proxy();
      break;
    }
    ++lc;
  }
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::remove_proxy_3"
void
ConsumerAdmin_i::remove_proxy(RDI_LocksHeld& held, ProxyPushSupplier_i* prx)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { return; }
  RDIDbgCAdmLog("Remove proxy [" << prx->_proxy_id() << "] from admin " << (void*)this << '\n');
  if ( _prx_any_push.exists(prx->_proxy_id()) ) {
    _prx_any_push.remove(prx->_proxy_id());
    _removed_push_proxy(prx);
  } else {
    RDIDbgCAdmLog("Invalid proxy [" << prx->_proxy_id() << "] provided\n");
  }
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::remove_proxy_4"
void
ConsumerAdmin_i::remove_proxy(RDI_LocksHeld& held, ProxyPullSupplier_i* prx)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { return; }
  RDIDbgCAdmLog("Remove proxy [" << prx->_proxy_id() << "] from admin " << (void*)this << '\n');
  if ( _prx_any_pull.exists(prx->_proxy_id()) ) {
    _prx_any_pull.remove(prx->_proxy_id());
    _removed_pull_proxy();
  } else {
    RDIDbgCAdmLog("Invalid proxy [" << prx->_proxy_id() << "] provided\n");
  }
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::remove_proxy_5"
void
ConsumerAdmin_i::remove_proxy(RDI_LocksHeld& held, StructuredProxyPushSupplier_i* prx)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { return; }
  RDIDbgCAdmLog("Remove proxy [" << prx->_proxy_id() << "] from admin " << (void*)this << '\n');
  if ( _prx_struc_push.exists(prx->_proxy_id()) ) {
    _prx_struc_push.remove(prx->_proxy_id());
    _removed_push_proxy(prx);
  } else {
    RDIDbgCAdmLog("Invalid proxy [" << prx->_proxy_id() << "] provided\n");
  }
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::remove_proxy_6"
void
ConsumerAdmin_i::remove_proxy(RDI_LocksHeld& held, StructuredProxyPullSupplier_i* prx)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { return; }
  RDIDbgCAdmLog("Remove proxy [" << prx->_proxy_id() << "] from admin " << (void*)this << '\n');
  if ( _prx_struc_pull.exists(prx->_proxy_id()) ) {
    _prx_struc_pull.remove(prx->_proxy_id());
    _removed_pull_proxy();
  } else {
    RDIDbgCAdmLog("Invalid proxy [" << prx->_proxy_id() << "] provided\n");
  }
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::remove_proxy_7"
void
ConsumerAdmin_i::remove_proxy(RDI_LocksHeld& held, SequenceProxyPushSupplier_i* prx)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { return; }
  RDIDbgCAdmLog("Remove proxy [" << prx->_proxy_id() << "] from admin " << (void*)this << '\n');
  if ( _prx_batch_push.exists(prx->_proxy_id()) ) {
    _prx_batch_push.remove(prx->_proxy_id());
    _removed_push_proxy(prx);
  } else {
    RDIDbgCAdmLog("Invalid proxy [" << prx->_proxy_id() << "] provided\n");
  }
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::remove_proxy_8"
void
ConsumerAdmin_i::remove_proxy(RDI_LocksHeld& held, SequenceProxyPullSupplier_i* prx)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { return; }
  RDIDbgCAdmLog("Remove proxy [" << prx->_proxy_id() << "] from admin " << (void*)this << '\n');
  if ( _prx_batch_pull.exists(prx->_proxy_id()) ) {
    _prx_batch_pull.remove(prx->_proxy_id());
    _removed_pull_proxy();
  } else {
    RDIDbgCAdmLog("Invalid proxy [" << prx->_proxy_id() << "] provided\n");
  }
}

#ifndef NO_OBJ_GC

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::gc_able"
CORBA::Boolean
ConsumerAdmin_i::gc_able(RDI_TimeT curtime, CORBA::ULong deadAdmin)
{
  RDI_OPLOCK_BUMP_SCOPE_LOCK(cadmin_lock, WHATFN, return 0);
  if (_disposed) { return 0; }
  return (_num_proxies == 0 && deadAdmin && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadAdmin));
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::obj_gc"
CORBA::Boolean
ConsumerAdmin_i::obj_gc(RDI_TimeT curtime, CORBA::ULong deadAdmin, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { return 0; }
  if (_disposed) { return 0; }

  EventProxyPushSupplierPtrSeq                                    cpush;
  EventProxyPullSupplierPtrSeq                                    cpull;
  ProxyPushSupplierPtrSeq                                         apush;
  ProxyPullSupplierPtrSeq                                         apull;
  StructuredProxyPushSupplierPtrSeq                               spush;
  StructuredProxyPullSupplierPtrSeq                               spull;
  SequenceProxyPushSupplierPtrSeq                                 bpush;
  SequenceProxyPullSupplierPtrSeq                                 bpull;

  CosEvCA_EventProxyPushSupplierPtrSeq                            cpushrefs;
  CosEvCA_EventProxyPullSupplierPtrSeq                            cpullrefs;
  CosNA_ProxyPushSupplierPtrSeq                                   apushrefs;
  CosNA_ProxyPullSupplierPtrSeq                                   apullrefs;
  CosNA_StructuredProxyPushSupplierPtrSeq                         spushrefs;
  CosNA_StructuredProxyPullSupplierPtrSeq                         spullrefs;
  CosNA_SequenceProxyPushSupplierPtrSeq                           bpushrefs;
  CosNA_SequenceProxyPullSupplierPtrSeq                           bpullrefs;

  RDI_ListCursor<EventProxyPushSupplier_i *>                      cpushcur;
  RDI_ListCursor<EventProxyPullSupplier_i *>                      cpullcur;
  RDI_HashCursor<CosNA::ProxyID, ProxyPushSupplier_i *>           apushcur;
  RDI_HashCursor<CosNA::ProxyID, ProxyPullSupplier_i *>           apullcur;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushSupplier_i *> spushcur;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullSupplier_i *> spullcur;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushSupplier_i * >  bpushcur;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullSupplier_i * >  bpullcur;

  // CosEvent clients
  unsigned int num_cpush, num_cpull, num_apush, num_apull, num_spush, num_spull, num_bpush, num_bpull, i;

  num_cpush = _cosevent_push.length();
  cpush.length(num_cpush);
  cpushrefs.length(num_cpush);
  for (i = 0, cpushcur = _cosevent_push.cursor(); i < num_cpush; ++i, ++cpushcur) {
    EventProxyPushSupplier_i* prx = *cpushcur;
    cpush[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    cpushrefs[i] = WRAPPED_IMPL2OREF(CosEvCA::ProxyPushSupplier, prx);
  }

  num_cpull = _cosevent_pull.length();
  cpull.length(num_cpull);
  cpullrefs.length(num_cpull);
  for (i = 0, cpullcur = _cosevent_pull.cursor(); i < num_cpull; ++i, ++cpullcur) {
    EventProxyPullSupplier_i* prx = *cpullcur;
    cpull[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    cpullrefs[i] = WRAPPED_IMPL2OREF(CosEvCA::ProxyPullSupplier, prx);
  }

  num_apush = _prx_any_push.length();
  apush.length(num_apush);
  apushrefs.length(num_apush);
  for (i = 0, apushcur = _prx_any_push.cursor(); i < num_apush; ++i, ++apushcur) {
    ProxyPushSupplier_i* prx = apushcur.val();
    apush[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    apushrefs[i] = WRAPPED_IMPL2OREF(CosNA::ProxyPushSupplier, prx);
  }

  num_apull = _prx_any_pull.length();
  apull.length(num_apull);
  apullrefs.length(num_apull);
  for (i = 0, apullcur = _prx_any_pull.cursor(); i < num_apull; ++i, ++apullcur) {
    ProxyPullSupplier_i* prx = apullcur.val();
    apull[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    apullrefs[i] = WRAPPED_IMPL2OREF(CosNA::ProxyPullSupplier, prx);
  }

  num_spush = _prx_struc_push.length();
  spush.length(num_spush);
  spushrefs.length(num_spush);
  for (i = 0, spushcur = _prx_struc_push.cursor(); i < num_spush; ++i, ++spushcur) {
    StructuredProxyPushSupplier_i* prx = spushcur.val();
    spush[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    spushrefs[i] = WRAPPED_IMPL2OREF(CosNA::StructuredProxyPushSupplier, prx);
  }

  num_spull = _prx_struc_pull.length();
  spull.length(num_spull);
  spullrefs.length(num_spull);
  for (i = 0, spullcur = _prx_struc_pull.cursor(); i < num_spull; ++i, ++spullcur) {
    StructuredProxyPullSupplier_i* prx = spullcur.val();
    spull[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    spullrefs[i] = WRAPPED_IMPL2OREF(CosNA::StructuredProxyPullSupplier, prx);
  }

  num_bpush = _prx_batch_push.length();
  bpush.length(num_bpush);
  bpushrefs.length(num_bpush);
  for (i = 0, bpushcur = _prx_batch_push.cursor(); i < num_bpush; ++i, ++bpushcur) {
    SequenceProxyPushSupplier_i* prx = bpushcur.val();
    bpush[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    bpushrefs[i] = WRAPPED_IMPL2OREF(CosNA::SequenceProxyPushSupplier, prx);
  }

  num_bpull = _prx_batch_pull.length();
  bpull.length(num_bpull);
  bpullrefs.length(num_bpull);
  for (i = 0, bpullcur = _prx_batch_pull.cursor(); i < num_bpull; ++i, ++bpullcur) {
    SequenceProxyPullSupplier_i* prx = bpullcur.val();
    bpull[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    bpullrefs[i] = WRAPPED_IMPL2OREF(CosNA::SequenceProxyPullSupplier, prx);
  }

  { // introduce temporary lock release scope
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cadmin, WHATFN);

    for (i = 0; i < num_cpush; i++) {
      cpush[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(cpushrefs[i]); // undo the refcount bump performed above
    }
    for (i = 0; i < num_cpull; i++) {
      cpull[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(cpullrefs[i]); // undo the refcount bump performed above
    }
    for (i = 0; i < num_apush; i++) {
      apush[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(apushrefs[i]); // undo the refcount bump performed above
    }
    for (i = 0; i < num_apull; i++) {
      apull[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(apullrefs[i]); // undo the refcount bump performed above
    }
    for (i = 0; i < num_spush; i++) {
      spush[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(spushrefs[i]); // undo the refcount bump performed above
    }
    for (i = 0; i < num_spull; i++) {
      spull[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(spullrefs[i]); // undo the refcount bump performed above
    }
    for (i = 0; i < num_bpush; i++) {
      bpush[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(bpushrefs[i]); // undo the refcount bump performed above
    }
    for (i = 0; i < num_bpull; i++) {
      bpull[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(bpullrefs[i]); // undo the refcount bump performed above
    }

  } // end temporary lock release scope
  if (!held.cadmin) {
    RDI_Fatal("ConsumerAdmin_i::obj_gc [**unexpected REACQUIRE failure**]\n");
  }
  if (_disposed) { return 0; }

  // now check whether we can be gc'd
  if (_serial != 0 && _num_proxies == 0 && deadAdmin && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadAdmin)) {
    RDIDbgCAdmLog("GC destroys cadmin " << _serial
		  << " curtime = " << curtime.fmt_local()
		  << " last_use = " << _last_use.fmt_local()
		  << "(diff = " << RDI_TIMET_DIFF_IN_SECS(_last_use, curtime)
		  << ") DeadAdminInterval = " << deadAdmin << '\n');
    _disconnect_clients_and_dispose(held, 0, 1, cadmin_lock.dispose_info); // arg 1 means update channel
    return 1;
  }
  return 0;
}
#endif

// RDINotifSupplier methods:
void
ConsumerAdmin_i::propagate_schange(RDI_LocksHeld&             held,
				   const CosN::EventTypeSeq&  added,
				   const CosN::EventTypeSeq&  deled,
				   Filter_i*                  filter)
{
  (void) _channel->update_mapping(held, added, deled, this, filter);
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::filter_destroy_i"
void
ConsumerAdmin_i::filter_destroy_i(Filter_i* filter)
{
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, return);
  _fa_helper.rem_filter_i(filter);
}

// -------------------- ** Private routines ** ----------------------------

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::_qos_changed"
void
ConsumerAdmin_i::_qos_changed(RDI_LocksHeld& held)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { return; }

  RDI_HashCursor<CosNA::ProxyID, ProxyPushSupplier_i *>           apushcur;
  RDI_HashCursor<CosNA::ProxyID, ProxyPullSupplier_i *>           apullcur;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushSupplier_i *> spushcur;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullSupplier_i *> spullcur;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushSupplier_i *>   bpushcur;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullSupplier_i *>   bpullcur;

  for ( apushcur = _prx_any_push.cursor(); apushcur.is_valid(); ++apushcur ) {
    apushcur.val()->_qos_changed(held);
  }
  for ( apullcur = _prx_any_pull.cursor(); apullcur.is_valid(); ++apullcur ) {
    apullcur.val()->_qos_changed(held);
  }
  for ( spushcur = _prx_struc_push.cursor(); spushcur.is_valid(); ++spushcur ) {
    spushcur.val()->_qos_changed(held);
  }
  for ( spullcur = _prx_struc_pull.cursor(); spullcur.is_valid(); ++spullcur ) {
    spullcur.val()->_qos_changed(held);
  }
  for ( bpushcur = _prx_batch_push.cursor(); bpushcur.is_valid(); ++bpushcur ) {
    bpushcur.val()->_qos_changed(held);
  }
  for ( bpullcur = _prx_batch_pull.cursor(); bpullcur.is_valid(); ++bpullcur ) {
    bpullcur.val()->_qos_changed(held);
  }
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::disconnect_clients_and_dispose"
void
ConsumerAdmin_i::disconnect_clients_and_dispose(CORBA::Boolean fast_destroy)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(cadmin_lock, held.cadmin, WHATFN);
  if (!held.cadmin) { return; }
  _disconnect_clients_and_dispose(held, fast_destroy, 0, cadmin_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::_disconnect_clients_and_dispose"
// *** requires caller has bumped scope lock, held.cadmin set
//  If update_channel is true, the admin unregisters itself.
//  If fast_destroy is true, can skip everything but the dispose step.
void
ConsumerAdmin_i::_disconnect_clients_and_dispose(RDI_LocksHeld&            held,
						 CORBA::Boolean            fast_destroy,
						 CORBA::Boolean            update_channel,
						 WRAPPED_DISPOSEINFO_PTR&  dispose_info)
{
  if (_disposed) {
    RDIDbgCAdmLog("ConsumerAdmin_i::_disconnect_clients_and_dispose() called more than once\n");
    return;
  }
  _disposed = 1; // acts as guard: the following is executed by only one thread 

  if (!fast_destroy) {

    if (update_channel) {
      // Unregister from the channel.  Must do before
      // the inuse loop below since channel has bumped us
      // while we are registered.

      { // introduce temporary lock release scope
	RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cadmin, WHATFN);
	// this is unlocked for rest of scope
	_channel->unregister(held, this);
      } // end temporary lock release scope
      if (!held.cadmin) {
	RDI_Fatal("ConsumerAdmin_i::_disconnect_clients_and_dispose [**unexpected REACQUIRE failure**]\n");
      }
    }

    // Wait for inuse to drop to one.  This ensures that any call that
    // bumps inuse and releases the lock has a chance to complete cleanly.
    while (_oplockptr->inuse() > 1) {
      RDI_OPLOCK_BROADCAST;
      RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
    }

    RDIDbgCAdmLog("Destruction of ConsumerAdmin " << (void*)this << " [" << _serial << "]\n");

    RDI_ListCursor<EventProxyPushSupplier_i *>                      cpushcur;
    RDI_ListCursor<EventProxyPullSupplier_i *>                      cpullcur;
    RDI_HashCursor<CosNA::ProxyID, ProxyPushSupplier_i *>           apushcur;
    RDI_HashCursor<CosNA::ProxyID, ProxyPullSupplier_i *>           apullcur;
    RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushSupplier_i *> spushcur;
    RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullSupplier_i *> spullcur;
    RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushSupplier_i *>   bpushcur;
    RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullSupplier_i *>   bpullcur;
    unsigned int i;

    // To do typemap updates (via proxy disconnect calls plus our own
    // remove_all_filters/update_mapping call)
    // we need to release our lock and then obtain locks in this order:
    //   channel -> typemap -> cadmin

    { // introduce scope to temporarily release bump lock
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.cadmin, WHATFN);
      { // introduce channel lock scope
	RDI_OPLOCK_SCOPE_LOCK_OTHER_TRACK(chan_lock, held.channel, _channel, WHATFN);
	if (!held.channel || _channel->shutting_down()) { goto skip_update; }
	{ // introduce typemap lock scope
	  RDI_CHAN_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(typemap_w_lock, held.typemap, _channel, WHATFN);
	  if (!held.typemap) { goto skip_update; }
	  { // introduce inner cadmin lock scope
	    RDI_OPLOCK_SCOPE_LOCK_TRACK(inner_cadmin_lock, held.cadmin, WHATFN);
	    if (!held.cadmin) { goto skip_update; }

	    // FINALLY we can do some real work.
	    cpushcur = _cosevent_push.cursor();
	    for (i=0; i < _cosevent_push.length(); ++i, ++cpushcur) {
	      EventProxyPushSupplier_i* prx = *cpushcur;
	      _removed_push_proxy(prx);
	      prx->disconnect_client_and_dispose(held, 0);
	    }
	    cpullcur = _cosevent_pull.cursor();
	    for (i=0; i < _cosevent_pull.length(); ++i, ++cpullcur) {
	      _removed_pull_proxy();
	      (*cpullcur)->disconnect_client_and_dispose(held, 0);
	    }
	    for ( apushcur = _prx_any_push.cursor(); apushcur.is_valid(); ++apushcur ) {
	      ProxyPushSupplier_i* prx = apushcur.val();
	      _removed_push_proxy(prx);
	      prx->disconnect_client_and_dispose(held, 0);
	    }
	    for ( apullcur = _prx_any_pull.cursor(); apullcur.is_valid(); ++apullcur ) {
	      _removed_pull_proxy();
	      apullcur.val()->disconnect_client_and_dispose(held, 0);
	    }
	    for ( spushcur = _prx_struc_push.cursor(); spushcur.is_valid(); ++spushcur ) {
	      StructuredProxyPushSupplier_i* prx = spushcur.val();
	      _removed_push_proxy(prx);
	      prx->disconnect_client_and_dispose(held, 0);
	    }
	    for ( spullcur = _prx_struc_pull.cursor(); spullcur.is_valid(); ++spullcur ) {
	      _removed_pull_proxy();
	      spullcur.val()->disconnect_client_and_dispose(held, 0);
	    }
	    for ( bpushcur = _prx_batch_push.cursor(); bpushcur.is_valid(); ++bpushcur ) {
	      SequenceProxyPushSupplier_i* prx = bpushcur.val();
	      _removed_push_proxy(prx);
	      prx->disconnect_client_and_dispose(held, 0);
	    }
	    for ( bpullcur = _prx_batch_pull.cursor(); bpullcur.is_valid(); ++bpullcur ) {
	      _removed_pull_proxy();
	      bpullcur.val()->disconnect_client_and_dispose(held, 0);
	    }

	    if ( _rqstypes.length() == 0 ) {
	      _fa_helper.remove_all_filters(held, (RDINotifySubscribe_ptr) this); // this => DO want final propagate_schange callbacks
	    } else {
	      CosN::EventTypeSeq added; added.length(0);
	      (void) _channel->update_mapping(held, added, _rqstypes, this, 0);
	    }

	  } // end inner cadmin lock scope
	} // end typemap lock scope
      } // end channel lock scope
    } // end temporary release scope

  skip_update:
    _cosevent_push.drain();  _cosevent_pull.drain();
    _prx_any_push.clear();   _prx_any_pull.clear();
    _prx_struc_push.clear(); _prx_struc_pull.clear();
    _prx_batch_push.clear(); _prx_batch_pull.clear();

    if ( ! CORBA::is_nil(_prio_filter) ) CORBA::release(_prio_filter);
    if ( ! CORBA::is_nil(_life_filter) ) CORBA::release(_life_filter);

  } // // end of !fast_destroy code

  if ( _qosprop ) {
    delete _qosprop; _qosprop = 0;
  }
  RDIDbgCAdmLog("ConsumerAdmin @ " << (void*)this << " [" << _serial << "] destroyed\n");
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

// NB ** Assumes oplock is held **
void
ConsumerAdmin_i::_removed_push_proxy(RDIProxyPushSupplier* proxy)
{
  _num_proxies -= 1;
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _channel->decr_consumers();
  RDI_NotifyConsumer* cpc = _channel->push_consumer();
  if ( cpc ) {       // Deregister Proxy
    cpc->remove_proxy(proxy);
  }
}

// NB ** Assumes oplock is held **
void
ConsumerAdmin_i::_removed_pull_proxy()
{
  _num_proxies -= 1;
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _channel->decr_consumers();
}

// OPLOCK is acquired/released by caller [EventChannel_i::admin_dispatch()]
void
ConsumerAdmin_i::dispatch_event(RDI_StructuredEvent*  event)
{
  RDI_ListCursor<EventProxyPushSupplier_i *> psc;
  RDI_ListCursor<EventProxyPullSupplier_i *> plc;
  unsigned int i=0;
  for (i=0,psc=_cosevent_push.cursor(); i<_cosevent_push.length(); ++i,++psc) {
    (*psc)->add_event(event);
  }
  for (i=0,plc=_cosevent_pull.cursor(); i<_cosevent_pull.length(); ++i,++plc) {
    (*plc)->add_event(event);
  }
}

// typemap --> cadmin locks acquired/released by caller
//  [EventChannel_i::admin_dispatch() or EventChannel_i::proxy_dispatch()]
#undef WHATFN
#define WHATFN "ConsumerAdmin_i::dispatch_event"
void
ConsumerAdmin_i::dispatch_event(RDI_StructuredEvent*  event,
				RDI_FilterState_t     astat, 
				RDI_TypeMap*          tmap)
{
  RDI_HashCursor<CosNA::ProxyID, ProxyPushSupplier_i *>           apushcur;
  RDI_HashCursor<CosNA::ProxyID, ProxyPullSupplier_i *>           apullcur;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushSupplier_i *> spushcur;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullSupplier_i *> spullcur;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushSupplier_i *>   bpushcur;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullSupplier_i *>   bpullcur;
  RDI_TypeMap::FNode_t* fnode=0;
  RDI_TypeMap::FList_t flist;

  // When consumers use 'subscription_change()'  to register 
  // interest in specific event types, we have 'has_filters()' return true
  // so that we execute the filter evaluation logic. However, there are no
  // filters registered and, hence, we need the '!fnode->_fltr' below...

  if ( _num_proxies == 0 ) {
    return;
  }
  const char* dname = event->get_domain_name();
  const char* tname = event->get_type_name();

  // Evaluate Filters for consumers using ProxyPushSupplier_i

  for (apushcur = _prx_any_push.cursor(); apushcur.is_valid(); ++apushcur ) {
    if ( astat == OrMatch ) {
      apushcur.val()->add_event(event);
      continue;
    }
    if ( ! apushcur.val()->has_filters() ) {
      if ( (astat == NoFilters) || (astat == AndMatch) ) {
	apushcur.val()->add_event(event);
      }
      continue;
    }

    tmap->lookup(dname, tname, apushcur.val(), flist);
    if ( ! flist._star_star && ! flist._domn_star &&
	 ! flist._star_type && ! flist._domn_type ) {
      if ( astat == OrMatch ) {
	apushcur.val()->add_event(event);
      }
      continue;
    }
    for ( fnode=flist._star_star; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	apushcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._domn_star; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	apushcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._star_type; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	apushcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._domn_type; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	apushcur.val()->add_event(event);
	continue;
      }
    }
  }

  // Evaluate Filters for consumers using ProxyPullSupplier_i

  for (apullcur = _prx_any_pull.cursor(); apullcur.is_valid(); ++apullcur ) {
    if ( astat == OrMatch ) {
      apullcur.val()->add_event(event);
      continue;
    }
    if ( ! apullcur.val()->has_filters() ) {
      if ( (astat == NoFilters) || (astat == AndMatch) ) {
	apullcur.val()->add_event(event);
      }
      continue;
    }

    tmap->lookup(dname, tname, apullcur.val(), flist);
    if ( ! flist._star_star && ! flist._domn_star &&
	 ! flist._star_type && ! flist._domn_type ) {
      if ( astat == OrMatch ) {
	apullcur.val()->add_event(event);
      }
      continue;
    }
    for ( fnode=flist._star_star; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	apullcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._domn_star; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	apullcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._star_type; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	apullcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._domn_type; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	apullcur.val()->add_event(event);
	continue;
      }
    }
  }

  // Evaluate Filters for consumers using StructuredProxyPushSupplier_i

  for ( spushcur=_prx_struc_push.cursor(); spushcur.is_valid(); ++spushcur ) {
    if ( astat == OrMatch ) {
      spushcur.val()->add_event(event);
      continue;
    }
    if ( ! spushcur.val()->has_filters() ) {
      if ( (astat == NoFilters) || (astat == AndMatch) ) {
	spushcur.val()->add_event(event);
      }
      continue;
    }

    tmap->lookup(dname, tname, spushcur.val(), flist);
    if ( ! flist._star_star && ! flist._domn_star &&
	 ! flist._star_type && ! flist._domn_type ) {
      if ( astat == OrMatch ) {
	spushcur.val()->add_event(event);
      }
      continue;
    }
    for ( fnode=flist._star_star; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	spushcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._domn_star; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	spushcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._star_type; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	spushcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._domn_type; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	spushcur.val()->add_event(event);
	continue;
      }
    }
  }

  // Evaluate Filters for consumers using StructuredProxyPullSupplier_i

  for ( spullcur=_prx_struc_pull.cursor(); spullcur.is_valid(); ++spullcur ) {
    if ( astat == OrMatch ) {
      spullcur.val()->add_event(event);
      continue;
    }
    if ( ! spullcur.val()->has_filters() ) {
      if ( (astat == NoFilters) || (astat == AndMatch) ) {
	spullcur.val()->add_event(event);
      }
      continue;
    }

    tmap->lookup(dname, tname, spullcur.val(), flist);
    if ( ! flist._star_star && ! flist._domn_star &&
	 ! flist._star_type && ! flist._domn_type ) {
      if ( astat == OrMatch ) {
	spullcur.val()->add_event(event);
      }
      continue;
    }
    for ( fnode=flist._star_star; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	spullcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._domn_star; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	spullcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._star_type; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	spullcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._domn_type; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	spullcur.val()->add_event(event);
	continue;
      }
    }
  }

  // Evaluate Filters for consumers using SequenceProxyPushSupplier_i

  for ( bpushcur=_prx_batch_push.cursor(); bpushcur.is_valid(); ++bpushcur ) {
    if ( astat == OrMatch ) {
      bpushcur.val()->add_event(event);
      continue;
    }
    if ( ! bpushcur.val()->has_filters() ) {
      if ( (astat == NoFilters) || (astat == AndMatch) ) {
	bpushcur.val()->add_event(event);
      }
      continue;
    }

    tmap->lookup(dname, tname, bpushcur.val(), flist);
    if ( ! flist._star_star && ! flist._domn_star &&
	 ! flist._star_type && ! flist._domn_type ) {
      if ( astat == OrMatch ) {
	bpushcur.val()->add_event(event);
      }
      continue;
    }
    for ( fnode=flist._star_star; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	bpushcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._domn_star; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	bpushcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._star_type; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	bpushcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._domn_type; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	bpushcur.val()->add_event(event);
	continue;
      }
    }
  }

  // Evaluate Filters for consumers using SequenceProxyPullSupplier_i

  for ( bpullcur=_prx_batch_pull.cursor(); bpullcur.is_valid(); ++bpullcur ) {
    if ( astat == OrMatch ) {
      bpullcur.val()->add_event(event);
      continue;
    }
    if ( ! bpullcur.val()->has_filters() ) {
      if ( (astat == NoFilters) || (astat == AndMatch) ) {
	bpullcur.val()->add_event(event);
      }
      continue;
    }

    tmap->lookup(dname, tname, bpullcur.val(), flist);
    if ( ! flist._star_star && ! flist._domn_star &&
	 ! flist._star_type && ! flist._domn_type ) {
      if ( astat == OrMatch ) {
	bpullcur.val()->add_event(event);
      }
      continue;
    }
    for ( fnode=flist._star_star; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	bpullcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._domn_star; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	bpullcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._star_type; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	bpullcur.val()->add_event(event);
	continue;
      }
    }
    for ( fnode=flist._domn_type; fnode; fnode=fnode->_next ) {
      if (!fnode->_fltr || fnode->_fltr->rdi_match(event,_channel)) {
	bpullcur.val()->add_event(event);
	continue;
      }
    }
  }
}

// ------------------------------------------------------------- //
// SupplierAdmin_i implementation                                //
// ------------------------------------------------------------- //

SupplierAdmin_i::SupplierAdmin_i(EventChannel_i* channel,
				 CosNA::InterFilterGroupOperator op,
				 const CosNA::AdminID& serial) :
  _oplockptr(0), _my_name(channel->L_my_name()), _disposed(0),
  _fa_helper("sadmin_fa_helper"), _channel(channel), _qosprop(0), 
  _serial(serial), _and_or_oper(op), _prx_serial(1), 
  _num_proxies(0), 
  _evtypes(RDI_EventType::hash, RDI_EventType::rank),
  _cosevent_push(), _cosevent_pull(),
  _prx_any_push(RDI_CorbaULongHash, RDI_CorbaULongRank),
  _prx_any_pull(RDI_CorbaULongHash, RDI_CorbaULongRank),
  _prx_struc_push(RDI_CorbaULongHash, RDI_CorbaULongRank),
  _prx_struc_pull(RDI_CorbaULongHash, RDI_CorbaULongRank),
  _prx_batch_push(RDI_CorbaULongHash, RDI_CorbaULongRank),
  _prx_batch_pull(RDI_CorbaULongHash, RDI_CorbaULongRank)
{
  RDI_OPLOCK_INIT("sadmin");
  _qosprop = new RDI_NotifQoS(_channel->qos_properties());
  RDI_AssertAllocThrowNo(_qosprop, "Memory allocation failure - RDI_NotifQoS\n");
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  char buf[20];
  sprintf(buf, "sadmin%ld", (long)serial);
  _my_name.length(_my_name.length()+1);
  _my_name[_my_name.length()-1] = (const char*)buf;
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
  _my_oref = WRAPPED_IMPL2OREF(AttN::SupplierAdmin, this);
}

SupplierAdmin_i::~SupplierAdmin_i() {
  // RDIDbgForceLog("XXX_REMOVE  ~SupplierAdmin_i() called for sadmin " << _serial << '\n');
  RDI_OPLOCK_DESTROY_CHECK("SupplierAdmin_i");
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::MyID"
CosNA::AdminID
SupplierAdmin_i::MyID( WRAPPED_IMPLARG_VOID ) 
{
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosNA::AdminID res = _serial;
  return res;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::MyChannel"
CosNA::EventChannel_ptr
SupplierAdmin_i::MyChannel( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosNA::EventChannel_ptr res = WRAPPED_IMPL2OREF(CosNA::EventChannel, _channel);
  return res;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::MyOperator"
CosNA::InterFilterGroupOperator
SupplierAdmin_i::MyOperator( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosNA::InterFilterGroupOperator res = _and_or_oper;
  return res;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::obtain_push_consumer"
CosEvCA::ProxyPushConsumer_ptr 
SupplierAdmin_i::obtain_push_consumer( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosEvCA::ProxyPushConsumer_ptr res = CosEvCA::ProxyPushConsumer::_nil();
  EventProxyPushConsumer_i* prx=0;
  RDIDbgSAdmLog("CosEvent ProxyPushConsumer creation requested\n");
  if ( ! _channel->incr_suppliers() ) {
    // return null res
  } else if ( ! (prx = new EventProxyPushConsumer_i(this, _channel, _prx_serial)) ) {
    _channel->decr_suppliers();
    // return null res
  } else if ( _cosevent_push.insert_tail(prx) != 0 ) {
    _channel->decr_suppliers();
    prx->disconnect_client_and_dispose(held, 0);
    // return null res
  } else { // return valid res
    _prx_serial++;
    _num_proxies++;
    res = WRAPPED_IMPL2OREF(CosEvCA::ProxyPushConsumer, prx);
  }
  return res;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::obtain_pull_consumer"
CosEvCA::ProxyPullConsumer_ptr
SupplierAdmin_i::obtain_pull_consumer( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosEvCA::ProxyPullConsumer_ptr res = 
    CosEvCA::ProxyPullConsumer::_nil();
  EventProxyPullConsumer_i* prx=0;
  RDIDbgSAdmLog("CosEvent ProxyPullConsumer creation requested\n");
  if ( ! _channel->incr_suppliers() ) {
    // return null res
  } else if ( ! (prx = new EventProxyPullConsumer_i(this, _channel, _prx_serial)) ) {
    _channel->decr_suppliers();
    // return null res
  } else if ( _cosevent_pull.insert_tail(prx) != 0 ) {
    _channel->decr_suppliers();
    prx->disconnect_client_and_dispose(held, 0);
    // return null res
  } else { // return valid res
    _prx_serial++;
    _num_proxies++;
    RDI_PullSupplier* cps = _channel->pull_supplier();
    if ( cps ) {      // Register Pull Proxy
      cps->insert_proxy(prx);
    }
    res = WRAPPED_IMPL2OREF(CosEvCA::ProxyPullConsumer, prx);
  }
  return res;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::offer_change"
// Called externally -- not clear that suppliers should be doing this
void
SupplierAdmin_i::offer_change(const CosN::EventTypeSeq& added,
			      const CosN::EventTypeSeq& deled   WRAPPED_IMPLARG )
{
  // this code assumes it is OK for suppliers to do this
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed

  CosN::EventTypeSeq added_copy = added;
  CosN::EventTypeSeq deled_copy = deled;

  // First, validate the entries in the provided lists to make sure
  // that correct values are provided for these event types .......

  CORBA::ULong vl=0;
  if ( ! RDI_EventType::valid_sequence(added_copy, vl) ) {
    throw CosNC::InvalidEventType(added[vl]);
  }
  if ( ! RDI_EventType::valid_sequence(deled_copy, vl) ) {
    throw CosNC::InvalidEventType(deled[vl]);
  }

  // The same call a proxy uses
  _propagate_ochange(held, added_copy, deled_copy);
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::pull_consumers"
CosNA::ProxyIDSeq*
SupplierAdmin_i::pull_consumers( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  RDI_HashCursor<CosNA::ProxyID, ProxyPullConsumer_i *>           ac;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullConsumer_i *> sc;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullConsumer_i *>   bc;
  unsigned long num;
  CosNA::ProxyIDSeq*  seq = new CosNA::ProxyIDSeq();
  RDI_AssertAllocThrowNo(seq, "Memory allocation failed -- ProxyIDSeq\n");
  num = _prx_any_pull.length() +
    _prx_struc_pull.length() + _prx_batch_pull.length();
  seq->length(num);
  for ( num=0, ac = _prx_any_pull.cursor(); ac.is_valid(); ++ac, ++num ) {
    (*seq)[num] = ac.key();
  }
  for ( sc = _prx_struc_pull.cursor(); sc.is_valid(); ++sc, ++num ) {
    (*seq)[num] = sc.key();
  }
  for ( bc = _prx_batch_pull.cursor(); bc.is_valid(); ++bc, ++num ) {
    (*seq)[num] = bc.key();
  }
  return seq;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::push_consumers"
CosNA::ProxyIDSeq*
SupplierAdmin_i::push_consumers( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  RDI_HashCursor<CosNA::ProxyID, ProxyPushConsumer_i *>           ac;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushConsumer_i *> sc;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushConsumer_i *>  bc;
  unsigned long num;
  CosNA::ProxyIDSeq*  seq = new CosNA::ProxyIDSeq();
  RDI_AssertAllocThrowNo(seq, "Memory allocation failed -- ProxyIDSeq\n");
  num = _prx_any_push.length() +
    _prx_struc_push.length() + _prx_batch_push.length();
  seq->length(num);
  for ( num=0, ac = _prx_any_push.cursor(); ac.is_valid(); ++ac, ++num ) {
    (*seq)[num] = ac.key();
  }
  for ( sc = _prx_struc_push.cursor(); sc.is_valid(); ++sc, ++num ) {
    (*seq)[num] = sc.key();        
  }
  for ( bc = _prx_batch_push.cursor(); bc.is_valid(); ++bc, ++num ) {
    (*seq)[num] = bc.key();
  }
  return seq;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::get_proxy_consumer"
CosNA::ProxyConsumer_ptr
SupplierAdmin_i::get_proxy_consumer(CosNA::ProxyID proxy_id   WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  ProxyPushConsumer_i*           apush=0;
  ProxyPullConsumer_i*           apull=0;
  StructuredProxyPushConsumer_i* spush=0;
  SequenceProxyPushConsumer_i*   bpush=0;
  StructuredProxyPullConsumer_i* spull=0;
  SequenceProxyPullConsumer_i*   bpull=0;

  CosNA::ProxyConsumer_ptr res = CosNA::ProxyConsumer::_nil();
  if ( proxy_id <= _prx_serial ) {
    if ( _prx_any_push.lookup(proxy_id, apush) ) {
      res = WRAPPED_IMPL2OREF(CosNA::ProxyConsumer, apush);
    } else if ( _prx_any_pull.lookup(proxy_id, apull) ) {
      res = WRAPPED_IMPL2OREF(CosNA::ProxyConsumer, apull);
    } else if ( _prx_struc_push.lookup(proxy_id, spush) ) {
      res = WRAPPED_IMPL2OREF(CosNA::ProxyConsumer, spush);
    } else if ( _prx_struc_pull.lookup(proxy_id, spull) ) {
      res = WRAPPED_IMPL2OREF(CosNA::ProxyConsumer, spull);
    } else if ( _prx_batch_push.lookup(proxy_id, bpush) ) {
      res = WRAPPED_IMPL2OREF(CosNA::ProxyConsumer, bpush);
    } else if ( _prx_batch_pull.lookup(proxy_id, bpull) ) {
      res = WRAPPED_IMPL2OREF(CosNA::ProxyConsumer, bpull);
    }
  }
  if (CORBA::is_nil(res)) {
    throw CosNA::ProxyNotFound();
  }
  return res;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::obtain_notification_pull_consumer"
CosNA::ProxyConsumer_ptr
SupplierAdmin_i::obtain_notification_pull_consumer(CosNA::ClientType ctype,
						   CosNA::ProxyID& proxy_id
                                                   WRAPPED_IMPLARG )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed

  RDIDbgSAdmLog("Creation of a new ProxyConsumer -- type " << ctype << '\n');

  // Check with  the channel if we are about to exceed the maximum
  // number of suppliers supported by the channel
  if ( ! _channel->incr_suppliers() ) {
    CosNA::AdminLimit limit;
    RDIDbgSAdmLog("Supplier limit has been reached creation failed\n");
    limit.name    = (const char *) "MaxSuppliers";
    limit.value <<= _channel->max_suppliers();
    throw CosNA::AdminLimitExceeded(limit);
  }

  CosNA::ProxyConsumer_ptr res = CosNA::ProxyConsumer::_nil();
  if ( ctype == CosNA::ANY_EVENT ) {
    ProxyPullConsumer_i* prx = 0;
    if ( ! (prx = new ProxyPullConsumer_i(this, _channel, _prx_serial)) ) {
      _channel->decr_suppliers();
    } else {
      proxy_id = _prx_serial++;
      if ( _prx_any_pull.insert(proxy_id, prx) != 0 ) {
	prx->disconnect_client_and_dispose(held, 0);
	_channel->decr_suppliers(); 
      } else {
	RDI_PullSupplier* cps = _channel->pull_supplier();
	if ( cps ) {   // Register Pull Proxy
	  cps->insert_proxy(prx);
	}
	_num_proxies += 1;
	res = WRAPPED_IMPL2OREF(CosNA::ProxyConsumer, prx);
      }
    }
  } else if ( ctype == CosNA::STRUCTURED_EVENT ) {
    StructuredProxyPullConsumer_i* prx = 
      new StructuredProxyPullConsumer_i(this,_channel,_prx_serial);
    if ( ! prx ) {
      _channel->decr_suppliers();
    } else {
      proxy_id = _prx_serial++;
      if ( _prx_struc_pull.insert(proxy_id, prx) != 0 ) {
	prx->disconnect_client_and_dispose(held, 0);
	_channel->decr_suppliers();
      } else {
	RDI_PullSupplier* cps = _channel->pull_supplier();
	if ( cps ) {   // Register Pull Proxy
	  cps->insert_proxy(prx);
	}
	_num_proxies += 1;
	res = WRAPPED_IMPL2OREF(CosNA::ProxyConsumer, prx);
      }
    }
  } else if ( ctype == CosNA::SEQUENCE_EVENT ) {
    SequenceProxyPullConsumer_i* prx =
      new SequenceProxyPullConsumer_i(this,_channel,_prx_serial);
    if ( ! prx ) {
      _channel->decr_suppliers();
    } else {
      proxy_id = _prx_serial++;
      if ( _prx_batch_pull.insert(proxy_id, prx) != 0 ) {
	prx->disconnect_client_and_dispose(held, 0);
	_channel->decr_suppliers();
      }  else {
	RDI_PullSupplier* cps = _channel->pull_supplier();
	if ( cps ) {     // Register Pull Proxy
	  cps->insert_proxy(prx);
	}
	_num_proxies += 1;
	res = WRAPPED_IMPL2OREF(CosNA::ProxyConsumer, prx);
      }
    }
  }
  return res;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::obtain_notification_push_consumer"
CosNA::ProxyConsumer_ptr
SupplierAdmin_i::obtain_notification_push_consumer(CosNA::ClientType ctype,
						   CosNA::ProxyID& proxy_id
                                                   WRAPPED_IMPLARG )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed

  RDIDbgSAdmLog("Creation of a new ProxyConsumer -- type " << ctype << '\n');

  // Check with  the channel if we are about to exceed the maximum
  // number of suppliers supported by the channel
  if ( ! _channel->incr_suppliers() ) {
    CosNA::AdminLimit limit;
    RDIDbgSAdmLog("Supplier limit has been reached creation failed\n");
    limit.name    = (const char *) "MaxSuppliers";
    limit.value <<= _channel->max_suppliers();
    throw CosNA::AdminLimitExceeded(limit);
  }

  CosNA::ProxyConsumer_ptr res = CosNA::ProxyConsumer::_nil();
  if ( ctype == CosNA::ANY_EVENT ) {
    ProxyPushConsumer_i* prx = 0;
    if ( ! (prx = new ProxyPushConsumer_i(this, _channel, _prx_serial)) ) {
      _channel->decr_suppliers();
    } else {
      proxy_id = _prx_serial++;
      if ( _prx_any_push.insert(proxy_id, prx) != 0 ) {
	prx->disconnect_client_and_dispose(held, 0);
	_channel->decr_suppliers(); 
      } else {
	_num_proxies += 1;
	res = WRAPPED_IMPL2OREF(CosNA::ProxyConsumer, prx);
      }
    }
  } else if ( ctype == CosNA::STRUCTURED_EVENT ) {
    StructuredProxyPushConsumer_i* prx =
      new StructuredProxyPushConsumer_i(this,_channel,_prx_serial);
    if ( ! prx ) {
      _channel->decr_suppliers();
    } else {
      proxy_id = _prx_serial++;
      if ( _prx_struc_push.insert(proxy_id, prx) != 0 ) {
	prx->disconnect_client_and_dispose(held, 0);
	_channel->decr_suppliers();
      } else {
	_num_proxies += 1;
	res = WRAPPED_IMPL2OREF(CosNA::ProxyConsumer, prx);
      }
    }
  } else if ( ctype == CosNA::SEQUENCE_EVENT ) {
    SequenceProxyPushConsumer_i* prx = 
      new SequenceProxyPushConsumer_i(this,_channel,_prx_serial);
    if ( ! prx ) {
      _channel->decr_suppliers();
    } else {
      proxy_id = _prx_serial++;
      if ( _prx_batch_push.insert(proxy_id, prx) != 0 ) {
	prx->disconnect_client_and_dispose(held, 0);
	_channel->decr_suppliers();
      } else {
	_num_proxies += 1;
	res = WRAPPED_IMPL2OREF(CosNA::ProxyConsumer, prx);
      }
    }
  }
  return res;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::safe_cleanup"
CORBA::Boolean
SupplierAdmin_i::safe_cleanup( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { RDI_THROW_INV_OBJREF; }
  if ( _serial == 0 ) { // Default SupplierAdmin not a cleanup candidate
    return 0;
  }
  if (_num_proxies > 0) { // Admin with proxies not a cleanup candidate
    return 0;
  }
  _disconnect_clients_and_dispose(held, 0, 1, sadmin_lock.dispose_info); // arg 1 means update channel
  return 1;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::destroy"
void
SupplierAdmin_i::destroy( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { RDI_THROW_INV_OBJREF; }
  if ( _serial == 0 ) {
    RDIDbgSAdmLog("Default SupplierAdmin is not allowed to be destroyed!\n");
    return;
  }
  _disconnect_clients_and_dispose(held, 0, 1, sadmin_lock.dispose_info); // 1 means update channel
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::get_qos"
CosN::QoSProperties*
SupplierAdmin_i::get_qos( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  //RDI_NotifQoS*   qosp = qos_properties();
  //RDI_Assert(qosp, "Event Channel should have RDI_NotifQoS set\n");
  //CosN::QoSProperties* res = qosp->get_qos(RDI_C_ADMIN);
  CosN::QoSProperties* res = _qosprop->get_qos(RDI_C_ADMIN);
  return res;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::set_qos"
void
SupplierAdmin_i::set_qos(const CosN::QoSProperties& r_qos   WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  if (r_qos.length() == 0) return;
  CosN::PropertyErrorSeq eseq;
  CosN::NamedPropertyRangeSeq rseq;
  CORBA::Boolean  subobjs = (_num_proxies ? 1 : 0);
  
  if (! RDI_NotifQoS::validate(r_qos, *_qosprop, RDI_C_ADMIN, eseq, rseq, subobjs)) {
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
#define WHATFN "SupplierAdmin_i::validate_qos"
void
SupplierAdmin_i::validate_qos(const CosN::QoSProperties& r_qos,
			      CosN_NamedPropertyRangeSeq_outarg rseq 
			      WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosN::PropertyErrorSeq eseq;
  CORBA::Boolean  subobjs = (_num_proxies ? 1 : 0);

  rseq = new CosNotification::NamedPropertyRangeSeq();
  RDI_AssertAllocThrowNo((CosNotification::NamedPropertyRangeSeq*)rseq,
			 "Memory allocation failed - NamedPropertyRangeSeq\n");
  if (! RDI_NotifQoS::validate(r_qos, *_qosprop, RDI_C_ADMIN, eseq, *rseq, subobjs)) {
    throw CosN::UnsupportedQoS(eseq);
  }
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::add_filter"
CosNF::FilterID
SupplierAdmin_i::add_filter(CosNF::Filter_ptr fltr WRAPPED_IMPLARG )
{
  RDI_LocksHeld    held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  return _fa_helper.add_filter_i(held, fltr, (RDINotifySubscribe_ptr) this, 0); // 0 => do NOT want propagate_schange callbacks
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::remove_filter"
void
SupplierAdmin_i::remove_filter(CosNF::FilterID fltrID WRAPPED_IMPLARG )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  _fa_helper.remove_filter(held, fltrID, 0);
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::get_filter"
CosNF::Filter_ptr
SupplierAdmin_i::get_filter(CosNF::FilterID fltrID WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosNF::Filter_ptr res = _fa_helper.get_filter(fltrID);
  return CosNF::Filter::_duplicate(res);
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::get_all_filters"
CosNF::FilterIDSeq*
SupplierAdmin_i::get_all_filters( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  CosNF::FilterIDSeq* res = _fa_helper.get_all_filters();
  return res;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::remove_all_filters"
void
SupplierAdmin_i::remove_all_filters( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { RDI_THROW_INV_OBJREF; }
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  _fa_helper.remove_all_filters(held, 0); // 0 => do NOT want propagate_schange callback
}

// -------------------- ** Local-only routines ** ----------------------------

#undef WHATFN
#define WHATFN "SupplierAdmin_i::propagate_ochange"
void
SupplierAdmin_i::propagate_ochange(RDI_LocksHeld&             held,
				   const CosN::EventTypeSeq&  added,
				   const CosN::EventTypeSeq&  deled)
{
  RDI_OPLOCK_BUMP_COND_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { return; }
  _propagate_ochange(held, added, deled);
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::remove_proxy_1"
void
SupplierAdmin_i::remove_proxy(RDI_LocksHeld& held, EventProxyPushConsumer_i* prx)
{
  RDI_ListCursor<EventProxyPushConsumer_i *> lc;
  unsigned int    indx = 0;
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { return; }
 
  for ( lc = _cosevent_push.cursor(); indx < _cosevent_push.length(); indx++ ) {
    EventProxyPushConsumer_i* proxy = *lc;
    if ( proxy == prx ) {
      _cosevent_push.remove(lc);
      _removed_push_proxy();
      break;
    }
    ++lc;
  }
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::remove_proxy_2"
void
SupplierAdmin_i::remove_proxy(RDI_LocksHeld& held, EventProxyPullConsumer_i* prx)
{
  RDI_ListCursor<EventProxyPullConsumer_i *> lc;
  unsigned int    indx = 0;
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { return; }
 
  for ( lc = _cosevent_pull.cursor(); indx < _cosevent_pull.length(); indx++ ) {
    EventProxyPullConsumer_i* proxy = *lc;
    if ( proxy == prx ) {
      _cosevent_pull.remove(lc);
      _removed_pull_proxy(prx);
      break;
    }
    ++lc;
  }
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::remove_proxy_3"
void
SupplierAdmin_i::remove_proxy(RDI_LocksHeld& held, ProxyPushConsumer_i* prx)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { return; }
  RDIDbgSAdmLog("Remove proxy [" << prx->_proxy_id() << "] from admin " << (void*)this << '\n');
  if ( _prx_any_push.exists(prx->_proxy_id()) ) {
    _prx_any_push.remove(prx->_proxy_id());
    _removed_push_proxy();
  } else {
    RDIDbgSAdmLog("Invalid proxy [" << prx->_proxy_id() << "] provided\n");
  }
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::remove_proxy_4"
void
SupplierAdmin_i::remove_proxy(RDI_LocksHeld& held, ProxyPullConsumer_i* prx)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { return; }
  RDIDbgSAdmLog("Remove proxy [" << prx->_proxy_id() << "] from admin " << (void*)this << '\n');
  if ( _prx_any_pull.exists(prx->_proxy_id()) ) {
    _prx_any_pull.remove(prx->_proxy_id());
    _removed_pull_proxy(prx);
  } else {
    RDIDbgSAdmLog("Invalid proxy [" << prx->_proxy_id() << "] provided\n");
  }
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::remove_proxy_5"
void
SupplierAdmin_i::remove_proxy(RDI_LocksHeld& held, StructuredProxyPushConsumer_i* prx)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { return; }
  RDIDbgSAdmLog("Remove proxy [" << prx->_proxy_id() << "] from admin " << (void*)this << '\n');
  if ( _prx_struc_push.exists(prx->_proxy_id()) ) {
    _prx_struc_push.remove(prx->_proxy_id());
    _removed_push_proxy();
  } else {
    RDIDbgSAdmLog("Invalid proxy [" << prx->_proxy_id() << "] provided\n");
  }
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::remove_proxy_6"
void
SupplierAdmin_i::remove_proxy(RDI_LocksHeld& held, StructuredProxyPullConsumer_i* prx)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { return; }
  RDIDbgSAdmLog("Remove proxy [" << prx->_proxy_id() << "] from admin " << (void*)this << '\n');
  if ( _prx_struc_pull.exists(prx->_proxy_id()) ) {
    _prx_struc_pull.remove(prx->_proxy_id());
    _removed_pull_proxy(prx);
  } else {
    RDIDbgSAdmLog("Invalid proxy [" << prx->_proxy_id() << "] provided\n");
  }
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::remove_proxy_7"
void
SupplierAdmin_i::remove_proxy(RDI_LocksHeld& held, SequenceProxyPushConsumer_i* prx)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { return; }
  RDIDbgSAdmLog("Remove proxy [" << prx->_proxy_id() << "] from admin " << (void*)this << '\n');
  if ( _prx_batch_push.exists(prx->_proxy_id()) ) {
    _prx_batch_push.remove(prx->_proxy_id());
    _removed_push_proxy();
  } else {
    RDIDbgSAdmLog("Invalid proxy [" << prx->_proxy_id() << "] provided\n");
  }
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::remove_proxy_8"
void
SupplierAdmin_i::remove_proxy(RDI_LocksHeld& held, SequenceProxyPullConsumer_i* prx)
{
  RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { return; }
  RDIDbgSAdmLog("Remove proxy [" << prx->_proxy_id() << "] from admin " << (void*)this << '\n');
  if ( _prx_batch_pull.exists(prx->_proxy_id()) ) {
    _prx_batch_pull.remove(prx->_proxy_id());
    _removed_pull_proxy(prx);
  } else {
    RDIDbgSAdmLog("Invalid proxy [" << prx->_proxy_id() << "] provided\n");
  }
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::match_event_1"
CORBA::Boolean
SupplierAdmin_i::match_event(const CORBA::Any& event)
{
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, return 0);
  RDIDbgSAdmLog("XXX SupplierAdmin_i::match_event(any) called\n");
  CORBA::Boolean matched = 1;
  if ( _fa_helper.has_filters() ) {
    CosNF::FilterIDSeq* filterseq = _fa_helper.get_all_filters();
    CosNF::Filter_ptr   filter = CosNF::Filter::_nil();
    Filter_i* rdfilter = 0;
    matched = 0;
    for (CORBA::ULong ix=0; ix < filterseq->length(); ix++) {
      filter = _fa_helper.get_filter((*filterseq)[ix]);
      rdfilter = Filter_i::Filter2Filter_i(filter);
      if (rdfilter) // XXX
	RDIDbgSAdmLog("XXX SupplierAdmin_i::match_event(any) calling rdfilter->match_chan\n");
      else // XXX
	RDIDbgSAdmLog("XXX SupplierAdmin_i::match_event(any) calling filter->match\n");
      if ( (rdfilter && rdfilter->match_chan(event, _channel)) ||
	   (!rdfilter && filter->match(event)) ) {
	matched = 1;
	break;
      }
    }
    delete filterseq;
  }
  return matched;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::match_event_2"
CORBA::Boolean
SupplierAdmin_i::match_event(const CosN::StructuredEvent* event, RDI_StructuredEvent* sevnt)
{
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, return 0);
  RDIDbgSAdmLog("XXX SupplierAdmin_i::match_event(event, sevnt) called\n");
  CORBA::Boolean matched = 1;
  if ( _fa_helper.has_filters() ) {
    CosNF::FilterIDSeq* filterseq = _fa_helper.get_all_filters();
    CosNF::Filter_ptr filter = CosNF::Filter::_nil();
    Filter_i* rdfilter = 0;
    matched = 0;
    for (CORBA::ULong ix=0; ix < filterseq->length(); ix++) {
      filter = _fa_helper.get_filter((*filterseq)[ix]);
      rdfilter = Filter_i::Filter2Filter_i(filter);
      if (rdfilter) // XXX
	RDIDbgSAdmLog("XXX SupplierAdmin_i::match_event calling rdfilter->rdi_match\n");
      else // XXX
	RDIDbgSAdmLog("XXX SupplierAdmin_i::match_event calling filter->match_structured\n");
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

#ifndef NO_OBJ_GC

#undef WHATFN
#define WHATFN "SupplierAdmin_i::gc_able"
CORBA::Boolean
SupplierAdmin_i::gc_able(RDI_TimeT curtime, CORBA::ULong deadAdmin)
{
  RDI_OPLOCK_BUMP_SCOPE_LOCK(sadmin_lock, WHATFN, return 0);
  if (_disposed) { return 0; }
  return (_num_proxies == 0 && deadAdmin && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadAdmin));
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::obj_gc"
CORBA::Boolean
SupplierAdmin_i::obj_gc(RDI_TimeT curtime, CORBA::ULong deadAdmin, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { return 0; }
  if (_disposed) { return 0; }

  EventProxyPushConsumerPtrSeq                                    cpush;
  EventProxyPullConsumerPtrSeq                                    cpull;
  ProxyPushConsumerPtrSeq                                         apush;
  ProxyPullConsumerPtrSeq                                         apull;
  StructuredProxyPushConsumerPtrSeq                               spush;
  StructuredProxyPullConsumerPtrSeq                               spull;
  SequenceProxyPushConsumerPtrSeq                                 bpush;
  SequenceProxyPullConsumerPtrSeq                                 bpull;

  CosEvCA_EventProxyPushConsumerPtrSeq                            cpushrefs;
  CosEvCA_EventProxyPullConsumerPtrSeq                            cpullrefs;
  CosNA_ProxyPushConsumerPtrSeq                                   apushrefs;
  CosNA_ProxyPullConsumerPtrSeq                                   apullrefs;
  CosNA_StructuredProxyPushConsumerPtrSeq                         spushrefs;
  CosNA_StructuredProxyPullConsumerPtrSeq                         spullrefs;
  CosNA_SequenceProxyPushConsumerPtrSeq                           bpushrefs;
  CosNA_SequenceProxyPullConsumerPtrSeq                           bpullrefs;

  RDI_ListCursor<EventProxyPushConsumer_i *>                      cpushcur;
  RDI_ListCursor<EventProxyPullConsumer_i *>                      cpullcur;
  RDI_HashCursor<CosNA::ProxyID, ProxyPushConsumer_i *>           apushcur;
  RDI_HashCursor<CosNA::ProxyID, ProxyPullConsumer_i *>           apullcur;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushConsumer_i *> spushcur;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullConsumer_i *> spullcur;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushConsumer_i * >  bpushcur;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullConsumer_i * >  bpullcur;

  // CosEvent clients
  unsigned int num_cpush, num_cpull, num_apush, num_apull, num_spush, num_spull, num_bpush, num_bpull, i;

  num_cpush = _cosevent_push.length();
  cpush.length(num_cpush);
  cpushrefs.length(num_cpush);
  for (i = 0, cpushcur = _cosevent_push.cursor(); i < num_cpush; ++i, ++cpushcur) {
    EventProxyPushConsumer_i* prx = *cpushcur;
    cpush[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    cpushrefs[i] = WRAPPED_IMPL2OREF(CosEvCA::ProxyPushConsumer, prx);
  }

  num_cpull = _cosevent_pull.length();
  cpull.length(num_cpull);
  cpullrefs.length(num_cpull);
  for (i = 0, cpullcur = _cosevent_pull.cursor(); i < num_cpull; ++i, ++cpullcur) {
    EventProxyPullConsumer_i* prx = *cpullcur;
    cpull[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    cpullrefs[i] = WRAPPED_IMPL2OREF(CosEvCA::ProxyPullConsumer, prx);
  }

  num_apush = _prx_any_push.length();
  apush.length(num_apush);
  apushrefs.length(num_apush);
  for (i = 0, apushcur = _prx_any_push.cursor(); i < num_apush; ++i, ++apushcur) {
    ProxyPushConsumer_i* prx = apushcur.val();
    apush[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    apushrefs[i] = WRAPPED_IMPL2OREF(CosNA::ProxyPushConsumer, prx);
  }

  num_apull = _prx_any_pull.length();
  apull.length(num_apull);
  apullrefs.length(num_apull);
  for (i = 0, apullcur = _prx_any_pull.cursor(); i < num_apull; ++i, ++apullcur) {
    ProxyPullConsumer_i* prx = apullcur.val();
    apull[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    apullrefs[i] = WRAPPED_IMPL2OREF(CosNA::ProxyPullConsumer, prx);
  }

  num_spush = _prx_struc_push.length();
  spush.length(num_spush);
  spushrefs.length(num_spush);
  for (i = 0, spushcur = _prx_struc_push.cursor(); i < num_spush; ++i, ++spushcur) {
    StructuredProxyPushConsumer_i* prx = spushcur.val();
    spush[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    spushrefs[i] = WRAPPED_IMPL2OREF(CosNA::StructuredProxyPushConsumer, prx);
  }

  num_spull = _prx_struc_pull.length();
  spull.length(num_spull);
  spullrefs.length(num_spull);
  for (i = 0, spullcur = _prx_struc_pull.cursor(); i < num_spull; ++i, ++spullcur) {
    StructuredProxyPullConsumer_i* prx = spullcur.val();
    spull[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    spullrefs[i] = WRAPPED_IMPL2OREF(CosNA::StructuredProxyPullConsumer, prx);
  }

  num_bpush = _prx_batch_push.length();
  bpush.length(num_bpush);
  bpushrefs.length(num_bpush);
  for (i = 0, bpushcur = _prx_batch_push.cursor(); i < num_bpush; ++i, ++bpushcur) {
    SequenceProxyPushConsumer_i* prx = bpushcur.val();
    bpush[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    bpushrefs[i] = WRAPPED_IMPL2OREF(CosNA::SequenceProxyPushConsumer, prx);
  }

  num_bpull = _prx_batch_pull.length();
  bpull.length(num_bpull);
  bpullrefs.length(num_bpull);
  for (i = 0, bpullcur = _prx_batch_pull.cursor(); i < num_bpull; ++i, ++bpullcur) {
    SequenceProxyPullConsumer_i* prx = bpullcur.val();
    bpull[i] = prx;
    // bump refcount of prx so that its storage cannot be deallocated before we call obj_gc on it
    bpullrefs[i] = WRAPPED_IMPL2OREF(CosNA::SequenceProxyPullConsumer, prx);
  }

  { // introduce temporary lock release scope
    RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sadmin, WHATFN);

    for (i = 0; i < num_cpush; i++) {
      cpush[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(cpushrefs[i]); // undo the refcount bump performed above
    }
    for (i = 0; i < num_cpull; i++) {
      cpull[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(cpullrefs[i]); // undo the refcount bump performed above
    }
    for (i = 0; i < num_apush; i++) {
      apush[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(apushrefs[i]); // undo the refcount bump performed above
    }
    for (i = 0; i < num_apull; i++) {
      apull[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(apullrefs[i]); // undo the refcount bump performed above
    }
    for (i = 0; i < num_spush; i++) {
      spush[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(spushrefs[i]); // undo the refcount bump performed above
    }
    for (i = 0; i < num_spull; i++) {
      spull[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(spullrefs[i]); // undo the refcount bump performed above
    }
    for (i = 0; i < num_bpush; i++) {
      bpush[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(bpushrefs[i]); // undo the refcount bump performed above
    }
    for (i = 0; i < num_bpull; i++) {
      bpull[i]->obj_gc(curtime, deadConProxy, deadOtherProxy);
      CORBA::release(bpullrefs[i]); // undo the refcount bump performed above
    }

  } // end temporary lock release scope
  if (!held.sadmin) {
    RDI_Fatal("SupplierAdmin_i::obj_gc [**unexpected REACQUIRE failure**]\n");
  }
  if (_disposed) { return 0; }

  // now check whether we can be gc'd
  if (_serial != 0 && _num_proxies == 0 && deadAdmin && RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadAdmin)) {
    RDIDbgSAdmLog("GC destroys sadmin " << _serial
		  << " curtime = " << curtime.fmt_local()
		  << " last_use = " << _last_use.fmt_local()
		  << "(diff = " << RDI_TIMET_DIFF_IN_SECS(_last_use, curtime)
		  << ") DeadAdminInterval = " << deadAdmin << '\n');
    _disconnect_clients_and_dispose(held, 0, 1, sadmin_lock.dispose_info); // arg 1 means update channel
    return 1;
  }
  return 0;
}
#endif

// RDINotifSupplier methods:
void
SupplierAdmin_i::propagate_schange(RDI_LocksHeld&             held,
				   const CosN::EventTypeSeq&  added,
				   const CosN::EventTypeSeq&  deled,
				   Filter_i*                  filter)
{
  RDI_Fatal("SupplierAdmin_i::propagate_schange: did not register, should not get here");
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::filter_destroy_i"
void
SupplierAdmin_i::filter_destroy_i(Filter_i* filter)
{
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, return);
  _fa_helper.rem_filter_i(filter);
}

// ---------------------- ** Private routines ** ------------------------------

#undef WHATFN
#define WHATFN "SupplierAdmin_i::disconnect_clients_and_dispose"
void
SupplierAdmin_i::disconnect_clients_and_dispose(CORBA::Boolean fast_destroy)
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(sadmin_lock, held.sadmin, WHATFN);
  if (!held.sadmin) { return; }
  _disconnect_clients_and_dispose(held, fast_destroy, 0, sadmin_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::_disconnect_clients_and_dispose"
// *** requires caller has bumped scope lock, held.sadmin set
//  If update_channel is true, the admin unregisters itself and also
//  does a propagate_ochange on the channel if there are
//  event types registered with the admin.
void
SupplierAdmin_i::_disconnect_clients_and_dispose(RDI_LocksHeld&            held,
						 CORBA::Boolean            fast_destroy,
						 CORBA::Boolean            update_channel,
						 WRAPPED_DISPOSEINFO_PTR&  dispose_info)
  // Always:
  // If update_channel is set:
{
  if (_disposed) {
    RDIDbgSAdmLog("SupplierAdmin_i::_disconnect_clients_and_dispose() called more than once\n");
    return;
  }
  _disposed = 1; // acts as guard: the following is executed by only one thread 

  if (!fast_destroy) {

    // Wait for inuse to drop to one.  This ensures that any call that
    // bumps inuse and releases the lock has a chance to complete cleanly.
    while (_oplockptr->inuse() > 1) {
      RDI_OPLOCK_BROADCAST;
      RDI_OPLOCK_INUSEONE_WAIT(WHATFN);
    }
    if (update_channel) {

      // never hold oplock when passing 'this' to a parent object
      {
	RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sadmin, WHATFN);
	// this is unlocked for rest of scope
	_channel->unregister(held, this);
      }
      if (!held.sadmin) {
	RDI_Fatal("SupplierAdmin_i::_disconnect_clients_and_dispose [**unexpected REACQUIRE failure**]\n");
      }

      if ( _evtypes.length() ) {
	// There are event types registered with this administrative
	// object; update the channel's aggregate information about them.
	// NOTE: we release the lock before we invoke channel operations so
	//      that we avoid potential deadlocks when the channel is being
	//      destroyed at the same time
	CosN::EventTypeSeq added, deled;
	added.length(0);
	deled.length( _evtypes.length() );
	RDI_HashCursor<CosN::EventType, CORBA::ULong> curs;
	CORBA::ULong ix=0;
	for ( ix=0, curs = _evtypes.cursor(); curs.is_valid(); ++ix, ++curs ) {
	  deled[ix] = curs.key();
	}
	EventChannel_i* chan = _channel;
	{
	  RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sadmin, WHATFN);
	  // this is unlocked for rest of scope
	  chan->propagate_ochange(held, added, deled);
	}
	if (!held.sadmin) {
	  RDI_Fatal("SupplierAdmin_i::_disconnect_clients_and_dispose [**unexpected REACQUIRE failure**]\n");
	}
      }
    }
    RDIDbgSAdmLog("Begin Destruction of SupplierAdmin @ " << (void*)this << " [" << _serial << "]\n");
    unsigned int i;
    RDI_ListCursor<EventProxyPushConsumer_i *>                      cpushcur;
    RDI_ListCursor<EventProxyPullConsumer_i *>                      cpullcur;
    RDI_HashCursor<CosNA::ProxyID, ProxyPushConsumer_i *>           apushcur;
    RDI_HashCursor<CosNA::ProxyID, ProxyPullConsumer_i *>           apullcur;
    RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushConsumer_i *> spushcur;
    RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullConsumer_i *> spullcur;
    RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushConsumer_i *>   bpushcur;
    RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullConsumer_i *>   bpullcur;

    cpushcur = _cosevent_push.cursor();
    for (i=0; i < _cosevent_push.length(); ++i, ++cpushcur) {
      _removed_push_proxy();
      (*cpushcur)->disconnect_client_and_dispose(held, 0);
    }
    cpullcur = _cosevent_pull.cursor();
    for (i=0; i < _cosevent_pull.length(); ++i, ++cpullcur) {
      EventProxyPullConsumer_i* prx = *cpullcur;
      _removed_pull_proxy(prx);
      prx->disconnect_client_and_dispose(held, 0);
    }
    for ( apushcur = _prx_any_push.cursor(); apushcur.is_valid(); ++apushcur ) {
      _removed_push_proxy();
      apushcur.val()->disconnect_client_and_dispose(held, 0);
    }
    for ( apullcur = _prx_any_pull.cursor(); apullcur.is_valid(); ++apullcur ) {
      ProxyPullConsumer_i* prx = apullcur.val();
      _removed_pull_proxy(prx);
      prx->disconnect_client_and_dispose(held, 0);
    }
    for ( spushcur = _prx_struc_push.cursor(); spushcur.is_valid(); ++spushcur ) {
      _removed_push_proxy();
      spushcur.val()->disconnect_client_and_dispose(held, 0);
    }
    for ( spullcur = _prx_struc_pull.cursor(); spullcur.is_valid(); ++spullcur ) {
      StructuredProxyPullConsumer_i* prx = spullcur.val();
      _removed_pull_proxy(prx);
      prx->disconnect_client_and_dispose(held, 0);
    }
    for ( bpushcur = _prx_batch_push.cursor(); bpushcur.is_valid(); ++bpushcur ) {
      _removed_push_proxy();
      bpushcur.val()->disconnect_client_and_dispose(held, 0);
    }
    for ( bpullcur = _prx_batch_pull.cursor(); bpullcur.is_valid(); ++bpullcur ) {
      SequenceProxyPullConsumer_i* prx = bpullcur.val();
      _removed_pull_proxy(prx);
      prx->disconnect_client_and_dispose(held, 0);
    }
    _cosevent_push.drain();  _cosevent_pull.drain();
    _prx_any_push.clear();   _prx_any_pull.clear();
    _prx_struc_push.clear(); _prx_struc_pull.clear();
    _prx_batch_push.clear(); _prx_batch_pull.clear();

    if ( _fa_helper.has_filters() ) {
      _fa_helper.remove_all_filters(held, 0); // 0 => do NOT want propagate_schange callback
    }
  } // end of !fast_destroy

  if ( _qosprop ) {
    delete _qosprop; _qosprop = 0;
  }
  RDIDbgSAdmLog("SupplierAdmin @ " << (void*)this << " [" << _serial << "] destroyed\n");
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::_propagate_ochange"
// *** requires caller has bumped scope lock, held.sadmin set
// ** Effect: Update the local hash table and if new entries are created or
//      deleted, update the channel as well
//      (releases oplock across call to channel to
//       avoid deadlocks due to reverse order of lock acquisitions).
void
SupplierAdmin_i::_propagate_ochange(RDI_LocksHeld&             held,
				    const CosN::EventTypeSeq&  added,
				    const CosN::EventTypeSeq&  deled)
{
  CORBA::ULong ix=0, sz=0, vl=0;
  CosN::EventTypeSeq new_added;
  CosN::EventTypeSeq old_deled;
  new_added.length(0);
  old_deled.length(0);

  for (sz=0, ix = 0; ix < added.length(); ix++) {
    if ( _evtypes.lookup(added[ix], vl) ) {
      vl += 1; _evtypes.replace(added[ix], vl);
    } else {
      vl  = 1; _evtypes.insert(added[ix], vl);
      new_added.length(sz + 1);
      new_added[sz++] = added[ix];
    }
  }
  for (sz=0, ix = 0; ix < deled.length(); ix++) {
    if ( _evtypes.lookup(deled[ix], vl) ) {
      if ( vl == 1 ) {
	_evtypes.remove(deled[ix]);
	old_deled.length(sz + 1);
	old_deled[sz++] = deled[ix];
      } else {
	vl -= 1; _evtypes.replace(deled[ix], vl);
      }
    } else {
      RDIDbgSAdmLog("Invalid "<<deled[ix].domain_name<<"::"<<deled[ix].type_name << '\n');
    }
  }
  EventChannel_i* chan = _channel;
  if ( new_added.length() || old_deled.length() ) {
    {
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.sadmin, WHATFN);
      // this is unlocked for rest of scope
      chan->propagate_ochange(held, new_added, old_deled);
    }
    if (!held.sadmin) {
      RDI_Fatal("SupplierAdmin_i::_propagate_ochange [**unexpected REACQUIRE failure**]\n");
    }
  }
}

// NB ** Assumes oplock is held **
void
SupplierAdmin_i::_removed_push_proxy()
{
  _num_proxies -= 1;
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _channel->decr_suppliers();
}

// NB ** Assumes oplock is held **
void
SupplierAdmin_i::_removed_pull_proxy(RDIProxyPullConsumer* prx)
{
  _num_proxies -= 1;
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _channel->decr_suppliers();
  RDI_PullSupplier* cps = _channel->pull_supplier();
  if ( cps ) {    // Deregister Pull Proxy
    cps->remove_proxy(prx);
  }
}

////////////////////////////////////////
// Logging

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::log_output"
RDIstrstream& ConsumerAdmin_i::log_output(RDIstrstream& str)
{
  RDI_ListCursor<EventProxyPushSupplier_i *>                      cpushcur;
  RDI_ListCursor<EventProxyPullSupplier_i *>                      cpullcur;
  RDI_HashCursor<CosNA::ProxyID, ProxyPushSupplier_i *>           apushcur;
  RDI_HashCursor<CosNA::ProxyID, ProxyPullSupplier_i *>           apullcur;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushSupplier_i *> spushcur;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullSupplier_i *> spullcur;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushSupplier_i *>   bpushcur;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullSupplier_i *>   bpullcur;

  unsigned int cnt = 0;
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, return str);

  str << "----------------------------------------------------------------------\n";
  str << "Consumer Admin " << _my_name;
  str << "\n----------------------------------------------------------------------\n";
  str << (void*)this <<" ID "; 
  str.setw(3); str << _serial;
  str.setw(15); str << "\n  Last Use ";
#ifndef NO_OBJ_GC
  _last_use.out_local(str);
#endif
  str << '\n';
  if ( _rqstypes.length() != 0 ) {
    for (CORBA::ULong ix = 0; ix < _rqstypes.length(); ix++) {
      str << "\n\t" << (const char*)_rqstypes[ix].domain_name; 
      str << "::" << (const char*)_rqstypes[ix].type_name;
    }
  }
  out_config(str);
  cpushcur = _cosevent_push.cursor();
  for (cnt=0; cnt < _cosevent_push.length(); ++cnt, ++cpushcur) {
    str.setw(4); str << cnt << ": " << *(*cpushcur) << '\n';
  }
  cpullcur = _cosevent_pull.cursor();
  for (cnt=0; cnt < _cosevent_pull.length(); ++cnt, ++cpullcur) {
    str.setw(4); str << cnt << ": " << *(*cpullcur) << '\n';
  }
  cnt = 0;
  for (apushcur = _prx_any_push.cursor(); apushcur.is_valid(); ++apushcur) {
    str.setw(4); str << ++cnt << ": " << *(apushcur.val()) << '\n';
  }
  cnt = 0;
  for (apullcur = _prx_any_pull.cursor(); apullcur.is_valid(); ++apullcur) {
    str.setw(4); str << ++cnt << ": " << *(apullcur.val()) << '\n';
  }
  cnt = 0;
  for (spushcur = _prx_struc_push.cursor(); spushcur.is_valid(); ++spushcur) {
    str.setw(4); str << ++cnt << ": " << *(spushcur.val()) << '\n';
  }
  cnt = 0;
  for (spullcur = _prx_struc_pull.cursor(); spullcur.is_valid(); ++spullcur) {
    str.setw(4); str << ++cnt << ": " << *(spullcur.val()) << '\n';
  }
  cnt = 0;
  for (bpushcur = _prx_batch_push.cursor(); bpushcur.is_valid(); ++bpushcur) {
    str.setw(4); str << ++cnt << ": " << *(bpushcur.val()) << '\n';
  }
  cnt = 0;
  for (bpullcur = _prx_batch_pull.cursor(); bpullcur.is_valid(); ++bpullcur) {
    str.setw(4); str << ++cnt << ": " << *(bpullcur.val()) << '\n';
  }
  return str;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::log_output"
RDIstrstream& SupplierAdmin_i::log_output(RDIstrstream& str)
{
  RDI_ListCursor<EventProxyPushConsumer_i *>                      cpushcur;
  RDI_ListCursor<EventProxyPullConsumer_i *>                      cpullcur;
  RDI_HashCursor<CosNA::ProxyID, ProxyPushConsumer_i *>           apushcur;
  RDI_HashCursor<CosNA::ProxyID, ProxyPullConsumer_i *>           apullcur;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushConsumer_i *> spushcur;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullConsumer_i *> spullcur;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushConsumer_i *>   bpushcur;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullConsumer_i *>   bpullcur;

  unsigned int cnt = 0;
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, return str);

  str << "----------------------------------------------------------------------\n";
  str << "Supplier Admin " << _my_name;
  str << "\n----------------------------------------------------------------------\n";
  str << (void*)this <<" ID "; 
  str.setw(3); str << _serial;
  str.setw(15); str << "\n  Last Use ";
#ifndef NO_OBJ_GC
  _last_use.out_local(str);
#endif
  str << '\n';
  out_config(str);
  cpushcur = _cosevent_push.cursor();
  for (cnt=0; cnt < _cosevent_push.length(); ++cnt, ++cpushcur) {
    str.setw(4); str << cnt << ": " << *(*cpushcur) << '\n';
  }
  cpullcur = _cosevent_pull.cursor();
  for (cnt=0; cnt < _cosevent_pull.length(); ++cnt, ++cpullcur) {
    str.setw(4); str << cnt << ": " << *(*cpullcur) << '\n';
  }
  cnt = 0;
  for (apushcur = _prx_any_push.cursor(); apushcur.is_valid(); ++apushcur) {
    str.setw(4); str << ++cnt << ": " << *(apushcur.val()) << '\n';
  }
  cnt = 0;
  for (apullcur = _prx_any_pull.cursor(); apullcur.is_valid(); ++apullcur) {
    str.setw(4); str << ++cnt << ": " << *(apullcur.val()) << '\n';
  }
  cnt = 0;
  for (spushcur = _prx_struc_push.cursor(); spushcur.is_valid(); ++spushcur) {
    str.setw(4); str << ++cnt << ": " << *(spushcur.val()) << '\n';
  }
  cnt = 0;
  for (spullcur = _prx_struc_pull.cursor(); spullcur.is_valid(); ++spullcur) {
    str.setw(4); str << ++cnt << ": " << *(spullcur.val()) << '\n';
  }
  cnt = 0;
  for (bpushcur = _prx_batch_push.cursor(); bpushcur.is_valid(); ++bpushcur) {
    str.setw(4); str << ++cnt << ": " << *(bpushcur.val()) << '\n';
  }
  cnt = 0;
  for (bpullcur = _prx_batch_pull.cursor(); bpullcur.is_valid(); ++bpullcur) {
    str.setw(4); str << ++cnt << ": " << *(bpullcur.val()) << '\n';
  }
  return str;
}

////////////////////////////////////////
// Interactive

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::my_name"
AttN::NameSeq*
ConsumerAdmin_i::my_name( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::NameSeq* res = new AttN::NameSeq(_my_name);
  if ( res == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  return res;
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::child_names"
AttN::NameSeq*
ConsumerAdmin_i::child_names( WRAPPED_IMPLARG_VOID )
{
  char buf[20];
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  AttN::NameSeq* names = new AttN::NameSeq;
  if ( names == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  unsigned long num = 
    _cosevent_push.length()  +
    _cosevent_pull.length()  +
    _prx_any_push.length()   +
    _prx_struc_push.length() +
    _prx_batch_push.length() +
    _prx_any_pull.length()   +
    _prx_struc_pull.length() +
    _prx_batch_pull.length();
  names->length(num);
  num = 0;
  // CosEvent clients
  unsigned int i;
  RDI_ListCursor<EventProxyPushSupplier_i *>                cpushcur;
  RDI_ListCursor<EventProxyPullSupplier_i *>                cpullcur;
  cpushcur = _cosevent_push.cursor();
  for (i=0; i < _cosevent_push.length(); ++i, ++cpushcur) {
    EventProxyPushSupplier_i* prx = *cpushcur;
    (*names)[num++] = (const char*)(prx->_my_name[prx->_my_name.length()-1]);
  }
  cpullcur = _cosevent_pull.cursor();
  for (i=0; i < _cosevent_pull.length(); ++i, ++cpullcur) {
    EventProxyPullSupplier_i* prx = *cpullcur;
    (*names)[num++] = (const char*)(prx->_my_name[prx->_my_name.length()-1]);
  }
  // CosNotify clients
  RDI_HashCursor<CosNA::ProxyID, ProxyPushSupplier_i *>           ac1;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushSupplier_i *> sc1;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushSupplier_i * >  bc1;
  RDI_HashCursor<CosNA::ProxyID, ProxyPullSupplier_i *>           ac2;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullSupplier_i *> sc2;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullSupplier_i * >  bc2;
  for ( ac1 = _prx_any_push.cursor(); ac1.is_valid(); ++ac1, ++num ) {
    sprintf(buf, "proxy%ld", (long)ac1.key());
    (*names)[num] = (const char*)buf;
  }
  for ( sc1 = _prx_struc_push.cursor(); sc1.is_valid(); ++sc1, ++num ) {
    sprintf(buf, "proxy%ld", (long)sc1.key());
    (*names)[num] = (const char*)buf;
  }
  for ( bc1 = _prx_batch_push.cursor(); bc1.is_valid(); ++bc1, ++num ) {
    sprintf(buf, "proxy%ld", (long)bc1.key());
    (*names)[num] = (const char*)buf;
  }
  for ( ac2 = _prx_any_pull.cursor(); ac2.is_valid(); ++ac2, ++num ) {
    sprintf(buf, "proxy%ld", (long)ac2.key());
    (*names)[num] = (const char*)buf;
  }
  for ( sc2 = _prx_struc_pull.cursor(); sc2.is_valid(); ++sc2, ++num ) {
    sprintf(buf, "proxy%ld", (long)sc2.key());
    (*names)[num] = (const char*)buf;
  }
  for ( bc2 = _prx_batch_pull.cursor(); bc2.is_valid(); ++bc2, ++num ) {
    sprintf(buf, "proxy%ld", (long)bc2.key());
    (*names)[num] = (const char*)buf;
  }
  return names;
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::out_info_filters"
void ConsumerAdmin_i::out_info_filters(RDIstrstream& str, CORBA::Boolean admin, CORBA::Boolean proxies) {
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, return);
  if (admin) {
    str << "----------------------------------------------------------------------\n";
    str << "Admin Filters attached to " << _my_name << '\n';
    str << "----------------------------------------------------------------------\n";
    _fa_helper.out_info_filters(str);
  }
  if (proxies) {
    RDI_HashCursor<CosNA::ProxyID, ProxyPushSupplier_i *>           ac1;
    RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushSupplier_i *> sc1;
    RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushSupplier_i * >  bc1;
    RDI_HashCursor<CosNA::ProxyID, ProxyPullSupplier_i *>           ac2;
    RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullSupplier_i *> sc2;
    RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullSupplier_i * >  bc2;
    for ( ac1 = _prx_any_push.cursor(); ac1.is_valid(); ++ac1 ) {
      ac1.val()->out_info_filters(str);
    }
    for ( sc1 = _prx_struc_push.cursor(); sc1.is_valid(); ++sc1 ) {
      sc1.val()->out_info_filters(str);
    }
    for ( bc1 = _prx_batch_push.cursor(); bc1.is_valid(); ++bc1 ) {
      bc1.val()->out_info_filters(str);
    }
    for ( ac2 = _prx_any_pull.cursor(); ac2.is_valid(); ++ac2 ) {
      ac2.val()->out_info_filters(str);
    }
    for ( sc2 = _prx_struc_pull.cursor(); sc2.is_valid(); ++sc2 ) {
      sc2.val()->out_info_filters(str);
    }
    for ( bc2 = _prx_batch_pull.cursor(); bc2.is_valid(); ++bc2 ) {
      bc2.val()->out_info_filters(str);
    }
  }
}

// _children: fills in 'ren' sequence
// NB: REQUIRES oplock is held
void
ConsumerAdmin_i::_children(AttN::IactSeq& ren,
			   CORBA::Boolean only_cleanup_candidates) {
  // Unconnected proxies are cleanup candidates
  unsigned int idx = 0;
  // start with max length
  ren.length(_cosevent_push.length()  +
	     _cosevent_pull.length()  +
	     _prx_any_push.length()   +
	     _prx_struc_push.length() +
	     _prx_batch_push.length() +
	     _prx_any_pull.length()   +
	     _prx_struc_pull.length() +
	     _prx_batch_pull.length());
  // CosEvent clients
  unsigned int i;
  RDI_ListCursor<EventProxyPushSupplier_i *>                cpushcur;
  RDI_ListCursor<EventProxyPullSupplier_i *>                cpullcur;
  cpushcur = _cosevent_push.cursor();
  for (i=0; i < _cosevent_push.length(); ++i, ++cpushcur) {
    EventProxyPushSupplier_i* prx = *cpushcur;
    if ((!only_cleanup_candidates) || (!prx->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, prx);
    }
  }
  cpullcur = _cosevent_pull.cursor();
  for (i=0; i < _cosevent_pull.length(); ++i, ++cpullcur) {
    EventProxyPullSupplier_i* prx = *cpullcur;
    if ((!only_cleanup_candidates) || (!prx->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, prx);
    }
  }
  // CosNotify clients
  RDI_HashCursor<CosNA::ProxyID, ProxyPushSupplier_i *>           ac1;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushSupplier_i *> sc1;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushSupplier_i * >  bc1;
  RDI_HashCursor<CosNA::ProxyID, ProxyPullSupplier_i *>           ac2;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullSupplier_i *> sc2;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullSupplier_i * >  bc2;
  for ( ac1 = _prx_any_push.cursor(); ac1.is_valid(); ++ac1 ) {
    if ((!only_cleanup_candidates) || (!ac1.val()->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, ac1.val());
    }
  }
  for ( sc1 = _prx_struc_push.cursor(); sc1.is_valid(); ++sc1 ) {
    if ((!only_cleanup_candidates) || (!sc1.val()->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, sc1.val());
    }
  }
  for ( bc1 = _prx_batch_push.cursor(); bc1.is_valid(); ++bc1 ) {
    if ((!only_cleanup_candidates) || (!bc1.val()->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, bc1.val());
    }
  }
  for ( ac2 = _prx_any_pull.cursor(); ac2.is_valid(); ++ac2 ) {
    if ((!only_cleanup_candidates) || (!ac2.val()->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, ac2.val());
    }
  }
  for ( sc2 = _prx_struc_pull.cursor(); sc2.is_valid(); ++sc2 ) {
    if ((!only_cleanup_candidates) || (!sc2.val()->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, sc2.val());
    }
  }
  for ( bc2 = _prx_batch_pull.cursor(); bc2.is_valid(); ++bc2 ) {
    if ((!only_cleanup_candidates) || (!bc2.val()->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, bc2.val());
    }
  }
  // trim to actual length
  ren.length(idx);
}

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::children"
AttN::IactSeq*
ConsumerAdmin_i::children(CORBA::Boolean only_cleanup_candidates  WRAPPED_IMPLARG )
{
  // Unconnected proxies are cleanup candidates
  RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::IactSeq* ren = new AttN::IactSeq;
  if ( ren == (AttN::IactSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  _children(*ren, only_cleanup_candidates);
  return ren;
}

void
ConsumerAdmin_i::cleanup(RDIstrstream& str, CORBA::Boolean admin, CORBA::Boolean proxies) {
  RDIInteractive::cleanup_admin(str, _my_oref, admin, proxies);
}

CORBA::Boolean
ConsumerAdmin_i::do_set_command(RDIstrstream& str, RDIParseCmd& p) {
  CosN::QoSProperties    n_qos;
  CosN::AdminProperties  a_qos;
  AttN::ServerProperties s_qos;
  CORBA::Boolean success = 1;
  success = RDI_AllQoS::parse_set_command(str, p, RDI_C_ADMIN, n_qos, a_qos, s_qos);
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

#undef WHATFN
#define WHATFN "ConsumerAdmin_i::do_go_command"
CORBA::Boolean
ConsumerAdmin_i::do_go_command(RDIstrstream& str, RDIParseCmd& p,
			       CORBA::Boolean& target_changed,
			       AttN_Interactive_outarg next_target) {
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
  unsigned int i;

  {
    // introduce scope where this is locked
    RDI_OPLOCK_SCOPE_LOCK(cadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
    // CosEvent clients
    RDI_ListCursor<EventProxyPushSupplier_i *>                cpushcur;
    RDI_ListCursor<EventProxyPullSupplier_i *>                cpullcur;
    cpushcur = _cosevent_push.cursor();
    for (i=0; i < _cosevent_push.length(); ++i, ++cpushcur) {
      EventProxyPushSupplier_i* prx = *cpushcur;
      if (RDI_STR_EQ_I(go_targ, prx->_my_name[prx->_my_name.length()-1])) {
	targ1 = WRAPPED_IMPL2OREF(AttN::CosEvProxyPushSupplier, prx);
	targ1_set = 1;
	break;
      }
    }
    if (targ1_set == 0) {
      cpullcur = _cosevent_pull.cursor();
      for (i=0; i < _cosevent_pull.length(); ++i, ++cpullcur) {
	EventProxyPullSupplier_i* prx = *cpullcur;
	if (RDI_STR_EQ_I(go_targ, prx->_my_name[prx->_my_name.length()-1])) {
	  targ1 = WRAPPED_IMPL2OREF(AttN::CosEvProxyPullSupplier, prx);
	  targ1_set = 1;
	  break;
	}
      }
    }
    // CosNotify  clients
    char buf[20];
    RDI_HashCursor<CosNA::ProxyID, ProxyPushSupplier_i *>           ac1;
    RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushSupplier_i *> sc1;
    RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushSupplier_i * >  bc1;
    RDI_HashCursor<CosNA::ProxyID, ProxyPullSupplier_i *>           ac2;
    RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullSupplier_i *> sc2;
    RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullSupplier_i * >  bc2;
    if (targ1_set == 0) {
      for ( ac1 = _prx_any_push.cursor(); ac1.is_valid(); ++ac1 ) {
	sprintf(buf, "proxy%ld", (long)ac1.key());
	if (RDI_STR_EQ_I(go_targ, buf)) {
	  targ1 = WRAPPED_IMPL2OREF(AttN::ProxyPushSupplier, ac1.val());
	  targ1_set = 1;
	  break;
	}
      }
    }
    if (targ1_set == 0) {
      for ( sc1 = _prx_struc_push.cursor(); sc1.is_valid(); ++sc1 ) {
	sprintf(buf, "proxy%ld", (long)sc1.key());
	if (RDI_STR_EQ_I(go_targ, buf)) {
	  targ1 = WRAPPED_IMPL2OREF(AttN::StructuredProxyPushSupplier, sc1.val());
	  targ1_set = 1;
	  break;
	}
      }
    }
    if (targ1_set == 0) {
      for ( bc1 = _prx_batch_push.cursor(); bc1.is_valid(); ++bc1 ) {
	sprintf(buf, "proxy%ld", (long)bc1.key());
	if (RDI_STR_EQ_I(go_targ, buf)) {
	  targ1 = WRAPPED_IMPL2OREF(AttN::SequenceProxyPushSupplier, bc1.val());
	  targ1_set = 1;
	  break;
	}
      }
    }
    if (targ1_set == 0) {
      for ( ac2 = _prx_any_pull.cursor(); ac2.is_valid(); ++ac2 ) {
	sprintf(buf, "proxy%ld", (long)ac2.key());
	if (RDI_STR_EQ_I(go_targ, buf)) {
	  targ1 = WRAPPED_IMPL2OREF(AttN::ProxyPullSupplier, ac2.val());
	  targ1_set = 1;
	  break;
	}
      }
    }
    if (targ1_set == 0) {
      for ( sc2 = _prx_struc_pull.cursor(); sc2.is_valid(); ++sc2 ) {
	sprintf(buf, "proxy%ld", (long)sc2.key());
	if (RDI_STR_EQ_I(go_targ, buf)) {
	  targ1 = WRAPPED_IMPL2OREF(AttN::StructuredProxyPullSupplier, sc2.val());
	  targ1_set = 1;
	  break;
	}
      }
    }
    if (targ1_set == 0) {
      for ( bc2 = _prx_batch_pull.cursor(); bc2.is_valid(); ++bc2 ) {
	sprintf(buf, "proxy%ld", (long)bc2.key());
	if (RDI_STR_EQ_I(go_targ, buf)) {
	  targ1 = WRAPPED_IMPL2OREF(AttN::SequenceProxyPullSupplier, bc2.val());
	  targ1_set = 1;
	  break;
	}
      }
    }
  }
  // end of scope, this is no longer locked
  if (targ1_set == 0) {
    str << "Invalid target " << p.argv[1] << " : " << go_targ << " is not a proxy name\n";
    str << "  (Use 'children' for list of valid proxy names)\n";
    success = 0;
  } else {
    str << "\nomniNotify: new target ==> " << go_targ << '\n';
    if (rest_go_targ && (RDI_STRLEN(rest_go_targ) > 0)) {
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

void ConsumerAdmin_i::out_config(RDIstrstream& str) {
  str << "----------------------------------------------------------------------\n";
  str << "NotifQos settings for " << _my_name << '\n';
  str << "----------------------------------------------------------------------\n";
  _qosprop->log_output(str);
  str << '\n';
}

void ConsumerAdmin_i::out_commands(RDIstrstream& str) {
  str << "omniNotify ConsumerAdmin commands:\n"
      << "  up                : change target to admin's channel\n"
      << "  go <proxy_name>   : change target to a specific proxy\n"
      << "                        ('children' lists proxy names)\n"
      << "  debug             : show debugging information for this admin + its proxies\n"
      << "  config            : show config information for this admin + its proxies\n"
      << "  info filters      : show filters attached to this admin and its proxies\n"
      << "  info afilters     : show filters attached to this admin\n"
      << "  info pfilters     : show filters attached to this admin's proxies\n"
      << "  cleanup proxies   : destroy all proxies of this admin that are not connected\n"
      << "  set <name1> <value1> [ <name2> <value2> ... ]\n"
      << "                    : set NotifQoS property name1 to value1, name2 to value2, etc.\n";
}

char*
ConsumerAdmin_i::do_command(const char* cmnd, CORBA::Boolean& success,
			    CORBA::Boolean& target_changed,
			    AttN_Interactive_outarg next_target  WRAPPED_IMPLARG )
{
  RDIParseCmd p(cmnd);
  success = 1;
  target_changed = 0;
  if (p.argc == 0) { return CORBA_STRING_DUP(""); }

  RDIstrstream str;
  if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "help")) {
    out_commands(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "debug")) {
    log_output(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "config")) {
    out_config(str);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "info") && RDI_STR_EQ_I(p.argv[1], "filters")) {
    out_info_filters(str, 1, 1);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "info") && RDI_STR_EQ_I(p.argv[1], "afilters")) {
    out_info_filters(str, 1, 0);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "info") && RDI_STR_EQ_I(p.argv[1], "pfilters")) {
    out_info_filters(str, 0, 1);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "cleanup") && RDI_STR_EQ_I(p.argv[1], "proxies")) {
    cleanup(str, 0, 1);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "up")) {
    target_changed = 1;
    next_target = WRAPPED_IMPL2OREF(AttN::EventChannel, _channel);
    str << "\nomniNotify: new target ==> " << _my_name[_my_name.length()-2] << '\n';
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
#define WHATFN "SupplierAdmin_i::my_name"
AttN::NameSeq*
SupplierAdmin_i::my_name( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::NameSeq* res = new AttN::NameSeq(_my_name);
  if ( res == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  return res;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::child_names"
AttN::NameSeq*
SupplierAdmin_i::child_names( WRAPPED_IMPLARG_VOID )
{
  char buf[20];
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_disposed) { RDI_THROW_INV_OBJREF; } // in process of being disposed
  AttN::NameSeq* names = new AttN::NameSeq;
  if ( names == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  unsigned long num = 
    _cosevent_push.length()  +
    _cosevent_pull.length()  +
    _prx_any_push.length()   +
    _prx_struc_push.length() +
    _prx_batch_push.length() +
    _prx_any_pull.length()   +
    _prx_struc_pull.length() +
    _prx_batch_pull.length();
  names->length(num);
  num = 0;
  // CosEvent clients
  unsigned int i;
  RDI_ListCursor<EventProxyPushConsumer_i *>                cpushcur;
  RDI_ListCursor<EventProxyPullConsumer_i *>                cpullcur;
  cpushcur = _cosevent_push.cursor();
  for (i=0; i < _cosevent_push.length(); ++i, ++cpushcur) {
    EventProxyPushConsumer_i* prx = *cpushcur;
    (*names)[num++] = (const char*)(prx->_my_name[prx->_my_name.length()-1]);
  }
  cpullcur = _cosevent_pull.cursor();
  for (i=0; i < _cosevent_pull.length(); ++i, ++cpullcur) {
    EventProxyPullConsumer_i* prx = *cpullcur;
    (*names)[num++] = (const char*)(prx->_my_name[prx->_my_name.length()-1]);
  }
  // CosNotify Clients
  RDI_HashCursor<CosNA::ProxyID, ProxyPushConsumer_i *>           ac1;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushConsumer_i *> sc1;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushConsumer_i *>   bc1;
  RDI_HashCursor<CosNA::ProxyID, ProxyPullConsumer_i *>           ac2;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullConsumer_i *> sc2;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullConsumer_i *>   bc2;
  for ( ac1 = _prx_any_push.cursor(); ac1.is_valid(); ++ac1, ++num ) {
    sprintf(buf, "proxy%ld", (long)ac1.key());
    (*names)[num] = (const char*)buf;
  }
  for ( sc1 = _prx_struc_push.cursor(); sc1.is_valid(); ++sc1, ++num ) {
    sprintf(buf, "proxy%ld", (long)sc1.key());
    (*names)[num] = (const char*)buf;
  }
  for ( bc1 = _prx_batch_push.cursor(); bc1.is_valid(); ++bc1, ++num ) {
    sprintf(buf, "proxy%ld", (long)bc1.key());
    (*names)[num] = (const char*)buf;
  }
  for ( ac2 = _prx_any_pull.cursor(); ac2.is_valid(); ++ac2, ++num ) {
    sprintf(buf, "proxy%ld", (long)ac2.key());
    (*names)[num] = (const char*)buf;
  }
  for ( sc2 = _prx_struc_pull.cursor(); sc2.is_valid(); ++sc2, ++num ) {
    sprintf(buf, "proxy%ld", (long)sc2.key());
    (*names)[num] = (const char*)buf;
  }
  for ( bc2 = _prx_batch_pull.cursor(); bc2.is_valid(); ++bc2, ++num ) {
    sprintf(buf, "proxy%ld", (long)bc2.key());
    (*names)[num] = (const char*)buf;
  }
  return names;
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::out_info_filters"
void SupplierAdmin_i::out_info_filters(RDIstrstream& str, CORBA::Boolean admin, CORBA::Boolean proxies) {
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, return);
  if (admin) {
    str << "----------------------------------------------------------------------\n";
    str << "Admin Filters attached to " << _my_name << '\n';
    str << "----------------------------------------------------------------------\n";
    _fa_helper.out_info_filters(str);
  }
  if (proxies) {
    RDI_HashCursor<CosNA::ProxyID, ProxyPushConsumer_i *>           ac1;
    RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushConsumer_i *> sc1;
    RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushConsumer_i *>   bc1;
    RDI_HashCursor<CosNA::ProxyID, ProxyPullConsumer_i *>           ac2;
    RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullConsumer_i *> sc2;
    RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullConsumer_i *>   bc2;
    for ( ac1 = _prx_any_push.cursor(); ac1.is_valid(); ++ac1 ) {
      ac1.val()->out_info_filters(str);
    }
    for ( sc1 = _prx_struc_push.cursor(); sc1.is_valid(); ++sc1 ) {
      sc1.val()->out_info_filters(str);
    }
    for ( bc1 = _prx_batch_push.cursor(); bc1.is_valid(); ++bc1 ) {
      bc1.val()->out_info_filters(str);
    }
    for ( ac2 = _prx_any_pull.cursor(); ac2.is_valid(); ++ac2 ) {
      ac2.val()->out_info_filters(str);
    }
    for ( sc2 = _prx_struc_pull.cursor(); sc2.is_valid(); ++sc2 ) {
      sc2.val()->out_info_filters(str);
    }
    for ( bc2 = _prx_batch_pull.cursor(); bc2.is_valid(); ++bc2 ) {
      bc2.val()->out_info_filters(str);
    }
  }
}

// _children: fills in 'ren'
// NB: REQUIRES oplock is held
void
SupplierAdmin_i::_children(AttN::IactSeq& ren, CORBA::Boolean only_cleanup_candidates) {
  // Unconnected proxies are cleanup candidates
  unsigned int idx = 0;
  // start with max length
  ren.length(_cosevent_push.length()  +
	     _cosevent_pull.length()  +
	     _prx_any_push.length()   +
	     _prx_struc_push.length() +
	     _prx_batch_push.length() +
	     _prx_any_pull.length()   +
	     _prx_struc_pull.length() +
	     _prx_batch_pull.length());
  // CosEvent clients
  unsigned int i;
  RDI_ListCursor<EventProxyPushConsumer_i *>                cpushcur;
  RDI_ListCursor<EventProxyPullConsumer_i *>                cpullcur;
  cpushcur = _cosevent_push.cursor();
  for (i=0; i < _cosevent_push.length(); ++i, ++cpushcur) {
    EventProxyPushConsumer_i* prx = *cpushcur;
    if ((!only_cleanup_candidates) || (!prx->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, prx);
    }
  }
  cpullcur = _cosevent_pull.cursor();
  for (i=0; i < _cosevent_pull.length(); ++i, ++cpullcur) {
    EventProxyPullConsumer_i* prx = *cpullcur;
    if ((!only_cleanup_candidates) || (!prx->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, prx);
    }
  }
  // CosNotify clients
  RDI_HashCursor<CosNA::ProxyID, ProxyPushConsumer_i *>           ac1;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushConsumer_i *> sc1;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushConsumer_i * >  bc1;
  RDI_HashCursor<CosNA::ProxyID, ProxyPullConsumer_i *>           ac2;
  RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullConsumer_i *> sc2;
  RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullConsumer_i * >  bc2;
  for ( ac1 = _prx_any_push.cursor(); ac1.is_valid(); ++ac1 ) {
    if ((!only_cleanup_candidates) || (!ac1.val()->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, ac1.val());
    }
  }
  for ( sc1 = _prx_struc_push.cursor(); sc1.is_valid(); ++sc1 ) {
    if ((!only_cleanup_candidates) || (!sc1.val()->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, sc1.val());
    }
  }
  for ( bc1 = _prx_batch_push.cursor(); bc1.is_valid(); ++bc1 ) {
    if ((!only_cleanup_candidates) || (!bc1.val()->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, bc1.val());
    }
  }
  for ( ac2 = _prx_any_pull.cursor(); ac2.is_valid(); ++ac2 ) {
    if ((!only_cleanup_candidates) || (!ac2.val()->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, ac2.val());
    }
  }
  for ( sc2 = _prx_struc_pull.cursor(); sc2.is_valid(); ++sc2 ) {
    if ((!only_cleanup_candidates) || (!sc2.val()->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, sc2.val());
    }
  }
  for ( bc2 = _prx_batch_pull.cursor(); bc2.is_valid(); ++bc2 ) {
    if ((!only_cleanup_candidates) || (!bc2.val()->is_connected())) {
      ren[idx++] = WRAPPED_IMPL2OREF(AttN::Interactive, bc2.val());
    }
  }
  // trim to actual length
  ren.length(idx);
}

#undef WHATFN
#define WHATFN "SupplierAdmin_i::children"
AttN::IactSeq*
SupplierAdmin_i::children(CORBA::Boolean only_cleanup_candidates  WRAPPED_IMPLARG )
{
  // Unconnected proxies are cleanup candidates
  RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::IactSeq* ren = new AttN::IactSeq;
  if ( ren == (AttN::IactSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  _children(*ren, only_cleanup_candidates);
  return ren;
}

void
SupplierAdmin_i::cleanup(RDIstrstream& str, CORBA::Boolean admin, CORBA::Boolean proxies) {
  RDIInteractive::cleanup_admin(str, _my_oref, admin, proxies);
}

CORBA::Boolean
SupplierAdmin_i::do_set_command(RDIstrstream& str, RDIParseCmd& p) {
  CosN::QoSProperties    n_qos;
  CosN::AdminProperties  a_qos;
  AttN::ServerProperties s_qos;
  CORBA::Boolean success = 1;
  success = RDI_AllQoS::parse_set_command(str, p, RDI_S_ADMIN, n_qos, a_qos, s_qos);
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

#undef WHATFN
#define WHATFN "SupplierAdmin_i::do_go_command"
CORBA::Boolean
SupplierAdmin_i::do_go_command(RDIstrstream& str, RDIParseCmd& p,
			       CORBA::Boolean& target_changed,
			       AttN_Interactive_outarg next_target) {
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
  unsigned int i;

  { // introduce scope where this is locked
    RDI_OPLOCK_SCOPE_LOCK(sadmin_lock, WHATFN, RDI_THROW_INV_OBJREF);

    // CosEvent clients
    RDI_ListCursor<EventProxyPushConsumer_i *>                cpushcur;
    RDI_ListCursor<EventProxyPullConsumer_i *>                cpullcur;
    cpushcur = _cosevent_push.cursor();
    for (i=0; i < _cosevent_push.length(); ++i, ++cpushcur) {
      EventProxyPushConsumer_i* prx = *cpushcur;
      if (RDI_STR_EQ_I(go_targ, prx->_my_name[prx->_my_name.length()-1])) {
	targ1 = WRAPPED_IMPL2OREF(AttN::CosEvProxyPushConsumer, prx);
	targ1_set = 1;
	break;
      }
    }
    if (targ1_set == 0) {
      cpullcur = _cosevent_pull.cursor();
      for (i=0; i < _cosevent_pull.length(); ++i, ++cpullcur) {
	EventProxyPullConsumer_i* prx = *cpullcur;
	if (RDI_STR_EQ_I(go_targ, prx->_my_name[prx->_my_name.length()-1])) {
	  targ1 = WRAPPED_IMPL2OREF(AttN::CosEvProxyPullConsumer, prx);
	  targ1_set = 1;
	  break;
	}
      }
    }
    // CosNotify clients
    char buf[20];
    RDI_HashCursor<CosNA::ProxyID, ProxyPushConsumer_i *>           ac1;
    RDI_HashCursor<CosNA::ProxyID, StructuredProxyPushConsumer_i *> sc1;
    RDI_HashCursor<CosNA::ProxyID, SequenceProxyPushConsumer_i *>   bc1;
    RDI_HashCursor<CosNA::ProxyID, ProxyPullConsumer_i *>           ac2;
    RDI_HashCursor<CosNA::ProxyID, StructuredProxyPullConsumer_i *> sc2;
    RDI_HashCursor<CosNA::ProxyID, SequenceProxyPullConsumer_i *>   bc2;
    if (targ1_set == 0) {
      for ( ac1 = _prx_any_push.cursor(); ac1.is_valid(); ++ac1 ) {
	sprintf(buf, "proxy%ld", (long)ac1.key());
	if (RDI_STR_EQ_I(go_targ, buf)) {
	  targ1 = WRAPPED_IMPL2OREF(AttN::ProxyPushConsumer, ac1.val());
	  targ1_set = 1;
	  break;
	}
      }
    }
    if (targ1_set == 0) {
      for ( sc1 = _prx_struc_push.cursor(); sc1.is_valid(); ++sc1 ) {
	sprintf(buf, "proxy%ld", (long)sc1.key());
	if (RDI_STR_EQ_I(go_targ, buf)) {
	  targ1 = WRAPPED_IMPL2OREF(AttN::StructuredProxyPushConsumer, sc1.val());
	  targ1_set = 1;
	  break;
	}
      }
    }
    if (targ1_set == 0) {
      for ( bc1 = _prx_batch_push.cursor(); bc1.is_valid(); ++bc1 ) {
	sprintf(buf, "proxy%ld", (long)bc1.key());
	if (RDI_STR_EQ_I(go_targ, buf)) {
	  targ1 = WRAPPED_IMPL2OREF(AttN::SequenceProxyPushConsumer, bc1.val());
	  targ1_set = 1;
	  break;
	}
      }
    }
    if (targ1_set == 0) {
      for ( ac2 = _prx_any_pull.cursor(); ac2.is_valid(); ++ac2 ) {
	sprintf(buf, "proxy%ld", (long)ac2.key());
	if (RDI_STR_EQ_I(go_targ, buf)) {
	  targ1 = WRAPPED_IMPL2OREF(AttN::ProxyPullConsumer, ac2.val());
	  targ1_set = 1;
	  break;
	}
      }
    }
    if (targ1_set == 0) {
      for ( sc2 = _prx_struc_pull.cursor(); sc2.is_valid(); ++sc2 ) {
	sprintf(buf, "proxy%ld", (long)sc2.key());
	if (RDI_STR_EQ_I(go_targ, buf)) {
	  targ1 = WRAPPED_IMPL2OREF(AttN::StructuredProxyPullConsumer, sc2.val());
	  targ1_set = 1;
	  break;
	}
      }
    }
    if (targ1_set == 0) {
      for ( bc2 = _prx_batch_pull.cursor(); bc2.is_valid(); ++bc2 ) {
	sprintf(buf, "proxy%ld", (long)bc2.key());
	if (RDI_STR_EQ_I(go_targ, buf)) {
	  targ1 = WRAPPED_IMPL2OREF(AttN::SequenceProxyPullConsumer, bc2.val());
	  targ1_set = 1;
	  break;
	}
      }
    }
  }
  // end of scope, this is no longer locked
  if (targ1_set == 0) {
    str << "Invalid target " << p.argv[1] << " : " << go_targ << " is not a proxy name\n";
    str << "  (Use 'children' for list of valid proxy names)\n";
    success = 0;
  } else {
    str << "\nomniNotify: new target ==> " << p.argv[1] << '\n';
    if (rest_go_targ && (RDI_STRLEN(rest_go_targ) > 0)) {
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

void SupplierAdmin_i::out_config(RDIstrstream& str) {
  str << "----------------------------------------------------------------------\n";
  str << "NotifQos settings for " << _my_name << '\n';
  str << "----------------------------------------------------------------------\n";
  _qosprop->log_output(str);
  str << '\n';
}

void SupplierAdmin_i::out_commands(RDIstrstream& str) {
  str << "omniNotify SupplierAdmin commands:\n"
      << "  debug             : show debugging information for this admin + its proxies\n"
      << "  up                : change target to admin's channel\n"
      << "  go <proxy_name>   : change target to a specific proxy\n"
      << "                        ('children' lists proxy names)\n"
      << "  config            : show config information for this admin + its proxies\n"
      << "  info filters      : show filters attached to this admin and its proxies\n"
      << "  info afilters     : show filters attached to this admin\n"
      << "  info pfilters     : show filters attached to this admin's proxies\n"
      << "  cleanup proxies   : destroy all proxies of this admin that are not connected\n"
      << "  set <name1> <value1> [ <name2> <value2> ... ]\n"
      << "                    : set NotifQoS property name1 to value1, name2 to value2, etc.\n";
}

char*
SupplierAdmin_i::do_command(const char* cmnd, CORBA::Boolean& success,
			    CORBA::Boolean& target_changed,
			    AttN_Interactive_outarg next_target  WRAPPED_IMPLARG )
{
  RDIParseCmd p(cmnd);
  success = 1;
  target_changed = 0;
  if (p.argc == 0) { return CORBA_STRING_DUP(""); }

  RDIstrstream str;
  if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "help")) {
    out_commands(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "debug")) {
    log_output(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "config")) {
    out_config(str);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "info") && RDI_STR_EQ_I(p.argv[1], "filters")) {
    out_info_filters(str, 1, 1);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "info") && RDI_STR_EQ_I(p.argv[1], "afilters")) {
    out_info_filters(str, 1, 0);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "info") && RDI_STR_EQ_I(p.argv[1], "pfilters")) {
    out_info_filters(str, 0, 1);
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "cleanup") && RDI_STR_EQ_I(p.argv[1], "proxies")) {
    cleanup(str, 0, 1);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "up")) {
    target_changed = 1;
    next_target = WRAPPED_IMPL2OREF(AttN::EventChannel, _channel);
    str << "\nomniNotify: new target ==> " << _my_name[_my_name.length()-2] << '\n';
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

