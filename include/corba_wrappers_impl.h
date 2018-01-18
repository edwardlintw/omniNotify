// -*- Mode: C++; -*-
//                              File      : corba_wrappers_impl.h
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
 
// This should be included by exactly one .cc file, e.g.,
// by the .cc file containing main().
//
// It is also included in the CosNotify library, thus if you link with
// that library you do not need to include it in one of your .cc files.

#ifndef __CORBA_WRAPPERS_IMPL_H__
#define __CORBA_WRAPPERS_IMPL_H__

// More ORBs could be supported.  For now we just support OMNIORB3/4 and MICO
#if !defined(__OMNIORB3__) && !defined(__OMNIORB4__) && !defined(__MICO__)
#  error "One of the following must be defined : __OMNIORB3__, __OMNIORB4__, __MICO__"
stop_compiling
#endif

#if defined(__OMNIORB3__) 
#  include <omniORB3/CORBA.h>
#  include <omniORB3/Naming.hh>
#  define ORB_INIT_NAME "omniORB3"
#  define BOA_INIT_NAME "omniORB3_BOA"
#ifdef COS_USES_BOA
#  include "omniorb_boa_wrappers_impl.h"
#else
#  include "omniorb_poa_wrappers_impl.h"
#endif
#elif defined(__OMNIORB4__) 
#  include <omniORB4/CORBA.h>
#  include <omniORB4/Naming.hh>
#  define ORB_INIT_NAME "omniORB4"
#  define BOA_INIT_NAME "omniORB4_BOA"
#ifdef COS_USES_BOA
#  include "omniorb_boa_wrappers_impl.h"
#else
#  include "omniorb_poa_wrappers_impl.h"
#endif
#elif defined(__MICO__) 
#  include <CORBA.h>
#  include <mico/CosNaming.h>
#  define ORB_INIT_NAME "mico-local-orb"
#  define BOA_INIT_NAME "mico-local-boa"
#ifdef COS_USES_BOA
#  include "mico_boa_wrappers_impl.h"
#else
#  include "mico_poa_wrappers_impl.h"
#endif
#endif

#endif /* __CORBA_WRAPPERS_IMPL_H__ */
