// -*- Mode: C++; -*-
//                              File      : omniorb_poa_wrappers.h
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
//    Wrappers hide BOA/POA differences 
//    See comments in corba_wrappers.h
 
#ifndef __OMNIORB_POA_WRAPPERS_H__
#define __OMNIORB_POA_WRAPPERS_H__

// N.B. We switched (July 2001) to using RefCountServantBase

#include "RDIsysdep.h"
#include "omniorb_common_wrappers.h"

class WRAPPED_ORB_OA {
public:
  static void init(int& argc, char** argv);
  static void cleanup();
  static void activate_oas();
  static void run();
  static void shutdown(CORBA::Boolean wait_for_completion);
  // the approved way to access the ORB reference:
  static CORBA::ORB_ptr orb() { return _orb; }
  // this state is public but should not be used except indirectly
  // by using the wrapper macros
  // (it is public so that the macros can access it)
  static _nfy_attr CORBA::ORB_ptr                 _orb;
  static _nfy_attr PortableServer::POA_ptr        _poa;
  static _nfy_attr CORBA::Boolean                 _poa_activated;
  static _nfy_attr CORBA::Boolean                 _one_call_per_connection;
  static _nfy_attr CORBA::ULong                   _max_giop_connections;
#ifdef __OMNIORB4__
  static _nfy_attr DynamicAny::DynAnyFactory_var  _dynany_fac;
#endif
};

// With refcounting POA, using _remove_ref enables POA cleanup of servant
#define WRAPPED_REGISTER_IMPL(implref) \
  do { PortableServer::ObjectId_var obv = WRAPPED_ORB_OA::_poa->activate_object(implref); implref->_remove_ref(); } while (0)

#ifdef DBG_POA_REFCOUNT
#define WRAPPED_REGISTER_IMPL2(implref, nm) \
  do { \
       implref->_setServantName(nm); \
       PortableServer::ObjectId_var obv = WRAPPED_ORB_OA::_poa->activate_object(implref); implref->_remove_ref(); \
       implref->_describeRefCount((const char*)"after active_object and _remove_ref"); \
     } while (0)
#else
#define WRAPPED_REGISTER_IMPL2(implref, nm) \
  do { PortableServer::ObjectId_var obv = WRAPPED_ORB_OA::_poa->activate_object(implref); implref->_remove_ref(); } while (0)
#endif

#define WRAPPED_IMPL2OREF(interfaceClass, implref) \
  implref->_this()

// Since _this() does a duplicate, we must do 2 releases.
// One is explicit, one is the result of using Object_var.
#define WRAPPED_RELEASE_IMPL(implref) \
  do { CORBA::Object_var oref = implref->_this(); CORBA::release(oref); } while (0)

// ** NB: note that either WRAPPED_DISPOSE_INFO
//    is in charge of freeing any storage that was allocated
//    by WRAPPED_IMP2DISPOSEINFO, or the type
//    WRAPPED_DISPOSEINFO_VAR must take handle this.
//    In this case, the former approach is used.

#define WRAPPED_DISPOSEINFO_VAR \
  PortableServer::ObjectId*

#define WRAPPED_DISPOSEINFO_PTR \
  PortableServer::ObjectId*

#define WRAPPED_DISPOSEINFO_NIL \
  (WRAPPED_DISPOSEINFO_PTR)0

#define WRAPPED_DISPOSEINFO_IS_NIL(info) \
  (info == WRAPPED_DISPOSEINFO_NIL)

#define WRAPPED_IMPL2DISPOSEINFO(implref) \
  WRAPPED_ORB_OA::_poa->servant_to_id(implref)

#define WRAPPED_DISPOSE_INFO(info) \
  do { \
       WRAPPED_ORB_OA::_poa->deactivate_object(*(info)); delete info; \
     } while (0)

#define WRAPPED_SKELETON_SUPER(prefix, interfacenm) \
  public POA_ ## prefix :: interfacenm, public PortableServer::RefCountServantBase

#endif  /*  __OMNIORB_POA_WRAPPERS_H__  */
