// -*- Mode: C++; -*-
//                              File      : omniorb_boa_wrappers_impl.h
//                              Package   : omniNotify-Library
//                              Created on: 30-Oct-2000
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
// Description:
//    wrappers hide BOA/POA differences 
 
#ifndef __OMNIORB_BOA_WRAPPERS_IMPL_H__
#define __OMNIORB_BOA_WRAPPERS_IMPL_H__

#include <stdio.h>
#include "omniorb_boa_wrappers.h"

CORBA::ORB_ptr WRAPPED_ORB_OA::_orb = 0;
CORBA::BOA_ptr WRAPPED_ORB_OA::_boa = 0;
CORBA::Boolean WRAPPED_ORB_OA::_invoked_impl_is_ready = 0;
CORBA::Boolean WRAPPED_ORB_OA::_one_call_per_connection = 1;
CORBA::ULong WRAPPED_ORB_OA::_max_giop_connections = 0;
#ifdef __OMNIORB4__
DynamicAny::DynAnyFactory_var WRAPPED_ORB_OA::_dynany_fac = DynamicAny::DynAnyFactory::_nil();
#endif

void WRAPPED_ORB_OA::init(int& argc, char** argv) {
  if (_max_giop_connections == 0) {
    _max_giop_connections = 10;
  }
#if defined(__OMNIORB3__)
  omniORB::maxTcpConnectionPerServer = _max_giop_connections;
  _orb = CORBA::ORB_init(argc, argv, ORB_INIT_NAME);
#elif defined(__OMNIORB4__)
  char maxgiopcon[100];
  sprintf(maxgiopcon, "%lu", (unsigned long)_max_giop_connections);
  const char* onepercon = _one_call_per_connection ? "1" : "0";
  const char* options[][2] = { { "maxGIOPConnectionPerServer", (const char*)maxgiopcon }, 
			       { "oneCallPerConnection", onepercon }, 
			       { (const char*)0, (const char*)0 } };
  _orb = CORBA::ORB_init(argc, argv, ORB_INIT_NAME, options);
#endif
  _boa = _orb->BOA_init(argc,argv, BOA_INIT_NAME);
  _invoked_impl_is_ready = 0;
#if defined(__OMNIORB4__)
  CORBA::Object_var obj2 = _orb->resolve_initial_references("DynAnyFactory");
  _dynany_fac = DynamicAny::DynAnyFactory::_narrow(obj2);
#endif
}

void WRAPPED_ORB_OA::cleanup() {
  if (_boa) _boa->destroy();
  if (_orb) _orb->destroy();
  _boa = 0;
  _orb = 0;
  _invoked_impl_is_ready = 0;
#ifdef __OMNIORB4__
  _dynany_fac = DynamicAny::DynAnyFactory::_nil();
#endif
}

void WRAPPED_ORB_OA::activate_oas() {
  if (!_invoked_impl_is_ready) {
    _boa->impl_is_ready(0,1);
    _invoked_impl_is_ready = 1;
  }
}

void WRAPPED_ORB_OA::run() {
  activate_oas();
  _orb->run();
}

void WRAPPED_ORB_OA::shutdown(CORBA::Boolean wait_for_completion) {
  _orb->shutdown(wait_for_completion);
}


#endif
