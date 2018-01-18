// -*- Mode: C++; -*-
//                              File      : CosNotifyShorthands.h
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
//
// The main purpose of CosNotifyShorthands.h is to hide the difference
// between including BOA vs. POA versions of the relevant .hh files.
//
// A secondary purpose is to introduce shorthands for the rather long
// names one finds in the CosNotification spec.

#ifndef _COS_NOTIFY_H_
#define _COS_NOTIFY_H_

#include "thread_wrappers.h"
#include "corba_wrappers.h"

#ifndef COS_USES_BOA
#  ifdef __MICO__
#    include <mico/TimeBaseNoLongLong.h>
#    include <mico/CosEventComm.h>
#    include <mico/CosEventChannelAdmin.h>
#    include <mico/CosNotification.h>
#    include <mico/CosNotifyComm.h>
#    include <mico/CosNotifyChannelAdmin.h>
#    include <mico/CosNotifyFilter.h>
#    include <mico/AttNotification.h>
#  else
#    include "COS/TimeBase.hh"
#    include "COS/CosEventComm.hh"
#    include "COS/CosEventChannelAdmin.hh"
#    include "COS/CosNotification.hh"
#    include "COS/CosNotifyComm.hh"
#    include "COS/CosNotifyChannelAdmin.hh"
#    include "COS/CosNotifyFilter.hh"
#    include "COS/AttNotification.hh"
#  endif
#else
#  ifdef __MICO__
#    error "mico boa use not supported"
#  else 
#    include "COS/BOA/TimeBase.hh"
#    include "COS/BOA/CosEventComm.hh"
#    include "COS/BOA/CosEventChannelAdmin.hh"
#    include "COS/BOA/CosNotification.hh"
#    include "COS/BOA/CosNotifyComm.hh"
#    include "COS/BOA/CosNotifyChannelAdmin.hh"
#    include "COS/BOA/CosNotifyFilter.hh"
#    include "COS/BOA/AttNotification.hh"
#  endif
#endif

#define AttN      AttNotification
#define CosEvC    CosEventComm
#define CosEvCA   CosEventChannelAdmin
#define CosN      CosNotification
#define CosNC     CosNotifyComm
#define CosNA     CosNotifyChannelAdmin
#define CosNF     CosNotifyFilter

// Useful OUT_arg typedefs
typedef WRAPPED_OUTARG_TYPE(AttNotification::Interactive)
  AttN_Interactive_outarg;
typedef WRAPPED_OUTARG_TYPE(CosNotification::NamedPropertyRangeSeq)
  CosN_NamedPropertyRangeSeq_outarg;

#endif

