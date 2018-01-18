// -*- Mode: C++; -*-
//                              File      : CosEventChannelAdmin_i.h
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
//     set of C++ definitions for the CosEventChanneAdmin module
//

#ifndef _COS_EVENT_CHANNEL_ADMIN_I_H_
#define _COS_EVENT_CHANNEL_ADMIN_I_H_

#include "corba_wrappers.h"
#include "thread_wrappers.h"
#include "RDIstrstream.h"
#include "RDIOplocks.h"
#include "RDIEvent.h"
#include "RDIChannelUtil.h"
#include "CosNotifyShorthands.h"
#include "RDIList.h"
#include "RDILocksHeld.h"

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// While the Notification Service can be used to offer the functionality //
// of the Event Service,  we need to implement the Proxy objects for the //
// Event Service in order to have full backward compatibility.           //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

class EventProxyPushConsumer_i : 
	WRAPPED_SKELETON_SUPER(AttNotification, CosEvProxyPushConsumer) {
  friend class SupplierAdmin_i;
public:
  EventProxyPushConsumer_i(SupplierAdmin_i* admin, EventChannel_i* channel,
			   const CosNA::ProxyID& prxID);

  // Methods from AttNotification::Interactive Interface
  char* do_command(const char* cmd, CORBA::Boolean& success, CORBA::Boolean& target_changed,
		   AttN_Interactive_outarg next_target  WRAPPED_DECLARG);
  AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID );
  AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID );
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates);
  CORBA::Boolean safe_cleanup();

  // Methods from CosEvC::PushConsumer Interface
  void push(const CORBA::Any& data  WRAPPED_DECLARG );
  void disconnect_push_consumer( WRAPPED_DECLARG_VOID );

  // Methods from CosEvCA::ProxyPushConsumer Interface
  void connect_push_supplier(CosEvC::PushSupplier_ptr supplier  
			     WRAPPED_DECLARG );

  // (Local only -- not available via RPC)
  CORBA::Boolean is_connected() const { return _pxstate == RDI_Connected; }
  void disconnect_client_and_dispose(RDI_LocksHeld&  held,
				     CORBA::Boolean  remove_proxy_from_admin);
#ifndef NO_OBJ_GC
  CORBA::Boolean obj_gc(RDI_TimeT curtime, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy);
#endif

  RDIstrstream& log_output(RDIstrstream& str) const;

  // Local-only static functions
  static void out_commands(RDIstrstream& str);

  const AttN::NameSeq& L_my_name() { return _my_name; }

private:
  RDIOplockEntry*                 _oplockptr;
#ifndef NO_OBJ_GC
  RDI_TimeT	                  _last_use;
#endif
  AttN::NameSeq                   _my_name;
  EventChannel_i*                 _channel;
  SupplierAdmin_i*                _myadmin;
  CORBA::ULong                    _nevents;
  RDI_ProxyState                  _pxstate;
  CosNA::ProxyID                  _pserial;
  CosEvC::PushSupplier_var        _supplier;

  // The real implementation (caller must have bumped scope lock)
  void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
				      CORBA::Boolean            remove_proxy_from_admin,
				      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  virtual ~EventProxyPushConsumer_i();
};

class EventProxyPullConsumer_i : 
	public virtual RDIProxyPullConsumer, 
	WRAPPED_SKELETON_SUPER(AttNotification, CosEvProxyPullConsumer) {
  friend class SupplierAdmin_i;
public:
  EventProxyPullConsumer_i(SupplierAdmin_i* admin, EventChannel_i* channel,
			   const CosNA::ProxyID& prxID);

  // Methods from AttNotification::Interactive Interface
  char* do_command(const char* cmd, CORBA::Boolean& success, CORBA::Boolean& target_changed,
		   AttN_Interactive_outarg next_target  WRAPPED_DECLARG);
  AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID );
  AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID );
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates);
  CORBA::Boolean safe_cleanup();

  // Methods from CosEvC::PullConsumer Interface
  void disconnect_pull_consumer( WRAPPED_DECLARG_VOID );

  // Methods from CosEvCA::ProxyPullConsumer Interface
  void connect_pull_supplier(CosEvC::PullSupplier_ptr supplier
			     WRAPPED_DECLARG );

  // (Local only -- not available via RPC)
  CORBA::Boolean is_connected() const { return _pxstate == RDI_Connected; }
  virtual CORBA::Boolean is_available(unsigned long* wait_s, unsigned long* wait_t);
  virtual void pull_event(CORBA::Boolean& invalid);
  void disconnect_client_and_dispose(RDI_LocksHeld&  held,
				     CORBA::Boolean  remove_proxy_from_admin);
#ifndef NO_OBJ_GC
  CORBA::Boolean obj_gc(RDI_TimeT curtime, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy);
#endif

  RDIstrstream& log_output(RDIstrstream& str) const;

  // Local-only static functions
  static void out_commands(RDIstrstream& str);

  const AttN::NameSeq& L_my_name() { return _my_name; }

private:
  RDIOplockEntry*                 _oplockptr;
#ifndef NO_OBJ_GC
  RDI_TimeT	                  _last_use;
#endif
  AttN::NameSeq                   _my_name;
  EventChannel_i*                 _channel;
  SupplierAdmin_i*                _myadmin;
  TW_Thread*                      _pworker;
  CORBA::Boolean                  _thrdone; // true when worker thread exits
  CORBA::ULong                    _nevents;
  RDI_ProxyState                  _pxstate;
  CosNA::ProxyID                  _pserial;
  CosEvC::PullSupplier_var        _supplier;

  // absolute time of next timeout
  unsigned long                   _timeout_s;
  unsigned long                   _timeout_n;

  void _pull();

  // The real implementation (caller must have bumped scope lock)
  void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
				      CORBA::Boolean            remove_proxy_from_admin,
				      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  virtual ~EventProxyPullConsumer_i();
};

class EventProxyPushSupplier_i : 
	public virtual RDIProxyPushSupplier, 
	WRAPPED_SKELETON_SUPER(AttNotification, CosEvProxyPushSupplier) {
  friend class ConsumerAdmin_i;
public:
  EventProxyPushSupplier_i(ConsumerAdmin_i* admin, EventChannel_i* channel,
			   const CosNA::ProxyID& prxID);

  // Methods from AttNotification::Interactive Interface
  char* do_command(const char* cmd, CORBA::Boolean& success, CORBA::Boolean& target_changed,
		   AttN_Interactive_outarg next_target  WRAPPED_DECLARG);
  AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID );
  AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID );
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates);
  CORBA::Boolean safe_cleanup();

  // Methods from CosEvCA::ProxyPushSupplier Interface
  void connect_push_consumer(CosEvC::PushConsumer_ptr consumer
			     WRAPPED_DECLARG );

  // Methods from CosEvC::PushSupplier Interface
  void disconnect_push_supplier( WRAPPED_DECLARG_VOID );

  // (Local only -- not available via RPC)
  CORBA::Boolean is_connected() const { return _pxstate == RDI_Connected; }
  void add_event(RDI_StructuredEvent* event);
  virtual void push_event(CORBA::Boolean& invalid);
  void disconnect_client_and_dispose(RDI_LocksHeld&  held,
				     CORBA::Boolean  remove_proxy_from_admin);
#ifndef NO_OBJ_GC
  CORBA::Boolean obj_gc(RDI_TimeT curtime, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy);
#endif
  virtual CORBA::Boolean has_events(unsigned long*, unsigned long*) { return _cnfqueue.length() ? 1 : 0; }

  RDIstrstream& log_output(RDIstrstream& str) const;

  // Local-only static functions
  static void out_commands(RDIstrstream& str);

  const AttN::NameSeq& L_my_name() { return _my_name; }

private:
  RDIOplockEntry*                 _oplockptr;
#ifndef NO_OBJ_GC
  RDI_TimeT	                  _last_use;
#endif
  AttN::NameSeq                   _my_name;
  EventChannel_i*                 _channel;
  ConsumerAdmin_i*                _myadmin;
  TW_Thread*           _pworker;
  CORBA::Boolean                  _thrdone;	// true when worker thread exits
  CORBA::ULong                    _nevents;
  RDI_ProxyState                  _pxstate;
  CosNA::ProxyID                  _pserial;
  CosEvC::PushConsumer_var        _consumer;
  RDI_List<RDI_StructuredEvent *> _cnfqueue;

  void _push();

  // The real implementation (caller must have bumped scope lock)
  void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
				      CORBA::Boolean            remove_proxy_from_admin,
				      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  // Remove all events from _cnfqueue, e.g., due to disconnect or _pxstate becoming RDI_Exception
  void _clear_cnfqueue();

  virtual ~EventProxyPushSupplier_i();
};

class EventProxyPullSupplier_i : 
	WRAPPED_SKELETON_SUPER(AttNotification, CosEvProxyPullSupplier) {
  friend class ConsumerAdmin_i;
public:
  EventProxyPullSupplier_i(ConsumerAdmin_i* admin, EventChannel_i* channel,
			   const CosNA::ProxyID& prxID);

  // Methods from AttNotification::Interactive Interface
  char* do_command(const char* cmd, CORBA::Boolean& success, CORBA::Boolean& target_changed,
		   AttN_Interactive_outarg next_target  WRAPPED_DECLARG);
  AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID );
  AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID );
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates);
  CORBA::Boolean safe_cleanup();

  // Methods from CosEvCA::ProxyPullSupplier
  void  connect_pull_consumer(CosEvC::PullConsumer_ptr consumer
			      WRAPPED_DECLARG );

  // Methods from CosEvC::PullSupplier
  CORBA::Any* pull( WRAPPED_DECLARG_VOID );
  CORBA::Any* try_pull(CORBA::Boolean& has_event WRAPPED_DECLARG );
  void        disconnect_pull_supplier( WRAPPED_DECLARG_VOID );

  // (Local only -- not available via RPC)
  CORBA::Boolean is_connected() const { return _pxstate == RDI_Connected; }
  void add_event(RDI_StructuredEvent* event);
  void disconnect_client_and_dispose(RDI_LocksHeld&  held,
				     CORBA::Boolean  remove_proxy_from_admin);
#ifndef NO_OBJ_GC
  CORBA::Boolean obj_gc(RDI_TimeT curtime, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy);
#endif

  RDIstrstream& log_output(RDIstrstream& str) const;

  // Local-only static functions
  static void out_commands(RDIstrstream& str);

  const AttN::NameSeq& L_my_name() { return _my_name; }

private:
  RDIOplockEntry*                 _oplockptr;
#ifndef NO_OBJ_GC
  RDI_TimeT	                  _last_use;
#endif
  AttN::NameSeq                   _my_name;
  EventChannel_i*                 _channel;
  ConsumerAdmin_i*                _myadmin;
  CORBA::ULong                    _nevents;
  RDI_ProxyState                  _pxstate;
  CosNA::ProxyID                  _pserial;
  CosEvC::PullConsumer_var        _consumer;
  RDI_List<RDI_StructuredEvent *> _cnfqueue;

  // The real implementation (caller must have bumped scope lock)
  void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
				      CORBA::Boolean            remove_proxy_from_admin,
				      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  // Remove all events from _cnfqueue, e.g., due to disconnect or _pxstate becoming RDI_Exception
  void _clear_cnfqueue();

  virtual ~EventProxyPullSupplier_i();
};

////////////////////////////////////////
// Logging

inline RDIstrstream& operator<< (RDIstrstream& str, const EventProxyPushConsumer_i& prx) { return prx.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, const EventProxyPullConsumer_i& prx) { return prx.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, const EventProxyPushSupplier_i& prx) { return prx.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, const EventProxyPullSupplier_i& prx) { return prx.log_output(str); }

#endif
