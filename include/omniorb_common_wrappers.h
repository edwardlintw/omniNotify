// -*- Mode: C++; -*-
//                              File      : omniorb_common_wrappers.h
//                              Package   : omniNotify-Library
//                              Created on: 16-Oct-2001
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
 
#ifndef __OMNIORB_COMMON_WRAPPERS_H__
#define __OMNIORB_COMMON_WRAPPERS_H__
 
#ifndef __WRAPPED_NULL_STMT
#define __WRAPPED_NULL_STMT 	do { } while (0)
#endif

#define WRAPPED_OUTARG_TYPE(T)  T ## _out
#define WRAPPED_DECLARG
#define WRAPPED_DECLARG_VOID    void
#define WRAPPED_IMPLARG
#define WRAPPED_IMPLARG_VOID    void

#define WRAPPED_ORB_SETMAXOUTGOING(x) \
  WRAPPED_ORB_OA::_max_giop_connections = x

#define WRAPPED_ORB_SET1CALLPERCON(x) \
  WRAPPED_ORB_OA::_one_call_per_connection = x

#define WRAPPED_RESOLVE_INITIAL_REFERENCES(nm) \
  WRAPPED_ORB_OA::_orb->resolve_initial_references(nm)

#define WRAPPED_OBJREF_HASH(objref, max) \
  objref->_hash(max)

#define WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(t,tseq) \
  typedef _CORBA_Unbounded_Sequence<t> tseq

#define RDI_ROUND_MILLISEC_TO_SEC(m, s) \
do { \
  s = 0; \
  if (m > 0) { \
    if (m <= 1000) { s = 1; } \
    else           { s = (m+500)/1000; } \
  } \
} while (0)

#if defined(__OMNIORB3__)
// N.B.: WRAPPED_ANY_THREADSAFE is NOT defined for omniORB3
#  define WRAPPED_DYNANY_MODULE CORBA
#  define WRAPPED_CREATE_DYN_ANY(value) \
     WRAPPED_ORB_OA::orb()->create_dyn_any(a)
#  define WRAPPED_CREATE_DYN_ANY_FROM_TYPE_CODE(type) \
     WRAPPED_ORB_OA::orb()->create_basic_dyn_any(type)
#  define WRAPPED_DYNENUM_GET_AS_STRING(da_enum) \
     da_enum->value_as_string()
#  define WRAPPED_DYNENUM_GET_AS_ULONG(da_enum) \
     da_enum->value_as_ulong()
#  define WRAPPED_DYNUNION_IS_SET_TO_DEFAULT_MEMBER(da_union) \
     da_union->set_as_default()
#  define WRAPPED_DYNUNION_GET_DISCRIMINATOR(da_union) \
     da_union->discriminator()
#  define WRAPPED_DYNSEQUENCE_GET_LENGTH(da_seq) \
     da_seq->length()
#  define WRAPPED_ORB_SET_CLIENT_TIMEOUT(m) \
do { \
  CORBA::ULong s; \
  RDI_ROUND_MILLISEC_TO_SEC(m, s); \
  omniORB::callTimeOutPeriod(omniORB::clientSide, s); \
} while (0)
#  define WRAPPED_ORB_SET_SERVER_TIMEOUT(m) \
do { \
  CORBA::ULong s; \
  RDI_ROUND_MILLISEC_TO_SEC(m, s); \
  omniORB::callTimeOutPeriod(omniORB::serverSide, s); \
} while (0)
#  define WRAPPED_ORB_SET_SCAN_GRANULARITY(s) \
     (omniORB::scanGranularity(s))
#  define WRAPPED_ORB_GET_CLIENT_TIMEOUT() \
     (omniORB::callTimeOutPeriod(omniORB::clientSide) * 1000)
#  define WRAPPED_ORB_GET_SERVER_TIMEOUT() \
     (omniORB::callTimeOutPeriod(omniORB::serverSide) * 1000)
#define WRAPPED_ORB_GET_SCAN_GRANULARITY() \
     (omniORB::scanGranularity())

#elif defined(__OMNIORB4__)
#  define WRAPPED_ANY_THREADSAFE
#  define WRAPPED_DYNANY_MODULE DynamicAny
#  define WRAPPED_CREATE_DYN_ANY(value) \
     WRAPPED_ORB_OA::_dynany_fac->create_dyn_any(value)
#  define WRAPPED_CREATE_DYN_ANY_FROM_TYPE_CODE(type) \
     WRAPPED_ORB_OA::_dynany_fac->create_dyn_any_from_type_code(type)
#  define WRAPPED_DYNENUM_GET_AS_STRING(da_enum) \
     da_enum->get_as_string()
#  define WRAPPED_DYNENUM_GET_AS_ULONG(da_enum) \
     da_enum->get_as_ulong()
#  define WRAPPED_DYNUNION_IS_SET_TO_DEFAULT_MEMBER(da_union) \
     da_union->is_set_to_default_member()
#  define WRAPPED_DYNUNION_GET_DISCRIMINATOR(da_union) \
     da_union->get_discriminator()
#  define WRAPPED_DYNSEQUENCE_GET_LENGTH(da_seq) \
     da_seq->get_length()

// The following would ideally be available as omniORB::foo calls
#  include <omniORB4/internal/orbParameters.h>
#  define WRAPPED_ORB_SET_CLIENT_TIMEOUT(m) \
do { \
  omni::orbParameters::clientCallTimeOutPeriod.s = m / 1000; \
  omni::orbParameters::clientCallTimeOutPeriod.ns = (m % 1000) * 1000000; \
} while (0)
#  define WRAPPED_ORB_SET_SERVER_TIMEOUT(m) \
do { \
  omni::orbParameters::serverCallTimeOutPeriod.s = m / 1000; \
  omni::orbParameters::serverCallTimeOutPeriod.ns = (m % 1000) * 1000000; \
} while (0)
#  define WRAPPED_ORB_SET_SCAN_GRANULARITY(s) \
do { \
  omni::orbParameters::scanGranularity = s; \
} while (0)
#  define WRAPPED_ORB_GET_CLIENT_TIMEOUT() \
     (omni::orbParameters::clientCallTimeOutPeriod.s * 1000 + omni::orbParameters::clientCallTimeOutPeriod.ns / 1000000)
#  define WRAPPED_ORB_GET_SERVER_TIMEOUT() \
     (omni::orbParameters::serverCallTimeOutPeriod.s * 1000 + omni::orbParameters::serverCallTimeOutPeriod.ns / 1000000)
#  define WRAPPED_ORB_GET_SCAN_GRANULARITY() \
     (omni::orbParameters::scanGranularity)

#endif /* omniORB3/4 switch */

#define WRAPPED_CORBA_LONGLONG_CONST(x) \
  _CORBA_LONGLONG_CONST(x)

#define WRAPPED_CORBA_LONGLONG_TYPE  _CORBA_LONGLONG_DECL
#define WRAPPED_CORBA_ULONGLONG_TYPE _CORBA_ULONGLONG_DECL

#ifndef NO_FLOAT
#  ifdef HAS_LongDouble
#    define WRAPPED_CORBA_LONGDOUBLE_TYPE _CORBA_LONGDOUBLE_DECL
#  endif
#endif


#if defined(_MSC_VER)
#  define WRAPPED_CORBA_ULONGLONG_CONST(x) (x)
#else
#  define WRAPPED_CORBA_ULONGLONG_CONST(x) (x##ULL)
#endif

#endif  /*  __OMNIORB_COMMON_WRAPPERS_H__  */

