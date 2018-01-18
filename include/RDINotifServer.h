// -*- Mode: C++; -*-
//                              File      : RDIServer_i.h
//                              Package   : omniNotify-Library
//                              Created on: 1-July-2001
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
//    impl of AttNotification::Server interface
//
 
#ifndef __RDI_NOTIF_SERVER_I_H__
#define __RDI_NOTIF_SERVER_I_H__

#include "corba_wrappers.h"
#include "CosNotifyShorthands.h"
#include "RDIstrstream.h"
#include "RDIConfig.h"
#include "RDIOplocks.h"
#include "CosNotification_i.h"

class EventChannel_i;
class EventChannelFactory_i;
class FilterFactory_i;
class RDINotifServer;

#define RDI_NOTIFSERVER_WORKER_PRIORITY TW_PRIORITY_NORMAL

typedef void (RDINotifServer::*RDINotifServerMethod)(void);

class RDINotifServerWorker : public TW_Thread {
public:
  RDINotifServerWorker(RDINotifServer* server, RDINotifServerMethod method,
		       TW_PRIORITY_T priority = RDI_NOTIFSERVER_WORKER_PRIORITY);
  void run(void *);
private:
  RDINotifServer*         _server;
  RDINotifServerMethod    _method;

  RDINotifServerWorker()  {;}
};

class RDINotifServer :
  WRAPPED_SKELETON_SUPER(AttNotification, Server) 
{
  friend class RDI;
public:
  RDINotifServer(RDI_Config*                   config,
		 FilterFactory_i*              ffact_i,
		 AttN::FilterFactory_ptr       ffact,
		 EventChannelFactory_i*        cfact_i,
		 AttN::EventChannelFactory_ptr cfact,
		 EventChannel_i*               chan_i, 
		 AttN::EventChannel_ptr        chan,
		 RDI_ServerQoS*                s_qos);

  // Create is used to help implement RDI::init_server
  static RDINotifServer* create(int& argc, char** argv);

  // Methods from AttNotification::Interactive Interface
  char* do_command(const char* cmd, CORBA::Boolean& success, CORBA::Boolean& target_changed,
		   AttN_Interactive_outarg next_target  WRAPPED_DECLARG);
  AttN::NameSeq* child_names( WRAPPED_DECLARG_VOID );
  AttN::NameSeq* my_name( WRAPPED_DECLARG_VOID );
  AttN::IactSeq* children(CORBA::Boolean only_cleanup_candidates  WRAPPED_DECLARG );
  CORBA::Boolean safe_cleanup( WRAPPED_DECLARG_VOID );

  // Methods from AttNotification::Server Interface
  void destroy( WRAPPED_DECLARG_VOID );
  AttN::FilterFactory_ptr       get_filter_factory( WRAPPED_DECLARG_VOID );
  AttN::EventChannelFactory_ptr get_channel_factory( WRAPPED_DECLARG_VOID );
  AttN::EventChannel_ptr        get_default_channel( WRAPPED_DECLARG_VOID );
  CORBA::Boolean results_to_file( WRAPPED_DECLARG_VOID );
  AttN::ServerProperties* get_server_props( WRAPPED_DECLARG_VOID );
  void  set_server_props(const AttN::ServerProperties& props WRAPPED_DECLARG );

  // Local-only methods
  FilterFactory_i*              get_filter_factory_i()   { return _ffactory_i; }
  EventChannelFactory_i*        get_channel_factory_i()  { return _cfactory_i; }
  EventChannel_i*               get_def_channel_i()      { return _channel_i; }

  static CORBA::Boolean is_startup_prop(const char* p);

  RDIstrstream& log_output(RDIstrstream& str);

  CORBA::Boolean do_go_command(RDIstrstream& str, RDIParseCmd& p,
			       CORBA::Boolean& target_changed,
			       AttN_Interactive_outarg next_target);
  CORBA::Boolean do_set_command(RDIstrstream& str, RDIParseCmd& p);
  void out_server_config(RDIstrstream& str);
  void out_flags(RDIstrstream& str);
  void out_commands(RDIstrstream& str);
  CORBA::Boolean out_all_stats(RDIstrstream& str, const char* targ);
  CORBA::Boolean out_all_debug_info(RDIstrstream& str, const char* targ);
  CORBA::Boolean out_all_config(RDIstrstream& str, const char* targ);
  CORBA::Boolean flag_change(RDIstrstream& str, const char* change);

  const AttN::NameSeq& L_my_name() { return _my_name; }

  // This method normally called by main program thread.
  // It waits for destroy to be invoked, then invokes _destroy.
  void L_wait_for_destroy(); 

  CORBA::Boolean destroyed() { return _destroyed; }

private:
  RDIOplockEntry*                _oplockptr;
  AttN::NameSeq                  _my_name;
  CORBA::Boolean                 _destroyed;
  CORBA::Boolean                 _destroy_called;
  RDI_Config*                    _configp; 
  FilterFactory_i*               _ffactory_i;
  AttN::FilterFactory_var        _ffactory; 
  EventChannelFactory_i*         _cfactory_i;
  AttN::EventChannelFactory_var  _cfactory;
  EventChannel_i*                _channel_i;
  AttN::EventChannel_var         _channel;
  RDI_ServerQoS*                 _server_qos;
  CORBA::ULong                   _outgoingTimeout;
  CORBA::ULong                   _incomingTimeout;
  CORBA::ULong                   _origScanGran;
#ifndef NO_OBJ_GC
  TW_Thread*                     _gcollector;
  TW_Condition*                  _gc_wait;
  TW_Condition*                  _gc_exit;
  CORBA::Boolean                 _gcisactive;
  CORBA::ULong                   _objGCPeriod;
#endif

  void _destroy(); // do the real destroy work
#ifndef NO_OBJ_GC
  void  gcollect();
#endif

  virtual ~RDINotifServer();
};

////////////////////////////////////////
// Logging

inline RDIstrstream& operator<< (RDIstrstream& str, RDINotifServer& s) { return s.log_output(str); }

#endif  /*  __RDI_NOTIF_SERVER_I_H__   */

