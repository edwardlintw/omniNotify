// -*- Mode: C++; -*-
//                              File      : omniNotify.h
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
//    the entry point to the omniNotify library
//
 
#ifndef __OMNI_NOTIFY_H__
#define __OMNI_NOTIFY_H__

#ifndef COS_USES_BOA
#  ifdef __MICO__
#    include <mico/AttNotification.h>
#  else
#    include "COS/AttNotification.hh"
#  endif
#else
#  ifdef __MICO__
#    error "mico boa use not supported"
#  else
#    include "COS/BOA/AttNotification.hh"
#  endif
#endif

// ---------------------------------------------------------------- //
// The following class exposes the functionality provided by the    //
// omniNotify library, i.e., creation of an AttNotification::Server //
// ---------------------------------------------------------------- //

#define OMNINOTIFY_VERSION "2.1"

class omniNotify {
public:
  // The first time omniNotify::init_server is called with valid arguments,
  // it initializes the omniNotify server.
  // On subsequent calls, init_server ignores the arguments and
  // simply behaves like get_server (only one server is supported per process).
  //
  // On initialization, the server creates an event channel factory and a
  // default channel, configured according to defaults and command line arguments.
  //
  // By default, these objects are named 'ChannelFactory' and 
  // 'EventChannel', respectively, and are registered with the
  // default NameService.
  //
  // Supported command line arguments include:
  // 
  //	-n      : do NOT register the event channel factory and 
  //              event channel with the NameService 
  //    -c name : use the configuration file 'name'
  // 
  // As well as arguments of the form:
  // 
  //    -Dparam_name=param_value
  // 
  // Here, 'param_name' must be a valid parameter name.  A list of
  // valid names can be found in 'channel.cfg', a configuration
  // file provided with the distribution.
  // 
  // ** Before calling init_server, the ORB and BOA/or/POA
  //    must have already been initialized using the
  //          WRAPPED_ORB_OA::init(argc, argv);
  //    call from corba_wrappers.h
  // 
  // init_server returns a reference to the resulting AttNotification::Server
  // (or a nil reference on error).

  static AttNotification::Server_ptr init_server(int& argc, char** argv);

  // get_server returns a reference to the AttNotification::Server
  // (or a nil reference if init_server has not yet been used to
  // initialize the server).
  static AttNotification::Server_ptr get_server();

  // wait for the server to be destroyed (via a destroy call)
  static void wait_for_destroy();
};

#endif
