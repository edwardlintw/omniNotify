// -*- Mode: C++; -*-
//                              File      : CosNotification.cc
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
//    Implementation of QoS and Admin Properties
//

#include "RDI.h"
#include "RDILimits.h"
#include "RDIStringDefs.h"
#include "RDITimeWrappers.h"
#include "RDIBitmap.h"
#include "CosNotification_i.h"

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// Notes
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//
// 4 error codes do not include a range of supported values:
//
// UNSUPPORTED_PROPERTY: This property is not supported by this
//                       impl for this type of target object.
// UNAVAILABLE_PROPERTY: This property cannot be set (to any value)
//                       in the current context.**
// BAD_PROPERTY:         This property name is unrecognized.
//                       The impl knows nothing about it.
// BAD_TYPE:             The type of the supplied value is incorrect
//                       for this property.
//
// 3 error codes DO include a range of supported values: 
//
// UNSUPPORTED_VALUE:    The value requested for this property is not 
//                       supported by the impl for this type of target.
// UNAVAILABLE_VALUE:    The value requested for this property is not
//                       supported in the current context.**
// BAD_VALUE:            An illegal value is supplied for this property.
//
// ** Current context means in the context of other QoS properties
//
// UNAVAILABLE_PROPERTY would be ???: one possible scenario:
//    if EventReliability is Persistent, ConnectionReliability is
//    unavailable (since it cannot be anything other than Persistent)
// 
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// helper routines 
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
typedef CORBA::Boolean RDI_UnsupPropTest_fn(RDI_ObjectKind otype);
typedef CORBA::Boolean RDI_UnavPropTest_fn(const RDI_NotifQoS&  a_qos);

typedef CORBA::Boolean RDI_RangeTest_short_fn(const CORBA::Any& a, CORBA::Short lo, CORBA::Short hi);
typedef CORBA::Boolean RDI_RangeTest_long_fn(const CORBA::Any& a, CORBA::Long lo, CORBA::Long hi);
typedef CORBA::Boolean RDI_RangeTest_bool_fn(const CORBA::Any& a, CORBA::Boolean lo, CORBA::Boolean hi);
typedef CORBA::Boolean RDI_RangeTest_TimeT_fn(const CORBA::Any& a, const TimeBase::TimeT& lo, const TimeBase::TimeT& hi);
typedef CORBA::Boolean RDI_RangeTest_UtcT_fn(const CORBA::Any& a, const TimeBase::UtcT& lo, const TimeBase::UtcT& hi);

typedef void RDI_SetRange_short_fn(CosN::PropertyRange& a, CORBA::Short lo, CORBA::Short hi);
typedef void RDI_SetRange_long_fn(CosN::PropertyRange& a, CORBA::Long lo, CORBA::Long hi);
typedef void RDI_SetRange_bool_fn(CosN::PropertyRange& a, CORBA::Boolean lo, CORBA::Boolean hi);
typedef void RDI_SetRange_TimeT_fn(CosN::PropertyRange& a, const TimeBase::TimeT& lo, const TimeBase::TimeT& hi);
typedef void RDI_SetRange_UtcT_fn(CosN::PropertyRange& a, const TimeBase::UtcT& lo, const TimeBase::UtcT& hi);

typedef RDI_UnsupPropTest_fn* RDI_UnsupPropTest;
typedef RDI_UnavPropTest_fn* RDI_UnavPropTest;

typedef RDI_RangeTest_short_fn* RDI_RangeTest_short;
typedef RDI_RangeTest_long_fn* RDI_RangeTest_long;
typedef RDI_RangeTest_bool_fn* RDI_RangeTest_bool;
typedef RDI_RangeTest_TimeT_fn* RDI_RangeTest_TimeT;
typedef RDI_RangeTest_UtcT_fn* RDI_RangeTest_UtcT;

typedef RDI_SetRange_short_fn* RDI_SetRange_short;
typedef RDI_SetRange_long_fn* RDI_SetRange_long;
typedef RDI_SetRange_bool_fn* RDI_SetRange_bool;
typedef RDI_SetRange_TimeT_fn* RDI_SetRange_TimeT;
typedef RDI_SetRange_UtcT_fn* RDI_SetRange_UtcT;

// ----------------------------------------
// Since these are used for NotifQoS testing, RDI_NFSERVER is never a valid otype

CORBA::Boolean RDI_UnsupPropTest_NotChannelNotMessage(RDI_ObjectKind otype) {
  return ((otype != RDI_ECHANNEL) && (otype != RDI_EMESSAGE));
}

CORBA::Boolean RDI_UnsupPropTest_Message(RDI_ObjectKind otype) {
  return ((otype == RDI_EMESSAGE) || (otype == RDI_NFSERVER));
}

CORBA::Boolean RDI_UnsupPropTest_NotMessage(RDI_ObjectKind otype) {
  return (otype != RDI_EMESSAGE);
}

CORBA::Boolean RDI_UnsupPropTest_false(RDI_ObjectKind otype) {
  return (otype == RDI_NFSERVER);
}

// ----------------------------------------

CORBA::Boolean RDI_RangeTest_short_impl(const CORBA::Any& a, CORBA::Short lo, CORBA::Short hi) {
  CORBA::Short val;
  a >>= val;
  return ((val < lo) || (val > hi));
}

CORBA::Boolean RDI_RangeTest_long_impl(const CORBA::Any& a, CORBA::Long lo, CORBA::Long hi) {
  CORBA::Long val;
  a >>= val;
  return ((val < lo) || (val > hi));
}

CORBA::Boolean RDI_RangeTest_bool_impl(const CORBA::Any& a, CORBA::Boolean lo, CORBA::Boolean hi) {
  CORBA::Boolean val;
  CORBA::Any::to_boolean a2val(val);
  a >>= a2val;
  return ((val < lo) || (val > hi));
}

CORBA::Boolean RDI_RangeTest_TimeT_impl(const CORBA::Any& a, const TimeBase::TimeT& lo, const TimeBase::TimeT& hi) {
#ifdef TIMEBASE_NOLONGLONG
  const TimeBase::TimeT* val;
  a >>= val;
  return ( RDI_TBTimeT_LT(*val, lo) || RDI_TBTimeT_GT(*val, hi) );
#else
  TimeBase::TimeT val;
  a >>= val;
  return ( RDI_TBTimeT_LT(val, lo) || RDI_TBTimeT_GT(val, hi) );
#endif
}

CORBA::Boolean RDI_RangeTest_UtcT_impl(const CORBA::Any& a, const TimeBase::UtcT& lo, const TimeBase::UtcT& hi) {
  const TimeBase::UtcT* val;
  a >>= val;
  return ( RDI_TBUtcT_LT(*val, lo) || RDI_TBUtcT_GT(*val, hi) );
}

// ----------------------------------------

void RDI_SetRange_short_impl(CosN::PropertyRange& r, CORBA::Short lo, CORBA::Short hi) {
  r.low_val  <<= lo;
  r.high_val <<= hi;
}

void RDI_SetRange_long_impl(CosN::PropertyRange& r, CORBA::Long lo, CORBA::Long hi) {
  r.low_val  <<= lo;
  r.high_val <<= hi;
}

void RDI_SetRange_bool_impl(CosN::PropertyRange& r, CORBA::Boolean lo, CORBA::Boolean hi) {
  CORBA::Any::from_boolean fromlo(lo);
  CORBA::Any::from_boolean fromhi(hi);
  r.low_val  <<= fromlo;
  r.high_val <<= fromhi;
}

void RDI_SetRange_TimeT_impl(CosN::PropertyRange& r, const TimeBase::TimeT& lo, const TimeBase::TimeT& hi) {
  r.low_val  <<= lo;
  r.high_val <<= hi;
}

void RDI_SetRange_UtcT_impl(CosN::PropertyRange& r, const TimeBase::UtcT& lo, const TimeBase::UtcT& hi) {
  r.low_val  <<= lo;
  r.high_val <<= hi;
}

// ----------------------------------------

CORBA::Boolean RDI_UnavPropTest_false(const RDI_NotifQoS&  a_qos) { return 0; }

CORBA::Boolean RDI_UnavPropTest_startTimeNotSupported(const RDI_NotifQoS&  a_qos) {
  return (! a_qos.startTimeSupported());
}

CORBA::Boolean RDI_UnavPropTest_stopTimeNotSupported(const RDI_NotifQoS&  a_qos) {
  return (! a_qos.stopTimeSupported());
}

////////////////////////////////////////////////////////////////////////////////
//            NotifQoS 'Table' and associated Macros
////////////////////////////////////////////////////////////////////////////////

const unsigned int           RDI_NProp0_num       = 0;
const char*                  RDI_NProp0_name      = "EventReliability";
const CORBA::TypeCode_ptr    RDI_NProp0_tcde      = CORBA::_tc_short;
const CORBA::Short           RDI_NProp0_lval      = CosN::BestEffort;
const CORBA::Short           RDI_NProp0_hval      = CosN::Persistent;
// currently, only supported value is CosN::BestEffort
const CORBA::Short           RDI_NProp0_lsupval   = CosN::BestEffort;
const CORBA::Short           RDI_NProp0_hsupval   = CosN::BestEffort;
const RDI_UnsupPropTest      RDI_NProp0_unsuprop  = RDI_UnsupPropTest_NotChannelNotMessage;
const RDI_RangeTest_short    RDI_NProp0_tstrange  = RDI_RangeTest_short_impl;
const RDI_UnavPropTest       RDI_NProp0_unavprop  = RDI_UnavPropTest_false;
const RDI_SetRange_short     RDI_NProp0_setrange  = RDI_SetRange_short_impl;
const char*                  RDI_NProp0_supmsg    = "For EventReliability, only BestEffort [0] is currently supported by omniNotify\n";
#define RDI_NProp0_check     check_prop_config_hilo
#define RDI_NProp0_checkset  checkset_prop_config_hilo

const unsigned int           RDI_NProp1_num       = 1;
const char*                  RDI_NProp1_name      = "ConnectionReliability";
const CORBA::TypeCode_ptr    RDI_NProp1_tcde      = CORBA::_tc_short;
const CORBA::Short           RDI_NProp1_lval      = CosN::BestEffort;
const CORBA::Short           RDI_NProp1_hval      = CosN::Persistent;
// currently, only supported value is CosN::BestEffort
const CORBA::Short           RDI_NProp1_lsupval   = CosN::BestEffort;
const CORBA::Short           RDI_NProp1_hsupval   = CosN::BestEffort;
const RDI_UnsupPropTest      RDI_NProp1_unsuprop  = RDI_UnsupPropTest_Message;
const RDI_RangeTest_short    RDI_NProp1_tstrange  = RDI_RangeTest_short_impl;
const RDI_UnavPropTest       RDI_NProp1_unavprop  = RDI_UnavPropTest_false;
const RDI_SetRange_short     RDI_NProp1_setrange  = RDI_SetRange_short_impl;
const char*                  RDI_NProp1_supmsg    = "For ConnectionReliability, only BestEffort [0] is currently supported by omniNotify\n";
#define RDI_NProp1_check     check_prop_config_hilo
#define RDI_NProp1_checkset  checkset_prop_config_hilo

const unsigned int           RDI_NProp2_num       = 2;
const char*                  RDI_NProp2_name      = "Priority";
const CORBA::TypeCode_ptr    RDI_NProp2_tcde      = CORBA::_tc_short;
const CORBA::Short           RDI_NProp2_lval      = CosN::LowestPriority;
const CORBA::Short           RDI_NProp2_hval      = CosN::HighestPriority;
const CORBA::Short           RDI_NProp2_lsupval   = CosN::LowestPriority;
const CORBA::Short           RDI_NProp2_hsupval   = CosN::HighestPriority;
const RDI_UnsupPropTest      RDI_NProp2_unsuprop  = RDI_UnsupPropTest_false;
const RDI_RangeTest_short    RDI_NProp2_tstrange  = RDI_RangeTest_short_impl;
const RDI_UnavPropTest       RDI_NProp2_unavprop  = RDI_UnavPropTest_false;
const RDI_SetRange_short     RDI_NProp2_setrange  = RDI_SetRange_short_impl;
const char*                  RDI_NProp2_supmsg    = 0;
#define RDI_NProp2_check     check_prop_config_triv
#define RDI_NProp2_checkset  checkset_prop_config_triv

const unsigned int           RDI_NProp3_num       = 3;
const char*                  RDI_NProp3_name      = "StartTime";
const CORBA::TypeCode_ptr    RDI_NProp3_tcde      = TimeBase::_tc_UtcT;
const TimeBase::UtcT         RDI_NProp3_lval      = RDI_UtcT::MinTimeBaseUtcT();
const TimeBase::UtcT         RDI_NProp3_hval      = RDI_UtcT::MaxTimeBaseUtcT();
const TimeBase::UtcT         RDI_NProp3_lsupval   = RDI_UtcT::MinTimeBaseUtcT();
const TimeBase::UtcT         RDI_NProp3_hsupval   = RDI_UtcT::MaxTimeBaseUtcT();
const RDI_UnsupPropTest      RDI_NProp3_unsuprop  = RDI_UnsupPropTest_NotMessage;
const RDI_RangeTest_UtcT     RDI_NProp3_tstrange  = RDI_RangeTest_UtcT_impl;
const RDI_UnavPropTest       RDI_NProp3_unavprop  = RDI_UnavPropTest_startTimeNotSupported;
const RDI_SetRange_UtcT      RDI_NProp3_setrange  = RDI_SetRange_UtcT_impl;
const char*                  RDI_NProp3_supmsg    = 0;
#define RDI_NProp3_check     check_prop_config_time
#define RDI_NProp3_checkset  checkset_prop_config_time

const unsigned int           RDI_NProp4_num       = 4;
const char*                  RDI_NProp4_name      = "StopTime";
const CORBA::TypeCode_ptr    RDI_NProp4_tcde      = TimeBase::_tc_UtcT;
const TimeBase::UtcT         RDI_NProp4_lval      = RDI_UtcT::MinTimeBaseUtcT();
const TimeBase::UtcT         RDI_NProp4_hval      = RDI_UtcT::MaxTimeBaseUtcT();
const TimeBase::UtcT         RDI_NProp4_lsupval   = RDI_UtcT::MinTimeBaseUtcT();
const TimeBase::UtcT         RDI_NProp4_hsupval   = RDI_UtcT::MaxTimeBaseUtcT();
const RDI_UnsupPropTest      RDI_NProp4_unsuprop  = RDI_UnsupPropTest_NotMessage;
const RDI_RangeTest_UtcT     RDI_NProp4_tstrange  = RDI_RangeTest_UtcT_impl;
const RDI_UnavPropTest       RDI_NProp4_unavprop  = RDI_UnavPropTest_stopTimeNotSupported;
const RDI_SetRange_UtcT      RDI_NProp4_setrange  = RDI_SetRange_UtcT_impl;
const char*                  RDI_NProp4_supmsg    = 0;
#define RDI_NProp4_check     check_prop_config_time
#define RDI_NProp4_checkset  checkset_prop_config_time

const unsigned int           RDI_NProp5_num       = 5;
const char*                  RDI_NProp5_name      = "Timeout";
const CORBA::TypeCode_ptr    RDI_NProp5_tcde      = TimeBase::_tc_TimeT;
const TimeBase::TimeT        RDI_NProp5_lval      = RDI_TimeT::MinTimeBaseTimeT();
const TimeBase::TimeT        RDI_NProp5_hval      = RDI_TimeT::MaxTimeBaseTimeT();
const TimeBase::TimeT        RDI_NProp5_lsupval   = RDI_TimeT::MinTimeBaseTimeT();
const TimeBase::TimeT        RDI_NProp5_hsupval   = RDI_TimeT::MaxTimeBaseTimeT();
const RDI_UnsupPropTest      RDI_NProp5_unsuprop  = RDI_UnsupPropTest_false;
const RDI_RangeTest_TimeT    RDI_NProp5_tstrange  = RDI_RangeTest_TimeT_impl;
const RDI_UnavPropTest       RDI_NProp5_unavprop  = RDI_UnavPropTest_false;
const RDI_SetRange_TimeT     RDI_NProp5_setrange  = RDI_SetRange_TimeT_impl;
const char*                  RDI_NProp5_supmsg    = 0;
#define RDI_NProp5_check     check_prop_config_time
#define RDI_NProp5_checkset  checkset_prop_config_time

const unsigned int           RDI_NProp6_num       = 6;
const char*                  RDI_NProp6_name      = "StartTimeSupported";
const CORBA::TypeCode_ptr    RDI_NProp6_tcde      = CORBA::_tc_boolean;
const CORBA::Boolean         RDI_NProp6_lval      = 0;
const CORBA::Boolean         RDI_NProp6_hval      = 1;
// currently, only supported value is false (0) 
const CORBA::Boolean         RDI_NProp6_lsupval   = 0;
const CORBA::Boolean         RDI_NProp6_hsupval   = 0;
const RDI_UnsupPropTest      RDI_NProp6_unsuprop  = RDI_UnsupPropTest_Message;
const RDI_RangeTest_bool     RDI_NProp6_tstrange  = RDI_RangeTest_bool_impl;
const RDI_UnavPropTest       RDI_NProp6_unavprop  = RDI_UnavPropTest_false;
const RDI_SetRange_bool      RDI_NProp6_setrange  = RDI_SetRange_bool_impl;
const char*                  RDI_NProp6_supmsg    = "For StartTimeSupported, only FALSE [0] is currently supported by omniNotify\n";
#define RDI_NProp6_check     check_prop_config_bool
#define RDI_NProp6_checkset  checkset_prop_config_bool

const unsigned int           RDI_NProp7_num       = 7;
const char*                  RDI_NProp7_name      = "StopTimeSupported";
const CORBA::TypeCode_ptr    RDI_NProp7_tcde      = CORBA::_tc_boolean;
const CORBA::Boolean         RDI_NProp7_lval      = 0;
const CORBA::Boolean         RDI_NProp7_hval      = 1;
// currently, only supported value is false (0) 
const CORBA::Boolean         RDI_NProp7_lsupval   = 0;
const CORBA::Boolean         RDI_NProp7_hsupval   = 0;
const RDI_UnsupPropTest      RDI_NProp7_unsuprop  = RDI_UnsupPropTest_Message;
const RDI_RangeTest_bool     RDI_NProp7_tstrange  = RDI_RangeTest_bool_impl;
const RDI_UnavPropTest       RDI_NProp7_unavprop  = RDI_UnavPropTest_false;
const RDI_SetRange_bool      RDI_NProp7_setrange  = RDI_SetRange_bool_impl;
const char*                  RDI_NProp7_supmsg    = "For StopTimeSupported, only FALSE [0] is currently supported by omniNotify\n";
#define RDI_NProp7_check     check_prop_config_bool
#define RDI_NProp7_checkset  checkset_prop_config_bool

const unsigned int           RDI_NProp8_num       = 8;
const char*                  RDI_NProp8_name      = "MaxEventsPerConsumer";
const CORBA::TypeCode_ptr    RDI_NProp8_tcde      = CORBA::_tc_long;
const CORBA::Long            RDI_NProp8_lval      = (CORBA::Long)0;
const CORBA::Long            RDI_NProp8_hval      = RDI_LONG_MAX;
// max events per consumer limited to event queue size, but no way to specify that here
const CORBA::Long            RDI_NProp8_lsupval   = (CORBA::Long)0;
const CORBA::Long            RDI_NProp8_hsupval   = RDI_LONG_MAX;
const RDI_UnsupPropTest      RDI_NProp8_unsuprop  = RDI_UnsupPropTest_Message;
const RDI_RangeTest_long     RDI_NProp8_tstrange  = RDI_RangeTest_long_impl;
const RDI_UnavPropTest       RDI_NProp8_unavprop  = RDI_UnavPropTest_false;
const RDI_SetRange_long      RDI_NProp8_setrange  = RDI_SetRange_long_impl;
const char*                  RDI_NProp8_supmsg    = 0;
#define RDI_NProp8_check     check_prop_config_hilo
#define RDI_NProp8_checkset  checkset_prop_config_hilo

const unsigned int           RDI_NProp9_num       = 9;
const char*                  RDI_NProp9_name      = "OrderPolicy";
const CORBA::TypeCode_ptr    RDI_NProp9_tcde      = CORBA::_tc_short;
const CORBA::Short           RDI_NProp9_lval      = CosN::AnyOrder;
const CORBA::Short           RDI_NProp9_hval      = CosN::DeadlineOrder;
const CORBA::Short           RDI_NProp9_lsupval   = CosN::AnyOrder;
const CORBA::Short           RDI_NProp9_hsupval   = CosN::DeadlineOrder;
const RDI_UnsupPropTest      RDI_NProp9_unsuprop  = RDI_UnsupPropTest_Message;
const RDI_RangeTest_short    RDI_NProp9_tstrange  = RDI_RangeTest_short_impl;
const RDI_UnavPropTest       RDI_NProp9_unavprop  = RDI_UnavPropTest_false;
const RDI_SetRange_short     RDI_NProp9_setrange  = RDI_SetRange_short_impl;
const char*                  RDI_NProp9_supmsg    = 0;
#define RDI_NProp9_check     check_prop_config_hilo
#define RDI_NProp9_checkset  checkset_prop_config_hilo

const unsigned int           RDI_NProp10_num      = 10;
const char*                  RDI_NProp10_name     = "DiscardPolicy";
const CORBA::TypeCode_ptr    RDI_NProp10_tcde     = CORBA::_tc_short;
const CORBA::Short           RDI_NProp10_lval     = CosN::AnyOrder;
const CORBA::Short           RDI_NProp10_hval     = CosN::LifoOrder;
const CORBA::Short           RDI_NProp10_lsupval  = CosN::AnyOrder;
const CORBA::Short           RDI_NProp10_hsupval  = CosN::LifoOrder;
const RDI_UnsupPropTest      RDI_NProp10_unsuprop = RDI_UnsupPropTest_Message;
const RDI_RangeTest_short    RDI_NProp10_tstrange = RDI_RangeTest_short_impl;
const RDI_UnavPropTest       RDI_NProp10_unavprop = RDI_UnavPropTest_false;
const RDI_SetRange_short     RDI_NProp10_setrange = RDI_SetRange_short_impl;
const char*                  RDI_NProp10_supmsg    = 0;
#define RDI_NProp10_check    check_prop_config_hilo
#define RDI_NProp10_checkset checkset_prop_config_hilo

const unsigned int           RDI_NProp11_num      = 11;
const char*                  RDI_NProp11_name     = "MaximumBatchSize";
const CORBA::TypeCode_ptr    RDI_NProp11_tcde     = CORBA::_tc_long;
const CORBA::Long            RDI_NProp11_lval     = (CORBA::Long)0;
const CORBA::Long            RDI_NProp11_hval     = RDI_LONG_MAX;
// max batch size limited to event queue size, but no way to specify that here
const CORBA::Long            RDI_NProp11_lsupval  = (CORBA::Long)0;
const CORBA::Long            RDI_NProp11_hsupval  = RDI_LONG_MAX;
const RDI_UnsupPropTest      RDI_NProp11_unsuprop = RDI_UnsupPropTest_Message;
const RDI_RangeTest_long     RDI_NProp11_tstrange = RDI_RangeTest_long_impl;
const RDI_UnavPropTest       RDI_NProp11_unavprop = RDI_UnavPropTest_false;
const RDI_SetRange_long      RDI_NProp11_setrange = RDI_SetRange_long_impl;
const char*                  RDI_NProp11_supmsg    = 0;
#define RDI_NProp11_check    check_prop_config_hilo
#define RDI_NProp11_checkset checkset_prop_config_hilo

const unsigned int           RDI_NProp12_num      = 12;
const char*                  RDI_NProp12_name     = "PacingInterval";
const CORBA::TypeCode_ptr    RDI_NProp12_tcde     = TimeBase::_tc_TimeT;
const TimeBase::TimeT        RDI_NProp12_lval     = RDI_TimeT::MinTimeBaseTimeT();
const TimeBase::TimeT        RDI_NProp12_hval     = RDI_TimeT::MaxTimeBaseTimeT();
const TimeBase::TimeT        RDI_NProp12_lsupval  = RDI_TimeT::MinTimeBaseTimeT();
const TimeBase::TimeT        RDI_NProp12_hsupval  = RDI_TimeT::MaxTimeBaseTimeT();
const RDI_UnsupPropTest      RDI_NProp12_unsuprop = RDI_UnsupPropTest_Message;
const RDI_RangeTest_TimeT    RDI_NProp12_tstrange = RDI_RangeTest_TimeT_impl;
const RDI_UnavPropTest       RDI_NProp12_unavprop = RDI_UnavPropTest_false;
const RDI_SetRange_TimeT     RDI_NProp12_setrange = RDI_SetRange_TimeT_impl;
const char*                  RDI_NProp12_supmsg    = 0;
#define RDI_NProp12_check    check_prop_config_time
#define RDI_NProp12_checkset checkset_prop_config_time

// validate_nprop is used within a while loop that has the following vars in scope:
//    otype : type of entity doing the validation
//    a_qos : the current qos properties of this entity
//    r_qos, ix: property seq being validated, and index into r_qos
//    r_tc : typecode of value r_qos[ix].value
//    smap : bitmap for tracking properties seen so far
//    error, ix_e : seq for describing errors, and index into error
//    succ : bool tracking whether there have been any errors
#define validate_nprop(prop) \
     if ( RDI_STR_EQ(r_qos[ix].name, prop ## _name) ) { \
	smap.set(prop ## _num); \
	if (prop ## _unsuprop(otype)) { \
	   error[ix_e].code = CosN::UNSUPPORTED_PROPERTY; \
	   error[ix_e].name = r_qos[ix].name; \
	   ix_e += 1; succ  = 0; continue; \
	} \
	if (prop ## _unavprop(a_qos)) { \
	   error[ix_e].code = CosN::UNAVAILABLE_PROPERTY; \
           error[ix_e].name = r_qos[ix].name; \
           ix_e += 1; succ  = 0; continue; \
	} \
	if ( ! prop ## _tcde ->equivalent(r_tc) ) { \
	   error[ix_e].code = CosN::BAD_TYPE; \
	   error[ix_e].name = r_qos[ix].name; \
	   ix_e += 1; succ  = 0; continue; \
	} \
	if (prop ## _tstrange(r_qos[ix].value, prop ## _lval, prop ## _hval)) { \
	   error[ix_e].code = CosN::BAD_VALUE; \
	   error[ix_e].name = r_qos[ix].name; \
	   prop ## _setrange(error[ix_e].available_range, prop ## _lsupval, prop ## _hsupval); \
	   ix_e += 1; succ  = 0; continue; \
	} \
	if (prop ## _tstrange(r_qos[ix].value, prop ## _lsupval, prop ## _hsupval)) { \
	   error[ix_e].code = CosN::UNSUPPORTED_VALUE; \
	   error[ix_e].name = r_qos[ix].name; \
	   prop ## _setrange(error[ix_e].available_range, prop ## _lsupval, prop ## _hsupval); \
	   ix_e += 1; succ  = 0; continue; \
	} \
        continue; \
     }

// add_range is used in context with the following vars in scope:
//    otype : type of entity doing the validation
//    smap : bitmap for tracking properties seen so far
//    range, ix_e : CosN::NamedPropertyRangeSeq for storing min/max requirements, and index into range
// if the property is not in smap and is supported, it is added to range
#define add_range(prop) \
     if ( (! smap.is_set(prop ## _num)) && (! prop ## _unsuprop(otype)) ) { \
	range[ix_e].name = prop ## _name; \
	prop ## _setrange(range[ix_e].range, prop ## _lsupval, prop ## _hsupval); \
	ix_e += 1; \
     }

// check_prop_config is used in context with following vars in scope:
//     config  : RDIConfig (configuration obj)
//     str     : RDIstrstream (logging obj)
//     otype   : type of entity doing the validation
//     success : boolean tracking error status
// the args are:
//     prop    : the property
//     tvar    : a variable of appropriate type for get_value call
//     action  : statement -- action to take if prop value is OK
// It tests whether the property is set in config, and, if so, whether
//   it has the right type and values in the acceptable range.
// If there is an error, an error message is added to str and 0 is returned,
// otherwise control flow falls through to next statement.

#define check_prop_config_impl(prop, tvar, action, hilo_test) \
  switch ( config.get_value(str, prop ## _name , tvar, 1) ) { \
  case 0: { \
    if (prop ## _unsuprop(otype)) { \
      str << "Property " << prop ## _name << " cannot be modified at "; \
      switch (otype) { \
      case RDI_NFSERVER: str << "server level\n"; break; \
      case RDI_EMESSAGE: str << "message level\n"; break; \
      case RDI_ECHANNEL: str << "channel/chanfact level\n"; break; \
      case RDI_C_ADMIN: case RDI_S_ADMIN: str << "admin level\n"; break; \
      default: str << "proxy level\n"; break; \
      } \
      success = 0; \
      break; \
    } \
    if (hilo_test) { \
      if (prop ## _supmsg) { \
        str << prop ## _supmsg; \
      } else if ( prop ## _lsupval == prop ## _hsupval ) { \
        str << "Invalid " << prop ## _name << " value (" << tvar << ") : omniNotify only supports value " << \
	prop ## _lsupval << '\n' ; \
      } else { \
        str << "Invalid " << prop ## _name << " value (" << tvar << ") : omniNotify supports values between " << \
	prop ## _lsupval << " and " << prop ## _hsupval << '\n' ; \
      } \
      success = 0; \
      break; \
    } \
    action ; \
    break; } \
  case -1: \
    break; \
  default: \
    success = 0; \
    break; \
  }

#define check_prop_config_hilo(prop, tvar, action) \
  check_prop_config_impl(prop, tvar, action, ((tvar < prop ## _lsupval ) || (tvar > prop ## _hsupval)))
#define check_prop_config_hi(prop, tvar, action) \
  check_prop_config_impl(prop, tvar, action, (tvar > prop ## _hsupval))
#define check_prop_config_bool(prop, tvar, action) \
  check_prop_config_impl(prop, tvar, action, ((tvar != prop ## _lsupval ) && (tvar != prop ## _hsupval)))
#define check_prop_config_triv(prop, tvar, action) \
  check_prop_config_impl(prop, tvar, action, 0)
#define check_prop_config_triv_bool(prop, tvar, action) \
  check_prop_config_impl(prop, tvar, action, 0)

// check_prop_config_time is somewhat like check_prop_config.  some differences:
//   scope assumed to include lval, a CORBA::Long temporary variable
//   setfn is used to set a RDI_TimeT value, e.g., ntfqos.pacingInterval
//      could be used as the setfn argument

#define check_prop_config_time(prop, setfn) \
  switch ( config.get_value(str, prop ## _name , lval, 1) ) { \
  case 0: { \
    if (prop ## _unsuprop(otype)) { \
      str << "Property " << prop ## _name << " cannot be modified at "; \
      switch (otype) { \
      case RDI_NFSERVER: str << "server level\n"; break; \
      case RDI_EMESSAGE: str << "message level\n"; break; \
      case RDI_ECHANNEL: str << "channel/chanfact level\n"; break; \
      case RDI_C_ADMIN: case RDI_S_ADMIN: str << "admin level\n"; break; \
      default: str << "proxy level\n"; break; \
      } \
      success = 0; \
      break; \
    } \
    unsigned long msec = (unsigned long)lval; \
    if ((lval < 0) || (msec > (RDI_ULONG_MAX / 10000))) { \
      str << "Invalid " << prop ## _name << " value (" << lval << ") : omniNotify supports " << \
	"\n\tvalues >= 0 and < " << (unsigned long)(RDI_ULONG_MAX / 10000) << '\n'; \
      success = 0; \
      break; \
    } \
    RDI_TimeT pi; \
    pi.set_rel_msecs(msec); \
    setfn(pi); \
    break; } \
  case -1: \
    break; \
  default: \
    success = 0; \
    break; \
  }

// checkset_prop_config: like check_prop_config only
// the action (built in) is to add a property setting to
// n_qos, which should be a pointer to a CosN::QoSProperties
// sequence

#define checkset_prop_config_impl(prop, tvar, hilo_test, extract) \
  switch ( config->get_value(str, prop ## _name , tvar, 1) ) { \
  case 0: { \
    if (prop ## _unsuprop(otype)) { \
      str << "Property " << prop ## _name << " cannot be modified at "; \
      switch (otype) { \
      case RDI_NFSERVER: str << "server level\n"; break; \
      case RDI_EMESSAGE: str << "message level\n"; break; \
      case RDI_ECHANNEL: str << "channel/chanfact level\n"; break; \
      case RDI_C_ADMIN: case RDI_S_ADMIN: str << "admin level\n"; break; \
      default: str << "proxy level\n"; break; \
      } \
      success = 0; \
      break; \
    } \
    if (hilo_test) { \
      if (prop ## _supmsg) { \
        str << prop ## _supmsg; \
      } else if ( prop ## _lsupval == prop ## _hsupval ) { \
        str << "Invalid " << prop ## _name << " value (" << tvar << ") : omniNotify only supports value " << \
	prop ## _lsupval << '\n' ; \
      } else { \
        str << "Invalid " << prop ## _name << " value (" << tvar << ") : omniNotify supports values between " << \
	prop ## _lsupval << " and " << prop ## _hsupval << '\n' ; \
      } \
      success = 0; \
      break; \
    } \
    n_qos.length(++n_idx); \
    n_qos[n_idx-1].name = CORBA::string_dup( prop ## _name ); \
    extract; \
    break; } \
  case -1: \
    break; \
  default: \
    success = 0; \
    break; \
  }

#define checkset_prop_config_hilo(prop, tvar) \
  checkset_prop_config_impl(prop, tvar, ((tvar < prop ## _lsupval ) || (tvar > prop ## _hsupval)), \
                            n_qos[n_idx-1].value <<= tvar)
#define checkset_prop_config_hi(prop, tvar) \
  checkset_prop_config_impl(prop, tvar, (tvar > prop ## _hsupval), \
                            n_qos[n_idx-1].value <<= tvar)
#define checkset_prop_config_bool(prop, tvar) \
  checkset_prop_config_impl(prop, tvar, ((tvar != prop ## _lsupval ) && (tvar != prop ## _hsupval)), \
                            CORBA::Any::from_boolean from_tvar(tvar); n_qos[n_idx-1].value <<= from_tvar)
#define checkset_prop_config_triv(prop, tvar) \
  checkset_prop_config_impl(prop, tvar, 0, \
                            n_qos[n_idx-1].value <<= tvar)
#define checkset_prop_config_triv_bool(prop, tvar) \
  checkset_prop_config_impl(prop, tvar, 0, \
                            CORBA::Any::from_boolean from_tvar(tvar); n_qos[n_idx-1].value <<= from_tvar)

// checkset_prop_config_time: like check_prop_config_time only
// the action (built in) is to add a property setting to
// n_qos, which should be a pointer to a CosN::QoSProperties
// sequence

#define checkset_prop_config_time(prop) \
  switch ( config->get_value(str, prop ## _name , lval, 1) ) { \
  case 0: { \
    if (prop ## _unsuprop(otype)) { \
      str << "Property " << prop ## _name << " cannot be modified at "; \
      switch (otype) { \
      case RDI_NFSERVER: str << "server level\n"; break; \
      case RDI_EMESSAGE: str << "message level\n"; break; \
      case RDI_ECHANNEL: str << "channel/chanfact level\n"; break; \
      case RDI_C_ADMIN: case RDI_S_ADMIN: str << "admin level\n"; break; \
      default: str << "proxy level\n"; break; \
      } \
      success = 0; \
      break; \
    } \
    unsigned long msec = (unsigned long)lval; \
    if ((lval < 0) || (msec > (RDI_ULONG_MAX / 10000))) { \
      str << "Invalid " << prop ## _name << " value (" << lval << ") : omniNotify supports " << \
	"\n\tvalues >= 0 and < " << (unsigned long)(RDI_ULONG_MAX / 10000) << '\n'; \
      success = 0; \
      break; \
    } \
    RDI_TimeT tt; \
    TimeBase::TimeT tt_TimeT; \
    tt.set_rel_msecs(msec); \
    tt.get_TimeBaseTimeT(tt_TimeT); \
    n_qos.length(++n_idx); \
    n_qos[n_idx-1].name = CORBA::string_dup( prop ## _name ); \
    n_qos[n_idx-1].value <<= tt_TimeT; \
    break; } \
  case -1: \
    break; \
  default: \
    success = 0; \
    break; \
  }

////////////////////////////////////////////////////////////////////////////////
//            AdminQoS 'Table' and associated Macros
////////////////////////////////////////////////////////////////////////////////

//const unsigned int         RDI_AProp0_num       = 0;
const char*                  RDI_AProp0_name      = "MaxQueueLength";
//const CORBA::TypeCode_ptr  RDI_AProp0_tcde      = CORBA::_tc_long;
//const CORBA::Long          RDI_AProp0_lval      = (CORBA::Long)0;
//const CORBA::Long          RDI_AProp0_hval      = RDI_LONG_MAX;
const CORBA::Long            RDI_AProp0_lsupval   = (CORBA::Long)0;
const CORBA::Long            RDI_AProp0_hsupval   = RDI_LONG_MAX;
const char*                  RDI_AProp0_supmsg    = 0;
#define RDI_AProp0_check     check_aprop_config_hilo
#define RDI_AProp0_checkset  checkset_aprop_config_hilo

//const unsigned int         RDI_AProp1_num       = 1;
const char*                  RDI_AProp1_name      = "MaxConsumers";
//const CORBA::TypeCode_ptr  RDI_AProp1_tcde      = CORBA::_tc_long;
//const CORBA::Long          RDI_AProp1_lval      = (CORBA::Long)0;
//const CORBA::Long          RDI_AProp1_hval      = RDI_LONG_MAX;
const CORBA::Long            RDI_AProp1_lsupval   = (CORBA::Long)0;
const CORBA::Long            RDI_AProp1_hsupval   = RDI_LONG_MAX;
const char*                  RDI_AProp1_supmsg    = 0;
#define RDI_AProp1_check     check_aprop_config_hilo
#define RDI_AProp1_checkset  checkset_aprop_config_hilo

//const unsigned int         RDI_AProp2_num       = 2;
const char*                  RDI_AProp2_name      = "MaxSuppliers";
//const CORBA::TypeCode_ptr  RDI_AProp2_tcde      = CORBA::_tc_long;
//const CORBA::Long          RDI_AProp2_lval      = (CORBA::Long)0;
//const CORBA::Long          RDI_AProp2_hval      = RDI_LONG_MAX;
const CORBA::Long            RDI_AProp2_lsupval   = (CORBA::Long)0;
const CORBA::Long            RDI_AProp2_hsupval   = RDI_LONG_MAX;
const char*                  RDI_AProp2_supmsg    = 0;
#define RDI_AProp2_check     check_aprop_config_hilo
#define RDI_AProp2_checkset  checkset_aprop_config_hilo

//const unsigned int         RDI_AProp3_num       = 3;
const char*                  RDI_AProp3_name      = "RejectNewEvents";
//const CORBA::TypeCode_ptr  RDI_AProp3_tcde      = CORBA::_tc_boolean;
//const CORBA::Boolean       RDI_AProp3_lval      = (CORBA::Boolean)0;
//const CORBA::Boolean       RDI_AProp3_hval      = (CORBA::Boolean)1;
const CORBA::Boolean         RDI_AProp3_lsupval   = (CORBA::Boolean)0;
const CORBA::Boolean         RDI_AProp3_hsupval   = (CORBA::Boolean)1;
const char*                  RDI_AProp3_supmsg    = 0;
#define RDI_AProp3_check     check_aprop_config_bool
#define RDI_AProp3_checkset  checkset_aprop_config_bool

#define check_aprop_config_impl(prop, tvar, action, hilo_test) \
  switch ( config.get_value(str, prop ## _name , tvar, 1) ) { \
  case 0: { \
    if ( otype != RDI_ECHANNEL ) { \
      str << "AdminQoS Property " << prop ## _name << " cannot be modified at "; \
      switch (otype) { \
      case RDI_NFSERVER: str << "server level\n"; break; \
      case RDI_EMESSAGE: str << "message level\n"; break; \
      case RDI_C_ADMIN: case RDI_S_ADMIN: str << "admin level\n"; break; \
      default: str << "proxy level\n"; break; \
      } \
      success = 0; \
      break; \
    } \
    if (hilo_test) { \
      if (prop ## _supmsg) { \
        str << prop ## _supmsg; \
      } else if ( prop ## _lsupval == prop ## _hsupval ) { \
        str << "Invalid " << prop ## _name << " value (" << tvar << ") : omniNotify only supports value " << \
	prop ## _lsupval << '\n' ; \
      } else { \
        str << "Invalid " << prop ## _name << " value (" << tvar << ") : omniNotify supports values between " << \
	prop ## _lsupval << " and " << prop ## _hsupval << '\n' ; \
      } \
      success = 0; \
      break; \
    } \
    action ; \
    break; } \
  case -1: \
    break; \
  default: \
    success = 0; \
    break; \
  }

#define check_aprop_config_hilo(prop, tvar, action) \
  check_aprop_config_impl(prop, tvar, action, ((tvar < prop ## _lsupval ) || (tvar > prop ## _hsupval)))
#define check_aprop_config_hi(prop, tvar, action) \
  check_aprop_config_impl(prop, tvar, action, (tvar > prop ## _hsupval))
#define check_aprop_config_bool(prop, tvar, action) \
  check_aprop_config_impl(prop, tvar, action, ((tvar != prop ## _lsupval ) && (tvar != prop ## _hsupval)))
#define check_aprop_config_triv(prop, tvar, action) \
  check_aprop_config_impl(prop, tvar, action, 0)
#define check_aprop_config_triv_bool(prop, tvar, action) \
  check_aprop_config_impl(prop, tvar, action, 0)

#define checkset_aprop_config_impl(prop, tvar, hilo_test, extract) \
  switch ( config->get_value(str, prop ## _name , tvar, 1) ) { \
  case 0: { \
    if ( otype != RDI_ECHANNEL ) { \
      str << "AdminQoS Property " << prop ## _name << " cannot be modified at "; \
      switch (otype) { \
      case RDI_NFSERVER: str << "server level\n"; break; \
      case RDI_EMESSAGE: str << "message level\n"; break; \
      case RDI_C_ADMIN: case RDI_S_ADMIN: str << "admin level\n"; break; \
      default: str << "proxy level\n"; break; \
      } \
      success = 0; \
      break; \
    } \
    if (hilo_test) { \
      if (prop ## _supmsg) { \
        str << prop ## _supmsg; \
      } else if ( prop ## _lsupval == prop ## _hsupval ) { \
        str << "Invalid " << prop ## _name << " value (" << tvar << ") : omniNotify only supports value " << \
	prop ## _lsupval << '\n' ; \
      } else { \
        str << "Invalid " << prop ## _name << " value (" << tvar << ") : omniNotify supports values between " << \
	prop ## _lsupval << " and " << prop ## _hsupval << '\n' ; \
      } \
      success = 0; \
      break; \
    } \
    a_qos.length(++a_idx); \
    a_qos[a_idx-1].name = CORBA::string_dup( prop ## _name ); \
    extract; \
    break; } \
  case -1: \
    break; \
  default: \
    success = 0; \
    break; \
  }

#define checkset_aprop_config_hilo(prop, tvar) \
  checkset_aprop_config_impl(prop, tvar, ((tvar < prop ## _lsupval ) || (tvar > prop ## _hsupval)), \
                             a_qos[a_idx-1].value <<= tvar)
#define checkset_aprop_config_hi(prop, tvar) \
  checkset_aprop_config_impl(prop, tvar, (tvar > prop ## _hsupval), \
                             a_qos[a_idx-1].value <<= tvar)
#define checkset_aprop_config_bool(prop, tvar) \
  checkset_aprop_config_impl(prop, tvar, ((tvar != prop ## _lsupval ) && (tvar != prop ## _hsupval)), \
                             CORBA::Any::from_boolean from_tvar(tvar); a_qos[a_idx-1].value <<= from_tvar)
#define checkset_aprop_config_triv(prop, tvar) \
  checkset_aprop_config_impl(prop, tvar, 0, \
                             a_qos[a_idx-1].value <<= tvar)
#define checkset_aprop_config_triv_bool(prop, tvar) \
  checkset_aprop_config_impl(prop, tvar, 0, \
                             CORBA::Any::from_boolean from_tvar(tvar); a_qos[a_idx-1].value <<= from_tvar)

////////////////////////////////////////////////////////////////////////////////
//            ServerQoS 'Table' and associated Macros
////////////////////////////////////////////////////////////////////////////////

#ifndef NO_OBJ_GC
#  define RDI_NUM_SPROPS 21
#else
#  define RDI_NUM_SPROPS 15
#endif

//const unsigned int         RDI_SProp0_num       = 0;
const char*                  RDI_SProp0_name      = "NumAdminGroups";
//const CORBA::TypeCode_ptr  RDI_SProp0_tcde      = CORBA::_tc_ushort;
//const CORBA::UShort        RDI_SProp0_lval      = (CORBA::UShort)0;
//const CORBA::UShort        RDI_SProp0_hval      = RDI_USHORT_MAX;
const CORBA::UShort          RDI_SProp0_lsupval   = (CORBA::UShort)1;
const CORBA::UShort          RDI_SProp0_hsupval   = RDI_USHORT_MAX;
const char*                  RDI_SProp0_supmsg    = 0;
#define RDI_SProp0_check     check_sprop_config_hilo
#define RDI_SProp0_checkset  checkset_sprop_config_hilo

//const unsigned int         RDI_SProp1_num       = 1;
const char*                  RDI_SProp1_name      = "NumAdminThreads";
//const CORBA::TypeCode_ptr  RDI_SProp1_tcde      = CORBA::_tc_ushort;
//const CORBA::UShort        RDI_SProp1_lval      = (CORBA::UShort)0;
//const CORBA::UShort        RDI_SProp1_hval      = RDI_USHORT_MAX;
const CORBA::UShort          RDI_SProp1_lsupval   = (CORBA::UShort)1;
const CORBA::UShort          RDI_SProp1_hsupval   = RDI_USHORT_MAX;
const char*                  RDI_SProp1_supmsg    = 0;
#define RDI_SProp1_check     check_sprop_config_hilo
#define RDI_SProp1_checkset  checkset_sprop_config_hilo

//const unsigned int         RDI_SProp2_num       = 2;
const char*                  RDI_SProp2_name      = "NumProxyThreads";
//const CORBA::TypeCode_ptr  RDI_SProp2_tcde      = CORBA::_tc_ushort;
//const CORBA::UShort        RDI_SProp2_lval      = (CORBA::UShort)0;
//const CORBA::UShort        RDI_SProp2_hval      = RDI_USHORT_MAX;
const CORBA::UShort          RDI_SProp2_lsupval   = (CORBA::UShort)0;
const CORBA::UShort          RDI_SProp2_hsupval   = RDI_USHORT_MAX;
const char*                  RDI_SProp2_supmsg    = 0;
#define RDI_SProp2_check     check_sprop_config_hi
#define RDI_SProp2_checkset  checkset_sprop_config_hi

//const unsigned int         RDI_SProp3_num       = 3;
const char*                  RDI_SProp3_name      = "NumPushThreads";
//const CORBA::TypeCode_ptr  RDI_SProp3_tcde      = CORBA::_tc_ushort;
//const CORBA::UShort        RDI_SProp3_lval      = (CORBA::UShort)0;
//const CORBA::UShort        RDI_SProp3_hval      = RDI_USHORT_MAX;
const CORBA::UShort          RDI_SProp3_lsupval   = (CORBA::UShort)0;
const CORBA::UShort          RDI_SProp3_hsupval   = RDI_USHORT_MAX;
const char*                  RDI_SProp3_supmsg    = 0;
#define RDI_SProp3_check     check_sprop_config_hi
#define RDI_SProp3_checkset  checkset_sprop_config_hi

//const unsigned int         RDI_SProp4_num       = 4;
const char*                  RDI_SProp4_name      = "NumPullThreads";
//const CORBA::TypeCode_ptr  RDI_SProp4_tcde      = CORBA::_tc_ushort;
//const CORBA::UShort        RDI_SProp4_lval      = (CORBA::UShort)0;
//const CORBA::UShort        RDI_SProp4_hval      = RDI_USHORT_MAX;
const CORBA::UShort          RDI_SProp4_lsupval   = (CORBA::UShort)0;
const CORBA::UShort          RDI_SProp4_hsupval   = RDI_USHORT_MAX;
const char*                  RDI_SProp4_supmsg    = 0;
#define RDI_SProp4_check     check_sprop_config_hi
#define RDI_SProp4_checkset  checkset_sprop_config_hi

//const unsigned int         RDI_SProp5_num       = 5;
const char*                  RDI_SProp5_name      = "NumSChangeThreads";
//const CORBA::TypeCode_ptr  RDI_SProp5_tcde      = CORBA::_tc_ushort;
//const CORBA::UShort        RDI_SProp5_lval      = (CORBA::UShort)0;
//const CORBA::UShort        RDI_SProp5_hval      = RDI_USHORT_MAX;
const CORBA::UShort          RDI_SProp5_lsupval   = (CORBA::UShort)0;
const CORBA::UShort          RDI_SProp5_hsupval   = RDI_USHORT_MAX;
const char*                  RDI_SProp5_supmsg    = 0;
#define RDI_SProp5_check     check_sprop_config_hi
#define RDI_SProp5_checkset  checkset_sprop_config_hi

//const unsigned int         RDI_SProp6_num       = 6;
const char*                  RDI_SProp6_name      = "NumOChangeThreads";
//const CORBA::TypeCode_ptr  RDI_SProp6_tcde      = CORBA::_tc_ushort;
//const CORBA::UShort        RDI_SProp6_lval      = (CORBA::UShort)0;
//const CORBA::UShort        RDI_SProp6_hval      = RDI_USHORT_MAX;
const CORBA::UShort          RDI_SProp6_lsupval   = (CORBA::UShort)0;
const CORBA::UShort          RDI_SProp6_hsupval   = RDI_USHORT_MAX;
const char*                  RDI_SProp6_supmsg    = 0;
#define RDI_SProp6_check     check_sprop_config_hi
#define RDI_SProp6_checkset  checkset_sprop_config_hi

//const unsigned int         RDI_SProp7_num       = 7;
const char*                  RDI_SProp7_name      = "PullEventPeriod";
//const CORBA::TypeCode_ptr  RDI_SProp7_tcde      = CORBA::_tc_ulong;
//const CORBA::ULong         RDI_SProp7_lval      = (CORBA::ULong)0;
//const CORBA::ULong         RDI_SProp7_hval      = RDI_ULONG_MAX;
const CORBA::ULong           RDI_SProp7_lsupval   = (CORBA::ULong)0;
const CORBA::ULong           RDI_SProp7_hsupval   = RDI_ULONG_MAX;
const char*                  RDI_SProp7_supmsg    = 0;
#define RDI_SProp7_check     check_sprop_config_hi
#define RDI_SProp7_checkset  checkset_sprop_config_hi

//const unsigned int         RDI_SProp8_num       = 8;
const char*                  RDI_SProp8_name      = "QueueGCPeriod";
//const CORBA::TypeCode_ptr  RDI_SProp8_tcde      = CORBA::_tc_ulong;
//const CORBA::ULong         RDI_SProp8_lval      = (CORBA::ULong)0;
//const CORBA::ULong         RDI_SProp8_hval      = RDI_ULONG_MAX;
const CORBA::ULong           RDI_SProp8_lsupval   = (CORBA::ULong)1;
const CORBA::ULong           RDI_SProp8_hsupval   = RDI_ULONG_MAX;
const char*                  RDI_SProp8_supmsg    = 0;
#define RDI_SProp8_check     check_sprop_config_hilo
#define RDI_SProp8_checkset  checkset_sprop_config_hilo

//const unsigned int         RDI_SProp9_num       = 9;
const char*                  RDI_SProp9_name      = "ReportingInterval";
//const CORBA::TypeCode_ptr  RDI_SProp9_tcde      = CORBA::_tc_ulong;
//const CORBA::ULong         RDI_SProp9_lval      = (CORBA::ULong)0;
//const CORBA::ULong         RDI_SProp9_hval      = RDI_ULONG_MAX;
const CORBA::ULong           RDI_SProp9_lsupval   = (CORBA::ULong)0;
const CORBA::ULong           RDI_SProp9_hsupval   = RDI_ULONG_MAX;
const char*                  RDI_SProp9_supmsg    = 0;
#define RDI_SProp9_check     check_sprop_config_hi
#define RDI_SProp9_checkset  checkset_sprop_config_hi

//const unsigned int         RDI_SProp10_num      = 10;
const char*                  RDI_SProp10_name     = "LocalClockInaccLo";
//const CORBA::TypeCode_ptr  RDI_SProp10_tcde     = CORBA::_tc_ulong;
//const CORBA::ULong         RDI_SProp10_lval     = (CORBA::ULong)0;
//const CORBA::ULong         RDI_SProp10_hval     = RDI_ULONG_MAX;
const CORBA::ULong           RDI_SProp10_lsupval  = (CORBA::ULong)0;
const CORBA::ULong           RDI_SProp10_hsupval  = RDI_ULONG_MAX;
const char*                  RDI_SProp10_supmsg   = 0;
#define RDI_SProp10_check    check_sprop_config_hi
#define RDI_SProp10_checkset checkset_sprop_config_hi

//const unsigned int         RDI_SProp11_num      = 11;
const char*                  RDI_SProp11_name     = "LocalClockInaccHi";
//const CORBA::TypeCode_ptr  RDI_SProp11_tcde     = CORBA::_tc_ushort;
//const CORBA::UShort        RDI_SProp11_lval     = (CORBA::UShort)0;
//const CORBA::UShort        RDI_SProp11_hval     = RDI_USHORT_MAX;
const CORBA::UShort          RDI_SProp11_lsupval  = (CORBA::UShort)0;
const CORBA::UShort          RDI_SProp11_hsupval  = RDI_USHORT_MAX;
const char*                  RDI_SProp11_supmsg   = 0;
#define RDI_SProp11_check    check_sprop_config_hi
#define RDI_SProp11_checkset checkset_sprop_config_hi

//const unsigned int         RDI_SProp12_num      = 12;
const char*                  RDI_SProp12_name     = "LocalClockTDF";
//const CORBA::TypeCode_ptr  RDI_SProp12_tcde     = CORBA::_tc_short;
//const CORBA::Short         RDI_SProp12_lval     = RDI_SHORT_MIN;
//const CORBA::Short         RDI_SProp12_hval     = RDI_SHORT_MAX;
const CORBA::Short           RDI_SProp12_lsupval  = RDI_SHORT_MIN;
const CORBA::Short           RDI_SProp12_hsupval  = RDI_SHORT_MAX;
const char*                  RDI_SProp12_supmsg   = 0;
#define RDI_SProp12_check    check_sprop_config_triv
#define RDI_SProp12_checkset checkset_sprop_config_triv

//const unsigned int         RDI_SProp13_num       = 13;
const char*                  RDI_SProp13_name      = "OutgoingTimeout";
//const CORBA::TypeCode_ptr  RDI_SProp13_tcde      = CORBA::_tc_ulong;
//const CORBA::ULong         RDI_SProp13_lval      = (CORBA::ULong)0;
//const CORBA::ULong         RDI_SProp13_hval      = RDI_ULONG_MAX;
const CORBA::ULong           RDI_SProp13_lsupval   = (CORBA::ULong)0;
const CORBA::ULong           RDI_SProp13_hsupval   = RDI_ULONG_MAX;
const char*                  RDI_SProp13_supmsg    = 0;
#define RDI_SProp13_check    check_sprop_config_hi
#define RDI_SProp13_checkset checkset_sprop_config_hi

//const unsigned int         RDI_SProp14_num       = 14;
const char*                  RDI_SProp14_name      = "IncomingTimeout";
//const CORBA::TypeCode_ptr  RDI_SProp14_tcde      = CORBA::_tc_ulong;
//const CORBA::ULong         RDI_SProp14_lval      = (CORBA::ULong)0;
//const CORBA::ULong         RDI_SProp14_hval      = RDI_ULONG_MAX;
const CORBA::ULong           RDI_SProp14_lsupval   = (CORBA::ULong)0;
const CORBA::ULong           RDI_SProp14_hsupval   = RDI_ULONG_MAX;
const char*                  RDI_SProp14_supmsg    = 0;
#define RDI_SProp14_check    check_sprop_config_hi
#define RDI_SProp14_checkset checkset_sprop_config_hi

#ifndef NO_OBJ_GC
//const unsigned int         RDI_SProp15_num       = 15;
const char*                  RDI_SProp15_name      = "ObjectGCPeriod";
//const CORBA::TypeCode_ptr  RDI_SProp15_tcde      = CORBA::_tc_ulong;
//const CORBA::ULong         RDI_SProp15_lval      = (CORBA::ULong)0;
//const CORBA::ULong         RDI_SProp15_hval      = RDI_ULONG_MAX;
const CORBA::ULong           RDI_SProp15_lsupval   = (CORBA::ULong)0;
const CORBA::ULong           RDI_SProp15_hsupval   = RDI_ULONG_MAX;
const char*                  RDI_SProp15_supmsg    = 0;
#define RDI_SProp15_check    check_sprop_config_hi
#define RDI_SProp15_checkset checkset_sprop_config_hi

//const unsigned int         RDI_SProp16_num       = 16;
const char*                  RDI_SProp16_name      = "DeadChanInterval";
//const CORBA::TypeCode_ptr  RDI_SProp16_tcde      = CORBA::_tc_ulong;
//const CORBA::ULong         RDI_SProp16_lval      = (CORBA::ULong)0;
//const CORBA::ULong         RDI_SProp16_hval      = RDI_ULONG_MAX;
const CORBA::ULong           RDI_SProp16_lsupval   = (CORBA::ULong)0;
const CORBA::ULong           RDI_SProp16_hsupval   = RDI_ULONG_MAX;
const char*                  RDI_SProp16_supmsg    = 0;
#define RDI_SProp16_check    check_sprop_config_hi
#define RDI_SProp16_checkset checkset_sprop_config_hi

//const unsigned int         RDI_SProp17_num       = 17;
const char*                  RDI_SProp17_name      = "DeadAdminInterval";
//const CORBA::TypeCode_ptr  RDI_SProp17_tcde      = CORBA::_tc_ulong;
//const CORBA::ULong         RDI_SProp17_lval      = (CORBA::ULong)0;
//const CORBA::ULong         RDI_SProp17_hval      = RDI_ULONG_MAX;
const CORBA::ULong           RDI_SProp17_lsupval   = (CORBA::ULong)0;
const CORBA::ULong           RDI_SProp17_hsupval   = RDI_ULONG_MAX;
const char*                  RDI_SProp17_supmsg    = 0;
#define RDI_SProp17_check    check_sprop_config_hi
#define RDI_SProp17_checkset checkset_sprop_config_hi

//const unsigned int         RDI_SProp18_num       = 18;
const char*                  RDI_SProp18_name      = "DeadConProxyInterval";
//const CORBA::TypeCode_ptr  RDI_SProp18_tcde      = CORBA::_tc_ulong;
//const CORBA::ULong         RDI_SProp18_lval      = (CORBA::ULong)0;
//const CORBA::ULong         RDI_SProp18_hval      = RDI_ULONG_MAX;
const CORBA::ULong           RDI_SProp18_lsupval   = (CORBA::ULong)0;
const CORBA::ULong           RDI_SProp18_hsupval   = RDI_ULONG_MAX;
const char*                  RDI_SProp18_supmsg    = 0;
#define RDI_SProp18_check    check_sprop_config_hi
#define RDI_SProp18_checkset checkset_sprop_config_hi

//const unsigned int         RDI_SProp19_num       = 19;
const char*                  RDI_SProp19_name      = "DeadOtherProxyInterval";
//const CORBA::TypeCode_ptr  RDI_SProp19_tcde      = CORBA::_tc_ulong;
//const CORBA::ULong         RDI_SProp19_lval      = (CORBA::ULong)0;
//const CORBA::ULong         RDI_SProp19_hval      = RDI_ULONG_MAX;
const CORBA::ULong           RDI_SProp19_lsupval   = (CORBA::ULong)0;
const CORBA::ULong           RDI_SProp19_hsupval   = RDI_ULONG_MAX;
const char*                  RDI_SProp19_supmsg    = 0;
#define RDI_SProp19_check    check_sprop_config_hi
#define RDI_SProp19_checkset checkset_sprop_config_hi

//const unsigned int         RDI_SProp20_num       = 20;
const char*                  RDI_SProp20_name      = "DeadFilterInterval";
//const CORBA::TypeCode_ptr  RDI_SProp20_tcde      = CORBA::_tc_ulong;
//const CORBA::ULong         RDI_SProp20_lval      = (CORBA::ULong)0;
//const CORBA::ULong         RDI_SProp20_hval      = RDI_ULONG_MAX;
const CORBA::ULong           RDI_SProp20_lsupval   = (CORBA::ULong)0;
const CORBA::ULong           RDI_SProp20_hsupval   = RDI_ULONG_MAX;
const char*                  RDI_SProp20_supmsg    = 0;
#define RDI_SProp20_check    check_sprop_config_hi
#define RDI_SProp20_checkset checkset_sprop_config_hi
#endif

#define check_sprop_config_impl(prop, tvar, action, hilo_test) \
  switch ( config.get_value(str, prop ## _name , tvar, 1) ) { \
  case 0: { \
    if (hilo_test) { \
      if (prop ## _supmsg) { \
        str << prop ## _supmsg; \
      } else if ( prop ## _lsupval == prop ## _hsupval ) { \
        str << "Invalid " << prop ## _name << " value (" << tvar << ") : omniNotify only supports value " << \
	prop ## _lsupval << '\n' ; \
      } else { \
        str << "Invalid " << prop ## _name << " value (" << tvar << ") : omniNotify supports values between " << \
	prop ## _lsupval << " and " << prop ## _hsupval << '\n' ; \
      } \
      success = 0; \
      break; \
    } \
    action ; \
    break; } \
  case -1: \
    break; \
  default: \
    success = 0; \
    break; \
  }

#define check_sprop_config_hilo(prop, tvar, action) \
  check_sprop_config_impl(prop, tvar, action, ((tvar < prop ## _lsupval ) || (tvar > prop ## _hsupval)))
#define check_sprop_config_hi(prop, tvar, action) \
  check_sprop_config_impl(prop, tvar, action, (tvar > prop ## _hsupval))
#define check_sprop_config_bool(prop, tvar, action) \
  check_sprop_config_impl(prop, tvar, action, ((tvar != prop ## _lsupval ) && (tvar != prop ## _hsupval)))
#define check_sprop_config_triv(prop, tvar, action) \
  check_sprop_config_impl(prop, tvar, action, 0)
#define check_sprop_config_triv_bool(prop, tvar, action) \
  check_sprop_config_impl(prop, tvar, action, 0)

#define checkset_sprop_config_impl(prop, tvar, hilo_test, extract) \
  switch ( config->get_value(str, prop ## _name , tvar, 1) ) { \
  case 0: { \
    if (hilo_test) { \
      if (prop ## _supmsg) { \
        str << prop ## _supmsg; \
      } else if ( prop ## _lsupval == prop ## _hsupval ) { \
        str << "Invalid " << prop ## _name << " value (" << tvar << ") : omniNotify only supports value " << \
	prop ## _lsupval << '\n' ; \
      } else { \
        str << "Invalid " << prop ## _name << " value (" << tvar << ") : omniNotify supports values between " << \
	prop ## _lsupval << " and " << prop ## _hsupval << '\n' ; \
      } \
      success = 0; \
      break; \
    } \
    s_qos.length(++s_idx); \
    s_qos[s_idx-1].name = CORBA::string_dup( prop ## _name ); \
    extract; \
    break; } \
  case -1: \
    break; \
  default: \
    success = 0; \
    break; \
  }

#define checkset_sprop_config_hilo(prop, tvar) \
  checkset_sprop_config_impl(prop, tvar, ((tvar < prop ## _lsupval ) || (tvar > prop ## _hsupval)), \
                             s_qos[s_idx-1].value <<= tvar)
#define checkset_sprop_config_hi(prop, tvar) \
  checkset_sprop_config_impl(prop, tvar, (tvar > prop ## _hsupval), \
                             s_qos[s_idx-1].value <<= tvar)
#define checkset_sprop_config_bool(prop, tvar) \
  checkset_sprop_config_impl(prop, tvar, ((tvar != prop ## _lsupval ) && (tvar != prop ## _hsupval)), \
                             CORBA::Any::from_boolean from_tvar(tvar); s_qos[s_idx-1].value <<= from_tvar)
#define checkset_sprop_config_triv(prop, tvar) \
  checkset_sprop_config_impl(prop, tvar, 0, \
                             s_qos[s_idx-1].value <<= tvar)
#define checkset_sprop_config_triv_bool(prop, tvar) \
  checkset_sprop_config_impl(prop, tvar, 0, \
                             CORBA::Any::from_boolean from_tvar(tvar); s_qos[s_idx-1].value <<= from_tvar)

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//                               QoS Properties                          //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

// The specification says AnyOrder is the default for
// the two ordering properties.  We now support FifoOrder, PriorityOrder
// and DeadlineOrder.
RDI_NotifQoS::RDI_NotifQoS(RDI_NotifQoS* parent) :
  _parent(parent), _all_inherited(1), _eventReliability_set(0), _connectionReliability_set(0), 
  _priority_set(0), _timeout_set(0), _startTimeSupported_set(0), 
  _stopTimeSupported_set(0), _orderPolicy_set(0), _discardPolicy_set(0),
  _pacingInterval_set(0), _maxEventsPerConsumer_set(0), _maximumBatchSize_set(0)
{
  if (!_parent) {
    _all_inherited = 0;
    eventReliability(CosN::BestEffort);
    connectionReliability(CosN::BestEffort);
    priority(CosN::DefaultPriority);
    startTimeSupported(0);
    stopTimeSupported(0);
    orderPolicy(CosN::FifoOrder);
    discardPolicy(CosN::FifoOrder);
    maxEventsPerConsumer(0);
    maximumBatchSize(8);

    _timeout.set_rel_msecs(0);
    // default pacing interval is 1 second
    _pacingInterval.set_rel_secs(1);

    _eventReliability_set = 1;
    _connectionReliability_set = 1;
    _priority_set = 1;
    _timeout_set = 1;
    _startTimeSupported_set = 1;
    _stopTimeSupported_set = 1;
    _orderPolicy_set = 1;
    _discardPolicy_set = 1;
    _pacingInterval_set = 1;
    _maxEventsPerConsumer_set = 1;
    _maximumBatchSize_set = 1;
  }
}

RDI_NotifQoS& RDI_NotifQoS::operator= (const RDI_NotifQoS& qos)
{
  _eventReliability      	= qos._eventReliability;
  _connectionReliability 	= qos._connectionReliability;
  _priority              	= qos._priority;
  _timeout               	= qos._timeout;
  _startTimeSupported    	= qos._startTimeSupported;
  _stopTimeSupported     	= qos._stopTimeSupported;
  _orderPolicy           	= qos._orderPolicy;
  _discardPolicy         	= qos._discardPolicy;
  _pacingInterval        	= qos._pacingInterval;
  _maxEventsPerConsumer  	= qos._maxEventsPerConsumer;
  _maximumBatchSize      	= qos._maximumBatchSize;

  _eventReliability_set      	= qos._eventReliability_set;
  _connectionReliability_set 	= qos._connectionReliability_set;
  _priority_set              	= qos._priority_set;
  _timeout_set               	= qos._timeout_set;
  _startTimeSupported_set    	= qos._startTimeSupported_set;
  _stopTimeSupported_set     	= qos._stopTimeSupported_set;
  _orderPolicy_set           	= qos._orderPolicy_set;
  _discardPolicy_set         	= qos._discardPolicy_set;
  _pacingInterval_set        	= qos._pacingInterval_set;
  _maxEventsPerConsumer_set  	= qos._maxEventsPerConsumer_set;
  _maximumBatchSize_set      	= qos._maximumBatchSize_set;

  _parent                	= qos._parent;
  _all_inherited                = qos._all_inherited;

  return *this;
}

CORBA::Boolean
RDI_NotifQoS::is_qos_prop(const char* pname) {
  return ( RDI_STR_EQ(pname, RDI_NProp0_name)  ||
	   RDI_STR_EQ(pname, RDI_NProp1_name)  ||  
	   RDI_STR_EQ(pname, RDI_NProp2_name)  ||  
	   RDI_STR_EQ(pname, RDI_NProp3_name)  ||  
	   RDI_STR_EQ(pname, RDI_NProp4_name)  ||  
	   RDI_STR_EQ(pname, RDI_NProp5_name)  ||  
	   RDI_STR_EQ(pname, RDI_NProp6_name)  ||  
	   RDI_STR_EQ(pname, RDI_NProp7_name)  ||  
	   RDI_STR_EQ(pname, RDI_NProp8_name)  ||  
	   RDI_STR_EQ(pname, RDI_NProp9_name)  ||  
	   RDI_STR_EQ(pname, RDI_NProp10_name) ||  
	   RDI_STR_EQ(pname, RDI_NProp11_name) ||  
	   RDI_STR_EQ(pname, RDI_NProp12_name) );
}

CosN::QoSProperties* 
RDI_NotifQoS::get_qos(RDI_ObjectKind otype) const
{
  CosN::QoSProperties* qos = new CosN::QoSProperties();
  RDI_Assert(qos, "Memory allocation failed - CosN::QoSProperties\n");
  CORBA::ULong idx=0;
  CORBA::Any::from_boolean stop(startTimeSupported());
  CORBA::Any::from_boolean start(stopTimeSupported());

  if ( otype == RDI_ECHANNEL ) {
    qos->length(RDI_NotifQoS::NUM_PROPERTIES);
    (*qos)[idx].name = CORBA::string_dup(CosN::EventReliability);
    (*qos)[idx++].value <<= eventReliability();
  } else {
    qos->length(RDI_NotifQoS::NUM_PROPERTIES - 1);
  }
  (*qos)[idx].name = CORBA::string_dup(CosN::ConnectionReliability);
  (*qos)[idx++].value <<= connectionReliability();
  (*qos)[idx].name = CORBA::string_dup(CosN::Priority);
  (*qos)[idx++].value <<= priority();
  (*qos)[idx].name = CORBA::string_dup(CosN::Timeout);
  TimeBase::TimeT to;
  timeout_timebase(to);
  (*qos)[idx++].value <<= to;
  (*qos)[idx].name = CORBA::string_dup(CosN::StartTimeSupported);
  (*qos)[idx++].value <<= start;
  (*qos)[idx].name = CORBA::string_dup(CosN::StopTimeSupported);
  (*qos)[idx++].value <<= stop;
  (*qos)[idx].name = CORBA::string_dup(CosN::OrderPolicy);
  (*qos)[idx++].value <<= orderPolicy();
  (*qos)[idx].name = CORBA::string_dup(CosN::DiscardPolicy);
  (*qos)[idx++].value <<= discardPolicy();
  (*qos)[idx].name = CORBA::string_dup(CosN::PacingInterval);
  TimeBase::TimeT pi;
  pacingInterval_timebase(pi);
  (*qos)[idx++].value <<= pi;
  (*qos)[idx].name = CORBA::string_dup(CosN::MaxEventsPerConsumer);
  (*qos)[idx++].value <<= maxEventsPerConsumer();
  (*qos)[idx].name = CORBA::string_dup(CosN::MaximumBatchSize);
  (*qos)[idx++].value <<= maximumBatchSize();
  return qos;
}

void RDI_NotifQoS::set_qos(const CosN::QoSProperties& r_qos)
{
  CORBA::Short s;
  CORBA::Long l;
  CORBA::Boolean b_start;
  CORBA::Boolean b_stop;
  
  for (CORBA::ULong ix=0; ix < r_qos.length(); ix++) {
    if ( RDI_STR_EQ(r_qos[ix].name, "EventReliability") ) {
      r_qos[ix].value >>= s;
      eventReliability(s); 
    } else if ( RDI_STR_EQ(r_qos[ix].name, "ConnectionReliability") ) {
      r_qos[ix].value >>= s;
      connectionReliability(s);
    } else if ( RDI_STR_EQ(r_qos[ix].name, "Priority") ) {
      r_qos[ix].value >>= s;
      priority(s);
    } else if ( RDI_STR_EQ(r_qos[ix].name, "Timeout") ) {
#ifdef TIMEBASE_NOLONGLONG
      const TimeBase::TimeT* tmout_ptr;
      r_qos[ix].value >>= tmout_ptr;
      timeout(*tmout_ptr);
#else
      TimeBase::TimeT tmout;
      r_qos[ix].value >>= tmout;
      timeout(tmout);
#endif
    } else if ( RDI_STR_EQ(r_qos[ix].name, "StartTimeSupported") ) {
      CORBA::Any::to_boolean strt(b_start);
      r_qos[ix].value >>= strt;
      startTimeSupported(b_start);
    } else if ( RDI_STR_EQ(r_qos[ix].name, "StopTimeSupported") ) {
      CORBA::Any::to_boolean stop(b_stop);
      r_qos[ix].value >>= stop;
      stopTimeSupported(b_stop);
    } else if ( RDI_STR_EQ(r_qos[ix].name, "OrderPolicy") ) {
      r_qos[ix].value >>= s;
      orderPolicy(s);
    } else if ( RDI_STR_EQ(r_qos[ix].name, "DiscardPolicy") ) {
      r_qos[ix].value >>= s;
      discardPolicy(s);
    } else if ( RDI_STR_EQ(r_qos[ix].name, "PacingInterval") ) {
#ifdef TIMEBASE_NOLONGLONG
      const TimeBase::TimeT* pcint_ptr;
      r_qos[ix].value >>= pcint_ptr;
      pacingInterval(*pcint_ptr);
#else
      TimeBase::TimeT pcint;
      r_qos[ix].value >>= pcint;
      pacingInterval(pcint);
#endif
    } else if ( RDI_STR_EQ(r_qos[ix].name, "MaxEventsPerConsumer") ) {
      r_qos[ix].value >>= l;
      maxEventsPerConsumer(l);
    } else if ( RDI_STR_EQ(r_qos[ix].name, "MaximumBatchSize") ) {
      r_qos[ix].value >>= l;
      maximumBatchSize(l);
    }
  }
}

/** RDI_NotifQoS::validate
 *
 * Validate the requested QoS properties, 'r_qos', for a given object.
 * The current QoS properties of this object are 'a_qos', and its kind
 * is 'otype'.  In case the object has a parent, the QoS properties of
 * its parent, 'p_qos', are used to resolve conflicts.  Since some QoS
 * properties cannot be changed once set for objects that manage other
 * objects, 'subobjs' is TRUE when the current object has sub-objects.
 */

CORBA::Boolean
RDI_NotifQoS::validate(const CosN::QoSProperties& r_qos,
		       const RDI_NotifQoS&  a_qos,
		       RDI_ObjectKind otype,
		       CosN::PropertyErrorSeq& error,
		       CosN::NamedPropertyRangeSeq& range,
		       CORBA::Boolean subobjs)
{
  //  const RDI_NotifQoS * p_qos = a_qos.parent(); NOT NEEDED AT THE MOMENT
  CORBA::Boolean succ=1;
  CORBA::ULong   ix_e=0;
  RDI_Bitmap     smap(RDI_NotifQoS::NUM_PROPERTIES);

  smap.clear();
  range.length(RDI_NotifQoS::NUM_PROPERTIES);
  error.length(r_qos.length());

  for (CORBA::ULong ix=0; ix < r_qos.length(); ix++) {
    CORBA::TypeCode_var r_tc = r_qos[ix].value.type();
    validate_nprop(RDI_NProp0);
    validate_nprop(RDI_NProp1);
    validate_nprop(RDI_NProp2);
    validate_nprop(RDI_NProp3);
    validate_nprop(RDI_NProp4);
    validate_nprop(RDI_NProp5);
    validate_nprop(RDI_NProp6);
    validate_nprop(RDI_NProp7);
    validate_nprop(RDI_NProp8);
    validate_nprop(RDI_NProp9);
    validate_nprop(RDI_NProp10);
    validate_nprop(RDI_NProp11);
    validate_nprop(RDI_NProp12);
    // above all use continue; r_qos[ix].name was not recognized
    error[ix_e].code = CosN::BAD_PROPERTY;
    error[ix_e].name = r_qos[ix].name;
    ix_e += 1; succ  = 0;
  }
  error.length(ix_e);   // Correct the length of this list
  // If no error occured, populate any additional properties that could
  // have been requested and supported
  ix_e = 0;
  if ( succ && (smap.num_set() != RDI_NotifQoS::NUM_PROPERTIES) ) {
    // add_range call for a property should not be performed if there is no
    // way to change its value 'in the current context'.  See examples in
    // section 2.5.6.8 of spec.
    // XXX marks cases where current impl only supports one value, but
    //     should be changed when support for other vals is added

    // add_range(RDI_NProp0); // XXX EventReliability must be BestEffort
    // add_range(RDI_NProp1); // XXX ConnectionReliability must be BestEffort
    add_range(RDI_NProp2); // Priority can always be changed
    // add_range(RDI_NProp3); // XXX StartTime not supported
    // add_range(RDI_NProp4); // XXX StopTime not supported
    add_range(RDI_NProp5); // Timeout can always be changed
    // add_range(RDI_NProp6); // XXX StartTimeSupported must be false
    // add_range(RDI_NProp7); // XXX StopTimeSupported must be false

    // all these props can always be changed, except at message level
    // -- this exclusion is handled by add_range 
    add_range(RDI_NProp8);  // MaxEventsPerConsumer
    add_range(RDI_NProp9);  // OrderPolicy
    add_range(RDI_NProp10); // DiscardPolicy
    add_range(RDI_NProp11); // MaximumBatchSize
    add_range(RDI_NProp12); // PacingInterval
  }
  range.length(ix_e); // Correct the length of this list
  return succ;
}

CORBA::Short RDI_NotifQoS::eventReliability() const
{
  if (_eventReliability_set) return _eventReliability;
  RDI_Assert(_parent, "Strange case, _parent should be set\n");
  return _parent->eventReliability();
}

void RDI_NotifQoS::eventReliability(CORBA::Short event_reliability)
{
  _eventReliability = event_reliability;
  _eventReliability_set = 1;
  _all_inherited = 0;
}

CORBA::Short RDI_NotifQoS::connectionReliability() const
{
  if (_connectionReliability_set) return _connectionReliability;
  RDI_Assert(_parent, "Strange case, _parent should be set\n");
  return _parent->connectionReliability();
}

void RDI_NotifQoS::connectionReliability(CORBA::Short connection_reliability)
{
  _connectionReliability = connection_reliability;
  _connectionReliability_set = 1;
  _all_inherited = 0;
}

CORBA::Short RDI_NotifQoS::priority() const
{
  if (_priority_set) return _priority;
  RDI_Assert(_parent, "Strange case, _parent should be set\n");
  return _parent->priority();
}

void RDI_NotifQoS::priority(CORBA::Short priority)
{
  _priority = priority;
  _priority_set = 1;
  _all_inherited = 0;
}

RDI_TimeT RDI_NotifQoS::timeout() const
{
  if (_timeout_set) return _timeout;
  RDI_Assert(_parent, "Strange case, _parent should be set\n");
  return _parent->timeout();
}

void RDI_NotifQoS::timeout_s_n(unsigned long& s, unsigned long& n) const
{
  RDI_TimeT v = timeout();
  v.get_rel_secs_nanosecs(s, n);
}

void RDI_NotifQoS::timeout_timebase(TimeBase::TimeT& to) const
{
  RDI_TimeT v = timeout();
  v.get_TimeBaseTimeT(to);
}

void RDI_NotifQoS::timeout(RDI_TimeT timeout)
{
  _timeout = timeout;
  _timeout_set = 1;
  _all_inherited = 0;
}

CORBA::Boolean RDI_NotifQoS::startTimeSupported() const
{
  if (_startTimeSupported_set) return _startTimeSupported;
  RDI_Assert(_parent, "Strange case, _parent should be set\n");
  return _parent->startTimeSupported();
}

void RDI_NotifQoS::startTimeSupported(CORBA::Boolean start_time_support)
{
  _startTimeSupported = start_time_support;
  _startTimeSupported_set = 1;
  _all_inherited = 0;
}

CORBA::Boolean RDI_NotifQoS::stopTimeSupported() const
{
  if (_stopTimeSupported_set) return _stopTimeSupported;
  RDI_Assert(_parent, "Strange case, _parent should be set\n");
  return _parent->stopTimeSupported();
}

void RDI_NotifQoS::stopTimeSupported(CORBA::Boolean stop_time_support)
{
  _stopTimeSupported = stop_time_support;
  _stopTimeSupported_set = 1;
  _all_inherited = 0;
}

CORBA::Short RDI_NotifQoS::orderPolicy() const
{
  if (_orderPolicy_set) return _orderPolicy;
  RDI_Assert(_parent, "Strange case, _parent should be set\n");
  return _parent->orderPolicy();
}

void RDI_NotifQoS::orderPolicy(CORBA::Short order_policy)
{
  _orderPolicy = order_policy;
  _orderPolicy_set = 1;
  _all_inherited = 0;
}

CORBA::Short RDI_NotifQoS::discardPolicy() const
{
  if (_discardPolicy_set) return _discardPolicy;
  RDI_Assert(_parent, "Strange case, _parent should be set\n");
  return _parent->discardPolicy();
}

void RDI_NotifQoS::discardPolicy(CORBA::Short discard_policy)
{
  _discardPolicy = discard_policy;
  _discardPolicy_set = 1;
  _all_inherited = 0;
}

RDI_TimeT RDI_NotifQoS::pacingInterval() const
{
  if (_pacingInterval_set) return _pacingInterval;
  RDI_Assert(_parent, "Strange case, _parent should be set\n");
  return _parent->pacingInterval();
}

void RDI_NotifQoS::pacingInterval_s_n(unsigned long& s, unsigned long& n) const
{
  RDI_TimeT v = pacingInterval();
  v.get_rel_secs_nanosecs(s, n);
}

void RDI_NotifQoS::pacingInterval_timebase(TimeBase::TimeT& to) const
{
  RDI_TimeT v = pacingInterval();
  v.get_TimeBaseTimeT(to);
}

void RDI_NotifQoS::pacingInterval(RDI_TimeT pacing_interval)
{
  _pacingInterval = pacing_interval;
  _pacingInterval_set = 1;
  _all_inherited = 0;
}

CORBA::Long RDI_NotifQoS::maxEventsPerConsumer() const
{
  if (_maxEventsPerConsumer_set) return _maxEventsPerConsumer;
  RDI_Assert(_parent, "Strange case, _parent should be set\n");
  return _parent->maxEventsPerConsumer();
}

void RDI_NotifQoS::maxEventsPerConsumer(CORBA::Long max_event_percunsumer)
{
  _maxEventsPerConsumer = max_event_percunsumer;
  _maxEventsPerConsumer_set = 1;
  _all_inherited = 0;
}

CORBA::Long RDI_NotifQoS::maximumBatchSize() const
{
  if (_maximumBatchSize_set) return _maximumBatchSize;
  RDI_Assert(_parent, "Strange case, _parent should be set\n");
  return _parent->maximumBatchSize();
}

void RDI_NotifQoS::maximumBatchSize(CORBA::Long max_batch_size)
{
  _maximumBatchSize = max_batch_size;
  _maximumBatchSize_set = 1;
  _all_inherited = 0;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//                          AdminQoS Properties                          //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

CORBA::Boolean
RDI_AdminQoS::is_admin_prop(const char* pname) {
  return ( RDI_STR_EQ(pname, RDI_AProp0_name)  ||  
	   RDI_STR_EQ(pname, RDI_AProp1_name)  ||  
	   RDI_STR_EQ(pname, RDI_AProp2_name)  ||  
	   RDI_STR_EQ(pname, RDI_AProp3_name) );
}

CosN::AdminProperties* RDI_AdminQoS::to_admin() const
{
  CORBA::Any::from_boolean f_bvl(rejectNewEvents);
  CosN::AdminProperties* aqos = new CosN::AdminProperties;
  RDI_Assert(aqos, "Memory allocation failure - AdminProperties\n");
  aqos->length(4);
  (*aqos)[0].name    = CORBA::string_dup("MaxQueueLength");
  (*aqos)[0].value <<= maxQueueLength;
  (*aqos)[1].name    = CORBA::string_dup("MaxConsumers");
  (*aqos)[1].value <<= maxConsumers;
  (*aqos)[2].name    = CORBA::string_dup("MaxSuppliers");
  (*aqos)[2].value <<= maxSuppliers;
  (*aqos)[3].name    = CORBA::string_dup("RejectNewEvents");
  (*aqos)[3].value <<= f_bvl;
  return aqos;
}

void RDI_AdminQoS::from_admin(const CosN::AdminProperties& r_qos)
{
  CORBA::Any::to_boolean vbool(rejectNewEvents);  
  for (CORBA::ULong ix=0; ix < r_qos.length(); ix++) {
    if ( RDI_STR_EQ(r_qos[ix].name, "MaxQueueLength") ) {
      r_qos[ix].value >>= maxQueueLength;
    } else if ( RDI_STR_EQ(r_qos[ix].name, "MaxConsumers") ) {
      r_qos[ix].value >>= maxConsumers;
    } else if ( RDI_STR_EQ(r_qos[ix].name, "MaxSuppliers") ) {
      r_qos[ix].value >>= maxSuppliers;
    } else if ( RDI_STR_EQ(r_qos[ix].name, "RejectNewEvents") ) {
      r_qos[ix].value >>= vbool;
    }
  }
}

CORBA::Boolean RDI_AdminQoS::validate(RDIstrstream&                 str,
				      const CosN::AdminProperties&  r_qos,
				      CosN::PropertyErrorSeq&       error)
{
  CORBA::Boolean succ=1;
  CORBA::ULong   ix_e=0;

  CORBA::Long    longTmp; 
  CORBA::Long    modMaxQueueLength = maxQueueLength;
  CORBA::Boolean modRejectNewEvents = rejectNewEvents;
  CORBA::Any::to_boolean vbool(modRejectNewEvents);

  CORBA::Boolean maxQLenErr = 0;
  CORBA::Boolean rejNwEvErr = 0;

  error.length(r_qos.length());
  for (CORBA::ULong ix=0; ix < r_qos.length(); ix++) {
    CORBA::TypeCode_var r_tc = r_qos[ix].value.type();
    if ( RDI_STR_EQ(r_qos[ix].name, "MaxQueueLength") ) {
      if ( ! CORBA::_tc_long->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = r_qos[ix].name;
	ix_e += 1; succ  = 0;
	maxQLenErr = 1; 
	continue;
      }
      r_qos[ix].value >>= modMaxQueueLength; // used in cross-value check
      if (modMaxQueueLength < 0) {
	error[ix_e].code = CosN::BAD_VALUE;
	error[ix_e].name = r_qos[ix].name;
	error[ix_e].available_range.low_val  <<= (CORBA::Long)0;
	error[ix_e].available_range.high_val <<= RDI_LONG_MAX;
	ix_e += 1; succ  = 0; 
	maxQLenErr = 1; 
	continue;
      }
      // We allow changes in the queue size to occur dynamically.
      // However,  depending on the provided value, these changes
      // may not be reflected immediately in the queue status.
      continue;
    }
    if ( RDI_STR_EQ(r_qos[ix].name, "MaxConsumers") ||
	 RDI_STR_EQ(r_qos[ix].name, "MaxSuppliers") ) {
      if ( ! CORBA::_tc_long->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = r_qos[ix].name;
	ix_e += 1; succ  = 0; 
	continue;
      }
      r_qos[ix].value >>= longTmp;
      if (longTmp < 0) {
	error[ix_e].code = CosN::BAD_VALUE;
	error[ix_e].name = r_qos[ix].name;
	error[ix_e].available_range.low_val  <<= (CORBA::Long)0;
	error[ix_e].available_range.high_val <<= RDI_LONG_MAX;
	ix_e += 1; succ  = 0; 
	continue;
      }
      // We allow changes to these values to be done dynamically.
      // However,  depending on the provided value, these changes
      // may not be reflected immediately in the channel status.
      continue;
    }
    if ( RDI_STR_EQ(r_qos[ix].name, "RejectNewEvents") ) {
      if ( ! CORBA::_tc_boolean->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = r_qos[ix].name;
	ix_e += 1; succ  = 0;
	rejNwEvErr = 0;
	continue;
      }
      r_qos[ix].value >>= vbool; // sets modRejectNewEvents; used in cross-value check
      // Here we assume that we can turn on and off this property
      // and, thus, we do not need to check the value requested.
      // Except for final check below!
      continue;
    }
    // r_qos[ix].name not a valid property
    error[ix_e].code = CosN::BAD_PROPERTY;
    error[ix_e].name = r_qos[ix].name;
    ix_e += 1; succ  = 0;
  }

  //
  // cross-value checks
  //

  if ((rejNwEvErr == 0) && (maxQLenErr == 0)) {
    if ((modRejectNewEvents == 0) && (modMaxQueueLength > 0)) {
      // choose which prop to complain about based on whether this is attempt to mod RejectNewEvents
      CORBA::Boolean use_reject = (modRejectNewEvents != rejectNewEvents);
      const char* nm = (use_reject) ? "RejectNewEvents" : "MaxQueueLength";
      str << "\n\tomniNotify does not currently support the following combination of admin properties:\n" <<
	"\t\tRejectNewEvents == 0   AND   MaxQueueLength > 0\n" <<
	"\tI.e., if you want to limit the queue length, you must (for now) specify\n" <<
	"\tthat incoming events should be discarded when the queue is full\n";
      error[ix_e].code = CosN::UNAVAILABLE_VALUE;
      error[ix_e].name = nm;
      if (use_reject) {
	CORBA::Any::from_boolean from_reject(rejectNewEvents);
	error[ix_e].available_range.low_val <<= from_reject;
	error[ix_e].available_range.high_val <<= from_reject;
      } else {
	error[ix_e].available_range.low_val <<= maxQueueLength;
	error[ix_e].available_range.high_val <<= maxQueueLength;
      }
      ix_e += 1; succ  = 0;
    }
  }
  // Done
  error.length(ix_e);   // Correct the length of this list
  return succ;
}

RDI_AdminQoS& RDI_AdminQoS::operator= (const RDI_AdminQoS& qos)
{
  maxQueueLength     = qos.maxQueueLength;
  maxConsumers       = qos.maxConsumers;
  maxSuppliers       = qos.maxSuppliers;
  rejectNewEvents    = qos.rejectNewEvents;
  return *this;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//                          ServerQoS Properties                         //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

CORBA::Boolean
RDI_ServerQoS::is_server_prop(const char* pname) {
  return ( RDI_STR_EQ(pname, RDI_SProp0_name)
	   || RDI_STR_EQ(pname, RDI_SProp1_name)
	   || RDI_STR_EQ(pname, RDI_SProp2_name)
	   || RDI_STR_EQ(pname, RDI_SProp3_name)
	   || RDI_STR_EQ(pname, RDI_SProp4_name)
	   || RDI_STR_EQ(pname, RDI_SProp5_name)
	   || RDI_STR_EQ(pname, RDI_SProp6_name)
	   || RDI_STR_EQ(pname, RDI_SProp7_name)
	   || RDI_STR_EQ(pname, RDI_SProp8_name)
	   || RDI_STR_EQ(pname, RDI_SProp9_name)
	   || RDI_STR_EQ(pname, RDI_SProp10_name)
	   || RDI_STR_EQ(pname, RDI_SProp11_name)
	   || RDI_STR_EQ(pname, RDI_SProp12_name)
	   || RDI_STR_EQ(pname, RDI_SProp13_name)
	   || RDI_STR_EQ(pname, RDI_SProp14_name)
#ifndef NO_OBJ_GC
	   || RDI_STR_EQ(pname, RDI_SProp15_name)
	   || RDI_STR_EQ(pname, RDI_SProp16_name)
	   || RDI_STR_EQ(pname, RDI_SProp17_name)
	   || RDI_STR_EQ(pname, RDI_SProp18_name)
	   || RDI_STR_EQ(pname, RDI_SProp19_name)
	   || RDI_STR_EQ(pname, RDI_SProp20_name)
#endif
	 );
}

AttN::ServerProperties* RDI_ServerQoS::to_server_props() const
{
  AttN::ServerProperties* sqos = new AttN::ServerProperties;
  RDI_Assert(sqos, "Memory allocation failure - ServerProperties\n");
  sqos->length(RDI_NUM_SPROPS);
  (*sqos)[0].name    = CORBA::string_dup("NumAdminGroups");
  (*sqos)[0].value <<= numAdminGroups;
  (*sqos)[1].name    = CORBA::string_dup("NumAdminThreads");
  (*sqos)[1].value <<= numAdminThreads;
  (*sqos)[2].name    = CORBA::string_dup("NumProxyThreads");
  (*sqos)[2].value <<= numProxyThreads;
  (*sqos)[3].name    = CORBA::string_dup("NumPushThreads");
  (*sqos)[3].value <<= numPushThreads;
  (*sqos)[4].name    = CORBA::string_dup("NumPullThreads");
  (*sqos)[4].value <<= numPullThreads;
  (*sqos)[5].name    = CORBA::string_dup("PullEventPeriod");
  (*sqos)[5].value <<= pullEventPeriod;
  (*sqos)[6].name    = CORBA::string_dup("QueueGCPeriod");
  (*sqos)[6].value <<= queueGCPeriod;
  (*sqos)[7].name    = CORBA::string_dup("ReportingInterval");
  (*sqos)[7].value <<= reportingInterval;
  (*sqos)[8].name    = CORBA::string_dup("NumOChangeThreads");
  (*sqos)[8].value <<= numOChangeThreads;
  (*sqos)[9].name    = CORBA::string_dup("NumSChangeThreads");
  (*sqos)[9].value <<= numSChangeThreads;
  (*sqos)[10].name   = CORBA::string_dup("LocalClockInaccLo");
  (*sqos)[10].value <<= localClockInaccLo;
  (*sqos)[11].name   = CORBA::string_dup("LocalClockInaccHi");
  (*sqos)[11].value <<= localClockInaccHi;
  (*sqos)[12].name   = CORBA::string_dup("LocalClockTDF");
  (*sqos)[12].value <<= localClockTDF;
  (*sqos)[13].name   = CORBA::string_dup("OutgoingTimeout");
  (*sqos)[13].value <<= outgoingTimeout;
  (*sqos)[14].name   = CORBA::string_dup("IncomingTimeout");
  (*sqos)[14].value <<= incomingTimeout;
#ifndef NO_OBJ_GC
  (*sqos)[15].name   = CORBA::string_dup("ObjectGCPeriod");
  (*sqos)[15].value <<= objectGCPeriod;
  (*sqos)[16].name   = CORBA::string_dup("DeadChanInterval");
  (*sqos)[16].value <<= deadChanInterval;
  (*sqos)[17].name   = CORBA::string_dup("DeadAdminInterval");
  (*sqos)[17].value <<= deadAdminInterval;
  (*sqos)[18].name   = CORBA::string_dup("DeadConProxyInterval");
  (*sqos)[18].value <<= deadConProxyInterval;
  (*sqos)[19].name   = CORBA::string_dup("DeadOtherProxyInterval");
  (*sqos)[19].value <<= deadOtherProxyInterval;
  (*sqos)[20].name   = CORBA::string_dup("DeadFilterInterval");
  (*sqos)[20].value <<= deadFilterInterval;
#endif
  return sqos;
}

void RDI_ServerQoS::from_server_props(const AttN::ServerProperties& s_qos)
{
  for (CORBA::ULong ix=0; ix < s_qos.length(); ix++) {
    if ( RDI_STR_EQ(s_qos[ix].name, "NumAdminGroups") ) {
      s_qos[ix].value >>= numAdminGroups;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "NumAdminThreads") ) {
      s_qos[ix].value >>= numAdminThreads;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "NumProxyThreads") ) {
      s_qos[ix].value >>= numProxyThreads;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "NumPushThreads") ) {
      s_qos[ix].value >>= numPushThreads;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "NumPullThreads") ) {
      s_qos[ix].value >>= numPullThreads;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "PullEventPeriod") ) {
      s_qos[ix].value >>= pullEventPeriod;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "QueueGCPeriod") ) {
      s_qos[ix].value >>= queueGCPeriod;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "ReportingInterval") ) {
      s_qos[ix].value >>= reportingInterval;
      if ((reportingInterval > 0) && (reportingInterval < 10)) {
	RDIRptServerQoSLog("Setting ReportingInterval to 10 secs (internal constraint) rather than " << reportingInterval);
	reportingInterval = 10;
      }
    } else if ( RDI_STR_EQ(s_qos[ix].name, "NumOChangeThreads") ) {
      s_qos[ix].value >>= numOChangeThreads;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "NumSChangeThreads") ) {
      s_qos[ix].value >>= numSChangeThreads;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "LocalClockInaccLo") ) {
      s_qos[ix].value >>= localClockInaccLo;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "LocalClockInaccHi") ) {
      s_qos[ix].value >>= localClockInaccHi;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "LocalClockTDF") ) {
      s_qos[ix].value >>= localClockTDF;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "OutgoingTimeout") ) {
      s_qos[ix].value >>= outgoingTimeout;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "IncomingTimeout") ) {
      s_qos[ix].value >>= incomingTimeout;
#ifndef NO_OBJ_GC
    } else if ( RDI_STR_EQ(s_qos[ix].name, "ObjectGCPeriod") ) {
      s_qos[ix].value >>= objectGCPeriod;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "DeadChanInterval") ) {
      s_qos[ix].value >>= deadChanInterval;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "DeadAdminInterval") ) {
      s_qos[ix].value >>= deadAdminInterval;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "DeadConProxyInterval") ) {
      s_qos[ix].value >>= deadConProxyInterval;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "DeadOtherProxyInterval") ) {
      s_qos[ix].value >>= deadOtherProxyInterval;
    } else if ( RDI_STR_EQ(s_qos[ix].name, "DeadFilterInterval") ) {
      s_qos[ix].value >>= deadFilterInterval;
#endif
    }
  }
}

CORBA::Boolean
RDI_ServerQoS::validate(RDIstrstream&                  str,
			const AttN::ServerProperties&  s_qos,
			CosN::PropertyErrorSeq&        error,
			CORBA::Boolean                 isfac)
{
  CORBA::Boolean succ=1;
  CORBA::UShort  sval=0;
  CORBA::ULong   ix_e=0;

  CORBA::ULong   ulongTmp; 

  error.length(s_qos.length());
  for (CORBA::ULong ix=0; ix < s_qos.length(); ix++) {
    CORBA::TypeCode_var r_tc = s_qos[ix].value.type();
    if ( RDI_STR_EQ(s_qos[ix].name, "NumAdminGroups") ) {
      if ( ! CORBA::_tc_ushort->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      s_qos[ix].value >>= sval;
      if ( (isfac && sval == 0) || (!isfac && sval != numAdminGroups) ) {
	error[ix_e].code = CosN::BAD_VALUE;
	error[ix_e].name = s_qos[ix].name;
	error[ix_e].available_range.low_val  <<= 
	  (isfac ? (CORBA::UShort) 1    : numAdminGroups);
	error[ix_e].available_range.high_val <<= 
	  (isfac ? (CORBA::UShort) 1000 : numAdminGroups);
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "NumAdminThreads") ) {
      if ( ! CORBA::_tc_ushort->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      s_qos[ix].value >>= sval;
      if ( (isfac && sval == 0) || (!isfac && sval != numAdminThreads) ) {
	error[ix_e].code = CosN::BAD_VALUE;
	error[ix_e].name = s_qos[ix].name;
	error[ix_e].available_range.low_val  <<= 
	  (isfac ? (CORBA::UShort) 1  : numAdminThreads);
	error[ix_e].available_range.high_val <<= 
	  (isfac ? (CORBA::UShort) 50 : numAdminThreads);
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "NumProxyThreads") ) {
      if ( ! CORBA::_tc_ushort->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      s_qos[ix].value >>= sval;
      if ( !isfac && sval != numProxyThreads ) {
	error[ix_e].code = CosN::BAD_VALUE;
	error[ix_e].name = s_qos[ix].name;
	error[ix_e].available_range.low_val  <<= numProxyThreads;
	error[ix_e].available_range.high_val <<= numProxyThreads;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "NumPushThreads") ) {
      if ( ! CORBA::_tc_ushort->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      s_qos[ix].value >>= sval;
      if ( !isfac && sval != numPushThreads ) {
	error[ix_e].code = CosN::BAD_VALUE;
	error[ix_e].name = s_qos[ix].name;
	error[ix_e].available_range.low_val  <<= numPushThreads;
	error[ix_e].available_range.high_val <<= numPushThreads;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "NumPullThreads") ) {
      if ( ! CORBA::_tc_ushort->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      s_qos[ix].value >>= sval;
      if ( !isfac && sval != numPullThreads ) {
	error[ix_e].code = CosN::BAD_VALUE;
	error[ix_e].name = s_qos[ix].name;
	error[ix_e].available_range.low_val  <<= numPullThreads;
	error[ix_e].available_range.high_val <<= numPullThreads;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "NumOChangeThreads") ) {
      if ( ! CORBA::_tc_ushort->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      s_qos[ix].value >>= sval;
      if ( !isfac && sval != numOChangeThreads ) {
	error[ix_e].code = CosN::BAD_VALUE;
	error[ix_e].name = s_qos[ix].name;
	error[ix_e].available_range.low_val  <<= numOChangeThreads;
	error[ix_e].available_range.high_val <<= numOChangeThreads;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "NumSChangeThreads") ) {
      if ( ! CORBA::_tc_ushort->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      s_qos[ix].value >>= sval;
      if ( !isfac && sval != numSChangeThreads ) {
	error[ix_e].code = CosN::BAD_VALUE;
	error[ix_e].name = s_qos[ix].name;
	error[ix_e].available_range.low_val  <<= numSChangeThreads;
	error[ix_e].available_range.high_val <<= numSChangeThreads;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "PullEventPeriod") ) {
      if ( ! CORBA::_tc_ulong->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "QueueGCPeriod") ) {
      if ( ! CORBA::_tc_ulong->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      s_qos[ix].value >>= ulongTmp;
      if (ulongTmp < 1) {
	error[ix_e].code = CosN::BAD_VALUE;
	error[ix_e].name = s_qos[ix].name;
	error[ix_e].available_range.low_val  <<= (CORBA::ULong)1;
	error[ix_e].available_range.high_val <<= RDI_ULONG_MAX;
	ix_e += 1; succ  = 0; 
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "ReportingInterval") ) {
      if ( ! CORBA::_tc_ulong->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "LocalClockInaccLo") ) {
      if ( ! CORBA::_tc_ulong->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "LocalClockInaccHi") ) {
      if ( ! CORBA::_tc_ushort->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "LocalClockTDF") ) {
      if ( ! CORBA::_tc_short->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "OutgoingTimeout") ) {
      if ( ! CORBA::_tc_ulong->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "IncomingTimeout") ) {
      if ( ! CORBA::_tc_ulong->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
#ifndef NO_OBJ_GC
    if ( RDI_STR_EQ(s_qos[ix].name, "ObjectGCPeriod") ) {
      if ( ! CORBA::_tc_ulong->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "DeadChanInterval") ) {
      if ( ! CORBA::_tc_ulong->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "DeadAdminInterval") ) {
      if ( ! CORBA::_tc_ulong->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "DeadConProxyInterval") ) {
      if ( ! CORBA::_tc_ulong->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "DeadOtherProxyInterval") ) {
      if ( ! CORBA::_tc_ulong->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
    if ( RDI_STR_EQ(s_qos[ix].name, "DeadFilterInterval") ) {
      if ( ! CORBA::_tc_ulong->equivalent(r_tc) ) {
	error[ix_e].code = CosN::BAD_TYPE;
	error[ix_e].name = s_qos[ix].name;
	ix_e += 1; succ  = 0;
	continue;
      }
      continue;
    }
#endif
    // s_qos[ix].name not a valid property
    error[ix_e].code = CosN::BAD_PROPERTY;
    error[ix_e].name = s_qos[ix].name;
    ix_e += 1; succ  = 0;
  }
  // Done
  error.length(ix_e);   // Correct the length of this list
  return succ;
}

RDI_ServerQoS&
RDI_ServerQoS::operator= (const RDI_ServerQoS& qos)
{
  numAdminGroups          = qos.numAdminGroups;
  numAdminThreads         = qos.numAdminThreads;
  numProxyThreads         = qos.numProxyThreads;
  numPushThreads          = qos.numPushThreads;
  numPullThreads          = qos.numPullThreads;
  pullEventPeriod         = qos.pullEventPeriod;
  queueGCPeriod           = qos.queueGCPeriod;
  reportingInterval       = qos.reportingInterval;
  numOChangeThreads       = qos.numOChangeThreads;
  numSChangeThreads       = qos.numSChangeThreads;
  localClockInaccLo       = qos.localClockInaccLo;
  localClockInaccHi       = qos.localClockInaccHi;
  localClockTDF           = qos.localClockTDF;
  outgoingTimeout         = qos.outgoingTimeout;
  incomingTimeout         = qos.incomingTimeout;
#ifndef NO_OBJ_GC
  objectGCPeriod          = qos.objectGCPeriod;
  deadChanInterval        = qos.deadChanInterval;
  deadAdminInterval       = qos.deadAdminInterval;
  deadConProxyInterval    = qos.deadConProxyInterval;
  deadOtherProxyInterval  = qos.deadOtherProxyInterval;
  deadFilterInterval      = qos.deadFilterInterval;
#endif
  return *this;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//                              RDI_AllQoS
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

// We need to install defaults because, when we do config->update_env()
// to get environment variable overrides, we need all of the
// following parameter names to be present in config.

void RDI_AllQoS::install_all_defaults(RDI_Config& config) {
  config.set_value("ChannelFactoryName",	"ChannelFactory");
  config.set_value("DefaultChannelName",	"EventChannel");
  config.set_value("FactoryIORFileName",	"/tmp/rdifact.ior");
  config.set_value("ChannelIORFileName",	"/tmp/rdichan.ior");
  config.set_value("EventReliability",		"0");
  config.set_value("ConnectionReliability",	"0");
  config.set_value("Priority",			"0");
  config.set_value("Timeout",			"0");
  config.set_value("StartTimeSupported",	"0");
  config.set_value("StopTimeSupported",		"0");
  config.set_value("OrderPolicy",		"0");
  config.set_value("DiscardPolicy",		"0");
  config.set_value("PacingInterval",		"1000");
  config.set_value("MaxEventsPerConsumer",	"0");
  config.set_value("MaximumBatchSize",		"8");
  config.set_value("MaxQueueLength",		"0");
  config.set_value("MaxConsumers",		"0");
  config.set_value("MaxSuppliers",		"0");
  config.set_value("RejectNewEvents",		"1");
  config.set_value("NumAdminGroups",		"2");
  config.set_value("NumAdminThreads",		"2");
  config.set_value("NumProxyThreads",		"0");
  config.set_value("NumPushThreads",		"4");
  config.set_value("NumPullThreads",		"2");
  config.set_value("NumSChangeThreads",		"1");
  config.set_value("NumOChangeThreads",		"1");
  config.set_value("PullEventPeriod",		"100");
  config.set_value("QueueGCPeriod",		"30");
  config.set_value("ReportingInterval",		"300");
  config.set_value("DebugLogFile",		"stderr");
  config.set_value("ReportLogFile",		"stdout");
  config.set_value("LocalClockInaccLo",         "0");
  config.set_value("LocalClockInaccHi",         "0");
  config.set_value("LocalClockTDF",             "300");
  config.set_value("OutgoingTimeout",           "10000");
  config.set_value("IncomingTimeout",           "10000");
#ifndef NO_OBJ_GC
  config.set_value("ObjectGCPeriod",		"600");
  config.set_value("DeadChanInterval",		"0");
  config.set_value("DeadAdminInterval",		"0");
  config.set_value("DeadConProxyInterval",	"0");
  config.set_value("DeadOtherProxyInterval",	"600");
  config.set_value("DeadFilterInterval",	"0");
#endif

  // debug flags default to 0
  config.set_value(RDIDbgDaemon_nm        , "0");
  config.set_value(RDIDbgFact_nm          , "0");
  config.set_value(RDIDbgFilt_nm          , "0");
  config.set_value(RDIDbgChan_nm          , "0");
  config.set_value(RDIDbgCAdm_nm          , "0");
  config.set_value(RDIDbgSAdm_nm          , "0");
  config.set_value(RDIDbgCPxy_nm          , "0");
  config.set_value(RDIDbgSPxy_nm          , "0");
  config.set_value(RDIDbgEvQ_nm           , "0");
  config.set_value(RDIDbgRDIEvent_nm      , "0");
  config.set_value(RDIDbgFAdmin_nm        , "0");
  config.set_value(RDIDbgEval_nm          , "0");
  config.set_value(RDIDbgCosCPxy_nm       , "0");
  config.set_value(RDIDbgCosSPxy_nm       , "0");
  config.set_value(RDIDbgNotifQoS_nm      , "0");
  config.set_value(RDIDbgAdminQoS_nm      , "0");
  config.set_value(RDIDbgNotifQueue_nm    , "0");
                        
  // report flags default to 1
  config.set_value(RDIRptChanStats_nm     , "1");
  config.set_value(RDIRptQSizeStats_nm    , "1");
  config.set_value(RDIRptCnctdCons_nm     , "1");
  config.set_value(RDIRptCnctdSups_nm     , "1");
  config.set_value(RDIRptCnctdFilts_nm    , "1");
  config.set_value(RDIRptUnCnctdFilts_nm  , "1");
  config.set_value(RDIRptRejects_nm       , "1");
  config.set_value(RDIRptDrops_nm         , "1");
  config.set_value(RDIRptNotifQoS_nm      , "1");
  config.set_value(RDIRptAdminQoS_nm      , "1");
  config.set_value(RDIRptServerQoS_nm     , "1");
  config.set_value(RDIRptInteractive_nm   , "1");
}

CORBA::Boolean RDI_AllQoS::validate_initial_config(RDIstrstream&  str, 
						   RDI_Config&    config,
						   RDI_NotifQoS&  n_qos,
						   RDI_AdminQoS&  a_qos, 
						   RDI_ServerQoS& s_qos)
{
  CORBA::Boolean success = 1;
  CORBA::Long    lval    = 0;
  CORBA::ULong   ulval   = 0;
  CORBA::Short   sval    = 0;
  CORBA::UShort  usval   = 0;
  CORBA::Boolean bval    = 0;
  RDI_ObjectKind otype   = RDI_ECHANNEL;

  //
  // ** NotifQoS Properties **
  //

  RDI_NProp0_check (RDI_NProp0, sval, sval=0); // EventReliability -- never changes
  RDI_NProp1_check (RDI_NProp1, sval, sval=0); // ConnectionReliability -- never changes
  RDI_NProp2_check (RDI_NProp2, sval, n_qos.priority(sval)); // Priority
  //  RDI_NProp3_check (RDI_NProp3, n_qos.startTime); // StartTime (only valid at msg level)
  //  RDI_NProp4_check (RDI_NProp4, n_qos.stopTime); // StopTime (only valid at msg level)
  RDI_NProp5_check (RDI_NProp5, n_qos.timeout); // Timeout
  RDI_NProp6_check (RDI_NProp6, bval, bval=0); // StartTimeSupported -- never changes
  RDI_NProp7_check (RDI_NProp7, bval, bval=0); // StopTimeSupported -- never changes
  RDI_NProp8_check (RDI_NProp8, lval, n_qos.maxEventsPerConsumer(lval)); // MaxEventsPerConsumer
  RDI_NProp9_check (RDI_NProp9, sval, n_qos.orderPolicy(sval)); // OrderPolicy
  RDI_NProp10_check(RDI_NProp10, sval, n_qos.discardPolicy(sval)); // DiscardPolicy
  RDI_NProp11_check(RDI_NProp11, lval, n_qos.maximumBatchSize(lval)); // MaximumBatchSize
  RDI_NProp12_check(RDI_NProp12, n_qos.pacingInterval); // PacingInterval
  
  //
  // ** AdminQoS Properties **
  //

  RDI_AProp0_check(RDI_AProp0,  lval,  a_qos.maxQueueLength    = lval);  // MaxQueueLength
  RDI_AProp1_check(RDI_AProp1,  lval,  a_qos.maxConsumers      = lval);  // MaxConsumers
  RDI_AProp2_check(RDI_AProp2,  lval,  a_qos.maxSuppliers      = lval);  // MaxSuppliers
  RDI_AProp3_check(RDI_AProp3,  bval,  a_qos.rejectNewEvents   = bval);  // RejectNewEvents
  
  //
  // ** ServerQoS Properties **
  //

  RDI_SProp0_check (RDI_SProp0,  usval, s_qos.numAdminGroups          = usval); // NumAdminGroups
  RDI_SProp1_check (RDI_SProp1,  usval, s_qos.numAdminThreads         = usval); // NumAdminThreads
  RDI_SProp2_check (RDI_SProp2,  usval, s_qos.numProxyThreads         = usval); // NumProxyThreads
  RDI_SProp3_check (RDI_SProp3,  usval, s_qos.numPushThreads          = usval); // NumPushThreads
  RDI_SProp4_check (RDI_SProp4,  usval, s_qos.numPullThreads          = usval); // NumPullThreads
  RDI_SProp5_check (RDI_SProp5,  usval, s_qos.numSChangeThreads       = usval); // NumSChangeThreads
  RDI_SProp6_check (RDI_SProp6,  usval, s_qos.numOChangeThreads       = usval); // NumOChangeThreads
  RDI_SProp7_check (RDI_SProp7,  ulval, s_qos.pullEventPeriod         = ulval); // PullEventPeriod
  RDI_SProp8_check (RDI_SProp8,  ulval, s_qos.queueGCPeriod           = ulval); // QueueGCPeriod
  RDI_SProp9_check (RDI_SProp9,  ulval, s_qos.reportingInterval       = ulval); // ReportingInterval
  RDI_SProp10_check(RDI_SProp10, ulval, s_qos.localClockInaccLo       = ulval); // LocalClockInaccLo
  RDI_SProp11_check(RDI_SProp11, usval, s_qos.localClockInaccHi       = usval); // LocalClockInaccHi
  RDI_SProp12_check(RDI_SProp12, sval,  s_qos.localClockTDF           = sval ); // LocalClockTDF
  RDI_SProp13_check(RDI_SProp13, ulval, s_qos.outgoingTimeout         = ulval); // OutgoingTimeout
  RDI_SProp14_check(RDI_SProp14, ulval, s_qos.incomingTimeout         = ulval); // IncomingTimeout
#ifndef NO_OBJ_GC
  RDI_SProp15_check(RDI_SProp15, ulval, s_qos.objectGCPeriod          = ulval); // ObjectGCPeriod
  RDI_SProp16_check(RDI_SProp16, ulval, s_qos.deadChanInterval        = ulval); // DeadChanInterval
  RDI_SProp17_check(RDI_SProp17, ulval, s_qos.deadAdminInterval       = ulval); // DeadAdminInterval
  RDI_SProp18_check(RDI_SProp18, ulval, s_qos.deadConProxyInterval    = ulval); // DeadConProxyInterval
  RDI_SProp19_check(RDI_SProp19, ulval, s_qos.deadOtherProxyInterval  = ulval); // DeadOtherProxyInterval
  RDI_SProp20_check(RDI_SProp20, ulval, s_qos.deadFilterInterval      = ulval); // DeadFilternterval
#endif

  if (success == 1) { // do some additional stuff 
    // additional single-value constraints
    if ((s_qos.reportingInterval > 0) && (s_qos.reportingInterval < 200)) {
      str << "Setting ReportingInterval to 10 secs (internal constraint) rather than " << s_qos.reportingInterval << '\n';
      s_qos.reportingInterval = 10;
    }
    // cross-value checks
    if ( s_qos.numAdminThreads > s_qos.numAdminGroups ) {
      str << "WARNING: NumAdminThreads [" << s_qos.numAdminThreads  <<
	"] greater than NumAdminGroups [" << s_qos.numAdminGroups << 
	"]\n\tSetting NumAdminThreads = NumAdminGroups\n";
      s_qos.numAdminThreads = s_qos.numAdminGroups;
    }
    if ((a_qos.rejectNewEvents == 0) && (a_qos.maxQueueLength > 0)) {
      str << "\n\tomniNotify does not currently support this combination of properties:\n" <<
	"\t\tRejectNewEvents == 0   AND   MaxQueueLength > 0\n" <<
	"\tI.e., if you want to limit the queue length, you must (for now) specify\n" <<
	"\tthat incoming events should be discarded when the queue is full\n";
      success = 0;
    }
  }
  return success;
}

CORBA::Boolean RDI_AllQoS::parse_set_command(RDIstrstream&           str, 
					     RDIParseCmd&            p,
					     RDI_ObjectKind          otype,
					     CosN::QoSProperties&    n_qos,
					     CosN::AdminProperties&  a_qos,
					     AttN::ServerProperties& s_qos)
{
  CORBA::Boolean  success = 1;
  CORBA::Long     lval    = 0;
  CORBA::ULong    ulval   = 0;
  CORBA::Short    sval    = 0;
  CORBA::UShort   usval   = 0;
  CORBA::Boolean  bval    = 0;
  CORBA::ULong    n_idx   = 0, a_idx = 0, s_idx = 0;

  if ((p.argc < 3) || ((p.argc % 2) == 0)) {
    str << "Set command must have one or more name-value pairs\n  (2, 4, 6, ... arguments)\n";
    return 0;
  }
  RDI_Config* config = 0;
  if ( ! (config = new RDI_Config()) ) {
    str << "**Memory allocation error**\n";
    return 0;
  }
  for (unsigned int i = 1; (int)i < p.argc; i += 2) {
    if (RDI_ServerQoS::is_server_prop(p.argv[i])) {
      if (otype != RDI_NFSERVER) {
	str << "ServerQoS Property " << p.argv[i] << " can only be set at server level\n";
	success = 0;
      } else {
	config->set_value(p.argv[i], p.argv[i+1]);
      }
    } else if (RDI_AdminQoS::is_admin_prop(p.argv[i])) {
      if (otype != RDI_ECHANNEL) {
	str << "AdminQoS Property " << p.argv[i] << " can only be set at channel / chanfact level\n";
	success = 0;
      } else {
	config->set_value(p.argv[i], p.argv[i+1]);
      }
    } else if (RDI_NotifQoS::is_qos_prop(p.argv[i])) {
      config->set_value(p.argv[i], p.argv[i+1]); // defer otype check 
    } else {
      if (otype == RDI_NFSERVER) {
	str << "Property name \"" << p.argv[i] << "\" is not a valid ServerQoS Property name\n";
      } else if (otype == RDI_ECHANNEL) {
	str << "Property name \"" << p.argv[i] << "\" is not a valid AdminQoS or NotifQoS Property name\n";
      } else {
	str << "Property name \"" << p.argv[i] << "\" is not a valid NotifQoS Property name\n";
      }
      success = 0;
    }
  }

  //
  // ** NotifQoS Properties **
  //

  RDI_NProp0_checkset (RDI_NProp0,  sval); // EventReliability
  RDI_NProp1_checkset (RDI_NProp1,  sval); // ConnectionReliability
  RDI_NProp2_checkset (RDI_NProp2,  sval); // Priority
  RDI_NProp3_checkset (RDI_NProp3);        // StartTime
  RDI_NProp4_checkset (RDI_NProp4);        // StopTime
  RDI_NProp5_checkset (RDI_NProp5);        // Timeout
  RDI_NProp6_checkset (RDI_NProp6, bval);  // StartTimeSupported
  RDI_NProp7_checkset (RDI_NProp7, bval);  // StopTimeSupported
  RDI_NProp8_checkset (RDI_NProp8,  lval); // MaxEventsPerConsumer
  RDI_NProp9_checkset (RDI_NProp9,  sval); // OrderPolicy
  RDI_NProp10_checkset(RDI_NProp10, sval); // DiscardPolicy
  RDI_NProp11_checkset(RDI_NProp11, lval); // MaximumBatchSize
  RDI_NProp12_checkset(RDI_NProp12);       // PacingInterval

  //
  // ** AdminQoS Properties **
  //

  RDI_AProp0_checkset(RDI_AProp0, lval); // MaxQueueLength
  RDI_AProp1_checkset(RDI_AProp1, lval); // MaxConsumers
  RDI_AProp2_checkset(RDI_AProp2, lval); // MaxSuppliers
  RDI_AProp3_checkset(RDI_AProp3, bval); // RejectNewEvents

  //
  // ** ServerQoS Properties **
  //

  RDI_SProp0_checkset (RDI_SProp0,  usval);    // NumAdminGroups
  RDI_SProp1_checkset (RDI_SProp1,  usval);    // NumAdminThreads
  RDI_SProp2_checkset (RDI_SProp2,  usval);    // NumProxyThreads
  RDI_SProp3_checkset (RDI_SProp3,  usval);    // NumPushThreads
  RDI_SProp4_checkset (RDI_SProp4,  usval);    // NumPullThreads
  RDI_SProp5_checkset (RDI_SProp5,  usval);    // NumSChangeThreads
  RDI_SProp6_checkset (RDI_SProp6,  usval);    // NumOChangeThreads
  RDI_SProp7_checkset (RDI_SProp7,  ulval);    // PullEventPeriod
  RDI_SProp8_checkset (RDI_SProp8,  ulval);    // QueueGCPeriod
  RDI_SProp9_checkset (RDI_SProp9,  ulval);    // ReportingInterval
  RDI_SProp10_checkset(RDI_SProp10, ulval);    // LocalClockInaccLo
  RDI_SProp11_checkset(RDI_SProp11, usval);    // LocalClockInaccHi
  RDI_SProp12_checkset(RDI_SProp12, sval);     // LocalClockTDF
  RDI_SProp13_checkset(RDI_SProp13, ulval);    // OutgoingTimeout
  RDI_SProp14_checkset(RDI_SProp14, ulval);    // IncomingTimeout
#ifndef NO_OBJ_GC
  RDI_SProp15_checkset(RDI_SProp15, ulval);    // ObjectGCPeriod
  RDI_SProp16_checkset(RDI_SProp16, ulval);    // DeadChanInterval
  RDI_SProp17_checkset(RDI_SProp17, ulval);    // DeadAdminInterval
  RDI_SProp18_checkset(RDI_SProp18, ulval);    // DeadConProxyInterval
  RDI_SProp19_checkset(RDI_SProp19, ulval);    // DeadOtherProxyInterval
  RDI_SProp20_checkset(RDI_SProp20, ulval);    // DeadFilterInterval
#endif

  delete config;
  return success;
}

////////////////////////////////////////
// Logging

RDIstrstream& RDI_NotifQoS::log_output(RDIstrstream& str) const
{
  unsigned long timeout_s, timeout_n;
  unsigned long pacing_interval_s, pacing_interval_n;
  timeout_s_n(timeout_s, timeout_n);
  pacingInterval_s_n(pacing_interval_s, pacing_interval_n);

  if (_parent && _eventReliability_set) {
    str << "  *EventReliability      "       ; str.setw(6);
  } else {
    str << "   EventReliability      "       ; str.setw(6);
  }
  str << eventReliability();
  if (_parent && _connectionReliability_set) { 
    str << " |*ConnectionReliability "       ; str.setw(6);
  } else {
    str << " | ConnectionReliability "       ; str.setw(6);
  }
  str << connectionReliability();
  if (_parent && _priority_set) {
    str << " |*Priority              "       ; str.setw(6);
  } else {
    str << " | Priority              "       ; str.setw(6);
  }
  str << priority() << '\n';
  if (_parent && _startTimeSupported_set) {
    str << "  *StartTimeSupported    "       ; str.setw(6);
  } else {
    str << "   StartTimeSupported    "       ; str.setw(6);
  }
  str << (startTimeSupported()?1:0);
  if (_parent && _stopTimeSupported_set) {
    str << " |*StopTimeSupported     "       ; str.setw(6);
  } else {
    str << " | StopTimeSupported     "       ; str.setw(6);
  }
  str << (stopTimeSupported()?1:0);
  if (_parent && _orderPolicy_set) {
    str << " |*OrderPolicy           "       ; str.setw(6);
  } else {
    str << " | OrderPolicy           "       ; str.setw(6);
  }
  str << orderPolicy() << '\n';
  if (_parent && _discardPolicy_set) {
    str << "  *DiscardPolicy         "       ; str.setw(6);
  } else {
    str << "   DiscardPolicy         "       ; str.setw(6);
  }
  str << discardPolicy();
  if (_parent && _maxEventsPerConsumer_set) {
    str << " |*MaxEventsPerConsumer  "       ; str.setw(6);
  } else {
    str << " | MaxEventsPerConsumer  "       ; str.setw(6);
  }
  str << maxEventsPerConsumer();
  if (_parent && _maximumBatchSize_set) {
    str << " |*MaximumBatchSize      "       ; str.setw(6);
  } else {
    str << " | MaximumBatchSize      "       ; str.setw(6);
  }
  str << maximumBatchSize() << '\n';
  if (_parent && _timeout_set) {
    str << "  *Timeout(s,n)          (" << timeout_s << "," << timeout_n << ")\n";
  } else {
    str << "   Timeout(s,n)          (" << timeout_s << "," << timeout_n << ")\n";
  }
  if (_parent && _pacingInterval_set) {
    str << "  *PacingInterval(s,n)   (" << pacing_interval_s << "," << pacing_interval_n << ")\n";
  } else {
    str << "   PacingInterval(s,n)   (" << pacing_interval_s << "," << pacing_interval_n << ")\n";
  }
  if (_parent) {
    if (_all_inherited) {
      str << "\nAll values are inherited from parent.\n";
    } else {
      str << "\nAll values marked with '*' are explicity set.\nOther values are inherited from parent.\n";
    }
  }
  return str;
}

RDIstrstream& RDI_AdminQoS::log_output(RDIstrstream& str) const
{
  str << "   MaxQueueLength        "       ; str.setw(6);
  str << maxQueueLength;
  str << " | MaxConsumers          "       ; str.setw(6);
  str << maxConsumers;
  str << " | MaxSuppliers          "       ; str.setw(6);
  str << maxSuppliers << '\n';
  str << "   RejectNewEvents       "       ; str.setw(6);
  str << (rejectNewEvents ? "YES" : "NO");
  str << '\n';
  return str;
}

RDIstrstream& RDI_ServerQoS::log_output(RDIstrstream& str) const
{
  str << "   NumAdminGroups        "       ; str.setw(6);
  str << numAdminGroups;
  str << " | NumAdminThreads       "       ; str.setw(6);
  str << numAdminThreads;
  str << " | NumProxyThreads       "       ; str.setw(6);
  str << numProxyThreads << '\n';
  str << "   NumPushThreads        "       ; str.setw(6);
  str << numPushThreads;
  str << " | NumPullThreads        "       ; str.setw(6);
  str << numPullThreads << '\n';
  str << "   NumOChangeThreads     "       ; str.setw(6);
  str << numOChangeThreads;
  str << " | NumSChangeThreads     "       ; str.setw(6);
  str << numSChangeThreads  << "\n\n";
  str << "  *PullEventPeriod       "       ; str.setw(6);
  str << pullEventPeriod;
  str << " |*ReportingInterval     "       ; str.setw(6);
  str << reportingInterval;
  str << " |*QueueGCPeriod         "       ; str.setw(6);
  str << queueGCPeriod  << '\n';
#ifndef NO_OBJ_GC
  str << "  *ObjectGCPeriod        "       ; str.setw(6);
  str << objectGCPeriod;
  str << " |*DeadChanInterval      "       ; str.setw(6);
  str << deadChanInterval;
  str << " |*DeadAdminInterval     "       ; str.setw(6);
  str << deadAdminInterval  << '\n';
  str << "  *DeadConProxyInterval  "       ; str.setw(6);
  str << deadConProxyInterval;
  str << " |*DeadOtherProxyInterval "      ; str.setw(5);
  str << deadOtherProxyInterval;
  str << " |*DeadFilterInterval    "       ; str.setw(6);
  str << deadFilterInterval  << '\n';
#endif
  str << "  *LocalClockInaccLo     "       ; str.setw(6);
  str << localClockInaccLo;
  str << " |*LocalClockInaccHi     "       ; str.setw(6);
  str << localClockInaccHi;
  str << " |*LocalClockTDF         "       ; str.setw(6);
  str << localClockTDF << '\n';
  str << "  *OutgoingTimeout       "       ; str.setw(6);
  str << outgoingTimeout;
  str << " |*IncomingTimeout       "       ; str.setw(6);
  str << incomingTimeout;
  str << "\n\nParams with an asterisk can be modified.\n";
  str << "The other params are fixed at server startup time.\n";
  return str;
}
