// -*- Mode: C++; -*-
//                              File      : RDINotifServer.cc
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
//    Implementation of RDINotifServer
//
 
#include "corba_wrappers.h"
#include "RDINotifServer.h"
#include "RDI.h"
#include "RDIstrstream.h"
#include "RDICatchMacros.h"
#include "RDIOplocksMacros.h"
#include "RDILimits.h"
#include "RDIStringDefs.h"
#include "RDITimeWrappers.h"
#include "CosNfyUtils.h"
#include "CosNotifyChannelAdmin_i.h"

// ----------------------------------------------------------- //

RDINotifServerWorker::RDINotifServerWorker(RDINotifServer*         server,
					   RDINotifServerMethod    method,
					   TW_PRIORITY_T           trprio) :
  TW_Thread(NULL, trprio), _server(server), _method(method)
{;}

void
RDINotifServerWorker::run(void *)
{
  (_server->*_method)();
}

// ----------------------------------------------------------- //

RDINotifServer::RDINotifServer(RDI_Config*                   config,
			       FilterFactory_i*              ffact_i,
			       AttN::FilterFactory_ptr       ffact,
			       EventChannelFactory_i*        cfact_i,
			       AttN::EventChannelFactory_ptr cfact,
			       EventChannel_i*               chan_i,
			       AttN::EventChannel_ptr        chan,
			       RDI_ServerQoS*                s_qos)
  : _oplockptr(0), _destroyed(0), _destroy_called(0), _configp(config),
    _ffactory_i(ffact_i), _cfactory_i(cfact_i), _channel_i(chan_i), _channel(chan), _server_qos(s_qos)
#ifndef NO_OBJ_GC
  , _gcollector(0), _gc_wait(0), _gc_exit(0),  _gcisactive(0)
#endif

{
  RDI_OPLOCK_INIT("server");
  _my_name.length(1);
  _my_name[0] = (const char*)"server";
  _ffactory = AttN::FilterFactory::_duplicate(ffact);
  _cfactory = AttN::EventChannelFactory::_duplicate(cfact);
  _origScanGran = WRAPPED_ORB_GET_SCAN_GRANULARITY();
  RDIDbgDaemonLog("Note: _origiScanGran set to " << _origScanGran << '\n');
  _outgoingTimeout = _server_qos->outgoingTimeout;
  _incomingTimeout = _server_qos->incomingTimeout;
  WRAPPED_ORB_SET_CLIENT_TIMEOUT(_outgoingTimeout);
  RDIDbgDaemonLog("Called WRAPPED_ORB_SET_CLIENT_TIMEOUT(" << _outgoingTimeout << ")\n");
  WRAPPED_ORB_SET_SERVER_TIMEOUT(_incomingTimeout);
  RDIDbgDaemonLog("Called WRAPPED_ORB_SET_SERVER_TIMEOUT(" << _incomingTimeout << ")\n");
  if (_outgoingTimeout || _incomingTimeout) { // Change the scan granularity to something reasonable for the timeouts
    // scanGran = min non-zero timeout (milliseconds)
    CORBA::ULong scanGran = (_outgoingTimeout && _outgoingTimeout < _incomingTimeout) ? _outgoingTimeout : _incomingTimeout;
    // convert scanGran to nearest NON-ZERO # of seconds
    scanGran = (scanGran < 1000) ? 1 : (scanGran + 500)/1000;
    if (_origScanGran == 0 || scanGran < _origScanGran) {
      WRAPPED_ORB_SET_SCAN_GRANULARITY(scanGran);
      RDIDbgDaemonLog("Called WRAPPED_ORB_SET_SCAN_GRANULARITY(" << scanGran << ") -- and recorded original value == " << _origScanGran << '\n');
    }
  }
  // Register 'this' to start receiving requests
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

RDINotifServer::~RDINotifServer()
{
  RDI_OPLOCK_DESTROY_CHECK("RDINotifServer");
}

RDINotifServer*
RDINotifServer::create(int& argc, char** argv)
{
  FILE*          iorfile = 0; 
  RDI_AdminQoS   defadm;
  RDI_NotifQoS   defqos((RDI_NotifQoS *) 0);
  RDI_ServerQoS* s_qos = 0;
  RDI_Config*    config = 0;

  char*          theChannelName    = 0;
  char*          theFactoryName    = 0;
  char*          factoryIORFile    = 0;
  char*          channelIORFile    = 0;
  char*          configFileName    = 0;
  char*          debugLogFile      = 0;
  char*          reportLogFile     = 0;
  CORBA::Boolean useNameService    = 1;

  CORBA::Boolean RDIDbgDaemon_set        = 0;  
  CORBA::Boolean RDIDbgFact_set          = 0;  
  CORBA::Boolean RDIDbgFilt_set          = 0;  
  CORBA::Boolean RDIDbgChan_set          = 0;  
  CORBA::Boolean RDIDbgCAdm_set          = 0;  
  CORBA::Boolean RDIDbgSAdm_set          = 0;  
  CORBA::Boolean RDIDbgCPxy_set          = 0;  
  CORBA::Boolean RDIDbgSPxy_set          = 0;  
  CORBA::Boolean RDIDbgEvQ_set           = 0;  
  CORBA::Boolean RDIDbgRDIEvent_set      = 0;  
  CORBA::Boolean RDIDbgFAdmin_set        = 0;  
  CORBA::Boolean RDIDbgEval_set          = 0;  
  CORBA::Boolean RDIDbgCosCPxy_set       = 0;  
  CORBA::Boolean RDIDbgCosSPxy_set       = 0;  
  CORBA::Boolean RDIDbgNotifQoS_set      = 0;  
  CORBA::Boolean RDIDbgAdminQoS_set      = 0;  
  CORBA::Boolean RDIDbgNotifQueue_set    = 0;  
                       
  CORBA::Boolean RDIRptChanStats_set     = 0;  
  CORBA::Boolean RDIRptQSizeStats_set    = 0;  
  CORBA::Boolean RDIRptCnctdCons_set     = 0;  
  CORBA::Boolean RDIRptCnctdSups_set     = 0;  
  CORBA::Boolean RDIRptCnctdFilts_set    = 0;  
  CORBA::Boolean RDIRptUnCnctdFilts_set  = 0;  
  CORBA::Boolean RDIRptRejects_set       = 0;  
  CORBA::Boolean RDIRptDrops_set         = 0;  
  CORBA::Boolean RDIRptNotifQoS_set      = 0;  
  CORBA::Boolean RDIRptAdminQoS_set      = 0;  
  CORBA::Boolean RDIRptServerQoS_set     = 0;  
  CORBA::Boolean RDIRptInteractive_set   = 0;  

  RDINotifServer*  server                = 0;
  EventChannel_i*  channel               = 0;
  FilterFactory_i* filter_factory        = 0;
  EventChannelFactory_i* factory         = 0;

  CosNA::ChannelID        channID;
  AttN::EventChannel_ptr  chanref;
  AttN::FilterFactory_var filtfactref;
  AttN::EventChannelFactory_var factref;

  RDIstrstream error_str; // for type error msgs

  // Parse command line arguments to locate any supported switches; 
  // They include '-n' to bypass the Naming Service, and '-c fname'
  // to use the configuration file 'fname'
  int indx = 0;
  while ( indx < argc ) {
    if ( RDI_STR_EQ(argv[indx], "-n") ) {
      useNameService = 0;
      RDI_rm_arg(argc, argv, indx);
    } else if ( RDI_STR_EQ(argv[indx], "-c") ) {
      RDI_rm_arg(argc, argv, indx);
      if ( indx < argc ) {
	configFileName = argv[indx];
	RDI_rm_arg(argc, argv, indx);
      } else {
	RDIOutLog("-c option requires config file argument\n");
	goto cleanup_and_return_0;
      }
    } else {
      indx += 1; // skip arg
    }
  }
  if ( ! (s_qos = new RDI_ServerQoS()) ) {
    RDIOutLog("Failed to create internal object [RDI_ServerQoS]\n");
    goto cleanup_and_return_0;
  }
  if ( ! (config = new RDI_Config()) ) {
    RDIOutLog("Failed to create internal object [RDI_Config]\n");
    goto cleanup_and_return_0;
  }
  // install 'built-in' defaults
  RDI_AllQoS::install_all_defaults(*config);

  // config file settings override built-in defaults
  if ( configFileName && config->import_settings(error_str, configFileName) ) {
    goto cleanup_and_return_0;
  }

  // environment variables override command line settings,
  // built-in defaults, and config file settings
  config->env_update();

  // command-line arguments override all of the above
  // (built-in defaults, config file settings, env variables)
  if ( config->parse_arguments(error_str, argc, argv, 1) ) {
    goto cleanup_and_return_0;
  }
  for (indx = 1; indx < argc; indx++) {
    RDIOutLog("omniNotify (WARNING) ignoring this argument: " << argv[indx] << '\n');
  }

  // The following are all strings so cannot have bad param types
  config->get_value(error_str, "ChannelFactoryName", theFactoryName, 0);
  config->get_value(error_str, "DefaultChannelName", theChannelName, 0);
  config->get_value(error_str, "FactoryIORFileName", factoryIORFile, 0);
  config->get_value(error_str, "ChannelIORFileName", channelIORFile, 0);
  config->get_value(error_str, "DebugLogFile",       debugLogFile,   0);
  config->get_value(error_str, "ReportLogFile",      reportLogFile,  0);

  // There are other params that must have a specific type...

  if (config->get_value(error_str, RDIDbgDaemon_nm        , RDIDbgDaemon_set,       1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgFact_nm          , RDIDbgFact_set,         1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgFilt_nm          , RDIDbgFilt_set,         1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgChan_nm          , RDIDbgChan_set,         1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgCAdm_nm          , RDIDbgCAdm_set,         1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgSAdm_nm          , RDIDbgSAdm_set,         1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgCPxy_nm          , RDIDbgCPxy_set,         1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgSPxy_nm          , RDIDbgSPxy_set,         1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgEvQ_nm           , RDIDbgEvQ_set,          1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgRDIEvent_nm      , RDIDbgRDIEvent_set,     1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgFAdmin_nm        , RDIDbgFAdmin_set,       1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgEval_nm          , RDIDbgEval_set,         1) == -2) { 
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgCosCPxy_nm       , RDIDbgCosCPxy_set,      1) == -2) { 
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgCosSPxy_nm       , RDIDbgCosSPxy_set,      1) == -2) { 
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgNotifQoS_nm      , RDIDbgNotifQoS_set,     1) == -2) { 
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgAdminQoS_nm      , RDIDbgAdminQoS_set,     1) == -2) { 
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIDbgNotifQueue_nm    , RDIDbgNotifQueue_set,   1) == -2) { 
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIRptChanStats_nm     , RDIRptChanStats_set,    1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIRptQSizeStats_nm    , RDIRptQSizeStats_set,   1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIRptCnctdCons_nm     , RDIRptCnctdCons_set,    1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIRptCnctdSups_nm     , RDIRptCnctdSups_set,    1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIRptCnctdFilts_nm    , RDIRptCnctdFilts_set,   1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIRptUnCnctdFilts_nm  , RDIRptUnCnctdFilts_set, 1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIRptRejects_nm       , RDIRptRejects_set,      1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIRptDrops_nm         , RDIRptDrops_set,        1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIRptNotifQoS_nm      , RDIRptNotifQoS_set,     1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIRptAdminQoS_nm      , RDIRptAdminQoS_set,     1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIRptServerQoS_nm     , RDIRptServerQoS_set,    1) == -2) {
    goto cleanup_and_return_0;
  }
  if (config->get_value(error_str, RDIRptInteractive_nm   , RDIRptInteractive_set,  1) == -2) {
    goto cleanup_and_return_0;
  }

  RDI::ClrDbgFlags();
  if (RDIDbgDaemon_set       ) RDI::AddDbgFlags(RDIDbgDaemonF        );
  if (RDIDbgFact_set         ) RDI::AddDbgFlags(RDIDbgFactF          );
  if (RDIDbgFilt_set         ) RDI::AddDbgFlags(RDIDbgFiltF          );
  if (RDIDbgChan_set         ) RDI::AddDbgFlags(RDIDbgChanF          );
  if (RDIDbgCAdm_set         ) RDI::AddDbgFlags(RDIDbgCAdmF          );
  if (RDIDbgSAdm_set         ) RDI::AddDbgFlags(RDIDbgSAdmF          );
  if (RDIDbgCPxy_set         ) RDI::AddDbgFlags(RDIDbgCPxyF          );
  if (RDIDbgSPxy_set         ) RDI::AddDbgFlags(RDIDbgSPxyF          );
  if (RDIDbgEvQ_set          ) RDI::AddDbgFlags(RDIDbgEvQF           );
  if (RDIDbgRDIEvent_set     ) RDI::AddDbgFlags(RDIDbgRDIEventF      );
  if (RDIDbgFAdmin_set       ) RDI::AddDbgFlags(RDIDbgFAdminF        ); 
  if (RDIDbgEval_set         ) RDI::AddDbgFlags(RDIDbgEvalF          ); 
  if (RDIDbgCosCPxy_set      ) RDI::AddDbgFlags(RDIDbgCosCPxyF       ); 
  if (RDIDbgCosSPxy_set      ) RDI::AddDbgFlags(RDIDbgCosSPxyF       ); 
  if (RDIDbgNotifQoS_set     ) RDI::AddDbgFlags(RDIDbgNotifQoSF      ); 
  if (RDIDbgAdminQoS_set     ) RDI::AddDbgFlags(RDIDbgAdminQoSF      ); 
  if (RDIDbgNotifQueue_set   ) RDI::AddDbgFlags(RDIDbgNotifQueueF    ); 
                        
  RDI::ClrRptFlags();
  if (RDIRptChanStats_set    ) RDI::AddRptFlags( RDIRptChanStatsF    );
  if (RDIRptQSizeStats_set   ) RDI::AddRptFlags( RDIRptQSizeStatsF   );
  if (RDIRptCnctdCons_set    ) RDI::AddRptFlags( RDIRptCnctdConsF    );
  if (RDIRptCnctdSups_set    ) RDI::AddRptFlags( RDIRptCnctdSupsF    );
  if (RDIRptCnctdFilts_set   ) RDI::AddRptFlags( RDIRptCnctdFiltsF   );
  if (RDIRptUnCnctdFilts_set ) RDI::AddRptFlags( RDIRptUnCnctdFiltsF );
  if (RDIRptRejects_set      ) RDI::AddRptFlags( RDIRptRejectsF      );
  if (RDIRptDrops_set        ) RDI::AddRptFlags( RDIRptDropsF        );
  if (RDIRptNotifQoS_set     ) RDI::AddRptFlags( RDIRptNotifQoSF     );
  if (RDIRptAdminQoS_set     ) RDI::AddRptFlags( RDIRptAdminQoSF     );
  if (RDIRptServerQoS_set    ) RDI::AddRptFlags( RDIRptServerQoSF    );
  if (RDIRptInteractive_set  ) RDI::AddRptFlags( RDIRptInteractiveF  );

  RDI::OpenDbgFile(debugLogFile);
  RDI::OpenRptFile(reportLogFile);
  // NB At this point it is OK to use RDIDbg*Log and RDIRpt*Log macros.
  // prior to this point, should use RDIErrLog or RDIOutLog.

  // Validate the ServerQoS, NotifQoS and AdminQoS Property values provided
  if (RDI_AllQoS::validate_initial_config(error_str, *config, defqos, defadm, *s_qos) == 0) {
    RDIOutLog("Invalid ServerQoS, AdminQoS, or NotifQoS Properties were provided:\n");
    goto cleanup_and_return_0;
  }

  // Create the default filter factory
  filter_factory = new FilterFactory_i("EXTENDED_TCL");
  if (! filter_factory) {
    RDIOutLog("Failed to create the default event channel factory\n");
    goto cleanup_and_return_0;
  }
  filtfactref = WRAPPED_IMPL2OREF(AttN::FilterFactory, filter_factory);

  factory = new EventChannelFactory_i(filter_factory, defqos, defadm, s_qos);
  if ( ! factory ) {
    RDIOutLog("Failed to create the default event channel factory\n");
    goto cleanup_and_return_0;
  }
  factref = WRAPPED_IMPL2OREF(AttN::EventChannelFactory, factory);

  if ( factoryIORFile ) {
    if ( ! (iorfile = fopen(factoryIORFile, "w")) ) {
      RDIOutLog("Failed to open IOR file: \"" << factoryIORFile << "\"\n");
    } else {
      char* ior_name = WRAPPED_ORB_OA::orb()->object_to_string(factref);
      fprintf(iorfile, "%s", ior_name);
      fclose(iorfile);
      delete [] ior_name;
    }
  }
  channel = factory->create_channel(channID);

  if ( ! channel ) {
    RDIOutLog("Failed to create the default event channel\n");
    goto cleanup_and_return_0;
  }
  chanref = WRAPPED_IMPL2OREF(AttN::EventChannel, channel);
  if ( channelIORFile ) {
    if ( ! (iorfile = fopen(channelIORFile, "w")) ) {
      RDIOutLog("Failed to open IOR file \"" << channelIORFile << "\"\n");
    } else {
      char* ior_name = WRAPPED_ORB_OA::orb()->object_to_string(chanref);
      fprintf(iorfile, "%s", ior_name);
      fclose(iorfile);
      delete [] ior_name;
    }
  }

  if ( useNameService ) {
    CosNaming::NamingContext_var nmcx;
    CORBA::Object_var            nmsv;
    CosNaming::Name              name;

    try {
      nmsv = WRAPPED_RESOLVE_INITIAL_REFERENCES("NameService");
      nmcx = CosNaming::NamingContext::_narrow(nmsv);
      if ( CORBA::is_nil(nmcx) ) {
	RDIOutLog("Unable to find the NameService\n");
	goto cleanup_and_return_0;
      
      }
    } catch ( CORBA::NO_RESOURCES ) {
      RDIOutLog("NO_RESOURCES exception while trying to find the NameService\n");
      goto cleanup_and_return_0;
    } catch ( CORBA::COMM_FAILURE& ex ) {
      RDIOutLog("COMM_FAILURE exception while trying to find the NameService\n");
      goto cleanup_and_return_0;
    }
    try {
      RDIDbgFactLog("Register '" << theFactoryName << "' with NameService\n");
      name.length(1);
      name[0].id   = CORBA_STRING_DUP(theFactoryName);
      name[0].kind = CORBA_STRING_DUP(theFactoryName);
      nmcx->rebind(name, factref);
    } catch ( CORBA::ORB::InvalidName& ex ) {
      RDIOutLog("InvalidName exception while registering channel factory name '" <<
		theFactoryName << "with NameService\n");
      goto cleanup_and_return_0;
    } catch ( CORBA::COMM_FAILURE& ex ) {
      RDIOutLog("COMM_FAILURE exception while registering factory with the NameService\n");
      goto cleanup_and_return_0;
    }
    try {
      RDIDbgChanLog("Register '" << theChannelName << "' with NameService\n");
      name[0].id   = CORBA_STRING_DUP(theChannelName);
      name[0].kind = CORBA_STRING_DUP(theChannelName);
      nmcx->rebind(name, chanref);
    } catch ( CORBA::ORB::InvalidName& ex ) {
      RDIOutLog("InvalidName exception while registering channel name '" <<
		theChannelName << "with NameService\n");
      goto cleanup_and_return_0;
    } catch ( CORBA::COMM_FAILURE& ex ) {
      RDIOutLog("COMM_FAILURE exception while registering channel with the NameService\n");
      goto cleanup_and_return_0;
    }
  }

  server = new RDINotifServer(config, filter_factory, filtfactref, factory, factref, channel, chanref, s_qos);
  if ( ! server ) {
    RDIOutLog("Failed to create a notifyServer\n");
    goto cleanup_and_return_0;
  }

#ifndef NO_OBJ_GC
  server->_objGCPeriod = server->_server_qos->objectGCPeriod;
  server->_gc_wait     = server->_oplockptr->add_condition();
  server->_gc_exit     = server->_oplockptr->add_condition();
  server->_gcollector  = new RDINotifServerWorker(server, &RDINotifServer::gcollect);
  RDI_AssertAllocThrowNo(server->_gcollector, "Memory allocation failed -- Thread\n");
  RDIDbgDaemonLog("server object GC thread created -- ID " << server->_gcollector->id() << '\n');
  server->_gcollector->start();
  server->_gcisactive = 1;
#endif

  return server;

 cleanup_and_return_0:
  RDIOutLog(error_str.buf()); // may be empty
  if (s_qos) {
    delete s_qos;
  }
  if (config) {
    delete config;
  }
  if (filter_factory) {
    WRAPPED_DISPOSE_IMPL(filter_factory);
  }
  if (factory) {
    factory->cleanup_and_dispose();
  }
  RDI::CleanupAll(); // in case log/report files were opened
  return 0;
}

#undef WHATFN
#define WHATFN "RDINotifServer::destroy"
void
RDINotifServer::destroy( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(server_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (_destroy_called) {
    RDIDbgDaemonLog("** RDINotifServer::destroy called twice\n");
    return;
  }
  _destroy_called = 1; // acts as a guard -- only one thread executes the following
  RDI_OPLOCK_BROADCAST;
}

#undef WHATFN
#define WHATFN "RDINotifServer::L_wait_for_destroy"
void
RDINotifServer::L_wait_for_destroy()
{
  { // introduce lock scope
    RDI_OPLOCK_SCOPE_LOCK(server_lock, WHATFN, return);
    while (!_destroy_called) {
      RDI_OPLOCK_WAIT(WHATFN);
    }
  } // end lock scope
  _destroy();
}

#undef WHATFN
#define WHATFN "RDINotifServer::_destroy"
void
RDINotifServer::_destroy()
{
  { // introduce lock scope
    RDI_OPLOCK_BUMP_SCOPE_LOCK(server_lock, WHATFN, return);
    if (_destroyed) {
      RDIDbgDaemonLog("** RDINotifServer::_destroy called twice\n");
      return;
    }
    _destroyed = 1; // acts as guard -- only one thread executes the following
    RDIDbgDaemonLog("** notifd is being destroyed\n");

#ifndef NO_OBJ_GC
    RDIDbgDaemonLog("** signal server object GC thread and wait for exit\n");
    while ( _gcisactive ) {
      _gc_wait->broadcast();
      RDI_OPLOCK_ALTCV_WAIT((*_gc_exit), WHATFN);
    }
    // no longer need these condition vars
    delete _gc_wait;   _gc_wait = 0;
    delete _gc_exit;   _gc_exit = 0;
#endif

    if ( _configp ) {
      RDIstrstream error_str; // for type error msgs
      char* factoryIORFile = 0;
      char* channelIORFile = 0;
      _configp->get_value(error_str, "FactoryIORFileName", factoryIORFile, 0);
      _configp->get_value(error_str, "ChannelIORFileName", channelIORFile, 0);
      if ( factoryIORFile ) 
	(void) remove(channelIORFile);
      if ( channelIORFile ) 
	(void) remove(channelIORFile);
      delete _configp; 
      _configp = 0;
    }
    if ( ! CORBA::is_nil(_cfactory) ) {
      CosNA::ChannelIDSeq* cids = _cfactory->get_all_channels();
      for ( CORBA::ULong ix = 0; ix < cids->length(); ix++ ) {
	CosNA::EventChannel_var chan_ptr;
	chan_ptr = _cfactory->get_event_channel( (*cids)[ix] );
	if ( ! CORBA::is_nil(chan_ptr) ) {
	  RDIDbgDaemonLog("Destroying Event Channel " << (*cids)[ix] << '\n');
	  chan_ptr->destroy();
	  RDIDbgDaemonLog("DONE Destroying Event Channel " << (*cids)[ix] << '\n');
	}
      }
      delete cids;
    }
    _cfactory = AttN::EventChannelFactory::_nil();
    _ffactory = AttN::FilterFactory::_nil();
    if (_cfactory_i) {
      RDIDbgDaemonLog("Destroying Channel Factory\n");
      _cfactory_i->cleanup_and_dispose();
      RDIDbgDaemonLog("Done Destroying Channel Factory\n");
      _cfactory_i = 0;
    }
    if (_ffactory_i) {
      RDIDbgDaemonLog("Destroying Filter Factory\n");
      _ffactory_i->cleanup_and_dispose();
      RDIDbgDaemonLog("Done Destroying Filter Factory\n");
      _ffactory_i = 0;
    }
    if (_server_qos) {
      delete _server_qos;
      _server_qos = 0;
    }
    RDI_OPLOCK_SET_DISPOSE_INFO(server_lock.dispose_info);
  } // end lock scope, server destroyed
  RDI::CleanupAll();
}

#undef WHATFN
#define WHATFN "RDINotifServer::get_filter_factory"
AttN::FilterFactory_ptr
RDINotifServer::get_filter_factory( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(server_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::FilterFactory_ptr res = WRAPPED_IMPL2OREF(AttN::FilterFactory, _ffactory_i);
  return res;
}

#undef WHATFN
#define WHATFN "RDINotifServer::get_channel_factory"
AttN::EventChannelFactory_ptr
RDINotifServer::get_channel_factory( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(server_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::EventChannelFactory_ptr res = WRAPPED_IMPL2OREF(AttN::EventChannelFactory, _cfactory_i);
  return res;
}

#undef WHATFN
#define WHATFN "RDINotifServer::get_default_channel"
AttN::EventChannel_ptr
RDINotifServer::get_default_channel( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(server_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::EventChannel_ptr res = WRAPPED_IMPL2OREF(AttN::EventChannel, _channel_i);
  return res;
}

#undef WHATFN
#define WHATFN "RDINotifServer::results_to_file"
CORBA::Boolean
RDINotifServer::results_to_file( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(server_lock, WHATFN, RDI_THROW_INV_OBJREF);
  CORBA::Boolean res = RDIRptLogIsFile;
  return res;
}

RDIstrstream& RDINotifServer::log_output(RDIstrstream& str)
{
  return str << "XXX TODO RDINotifServer::log_output\n";
}

#undef WHATFN
#define WHATFN "RDINotifServer::my_name"
AttN::NameSeq*
RDINotifServer::my_name( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(server_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::NameSeq* res = new AttN::NameSeq(_my_name);
  if ( res == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  return res;
}

#undef WHATFN
#define WHATFN "RDINotifServer::child_names"
AttN::NameSeq*
RDINotifServer::child_names( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(server_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::NameSeq* names = new AttN::NameSeq;
  if ( names == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  names->length(2);
  (*names)[0] = (const char*)"chanfact";
  (*names)[1] = (const char*)"filtfact";
  return names;
}

#undef WHATFN
#define WHATFN "RDINotifServer::children"
AttN::IactSeq*
RDINotifServer::children(CORBA::Boolean only_cleanup_candidates WRAPPED_IMPLARG )
{
  // All factories are cleanup candidates
  RDI_OPLOCK_SCOPE_LOCK(server_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::IactSeq* ren = new AttN::IactSeq;
  if ( ren == (AttN::IactSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  ren->length(2);
  (*ren)[0] = WRAPPED_IMPL2OREF(AttN::Interactive, _cfactory_i);
  (*ren)[1] = WRAPPED_IMPL2OREF(AttN::Interactive, _ffactory_i);
  return ren;
}

CORBA::Boolean
RDINotifServer::safe_cleanup( WRAPPED_IMPLARG_VOID )
{
  return 0; // not destroyed
}

#undef WHATFN
#define WHATFN "RDINotifServer::get_server_props"
AttN::ServerProperties*
RDINotifServer::get_server_props( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(server_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::ServerProperties* res = _server_qos->to_server_props();
  return res;
}

#undef WHATFN
#define WHATFN "RDINotifServer::set_server_props"
void
RDINotifServer::set_server_props(const AttN::ServerProperties& props  WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(server_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if (props.length() == 0) return;
  CosN::PropertyErrorSeq error;
  RDIstrstream str;
  if ( ! _server_qos->validate(str, props, error, 0) ) {
    if (str.len() > 0) {
      RDIRptForceLog(str.buf());
    }
    throw CosN::UnsupportedAdmin(error);
  }
  _server_qos->from_server_props(props);
  if (_outgoingTimeout != _server_qos->outgoingTimeout || _incomingTimeout != _server_qos->incomingTimeout) {
    if (_outgoingTimeout != _server_qos->outgoingTimeout) {
      _outgoingTimeout = _server_qos->outgoingTimeout;
      WRAPPED_ORB_SET_CLIENT_TIMEOUT(_outgoingTimeout);
      RDIDbgDaemonLog("Called WRAPPED_ORB_SET_CLIENT_TIMEOUT(" << _outgoingTimeout << ")\n");
    }
    if (_incomingTimeout != _server_qos->incomingTimeout) {
      _incomingTimeout = _server_qos->incomingTimeout;
      WRAPPED_ORB_SET_SERVER_TIMEOUT(_incomingTimeout);
      RDIDbgDaemonLog("Called WRAPPED_ORB_SET_SERVER_TIMEOUT(" << _incomingTimeout << ")\n");
    }
    CORBA::ULong curScanGran = WRAPPED_ORB_GET_SCAN_GRANULARITY();
    if (_outgoingTimeout || _incomingTimeout) { // Change the scan granularity to something reasonable for the timeouts
      // scanGran = min non-zero timeout (milliseconds)
      CORBA::ULong scanGran = (_outgoingTimeout && _outgoingTimeout < _incomingTimeout) ? _outgoingTimeout : _incomingTimeout;
      // convert scanGran to nearest NON-ZERO # of seconds
      scanGran = (scanGran < 1000) ? 1 : (scanGran + 500)/1000;
      if (curScanGran == 0 || scanGran < curScanGran) {
	WRAPPED_ORB_SET_SCAN_GRANULARITY(scanGran);
	RDIDbgDaemonLog("Called WRAPPED_ORB_SET_SCAN_GRANULARITY(" << scanGran << ")\n");
      }
    } else {
      // No longer need a scan granularity setting that works with the timeouts, so set it back to its original value
      if (curScanGran != _origScanGran) {
	WRAPPED_ORB_SET_SCAN_GRANULARITY(_origScanGran);
	RDIDbgDaemonLog("Called WRAPPED_ORB_SET_SCAN_GRANULARITY(" << _origScanGran << ") -- restoring original value\n");
      }
    }
  }
  if (RDIRptServerQoS) {
    RDIRptLogger(l, RDIRptServerQoS_nm);
    l.str << _my_name << ": ServerQoS param(s) modified as follows\n";
    for (unsigned int i = 0; i < props.length(); i++) {
      l.str << "  " << props[i].name << " set to "; RDI_pp_any(l.str, props[i].value); l.str << '\n';
    }
    l.str << '\n';
  }
  // inform channel factory of relevant qos changed
  // it will inform all the channels (this will change once more
  // processing moves up to server level)
  _cfactory_i->server_qos_changed();
}

#undef WHATFN
#define WHATFN "RDINotifServer::do_set_command"
CORBA::Boolean
RDINotifServer::do_set_command(RDIstrstream& str, RDIParseCmd& p)
{
  CosN::QoSProperties    n_qos;
  CosN::AdminProperties  a_qos;
  AttN::ServerProperties s_qos;
  CosN::PropertyErrorSeq eseq;
  CORBA::Boolean         sqos_valid;
  CORBA::Boolean         success;

  success = RDI_AllQoS::parse_set_command(str, p, RDI_NFSERVER, n_qos, a_qos, s_qos);
  if (success && s_qos.length() > 0) {
    { // introduce lock scope
      RDI_OPLOCK_SCOPE_LOCK(server_lock, WHATFN, RDI_THROW_INV_OBJREF);
      sqos_valid = _server_qos->validate(str, s_qos, eseq, 0);
    } // end lock scope
    if (sqos_valid) {
      try {
	set_server_props(s_qos);
      } catch (AttN::UnsupportedServerProp& e) {
	str << "\nThe following ServerQoS Property Settings are invalid:\n";
	RDI_describe_prop_errors(str, e.server_prop_err);
	str << '\n';
	success = 0;
      } catch (CORBA::INV_OBJREF) { throw; }
      if (success) {
	str << '\n';
	for (unsigned int i = 0; i < s_qos.length(); i++) {
	  str << s_qos[i].name << " set to "; RDI_pp_any(str, s_qos[i].value); str << '\n';
	}
	str << "\nSome properties updated successfully.  Current settings:\n\n";
	out_all_config(str, "server");
      }
    } else {
      str << "\nThe following ServerQOS Property Settings are invalid:\n";
      RDI_describe_prop_errors(str, eseq);
      str << '\n';
      success = 0;
    }
  }
  return success;
}

CORBA::Boolean
RDINotifServer::do_go_command(RDIstrstream& str, RDIParseCmd& p,
			      CORBA::Boolean& target_changed,
			      AttN_Interactive_outarg next_target)
{
  CORBA::Boolean success = 1;
  AttN::Interactive_ptr targ1 = AttN::Interactive::_nil();
  AttN::Interactive_ptr targ2 = AttN::Interactive::_nil();
  CORBA::Boolean targ1_set = 0, targ2_set = 0;
  target_changed = 0;
  char* go_targ = CORBA_STRING_DUP(p.argv[1]);
  char* rest_go_targ = RDI_STRCHR(go_targ, '.');
  if (rest_go_targ) {
    *rest_go_targ = '\0';
    rest_go_targ++;
  }
  if (RDI_STR_EQ_I(go_targ, "chanfact")) {
    targ1 = WRAPPED_IMPL2OREF(AttN::EventChannelFactory, _cfactory_i);
    targ1_set = 1;
    str << "\nomniNotify: new target ==> chanfact\n";
  } else if (RDI_STR_EQ_I(go_targ, "filtfact")) {
    targ1 = WRAPPED_IMPL2OREF(AttN::FilterFactory, _ffactory_i);
    targ1_set = 1;
    str << "\nomniNotify: new target ==> filtfact\n";
  }
  if (targ1_set == 0) {
    str << "Invalid target " << p.argv[1] << " : " <<  " must be chanfact or filtfact\n";
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
RDINotifServer::out_commands(RDIstrstream& str)
{
  str << "Server commands:\n"
      << "  cleanup filters   : destroy all filters that have zero callbacks\n"
      << "                        (normally means a client forgot to destroy a filter;\n"
      << "                         sometimes filter not yet added to a proxy or admin)\n"
      << "  cleanup proxies   : destroy all proxies of all channels that are not connected\n"
      << "  cleanup admins    : destroy all admins  of all channels that have no connected proxies\n"
      << "  cleanup both      : combines 'cleanup proxies' and 'cleanup admins'\n"
      << "  cleanup all       : combines 'cleanup both' and 'cleanup filters'\n"
      << "  debug  [ <targ> ] : show debugging information for <targ>\n"
      << "  config [ <targ> ] : show configuration info for <targ>\n"
      << "  stats  [ <targ> ] : show statistics for <targ>\n"
      << "    (<targ> can be default, server, chans, filts, or all)\n"
      << "  set <name1> <value1> [ <name2> <value2> ... ]\n"
      << "                    : set ServerQoS property name1 to value1, name2 to value2, etc.\n"
      << "                      (use 'config' to see all ServerQoS properties)\n"
      << "\n";
}

char*
RDINotifServer::do_command(const char* cmnd, CORBA::Boolean& success,
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
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "stats")) {
    success = out_all_stats(str, "chans");
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "stats")) {
    success = out_all_stats(str, p.argv[1]);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "debug")) {
    success = out_all_debug_info(str, "chans");
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "debug")) {
    success = out_all_debug_info(str, p.argv[1]);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "config")) {
    success = out_all_config(str, "server");
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "config")) {
    success = out_all_config(str, p.argv[1]);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "flags")) {
    out_flags(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "up")) {
    str << "\nomniNotify: server is the top target\n";
  } else if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "go")) {
    success = do_go_command(str, p, target_changed, next_target);
  } else if ((p.argc >= 1) && RDI_STR_EQ_I(p.argv[0], "set")) {
    success = do_set_command(str, p);
  } else if ((p.argc == 2) &&
	     RDI_STR_EQ_I(p.argv[0], "cleanup") && RDI_STR_EQ_I(p.argv[1], "filters")) {
    _ffactory_i->cleanup_all(str);
  } else if ((p.argc == 2) &&
	     RDI_STR_EQ_I(p.argv[0], "cleanup") && RDI_STR_EQ_I(p.argv[1], "proxies")) {
    _cfactory_i->cleanup_all(str, 0, 1);
  } else if ((p.argc == 2) &&
	     RDI_STR_EQ_I(p.argv[0], "cleanup") && RDI_STR_EQ_I(p.argv[1], "admins")) {
    _cfactory_i->cleanup_all(str, 1, 0);
  } else if ((p.argc == 2) &&
	     RDI_STR_EQ_I(p.argv[0], "cleanup") && RDI_STR_EQ_I(p.argv[1], "both")) {
    _cfactory_i->cleanup_all(str, 1, 1);
  } else if ((p.argc == 2) &&
	     RDI_STR_EQ_I(p.argv[0], "cleanup") && RDI_STR_EQ_I(p.argv[1], "all")) {
    // cleanup channels first; may allow more filters to be destroyed
    _cfactory_i->cleanup_all(str, 1, 1);
    _ffactory_i->cleanup_all(str);
  } else if ((p.argc == 1) && 
	     ((RDI_STRLEN(p.argv[0]) > 1) && ((p.argv[0][0] == '+') || (p.argv[0][0] == '-'))) ) {
    success = flag_change(str, p.argv[0]);
  } else {
    success = 0;
    str << "\nomniNotify: Invalid command: " << cmnd << "\n\n";
  }
  RDIRptInteractiveLog(_my_name << " received command: " << cmnd << "\nResult:\n" << str.buf());
  // this is the only safe way to return a string?
  return CORBA_STRING_DUP(str.buf());
}

CORBA::Boolean
RDINotifServer::out_all_stats(RDIstrstream& str, const char* targ)
{
  if (RDI_STR_EQ_I(targ, "all") || RDI_STR_EQ_I(targ, "chans")) {
    _cfactory_i->out_all_stats(str);
  } else if (RDI_STR_EQ_I(targ, "filts") || RDI_STR_EQ_I(targ, "server")) {
    str << "Currently, no stats for target " << targ << '\n';
  } else {
    str << "Bad target \"" << targ << "\" : currently, valid stats targets are 'chans' and 'all'\n";
    return 0;
  }
  return 1;
}

CORBA::Boolean
RDINotifServer::out_all_debug_info(RDIstrstream& str, const char* targ)
{
  if (RDI_STR_EQ_I(targ, "all") || RDI_STR_EQ_I(targ, "chans")) {
    _cfactory_i->out_all_debug_info(str);
  } else if (RDI_STR_EQ_I(targ, "filts") || RDI_STR_EQ_I(targ, "server")) {
    str << "Currently, no debug info for target " << targ << '\n';
  } else {
    str << "Bad target \"" << targ << "\" : currently, valid debug targets are 'chans' and 'all'\n";
    return 0;
  }
  return 1;
}

CORBA::Boolean
RDINotifServer::out_all_config(RDIstrstream& str, const char* targ)
{
  if (RDI_STR_EQ_I(targ, "all")) {
    out_server_config(str);
    _cfactory_i->out_default_config(str);
    _cfactory_i->out_all_config(str);
  } else if (RDI_STR_EQ_I(targ, "default")) {
    _cfactory_i->out_default_config(str);
  } else if (RDI_STR_EQ_I(targ, "server")) {
    out_server_config(str);
  } else if (RDI_STR_EQ_I(targ, "chans")) {
    _cfactory_i->out_all_config(str);
  } else if (RDI_STR_EQ_I(targ, "filts")) {
    str << "Currently, no config info for target " << targ << '\n';
  } else {
    str << "Bad target \"" << targ << "\" : currently, valid config targets are 'server', 'chans' and 'all'\n";
    return 0;
  }
  return 1;
}

void
RDINotifServer::out_server_config(RDIstrstream& str)
{
  str << "======================================================================\n";
  str << "Server Configuration: Server-Level Params\n";
  str << "======================================================================\n";
  _server_qos->log_output(str);
}

void
RDINotifServer::out_flags(RDIstrstream& str)
{
  str << "Current Report Flag Values:\n\n\t";
  str.setw(25); str << RDIRptChanStats_nm; if (RDIRptChanStats) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIRptQSizeStats_nm; if (RDIRptQSizeStats) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIRptCnctdCons_nm; if (RDIRptCnctdCons) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIRptCnctdSups_nm; if (RDIRptCnctdSups) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIRptCnctdFilts_nm; if (RDIRptCnctdFilts) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIRptUnCnctdFilts_nm; if (RDIRptUnCnctdFilts) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIRptRejects_nm; if (RDIRptRejects) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIRptDrops_nm; if (RDIRptDrops) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIRptNotifQoS_nm; if (RDIRptNotifQoS) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIRptAdminQoS_nm; if (RDIRptAdminQoS) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIRptServerQoS_nm; if (RDIRptServerQoS) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIRptInteractive_nm; if (RDIRptInteractive) str << " true\n\t"; else str << " false\n\t";

  str << "\nCurrent Debug Flag Values:\n\n\t";
  str.setw(25); str << RDIDbgDaemon_nm; if (RDIDbgDaemon) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgFact_nm; if (RDIDbgFact) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgFilt_nm; if (RDIDbgFilt) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgChan_nm; if (RDIDbgChan) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgCAdm_nm; if (RDIDbgCAdm) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgSAdm_nm; if (RDIDbgSAdm) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgCPxy_nm; if (RDIDbgCPxy) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgSPxy_nm; if (RDIDbgSPxy) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgEvQ_nm; if (RDIDbgEvQ) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgRDIEvent_nm; if (RDIDbgRDIEvent) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgFAdmin_nm; if (RDIDbgFAdmin) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgEval_nm; if (RDIDbgEval) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgCosCPxy_nm; if (RDIDbgCosCPxy) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgCosSPxy_nm; if (RDIDbgCosSPxy) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgNotifQoS_nm; if (RDIDbgNotifQoS) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgAdminQoS_nm; if (RDIDbgAdminQoS) str << " true\n\t"; else str << " false\n\t";
  str.setw(25); str << RDIDbgNotifQueue_nm; if (RDIDbgNotifQueue) str << " true\n\t"; else str << " false\n\n";
}

// flag_change expects one of the following in a string arg:
//   +<flag-name>
//   -<flag-name>
//   +alldebug
//   +allreport
//   -alldebug
//   -allreport
// It returns 0 on error, 1 for success
CORBA::Boolean
RDINotifServer::flag_change(RDIstrstream& str, const char* change)
{
  if ((RDI_STRLEN(change) < 2) || ((change[0] != '+') && (change[0] != '-'))) {
    str << "Invalid command: " << change << '\n';
    return 0;
  }
  CORBA::Boolean addflag = (change[0] == '+') ? 1 : 0;
  const char* addflagstr = (change[0] == '+') ? "true" : "false";
  const char* flag = change + 1;
  CORBA::Boolean alldbg = (RDI_STR_EQ_I(flag, "alldebug")) ? 1 : 0;
  CORBA::Boolean allrpt = (RDI_STR_EQ_I(flag, "allreport")) ? 1 : 0;

  CORBA::Boolean found_dbg_flag = 0;
  CORBA::Boolean found_rpt_flag = 0;
  // DEBUG FLAGS
  if (alldbg || RDI_STR_EQ(flag, RDIDbgDaemon_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgDaemonF); else RDI::RemDbgFlags(RDIDbgDaemonF);
    str << "  Debug Flag " << RDIDbgDaemon_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgFact_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgFactF); else RDI::RemDbgFlags(RDIDbgFactF);
    str << "  Debug Flag " << RDIDbgFact_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgFilt_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgFiltF); else RDI::RemDbgFlags(RDIDbgFiltF);
    str << "  Debug Flag " << RDIDbgFilt_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgChan_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgChanF); else RDI::RemDbgFlags(RDIDbgChanF);
    str << "  Debug Flag " << RDIDbgChan_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgCAdm_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgCAdmF); else RDI::RemDbgFlags(RDIDbgCAdmF);
    str << "  Debug Flag " << RDIDbgCAdm_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgSAdm_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgSAdmF); else RDI::RemDbgFlags(RDIDbgSAdmF);
    str << "  Debug Flag " << RDIDbgSAdm_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgCPxy_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgCPxyF); else RDI::RemDbgFlags(RDIDbgCPxyF);
    str << "  Debug Flag " << RDIDbgCPxy_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgSPxy_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgSPxyF); else RDI::RemDbgFlags(RDIDbgSPxyF);
    str << "  Debug Flag " << RDIDbgSPxy_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgEvQ_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgEvQF); else RDI::RemDbgFlags(RDIDbgEvQF);
    str << "  Debug Flag " << RDIDbgEvQ_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgRDIEvent_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgRDIEventF); else RDI::RemDbgFlags(RDIDbgRDIEventF);
    str << "  Debug Flag " << RDIDbgRDIEvent_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgFAdmin_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgFAdminF); else RDI::RemDbgFlags(RDIDbgFAdminF);
    str << "  Debug Flag " << RDIDbgFAdmin_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgEval_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgEvalF); else RDI::RemDbgFlags(RDIDbgEvalF);
    str << "  Debug Flag " << RDIDbgEval_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgCosCPxy_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgCosCPxyF); else RDI::RemDbgFlags(RDIDbgCosCPxyF);
    str << "  Debug Flag " << RDIDbgCosCPxy_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgCosSPxy_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgCosSPxyF); else RDI::RemDbgFlags(RDIDbgCosSPxyF);
    str << "  Debug Flag " << RDIDbgCosSPxy_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgNotifQoS_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgNotifQoSF); else RDI::RemDbgFlags(RDIDbgNotifQoSF);
    str << "  Debug Flag " << RDIDbgNotifQoS_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgAdminQoS_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgAdminQoSF); else RDI::RemDbgFlags(RDIDbgAdminQoSF);
    str << "  Debug Flag " << RDIDbgAdminQoS_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  if (alldbg || RDI_STR_EQ(flag, RDIDbgNotifQueue_nm)) {
    if (addflag) RDI::AddDbgFlags(RDIDbgNotifQueueF); else RDI::RemDbgFlags(RDIDbgNotifQueueF);
    str << "  Debug Flag " << RDIDbgNotifQueue_nm << " set to " << addflagstr << '\n';
    found_dbg_flag = 1;
  }
  // REPORT FLAGS
  if (allrpt || RDI_STR_EQ(flag, RDIRptChanStats_nm)) {
    if (addflag) RDI::AddRptFlags(RDIRptChanStatsF); else RDI::RemRptFlags(RDIRptChanStatsF);
    str << "  Report Flag " << RDIRptChanStats_nm << " set to " << addflagstr << '\n';
    found_rpt_flag = 1;
  }
  if (allrpt || RDI_STR_EQ(flag, RDIRptQSizeStats_nm)) {
    if (addflag) RDI::AddRptFlags(RDIRptQSizeStatsF); else RDI::RemRptFlags(RDIRptQSizeStatsF);
    str << "  Report Flag " << RDIRptQSizeStats_nm << " set to " << addflagstr << '\n';
    found_rpt_flag = 1;
  }
  if (allrpt || RDI_STR_EQ(flag, RDIRptCnctdCons_nm)) {
    if (addflag) RDI::AddRptFlags(RDIRptCnctdConsF); else RDI::RemRptFlags(RDIRptCnctdConsF);
    str << "  Report Flag " << RDIRptCnctdCons_nm << " set to " << addflagstr << '\n';
    found_rpt_flag = 1;
  }
  if (allrpt || RDI_STR_EQ(flag, RDIRptCnctdSups_nm)) {
    if (addflag) RDI::AddRptFlags(RDIRptCnctdSupsF); else RDI::RemRptFlags(RDIRptCnctdSupsF);
    str << "  Report Flag " << RDIRptCnctdSups_nm << " set to " << addflagstr << '\n';
    found_rpt_flag = 1;
  }
  if (allrpt || RDI_STR_EQ(flag, RDIRptCnctdFilts_nm)) {
    if (addflag) RDI::AddRptFlags(RDIRptCnctdFiltsF); else RDI::RemRptFlags(RDIRptCnctdFiltsF);
    str << "  Report Flag " << RDIRptCnctdFilts_nm << " set to " << addflagstr << '\n';
    found_rpt_flag = 1;
  }
  if (allrpt || RDI_STR_EQ(flag, RDIRptUnCnctdFilts_nm)) {
    if (addflag) RDI::AddRptFlags(RDIRptUnCnctdFiltsF); else RDI::RemRptFlags(RDIRptUnCnctdFiltsF);
    str << "  Report Flag " << RDIRptUnCnctdFilts_nm << " set to " << addflagstr << '\n';
    found_rpt_flag = 1;
  }
  if (allrpt || RDI_STR_EQ(flag, RDIRptRejects_nm)) {
    if (addflag) RDI::AddRptFlags(RDIRptRejectsF); else RDI::RemRptFlags(RDIRptRejectsF);
    str << "  Report Flag " << RDIRptRejects_nm << " set to " << addflagstr << '\n';
    found_rpt_flag = 1;
  }
  if (allrpt || RDI_STR_EQ(flag, RDIRptDrops_nm)) {
    if (addflag) RDI::AddRptFlags(RDIRptDropsF); else RDI::RemRptFlags(RDIRptDropsF);
    str << "  Report Flag " << RDIRptDrops_nm << " set to " << addflagstr << '\n';
    found_rpt_flag = 1;
  }
  if (allrpt || RDI_STR_EQ(flag, RDIRptNotifQoS_nm)) {
    if (addflag) RDI::AddRptFlags(RDIRptNotifQoSF); else RDI::RemRptFlags(RDIRptNotifQoSF);
    str << "  Report Flag " << RDIRptNotifQoS_nm << " set to " << addflagstr << '\n';
    found_rpt_flag = 1;
  }
  if (allrpt || RDI_STR_EQ(flag, RDIRptAdminQoS_nm)) {
    if (addflag) RDI::AddRptFlags(RDIRptAdminQoSF); else RDI::RemRptFlags(RDIRptAdminQoSF);
    str << "  Report Flag " << RDIRptAdminQoS_nm << " set to " << addflagstr << '\n';
    found_rpt_flag = 1;
  }
  if (allrpt || RDI_STR_EQ(flag, RDIRptServerQoS_nm)) {
    if (addflag) RDI::AddRptFlags(RDIRptServerQoSF); else RDI::RemRptFlags(RDIRptServerQoSF);
    str << "  Report Flag " << RDIRptServerQoS_nm << " set to " << addflagstr << '\n';
    found_rpt_flag = 1;
  }
  if (allrpt || RDI_STR_EQ(flag, RDIRptInteractive_nm)) {
    if (addflag) RDI::AddRptFlags(RDIRptInteractiveF); else RDI::RemRptFlags(RDIRptInteractiveF);
    str << "  Report Flag " << RDIRptInteractive_nm << " set to " << addflagstr << '\n';
    found_rpt_flag = 1;
  }
  // Not a legal flag?
  if ((found_dbg_flag == 0) && (found_rpt_flag == 0)) {
    str << "\nInvalid flag command: " << change << '\n';
    return 0;
  }
  return 1;
}

CORBA::Boolean
RDINotifServer::is_startup_prop(const char* p)
{
  return (RDI_STR_EQ(p, "ChannelFactoryName") ||
	  RDI_STR_EQ(p, "DefaultChannelName") ||
	  RDI_STR_EQ(p, "FactoryIORFileName") ||
	  RDI_STR_EQ(p, "ChannelIORFileName") ||
	  RDI_STR_EQ(p, "DebugLogFile") ||
	  RDI_STR_EQ(p, "ReportLogFile") ||
	  RDI_STR_EQ(p, "DebugDaemon") ||
	  RDI_STR_EQ(p, "DebugChannelFactory") ||
	  RDI_STR_EQ(p, "DebugFilter") ||
	  RDI_STR_EQ(p, "DebugChannel") ||
	  RDI_STR_EQ(p, "DebugConsumerAdmin") ||
	  RDI_STR_EQ(p, "DebugSupplireAdmin") ||
	  RDI_STR_EQ(p, "DebugConsumerProxy") ||
	  RDI_STR_EQ(p, "DebugSupplierProxy") ||
	  RDI_STR_EQ(p, "DebugEventQueue") ||
	  RDI_STR_EQ(p, "DebugRDIEvent") ||
	  RDI_STR_EQ(p, "DebugFilterAdmin") ||
	  RDI_STR_EQ(p, "DebugFilterEval") ||
	  RDI_STR_EQ(p, "DebugCosConsumerProxies") ||
	  RDI_STR_EQ(p, "DebugCosSupplierProxies") ||
	  RDI_STR_EQ(p, "DebugNotifQoS") ||
	  RDI_STR_EQ(p, "DebugAdminQoS") ||
	  RDI_STR_EQ(p, "DebugNotifQueue") ||
	  RDI_STR_EQ(p, "ReportChannelStats") ||
	  RDI_STR_EQ(p, "ReportQueueSizeStats") ||
	  RDI_STR_EQ(p, "ReportConnectedConsumers") ||
	  RDI_STR_EQ(p, "ReportConnectedSuppliers") ||
	  RDI_STR_EQ(p, "ReportConnectedFilters") ||
	  RDI_STR_EQ(p, "ReportUnconnectedFilters") ||
	  RDI_STR_EQ(p, "ReportEventRejections") ||
	  RDI_STR_EQ(p, "ReportEventDrops") ||
	  RDI_STR_EQ(p, "ReportNotifQoS") ||
	  RDI_STR_EQ(p, "ReportAdminQoS") ||
	  RDI_STR_EQ(p, "ReportServerQoS"));
}

#undef WHATFN
#define WHATFN "RDINotifServer::gcollect"

#ifndef NO_OBJ_GC
void
RDINotifServer::gcollect()
{
  RDI_LocksHeld    held = { 0 };
  unsigned long    tid = TW_ID();
  RDI_TimeT        curtime;

  while ( 1 ) {
    unsigned long secs, nanosecs;

    { // introduce bumped lock scope
      RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(server_lock, held.server, WHATFN);
      if (!held.server) {
	RDIDbgForceLog("   - GC thread " << tid << " for server exits ABNORMALLY: ** unexpected acquire failure **\n");
	_gcisactive = 0;
	_gc_exit->broadcast();
	goto gcollect_exit;
      }
      if ( _destroyed ) {
	RDIDbgDaemonLog("   - GC thread " << tid << " for server exits\n");
	_gcisactive = 0;
	_gc_exit->broadcast();
	goto gcollect_exit;
      }
      _objGCPeriod = _server_qos->objectGCPeriod;
    redo_wait:
      CORBA::ULong gcPeriod = _objGCPeriod;
      if (gcPeriod == 0) {
	gcPeriod = 60 * 60 * 24 * 365; // 1 year
      }
      TW_GET_TIME(&secs, &nanosecs, gcPeriod, 0);
      RDI_OPLOCK_ALTCV_TIMEDWAIT((*_gc_wait), secs, nanosecs, WHATFN);
      if ( _destroyed ) {
	RDIDbgDaemonLog("   - GC thread " << tid << " for server exits\n");
	_gcisactive = 0;
	_gc_exit->broadcast();
	goto gcollect_exit;
      }

      // if we were woken due to a change in _server_qos->objectGCPeriod and the period
      // just got longer (or was set to 0), redo the wait before attempting GC
      if (_server_qos->objectGCPeriod == 0 ||
	  _server_qos->objectGCPeriod > _objGCPeriod) {
	_objGCPeriod = _server_qos->objectGCPeriod;
	goto redo_wait;
      }
      CORBA::ULong deadFilter = _server_qos->deadFilterInterval;
      if (deadFilter) { // filter GC enabled
	{ // temp lock release scope
	  RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.server, WHATFN);

	  curtime.set_curtime();
	  // get classes Filter_i and MappingFilter_i to do filter object gc
	  Filter_i::obj_gc_all(curtime, deadFilter);
#if 0
	  MapppingFilter_i::obj_gc_all(curtime, deadFilter);
#endif
	} // end temp lock release scope
      }

      if (!held.server) {
	RDI_Fatal("RDINotifServer::gcollect [**unexpected REACQUIRE failure**]\n");
      }
      if ( _destroyed ) {
	RDIDbgDaemonLog("   - GC thread " << tid << " for server exits\n");
	_gcisactive = 0;
	_gc_exit->broadcast();
	goto gcollect_exit;
      }

    } // end lock scope
  } // end while loop

 gcollect_exit:
  TW_EXIT();
}
#endif
