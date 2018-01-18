// -*- Mode: C++; -*-
//                              File      : CosNotification_i.h
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
//     set of C++ definitions for the CosNotification module
//
 
#ifndef _COS_NOTIFICATION_I_H_
#define _COS_NOTIFICATION_I_H_

#include "thread_wrappers.h"
#include "RDIStringDefs.h"
#include "RDITimeWrappers.h"
#include "RDIConfig.h"
#include "CosNotifyShorthands.h"

////////////////////////////////////////////////////////////////////////////
// QoS properties can be set at multiple levels: proxy and admin objects, //
// event channels, and individual event messages.                         //
//                                                                        //
//   eventReliability      : BestEffort or Persistent - can be set on the //
//                         : event message or the event channel           //
//   connectionReliability : BestEffort or Persistent - can be set on the //
//                         : event channel only                           //
//   priority              : The priority used to dispatch matced events  //
//   timeout               : Expiration time assigned to event messages   //
//   startTimeSupported    : Scheduled future delivery of events starts   //
//   stopTimeSupported     : Scheduled future delivery of events ends     //
//   orderPolicy           : Ordering policy used for dispatching events  //
//   discardPolicy         : Discarding policy used when queues overflow  //
//   pacingInterval        : Time period for collecting events before the //
//                         : delivery to a consumer                       // 
//   maxEventsPerConsumer  : Maximum number of queued events for a given  //
//                         : consumer -- 0 does not impose any limits     //
//   maximumBatchSize      : Maximum number of events in a batch; used by //
//                         : consumers that receive sequences of events   //
////////////////////////////////////////////////////////////////////////////

enum RDI_ObjectKind { RDI_NFSERVER, RDI_ECHANNEL, RDI_C_ADMIN,  RDI_S_ADMIN,
                      RDI_S_AnyPRX, RDI_S_StrPRX, RDI_S_SeqPRX, RDI_C_AnyPRX,
		      RDI_C_StrPRX, RDI_C_SeqPRX, RDI_EMESSAGE };

class RDI_NotifQoS {
public:
  RDI_NotifQoS() {}
  RDI_NotifQoS(const RDI_NotifQoS& qos)	{ *this = qos; }
  RDI_NotifQoS(RDI_NotifQoS * parent);
  
  RDI_NotifQoS& operator= (const RDI_NotifQoS& qos);

  CosN::QoSProperties* get_qos(RDI_ObjectKind otype) const;

  // Set some/all values of the QoS properties based on the values given
  // in 'r_qos'.  We assume that validation has already taken place

  void set_qos(const CosN::QoSProperties& r_qos);
  
  // Validate the requested QoS properties, 'r_qos', for a given object.
  // The current QoS properties of this object are 'a_qos', and its kind
  // is 'otype'.  In case the object has a parent, the QoS properties of
  // its parent, 'p_qos', are used to resolve conflicts.  Since some QoS
  // properties cannot be changed once set for objects that manage other
  // objects, 'subobjs' is TRUE when the current object has sub-objects.

  static CORBA::Boolean validate(const CosN::QoSProperties& r_qos,
				 const RDI_NotifQoS& a_qos,
				 RDI_ObjectKind otype,
				 CosN::PropertyErrorSeq& error,
				 CosN::NamedPropertyRangeSeq& range,
				 CORBA::Boolean subobjs);

  static CORBA::Boolean is_qos_prop(const char* pname);

  enum { NUM_PROPERTIES = 11 };

  RDI_NotifQoS* parent() const { return _parent; }
  CORBA::Boolean all_inherited() { return _all_inherited; }

  // Get Function
  CORBA::Short eventReliability() const; 
  CORBA::Short connectionReliability() const;
  CORBA::Short priority() const;
  CORBA::Boolean startTimeSupported() const;
  CORBA::Boolean  stopTimeSupported() const;
  CORBA::Short orderPolicy() const;
  CORBA::Short discardPolicy() const;
  CORBA::Long maxEventsPerConsumer() const;
  CORBA::Long maximumBatchSize() const;

  // 3 variants for TimeT
  RDI_TimeT timeout() const;
  void timeout_timebase(TimeBase::TimeT& to) const;
  void timeout_s_n(unsigned long& s, unsigned long &n) const;
  // 3 variants for TimeT
  RDI_TimeT pacingInterval() const;
  void pacingInterval_timebase(TimeBase::TimeT& to) const;
  void pacingInterval_s_n(unsigned long& s, unsigned long &n) const;

  // Set Functions
  void eventReliability(CORBA::Short event_reliability);
  void connectionReliability(CORBA::Short connection_reliability);
  void priority(CORBA::Short priority);
  void startTimeSupported(CORBA::Boolean start_time_support);
  void stopTimeSupported(CORBA::Boolean stop_time_support);
  void orderPolicy(CORBA::Short order_policy);
  void discardPolicy(CORBA::Short discard_policy);
  void maxEventsPerConsumer(CORBA::Long max_event_percunsumer);
  void maximumBatchSize(CORBA::Long max_batch_size);
  void timeout(RDI_TimeT timeout);
  void pacingInterval(RDI_TimeT pacing_interval);

  RDIstrstream& log_output(RDIstrstream& str) const;
private:
  RDI_NotifQoS*   _parent;
  CORBA::Boolean  _all_inherited;

  CORBA::Short    _eventReliability;
  CORBA::Boolean  _eventReliability_set;

  CORBA::Short    _connectionReliability;
  CORBA::Boolean  _connectionReliability_set;

  CORBA::Short    _priority;
  CORBA::Boolean  _priority_set;

  RDI_TimeT       _timeout;
  CORBA::Boolean  _timeout_set;

  CORBA::Boolean  _startTimeSupported;
  CORBA::Boolean  _startTimeSupported_set;

  CORBA::Boolean  _stopTimeSupported;
  CORBA::Boolean  _stopTimeSupported_set;

  CORBA::Short    _orderPolicy;
  CORBA::Boolean  _orderPolicy_set;

  CORBA::Short    _discardPolicy;
  CORBA::Boolean  _discardPolicy_set;

  RDI_TimeT       _pacingInterval;
  CORBA::Boolean  _pacingInterval_set;

  CORBA::Long     _maxEventsPerConsumer;
  CORBA::Boolean  _maxEventsPerConsumer_set;

  CORBA::Long     _maximumBatchSize;
  CORBA::Boolean  _maximumBatchSize_set;
};

////////////////////////////////////////////////////////////////////////////
// The following are the Administrative QoS properties that can be set on //
// a per-channel basis. They are part of the CORBA Notification spec.     //
//                                                                        //
//   maxQueueLength  : Max size of internal event queues - 0 for no limit //
//   maxConsumers    : Max number of connected consumers - 0 for no limit //
//   maxSuppliers    : Max number of connected suppliers - 0 for no limit //
//   rejectNewEvents : If TRUE, new events are rejected on queue overflow //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

class RDI_AdminQoS {
public:
  RDI_AdminQoS() : maxQueueLength(0),  maxConsumers(0),   maxSuppliers(0),
		   rejectNewEvents(1) {;}
  RDI_AdminQoS(const RDI_AdminQoS& qos)	{ *this = qos; }

  // Generate the 'CosN::AdminProperties' list from the
  // existing values for the various administrative QoS properties

  CosN::AdminProperties* to_admin() const;

  // Assuming that the provided 'CosN::AdminProperties'
  // list is valid, set the administrative QoS properties 

  void from_admin(const CosN::AdminProperties& a_qos);

  // Validate the provided QoS property list based on the existing
  // settings.  Invalid properties/values are recorded in 'error'.
  // ** Explanations for omniNotify-specific ("strange") error cases
  // are placed on str.

  CORBA::Boolean validate(RDIstrstream& str,
			  const CosN::AdminProperties& a_qos,
			  CosN::PropertyErrorSeq& error);

  RDI_AdminQoS& operator= (const RDI_AdminQoS& qos);

  RDIstrstream& log_output(RDIstrstream& str) const;

  static CORBA::Boolean is_admin_prop(const char* pname);

public:
  CORBA::Long    maxQueueLength;
  CORBA::Long    maxConsumers;
  CORBA::Long    maxSuppliers;
  CORBA::Boolean rejectNewEvents;
};

////////////////////////////////////////////////////////////////////////////
// The following are Server-level Admin QoS properties that can be set on //
// at the server level only. (They are not part of the standard.)         //
//                                                                        //
//   numAdminGroups  : # administrative groups - should be at least 1     //
//   numAdminThreads : # threads for filter evaluation at the admin level //
//                   : These threads perfrom proxy filter evaluation when //
//                   : 'numProxyThreads' is 0 - should be at least 1      //
//   numProxyThreads : # threads used for proxy filter evaluation         //
//   numPushThreads  : # threads used for dispatching events to consumers //
//                   : If 0, each push consumer has its own thread        //
//   numPullThreads  : # threads used for 'pulling' events from suppliers //
//                   : If 0, each pull suppliers has its own thread       //
//   pullEventPeriod : Period, in milliseconds, when pulling events. When //
//                   : set to 0, we ask for new events without any delays //
//   queueGCPeriod   : Period, in seconds, used by the garbage collector  //
//                     of the event queue.  minimum value is 1            //
//   numOChangeThreads : # threads used for offer_change msgs             //
//                                     sent to consumers                  //
//   numSChangeThreads : # threads used for subscription_change msgs      //
//                                     sent to suppliers                  //
//   reportingInterval : Period, in seconds, for adding channel           //
//                       reports to the report log.                       //
//                       0 means do not add such periodic reports.        //
//   localClockInaccLo/Hi: local clock inaccuracy in units of 100ns       //
//                         (must fit in 48 bits, 32 for Lo, 16 for Hi)    //
//   localClockTDF    : Local time zone expressed in minutes west of      //
//                      greenwhich meridian; 300=Eastern time zone, USA   // 
//   outgoingTimeout  : Interval, in milliseconds.  If non-zero and an    //
//                      outgoing call exceeds timeout, it is terminated   //
//                      and the client is marked as disconnected -- must  //
//                      connect to a DIFFERENT proxy to start interacting //
//                      with the channel again.  For omniORB3, which can  //
//                      only do timeouts in seconds, a non-zero value is  //
//                      converted to the closes NON-ZERO seconds value    //
//                      (we never round a non-zero value to zero).        //
//   incomingTimeout  : Interval, in milliseconds.  If non-zero and an    //
//                      incoming call exceeds timeout (usu. while the ORB //
//                      is in the process of receiving the request, due   //
//                      to a hung client) then the connection is dropped. //
//                      Unlike the outgoing case, the client is not       //
//                      marked as disconnected.                           //
//                                                                        //
// If NO_OBJ_GC is not defined, these are available:                      //
//                                                                        //
//   objectGCPeriod  : Period in seconds, used by obj garbage collector.  //
//                     Can be 0 to disable object gc.                     //
//   deadChanInterval : GC chan not used in # secs                        //
//   deadAdminInterval : GC admin not used in # secs                      //
//   deadConProxyInterval : GC connected proxy not used in # secs         //
//   deadOtherProxyInterval : GC disconnected proxy not used in # secs      //
//   deadFitlerInterval : GC filter not used in # secs                    //
////////////////////////////////////////////////////////////////////////////

class RDI_ServerQoS {
public:
  RDI_ServerQoS() :
    numAdminGroups(2), numAdminThreads(2),
    numProxyThreads(0), numPushThreads(4), numPullThreads(2),
    numOChangeThreads(1), numSChangeThreads(1),
    pullEventPeriod(100), queueGCPeriod(300), 

    reportingInterval(0),
    localClockInaccLo(0), localClockInaccHi(0), localClockTDF(300),
    outgoingTimeout(10000), incomingTimeout(10000)
#ifndef NO_OBJ_GC
    , objectGCPeriod(600),
    deadChanInterval(0), deadAdminInterval(0), deadConProxyInterval(0),
    deadOtherProxyInterval(600), deadFilterInterval(0)
#endif
  {;}
  RDI_ServerQoS(const RDI_ServerQoS& qos)	{ *this = qos; }

  // Generate the 'CosN::AdminProperties' list from the
  // existing values for the various administrative QoS properties

  AttN::ServerProperties* to_server_props() const;

  // Assuming that the provided 'AttN::ServerProperties'
  // list is valid, set the server QoS properties 

  void from_server_props(const AttN::ServerProperties& s_qos);

  // Validate the provided QoS property list based on the existing
  // settings.  Invalid properties/values are recorded in 'error'.
  // This can be called by either at startup or once the server
  // is running.  In the latter case, some QoS values cannot change.
  // ** Explanations for omniNotify-specific ("strange") error cases
  // are placed on str.

  CORBA::Boolean validate(RDIstrstream& str,
			  const AttN::ServerProperties& s_qos,
			  CosN::PropertyErrorSeq& error,
			  CORBA::Boolean startup);

  RDI_ServerQoS& operator= (const RDI_ServerQoS& qos);

  RDIstrstream& log_output(RDIstrstream& str) const;

  static CORBA::Boolean is_server_prop(const char* pname);

public:
  CORBA::UShort  numAdminGroups;
  CORBA::UShort  numAdminThreads;
  CORBA::UShort  numProxyThreads;
  CORBA::UShort  numPushThreads;
  CORBA::UShort  numPullThreads;
  CORBA::UShort  numOChangeThreads;
  CORBA::UShort  numSChangeThreads;
  CORBA::ULong   pullEventPeriod;
  CORBA::ULong   queueGCPeriod;
  CORBA::ULong   reportingInterval;
  CORBA::ULong   localClockInaccLo;
  CORBA::UShort  localClockInaccHi;
  CORBA::Short   localClockTDF;
  CORBA::ULong   outgoingTimeout;
  CORBA::ULong   incomingTimeout;
#ifndef NO_OBJ_GC
  CORBA::ULong   objectGCPeriod;
  CORBA::ULong   deadChanInterval;
  CORBA::ULong   deadAdminInterval;
  CORBA::ULong   deadConProxyInterval;
  CORBA::ULong   deadOtherProxyInterval;
  CORBA::ULong   deadFilterInterval;
#endif
};

////////////////////////////////////////////////////////////////////////////
// RDI_AllQoS is a placeholder class with static functions
// that operate over RDI_NotifQoS, RDI_AdminQoS, and RDI_ServerQoS properties.
//
// void install_all_defaults(config)
//
// CORBA::Boolean validate_initial_config(RDIstrstream& str, 
//	 				  const RDIConfig& config,
//					  RDI_NotifQos& n_qos,
//					  RDI_AdminQos& a_qos,
//					  RDI_AdminQos& s_qos);
//     config contains a complete initial configuration 
//     to be used as the default for the event channel factory.
//     Returns 1 on success, 0 on failure.
//     Fills in nqos, aqos, sqos with values as found in config.
//     Puts error msgs on RDIOutLog (stdout).
//
//  static CORBA::Boolean parse_set_command(RDIstrstream& str, 
//					    RDIParseCmd& p,
//					    RDI_ObjectKind otype,
//					    CosN::QoSProperties&   n_qos,
//					    CosN::AdminProperties& a_qos,
//					    CosN::AdminProperties& s_qos);
//     p is parsed form of 'set' command, which has the form
//         set <name> <value> [ <name> <value> ... ]
//     (p.argv[0] is 'set', p.argv[1/2] is first pair, p.argv[3/4] is 2nd pair, etc.)
//     n_qos and a_qos should start empty and are filled in
//     in preperation for appropriate set_qos / set_admin calls
//     Returns 1 on success, 0 on failure
//     Puts error msgs in str.
//     Note: type errors and bad param names are detected, but an
//       illegal value (of the required type) is not detected -- 
//       set_qos or set_admin calls will detect such errors.
////////////////////////////////////////////////////////////////////////////

class RDI_AllQoS {
public:
  static void install_all_defaults(RDI_Config& config);

  static CORBA::Boolean validate_initial_config(RDIstrstream&  str, 
						RDI_Config&    config,
						RDI_NotifQoS&  n_qos,
						RDI_AdminQoS&  a_qos,
						RDI_ServerQoS& s_qos);

  static CORBA::Boolean parse_set_command(RDIstrstream&                str, 
					  RDIParseCmd&                 p,
					  RDI_ObjectKind               otype,
					  CosN::QoSProperties&         n_qos,
					  CosN::AdminProperties&       a_qos,
					  AttN::ServerProperties&      s_qos);
};

////////////////////////////////////////
// Logging

inline RDIstrstream& operator<< (RDIstrstream& str, const RDI_NotifQoS& qos)   { return qos.log_output(str);  }
inline RDIstrstream& operator<< (RDIstrstream& str, const RDI_AdminQoS& aqos)  { return aqos.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, const RDI_ServerQoS& sqos) { return sqos.log_output(str); }

#endif
