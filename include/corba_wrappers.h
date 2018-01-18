// -*- Mode: C++; -*-
//                              File      : corba_wrappers.h
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
 
//////////////////////////////////////////////////////////////////////
//
//                WRAPPER NOTES
//
// NEW: The static state required for WRAPPED_ORB_OA must be declared
// in exactly one .cc file by including corba_wrappers_impl.h.
//
// If you are using the CosNotify library, then this initialization
// is already included in the library and you need not do anything more.
//
// If you are using corba_wrappers.h (and related wrappers files)
// with a stand-alone application then you should include corba_wrappers_impl.h
// in, e.g., the .cc file that contains your main().
//
// Pre-Initialization
// ------------------
// The following calls set configuration parameters of the ORB.
// Not all ORBs will support these calls, so only things that optimize
// performance if they are available should be here.
//
//  WRAPPED_ORB_SETMAXOUTGOING(x);
//     ** N.B.: Must be called *before* calling WRAPPED_ORB_OA::init
//     Sets maximum number of outgoing connections to a given server endpoint to x.
// 
//  WRAPPED_ORB_SET1CALLPERCON(x);
//      ** N.B.: Must be called *before* calling WRAPPED_ORB_OA::init
//      x is 0 or 1: if 1, allows more than one call in flight over a single connection
// 
// WRAPPED_ORB_OA Initialization
// -----------------------------
//   Always use this generic init form at top of main:
//          WRAPPED_ORB_OA::init(argc, argv);
//   N.B. This stores some state in static class variables of the
//   class WRAPPED_ORB_OA.  Other calls below rely on this state
//   being set.
//
//   This call initializes an orb and either a boa or a poa.
//   N.B.: argc, argv are modified to remove any orb-specific arguments
// 
// OBTAINING THE ORB
// -----------------
//  Use 
//       WRAPPED_ORB_OA::orb() 
//  if you need a reference to the orb.  You should *not* release
//  this reference (unless you duplicate it yourself).
//
// DYNAMICALLY CHANGING ORB CONFIG
// -------------------------------
// N.B. Not all ORBs will support these calls; they may do nothing.
//
//    WRAPPED_ORB_SET_CLIENT_TIMEOUT(m);
//    WRAPPED_ORB_SET_SERVER_TIMEOUT(m);
//
// Incoming (server) or outgoing (client) connections are checked to
// see if they have been going for m milliseconds.  If yes, they shut
// down with a COMM_FAILURE exception (for client timeout) or simply
// with a connection shutdown (for server timeout).  Depending on the
// ORB, the server-side timeout may limit the entire call time, or it
// may simply limit the time the ORB waits for an incoming request to
// be completely received once it has started receiving the request.
//
// For most ORBs the default is not to timeout; this can be set
// by using value 0.
//
// N.B. Some orbs only support timeouts in seconds.  In this case
// the m value in milliseconds is rounded to the nearest second, with
// the exception that any value > 0 that would normally be rounded
// to zero is rounded to 1 instead, on the assumption that SOME
// timeout is desired.
//
//   WRAPPED_ORB_SET_SCAN_GRANULARITY(s);
// 
// Set scan granularity for checking connections to s seconds.  Note
// that this granularity places a limit on the ability to detect
// timeouts: if you set a timeout to 1 second but set the scan
// granularity to 5 seconds, then detecting a timeout could take
// just under 6 seconds in the worst case.
//
//    m = WRAPPED_ORB_GET_CLIENT_TIMEOUT();
//    m = WRAPPED_ORB_GET_SERVER_TIMEOUT();
// 
// Returns the current client or server timeout (in milliseconds).
//
//    s = WRAPPED_ORB_GET_SCAN_GRANULARITY();
// 
// Returns the current scan granularity (in seconds).
//
// WRAPPED_ORB_OA Cleanup
// ----------------------
//          WRAPPED_ORB_OA::cleanup();
//
//   This call cleans up the orb and boa or poa that were
//   initialized via WRAPPED_ORB_OA::init.  None of the
//   calls below should be used after invoking cleanup.
//   Should only be called by main thread.
//
// Constructing and registering objects
// ------------------------------------
//      Foo_i* myfoo = new Foo_i(....); // construct a new impl of Foo interface using class Foo_i
//       // returns 1 on error, return 0 on success...
//      CORBA::Boolean failcode = WRAPPED_REGISTER_IMPL(myfoo);
//
//    This will register the object with the boa or poa manager.
//
// Activate *OAs
// -------------
// Call
//      WRAPPED_ORB_OA::activate_oas();
//
// to active the boa or poa.  For boa it does a non-blocking
// impl_is_ready (for some orbs this means spawning a thread, but for
// omniorb there is a nonblocking impl_is_ready option)
//
// Waiting for shutdown
// --------------------
//     WRAPPED_ORB_OA::run();
//
//   This call blocks until a shutdown occurs, due to a failure
//   or to an explicit shutdown...
//
// Causing a shutdown
// ------------------
//     WRAPPED_ORB_OA::shutdown(CORBA::Boolean wait_for_completion);
//
// If the argument wait_for_completion is false, then the actual work of
// carrying out a shutdown will occur in a separate thread and the
// shutdown call will return immediately.  If it is true then the call
// blocks until the shutdown work has been done.
//
// Reference management
// --------------------
// When you have a CORBA object reference, there are standard ways to
// duplicate the reference and release the reference:
//
//     Foo::_duplicate(fooptr); // duplicate an obj reference
//     CORBA::release(fooptr);  // release   an obj reference
//
// However, when you have an implementation reference, there is no
// standard way to do a duplicate or release of the associated
// object reference.  The member function _this() converts from
// the impl reference to the object reference, but it sometimes has
// a reference-duplicating side-effect, and sometimes not.
// For this reason you should avoid using _this() in your code,
// and instead use these macros...
//
// After creating a Foo_i object called myfoo, if you want a
// CORBA object reference, use
//
//   Foo_ptr fooptr = WRAPPED_IMPL2OREF(Foo,myfoo);
//
// This has a side-effect of duplicating the reference.
// ** Remember to release a reference produced by WRAPPED_IMPL2OREF.
//
// Note: with some orbs,
//
//     Foo_ptr fooptr = myfoo->_this();
//
// has a duplicating side-effect, but with some it does not,
// in which case the above wrapper does
//
//     Foo_ptr fooptr = Foo::_duplicate(myfoo->_this());
//
// With the wrapper you know you always get the duplicating side-effect;
// so you can avoid using _this() in your code.
//
// If you have a myfoo and you want to do a release, use
//
//     WRAPPED_RELEASE_IMPL(myfoo);
//
// Safe object disposal
// --------------------
// We use a special helper class, RDIOPlocks, to manage
// safe disposal of objects.  An RDIOplockEntry is allocated
// for each managed object, where the _oplockptr member point
// to this entry.  The entry has an inuse count that, if
// non-zero, is used to defer disposal of the associated
// object until it is safe to do so.  See RDIOplocks.h
// for details.  The following macros are used by the helper class:
//
// WRAPPED_DISPOSEINFO_PTR, WRAPPED_DISPOSEINFO_VAR, WRAPPED_DISPOSEINFO_NIL:
//   the ptr and var types, and the null value, for the information required
//   to dispose an object implementation.  For example, for a POA manager,
//   a PortableServer::ObjectId is needed to deactivate an object implementation.
//   This information is passed to RDIOplocks in a release_entry call.
//   RDIOplocks does the actual disposal once the inuse count is zero.
//
// WRAPPED_IMPL2DISPOSEINFO(implref)
//   Given an impl reference, produce the necessary dispose info.
// 
// WRAPPED_DISPOSE_INFO(info)
//   Dispose the impl associated with info.
// ==> should normally be followed by:
// info = WRAPPED_DISPOSEINFO_NIL
//
// Note that the macro
//     WRAPPED_DISPOSE_IMPL(implref)
//
// is a shorthand for these 2 steps:
//
//    WRAPPED_DISPOSEINFO_VAR info = WRAPPED_IMPL2DISPOSEINFO(implref);
//    WRAPPED_DISPOSE_INFO(info);
//
//
// Implementing the Foo_i class
// ----------------------------
// Let us say there is a module M with interface Foo, and you want
// to define an impl class Foo_i.  You declare the class as follows:
//
// class Foo_i : WRAPPED_SKELETON_SUPER(M, Foo) {
// public:
//    <constructor decl>
//    void foo ( WRAPPED_DECLARG_VOID );
//    void bar ( CORBA::ULong u    WRAPPED_DECLARG );
// protected:
//    <rest of class decl>
// }
//
// And the corresponding method implementations look like:
// 
// void Foo_i::foo ( WRAPPED_IMPLARG_VOID ) { <impl> };
// void Foo_i::bar ( CORBA::ULong u WRAPPED_IMPLARG ) { <impl> };
//
// If the ULong argument u was an out param rather than an in param,
// you should use WRAPPED_OUTARG_TYPE(CORBA::ULong) for both the
// declaration and implementation of the bar method.
//
//
// The WRAPPED_SKELETON_SUPER macro can expand to one or more
// BOA or POA helper classes that together provide the skeleton
// infrastructure for implementing an object that supports
// CORBA interface M::Foo.
//
// WRAPPED_DECLARG_VOID / WRAPPED_IMPLARG_VOID are used
// for methods with no parameters, while 
// WRAPPED_DECLARG / WRAPPED_IMPLARG are used for methods
// with one or more parameters.  Note that there is NO COMMA 
// between the last param and the DECLARG or IMPLARG macro.
// 
//
// Resolving initial references
// ----------------------------
//  Although all ORBs provide an orb->resolve_initial_references function,
//  configuring the ORB do use, say, a different name service, can be a
//  major hassle.  Sometimes it is easier to put special case handling
//  in a macro, which is why we provide the following macro.  Here is
//  an example use:
//
//    CORBA::Object_var nmsv = WRAPPED_RESOLVE_INITIAL_REFERENCE("NameService");
//    CosNaming::NamingContext_var nmcx = CosNaming::NamingContext::_narrow(nmsv);
//    if ( CORBA::is_nil(nmcx) ) { ... something went wrong ... }
//    ... use nmcx (root naming context) ...
//
// Hashing an object reference
//   WRAPPED_OBJREF_HASH(objref, max) :
// returns an unsigned long hash value for objref,
// where max is the max value that should be returned
//
// MISC
// ----
// Some of these are to hide differences between CORBA 2.2 DynAnys and CORBA 2.5 DynAnys
//
// WRAPPED_DYNANY_MODULE  -- will be either CORBA or DynamicAny
//
// WRAPPED_CREATE_DYN_ANY(value) -- create a DynAny from a CORBA::Any value
//
// WRAPPED_CREATE_DYN_ANY_FROM_TYPE_CODE(type) -- create a DynAny from a TypeCode
//
// WRAPPED_DYNENUM_GET_AS_STRING(da_enum) -- DynEnum value as string
//
// WRAPPED_DYNENUM_GET_AS_ULONG(da_enum)  -- DynEnum value as ulong
//
// WRAPPED_DYNUNION_IS_SET_TO_DEFAULT_MEMBER(da_union) -- Is DynUnion set to default member
//
// WRAPPED_DYNUNION_GET_DISCRIMINATOR(da_union) -- DynUnion get discriminator
//
// WRAPPED_DYNSEQUENCE_GET_LENGTH(da_seq) -- DynSequence get length
//
// WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(t,tseq)
//    We assume all ORBs have a sequence template type under the covers
//    used to implement sequence types declared in idl files
//    this macro allows us to typedef a new type tseq which is an
//    unbounded sequence of t with the operations you would
//    expect from an idl sequence type (such as length() and length(i)) 
//
// WRAPPED_CORBA_LONGLONG_CONST(x) -- declare a constant with type CORBA::LongLong
//
// WRAPPED_CORBA_LONGLONG_TYPE   -- the underlying type for a CORBA::LongLong
// WRAPPED_CORBA_ULONGLONG_TYPE  -- the underlying type for a CORBA::ULongLong
// WRAPPED_CORBA_LONGDOUBLE_TYPE -- the underlying type for a CORBA::LongDouble
//
// Macros that are defined/not defined based on ORB:
//
// WRAPPED_ANY_THREADSAFE : defined or not
//    The following actions all examine the state of a CORBA::Any
//           - copying
//           - marshalling
//           - conversion to DynAny
//    The underlying implementation of these things may share some internal pointer(s),
//    in which case it is not safe for multiple threads to perform these actions over
//    the same CORBA::Any (or over something that contains one or more CORBA::Any,
//    such as an event) without doing some kind of mutual exclusion across the threads.
//    To program for different cases, one can use, e.g.:
//
// #ifdef WRAPPED_ANY_THREADSAFE
// ... (locking not required)
// #else
// ... (locking required)
// #endif
//
//////////////////////////////////////////////////////////////////////

#ifndef __CORBA_WRAPPERS_H__
#define __CORBA_WRAPPERS_H__

// More ORBs could be supported.  For now we just support OMNIORB3/4 and MICO
#if !defined(__OMNIORB3__) && !defined(__OMNIORB4__) && !defined(__MICO__)
#  error "One of the following must be defined : __OMNIORB3__, __OMNIORB4__, __MICO__"
stop_compiling
#endif

#if defined(__OMNIORB3__)
#  include <omniORB3/CORBA.h>
#  include <omniORB3/Naming.hh>
#ifdef COS_USES_BOA
#  include "omniorb_boa_wrappers.h"
#else
#  include "omniorb_poa_wrappers.h"
#endif
#elif defined(__OMNIORB4__) 
#  include <omniORB4/CORBA.h>
#  include <omniORB4/Naming.hh>
#ifdef COS_USES_BOA
#  include "omniorb_boa_wrappers.h"
#else
#  include "omniorb_poa_wrappers.h"
#endif
#elif defined(__MICO__)
#  include <CORBA.h>
#  include <mico/CosNaming.h>
#ifdef COS_USES_BOA
#  include "mico_boa_wrappers.h"
#else
#  include "mico_poa_wrappers.h"
#endif
#endif

////////////////////////////////////////////////////////////////////
// Macros that are always the same (combinations of other macros) //
////////////////////////////////////////////////////////////////////

#define WRAPPED_DISPOSE_IMPL(implref) \
  do {\
    WRAPPED_DISPOSEINFO_VAR info = WRAPPED_IMPL2DISPOSEINFO(implref); \
    WRAPPED_DISPOSE_INFO(info); } while (0)


#endif /* __CORBA_WRAPPERS_H__ */
