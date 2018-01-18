// -*- Mode: C++; -*-
//                              File      : CosNotifyChannelAdmin_i.h
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
//    set of C++ definitions for the CosNotifyChannelAdmin module
//
 
#ifndef _COS_NOTIFY_CHANNEL_ADMIN_I_H_
#define _COS_NOTIFY_CHANNEL_ADMIN_I_H_

#include "thread_wrappers.h"
#include "corba_wrappers.h"
#include "CosNotifyShorthands.h"
#include "CosNotifyFilter_i.h"
#include "CosNotification_i.h"
#include "RDIstrstream.h"
#include "RDIOplocks.h"
#include "RDIEvent.h"
#include "RDIEventQueue.h"
#include "RDIChannelUtil.h"
#include "RDINotifQueue.h"
#include "RDITimeWrappers.h"
#include "RDIList.h"
#include "RDILocksHeld.h"
#include "RDISeqDecls.h"

class RDI_TypeMap;
class EventChannelFactory_i;

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//   GENERAL NOTES                                                       //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//
// Proxy disposal is tricky because there can be outstanding outcalls
// at the time a disposal is requested.  If there are *incoming* calls
// when an object is disposed then the BOA (or POA) will not actually
// invoke the delete operator on the object until these complete,
// but we also need to keep the BOA/POA from invoking the delete operator
// while there is an outstanding outcall to a client running in a local
// method that is not associated with an incoming call.  Each proxy has
// an _oplockptr which is a reference to an externally managed
// RDIOplockEntry which supports deferred disposal of an entry whose
// 'inuse' count is > 0 at the time the entr is released.  
//
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//               Helper for All Proxy Supplier implementations           //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

class RDIProxySupplier : 
  public RDINotifySubscribe
{
  friend class RDI_ChangePool;
  friend class EventChannel_i;
  friend class ConsumerAdmin_i;
public:
  RDIProxySupplier(const char*           resty,
		   const char*           fa_helper_resty,
		   ConsumerAdmin_i*      admin, 
		   EventChannel_i*       chann,
		   const RDI_ObjectKind  otype,
		   const CosNA::ProxyType prtype,
		   const CosNA::ProxyID&  prxID);

  // Methods from AttN::Interactive Interface
  char* do_command(const char* cmd, CORBA::Boolean& success, CORBA::Boolean& target_changed,
		   AttN_Interactive_outarg next_target);
  AttN::NameSeq* child_names();
  AttN::NameSeq* my_name();
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates);
  CORBA::Boolean safe_cleanup();

  // Methods from CosNA::ProxySupplier Interface
  CosNA::ProxyType         MyType();
  CosNA::ProxyID           MyID();
  CosNA::ConsumerAdmin_ptr MyAdmin();
  CosNF::MappingFilter_ptr priority_filter();
  void                     priority_filter(CosNF::MappingFilter_ptr map_filter);
  CosNF::MappingFilter_ptr lifetime_filter();
  void                     lifetime_filter(CosNF::MappingFilter_ptr life_filter);
  CosN::EventTypeSeq*      obtain_offered_types(CosNA::ObtainInfoMode mode);
  void validate_event_qos(const CosN::QoSProperties& r_qos,
			  CosN_NamedPropertyRangeSeq_outarg a_qos);

  // Methods from CosN::QoSAdmin Interface
  CosN::QoSProperties* get_qos();
  void set_qos(const CosN::QoSProperties& qos);
  void validate_qos(const CosN::QoSProperties& r_qos,
		    CosN_NamedPropertyRangeSeq_outarg a_qos);

  // Methods from CosNF::FilterAdmin Interface
  CosNF::FilterID     add_filter(CosNF::Filter_ptr filter);
  void                remove_filter(CosNF::FilterID fltrID);
  void                remove_all_filters();
  CosNF::Filter_ptr   get_filter(CosNF::FilterID fltrID);
  CosNF::FilterIDSeq* get_all_filters();

  // Methods from CosNC::NotifySubscribe Interface
  void subscription_change(const CosN::EventTypeSeq& added,
			   const CosN::EventTypeSeq& deled);

  // (Local only -- not available via RPC)

  CORBA::Boolean is_connected() const { return _pxstate == RDI_Connected; }
  CORBA::Boolean is_active()    const { return _active; }
  CORBA::Boolean has_filters()  const { return (_rqstypes.length() ? 1 : _fa_helper.has_filters()); }
#ifndef NO_OBJ_GC
  CORBA::Boolean obj_gc(RDI_TimeT curtime, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy);
#endif

  // helper routines called by _channel
  void _enable_updates();
  void _disable_updates();

  // RDINotifySubscribe methods:
  void filter_destroy_i(Filter_i* filter);
  void propagate_schange(RDI_LocksHeld&             held,
			 const CosN::EventTypeSeq&  added,
			 const CosN::EventTypeSeq&  deled,
			 Filter_i*                  filter);

  // these methods are called when the public methods cannot be used
  // (the public methods obtain OPLOCK)
  CosNA::ProxyID _proxy_id() { return _pserial; } // used when proxy lock already held [MyID obtains lock]

  const AttN::NameSeq& L_my_name() { return _my_name; }

  // Local-only functions
  CORBA::Boolean do_set_command(RDIstrstream& str, RDIParseCmd& p);
  void out_config(RDIstrstream& str);
  void out_commands(RDIstrstream& str);
  void out_info_filters(RDIstrstream& str);

protected:
  RDIOplockEntry*           _oplockptr;
#ifndef NO_OBJ_GC
  RDI_TimeT	            _last_use;
#endif
  AttN::NameSeq             _my_name;
  FAdminHelper              _fa_helper;
  EventChannel_i*           _channel;
  ConsumerAdmin_i*          _myadmin;
  RDI_ObjectKind            _otype;
  CosNA::ProxyType          _prxtype;
  CosNA::ProxyID            _pserial;
  CORBA::ULong              _nevents;
  RDI_ProxyState            _pxstate;
  CORBA::Boolean            _active;	// TRUE if connection is active
  RDI_NotifQoS*             _qosprop;
  CORBA::Boolean            _oc_off;	// TRUE if offer_change() is disabled
  CosN::EventTypeSeq        _rqstypes;
  CosNF::MappingFilter_var  _pfilter;
  CosNF::MappingFilter_var  _lfilter;
  CosNC::NotifyPublish_var  _oc_subscriber;
  RDINotifQueue             _ntfqueue;

  // This is invoked by the administrative object during destruction.
  void disconnect_client_and_dispose(RDI_LocksHeld&  held,
				     CORBA::Boolean  remove_proxy_from_admin);

  // Every real class must implement this helper method  which does the real work
  // (caller must have bumped scope lock)
  virtual void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
					      CORBA::Boolean            remove_proxy_from_admin,
					      WRAPPED_DISPOSEINFO_PTR&  dispose_info) = 0;

  // Every real class must implement this logging method
  virtual RDIstrstream& log_output(RDIstrstream& str) const = 0;

  // Invoked by RDI_ChangePool threads
  CORBA::Boolean send_offer_change(const CosN::EventTypeSeq& added,
				   const CosN::EventTypeSeq& deled);

  // Does the main add_event work for each add_event below
  CORBA::Boolean _add_event(RDI_StructuredEvent* event);

  // Remove all events from _ntfqueue, e.g., due to disconnect or _pxstate becoming RDI_Exception
  void _clear_ntfqueue();

  void _qos_changed(RDI_LocksHeld& held);

  virtual ~RDIProxySupplier();
};

// The following macro inserts declarations into a real proxy class
// which define each of the given methods to use the method inherited
// from the RDIProxySupplier helper class.  (If the method has inline
// code we just put the code here, in which case the method does not
// appear above, even though it could/should.).  This is done because
// we only declare the IDL-generated skeleton class in the real proxy
// class, and must provide explicit implementations of all of the
// external methods within the proxy class -- cannot declare 
// any skeleton classes for helper class without causing compilation problems.
// If someone knows a better approach, please let us know!

#define UseRDIProxySupplierMethods 							\
  char* do_command(const char* cmd, CORBA::Boolean& success,				\
		   CORBA::Boolean& target_changed, 					\
		   AttN_Interactive_outarg next_target  WRAPPED_DECLARG )		\
   { return RDIProxySupplier::do_command(cmd, success, target_changed, next_target); }	\
  AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID )					\
      { return RDIProxySupplier::child_names(); }					\
  AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID )					\
      { return RDIProxySupplier::my_name(); }						\
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates  WRAPPED_DECLARG ) 	\
      { return RDIProxySupplier::children(only_cleanup_candidates); }			\
  CORBA::Boolean safe_cleanup( WRAPPED_DECLARG_VOID ) 					\
      { return RDIProxySupplier::safe_cleanup(); }					\
  CosNA::ProxyType 									\
  MyType( WRAPPED_DECLARG_VOID ) 							\
      { return RDIProxySupplier::MyType(); } 						\
  CosNA::ProxyID 									\
  MyID( WRAPPED_DECLARG_VOID ) 								\
      { return RDIProxySupplier::MyID(); }						\
  CosNA::ConsumerAdmin_ptr 								\
  MyAdmin( WRAPPED_DECLARG_VOID ) 							\
      { return RDIProxySupplier::MyAdmin(); } 						\
  CosNF::MappingFilter_ptr 								\
  priority_filter( WRAPPED_DECLARG_VOID ) 						\
      { return RDIProxySupplier::priority_filter(); } 					\
  void 											\
  priority_filter(CosNF::MappingFilter_ptr map_filter WRAPPED_DECLARG ) 		\
      { RDIProxySupplier::priority_filter(map_filter); } 				\
  CosNF::MappingFilter_ptr 								\
  lifetime_filter( WRAPPED_DECLARG_VOID ) 						\
      { return RDIProxySupplier::lifetime_filter(); } 					\
  void 											\
  lifetime_filter(CosNF::MappingFilter_ptr life_filter  WRAPPED_DECLARG ) 		\
      { RDIProxySupplier::lifetime_filter(life_filter); } 				\
  CosN::EventTypeSeq* 									\
  obtain_offered_types(CosNA::ObtainInfoMode mode     WRAPPED_DECLARG ) 		\
      { return RDIProxySupplier::obtain_offered_types(mode); } 				\
  void 											\
  validate_event_qos(const CosN::QoSProperties& r_qos, 					\
                     CosN_NamedPropertyRangeSeq_outarg a_qos 				\
                     WRAPPED_DECLARG ) 							\
      { RDIProxySupplier::validate_event_qos(r_qos, a_qos); } 				\
  CosN::QoSProperties* 									\
  get_qos( WRAPPED_DECLARG_VOID ) 							\
      { return RDIProxySupplier::get_qos(); } 						\
  void 											\
  set_qos(const CosN::QoSProperties& qos WRAPPED_DECLARG ) 				\
      { RDIProxySupplier::set_qos(qos); } 						\
  void 											\
  validate_qos(const CosN::QoSProperties& r_qos, 					\
               CosN_NamedPropertyRangeSeq_outarg a_qos 					\
               WRAPPED_DECLARG ) 							\
      { RDIProxySupplier::validate_qos(r_qos, a_qos); } 				\
  CosNF::FilterID 									\
  add_filter(CosNF::Filter_ptr filter WRAPPED_DECLARG ) 				\
      { return RDIProxySupplier::add_filter(filter); } 					\
  void 											\
  remove_filter(CosNF::FilterID fltrID WRAPPED_DECLARG ) 				\
      { RDIProxySupplier::remove_filter(fltrID); } 					\
  void 											\
  remove_all_filters( WRAPPED_DECLARG_VOID ) 						\
      { RDIProxySupplier::remove_all_filters(); } 					\
  CosNF::Filter_ptr 									\
  get_filter(CosNF::FilterID fltrID WRAPPED_DECLARG ) 					\
      { return RDIProxySupplier::get_filter(fltrID); } 					\
  CosNF::FilterIDSeq* 									\
  get_all_filters( WRAPPED_DECLARG_VOID ) 						\
      { return RDIProxySupplier::get_all_filters(); } 					\
  void 											\
  subscription_change(const CosN::EventTypeSeq& added, 					\
                      const CosN::EventTypeSeq& deled WRAPPED_DECLARG ) 		\
      { RDIProxySupplier::subscription_change(added, deled); } 				\


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//               Pull-based Supplier Proxy implementations               //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

/** ProxyPullSupplier
  * This proxy object is connected to a pull consumer and provides
  * CORBA::Any events
  */

class ProxyPullSupplier_i : 
  public virtual RDIProxySupplier,
  WRAPPED_SKELETON_SUPER(AttNotification, ProxyPullSupplier) 
{
  friend class EventChannel_i;
  friend class ConsumerAdmin_i;
public:
  ProxyPullSupplier_i(ConsumerAdmin_i* admin,
		      EventChannel_i*  chann,
		      const CosNA::ProxyID& prxID);

  // See comments for this macro above
  UseRDIProxySupplierMethods

  // Methods from CosNA::ProxyPullSupplier Interface
  void connect_any_pull_consumer(CosEvC::PullConsumer_ptr cons
			         WRAPPED_DECLARG );

  // Methods from CosEvC::PullSupplier Interface
  CORBA::Any* pull( WRAPPED_DECLARG_VOID );
  CORBA::Any* try_pull(CORBA::Boolean& has_event WRAPPED_DECLARG );
  void        disconnect_pull_supplier( WRAPPED_DECLARG_VOID );

  // (Local only -- not available via RPC)

  void add_event(RDI_StructuredEvent* event);

  virtual RDIstrstream& log_output(RDIstrstream& str) const;

private:
  CosEvC::PullConsumer_var  _consumer;
  CosNC::PullConsumer_var          _nc_consumer;

  // The real implementation (caller must have bumped scope lock)
  virtual void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
					      CORBA::Boolean            remove_proxy_from_admin,
					      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  virtual ~ProxyPullSupplier_i();
};

/** StructuredProxyPullSupplier
  * This proxy object is connected to a pull consumer and provides
  * CosN::StructuredEvent events 
  */

class StructuredProxyPullSupplier_i : 
  	public virtual RDIProxySupplier,
  	WRAPPED_SKELETON_SUPER(AttNotification, StructuredProxyPullSupplier) 
{
  friend class EventChannel_i;
  friend class ConsumerAdmin_i;
 public:
  StructuredProxyPullSupplier_i(ConsumerAdmin_i* admin,
				EventChannel_i*  chann, 
				const CosNA::ProxyID& prxID);

  // See comments for this macro above
  UseRDIProxySupplierMethods

  // Methods from CosNA::StructuredProxyPullSupplier Interface
  void connect_structured_pull_consumer( 
		CosNC::StructuredPullConsumer_ptr consumer WRAPPED_DECLARG );

  // Methods from CosNC::StructuredPullSupplier Interface
  CosN::StructuredEvent* pull_structured_event( WRAPPED_DECLARG_VOID );
  CosN::StructuredEvent* try_pull_structured_event(CORBA::Boolean& has_event
					          WRAPPED_DECLARG );
  void disconnect_structured_pull_supplier( WRAPPED_DECLARG_VOID );

  // (Local only -- not available via RPC)

  // Add a new event in the notification queue
  void add_event(RDI_StructuredEvent* event);

  virtual RDIstrstream& log_output(RDIstrstream& str) const;

private:
  CosNC::StructuredPullConsumer_var _consumer;

  // The real implementation (called must have bumped scope lock):
  virtual void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
					      CORBA::Boolean            remove_proxy_from_admin,
					      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  virtual ~StructuredProxyPullSupplier_i();
};

/** SequenceProxyPullSupplier
  * This proxy object is connected to a pull consumer and provides
  * CosN::EventBatch events
  */

class SequenceProxyPullSupplier_i : 
  	public virtual RDIProxySupplier, 
  	WRAPPED_SKELETON_SUPER(AttNotification, SequenceProxyPullSupplier) 
{
  friend class EventChannel_i;
  friend class ConsumerAdmin_i;
 public:
  SequenceProxyPullSupplier_i(ConsumerAdmin_i* admin, 
			      EventChannel_i*  chann, 
			      const CosNA::ProxyID& prxID);

  // See comments for this macro above
  UseRDIProxySupplierMethods

  // Methods from CosNA::SequenceProxyPullSupplier Interface
  void connect_sequence_pull_consumer(
		CosNC::SequencePullConsumer_ptr consumer WRAPPED_DECLARG );

  // Methods from CosNC::SequencePullSupplier Interface
  CosN::EventBatch* pull_structured_events(CORBA::Long max_number 
					  WRAPPED_DECLARG );
  CosN::EventBatch* try_pull_structured_events(CORBA::Long max_number,
                                              CORBA::Boolean& has_event
				              WRAPPED_DECLARG );
  void disconnect_sequence_pull_supplier( WRAPPED_DECLARG_VOID );

  // (Local only -- not available via RPC)

  // Add a new event in the notification queue
  void add_event(RDI_StructuredEvent* event);

  virtual RDIstrstream& log_output(RDIstrstream& str) const;

private:
  CosNC::SequencePullConsumer_var  _consumer;

  // absolute time of next timeout (for blocking pull_structured_events calls)
  unsigned long                   _timeout_s;
  unsigned long                   _timeout_n;

  // The real implementation (called must have bumped scope lock)
  virtual void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
					      CORBA::Boolean            remove_proxy_from_admin,
					      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  virtual ~SequenceProxyPullSupplier_i();
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//               Push-based Supplier Proxy implementations               //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

/** ProxyPushSupplier
  * This proxy object is connected to a push consumer and provides
  * CORBA::Any events
  */

class ProxyPushSupplier_i : 
  	public virtual RDIProxySupplier, 
	public virtual RDIProxyPushSupplier, 
	WRAPPED_SKELETON_SUPER(AttNotification, ProxyPushSupplier)
{
  friend class EventChannel_i;
  friend class ConsumerAdmin_i;
public:
  ProxyPushSupplier_i(ConsumerAdmin_i* admin, 
		      EventChannel_i*  chann, const CosNA::ProxyID& prxID);

  // See comments for this macro above
  UseRDIProxySupplierMethods

  // Methods from CosNA::ProxyPushSupplier Interface
  void connect_any_push_consumer(
		CosEvC::PushConsumer_ptr consumer WRAPPED_DECLARG );
  void suspend_connection( WRAPPED_DECLARG_VOID );
  void resume_connection( WRAPPED_DECLARG_VOID );

  // Methods from CosEvC::PushSupplier Interface
  void disconnect_push_supplier( WRAPPED_DECLARG_VOID );

  // (Local only -- not available via RPC)

  virtual CORBA::Boolean has_events(unsigned long* wait_s, unsigned long* wait_t);
  void add_event(RDI_StructuredEvent* event);
  virtual void push_event(CORBA::Boolean& invalid);

  virtual RDIstrstream& log_output(RDIstrstream& str) const;

private:
  TW_Thread*                _worker;
  CosEvC::PushConsumer_var  _consumer;
  CosNC::PushConsumer_var   _nc_consumer;

  void _push_event();

  // The real implementation (called must have bumped scope lock):
  virtual void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
					      CORBA::Boolean            remove_proxy_from_admin,
					      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  virtual ~ProxyPushSupplier_i();
};

/** StructuredProxyPushSupplier
  * This proxy object is connected to a push consumer and provides
  * CosN::StructuredEvent events
  */

class StructuredProxyPushSupplier_i : 
  	public virtual RDIProxySupplier, 
	public virtual RDIProxyPushSupplier, 
	WRAPPED_SKELETON_SUPER(AttNotification, StructuredProxyPushSupplier) 
{
  friend class EventChannel_i;
  friend class ConsumerAdmin_i;
 public:
  StructuredProxyPushSupplier_i(ConsumerAdmin_i* admin, 
				EventChannel_i*  chann, 
				const CosNA::ProxyID& prxID);

  // See comments for this macro above
  UseRDIProxySupplierMethods

  // Methods from CosNA::StructuredProxyPushSupplier Interface
  void connect_structured_push_consumer(
		CosNC::StructuredPushConsumer_ptr consumer WRAPPED_DECLARG );
  void suspend_connection( WRAPPED_DECLARG_VOID );
  void resume_connection( WRAPPED_DECLARG_VOID );

  // Methods from CosNC::StructuredPushSupplier Interface
  void disconnect_structured_push_supplier( WRAPPED_DECLARG_VOID );

  // (Local only -- not available via RPC)

  virtual CORBA::Boolean has_events(unsigned long* wait_s, unsigned long* wait_t);
  void add_event(RDI_StructuredEvent* event);
  virtual void push_event(CORBA::Boolean& invalid);

  virtual RDIstrstream& log_output(RDIstrstream& str) const;

private:
  TW_Thread*                         _worker;
  CosNC::StructuredPushConsumer_var  _consumer;

  void _push_event();

  // The real implementation (called must have bumped scope lock):
  virtual void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
					      CORBA::Boolean            remove_proxy_from_admin,
					      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  virtual ~StructuredProxyPushSupplier_i();
};

/** SequenceProxyPushSupplier
  * This proxy object is connected to a push consumer and provides
  * CosN::EventBatch events
  */

class SequenceProxyPushSupplier_i : 
  	public virtual RDIProxySupplier, 
	public virtual RDIProxyPushSupplier, 
	WRAPPED_SKELETON_SUPER(AttNotification, SequenceProxyPushSupplier) 
{
  friend class EventChannel_i;
  friend class ConsumerAdmin_i;
public:
  SequenceProxyPushSupplier_i(ConsumerAdmin_i* admin, 
			      EventChannel_i*  chann, 
			      const CosNA::ProxyID& prxID);

  // See comments for this macro above
  UseRDIProxySupplierMethods

  // Methods from CosNA::SequenceProxyPushSupplier Interface
  void connect_sequence_push_consumer(
		CosNC::SequencePushConsumer_ptr consumer WRAPPED_DECLARG );
  void suspend_connection( WRAPPED_DECLARG_VOID );
  void resume_connection( WRAPPED_DECLARG_VOID );

  // Methods from CosNC::StructuredPushSupplier Interface
  void disconnect_sequence_push_supplier( WRAPPED_DECLARG_VOID );

  // (Local only -- not available via RPC)

  // has_events determines whether a push should occur.
  // in this case the requirements are that either the max batch size
  // has been reached or there is at least 1 event and pacing interval reached
  virtual CORBA::Boolean has_events(unsigned long* wait_s, unsigned long* wait_n);

  // If there is >= 1 event but a timeout has not occured, wait_s and wait_t
  // are updated to the timeout point (unless they already contain an earlier
  // non-zero timeout point).

  void add_event(RDI_StructuredEvent* event);
  virtual void push_event(CORBA::Boolean& invalid);

  virtual RDIstrstream& log_output(RDIstrstream& str) const;

private:
  TW_Thread*                       _worker;
  CosNC::SequencePushConsumer_var  _consumer;

  // absolute time of next timeout
  unsigned long                   _timeout_s;
  unsigned long                   _timeout_n;

  void _push_event();

  // The real implementation (called must have bumped scope lock):
  virtual void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
					      CORBA::Boolean            remove_proxy_from_admin,
					      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  virtual ~SequenceProxyPushSupplier_i();
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//               Helper for All Proxy Consumer implementations           //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

class RDIProxyConsumer :
  public RDINotifySubscribe
{
  friend class RDI_ChangePool;
  friend class EventChannel_i;
  friend class SupplierAdmin_i;
public:
  RDIProxyConsumer(const char*	resty,
		   const char*  fa_helper_resty,
		   SupplierAdmin_i* admin, 
		   EventChannel_i*  chann,
		   const RDI_ObjectKind  otype,
		   const CosNA::ProxyType prtype,
		   const CosNA::ProxyID& prxID);

  // Methods from AttN::Interactive Interface
  char* do_command(const char* cmd, CORBA::Boolean& success, CORBA::Boolean& target_changed,
		   AttN_Interactive_outarg next_target);
  AttN::NameSeq* child_names();
  AttN::NameSeq* my_name();
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates);
  CORBA::Boolean safe_cleanup();

  // Methods from CosNA::ProxyConsumer Interface
  CosNA::ProxyType         MyType();
  CosNA::ProxyID           MyID();
  CosNA::SupplierAdmin_ptr MyAdmin();
  CosN::EventTypeSeq*      obtain_subscription_types(CosNA::ObtainInfoMode mode);
  void validate_event_qos(const CosN::QoSProperties& r_qos,
			  CosN_NamedPropertyRangeSeq_outarg a_qos);

  // Methods from CosN::QoSAdmin Interface
  CosN::QoSProperties* get_qos();
  void set_qos(const CosN::QoSProperties& qos);
  void validate_qos(const CosN::QoSProperties& r_qos,
		    CosN_NamedPropertyRangeSeq_outarg a_qos);

  // Methods from CosNF::FilterAdmin Interface
  CosNF::FilterID     add_filter(CosNF::Filter_ptr filter);
  void                remove_filter(CosNF::FilterID fltrID);
  void                remove_all_filters();
  CosNF::Filter_ptr   get_filter(CosNF::FilterID fltrID);
  CosNF::FilterIDSeq* get_all_filters();

  // Methods from CosNC::NotifyPublish Interface
  void offer_change(const CosN::EventTypeSeq& added, 
		    const CosN::EventTypeSeq& deled);

  // (Local only -- not available via RPC)

  CORBA::Boolean is_connected() const { return _pxstate == RDI_Connected; }
  CORBA::Boolean is_active()    const { return _active; }
  CORBA::Boolean has_filters()  const { return _fa_helper.has_filters(); }
#ifndef NO_OBJ_GC
  CORBA::Boolean obj_gc(RDI_TimeT curtime, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy);
#endif

  // helper routine called when pxstate changes to Disconnected or Exception
  void _revoke_offers(RDI_LocksHeld& held);

  // helper routines called by _channel->_type_map
  void _enable_updates();
  void _disable_updates();

  // RDINotifySubscribe methods:
  void filter_destroy_i(Filter_i* filter);
  void propagate_schange(RDI_LocksHeld&             held,
			 const CosN::EventTypeSeq&  added,
			 const CosN::EventTypeSeq&  deled,
			 Filter_i*                  filter);

  // these methods are called when the public methods cannot be used
  // (the public methods obtain OPLOCK)
  CosNA::ProxyID _proxy_id()  { return _pserial; } // used when proxy lock already held [MyID obtains lock]

  const AttN::NameSeq& L_my_name() { return _my_name; }

  // Local-only functions
  CORBA::Boolean do_set_command(RDIstrstream& str, RDIParseCmd& p);
  void out_config(RDIstrstream& str);
  void out_commands(RDIstrstream& str);
  void out_info_filters(RDIstrstream& str);

protected:
  RDIOplockEntry*             _oplockptr;
#ifndef NO_OBJ_GC
  RDI_TimeT	              _last_use;
#endif
  AttN::NameSeq               _my_name;
  FAdminHelper                _fa_helper;
  EventChannel_i*             _channel;
  SupplierAdmin_i*            _myadmin;
  RDI_ObjectKind              _otype;
  CosNA::ProxyType            _prxtype;
  CosNA::ProxyID              _pserial;
  CORBA::ULong                _nevents;
  RDI_ProxyState              _pxstate;
  CORBA::Boolean              _active;	// TRUE if connection  is active
  RDI_NotifQoS*               _qosprop;
  CORBA::Boolean              _sc_off;  // TRUE if subscription_change() is disabled
  CosNC::NotifySubscribe_var  _sc_subscriber;
  RDI_Hash<CosN::EventType, CORBA::ULong> _evtypes;

  // internal helper routines -- matching over CORBA::Any and RDI_StructuredEvent
  CORBA::Boolean _match_event_admin_level(const CORBA::Any& event);
  CORBA::Boolean _match_event_proxy_level(const CORBA::Any& event);
  CORBA::Boolean _match_event(const CORBA::Any& event);

  CORBA::Boolean _match_event_admin_level(const CosN::StructuredEvent* event, RDI_StructuredEvent* sevnt);
  CORBA::Boolean _match_event_proxy_level(const CosN::StructuredEvent* event, RDI_StructuredEvent* sevnt);
  CORBA::Boolean _match_event(const CosN::StructuredEvent* event, RDI_StructuredEvent* sevnt);

  // This is invoked by the administrative object during destruction
  void disconnect_client_and_dispose(RDI_LocksHeld&  held,
				     CORBA::Boolean  remove_proxy_from_admin);

  // Describe event rejection due to MaxQueueLength 
  void _report_reject_se(const char* cproxy_descr, CosNA::ProxyID consumer_id,
			 const CosN::StructuredEvent& evnt);
  void _report_reject_any(const char* cproxy_descr, CosNA::ProxyID consumer_id,
			  const CORBA::Any& evnt);

  // Every real class must implement this helper function which does the real work
  // (caller must have bumped scope lock):
  virtual void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
					      CORBA::Boolean            remove_proxy_from_admin,
					      WRAPPED_DISPOSEINFO_PTR&  dispose_info) = 0;

  // Every real class must implement this logging method
  virtual RDIstrstream& log_output(RDIstrstream& str) const = 0;

  // Invoked by RDI_ChangePool threads -- every real class must implement this
  CORBA::Boolean send_subscription_change(const CosN::EventTypeSeq& added,
					  const CosN::EventTypeSeq& deled);

  virtual ~RDIProxyConsumer();
};

// The following macro inserts declarations into a real proxy class
// which define each of the given methods to use the method inherited
// from the RDIProxyConsumer helper class.  (If the method has inline
// code we just put the code here, in which case the method does not
// appear above, even though it could/should.).  This is done because
// we only declare the IDL-generated skeleton class in the real proxy
// class, and must provide explicit implementations of all of the
// external methods within the proxy class -- cannot declare 
// any skeleton classes for helper class without causing compilation problems.
// If someone knows a better approach, please let us know!

#define UseRDIProxyConsumerMethods 							\
  char* do_command(const char* cmd, CORBA::Boolean& success,				\
		   CORBA::Boolean& target_changed, 					\
		   AttN_Interactive_outarg next_target  WRAPPED_DECLARG )		\
   { return RDIProxyConsumer::do_command(cmd, success, target_changed, next_target); }	\
  AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID )					\
      { return RDIProxyConsumer::child_names(); }					\
  AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID )					\
      { return RDIProxyConsumer::my_name(); }						\
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates  WRAPPED_DECLARG ) 	\
      { return RDIProxyConsumer::children(only_cleanup_candidates); }			\
  CORBA::Boolean safe_cleanup( WRAPPED_DECLARG_VOID ) 					\
      { return RDIProxyConsumer::safe_cleanup(); }					\
  CosNA::ProxyType 									\
  MyType( WRAPPED_DECLARG_VOID ) 							\
        { return RDIProxyConsumer::MyType(); } 						\
  CosNA::ProxyID 									\
  MyID( WRAPPED_DECLARG_VOID ) 								\
        { return RDIProxyConsumer::MyID(); } 						\
  CosNA::SupplierAdmin_ptr 								\
  MyAdmin( WRAPPED_DECLARG_VOID ) 							\
        { return RDIProxyConsumer::MyAdmin(); } 					\
  CosN::EventTypeSeq* 									\
  obtain_subscription_types(CosNA::ObtainInfoMode mode WRAPPED_DECLARG ) 		\
        { return RDIProxyConsumer::obtain_subscription_types(mode); } 			\
  void 											\
  validate_event_qos(const CosN::QoSProperties& r_qos, 					\
                     CosN_NamedPropertyRangeSeq_outarg a_qos			 	\
                     WRAPPED_DECLARG) 							\
        { RDIProxyConsumer::validate_event_qos(r_qos, a_qos); } 			\
  CosN::QoSProperties* 									\
  get_qos( WRAPPED_DECLARG_VOID ) 							\
        { return RDIProxyConsumer::get_qos(); } 					\
  void 											\
  set_qos(const CosN::QoSProperties& qos WRAPPED_DECLARG ) 				\
        { RDIProxyConsumer::set_qos(qos); } 						\
  void 											\
  validate_qos(const CosN::QoSProperties& r_qos, 					\
               CosN_NamedPropertyRangeSeq_outarg a_qos 					\
               WRAPPED_DECLARG ) 							\
        { RDIProxyConsumer::validate_qos(r_qos, a_qos); } 				\
  CosNF::FilterID 									\
  add_filter(CosNF::Filter_ptr filter WRAPPED_DECLARG ) 				\
        { return RDIProxyConsumer::add_filter(filter); } 				\
  void 											\
  remove_filter(CosNF::FilterID fltrID  WRAPPED_DECLARG ) 				\
        { RDIProxyConsumer::remove_filter(fltrID); } 					\
  void 											\
  remove_all_filters( WRAPPED_DECLARG_VOID ) 						\
        { RDIProxyConsumer::remove_all_filters(); } 					\
  CosNF::Filter_ptr 									\
  get_filter(CosNF::FilterID fltrID WRAPPED_DECLARG ) 					\
        { return RDIProxyConsumer::get_filter(fltrID); } 				\
  CosNF::FilterIDSeq* 									\
  get_all_filters( WRAPPED_DECLARG_VOID ) 						\
        { return RDIProxyConsumer::get_all_filters(); } 				\
  void 											\
  offer_change(const CosN::EventTypeSeq& added, 					\
               const CosN::EventTypeSeq& deled WRAPPED_DECLARG ) 			\
        { RDIProxyConsumer::offer_change(added, deled); } 				\


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//               Push-based Consumer Proxy implementations               //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

/** ProxyPushConsumer
  * This proxy object is connected to a push supplier and receives
  * CORBA::Any events
  */
class ProxyPushConsumer_i :
  	public virtual RDIProxyConsumer, 
	WRAPPED_SKELETON_SUPER(AttNotification, ProxyPushConsumer) 
{
  friend class EventChannel_i;
  friend class SupplierAdmin_i;
public:
  ProxyPushConsumer_i(SupplierAdmin_i* admin, 
		      EventChannel_i*  chann, const CosNA::ProxyID& prxID);

  // See comments for this macro above
  UseRDIProxyConsumerMethods

  // Methods from CosNA::ProxyPushConsumer Interface
  void connect_any_push_supplier(CosEvC::PushSupplier_ptr supplier
				 WRAPPED_DECLARG);

  // Methods from CosEvC::PushConsumer Interface
  void push(const CORBA::Any& data WRAPPED_DECLARG );
  void disconnect_push_consumer( WRAPPED_DECLARG_VOID );

  virtual RDIstrstream& log_output(RDIstrstream& str) const;

private:
  CosEvC::PushSupplier_var  _supplier;
  CosNC::PushSupplier_var          _nc_supplier; // nil if not a CosNC client

  // The real implementation (caller must have bumped scope lock acquiring oplock):
  virtual void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
					      CORBA::Boolean            remove_proxy_from_admin,
					      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  virtual ~ProxyPushConsumer_i();
};

/** StructuredProxyPushConsumer
  * This proxy object is connected to a push supplier and receives
  * CosN::StructuredEvent events
  */
class StructuredProxyPushConsumer_i : 
  	public virtual RDIProxyConsumer,
   	WRAPPED_SKELETON_SUPER(AttNotification, StructuredProxyPushConsumer) 
{
  friend class EventChannel_i;
  friend class SupplierAdmin_i;
public:
  StructuredProxyPushConsumer_i(SupplierAdmin_i*     admin,
				EventChannel_i*      chann, 
				const CosNA::ProxyID& prxID);

  // See comments for this macro above
  UseRDIProxyConsumerMethods

  // Methods from CosNA::StructuredProxyPushConsumer Interface
  void connect_structured_push_supplier(
		CosNC::StructuredPushSupplier_ptr supplier WRAPPED_DECLARG );

  // Methods from CosNC::StructuredPushConsumer Interface
  void push_structured_event(const CosN::StructuredEvent& event WRAPPED_DECLARG);
  void disconnect_structured_push_consumer( WRAPPED_DECLARG_VOID );

  // (Local only -- not available via RPC)

  virtual RDIstrstream& log_output(RDIstrstream& str) const;

private:
  CosNC::StructuredPushSupplier_var        _supplier;

  // The real implementation (caller must have bumped scope lock acquiring oplock):
  virtual void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
					      CORBA::Boolean            remove_proxy_from_admin,
					      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  virtual ~StructuredProxyPushConsumer_i();
};

/** SequenceProxyPushConsumer
  * This proxy object is connected to a push supplier and receives
  * CosN::EventBatch events
  */

class SequenceProxyPushConsumer_i : 
  	public virtual RDIProxyConsumer, 
  	WRAPPED_SKELETON_SUPER(AttNotification, SequenceProxyPushConsumer) 
{
  friend class EventChannel_i;
  friend class SupplierAdmin_i;
 public:
  SequenceProxyPushConsumer_i(SupplierAdmin_i* admin, 
			      EventChannel_i*  chann, 
			      const CosNA::ProxyID& prxID);

  // See comments for this macro above
  UseRDIProxyConsumerMethods

  // Methods from CosNA::SequenceProxyPushConsumer Interface
  void connect_sequence_push_supplier(CosNC::SequencePushSupplier_ptr supplier
				      WRAPPED_DECLARG );

  // Methods from CosNC::SequencePushConsumer Interface
  void push_structured_events(const CosN::EventBatch& events WRAPPED_DECLARG );
  void disconnect_sequence_push_consumer( WRAPPED_DECLARG_VOID );

  // (Local only -- not available via RPC)

  virtual RDIstrstream& log_output(RDIstrstream& str) const;

private:
  CosNC::SequencePushSupplier_var _supplier;

  // The real implementation (caller must have bumped scope lock acquiring oplock):
  virtual void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
					      CORBA::Boolean            remove_proxy_from_admin,
					      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  virtual ~SequenceProxyPushConsumer_i();
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//               Pull-based Consumer Proxy implementations               //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

/** ProxyPullConsumer
  * This proxy object is connected to a pull supplier and retrieves
  * CORBA::Any events
  */

class ProxyPullConsumer_i : 
  	public virtual RDIProxyConsumer, 
  	public virtual RDIProxyPullConsumer, 
  	WRAPPED_SKELETON_SUPER(AttNotification, ProxyPullConsumer) 
{
  friend class EventChannel_i;
  friend class SupplierAdmin_i;
 public:
  ProxyPullConsumer_i(SupplierAdmin_i* admin, 
		      EventChannel_i*  chann, const CosNA::ProxyID& prxID);

  // See comments for this macro above
  UseRDIProxyConsumerMethods

  // Methods from CosNA::ProxyPullConsumer Interface
  void connect_any_pull_supplier(
		CosEvC::PullSupplier_ptr supplier WRAPPED_DECLARG );
  void suspend_connection( WRAPPED_DECLARG_VOID );
  void resume_connection( WRAPPED_DECLARG_VOID ); 

  // Methods from CosEvC::PullConsumer Interface
  void disconnect_pull_consumer( WRAPPED_DECLARG_VOID );

  // (Local only -- not available via RPC)

  virtual CORBA::Boolean is_available(unsigned long* wait_s, unsigned long* wait_t);
  virtual void pull_event(CORBA::Boolean& invalid);

  virtual RDIstrstream& log_output(RDIstrstream& str) const;

private:
  TW_Thread*                 _worker;
  CORBA::Boolean             _thrdone;    // true when worker thread exits
  CosEvC::PullSupplier_var   _supplier;
  CosNC::PullSupplier_var    _nc_supplier;

  // absolute time of next timeout
  unsigned long                   _timeout_s;
  unsigned long                   _timeout_n;

  void _pull_event();

  // The real implementation (caller must have bumped scope lock acquiring oplock):
  virtual void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
					      CORBA::Boolean            remove_proxy_from_admin,
					      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  virtual ~ProxyPullConsumer_i();
};

/** StructuredProxyPullConsumer
  * This proxy object is connected to a push supplier and retrieves
  * CosN::StructuredEvent events
  */

class StructuredProxyPullConsumer_i : 
  	public virtual RDIProxyConsumer, 
  	public virtual RDIProxyPullConsumer, 
  	WRAPPED_SKELETON_SUPER(AttNotification, StructuredProxyPullConsumer) 
{
  friend class EventChannel_i;
  friend class SupplierAdmin_i;
 public:
  StructuredProxyPullConsumer_i(SupplierAdmin_i* admin,
				EventChannel_i*  chann, 
				const CosNA::ProxyID& prxID);

  // See comments for this macro above
  UseRDIProxyConsumerMethods

  // Methods from CosNA::StructuredProxyPullConsumer Interface
  void connect_structured_pull_supplier(
		CosNC::StructuredPullSupplier_ptr supplier WRAPPED_DECLARG );
  void suspend_connection( WRAPPED_DECLARG_VOID );
  void resume_connection( WRAPPED_DECLARG_VOID );

  // Methods from CosNC::StructuredPullConsumer Interface
  void disconnect_structured_pull_consumer( WRAPPED_DECLARG_VOID );

  // (Local only -- not available via RPC)

  virtual CORBA::Boolean is_available(unsigned long* wait_s, unsigned long* wait_t);
  virtual void pull_event(CORBA::Boolean& invalid);

  virtual RDIstrstream& log_output(RDIstrstream& str) const;

private:
  TW_Thread*                         _worker;
  CORBA::Boolean                     _thrdone;  // true when worker thread exits
  CosNC::StructuredPullSupplier_var  _supplier;

  // absolute time of next timeout
  unsigned long                   _timeout_s;
  unsigned long                   _timeout_n;

  void _pull_event();

  // The real implementation (caller must have bumped scope lock acquiring oplock):
  virtual void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
					      CORBA::Boolean            remove_proxy_from_admin,
					      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  virtual ~StructuredProxyPullConsumer_i();
};

/** SequenceProxyPullConsumer
  * This proxy object is connected to a push supplier and retrieves
  * CosN::EventBatch events
  */

class SequenceProxyPullConsumer_i : 
  	public virtual RDIProxyConsumer, 
  	public virtual RDIProxyPullConsumer, 
  	WRAPPED_SKELETON_SUPER(AttNotification, SequenceProxyPullConsumer) 
{
  friend class EventChannel_i;
  friend class SupplierAdmin_i;
 public:
  SequenceProxyPullConsumer_i(SupplierAdmin_i* admin, 
			      EventChannel_i*  chann, 
			      const CosNA::ProxyID& prxID);

  // See comments for this macro above
  UseRDIProxyConsumerMethods

  // Methods from CosNA::SequenceProxyPullConsumer Interface
  void connect_sequence_pull_supplier(
		CosNC::SequencePullSupplier_ptr supplier WRAPPED_DECLARG );
  void suspend_connection( WRAPPED_DECLARG_VOID );
  void resume_connection( WRAPPED_DECLARG_VOID );

  // Methods from CosNC::SequencePullConsumer Interface
  void disconnect_sequence_pull_consumer( WRAPPED_DECLARG_VOID );

  // (Local only -- not available via RPC)

  virtual CORBA::Boolean is_available(unsigned long* wait_s, unsigned long* wait_t);
  virtual void pull_event(CORBA::Boolean& invalid);

  virtual RDIstrstream& log_output(RDIstrstream& str) const;

private:
  TW_Thread*                       _worker;
  CORBA::Boolean                   _thrdone; // true when worker thread exits
  CosNC::SequencePullSupplier_var  _supplier;

  // absolute time of next timeout
  unsigned long                   _timeout_s;
  unsigned long                   _timeout_n;

  void _pull_event();

  // The real implementation (caller must have bumped scope lock acquiring oplock):
  virtual void _disconnect_client_and_dispose(RDI_LocksHeld&            held,
					      CORBA::Boolean            remove_proxy_from_admin,
					      WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  virtual ~SequenceProxyPullConsumer_i();
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//            ConsumerAdmin and SupplierAdmin implementations            //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

/** ConsumerAdmin_i objects are responsible for creating and managing
  * proxy supplier objects  within instances of  Notification Service
  * Event Channels.  ConsumerAdmin_i objects are also responsible for
  * managing a common set of QoS requirements and filter objects that
  * apply to all of their proxy objects.
  */

class ConsumerAdmin_i :	 
	public RDINotifySubscribe, 
	WRAPPED_SKELETON_SUPER(AttNotification, ConsumerAdmin) 
{
  friend class EventChannel_i;
public:
  ConsumerAdmin_i(EventChannel_i* channel, CosNA::InterFilterGroupOperator op, 
		  const CosNA::AdminID& serial);

  // Methods from AttNotification::Interactive Interface
  char* do_command(const char* cmd, CORBA::Boolean& success, CORBA::Boolean& target_changed,
		   AttN_Interactive_outarg next_target  WRAPPED_DECLARG);
  AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID );
  AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID );
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates  WRAPPED_DECLARG );
  CORBA::Boolean safe_cleanup( WRAPPED_DECLARG_VOID );

  // Methods from CosEvCA::ConsumerAdmin Interface
  CosEvCA::ProxyPushSupplier_ptr obtain_push_supplier( WRAPPED_DECLARG_VOID );
  CosEvCA::ProxyPullSupplier_ptr obtain_pull_supplier( WRAPPED_DECLARG_VOID );

  // Methods from CosNC::ConsumerAdmin Interface
  CosNA::AdminID                  MyID( WRAPPED_DECLARG_VOID );
  CosNA::EventChannel_ptr         MyChannel( WRAPPED_DECLARG_VOID );	
  CosNA::InterFilterGroupOperator MyOperator( WRAPPED_DECLARG_VOID );
  CosNF::MappingFilter_ptr priority_filter( WRAPPED_DECLARG_VOID );
  void  priority_filter(CosNF::MappingFilter_ptr fltr WRAPPED_DECLARG );
  CosNF::MappingFilter_ptr lifetime_filter( WRAPPED_DECLARG_VOID );
  void  lifetime_filter(CosNF::MappingFilter_ptr fltr WRAPPED_DECLARG );
  CosNA::ProxyIDSeq*       pull_suppliers( WRAPPED_DECLARG_VOID );
  CosNA::ProxyIDSeq*       push_suppliers( WRAPPED_DECLARG_VOID );
  CosNA::ProxySupplier_ptr get_proxy_supplier(CosNA::ProxyID proxy_id 
					     WRAPPED_DECLARG );
  CosNA::ProxySupplier_ptr obtain_notification_pull_supplier(
				CosNA::ClientType ctype, 
			        CosNA::ProxyID&   proxy_id WRAPPED_DECLARG );
  CosNA::ProxySupplier_ptr obtain_notification_push_supplier(
				CosNA::ClientType ctype, 
			        CosNA::ProxyID&   proxy_id WRAPPED_DECLARG );
  void destroy( WRAPPED_DECLARG_VOID );

  // Methods from CosN::QoSAdmin Interface
  CosN::QoSProperties* get_qos( WRAPPED_DECLARG_VOID );
  void set_qos(const CosN::QoSProperties& qos WRAPPED_DECLARG );
  void validate_qos(const CosN::QoSProperties& r_qos,
		    CosN_NamedPropertyRangeSeq_outarg a_qos
		    WRAPPED_DECLARG );

  // Methods from CosNF::FilterAdmin Interface
  CosNF::FilterID     add_filter(CosNF::Filter_ptr fltr WRAPPED_DECLARG );
  void               remove_filter(CosNF::FilterID fltrID WRAPPED_DECLARG );
  CosNF::Filter_ptr   get_filter(CosNF::FilterID fltrID WRAPPED_DECLARG );
  CosNF::FilterIDSeq* get_all_filters( WRAPPED_DECLARG_VOID );
  void               remove_all_filters( WRAPPED_DECLARG_VOID );

  // Methods from CosNC::NotifySubscribe Interface
  void subscription_change(const CosN::EventTypeSeq& added,
                           const CosN::EventTypeSeq& deled WRAPPED_DECLARG );

  // (Local only -- not available via RPC)

  CORBA::Boolean has_filters() const { return (_rqstypes.length() ? 1 : _fa_helper.has_filters()); }
  //  CORBA::Boolean has_filters() const { return _fa_helper.has_filters(); }

  // RDINotifySubscribe methods:
  void filter_destroy_i(Filter_i* filter);
  void propagate_schange(RDI_LocksHeld&             held,
			 const CosN::EventTypeSeq&  added,
			 const CosN::EventTypeSeq&  deled,
			 Filter_i*                  filter);

  // Proxy de-registration members
  void remove_proxy(RDI_LocksHeld& held, ProxyPushSupplier_i* prx);
  void remove_proxy(RDI_LocksHeld& held, ProxyPullSupplier_i* prx);
  void remove_proxy(RDI_LocksHeld& held, EventProxyPushSupplier_i* prx);
  void remove_proxy(RDI_LocksHeld& held, EventProxyPullSupplier_i* prx);
  void remove_proxy(RDI_LocksHeld& held, StructuredProxyPushSupplier_i* prx);
  void remove_proxy(RDI_LocksHeld& held, StructuredProxyPullSupplier_i* prx);
  void remove_proxy(RDI_LocksHeld& held, SequenceProxyPushSupplier_i* prx);
  void remove_proxy(RDI_LocksHeld& held, SequenceProxyPullSupplier_i* prx);

  CORBA::ULong         NumProxies() const	{ return _num_proxies; }
  RDI_NotifQoS*        qos_properties()		{ return _qosprop; }

  CORBA::Boolean       gc_able(CORBA::ULong DeadAdminInterval);

  // these methods are called when the public methods cannot be used
  // (the public methods obtain OPLOCK)
  CosNA::AdminID                  _admin_id()       { return _serial; }
  CosNA::InterFilterGroupOperator _admin_operator() { return _and_or_oper; }

  // Local-only functions

  RDIstrstream& log_output(RDIstrstream& str);
  CORBA::Boolean do_go_command(RDIstrstream& str, RDIParseCmd& p,
			       CORBA::Boolean& target_changed,
			       AttN_Interactive_outarg next_target);
  CORBA::Boolean do_set_command(RDIstrstream& str, RDIParseCmd& p);
  void out_config(RDIstrstream& str);
  void out_commands(RDIstrstream& str);
  void out_info_filters(RDIstrstream& str, CORBA::Boolean admin, CORBA::Boolean proxies);
  void cleanup(RDIstrstream& str, CORBA::Boolean admin, CORBA::Boolean proxies);
#ifndef NO_OBJ_GC
  CORBA::Boolean obj_gc(RDI_TimeT curtime, CORBA::ULong deadAdmin, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy);
  CORBA::Boolean gc_able(RDI_TimeT curtime, CORBA::ULong deadAmin);
#endif

  const AttN::NameSeq& L_my_name() { return _my_name; }

  // N.B. only the macros in RDIOplocksMacros.h should call this function
  RDIOplockEntry** macro_get_oplockptr_ptr() { return &_oplockptr; }

private:
  RDIOplockEntry*                 _oplockptr;
#ifndef NO_OBJ_GC
  RDI_TimeT	                  _last_use;
#endif
  AttN::ConsumerAdmin_var         _my_oref;
  AttN::NameSeq                   _my_name;
  // _disposed is true once _disconnect_clients_and_dispose has been called
  CORBA::Boolean                  _disposed;
  FAdminHelper     	          _fa_helper;
  EventChannel_i*                 _channel;
  RDI_NotifQoS*                   _qosprop;
  CosNA::AdminID                  _serial;
  CosNA::InterFilterGroupOperator _and_or_oper;
  CosN::EventTypeSeq              _rqstypes;
  CosNF::MappingFilter_var        _prio_filter;	// Priority mapping filter
  CosNF::MappingFilter_var        _life_filter;	// Lifetime mapping filter
  CosNA::ProxyID                  _prx_serial;	// Factory for proxy IDs
  CORBA::ULong                    _num_proxies;	// Number of active proxies

  // Since the Event Service proxy objects do not have a unique ID
  // associated with them, we keep these proxies in a regular list

  RDI_List<EventProxyPushSupplier_i *> _cosevent_push;
  RDI_List<EventProxyPullSupplier_i *> _cosevent_pull;

  // All Notification Service proxy objects have unique IDs. Thus, 
  // we maintain them using hash tables for fast lookups 

  RDI_Hash<CosNA::ProxyID, ProxyPushSupplier_i *>           _prx_any_push;
  RDI_Hash<CosNA::ProxyID, ProxyPullSupplier_i *>           _prx_any_pull;
  RDI_Hash<CosNA::ProxyID, StructuredProxyPushSupplier_i *> _prx_struc_push;
  RDI_Hash<CosNA::ProxyID, StructuredProxyPullSupplier_i *> _prx_struc_pull;
  RDI_Hash<CosNA::ProxyID, SequenceProxyPushSupplier_i *>   _prx_batch_push;
  RDI_Hash<CosNA::ProxyID, SequenceProxyPullSupplier_i *>   _prx_batch_pull;

  // Dispatch an event to all consumers using the CORBA Event Service
  // interface -- no filtering is performed here

  void dispatch_event(RDI_StructuredEvent*  event);

  // Dispatch an event to all consumers using the CORBA Notification
  // Service interface -- filtering is performed here

  void dispatch_event(RDI_StructuredEvent*  event, 
		      RDI_FilterState_t     fltstat,
		      RDI_TypeMap*          typemap);

  void disconnect_clients_and_dispose(CORBA::Boolean fast_destroy);

  // The real implementation (caller must have bumped scope lock acquiring oplock):
  void _disconnect_clients_and_dispose(RDI_LocksHeld&            held,
				       CORBA::Boolean            fast_destroy,
				       CORBA::Boolean            update_channel,
				       WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  void _children(AttN::IactSeq& ren, CORBA::Boolean only_cleanup_candidates);
  void _qos_changed(RDI_LocksHeld& held);
  void _removed_push_proxy(RDIProxyPushSupplier* proxy);
  void _removed_pull_proxy();

  virtual ~ConsumerAdmin_i();
};

/** SupplierAdmin_i objects are responsible for creating and managing
  * proxy consumer objects  within instances of  Notification Service
  * Event Channels.  ConsumerAdmin_i objects are also responsible for
  * managing a common set of QoS requirements that apply to all their
  * proxy objects.
  */

class SupplierAdmin_i : 
	public RDINotifySubscribe, 
	WRAPPED_SKELETON_SUPER(AttNotification, SupplierAdmin) 
{
  friend class EventChannel_i;
public:
  SupplierAdmin_i(EventChannel_i* channel, CosNA::InterFilterGroupOperator op, 
		  const CosNA::AdminID& serial);

  // Methods from AttNotification::Interactive Interface
  char* do_command(const char* cmd, CORBA::Boolean& success, CORBA::Boolean& target_changed,
		   AttN_Interactive_outarg next_target  WRAPPED_DECLARG);
  AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID );
  AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID );
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates  WRAPPED_DECLARG );
  CORBA::Boolean safe_cleanup( WRAPPED_DECLARG_VOID );

  // Methods from CosEvCA::SupplierAdmin Interface
  CosEvCA::ProxyPushConsumer_ptr obtain_push_consumer( WRAPPED_DECLARG_VOID );
  CosEvCA::ProxyPullConsumer_ptr obtain_pull_consumer( WRAPPED_DECLARG_VOID );

  // Methods from CosNA::SupplierAdmin Interface
  CosNA::AdminID                  MyID( WRAPPED_DECLARG_VOID ); 
  CosNA::EventChannel_ptr         MyChannel( WRAPPED_DECLARG_VOID );
  CosNA::InterFilterGroupOperator MyOperator( WRAPPED_DECLARG_VOID );
  CosNA::ProxyIDSeq*       pull_consumers( WRAPPED_DECLARG_VOID );
  CosNA::ProxyIDSeq*       push_consumers( WRAPPED_DECLARG_VOID );
  CosNA::ProxyConsumer_ptr get_proxy_consumer(CosNA::ProxyID proxy_id 
					     WRAPPED_DECLARG );
  CosNA::ProxyConsumer_ptr obtain_notification_pull_consumer(
				CosNA::ClientType ctype, 
				CosNA::ProxyID&   proxy_id WRAPPED_DECLARG );
  CosNA::ProxyConsumer_ptr obtain_notification_push_consumer(
				CosNA::ClientType ctype, 
			        CosNA::ProxyID&   proxy_id WRAPPED_DECLARG );
  void destroy( WRAPPED_DECLARG_VOID );

  // Methods from CosN::QoSAdmin Interface
  CosN::QoSProperties* get_qos( WRAPPED_DECLARG_VOID );
  void set_qos(const CosN::QoSProperties& qos WRAPPED_DECLARG );
  void validate_qos(const CosN::QoSProperties& r_qos,
		    CosN_NamedPropertyRangeSeq_outarg a_qos
		    WRAPPED_DECLARG );

  // Methods from CosNF::FilterAdmin Interface
  CosNF::FilterID     add_filter(CosNF::Filter_ptr filter WRAPPED_DECLARG );
  void                remove_filter(CosNF::FilterID fltrID WRAPPED_DECLARG );
  CosNF::Filter_ptr   get_filter(CosNF::FilterID fltrID WRAPPED_DECLARG );
  CosNF::FilterIDSeq* get_all_filters( WRAPPED_DECLARG_VOID );
  void                remove_all_filters( WRAPPED_DECLARG_VOID );

  // Methods from CosNC::NotifyPublish Interface
  // Called externally -- not clear that suppliers should be doing this!
  void offer_change(const CosN::EventTypeSeq& added, 
		    const CosN::EventTypeSeq& removed WRAPPED_DECLARG );

  // (Local only -- not available via RPC)

  CORBA::Boolean has_filters() const { return _fa_helper.has_filters(); }

  // these methods are called when the public methods cannot be used
  // (the public methods obtain OPLOCK)
  CosNA::AdminID                  _admin_id()       { return _serial; }
  CosNA::InterFilterGroupOperator _admin_operator() { return _and_or_oper; }

  // Called by the admin's proxies
  void propagate_ochange(RDI_LocksHeld&             held,
			 const CosN::EventTypeSeq& added, 
			 const CosN::EventTypeSeq& removed);

  // RDINotifySubscribe methods:
  void filter_destroy_i(Filter_i* filter);
  void propagate_schange(RDI_LocksHeld&             held,
			 const CosN::EventTypeSeq&  added,
			 const CosN::EventTypeSeq&  deled,
			 Filter_i*                  filter);

  // Proxy de-registration members
  void remove_proxy(RDI_LocksHeld& held, ProxyPushConsumer_i* prx);
  void remove_proxy(RDI_LocksHeld& held, ProxyPullConsumer_i* prx);
  void remove_proxy(RDI_LocksHeld& held, EventProxyPushConsumer_i* prx);
  void remove_proxy(RDI_LocksHeld& held, EventProxyPullConsumer_i* prx);
  void remove_proxy(RDI_LocksHeld& held, StructuredProxyPushConsumer_i* prx);
  void remove_proxy(RDI_LocksHeld& held, StructuredProxyPullConsumer_i* prx);
  void remove_proxy(RDI_LocksHeld& held, SequenceProxyPushConsumer_i* prx);
  void remove_proxy(RDI_LocksHeld& held, SequenceProxyPullConsumer_i* prx);

  CORBA::ULong         NumProxies() const	{ return _num_proxies; }
  RDI_NotifQoS*        qos_properties()		{ return _qosprop; }

  CORBA::Boolean       gc_able(CORBA::ULong DeadAdminInterval);

  // The following methods are invoked by connected proxies to check if
  // a given supplied event can be inserted into the channel

  CORBA::Boolean match_event(const CORBA::Any& event);
  CORBA::Boolean match_event(const CosN::StructuredEvent* event, RDI_StructuredEvent* sevnt);

  // Local-only functions
  RDIstrstream& log_output(RDIstrstream& str);
  CORBA::Boolean do_go_command(RDIstrstream& str, RDIParseCmd& p,
			       CORBA::Boolean& target_changed,
			       AttN_Interactive_outarg next_target);
  CORBA::Boolean do_set_command(RDIstrstream& str, RDIParseCmd& p);
  void out_config(RDIstrstream& str);
  void out_commands(RDIstrstream& str);
  void out_info_filters(RDIstrstream& str, CORBA::Boolean admin, CORBA::Boolean proxies);
  void cleanup(RDIstrstream& str, CORBA::Boolean admin, CORBA::Boolean proxies);
#ifndef NO_OBJ_GC
  CORBA::Boolean obj_gc(RDI_TimeT curtime, CORBA::ULong deadAdmin, CORBA::ULong deadConProxy, CORBA::ULong deadOtherProxy);
  CORBA::Boolean gc_able(RDI_TimeT curtime, CORBA::ULong deadAmin);
#endif

  const AttN::NameSeq& L_my_name() { return _my_name; }

  // N.B. only the macros in RDIOplocksMacros.h should call this function
  RDIOplockEntry** macro_get_oplockptr_ptr() { return &_oplockptr; }

private:
  RDIOplockEntry*                 _oplockptr;
#ifndef NO_OBJ_GC
  RDI_TimeT	                  _last_use;
#endif
  AttN::SupplierAdmin_var         _my_oref;
  AttN::NameSeq                   _my_name;
  // _disposed is true once _disconnect_clients_and_dispose has been called
  CORBA::Boolean                  _disposed;
  FAdminHelper                    _fa_helper;
  EventChannel_i*                 _channel;
  RDI_NotifQoS*                   _qosprop;
  CosNA::AdminID                  _serial;
  CosNA::InterFilterGroupOperator _and_or_oper;
  CosNA::ProxyID                  _prx_serial;        // Factory for proxy IDs
  CORBA::ULong                    _num_proxies;

  // To support offer_change() we need a hash table of event types

  RDI_Hash<CosN::EventType, CORBA::ULong> _evtypes;

  // Since the Event Service proxy objects do not have a unique ID
  // associated with them, we keep these proxies in a regular list

  RDI_List<EventProxyPushConsumer_i *> _cosevent_push;
  RDI_List<EventProxyPullConsumer_i *> _cosevent_pull;

  // All Notification Service proxy objects have unique IDs. Thus,
  // we maintain them using hash tables for fast lookups

  RDI_Hash<CosNA::ProxyID, ProxyPushConsumer_i *>           _prx_any_push;
  RDI_Hash<CosNA::ProxyID, ProxyPullConsumer_i *>           _prx_any_pull;
  RDI_Hash<CosNA::ProxyID, StructuredProxyPushConsumer_i *> _prx_struc_push;
  RDI_Hash<CosNA::ProxyID, StructuredProxyPullConsumer_i *> _prx_struc_pull;
  RDI_Hash<CosNA::ProxyID, SequenceProxyPushConsumer_i *>   _prx_batch_push;
  RDI_Hash<CosNA::ProxyID, SequenceProxyPullConsumer_i *>   _prx_batch_pull;

  void disconnect_clients_and_dispose(CORBA::Boolean fast_destroy);

  // The real implementation (caller must have bumped scope lock acquiring oplock):
  void _disconnect_clients_and_dispose(RDI_LocksHeld&            held,
				       CORBA::Boolean            fast_destroy,
				       CORBA::Boolean            update_channel,
				       WRAPPED_DISPOSEINFO_PTR&  dispose_info);

  // does the real work  (caller must have bumped scope lock acquiring oplock):
  void _propagate_ochange(RDI_LocksHeld&             held,
			  const CosN::EventTypeSeq&  added, 
			  const CosN::EventTypeSeq&  removed);

  void _children(AttN::IactSeq& ren, CORBA::Boolean only_cleanup_candidates);
  void _removed_push_proxy();
  void _removed_pull_proxy(RDIProxyPullConsumer* prx);

  virtual ~SupplierAdmin_i();
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//         EventChannel and EventChannelFactory implementations          //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

/** Each event channel has a default filter factory that is used 
  * for creating filter objects. 
  */

class EventChannel_i : 
	WRAPPED_SKELETON_SUPER(AttNotification, EventChannel) 
{
  friend class EventChannelFactory_i;
public:
  EventChannel_i(EventChannelFactory_i*        cfactory,
		 FilterFactory_i*              ffactory,
		 const CosN::QoSProperties&    init_qos,
	         const CosN::AdminProperties&  init_adm,
		 RDI_ServerQoS*                s_qos,
		 const CosNA::ChannelID&       myserial);

  // Methods from AttNotification::Interactive Interface
  char* do_command(const char* cmd, CORBA::Boolean& success, CORBA::Boolean& target_changed,
		   AttN_Interactive_outarg next_target  WRAPPED_DECLARG);
  AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID );
  AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID );
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates  WRAPPED_DECLARG );
  CORBA::Boolean safe_cleanup( WRAPPED_DECLARG_VOID );

  // Methods from CosEvCA::EventChannel Interface
  CosEvCA::ConsumerAdmin_ptr for_consumers( WRAPPED_DECLARG_VOID );
  CosEvCA::SupplierAdmin_ptr for_suppliers( WRAPPED_DECLARG_VOID );
  void destroy( WRAPPED_DECLARG_VOID );

  // Methods from CosNA::EventChannel Interface
  CosNA::EventChannelFactory_ptr MyFactory( WRAPPED_DECLARG_VOID );
  CosNA::ConsumerAdmin_ptr default_consumer_admin( WRAPPED_DECLARG_VOID );
  CosNA::SupplierAdmin_ptr default_supplier_admin( WRAPPED_DECLARG_VOID );
  CosNF::FilterFactory_ptr default_filter_factory( WRAPPED_DECLARG_VOID );
  CosNA::ConsumerAdmin_ptr new_for_consumers(
					CosNA::InterFilterGroupOperator op,
					CosNA::AdminID& id WRAPPED_DECLARG );
  CosNA::SupplierAdmin_ptr new_for_suppliers(
					CosNA::InterFilterGroupOperator op,
                                        CosNA::AdminID& id WRAPPED_DECLARG );
  CosNA::ConsumerAdmin_ptr get_consumeradmin(CosNA::AdminID id  WRAPPED_DECLARG );
  CosNA::SupplierAdmin_ptr get_supplieradmin(CosNA::AdminID id  WRAPPED_DECLARG );
  CosNA::AdminIDSeq *      get_all_consumeradmins( WRAPPED_DECLARG_VOID );
  CosNA::AdminIDSeq *      get_all_supplieradmins( WRAPPED_DECLARG_VOID );


  // Methods from CosN::AdminPropertiesAdmin Interface
  CosN::AdminProperties* get_admin( WRAPPED_DECLARG_VOID );
  void set_admin(const CosN::AdminProperties& admin WRAPPED_DECLARG );

  // Methods from CosN::QoSAdmin Interface
  CosN::QoSProperties* get_qos( WRAPPED_DECLARG_VOID );
  void set_qos(const CosN::QoSProperties& qos WRAPPED_DECLARG );
  void validate_qos(const CosN::QoSProperties& r_qos,
		    CosN_NamedPropertyRangeSeq_outarg a_qos
		    WRAPPED_DECLARG );

  // Additional methods from AttNotification::EventChannel
  CosN::EventTypeSeq* obtain_offered_types( WRAPPED_DECLARG_VOID );
  CosN::EventTypeSeq* obtain_subscription_types( WRAPPED_DECLARG_VOID );
  AttN::ChannelStats  obtain_stats( WRAPPED_DECLARG_VOID );

  // ----------------------------------------------------------------------------------
  // (Local only -- not available via RPC)

  void server_qos_changed();
  RDI_NotifQoS*   qos_properties() 	{ return _qosprop; }
  CosNA::ChannelID MyID()		{ return _serial;  }

  // An event is sent to the channel by some connected supplier

  int new_structured_event(RDI_StructuredEvent* event);
  int new_any_event(const CORBA::Any& event);
  int new_structured_event(const CosN::StructuredEvent& event);

  // Increment/decrement the number of consumers and suppliers
 
  CORBA::Boolean incr_consumers();
  void           decr_consumers();
  CORBA::Boolean incr_suppliers();
  void decr_suppliers();

  // Retrieve administrative properties of the channel. We do not
  // acquire a lock since we assume that they do not change often
  // and, in addition, a snapshop is sufficient

  CORBA::Long   max_consumers() const	{ return _admin_qos.maxConsumers;    }
  CORBA::Long   max_suppliers() const	{ return _admin_qos.maxSuppliers;    }
  CORBA::UShort push_threads() const 	{ return _server_qos->numPushThreads;  }
  CORBA::UShort pull_threads() const 	{ return _server_qos->numPullThreads;  }
  CORBA::UShort pull_period() const 	{ return _server_qos->pullEventPeriod; }
  CORBA::UShort ochange_threads() const	{ return _server_qos->numOChangeThreads;  }
  CORBA::UShort schange_threads() const	{ return _server_qos->numSChangeThreads;  }

  void pull_period_s_n(unsigned long &s, unsigned long &n) const {
    unsigned long msec = (unsigned long)_server_qos->pullEventPeriod;
    s = msec / 1000;
    n = (msec % 1000) * 1000000;
  }
  // Unregister Admin objects

  void unregister(RDI_LocksHeld& held, SupplierAdmin_i* suplAdmin);
  void unregister(RDI_LocksHeld& held, ConsumerAdmin_i* consAdmin);

  CORBA::Boolean update_mapping(RDI_LocksHeld&             held,
				const CosN::EventTypeSeq&  added,
				const CosN::EventTypeSeq&  deled,
				ConsumerAdmin_i*           admin,
				Filter_i*                  filter);

  CORBA::Boolean update_mapping(RDI_LocksHeld&             held,
				const CosN::EventTypeSeq&  added,
				const CosN::EventTypeSeq&  deled,
				RDIProxySupplier*          proxy,
				Filter_i*                  filter);

  struct ProxyDispatch_t {
	RDI_StructuredEvent* _event;
	ConsumerAdmin_i*     _admin;
	RDI_FilterState_t    _state;

	ProxyDispatch_t(RDI_StructuredEvent* e=0, 
		        ConsumerAdmin_i* a=0, RDI_FilterState_t s=NoFilters) : 
			_event(e), _admin(a), _state(s) {;}
	ProxyDispatch_t(const ProxyDispatch_t& p) : 
			_event(p._event), _admin(p._admin), _state(p._state) {;}
	ProxyDispatch_t& operator= (const ProxyDispatch_t& p) {
			_event=p._event; _admin=p._admin; 
			_state=p._state; return *this;         }
  };

  // Update the hash table that keeps information about the event  //
  // types supplied by suppliers.  When a new entry is created or  //
  // an existing entry is deleted, insert the delta into the       //
  // _ochange_pool, causing _offer_change msgs to be sent          //

  void propagate_ochange(RDI_LocksHeld&             held,
			 const CosN::EventTypeSeq&  added,
			 const CosN::EventTypeSeq&  deled);

  // There have been some changes in the event types referenced in //
  // consumers filters. Insert the delta into the _schange_pool,   //
  // causing _subscription_change msgs to be sent                  //
  // [This method is invoked by the RDI_TypeMap module]            //

  void propagate_schange(RDI_LocksHeld&             held,
			 const CosN::EventTypeSeq&  added,
			 const CosN::EventTypeSeq&  deled);

  // These versions retrieve the sequence of event types supplied by suppliers and
  // the sequence of event types referenced in all consumer filters 
  CosN::EventTypeSeq* pxy_obtain_offered_types(RDIProxySupplier* pxy, CosNA::ObtainInfoMode mode);
  CosN::EventTypeSeq* pxy_obtain_subscription_types(RDIProxyConsumer* pxy, CosNA::ObtainInfoMode mode);

  // for stats
  inline void incr_num_rdi_match();
  inline void incr_num_rvm_eval();
  inline void incr_num_announcements();
  inline void incr_num_notifications(unsigned int qsize);
  void        dump_stats(RDI_LocksHeld& held, CORBA::Boolean force);
  void        _rpt_stats(RDIstrstream& str); // called by dump_stats

  // From time to time we may need to debug the event queue
  void out_queue_debug_info(RDIstrstream& str, CORBA::Boolean showEvents=0) { _events->out_debug_info(str, showEvents); }

  RDIstrstream& log_output(RDIstrstream& str, CORBA::Boolean show_events = 0);
  RDIstrstream& log_output_config(RDIstrstream& str);

  // Local-only functions
  CORBA::Boolean do_go_command(RDIstrstream& str, RDIParseCmd& p,
			       CORBA::Boolean& target_changed,
			       AttN_Interactive_outarg next_target);
  CORBA::Boolean do_set_command(RDIstrstream& str, RDIParseCmd& p);
  void out_commands(RDIstrstream& str);
  void out_heading(RDIstrstream& str);
  void out_stats(RDIstrstream& str);
  void out_debug_info(RDIstrstream& str, CORBA::Boolean show_events = 0);
  void out_config(RDIstrstream& str);
  void out_info_filters(RDIstrstream& str, CORBA::Boolean admins, CORBA::Boolean proxies);
  void cleanup(RDIstrstream& str, CORBA::Boolean admins, CORBA::Boolean proxies);

  const AttN::NameSeq& L_my_name() { return _my_name; }

  RDI_PullSupplier*      pull_supplier() { if (_shutmedown) return 0; return _pull_supplier; }
  RDI_NotifyConsumer*    push_consumer() { if (_shutmedown) return 0; return _push_consumer; }
  RDI_ChangePool*        ochange_pool()  { if (_shutmedown) return 0; return _ochange_pool; }
  RDI_ChangePool*        schange_pool()  { if (_shutmedown) return 0; return _schange_pool; }

  CORBA::Boolean shutting_down() { return _shutmedown; }

  // N.B. only the macros in RDIOplocksMacros.h should call this function
  RDIOplockEntry** macro_get_oplockptr_ptr() { return &_oplockptr; }
  RDI_TypeMap*     macro_get_typemap_ptr()   { return _type_map; }

private:
  RDIOplockEntry*        _oplockptr;
#ifndef NO_OBJ_GC
  RDI_TimeT	         _last_use;
  TW_Thread*             _gcollector;
  TW_Condition*          _gc_wait;
  TW_Condition*          _gc_exit;
  CORBA::Boolean         _gcisactive;
  CORBA::ULong           _objGCPeriod;
#endif
  AttN::EventChannel_var _my_oref;
  AttN::NameSeq          _my_name;
  EventChannelFactory_i* _my_channel_factory;
  ConsumerAdmin_i*       _def_consumer_admin;
  SupplierAdmin_i*       _def_supplier_admin;
  FilterFactory_i*       _def_filter_factory;
  RDI_NotifQoS*          _qosprop;
  RDI_TypeMap*           _type_map;
  RDI_AdminQoS           _admin_qos;
  RDI_ServerQoS*         _server_qos;
  CORBA::ULong           _serial;
  CORBA::ULong           _admin_serial;
  CORBA::ULong           _num_consadmin;
  CORBA::ULong           _num_suppadmin;
  CORBA::ULong           _num_consumers;
  CORBA::ULong           _num_suppliers;

  TW_Mutex               _proxy_lock;   // Proxy event queue mutex
  TW_Condition           _proxy_empty;  // Proxy event queue condition

  // Event filtering and dispatching thread pool
  EventChannelDispatch*  _rdi_dispatch;

  // Global thread responsible for channel maintainance and cleanup.
  // The conditional variable is used to signal channel destruction,
  // as well as implement perioding garbage collection.
  // When 'destroy' is called on the channel,  '_shutmedown' becomes
  // non zero. 
  // When the garbage collection thread exits, '_gcisactive' becomes
  // zero.

  CORBA::Boolean         _shutmedown;

  TW_Thread*             _reporter;
  TW_Condition*          _rep_wait;
  TW_Condition*          _rep_exit;
  CORBA::Boolean         _repisactive;
  CORBA::ULong           _repInterval;

  // Lists of announced events and entries for proxy dispatching
  RDI_EventQueue*        _events;

  TW_Mutex               _qos_lock;
  TW_Mutex               _stats_lock;
  RDI_Watch              _performance_timer;
  RDI_ThStat*            _thread_stats;

  CORBA::ULong           _gq_acm;
  CORBA::ULong           _gq_ctr;
  CORBA::ULong           _pq_acm;
  CORBA::ULong           _pq_ctr;

  CORBA::ULong           _prev_num_rdi_match;
  CORBA::ULong           _prev_num_rvm_eval;
  CORBA::ULong           _prev_num_announcements;
  CORBA::ULong           _prev_num_notifications;

  CORBA::ULong           _stat_update_counter;
  CORBA::ULong           _stat_delta_target;

  CORBA::LongLong        _cum_msecs;
  CORBA::Boolean         _second_delta;
  // sleep this many nanosecs on each global event queue insert
  // (starts zero, adjusted according to average notif queue size) 
  unsigned long          _gq_sleep_nanosecs;
  double                 _prev_avg_nq_sz;

  RDI_List<ProxyDispatch_t> _proxy_events;

  // The following is the group manager for ConsumerAdmin_i objects
  // and it is used by the threads that perfrom filtering on behalf
  // of ConsumerAdmin_i objects

  CAdminGroupMgr*           _admin_group;

  // Hash tables for the various admin objects

  RDI_Hash<CosNA::AdminID, SupplierAdmin_i *> _supl_admin;
  RDI_Hash<CosNA::AdminID, ConsumerAdmin_i *> _cons_admin;

  // To support offer_change() we need a hash table of event types

  RDI_Hash<CosN::EventType, CORBA::ULong> _evtypes;

  // The thread pools used for pulling event from Pull suppliers
  // and pushing events to push consumers -- they may be NULL

  RDI_PullSupplier*      _pull_supplier;
  RDI_NotifyConsumer*    _push_consumer;

  // The thread pools used for sending offer_change and subscription_change
  // messages -- they may be NULL
  RDI_ChangePool*       _ochange_pool;
  RDI_ChangePool*       _schange_pool;

  // Do actual work of adding event to global event queue
  // returns -1 on failure, else 0
  int _new_structured_event(RDI_StructuredEvent* event);

  // Carry out event filtering and dispatching for ConsumerAdmin 
  // and ProxySupplier objects

  void  admin_dispatch();
  void  proxy_dispatch();

  // Does one of the filters, if any, of a given ConsumerAdmin_i
  // object match an announced event having the provided type?

  CORBA::Boolean match_event(ConsumerAdmin_i*      admin,
			     RDI_StructuredEvent*  event,
			     RDI_FilterState_t&    fstate);

#ifndef NO_OBJ_GC
  void  gcollect();
#endif
  void  periodic_report();
  void _children(AttN::IactSeq& ren, CORBA::Boolean only_cleanup_candidates);

  virtual ~EventChannel_i();
};

#undef WHATFN
#define WHATFN "EventChannel_i::incr_num_rdi_match"
inline void EventChannel_i::incr_num_rdi_match()
{ 
  int id = RDI_TH_INDEX();
  {
    TW_SCOPE_LOCK(_thread_stats_lock, _thread_stats[id]._lock, "thread_stats", WHATFN);
    ++_thread_stats[id]._num_rdi_match;
  }
}

#undef WHATFN
#define WHATFN "EventChannel_i::incr_num_rvm_eval"
inline void EventChannel_i::incr_num_rvm_eval()
{ 
  int id = RDI_TH_INDEX();
  {
    TW_SCOPE_LOCK(_thread_stats_lock, _thread_stats[id]._lock, "thread_stats", WHATFN);
    ++_thread_stats[id]._num_rvm_eval;
  }
}

#undef WHATFN
#define WHATFN "EventChannel_i::incr_num_announcements"
inline void EventChannel_i::incr_num_announcements()
{
  int id = RDI_TH_INDEX(), do_minor;
  {
    TW_SCOPE_LOCK(_thread_stats_lock, _thread_stats[id]._lock, "thread_stats", WHATFN);
    do_minor = (++_thread_stats[id]._num_announcements % RDI_STATS_MINOR_INCREMENT == 0);
  }
  if (do_minor) {
    RDI_LocksHeld held = { 0 };
    TW_SCOPE_LOCK_TRACK(_chan_stats_lock, held.chan_stats, _stats_lock, "chan_stats", WHATFN);
    _gq_ctr++; _gq_acm += _events->length();
    _pq_ctr++; _pq_acm += _proxy_events.length();
    if (++_stat_update_counter == _stat_delta_target) {
      _stat_delta_target += RDI_STATS_DELTA_INCREMENT;
      dump_stats(held, 0);
    }
  }
}

#undef WHATFN
#define WHATFN "EventChannel_i::incr_num_notifications"
inline void EventChannel_i::incr_num_notifications(unsigned int qsize)
{
  int id = RDI_TH_INDEX(), do_minor;
  {
    TW_SCOPE_LOCK(_thread_stats_lock, _thread_stats[id]._lock, "thread_stats", WHATFN);
    do_minor = (++_thread_stats[id]._num_notifications % RDI_STATS_MINOR_INCREMENT == 0);
    if (do_minor) {
      _thread_stats[id]._qsize_ctr++;
      _thread_stats[id]._qsize_acum += qsize;
    }
  }
  if (do_minor) {
    RDI_LocksHeld held = { 0 };
    TW_SCOPE_LOCK_TRACK(_chan_stats_lock, held.chan_stats, _stats_lock, "chan_stats", WHATFN);
    _gq_ctr++; _gq_acm += _events->length();
    _pq_ctr++; _pq_acm += _proxy_events.length();
    if (++_stat_update_counter == _stat_delta_target) {
      _stat_delta_target += RDI_STATS_DELTA_INCREMENT;
      dump_stats(held, 0);
    }
  }
}

/** The event channel factory is responsible for creating event
  * channels.  This object supports concurrency and maintains a
  * list of all the channels it has created so far. 
  */

class EventChannelFactory_i : 
	WRAPPED_SKELETON_SUPER(AttNotification, EventChannelFactory) 
{
  friend class EventChannel_i;
public:
  EventChannelFactory_i(FilterFactory_i* ffactory,
			const RDI_NotifQoS& defqos,
			const RDI_AdminQoS& defadm,
			RDI_ServerQoS*      s_qos);

  // Methods from AttNotification::Interactive Interface
  char* do_command(const char* cmd, CORBA::Boolean& success, CORBA::Boolean& target_changed,
		   AttN_Interactive_outarg next_target  WRAPPED_DECLARG);
  AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID );
  AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID );
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates  WRAPPED_DECLARG );
  CORBA::Boolean safe_cleanup( WRAPPED_DECLARG_VOID );

  // Methods from CosN::QoSAdmin Interface (apply to *defaults*)
  CosN::QoSProperties* get_qos( WRAPPED_DECLARG_VOID );
  void set_qos(const CosN::QoSProperties& qos WRAPPED_DECLARG );
  void validate_qos(const CosN::QoSProperties& r_qos,
		    CosN_NamedPropertyRangeSeq_outarg a_qos
		    WRAPPED_DECLARG );

  // Methods from CosN::AdminPropertiesAdmin Interface (apply to *defaults*)
  CosN::AdminProperties* get_admin( WRAPPED_DECLARG_VOID );
  void set_admin(const CosN::AdminProperties& admin WRAPPED_DECLARG );

  // Methods from CosN::EventChannelFactory Interface
  CosNA::EventChannel_ptr 
	create_channel(const CosN::QoSProperties&   init_qos,
		       const CosN::AdminProperties& init_adm,
		       CosNA::ChannelID& myserial WRAPPED_DECLARG );

  CosNA::ChannelIDSeq* get_all_channels( WRAPPED_DECLARG_VOID );

  CosNA::EventChannel_ptr get_event_channel(CosNA::ChannelID id WRAPPED_DECLARG );

  // (Local only -- not available via RPC)

  void server_qos_changed();
  EventChannel_i* _create_channel(CosNA::ChannelID& myserial); // does the real work, assumes oplock is held
  EventChannel_i* create_channel(CosNA::ChannelID& myserial); 
  void            remove_channel(CosNA::ChannelID id);

  void cleanup_and_dispose();

  RDIstrstream& log_output(RDIstrstream& str);

  // Local-only functions
  CORBA::Boolean do_go_command(RDIstrstream& str, RDIParseCmd& p,
			       CORBA::Boolean& target_changed,
			       AttN_Interactive_outarg next_target);
  CORBA::Boolean do_set_command(RDIstrstream& str, RDIParseCmd& p);
  void out_commands(RDIstrstream& str);
  void out_all_stats(RDIstrstream& str);
  void out_all_debug_info(RDIstrstream& str);
  void out_default_config(RDIstrstream& str);
  void out_all_config(RDIstrstream& str);
  void cleanup_all(RDIstrstream& str, CORBA::Boolean admins, CORBA::Boolean proxies);

  const AttN::NameSeq& L_my_name() { return _my_name; }

private:
  RDIOplockEntry*                              _oplockptr;
  AttN::EventChannelFactory_var                _my_oref;
  FilterFactory_i*                             _ffactory;
  AttN::NameSeq                                _my_name;
  // _disposed is true once cleanup_and_dispose has been called
  CORBA::Boolean                               _disposed;
  CORBA::ULong                                 _serial;
  RDI_NotifQoS                                 _defqos;
  RDI_AdminQoS                                 _defadm;
  RDI_ServerQoS*                               _server_qos;
  RDI_Hash<CosNA::ChannelID, EventChannel_i*>  _channel;

  void _children(AttN::IactSeq& ren, CORBA::Boolean only_cleanup_candidates);

  virtual ~EventChannelFactory_i();
};

////////////////////////////////////////
// Logging

inline RDIstrstream& operator<< (RDIstrstream& str, const ProxyPullSupplier_i& prx) { return prx.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, const StructuredProxyPullSupplier_i& prx) { return prx.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, const SequenceProxyPullSupplier_i& prx) { return prx.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, const ProxyPushSupplier_i& prx) { return prx.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, const StructuredProxyPushSupplier_i& prx) { return prx.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, const SequenceProxyPushSupplier_i& prx) { return prx.log_output(str); }

inline RDIstrstream& operator<< (RDIstrstream& str, const ProxyPushConsumer_i& prx) { return prx.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, const StructuredProxyPushConsumer_i& prx) { return prx.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, const SequenceProxyPushConsumer_i& prx) { return prx.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, const ProxyPullConsumer_i& prx) { return prx.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, const StructuredProxyPullConsumer_i& prx) { return prx.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, const SequenceProxyPullConsumer_i& prx) { return prx.log_output(str); }

inline RDIstrstream& operator<< (RDIstrstream& str, ConsumerAdmin_i& adm) { return adm.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, SupplierAdmin_i& adm) { return adm.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, EventChannel_i& evc) { return evc.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, EventChannelFactory_i& ecf) { return ecf.log_output(str); }

#endif
