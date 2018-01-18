// -*- Mode: C++; -*-
//                              File      : omniorb_boa_wrappers.h
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

#ifndef __OMNIORB_BOA_WRAPPERS_H__
#define __OMNIORB_BOA_WRAPPERS_H__

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
  static _nfy_attr CORBA::BOA_ptr                 _boa;
  static _nfy_attr CORBA::Boolean                 _invoked_impl_is_ready;
  static _nfy_attr CORBA::Boolean                 _one_call_per_connection;
  static _nfy_attr CORBA::ULong                   _max_giop_connections;
#ifdef __OMNIORB4__
  static _nfy_attr DynamicAny::DynAnyFactory_var  _dynany_fac;
#endif
};

#define WRAPPED_REGISTER_IMPL(implref) \
  implref->_obj_is_ready(WRAPPED_ORB_OA::_boa)
#define WRAPPED_REGISTER_IMPL2(implref, nm) \
  implref->_obj_is_ready(WRAPPED_ORB_OA::_boa)

#define WRAPPED_IMPL2OREF(interfaceClass, implref) \
  interfaceClass::_duplicate(implref->_this())

#define WRAPPED_RELEASE_IMPL(implref)\
  CORBA::release(implref->_this())

// ** NB: note that either WRAPPED_DISPOSE_INFO
//    is in charge of freeing any storage that was allocated
//    by WRAPPED_IMP2DISPOSEINFO, or the type
//    WRAPPED_DISPOSEINFO_VAR must take handle this.
//    In this case, the latter approach is used.

#define WRAPPED_DISPOSEINFO_VAR \
  CORBA::Object_var

#define WRAPPED_DISPOSEINFO_PTR \
  CORBA::Object_ptr

#define WRAPPED_DISPOSEINFO_NIL \
  CORBA::Object::_nil()

#define WRAPPED_DISPOSEINFO_IS_NIL(info) \
  CORBA::is_nil(info)

#define WRAPPED_IMPL2DISPOSEINFO(implref) \
  implref->_this()

#define WRAPPED_DISPOSE_INFO(info) \
  WRAPPED_ORB_OA::_boa->dispose(info)

#define WRAPPED_SKELETON_SUPER(prefix, interfacenm) \
  public virtual  prefix :: _sk_ ## interfacenm

#endif  /*   __OMNIORB_BOA_WRAPPERS_H__  */
