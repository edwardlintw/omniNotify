// -*- Mode: C++; -*-
//                              File      : RDIRVM.cc
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
//    Implementation of RDI_RVM [Virtual Machine used for constraint parsing] 
//
 
// TODO
// [ ] assoc operator currently accepts anything that has a string as first
//     component and something else as second component (must have >= 2 
//     components).  Actual requirement is that it be an nvpair list of some
//     kind -- could add more checking
// [ ] lhs enum_val for in operator not handled?  call CORBA_STRING_FREE on 
//     get_as_string?
// [ ] dot_num:  for union case, I do a name-based comparison by extracting 
//     current member name and comparing to the name recording in the typecode
//     at offset num.  What is the name at the default offset?
// [ ] what are the type checking rules for the in operator (am I being too 
//     lenient by convering all numerics to double? perhaps only lhs constants
//     that are bool or char can be used as numbers -- a sequence of bool 
//     cannot be compared a LHS that evals to, say, double val 1.0
// [ ] same question for tag values -- true or 'c' can be used if the tag kind
//     is double, but can 1.0 be used if the tag kind is bool, or 65.0 if char?
// [ ] what about for arith?
// => it seems to me that bool and char LITERALS can be converted to numbers,
//     but not bool and char VALUES (via component expr, arith expr, etc)

#include "thread_wrappers.h"
#include "RDI.h"
#include "RDIStringDefs.h"
#include "RDITimeWrappers.h"
#include "RDIDynamicEvalDefs.h"
#include "RDIEvent.h"
#include "CosNotification_i.h"

TW_Mutex      RDI_RVM::_cls_lock;
unsigned long RDI_RVM::_rvmIDctr=0;

// ------------------------------------------------------------------------
// debugging macros

// force assertion checking even if NDEBUG is set
// write assertion to both std and fatal logs

#define RDI_RVM_Assert(x, str) RDI_Assert(x, "[rvmID = " << rvmID << "] " << str << '\n')

#ifndef NDEBUG

#define RDI_RVM_DbgLog(streamstuff)\
RDIDbgEvalLog("[rvmID = " << rvmID << "] " << streamstuff << '\n')

#define RDI_RVM_DbgLog_Op(pc, op, streamstuff)\
  RDI_RVM_DbgLog("PC = " << pc << " op = (" << op << ")" << " : " << streamstuff)

#define RDI_RVM_DbgLog_Top1(rtval)\
  RDI_RVM_DbgLog("<Top>     = " << rtval)

#define RDI_RVM_DbgLog_Top2(rtvaltopminus1, rtvaltop)\
  RDI_RVM_DbgLog("<Top-1>   = " << rtvaltopminus1);\
  RDI_RVM_DbgLog("<Top>     = " << rtvaltop)

#define RDI_RVM_DbgLog_Op_DA_Mismatch(pc, op, dynany_ptr, expecting) \
  do { CORBA::TypeCode_var tmp_tcp = dynany_ptr->type(); \
       RDI_RVM_DbgLog_Op(pc, op, "Type Mismatch, expecting " << expecting << ", got " << (*tmp_tcp));  \
     } while (0)

#define RDI_RVM_DbgLog_Op_TC_Mismatch(pc, op, rditckind, expecting) \
RDI_RVM_DbgLog_Op(pc, op, "Type Mismatch, expecting " << expecting << ", got " << RDI_RTValKind2string[rditckind])

#define RDI_RVM_DbgLog_Op_CMP_Mismatch(pc, op, lhskind, rhskind) \
  RDI_RVM_DbgLog_Op(pc, op, "Type Mismatch, cannot compare " << RDI_RTValKind2string[lhskind] << " and " << RDI_RTValKind2string[rhskind])

#define RDI_RVM_DbgLog_Op_SEQ_Mismatch(pc, op, tc, expecting) \
  RDI_RVM_DbgLog_Op(pc, op, "Type Mismatch, expecting sequence or array of " << expecting << ", got sequence or array of " << tc)

# else

#define RDI_RVM_DbgLog(streamstuff)                                  __RDI_NULL_STMT
#define RDI_RVM_DbgLog_Op(pc, op, streamstuff)                       __RDI_NULL_STMT
#define RDI_RVM_DbgLog_Top1(rtval)                                   __RDI_NULL_STMT
#define RDI_RVM_DbgLog_Top2(rtvaltopminus1, rtvaltop)                __RDI_NULL_STMT 
#define RDI_RVM_DbgLog_Op_DA_Mismatch(pc, op, dynany_ptr, expecting) __RDI_NULL_STMT 
#define RDI_RVM_DbgLog_Op_TC_Mismatch(pc, op, rditckind, expecting)  __RDI_NULL_STMT
#define RDI_RVM_DbgLog_Op_CMP_Mismatch(pc, op, lhskind, rhskind)     __RDI_NULL_STMT
#define RDI_RVM_DbgLog_Op_SEQ_Mismatch(pc, op, tc, expecting)        __RDI_NULL_STMT

#endif


// ------------------------------------------------------------------------
// In future CLEANUP_RETURN and CLEANUP_SIGNAL might be slightly different
// The current macro assumes r_code has been set

#define RDI_CLEANUP_RETURN { \
  RDI_RVM_DbgLog("eval RETURNING " << (r_bool ? "TRUE" : "FALSE")); \
  _clear_stack(); \
  return 1; \
}

#define RDI_CLEANUP_SIGNAL { \
  RDI_RVM_DbgLog("eval SIGNALING " << RDI_RTRetCode2string[r_code]); \
  _clear_stack(); \
  return 1; \
}

// default behavior after most ops is to advance _PC and continue
#define RDI_NEXT_OP { \
  _PC++; if (_PC > _ops->_opstail) { RDI_Fatal("ran off end of opseq"); } \
  return 0; \
}

// after a jump, do not modify _PC
#define RDI_NEXT_AFTER_JUMP { \
  return 0; \
}

// ------------------------------------------------------------------------
// Tests over CORBA::TypeCode_ptr

// a string?
#define RDI_CORBATC_STRING(tcp) (CORBA::_tc_string->equivalent(tcp))

// a number?
#ifdef TIMEBASE_NOLONGLONG
#  define RDI_CORBATC_NUMBER(tcp) \
     ((CORBA::_tc_ushort->equivalent(tcp)) || \
      (CORBA::_tc_short->equivalent(tcp)) || \
      (CORBA::_tc_ulong->equivalent(tcp)) || \
      (CORBA::_tc_long->equivalent(tcp)) || \
      (CORBA::_tc_float->equivalent(tcp)) || \
      (CORBA::_tc_double->equivalent(tcp)))
#else
#  define RDI_CORBATC_NUMBER(tcp) \
     ((CORBA::_tc_ushort->equivalent(tcp)) || \
      (CORBA::_tc_short->equivalent(tcp)) || \
      (CORBA::_tc_ulong->equivalent(tcp)) || \
      (CORBA::_tc_long->equivalent(tcp)) || \
      (CORBA::_tc_ulonglong->equivalent(tcp)) || \
      (CORBA::_tc_longlong->equivalent(tcp)) || \
      (CORBA::_tc_float->equivalent(tcp)) || \
      (CORBA::_tc_double->equivalent(tcp)))
#endif

// convertable to a number?
#define RDI_CORBATC_CANBE_NUMBER(tcp) \
     ((CORBA::_tc_boolean->equivalent(tcp)) || \
      (CORBA::_tc_char->equivalent(tcp)) || \
      (CORBA::_tc_octet->equivalent(tcp)))

// a number or convertable to a number?
#define RDI_CORBATC_NUMERIC(tcp) \
	(RDI_CORBATC_NUMBER(tcp)||RDI_CORBATC_CANBE_NUMBER(tcp))

// ------------------------------------------------------------------------
// Tests over RDI_RTValKind

// a string?
#define RDI_TCKIND_STRING(tckind) \
     ((tckind == RDI_rtk_string) || \
      (tckind == RDI_rtk_char_or_string))

// an enum val or enum ident?
#define RDI_TCKIND_ENUM(tckind) \
	((tckind == RDI_rtk_enum_val)||(tckind == RDI_rtk_enum_ident))

// a number?
#define RDI_TCKIND_NUMBER(tckind) \
     ((tckind == RDI_rtk_ushort) || \
      (tckind == RDI_rtk_short) || \
      (tckind == RDI_rtk_ulong) || \
      (tckind == RDI_rtk_long) || \
      (tckind == RDI_rtk_ulonglong) || \
      (tckind == RDI_rtk_longlong) || \
      (tckind == RDI_rtk_float) || \
      (tckind == RDI_rtk_double))

// convertable to a number?
#define RDI_TCKIND_CANBE_NUMBER(tckind) \
     ((tckind == RDI_rtk_boolean) || \
      (tckind == RDI_rtk_char) || \
      (tckind == RDI_rtk_octet) || \
      (tckind == RDI_rtk_char_or_string))

// a number or convertable to a number?
#define RDI_TCKIND_NUMERIC(tckind) \
	(RDI_TCKIND_NUMBER(tckind)||RDI_TCKIND_CANBE_NUMBER(tckind))

#define RDI_TCKIND_IS_SIGNED(k) \
   ((k != RDI_rtk_ulonglong) && (k != RDI_rtk_ulong) && (k != RDI_rtk_ushort))

// ------------------------------------------------------------------------ //

#undef WHATFN
#define WHATFN "RDI_RVM::RDI_RVM"
RDI_RVM::RDI_RVM() : r_code(RDI_RTRet_UNDEFINED),  r_bool(0), cexprs(0),
		     rvmID(0), _PC(-1), _top(-1), _free_ops(0), _ops(0), _op(0)
{
  TW_SCOPE_LOCK(class_lock, _cls_lock, "rvm_class", WHATFN);
  rvmID = _rvmIDctr++;
}

// ------------------------------------------------------------------------ //

/** Force a 'canbe' case to a number
  * Using ushort for all these cases
  *
  * REQUIRES:  RDI_TCKIND_CANBE_NUMBER(v._tckind) should be true
  * EFFECT:    modifies v
  */

void RDI_RVM::_force2num(RDI_RTVal& v) 
{
  switch (v._tckind) {
  case RDI_rtk_char:
	v.set_ushort((CORBA::UShort)(v._v_char));
	break;
  case RDI_rtk_boolean:
	v.set_ushort((CORBA::UShort)(v._v_boolean ? 1 : 0));
	break;
  case RDI_rtk_octet:
	v.set_ushort((CORBA::UShort)(v._v_octet));
	break;
  case RDI_rtk_char_or_string: 
	v.set_ushort((CORBA::UShort)(v._v_string_ptr[0]));
	break;
  default: 
	RDI_Fatal("Should be called with char, bool, octet, or char_or_string");
  }
}

/** REQUIRES: lhskind and rhskind must be numeric kinds, which,
  *           in order of precedence, are:
  *               RDI_rtk_ushort
  *               RDI_rtk_short
  *               RDI_rtk_ulong
  *		  RDI_rtk_long
  *               RDI_rtk_ulonglong
  *		  RDI_rtk_longlong
  *               RDI_rtk_float
  *               RDI_rtk_double
  *               RDI_rtk_ldouble      // OMNIORB DOES NOT SUPPORT LONG DOUBLE YET
  * EFFECT:   applies arith conversion rules defined in section
  *           2.4.3 of the Notification Service specification.
  */

void RDI_RVM::_widenAndResultKinds(RDI_RTValKind lhskind, 
				   RDI_RTValKind rhskind, 
				   RDI_RTValKind& widenkind, 
				   RDI_RTValKind& resultkind) 
{
  RDI_RVM_Assert(RDI_TCKIND_NUMBER(lhskind), "if n_required was used; lhs should be numeric");
  RDI_RVM_Assert(RDI_TCKIND_NUMBER(rhskind), "if n_required was used; rhs should be numeric");
  resultkind = (lhskind < rhskind) ? rhskind : lhskind;
  // resultkind cannot be unsigned unless both sides are unsigned
  if ( RDI_TCKIND_IS_SIGNED(lhskind) || RDI_TCKIND_IS_SIGNED(rhskind) ) {
    // resultkind must be signed
    switch (resultkind) {
    case RDI_rtk_ushort:
      resultkind = RDI_rtk_short;
      break;
    case RDI_rtk_ulong:
      resultkind = RDI_rtk_long;
      break;
    case RDI_rtk_ulonglong:
      resultkind = RDI_rtk_longlong;
      break;
    default:
      break;
    }
  }
  // there are fewer widen kinds than result kinds
  switch (resultkind) {
  case RDI_rtk_ushort:
    widenkind = RDI_rtk_ulong;
    break;
  case RDI_rtk_short:
    widenkind = RDI_rtk_long;
    break;
  case RDI_rtk_ulong:
    widenkind = RDI_rtk_ulong;
    break;
  case RDI_rtk_long:
    widenkind = RDI_rtk_long;
    break;
  case RDI_rtk_ulonglong:
    widenkind = RDI_rtk_ulonglong;
    break;
  case RDI_rtk_longlong:
    widenkind = RDI_rtk_longlong;
    break;
  case RDI_rtk_float:
    widenkind = RDI_rtk_double;
    break;
  case RDI_rtk_double:
    widenkind = RDI_rtk_double;
    break;
#if 0
  case RDI_rtk_ldouble:
    widenkind = RDI_rtk_ldouble;
    break;
#endif
  default:
    break;
  }
}

/** Returns TRUE if a signal should occur and sets r_code appropriately,
  * otherwise returns FALSE
  */

CORBA::Boolean RDI_RVM::_eval_arithop(RDI_ArithOp arithop) 
{
  RDI_RVM_DbgLog("_eval_arithop called");
  RDI_RTValKind lhs_kind   = _stack[_top-1]._tckind;
  RDI_RTValKind rhs_kind   = _stack[_top]._tckind;
  RDI_RTValKind widenkind;
  RDI_RTValKind resultkind;
  _widenAndResultKinds(lhs_kind, rhs_kind, widenkind, resultkind);

  switch (widenkind) {
  case RDI_rtk_ulong: {
    CORBA::ULong ulong_lhs = 0;
    CORBA::ULong ulong_rhs = 0;
    switch (rhs_kind) {
    case RDI_rtk_ushort:    
	ulong_rhs = ((CORBA::ULong)(_stack[_top]._v_ushort)); break;
    case RDI_rtk_ulong:     
	ulong_rhs = ((CORBA::ULong)(_stack[_top]._v_ulong));  break;
    default: 
	RDI_Fatal("should not get here: type never widened to a ulong");
    }
    _stack[_top--].clear(); // pop top (we will overwrite lhs [new top])
    if ( (ulong_rhs == 0) &&
         ((arithop == RDI_ArithOp_div) || (arithop == RDI_ArithOp_rem)) ) {
      r_code = RDI_RTRet_DIVIDE_BY_ZERO;
      RDI_RVM_DbgLog("_eval_arithop returning RDI_RTRet_DIVIDE_BY_ZERO r_code");
      return 1; // ERROR
    }
    switch (lhs_kind) {
    case RDI_rtk_ushort:    
	ulong_lhs = ((CORBA::ULong)(_stack[_top]._v_ushort)); break;
    case RDI_rtk_ulong:     
	ulong_lhs = ((CORBA::ULong)(_stack[_top]._v_ulong));  break;
    default: 
	RDI_Fatal("should not get here: type never widened to a ulong");
    }
    switch (arithop) {
    case RDI_ArithOp_cmp:
      // we ignore the resultkind and use one of 3 short vals   (-1, 0, 1)
      if      (ulong_lhs <  ulong_rhs)    _stack[_top].set_short(-1);
      else if (ulong_lhs == ulong_rhs)    _stack[_top].set_short( 0);
      else  /* ulong_lhs >  ulong_rhs */  _stack[_top].set_short( 1);
      RDI_RVM_DbgLog("_eval_arithop OK: pop'd RHS, overwrote LHS with:");
      RDI_RVM_DbgLog_Top1(_stack[_top]);
      return 0; // OK result

    // for + - * / rem put result in ulong_rhs
    case RDI_ArithOp_add: ulong_rhs = ulong_lhs + ulong_rhs;  break;
    case RDI_ArithOp_sub: ulong_rhs = ulong_lhs - ulong_rhs;  break;
    case RDI_ArithOp_mul: ulong_rhs = ulong_lhs * ulong_rhs;  break;
    case RDI_ArithOp_div: ulong_rhs = ulong_lhs / ulong_rhs;  break;
    case RDI_ArithOp_rem: ulong_rhs = ulong_lhs % ulong_rhs;  break;
    }
    // for widenkind ulong, result kind is either ushort or ulong
    switch (resultkind) {
    case RDI_rtk_ushort:     
	_stack[_top].set_ushort((CORBA::UShort)ulong_rhs);     break;
    case RDI_rtk_ulong:      
	_stack[_top].set_ulong ((CORBA::ULong) ulong_rhs);      break;
    default: 
	RDI_Fatal("should not get here: unexpected result kind " << resultkind);
    }
    RDI_RVM_DbgLog("_eval_arithop OK: pop'd RHS, overwrote LHS with:");
    RDI_RVM_DbgLog_Top1(_stack[_top]);
    return 0; // OK result
    // END ulong arm
  }
  case RDI_rtk_long: {
    CORBA::Long long_lhs = 0;
    CORBA::Long long_rhs = 0;
    switch (rhs_kind) {
    case RDI_rtk_ushort:    
	long_rhs = ((CORBA::Long)(_stack[_top]._v_ushort));  break;
    case RDI_rtk_short:     
	long_rhs = ((CORBA::Long)(_stack[_top]._v_short));   break;
    case RDI_rtk_ulong:
        long_rhs = ((CORBA::Long)(_stack[_top]._v_ulong));   break; 
    case RDI_rtk_long:
        long_rhs = ((CORBA::Long)(_stack[_top]._v_long));    break; 
    default: 
	RDI_Fatal("should not get here: type never widened to a long");
    }
    _stack[_top--].clear(); // pop top (we will overwrite lhs [new top])
    if ( (long_rhs == 0) && 
         ((arithop == RDI_ArithOp_div) || (arithop == RDI_ArithOp_rem)) ) {
      r_code = RDI_RTRet_DIVIDE_BY_ZERO;
      RDI_RVM_DbgLog("_eval_arithop returning RDI_RTRet_DIVIDE_BY_ZERO r_code");
      return 1; // ERROR
    }
    switch (lhs_kind) {
    case RDI_rtk_ushort:    
	long_lhs = ((CORBA::Long)(_stack[_top]._v_ushort));  break;
    case RDI_rtk_short:     
	long_lhs = ((CORBA::Long)(_stack[_top]._v_short));   break;
    case RDI_rtk_ulong:      
	long_lhs = ((CORBA::Long)(_stack[_top]._v_ulong));   break; 
    case RDI_rtk_long:      
	long_lhs = ((CORBA::Long)(_stack[_top]._v_long));    break; 
    default: 
	RDI_Fatal("should not get here: type never widened to a long");	
    }
    if (arithop == RDI_ArithOp_cmp) {
      // we ignore the resultkind and use one of 3 short vals   (-1, 0, 1)
      if      (long_lhs <  long_rhs)    _stack[_top].set_short(-1);
      else if (long_lhs == long_rhs)    _stack[_top].set_short( 0);
      else  /* long_lhs >  long_rhs */  _stack[_top].set_short( 1);
      RDI_RVM_DbgLog("_eval_arithop OK: pop'd RHS, overwrote LHS with:");
      RDI_RVM_DbgLog_Top1(_stack[_top]);
      return 0; // OK result
    }
    // for + - * / rem put result in long_rhs
    switch (arithop) {
    case RDI_ArithOp_add: long_rhs = long_lhs + long_rhs;  break;
    case RDI_ArithOp_sub: long_rhs = long_lhs - long_rhs;  break;
    case RDI_ArithOp_mul: long_rhs = long_lhs * long_rhs;  break;
    case RDI_ArithOp_div: long_rhs = long_lhs / long_rhs;  break;
    case RDI_ArithOp_rem: long_rhs = long_lhs % long_rhs;  break;
    default: RDI_Fatal("cannot get here");
    }
    // for widenkind long, result kind is either short or long
    switch (resultkind) {
    case RDI_rtk_short:      
	_stack[_top].set_short ((CORBA::Short)long_rhs);   break;
    case RDI_rtk_long:       
	_stack[_top].set_long  ((CORBA::Long) long_rhs);    break;
    default: 
	RDI_Fatal("should not get here : unexpected result kind " << resultkind);
    }
    RDI_RVM_DbgLog("_eval_arithop OK: pop'd RHS, overwrote LHS with:");
    RDI_RVM_DbgLog_Top1(_stack[_top]);
    return 0; // OK result
    // END long arm
  }
  case RDI_rtk_ulonglong: {
    CORBA::ULongLong ulonglong_lhs = 0;
    CORBA::ULongLong ulonglong_rhs = 0;
    switch (rhs_kind) {
    case RDI_rtk_ushort:    
	ulonglong_rhs = ((CORBA::ULongLong)(_stack[_top]._v_ushort));    break;
    case RDI_rtk_ulong:     
	ulonglong_rhs = ((CORBA::ULongLong)(_stack[_top]._v_ulong));     break;
    case RDI_rtk_ulonglong:      
	ulonglong_rhs = ((CORBA::ULongLong)(_stack[_top]._v_ulonglong)); break; 
    default: 
	RDI_Fatal("should not get here: type never widened to a ulonglong");
    }
    _stack[_top--].clear(); // pop top (we will overwrite lhs [new top])
    if ( (ulonglong_rhs == 0) &&
         ((arithop == RDI_ArithOp_div) || (arithop == RDI_ArithOp_rem)) ) {
      r_code = RDI_RTRet_DIVIDE_BY_ZERO;
      RDI_RVM_DbgLog("_eval_arithop returning RDI_RTRet_DIVIDE_BY_ZERO r_code");
      return 1; // ERROR
    }
    switch (lhs_kind) {
    case RDI_rtk_ushort:    
	ulonglong_lhs = ((CORBA::ULongLong)(_stack[_top]._v_ushort)); break;
    case RDI_rtk_ulong:     
	ulonglong_lhs = ((CORBA::ULongLong)(_stack[_top]._v_ulong));  break;
    case RDI_rtk_ulonglong:     
	ulonglong_lhs = ((CORBA::ULongLong)(_stack[_top]._v_ulonglong));  break;
    default: 
	RDI_Fatal("should not get here: type never widened to a ulonglong");
    }
    switch (arithop) {
    case RDI_ArithOp_cmp:
      // we ignore the resultkind and use one of 3 short vals   (-1, 0, 1)
      if      (ulonglong_lhs <  ulonglong_rhs)    _stack[_top].set_short(-1);
      else if (ulonglong_lhs == ulonglong_rhs)    _stack[_top].set_short( 0);
      else  /* ulonglong_lhs >  ulonglong_rhs */  _stack[_top].set_short( 1);
      RDI_RVM_DbgLog("_eval_arithop OK: pop'd RHS, overwrote LHS with:");
      RDI_RVM_DbgLog_Top1(_stack[_top]);
      return 0; // OK result

    // for + - * / rem put result in ulonglong_rhs
    case RDI_ArithOp_add: ulonglong_rhs = ulonglong_lhs + ulonglong_rhs;  break;
    case RDI_ArithOp_sub: ulonglong_rhs = ulonglong_lhs - ulonglong_rhs;  break;
    case RDI_ArithOp_mul: ulonglong_rhs = ulonglong_lhs * ulonglong_rhs;  break;
    case RDI_ArithOp_div: ulonglong_rhs = ulonglong_lhs / ulonglong_rhs;  break;
    case RDI_ArithOp_rem: ulonglong_rhs = ulonglong_lhs % ulonglong_rhs;  break;
    }
    // for widenkind ulonglong, result kind is always ulonglong
    switch (resultkind) {
    case RDI_rtk_ulonglong:      
	_stack[_top].set_ulonglong ((CORBA::ULongLong)ulonglong_rhs);     break;
    default: 
	RDI_Fatal("should not get here: uanexpected result kind " << resultkind);
    }
    RDI_RVM_DbgLog("_eval_arithop OK: pop'd RHS, overwrote LHS with:");
    RDI_RVM_DbgLog_Top1(_stack[_top]);
    return 0; // OK result
    // END ulonglong arm
  }
  case RDI_rtk_longlong: {
    CORBA::LongLong longlong_lhs = 0;
    CORBA::LongLong longlong_rhs = 0;
    switch (rhs_kind) {
    case RDI_rtk_ushort:    
	longlong_rhs = ((CORBA::LongLong)(_stack[_top]._v_ushort));    break;
    case RDI_rtk_short:     
	longlong_rhs = ((CORBA::LongLong)(_stack[_top]._v_short));     break;
    case RDI_rtk_ulong:
        longlong_rhs = ((CORBA::LongLong)(_stack[_top]._v_ulong));     break; 
    case RDI_rtk_long:
        longlong_rhs = ((CORBA::LongLong)(_stack[_top]._v_long));      break; 
    case RDI_rtk_ulonglong:
        longlong_rhs = ((CORBA::LongLong)(_stack[_top]._v_ulonglong)); break; 
    case RDI_rtk_longlong:
        longlong_rhs = ((CORBA::LongLong)(_stack[_top]._v_longlong));  break; 
    default: 
	RDI_Fatal("should not get here: type never widened to a longlong");
    }
    _stack[_top--].clear(); // pop top (we will overwrite lhs [new top])
    if ( (longlong_rhs == 0) && 
         ((arithop == RDI_ArithOp_div) || (arithop == RDI_ArithOp_rem)) ) {
      r_code = RDI_RTRet_DIVIDE_BY_ZERO;
      RDI_RVM_DbgLog("_eval_arithop returning RDI_RTRet_DIVIDE_BY_ZERO r_code");
      return 1; // ERROR
    }
    switch (lhs_kind) {
    case RDI_rtk_ushort:    
	longlong_lhs = ((CORBA::LongLong)(_stack[_top]._v_ushort));  break;
    case RDI_rtk_short:     
	longlong_lhs = ((CORBA::LongLong)(_stack[_top]._v_short));   break;
    case RDI_rtk_ulong:      
	longlong_lhs = ((CORBA::LongLong)(_stack[_top]._v_ulong));    break; 
    case RDI_rtk_long:      
	longlong_lhs = ((CORBA::LongLong)(_stack[_top]._v_long));    break; 
    case RDI_rtk_ulonglong:      
	longlong_lhs = ((CORBA::LongLong)(_stack[_top]._v_ulonglong));    break; 
    case RDI_rtk_longlong:      
	longlong_lhs = ((CORBA::LongLong)(_stack[_top]._v_longlong));    break; 
    default: 
	RDI_Fatal("should not get here: type never widened to a longlong");	
    }
    if (arithop == RDI_ArithOp_cmp) {
      // we ignore the resultkind and use one of 3 short vals   (-1, 0, 1)
      if      (longlong_lhs <  longlong_rhs)    _stack[_top].set_short(-1);
      else if (longlong_lhs == longlong_rhs)    _stack[_top].set_short( 0);
      else  /* longlong_lhs >  longlong_rhs */  _stack[_top].set_short( 1);
      RDI_RVM_DbgLog("_eval_arithop OK: pop'd RHS, overwrote LHS with:");
      RDI_RVM_DbgLog_Top1(_stack[_top]);
      return 0; // OK result
    }
    // for + - * / rem put result in longlong_rhs
    switch (arithop) {
    case RDI_ArithOp_add: longlong_rhs = longlong_lhs + longlong_rhs;  break;
    case RDI_ArithOp_sub: longlong_rhs = longlong_lhs - longlong_rhs;  break;
    case RDI_ArithOp_mul: longlong_rhs = longlong_lhs * longlong_rhs;  break;
    case RDI_ArithOp_div: longlong_rhs = longlong_lhs / longlong_rhs;  break;
    case RDI_ArithOp_rem: longlong_rhs = longlong_lhs % longlong_rhs;  break;
    default: RDI_Fatal("cannot get here");
    }
    // for widenkind longlong, result kind is always longlong
    switch (resultkind) {
    case RDI_rtk_longlong:      
	_stack[_top].set_longlong ((CORBA::LongLong)longlong_rhs);     break;
    default: 
	RDI_Fatal("should not get here : unexpected result kind " << resultkind);
    }
    RDI_RVM_DbgLog("_eval_arithop OK: pop'd RHS, overwrote LHS with:");
    RDI_RVM_DbgLog_Top1(_stack[_top]);
    return 0; // OK result
    // END longlong arm
  }
  case RDI_rtk_double: {
    if (arithop == RDI_ArithOp_rem) { // cannot apply rem to floats/doubles
      r_code = RDI_RTRet_TYPE_MISMATCH;
      RDI_RVM_DbgLog("_eval_arithop returning RDI_RTRet_TYPE_MISMATCH r_code");
      return 1; // ERROR
    }
    CORBA::Double dbl_lhs = 0;
    CORBA::Double dbl_rhs = 0;
    switch (rhs_kind) {
    case RDI_rtk_ushort:    
	dbl_rhs = ((CORBA::Double)(_stack[_top]._v_ushort));    break;
    case RDI_rtk_short:     
	dbl_rhs = ((CORBA::Double)(_stack[_top]._v_short));     break;
    case RDI_rtk_ulong:     
	dbl_rhs = ((CORBA::Double)(_stack[_top]._v_ulong));     break;
    case RDI_rtk_long:      
	dbl_rhs = ((CORBA::Double)(_stack[_top]._v_long));      break; 
#ifdef _MSC_VER
    case RDI_rtk_ulonglong:
        dbl_rhs = ((CORBA::Double)(CORBA::LongLong)(_stack[_top]._v_ulonglong)); break; 
#else
    case RDI_rtk_ulonglong:
        dbl_rhs = ((CORBA::Double)(_stack[_top]._v_ulonglong)); break; 
#endif
    case RDI_rtk_longlong:
        dbl_rhs = ((CORBA::Double)(_stack[_top]._v_longlong));  break; 
    case RDI_rtk_float:     
	dbl_rhs = ((CORBA::Double)(_stack[_top]._v_float));     break;
    case RDI_rtk_double:    
	dbl_rhs = ((CORBA::Double)(_stack[_top]._v_double));    break;
    default: 
	RDI_Fatal("should not get here: type never widened to a double");
    }
    _stack[_top--].clear(); // pop top (we will overwrite lhs [new top])
    if ((arithop == RDI_ArithOp_div) && (dbl_rhs == 0)) {
      r_code = RDI_RTRet_DIVIDE_BY_ZERO;
      RDI_RVM_DbgLog("_eval_arithop returning RDI_RTRet_DIVIDE_BY_ZERO r_code");
      return 1; // ERROR
    }
    switch (lhs_kind) {
    case RDI_rtk_ushort:    
	dbl_lhs = ((CORBA::Double)(_stack[_top]._v_ushort));    break;
    case RDI_rtk_short:     
	dbl_lhs = ((CORBA::Double)(_stack[_top]._v_short));     break;
    case RDI_rtk_ulong:     
	dbl_lhs = ((CORBA::Double)(_stack[_top]._v_ulong));     break;
    case RDI_rtk_long:      
	dbl_lhs = ((CORBA::Double)(_stack[_top]._v_long));      break; 
#ifdef _MSC_VER
    case RDI_rtk_ulonglong:
        dbl_lhs = ((CORBA::Double)(CORBA::LongLong)(_stack[_top]._v_ulonglong)); break; 
#else
    case RDI_rtk_ulonglong:
        dbl_lhs = ((CORBA::Double)(_stack[_top]._v_ulonglong)); break; 
#endif
    case RDI_rtk_longlong:
        dbl_lhs = ((CORBA::Double)(_stack[_top]._v_longlong));  break; 
    case RDI_rtk_float:     
	dbl_lhs = ((CORBA::Double)(_stack[_top]._v_float));     break;
    case RDI_rtk_double:    
	dbl_lhs = ((CORBA::Double)(_stack[_top]._v_double));    break;
    default: 
	RDI_Fatal("should not get here: type never widened to a double");
    }
    switch (arithop) {
    case RDI_ArithOp_cmp:
      if      (dbl_lhs <  dbl_rhs)    _stack[_top].set_short(-1);
      else if (dbl_lhs == dbl_rhs)    _stack[_top].set_short( 0);
      else  /* dbl_lhs >  dbl_rhs */  _stack[_top].set_short( 1);
      RDI_RVM_DbgLog("_eval_arithop OK: pop'd RHS, overwrote LHS with:");
      RDI_RVM_DbgLog_Top1(_stack[_top]);
      return 0; // OK result

    // for + - * / put result in dbl_rhs

    case RDI_ArithOp_add: dbl_rhs = dbl_lhs + dbl_rhs;  break;
    case RDI_ArithOp_sub: dbl_rhs = dbl_lhs - dbl_rhs;  break;
    case RDI_ArithOp_mul: dbl_rhs = dbl_lhs * dbl_rhs;  break;
    case RDI_ArithOp_div: dbl_rhs = dbl_lhs / dbl_rhs;  break;
    case RDI_ArithOp_rem: /* cannot get here */         break;
    }
    // for widenkind double, result kind is either float or double
    switch (resultkind) {
    case RDI_rtk_float:
	_stack[_top].set_float ((CORBA::Float) dbl_rhs); break;
    case RDI_rtk_double:
	_stack[_top].set_double((CORBA::Double)dbl_rhs); break;
    default: 
	RDI_Fatal("should not get here: unexpected result kind " << resultkind);
    }
    RDI_RVM_DbgLog("_eval_arithop OK: pop'd RHS, overwrote LHS with:");
    RDI_RVM_DbgLog_Top1(_stack[_top]);
    return 0; // OK result
    // END dbl arm
  }
  default: RDI_Fatal("should not get here : unexpected widen kind " << widenkind);
  }
  RDI_Fatal("_eval_arithop: Should not get here");
  return 0;  // control does not reach here
}

//////////////////////////////////////////////////////////////////////
// helpful macros
//////////////////////////////////////////////////////////////////////

#define RDI_DECL_iter_i      unsigned int                            iter_i
#define RDI_DECL_seq_len     CORBA::ULong                            seq_len
#define RDI_DECL_tc_rhs      CORBA::TypeCode_var                     tc_rhs
#define RDI_DECL_da_union    WRAPPED_DYNANY_MODULE::DynUnion_var     da_union
#define RDI_DECL_da_enum     WRAPPED_DYNANY_MODULE::DynEnum_var      da_enum
#define RDI_DECL_da_seq      WRAPPED_DYNANY_MODULE::DynSequence_var  da_seq
#define RDI_DECL_da_array    WRAPPED_DYNANY_MODULE::DynArray_var     da_array
#define RDI_DECL_da_struct   WRAPPED_DYNANY_MODULE::DynStruct_var    da_struct
#define RDI_DECL_da_match    WRAPPED_DYNANY_MODULE::DynAny_var       da_match
#define RDI_DECL_da_elt      WRAPPED_DYNANY_MODULE::DynAny_var       da_elt
#define RDI_DECL_da_string   WRAPPED_DYNANY_MODULE::DynAny_var       da_string

//////////////////////////////////////////////////////////////////////
// each of these functions does the work of one opcode
//////////////////////////////////////////////////////////////////////

// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_nop(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_return_b(RDI_StructuredEvent* evp) {
  // <top> must be a bool
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 0, "");
  RDI_RVM_Assert(_stack[_top]._tckind == RDI_rtk_boolean, "");
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  CORBA::Boolean result = _stack[_top]._v_boolean;
  r_bool = result;
  r_code = RDI_RTRet_OK;
  RDI_CLEANUP_RETURN;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_signal_N(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // <top> must be a ushort
  RDI_RVM_Assert(_top >= 0, "");
  RDI_RVM_Assert(_stack[_top]._tckind == RDI_rtk_ushort, "");
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  RDI_RTRetCode rcode = (RDI_RTRetCode)_stack[_top]._v_ushort;
  RDI_RVM_Assert(rcode != RDI_RTRet_OK, "Do not use RDI_OpCode_signal_N for OK result");
  r_code = rcode;
  RDI_CLEANUP_SIGNAL;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_push_cC2c(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // push a length 1 char_or_string constant
  _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
  _stack[_top].set_char_or_string(_op[_PC]._arg._v_sc);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_push_sC2s(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // push a string constant
  _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
  _stack[_top].set_string(_op[_PC]._arg._v_sc);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_push_iC2i(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // push an enum ident
  _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
  _stack[_top].set_enum_ident(_op[_PC]._arg._v_sc);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_push_bC2b(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // push a boolean constant
  _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
  _stack[_top].set_bool(_op[_PC]._arg._v_bc);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_push_NC2N(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // push a ushort constant
  _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
  _stack[_top].set_ushort(_op[_PC]._arg._v_nc_us);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_push_nC2n_s(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // push a short constant
  _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
  _stack[_top].set_short(_op[_PC]._arg._v_nc_s);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_push_nC2n_ul(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // push a ulong constant
  _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
  _stack[_top].set_ulong(_op[_PC]._arg._v_nc_ul);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_push_nC2n_l(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // push a long constant
  _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
  _stack[_top].set_long(_op[_PC]._arg._v_nc_l);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_push_nC2n_ull(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // push a ulonglong constant
  _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
  _stack[_top].set_ulonglong(_op[_PC]._arg._v_nc_ull);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_push_nC2n_ll(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // push a longlong constant
  _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
  _stack[_top].set_longlong(_op[_PC]._arg._v_nc_ll);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_push_nC2n_f(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // push a float constant
  _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
  _stack[_top].set_float(_op[_PC]._arg._v_nc_f);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_push_nC2n_d(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // push a double constant
  _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
  _stack[_top].set_double(_op[_PC]._arg._v_nc_d);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_ctelt_NC2n(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // Assumes evp->get_timestamp is greenwhich time (so tdf is zero), but derived from local clock
  RDI_UtcT evaltime(evp->get_timestamp());
  RDI_ServerQoS* s_qos = RDI::get_server_qos();
  if (s_qos) {
    evaltime.inacclo = s_qos->localClockInaccLo;
    evaltime.inacchi = s_qos->localClockInaccHi;
  } else {
    evaltime.inacclo = 0;
    evaltime.inacchi = 0;
  }
  // the correct curtime elt to push is specified in the opocode (in _arg._v_nc_us)
  switch ((RDI_CurTimeElt)(_op[_PC]._arg._v_nc_us)) {
  case RDI_CurTime_error: { // an invalid $curtime expr was used
    RDI_RVM_DbgLog("generating runtime error for an invalid $curtime expression (there were invalid comp-parts)");
    r_code = RDI_RTRet_NONE_SUCH;
    RDI_CLEANUP_SIGNAL;
  }
  // $curtime cannot be used as operand
  //  in a binop or other expr (XXX could change if we extend support for timevals)
  case RDI_CurTime_itself: {
    RDI_RVM_DbgLog("cannot use $curtime by itself in expr, must select an element, e.g., $curtime.time.low");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_CLEANUP_SIGNAL;
  }
  case RDI_CurTime_time: {
    RDI_RVM_DbgLog("Pushing $curtime.time as a ulonglong");
    _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
    _stack[_top].set_ulonglong(evaltime.time);
#if 0
    // XXX_REMOVE_THIS 
    do {
      RDIDbgLogger1(l);
      l.str << "XXX $curtime.time = ";
      evaltime.out_gmt(l.str);
      l.str << " XXX\n";
    } while (0);
#endif
    break;
  }
  case RDI_CurTime_time_low: {
    RDI_RVM_DbgLog("Pushing $curtime.time.low");
    _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
    _stack[_top].set_ulong(evaltime.low());
    break;
  }
  case RDI_CurTime_time_high: {
    RDI_RVM_DbgLog("Pushing $curtime.time.high");
    _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
    _stack[_top].set_ulong(evaltime.high());
    break;
  }
  case RDI_CurTime_inacclo: {
    RDI_RVM_DbgLog("Pushing $curtime.inacclo");
    _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
    _stack[_top].set_ulong(evaltime.inacclo);
    break;
  }
  case RDI_CurTime_inacchi: {
    RDI_RVM_DbgLog("Pushing $curtime.inacchi");
    _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
    _stack[_top].set_ushort(evaltime.inacchi);
    break;
  }
  case RDI_CurTime_tdf: {
    RDI_RVM_DbgLog("Pushing $curtime.tdf");
    _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
    _stack[_top].set_short(evaltime.tdf);
    break;
  }
  }
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_swap_uu2uu(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 1, "");
  RDI_RVM_DbgLog_Top2(_stack[_top-1], _stack[_top]);
  _stack[_top].swap(_stack[_top-1]);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_pop_u(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 0, "");
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  _stack[_top--].clear();
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_pop_uu(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 1, "");
  RDI_RVM_DbgLog_Top2(_stack[_top-1], _stack[_top]);
  _stack[_top--].clear();
  _stack[_top--].clear();
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_cvt_u2b(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 0, "");
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  if (_stack[_top]._tckind == RDI_rtk_boolean) {
    // nothing to do -- leave top as-is
    RDI_NEXT_OP;
  } else if (_stack[_top]._tckind == RDI_rtk_dynany) {
    // try to pull a bool out
    try {
      CORBA::Boolean tmpres = _stack[_top]._v_dynanyval._my_ptr->get_boolean();
      _stack[_top].set_bool(tmpres); // success
      RDI_NEXT_OP;
    } catch (...) { // will fall through to signal
      RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], _stack[_top]._v_dynanyval._my_ptr, "bool");
    }
  } else {
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "bool");
  }
  r_code = RDI_RTRet_TYPE_MISMATCH;
  RDI_CLEANUP_SIGNAL;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_cvt_u2s(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 0, "");
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  if (_stack[_top]._tckind == RDI_rtk_string) {
    // nothing to do -- leave top as-is
    RDI_NEXT_OP;
  } else if (_stack[_top]._tckind == RDI_rtk_char_or_string) {
    // force to string
    _stack[_top]._tckind = RDI_rtk_string;
    RDI_NEXT_OP;
  } else if (_stack[_top]._tckind == RDI_rtk_dynany) {
    // try to pull a string out
    char* tmpres = NULL;
    try {
      tmpres = _stack[_top]._v_dynanyval._my_ptr->get_string();
    } catch (...) { } //catch exception due to get_string
    if (tmpres == NULL) {
      RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], _stack[_top]._v_dynanyval._my_ptr, "string");
      r_code = RDI_RTRet_TYPE_MISMATCH;
      RDI_CLEANUP_SIGNAL;
    }
    // note that the stack slot is made responsible for freeing the string
    _stack[_top].set_string(tmpres, 1); // success
    RDI_NEXT_OP;
  } else {
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "string");
  }
  r_code = RDI_RTRet_TYPE_MISMATCH;
  RDI_CLEANUP_SIGNAL;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_cvt_u2n(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 0, "");
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  switch (_stack[_top]._tckind) {
  case RDI_rtk_short:
  case RDI_rtk_ushort:
  case RDI_rtk_long:
  case RDI_rtk_ulong:
  case RDI_rtk_float:
  case RDI_rtk_double:
    break; // above are all OK
    // the following 4 kinds can be converted to ushort
  case RDI_rtk_boolean:        _stack[_top].set_ushort((CORBA::UShort)(_stack[_top]._v_boolean ? 1 : 0));  break;
  case RDI_rtk_char:           _stack[_top].set_ushort((CORBA::UShort)(_stack[_top]._v_char));             break;
  case RDI_rtk_octet:          _stack[_top].set_ushort((CORBA::UShort)(_stack[_top]._v_octet));            break;
  case RDI_rtk_char_or_string: _stack[_top].set_ushort((CORBA::UShort)(_stack[_top]._v_string_ptr[0]));    break;
  default: { // remaining kinds cannot be converted to a numeric kind
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "number");
    // signal type mismatch
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_CLEANUP_SIGNAL;
  }
  }
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_or_bb2b (RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert((_top >= 1) && (_stack[_top]._tckind == RDI_rtk_boolean) && (_stack[_top-1]._tckind == RDI_rtk_boolean), "");
  RDI_RVM_DbgLog_Top2(_stack[_top-1], _stack[_top]);
  _stack[_top-1].set_bool(_stack[_top-1]._v_boolean || _stack[_top]._v_boolean);
  _stack[_top--].clear();
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_and_bb2b(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert((_top >= 1) && (_stack[_top]._tckind == RDI_rtk_boolean) && (_stack[_top-1]._tckind == RDI_rtk_boolean), "");
  RDI_RVM_DbgLog_Top2(_stack[_top-1], _stack[_top]);
  _stack[_top-1].set_bool(_stack[_top-1]._v_boolean && _stack[_top]._v_boolean);
  _stack[_top--].clear();
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_not_b2b(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert((_top >= 0) && (_stack[_top]._tckind == RDI_rtk_boolean), "");
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  _stack[_top].set_bool(! (_stack[_top]._v_boolean));
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_in_uu2b(RDI_StructuredEvent* evp) {
  RDI_DECL_iter_i;
  RDI_DECL_tc_rhs;
  RDI_DECL_da_enum;
  RDI_DECL_da_seq;
  RDI_DECL_da_array;
  RDI_DECL_da_elt;
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert((_top >= 1), "");
  RDI_RVM_DbgLog_Top2(_stack[_top-1], _stack[_top]);
  RDI_RTValKind lhs_kind = _stack[_top-1]._tckind;
  if (( ! RDI_TCKIND_NUMERIC(lhs_kind)) &&
      ( ! RDI_TCKIND_STRING(lhs_kind))  &&
      ( ! RDI_TCKIND_ENUM(lhs_kind))) {
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], lhs_kind, "simple type (number, bool, char, octet, enum, or string)");
    // signal type mismatch
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_CLEANUP_SIGNAL;
  }
  // RHS (top) must be a dynany containing a sequence
  if (_stack[_top]._tckind != RDI_rtk_dynany) {
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "sequence or array");
    // signal type mismatch
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_CLEANUP_SIGNAL;
  }
  CORBA::Boolean rhs_is_seq = 1;
  da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
  try {
    da_seq = WRAPPED_DYNANY_MODULE::DynSequence::_narrow(_stack[_top]._v_dynanyval._my_ptr);
    if (! CORBA::is_nil(da_seq)) {
      if (da_seq->seek(0)) {
	da_elt = da_seq->current_component();
      }
    } else {
      rhs_is_seq = 0;
      da_array = WRAPPED_DYNANY_MODULE::DynArray::_narrow(_stack[_top]._v_dynanyval._my_ptr);
      if (!CORBA::is_nil(da_array)) {
	if (da_array->seek(0)) {
	  da_elt = da_array->current_component();
	}
      } else {
	RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], _stack[_top]._v_dynanyval._my_ptr, "sequence or array");
	// signal type mismatch
	r_code = RDI_RTRet_TYPE_MISMATCH;
	RDI_CLEANUP_SIGNAL;
      }
    }
  } catch (...) { } // catch exception due to seek or current_component
  if (CORBA::is_nil(da_elt)) {
    RDI_RVM_DbgLog("exception due to seek or current_component -- assume empty sequence so return false");
    // pop 2, push loop_result
    _stack[_top--].clear();
    _stack[_top].set_bool(0);
    RDI_NEXT_OP;
  }
  tc_rhs = da_elt->type();
      
  if (lhs_kind ==  RDI_rtk_char_or_string) {
    if (RDI_CORBATC_NUMERIC(tc_rhs)) {  // force lhs to long
      _stack[_top-1].set_long((CORBA::Long)(_stack[_top-1]._v_string_ptr[0]));
      lhs_kind       = RDI_rtk_long;
    } else {
      // force lhs to string; may not be compat with rhs elt kind, but we check below
      _stack[_top-1]._tckind = RDI_rtk_string;
      lhs_kind       = RDI_rtk_string;
    }
  }
  if (lhs_kind == RDI_rtk_enum_val) {
    // when comparing enum values (as opposed to enum literal with enum value),
    // typecodes must be equivalent
    if (! tc_rhs->equivalent(_stack[_top-1]._v_enumval._my_ptr->type()) ) {
      RDI_RVM_DbgLog_Op(_PC, _op[_PC], "Type Mismatch: lhs and rhs both enums, but typecodes are not equivalent");
      // signal type mismatch
      r_code = RDI_RTRet_TYPE_MISMATCH;
      RDI_CLEANUP_SIGNAL;
    }
  }
  // finally ready to do a string scan, numeric scan, or enum scan (or signal type mismatch)
  CORBA::Boolean loop_result = 0;
  if (lhs_kind == RDI_rtk_string) {
    // ------------------------------   string scan   ---------------------------------------
    if (! RDI_CORBATC_STRING(tc_rhs)) {
      RDI_RVM_DbgLog_Op_SEQ_Mismatch(_PC, _op[_PC], (*tc_rhs), "string");
      // signal type mismatch
      r_code = RDI_RTRet_TYPE_MISMATCH;
      RDI_CLEANUP_SIGNAL;
    }
    char* lhs_string = _stack[_top-1]._v_string_ptr;
    for (iter_i = 0; ; iter_i++) {
      char* rhs_elt_str = NULL;
      try {
	if (rhs_is_seq) {
	  if (! da_seq->seek(iter_i)) {
	    break; // end of seq
	  }
	  rhs_elt_str = da_seq->get_string();
	} else {
	  if (! da_array->seek(iter_i)) {
	    break; // end of array
	  }
	  rhs_elt_str = da_array->get_string();
	}
      } catch (...) { } // catch exception due to seek or get_string 
      if (rhs_elt_str == NULL) {
	RDI_RVM_DbgLog("exception due to seek or get_string -- assume found end of seq or array");
	break; // break from loop, not from switch
      }
      RDI_RVM_DbgLog("comparing " << lhs_string << " to " << rhs_elt_str);
      loop_result = RDI_STR_EQ(lhs_string, rhs_elt_str);
      CORBA_STRING_FREE(rhs_elt_str);
      if (loop_result) break; // break from loop, not from switch
    }
  } else if (RDI_TCKIND_NUMERIC(lhs_kind)) {
    // ------------------------------   numeric scan  ---------------------------------------
    if (! RDI_CORBATC_NUMERIC(tc_rhs)) {
      RDI_RVM_DbgLog_Op_SEQ_Mismatch(_PC, _op[_PC], (*tc_rhs), "bool, char, octet, or number");
      // signal type mismatch
      r_code = RDI_RTRet_TYPE_MISMATCH;
      RDI_CLEANUP_SIGNAL;
    }
    CORBA::Double lhs_val;
    if (_stack[_top-1].cvt2dbl(lhs_val)) {
      RDI_RVM_DbgLog("unexpected failure converting to double while scanning numeric sequence");
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_CLEANUP_SIGNAL;
    }
    for (iter_i = 0; ; iter_i++) {
      da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
      try {
	if (rhs_is_seq) {
	  if (! da_seq->seek(iter_i)) {
	    break; // end of seq
	  }
	  da_elt = da_seq->current_component();
	} else {
	  if (! da_array->seek(iter_i)) {
	    break; // end of array
	  }
	  da_elt = da_array->current_component();
	}
      } catch (...) { } // catch exception due to seek, next, or current_component
      if (CORBA::is_nil(da_elt)) {
	RDI_RVM_DbgLog("exception due to seek, next, or current_component -- assume found end of seq or array");
	break; // break from loop, not from switch
      }
      // convert and compare
      CORBA::Double rhs_elt_dbl;
      RDI_RTVal rhs_elt_val;
      rhs_elt_val.set_dynany(da_elt, NULL, 0); // not in charge of destroying a "top" dynany 
      rhs_elt_val.simplify();
      if (rhs_elt_val.cvt2dbl(rhs_elt_dbl)) {
	RDI_RVM_DbgLog("unexpected failure converting to double while scanning numeric sequence");
	r_code = RDI_RTRet_NONE_SUCH;
	RDI_CLEANUP_SIGNAL;
      }
      RDI_RVM_DbgLog("comparing " << lhs_val << " to " << rhs_elt_dbl);
      if (lhs_val == rhs_elt_dbl) {
	loop_result = 1; break; // break from loop, not from switch
      }
    }
  } else { // lhs is enum
    // ------------------------------    enum scan    ---------------------------------------
    // When lhs is an enum literal, there is no typecode to use for equivalency check.
    // Due to possible aliasing, the only way to check whether the rhs sequence
    // contains enum values is to attempt to narrow an element of the sequence.

    char* lhs_enum_ident = NULL;
    if (lhs_kind == RDI_rtk_enum_ident) {
      lhs_enum_ident = _stack[_top-1]._v_string_ptr;
    } else  if (lhs_kind == RDI_rtk_enum_val) {
      try {
	// this can return empty string (a valid string of length zero)
	lhs_enum_ident = WRAPPED_DYNENUM_GET_AS_STRING(_stack[_top-1]._v_enumval._my_ptr);
      } catch (...) { } // catch exception due to get_as_string
    } else {
      RDI_Fatal("should not get here");
    }
    if (lhs_enum_ident == NULL) {
      RDI_RVM_DbgLog("unexpected failure determining lhs enum ident");
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_CLEANUP_SIGNAL;
    }
    for (iter_i = 0; ; iter_i++) {
      da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
      try {
	if (rhs_is_seq) {
	  if (! da_seq->seek(iter_i)) {
	    break; // end of seq
	  }
	  da_elt = da_seq->current_component();
	} else {
	  if (! da_array->seek(iter_i)) {
	    break; // end of array
	  }
	  da_elt = da_array->current_component();
	}
      } catch (...) { } // catch exception due to seek, next, or current_component
      if (CORBA::is_nil(da_elt)) {
	RDI_RVM_DbgLog("exception due to seek, next, or current_component -- assume found end of seq or array");
	break; // break from loop, not from switch
      }
      // cast to DynEnum
      da_enum = WRAPPED_DYNANY_MODULE::DynEnum::_narrow(da_elt);
      if (CORBA::is_nil(da_enum)) {
	RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], da_elt, "enum");
	r_code = RDI_RTRet_TYPE_MISMATCH;
	RDI_CLEANUP_SIGNAL;
      }
      char* rhs_elt_enum_ident = NULL;
      try {
	// this can return empty string (a valid string of length zero)
	rhs_elt_enum_ident = WRAPPED_DYNENUM_GET_AS_STRING(da_enum);
      } catch (...) { } // catch exception due to get_as_string
      if (rhs_elt_enum_ident == NULL) {
	RDI_RVM_DbgLog("unexpected failure getting enum ident from rhs eltof enum sequence");
	r_code = RDI_RTRet_NONE_SUCH;
	RDI_CLEANUP_SIGNAL;
      }
      RDI_RVM_DbgLog("comparing " << lhs_enum_ident << " to " << rhs_elt_enum_ident);
      if (RDI_STR_EQ(lhs_enum_ident, rhs_elt_enum_ident)) {
	loop_result = 1; break; // break from loop, not from switch
      } 
    }
  }
  // pop 2, push loop_result
  _stack[_top--].clear();
  _stack[_top].set_bool(loop_result);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_add_nn2n(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 1, "");
  RDI_RVM_DbgLog_Top2(_stack[_top-1], _stack[_top]);
  if (_eval_arithop(RDI_ArithOp_add)) {
    RDI_CLEANUP_SIGNAL;
  }
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_sub_nn2n(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 1, "");
  RDI_RVM_DbgLog_Top2(_stack[_top-1], _stack[_top]);
  if (_eval_arithop(RDI_ArithOp_sub)) {
    RDI_CLEANUP_SIGNAL;
  }
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_mul_nn2n(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 1, "");
  RDI_RVM_DbgLog_Top2(_stack[_top-1], _stack[_top]);
  if (_eval_arithop(RDI_ArithOp_mul)) {
    RDI_CLEANUP_SIGNAL;
  }
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_div_nn2n(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 1, "");
  RDI_RVM_DbgLog_Top2(_stack[_top-1], _stack[_top]);
  if (_eval_arithop(RDI_ArithOp_div)) {
    RDI_CLEANUP_SIGNAL;
  }
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_rem_nn2n(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 1, "");
  RDI_RVM_DbgLog_Top2(_stack[_top-1], _stack[_top]);
  if (_eval_arithop(RDI_ArithOp_rem)) {
    RDI_CLEANUP_SIGNAL;
  }
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_substr_ss2b(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert((_top >= 1) && (_stack[_top]._tckind == RDI_rtk_string) && (_stack[_top-1]._tckind == RDI_rtk_string), "");
  RDI_RVM_DbgLog_Top2(_stack[_top-1], _stack[_top]);
  // test whether lhs (top-1) is contained within rhs (top)
  // note that strstr checks whether arg2 is contained in arg1
  _stack[_top-1].set_bool((CORBA::Boolean)(strstr(_stack[_top]._v_string_ptr, _stack[_top-1]._v_string_ptr) != (char*)0));
  _stack[_top--].clear();
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_cmp_uu2n(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 1, "");
  RDI_RVM_DbgLog_Top2(_stack[_top-1], _stack[_top]);

  RDI_RTValKind      lhs_kind       = _stack[_top-1]._tckind;
  RDI_RTValKind      rhs_kind       = _stack[_top]._tckind;

  if (RDI_TCKIND_STRING(lhs_kind) && RDI_TCKIND_STRING(rhs_kind)) {
    // -------------------------  string lexicographic comparison ----------------------------------------
    _stack[_top-1].set_long((CORBA::Long)RDI_STR_RANK(_stack[_top-1]._v_string_ptr, _stack[_top]._v_string_ptr));
    _stack[_top--].clear();
  } else if (RDI_TCKIND_NUMERIC(lhs_kind) && RDI_TCKIND_NUMERIC(rhs_kind)) {
    // ----------------------------      numeric comparison    -------------------------------------------
    if (RDI_TCKIND_CANBE_NUMBER(lhs_kind)) {
      _force2num(_stack[_top-1]);
      lhs_kind = _stack[_top-1]._tckind;
    }
    if (RDI_TCKIND_CANBE_NUMBER(rhs_kind)) {
      _force2num(_stack[_top]);
      rhs_kind = _stack[_top]._tckind;
    }
    if (_eval_arithop(RDI_ArithOp_cmp)) {
      RDI_CLEANUP_SIGNAL;
    }
  } else if (RDI_TCKIND_ENUM(lhs_kind) && RDI_TCKIND_ENUM(rhs_kind)) {
    // ----------------------------       enum comparison     --------------------------------------------
    // if both are idents it is an error
    if ((lhs_kind == RDI_rtk_enum_ident) && (rhs_kind == RDI_rtk_enum_ident)) {
      RDI_RVM_DbgLog("Cannot compare two enum identifier literals");
      r_code = RDI_RTRet_TYPE_MISMATCH;
      RDI_CLEANUP_SIGNAL;
    }
    // if next opcode is not an equality/inequality test it is an error
    if (_PC+1 > _ops->_opstail) {
      RDI_Fatal("ran off end of opseq checking successor of cmp_uu2n");
    }
    if ((_op[_PC+1]._code != RDI_OpCode_eqz_n2b) && (_op[_PC+1]._code != RDI_OpCode_nez_n2b)) {
      RDI_RVM_DbgLog("Can only use == and != for enum comparison, found  " << 
		 RDI_OpCode2string[_op[_PC+1]._code] << " instead");
      r_code = RDI_RTRet_TYPE_MISMATCH;
      RDI_CLEANUP_SIGNAL;
    }
    // do ident comparison 
    char* lhs_enum_ident = NULL;
    if (lhs_kind == RDI_rtk_enum_ident) {
      lhs_enum_ident = _stack[_top-1]._v_string_ptr;
    } else  if (lhs_kind == RDI_rtk_enum_val) {
      try {
	// this can return empty string (a valid string of length zero)
	lhs_enum_ident = WRAPPED_DYNENUM_GET_AS_STRING(_stack[_top-1]._v_enumval._my_ptr);
      } catch (...) { } // catch exception due to get_as_string
    } else {
      RDI_Fatal("should not get here");
    }
    if (lhs_enum_ident == NULL) {
      RDI_RVM_DbgLog("unexpected failure determining lhs enum ident");
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_CLEANUP_SIGNAL;
    }
    char* rhs_enum_ident = NULL;
    if (rhs_kind == RDI_rtk_enum_ident) {
      rhs_enum_ident = _stack[_top]._v_string_ptr;
    } else  if (rhs_kind == RDI_rtk_enum_val) {
      try {
	// this can return empty string (a valid string of length zero)
	rhs_enum_ident = WRAPPED_DYNENUM_GET_AS_STRING(_stack[_top]._v_enumval._my_ptr);
      } catch (...) { } // catch exception due to get_as_string
    } else {
      RDI_Fatal("should not get here");
    }
    if (rhs_enum_ident == NULL) {
      RDI_RVM_DbgLog("unexpected failure determining rhs enum ident");
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_CLEANUP_SIGNAL;
    }
    RDI_RVM_DbgLog("comparing " << lhs_enum_ident << " to " << rhs_enum_ident);
    _stack[_top-1].set_long((CORBA::Long)RDI_STR_RANK(lhs_enum_ident, rhs_enum_ident));
    _stack[_top--].clear();
  } else {
    // ----------------------------  error -- cannot compare   -------------------------------------------
    RDI_RVM_DbgLog_Op_CMP_Mismatch(_PC, _op[_PC], lhs_kind, rhs_kind);
    // signal type mismatch
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_CLEANUP_SIGNAL;
  }
  // done with compare op
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_eqz_n2b(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 0, "");
  RDI_RTValKind kind = _stack[_top]._tckind;
  RDI_RVM_Assert(RDI_TCKIND_NUMERIC(kind), "");
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  if (RDI_TCKIND_CANBE_NUMBER(kind)) {
    _force2num(_stack[_top]);
    kind = _stack[_top]._tckind;
  }
  CORBA::Boolean res = 0;
  switch (kind) {
  case RDI_rtk_ushort:    res = (_stack[_top]._v_ushort    == 0); break;
  case RDI_rtk_short:     res = (_stack[_top]._v_short     == 0); break;
  case RDI_rtk_ulong:     res = (_stack[_top]._v_ulong     == 0); break;
  case RDI_rtk_long:      res = (_stack[_top]._v_long      == 0); break; 
  case RDI_rtk_ulonglong: res = (_stack[_top]._v_ulonglong == 0); break; 
  case RDI_rtk_longlong:  res = (_stack[_top]._v_longlong  == 0); break; 
  case RDI_rtk_float:     res = (_stack[_top]._v_float     == 0); break;
  case RDI_rtk_double:    res = (_stack[_top]._v_double    == 0); break;
  default: RDI_Fatal("should not get here");
  }
  _stack[_top].set_bool(res);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_nez_n2b(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 0, "");
  RDI_RTValKind kind = _stack[_top]._tckind;
  RDI_RVM_Assert(RDI_TCKIND_NUMERIC(kind), "");
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  if (RDI_TCKIND_CANBE_NUMBER(kind)) {
    _force2num(_stack[_top]);
    kind = _stack[_top]._tckind;
  }
  CORBA::Boolean res = 0;
  switch (kind) {
  case RDI_rtk_ushort:    res = (_stack[_top]._v_ushort    != 0); break;
  case RDI_rtk_short:     res = (_stack[_top]._v_short     != 0); break;
  case RDI_rtk_ulong:     res = (_stack[_top]._v_ulong     != 0); break;
  case RDI_rtk_long:      res = (_stack[_top]._v_long      != 0); break; 
  case RDI_rtk_ulonglong: res = (_stack[_top]._v_ulonglong != 0); break; 
  case RDI_rtk_longlong:  res = (_stack[_top]._v_longlong  != 0); break; 
  case RDI_rtk_float:     res = (_stack[_top]._v_float     != 0); break;
  case RDI_rtk_double:    res = (_stack[_top]._v_double    != 0); break;
  default: RDI_Fatal("should not get here");
  }
  _stack[_top].set_bool(res);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_lez_n2b(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 0, "");
  RDI_RTValKind kind = _stack[_top]._tckind;
  RDI_RVM_Assert(RDI_TCKIND_NUMERIC(kind), "");
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  if (RDI_TCKIND_CANBE_NUMBER(kind)) {
    _force2num(_stack[_top]);
    kind = _stack[_top]._tckind;
  }
  CORBA::Boolean res = 0;
  switch (kind) {
  case RDI_rtk_ushort:    res = (_stack[_top]._v_ushort    <= 0); break;
  case RDI_rtk_short:     res = (_stack[_top]._v_short     <= 0); break;
  case RDI_rtk_ulong:     res = (_stack[_top]._v_ulong     <= 0); break;
  case RDI_rtk_long:      res = (_stack[_top]._v_long      <= 0); break; 
  case RDI_rtk_ulonglong: res = (_stack[_top]._v_ulonglong <= 0); break; 
  case RDI_rtk_longlong:  res = (_stack[_top]._v_longlong  <= 0); break; 
  case RDI_rtk_float:     res = (_stack[_top]._v_float     <= 0); break;
  case RDI_rtk_double:    res = (_stack[_top]._v_double    <= 0); break;
  default: RDI_Fatal("should not get here");
  }
  _stack[_top].set_bool(res);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_ltz_n2b(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 0, "");
  RDI_RTValKind kind = _stack[_top]._tckind;
  RDI_RVM_Assert(RDI_TCKIND_NUMERIC(kind), "");
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  if (RDI_TCKIND_CANBE_NUMBER(kind)) {
    _force2num(_stack[_top]);
    kind = _stack[_top]._tckind;
  }
  CORBA::Boolean res = 0;
  switch (kind) {
  case RDI_rtk_ushort:    res = 0; break; // cannot be ltz
  case RDI_rtk_short:     res = (_stack[_top]._v_short     <  0); break;
  case RDI_rtk_ulong:     res = 0; break; // cannot be ltz
  case RDI_rtk_long:      res = (_stack[_top]._v_long      <  0); break; 
  case RDI_rtk_ulonglong: res = 0; break; // cannot be ltz
  case RDI_rtk_longlong:  res = (_stack[_top]._v_longlong  <  0); break; 
  case RDI_rtk_float:     res = (_stack[_top]._v_float     <  0); break;
  case RDI_rtk_double:    res = (_stack[_top]._v_double    <  0); break;
  default: RDI_Fatal("should not get here");
  }
  _stack[_top].set_bool(res);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_gez_n2b(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 0, "");
  RDI_RTValKind kind = _stack[_top]._tckind;
  RDI_RVM_Assert(RDI_TCKIND_NUMERIC(kind), "");
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  if (RDI_TCKIND_CANBE_NUMBER(kind)) {
    _force2num(_stack[_top]);
    kind = _stack[_top]._tckind;
  }
  CORBA::Boolean res = 0;
  switch (kind) {
  case RDI_rtk_ushort:    res = 1; break; // always true
  case RDI_rtk_short:     res = (_stack[_top]._v_short     >= 0); break;
  case RDI_rtk_ulong:     res = 1; break; // always true
  case RDI_rtk_long:      res = (_stack[_top]._v_long      >= 0); break; 
  case RDI_rtk_ulonglong: res = 1; break; // always true
  case RDI_rtk_longlong:  res = (_stack[_top]._v_longlong  >= 0); break; 
  case RDI_rtk_float:     res = (_stack[_top]._v_float     >= 0); break;
  case RDI_rtk_double:    res = (_stack[_top]._v_double    >= 0); break;
  default: RDI_Fatal("should not get here");
  }
  _stack[_top].set_bool(res);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_gtz_n2b(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_top >= 0, "");
  RDI_RTValKind kind = _stack[_top]._tckind;
  RDI_RVM_Assert(RDI_TCKIND_NUMERIC(kind), "");
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  if (RDI_TCKIND_CANBE_NUMBER(kind)) {
    _force2num(_stack[_top]);
    kind = _stack[_top]._tckind;
  }
  CORBA::Boolean res = 0;
  switch (kind) {
  case RDI_rtk_ushort:    res = (_stack[_top]._v_ushort    >  0); break;
  case RDI_rtk_short:     res = (_stack[_top]._v_short     >  0); break;
  case RDI_rtk_ulong:     res = (_stack[_top]._v_ulong     >  0); break;
  case RDI_rtk_long:      res = (_stack[_top]._v_long      >  0); break; 
  case RDI_rtk_ulonglong: res = (_stack[_top]._v_ulonglong >  0); break; 
  case RDI_rtk_longlong:  res = (_stack[_top]._v_longlong  >  0); break; 
  case RDI_rtk_float:     res = (_stack[_top]._v_float     >  0); break;
  case RDI_rtk_double:    res = (_stack[_top]._v_double    >  0); break;
  default: RDI_Fatal("should not get here");
  }
  _stack[_top].set_bool(res);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_ifT_b2b(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert((_top >= 0) && (_stack[_top]._tckind == RDI_rtk_boolean), "");
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  if (_stack[_top]._v_boolean == 1) {
    RDI_RVM_Assert((_PC + _op[_PC]._arg._v_lbl._offset >= 0), "jump to negative PC value!");
    RDI_RVM_Assert(_op[_PC]._arg._v_lbl._offset != 0, "zero offset causes tight infinite loop!");
    RDI_RVM_Assert((_PC + _op[_PC]._arg._v_lbl._offset <= _ops->_opstail), "jump beyond end of op sequence!");
    _PC += _op[_PC]._arg._v_lbl._offset;
    RDI_RVM_DbgLog("BRANCH TAKEN: _PC is now " << _PC);
    RDI_NEXT_AFTER_JUMP;
  }
  RDI_RVM_DbgLog("BRANCH NOT TAKEN");
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_ifF_b2b(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert((_top >= 0) && (_stack[_top]._tckind == RDI_rtk_boolean), "");
  RDI_RVM_DbgLog_Top1(_stack[_top]);
  if (_stack[_top]._v_boolean == 0) {
    RDI_RVM_Assert(_op[_PC]._arg._v_lbl._offset != 0, "zero offset causes tight infinite loop!");
    RDI_RVM_Assert((_PC + _op[_PC]._arg._v_lbl._offset >= 0), "jump to negative PC value!");
    RDI_RVM_Assert((_PC + _op[_PC]._arg._v_lbl._offset <= _ops->_opstail), "jump beyond end of op sequence!");
    _PC += _op[_PC]._arg._v_lbl._offset;
    RDI_RVM_DbgLog("BRANCH TAKEN: _PC is now " << _PC);
    RDI_NEXT_AFTER_JUMP;
  }
  RDI_RVM_DbgLog("BRANCH NOT TAKEN");
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_goto(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(_op[_PC]._arg._v_lbl._offset != 0, "zero offset causes tight infinite loop!");
  RDI_RVM_Assert((_PC + _op[_PC]._arg._v_lbl._offset >= 0), "jump to negative PC value!");
  RDI_RVM_Assert((_PC + _op[_PC]._arg._v_lbl._offset <= _ops->_opstail), "jump beyond end of op sequence!");
  _PC += _op[_PC]._arg._v_lbl._offset;
  RDI_RVM_DbgLog("BRANCH TAKEN: _PC is now " << _PC);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_label(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // nothing to do
  RDI_NEXT_OP;
}
// ================================= comp 'start' ops ==============================================
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_wkp_NC2u(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(r_code == RDI_RTRet_UNDEFINED, "");
  _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
  _stack[_top].set_null();
  switch ((RDI_WellKnownPart)(_op[_PC]._arg._v_nc_us)) {
  case RDI_WKP_dollar: {
    RDI_RVM_DbgLog("WellKnownPart = $");
    const RDI_RTVal* rtv = evp->get_top_rtval();
    if (! rtv) { // should not happen
      RDI_RVM_DbgLog("unexpected failure evp->get_top_rtval returned NULL");
      r_code = RDI_RTRet_NONE_SUCH;
      break;
    }
    _stack[_top] = *rtv; // _stack[_top] not responsible for state shared w/ rtv
    RDI_RVM_DbgLog("WKP $: <top> set to " << _stack[_top]);
    break;
  }
  case RDI_WKP_hdr: {
    RDI_RVM_DbgLog("WellKnownPart = $header");
    const RDI_RTVal* rtv = evp->get_hdr_rtval();
    if (! rtv) { // could happen if evp contains an any
      RDI_RVM_DbgLog("evp->get_hdr_rtval returned NULL");
      r_code = RDI_RTRet_NONE_SUCH;
      break;
    }
    _stack[_top] = *rtv; // _stack[_top] not responsible for state shared w/ rtv
    RDI_RVM_DbgLog("WKP: _stack[_top] set to " << _stack[_top]);
    break;
  }
  case RDI_WKP_fdata: {
    RDI_RVM_DbgLog("WellKnownPart = $filterable_data");
    const RDI_RTVal* rtv = evp->get_fdata_rtval();
    if (! rtv) { // could happen if evp contains an any
      RDI_RVM_DbgLog("evp->get_fdata_rtval returned NULL");
      r_code = RDI_RTRet_NONE_SUCH;
      break;
    }
    _stack[_top] = *rtv; // _stack[_top] not responsible for state shared w/ rtv
    RDI_RVM_DbgLog("WKP: _stack[_top] set to " << _stack[_top]);
    break;
  }
  case RDI_WKP_rob: {
    RDI_RVM_DbgLog("WellKnownPart = $remainder_of_body");
    const RDI_RTVal* rtv = evp->get_rob_rtval();
    if (! rtv) { // could happen if evp contains an any
      RDI_RVM_DbgLog("evp->get_rob_rtval returned NULL");
      r_code = RDI_RTRet_NONE_SUCH;
      break;
    }
    _stack[_top] = *rtv; // _stack[_top] not responsible for state shared w/ rtv
    RDI_RVM_DbgLog("WKP: _stack[_top] set to " << _stack[_top]);
    break;
  }
  case RDI_WKP_fhdr: {
    RDI_RVM_DbgLog("WellKnownPart = $header.fixed_header");
    const RDI_RTVal* rtv = evp->get_fhdr_rtval();
    if (! rtv) { // could happen if evp contains an any
      RDI_RVM_DbgLog("evp->get_fhdr_rtval returned NULL");
      r_code = RDI_RTRet_NONE_SUCH;
      break;
    }
    _stack[_top] = *rtv; // _stack[_top] not responsible for state shared w/ rtv
    RDI_RVM_DbgLog("WKP: _stack[_top] set to " << _stack[_top]);
    break;
  }
  case RDI_WKP_vhdr: {
    RDI_RVM_DbgLog("WellKnownPart = $header.variable_header");
    const RDI_RTVal* rtv = evp->get_vahdr_rtval();
    if (! rtv) { // could happen if evp contains an any
      RDI_RVM_DbgLog("evp->get_vhdr_rtval returned NULL");
      r_code = RDI_RTRet_NONE_SUCH;
      break;
    }
    _stack[_top] = *rtv; // _stack[_top] not responsible for state shared w/ rtv
    RDI_RVM_DbgLog("WKP: _stack[_top] set to " << _stack[_top]);
    break;
  }
  case RDI_WKP_etype: {
    RDI_RVM_DbgLog("WellKnownPart = $event_type");
    const RDI_RTVal* rtv = evp->get_etype_rtval();
    if (! rtv) { // should not happen
      RDI_RVM_DbgLog("unexpected failure evp->get_etype_rtval returned NULL");
      r_code = RDI_RTRet_NONE_SUCH;
      break;
    }
    _stack[_top] = *rtv; // _stack[_top] not responsible for state shared w/ rtv
    RDI_RVM_DbgLog("WKP: _stack[_top] set to " << _stack[_top]);
    break;
  }
  case RDI_WKP_ename: {
    RDI_RVM_DbgLog("WellKnownPart = $event_name");
    const RDI_RTVal* rtv = evp->get_ename_rtval();
    if (! rtv) { // could happen if evp contains an any
      RDI_RVM_DbgLog("evp->get_ename_rtval returned NULL");
      r_code = RDI_RTRet_NONE_SUCH;
      break;
    }
    _stack[_top] = *rtv; // _stack[_top] not responsible for state shared w/ rtv
    RDI_RVM_DbgLog("WKP: _stack[_top] set to " << _stack[_top]);
    break;
  }
  case RDI_WKP_tname: {
    RDI_RVM_DbgLog("WellKnownPart = $type_name");
    const RDI_RTVal* rtv = evp->get_tname_rtval();
    if (! rtv) { // could happen if evp contains an any
      RDI_RVM_DbgLog("evp->get_tname_rtval returned NULL");
      r_code = RDI_RTRet_NONE_SUCH;
      break;
    }
    _stack[_top] = *rtv; // _stack[_top] not responsible for state shared w/ rtv
    RDI_RVM_DbgLog("WKP: _stack[_top] set to " << _stack[_top]);
    break;
  }
  case RDI_WKP_dname: {
    RDI_RVM_DbgLog("WellKnownPart = $domain_name");
    const RDI_RTVal* rtv = evp->get_dname_rtval();
    if (! rtv) { // could happen if evp contains an any
      RDI_RVM_DbgLog("evp->get_dname_rtval returned NULL");
      r_code = RDI_RTRet_NONE_SUCH;
      break;
    }
    _stack[_top] = *rtv; // _stack[_top] not responsible for state shared w/ rtv
    RDI_RVM_DbgLog("WKP: _stack[_top] set to " << _stack[_top]);
    break;
  }
  }
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_special_sC2u(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_Assert(r_code == RDI_RTRet_UNDEFINED, "");
  _top++; RDI_RVM_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE");
  _stack[_top].set_null();
  char* nm = _op[_PC]._arg._v_sc;
  RDI_RVM_DbgLog("SpecialId = $" << nm);
  RDI_RVM_DbgLog("Looking up " << nm << " using evp->lookup_rtval");
  const RDI_RTVal* rtv = evp->lookup_rtval(nm);
  if (rtv) {
    RDI_RVM_DbgLog("Found " << nm);
    _stack[_top] = *rtv;  // _stack[_top] not responsible for state shared w/ rtv
    RDI_RVM_DbgLog("<top> set to: " << _stack[_top]);
    RDI_NEXT_OP;
  }
  RDI_RVM_DbgLog("Did Not Find " << nm);
  r_code =  RDI_RTRet_NONE_SUCH;
  RDI_NEXT_OP;
}
// =================================  comp 'end' ops  ==============================================
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_compend(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  if (r_code != RDI_RTRet_UNDEFINED) {
    RDI_CLEANUP_SIGNAL;
  }
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_default_X2b(RDI_StructuredEvent* evp) {
  RDI_DECL_da_union;
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  if (r_code == RDI_RTRet_UNDEFINED) {
    if (_stack[_top]._tckind != RDI_rtk_dynany) {
      RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "union");
      r_code = RDI_RTRet_TYPE_MISMATCH;
    } else {
      da_union = WRAPPED_DYNANY_MODULE::DynUnion::_narrow(_stack[_top]._v_dynanyval._my_ptr);
      if (CORBA::is_nil(da_union)) {
	RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], _stack[_top]._v_dynanyval._my_ptr, "union");
	r_code = RDI_RTRet_TYPE_MISMATCH;
      }
    }
  }
  if (r_code != RDI_RTRet_UNDEFINED) {
    RDI_CLEANUP_SIGNAL;
  }
  CORBA::Boolean is_default = 0;
  try {
    is_default = WRAPPED_DYNUNION_IS_SET_TO_DEFAULT_MEMBER(da_union);
  } catch (...) { } // catch exception due to is_set_to_default_member
  _stack[_top].set_bool(is_default);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_exist_X2b(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  // set bool result, restore r_code to undefined state
  _stack[_top].set_bool( (r_code == RDI_RTRet_UNDEFINED) );
  r_code = RDI_RTRet_UNDEFINED;
  RDI_NEXT_OP;
}
// =====================  comp-part ops (which could be end ops)  ==================================
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_dot_len_u2n_l(RDI_StructuredEvent* evp) {
  RDI_DECL_seq_len;
  RDI_DECL_da_seq;
  RDI_DECL_da_array;
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  if (r_code != RDI_RTRet_UNDEFINED) RDI_NEXT_OP; // skipping parts to find an 'end' op 
  // <top>._length -- valid for array or sequence only
  if (_stack[_top]._tckind != RDI_rtk_dynany) {
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "array or sequence");
    r_code = RDI_RTRet_NONE_SUCH;
    RDI_NEXT_OP;
  }
  // attempt to narrow to DynSequence
  try {
    da_seq = WRAPPED_DYNANY_MODULE::DynSequence::_narrow(_stack[_top]._v_dynanyval._my_ptr);
    if (! (CORBA::is_nil(da_seq))) {
      seq_len = WRAPPED_DYNSEQUENCE_GET_LENGTH(da_seq);
      _stack[_top].set_ulong(seq_len);
      RDI_RVM_DbgLog("<top>._length : _stack[_top] set to sequence length = " << _stack[_top]);
      RDI_NEXT_OP;
    }
  } catch (...) {;} // not a sequence
  // attempt to narrow to DynArray
  try {
    da_array = WRAPPED_DYNANY_MODULE::DynArray::_narrow(_stack[_top]._v_dynanyval._my_ptr);
    if (!(CORBA::is_nil(da_array))) {
      // array does not have a length() member
      seq_len = 0;
      while (1) {
	try {
	  if (da_array->seek(seq_len) == 0) break;
	  seq_len++;
	} catch (...) { break;} // catch exception due to seek problem
      }
      _stack[_top].set_ulong(seq_len);
      RDI_RVM_DbgLog("<top>._length : _stack[_top] set to array length = " << _stack[_top]);
      RDI_NEXT_OP;
    }
  } catch (...) {;} // not an array
  // not an array or sequence
  RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], _stack[_top]._v_dynanyval._my_ptr, "array or sequence");
  r_code = RDI_RTRet_NONE_SUCH;
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_dot_d_u2u(RDI_StructuredEvent* evp) {
  RDI_DECL_da_union;
  RDI_DECL_da_elt;
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  if (r_code != RDI_RTRet_UNDEFINED) RDI_NEXT_OP; // skipping parts to find an 'end' op 
  if (_stack[_top]._tckind != RDI_rtk_dynany) {
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "union");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  da_union = WRAPPED_DYNANY_MODULE::DynUnion::_narrow(_stack[_top]._v_dynanyval._my_ptr);
  if (CORBA::is_nil(da_union)) { // not a union
    RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], _stack[_top]._v_dynanyval._my_ptr, "union");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
  try {
    da_elt = WRAPPED_DYNUNION_GET_DISCRIMINATOR(da_union);
  } catch (...) { } // catch exception due to discriminator
  if (CORBA::is_nil(da_elt)) {
    RDI_RVM_DbgLog("unexpected failure to extract discriminator value from a dynunion");
    r_code = RDI_RTRet_NONE_SUCH;
    RDI_NEXT_OP;
  }
  _stack[_top].set_dynany(da_elt, NULL, 0); // stack slot not in charge of destroying a "top" dynany
  _stack[_top].simplify();
  RDI_RVM_DbgLog("<top>._d : <top> set to " << _stack[_top]);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_dot_tid_u2s(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  if (r_code != RDI_RTRet_UNDEFINED) RDI_NEXT_OP; // skipping parts to find an 'end' op
  char* nm = _stack[_top].type_id();
  // note that the stack slot is made responsible for freeing the string
  _stack[_top].set_string(nm, 1);
  RDI_RVM_DbgLog("<top>._type_id : <top> set to " << _stack[_top]);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_dot_rid_u2s(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  if (r_code != RDI_RTRet_UNDEFINED) RDI_NEXT_OP; // skipping parts to find an 'end' op
  char* id = _stack[_top].repos_id();
  // note that the stack slot is made responsible for freeing the string
  _stack[_top].set_string(id, 1);
  RDI_RVM_DbgLog("<top>._repos_id : <top> set to " << _stack[_top]);
  RDI_NEXT_OP;
}
// ================================  comp 'part' ops  ==============================================
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_dot_id_usC2u(RDI_StructuredEvent* evp) {
  RDI_DECL_iter_i;
  RDI_DECL_da_union;
  RDI_DECL_da_struct;
  RDI_DECL_da_elt;
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  if (r_code != RDI_RTRet_UNDEFINED) RDI_NEXT_OP; // skipping parts to find an 'end' op
  if (_stack[_top]._tckind != RDI_rtk_dynany) {
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "struct or union");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  char* id = _op[_PC]._arg._v_sc;
  CORBA::Boolean found_it = 0;
  // attempt to narrow to DynStruct
  try {
    da_struct = WRAPPED_DYNANY_MODULE::DynStruct::_narrow(_stack[_top]._v_dynanyval._my_ptr);
    if (!CORBA::is_nil(da_struct)) {
      // would be nice to avoid string dup / string free over member names
      for (iter_i = 0; !found_it; iter_i++) {
	char* nm = NULL;
	try {
	  if (! da_struct->seek(iter_i)) {
	    break; // end of struct
	  }
	  nm = da_struct->current_member_name();
	} catch(...) { } // catch exceptions due to seek, next, current_member_name
	if (nm == NULL) break; // no more components
	RDI_RVM_DbgLog("comparing " << id << " to " << nm);
	found_it = (RDI_STR_EQ(id, nm));
	RDI_RVM_DbgLog("calling CORBA_STRING_FREE(nm)");
	CORBA_STRING_FREE(nm);
      }
      if (! found_it) {
	RDI_RVM_DbgLog("struct found but it has no " << id << " member");
	r_code = RDI_RTRet_NONE_SUCH;
	RDI_NEXT_OP;
      }
      // found the right member
      RDI_RVM_DbgLog("Found the right member, about to call current_component");
      da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
      try {
	da_elt = da_struct->current_component();
      } catch (...) { } // catch exception due to invalid current_component
      if (CORBA::is_nil(da_elt)) {
	RDI_RVM_DbgLog("unexpected failure to extract struct member");
	r_code = RDI_RTRet_NONE_SUCH;
	RDI_NEXT_OP;
      }
      RDI_RVM_DbgLog("setting top to current component and simplifying");
      _stack[_top].set_dynany(da_elt, NULL, 0); // stack slot not in charge of destroying a "top" dynany
      _stack[_top].simplify();
      RDI_RVM_DbgLog("<top> . " << id << " : <top> set to " << _stack[_top]);
      RDI_NEXT_OP;
    }
  } catch (...) {;} // not a struct
  // attempt to narrow to DynUnion
  try {
    da_union = WRAPPED_DYNANY_MODULE::DynUnion::_narrow(_stack[_top]._v_dynanyval._my_ptr);
    if (!CORBA::is_nil(da_union)) {
      char* nm = NULL;
      try {
	nm = da_union->member_name();
      } catch (...) { } // catch exception due to member_name
      if (nm == NULL) {
	RDI_RVM_DbgLog("unexpected failure to extract union member name");
	r_code = RDI_RTRet_NONE_SUCH;
	RDI_NEXT_OP;
      }
      RDI_RVM_DbgLog("comparing " << id << " to " << nm);
      found_it = (RDI_STR_EQ(id, nm));
      CORBA_STRING_FREE(nm);
      if (!found_it) {
	RDI_RVM_DbgLog("not the right member name -- raise NONE_SUCH");
	r_code = RDI_RTRet_NONE_SUCH;
	RDI_NEXT_OP;
      }
      da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
      try {
	da_elt = da_union->member();
      } catch (...) { } // catch exception due to member
      if (CORBA::is_nil(da_elt)) {
	RDI_RVM_DbgLog("unexpected failure to extract member value from a dynunion");
	r_code = RDI_RTRet_NONE_SUCH;
	RDI_NEXT_OP;
      }
      _stack[_top].set_dynany(da_elt, NULL, 0); // stack slot not in charge of destroying a "top" dynany
      _stack[_top].simplify();
      RDI_RVM_DbgLog("<top> . " << id << " : <top> set to " << _stack[_top]);
      RDI_NEXT_OP;
    }
  } catch (...) {;} // not a union
  // not a struct or union
  RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], _stack[_top]._v_dynanyval._my_ptr, "struct or union");
  r_code = RDI_RTRet_TYPE_MISMATCH;
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_dot_num_ulC2u(RDI_StructuredEvent* evp) {
  RDI_DECL_tc_rhs;
  RDI_DECL_da_union;
  RDI_DECL_da_struct;
  RDI_DECL_da_elt;
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  if (r_code != RDI_RTRet_UNDEFINED) RDI_NEXT_OP; // skipping parts to find an 'end' op
  if (_stack[_top]._tckind != RDI_rtk_dynany) {
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "struct or union");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  CORBA::Long num = _op[_PC]._arg._v_nc_l;
  // try to narrow to DynStruct
  try {
    da_struct = WRAPPED_DYNANY_MODULE::DynStruct::_narrow(_stack[_top]._v_dynanyval._my_ptr);
    if (!CORBA::is_nil(da_struct)) {
      da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
      try {
	if (da_struct->seek(num)) {
	  da_elt = da_struct->current_component();
	}
      } catch (...) { } // catch exceptions due to seek, current_component
      if (CORBA::is_nil(da_elt)) {
	RDI_RVM_DbgLog("no member with seek offset " << num);
	r_code = RDI_RTRet_NONE_SUCH;
	RDI_NEXT_OP;
      }
      _stack[_top].set_dynany(da_elt, NULL, 0); // stack slot not in charge of destroying a "top" dynany
      _stack[_top].simplify();
      RDI_RVM_DbgLog("<top> . " << num << " : <top> set to " << _stack[_top]);
      RDI_NEXT_OP;
    }
  } catch (...) {;} // not a struct
  // try to narrow to DynUnion
  try {
    da_union = WRAPPED_DYNANY_MODULE::DynUnion::_narrow(_stack[_top]._v_dynanyval._my_ptr);
    if (!CORBA::is_nil(da_union)) {
      char* nm = NULL;
      try {
	nm = da_union->member_name();
      } catch (...) { } // catch exception due to member_name
      if (nm == NULL) {
	RDI_RVM_DbgLog("unexpected failure to extract union member name");
	r_code = RDI_RTRet_NONE_SUCH;
	RDI_NEXT_OP;
      }
      tc_rhs = da_union->type();
      tc_rhs = tc_rhs->content_type();
      CORBA::Long members = ((CORBA::Long)tc_rhs->member_count());
      if (num >= members) {
	RDI_RVM_DbgLog("offset " << num << " is >= member count " << members);
	r_code = RDI_RTRet_NONE_SUCH;
	RDI_NEXT_OP;
      }
      const char* member_nm = tc_rhs->member_name(num);
      if (!member_nm) member_nm = "";
      if (RDI_STR_NEQ(nm, member_nm)) {
	RDI_RVM_DbgLog("offset " << num << " has mamber name " << member_nm << " but current member has name " << nm);
	r_code = RDI_RTRet_NONE_SUCH;
	RDI_NEXT_OP;
      }
      // the correct member was selected
      da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
      try {
	da_elt = da_union->member();
      } catch (...) { } // catch exception due to member
      if (CORBA::is_nil(da_elt)) {
	RDI_RVM_DbgLog("unexpected failure to extract member value from a dynunion");
	r_code = RDI_RTRet_NONE_SUCH;
	RDI_NEXT_OP;
      }
      _stack[_top].set_dynany(da_elt, NULL, 0); // stack slot not in charge of destroying a "top" dynany
      _stack[_top].simplify();
      RDI_RVM_DbgLog("<top> . " << num << " : <top> set to " << _stack[_top]);
      RDI_NEXT_OP;
    }
  } catch (...) {;} // not a union
  // not a struct or union
  RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], _stack[_top]._v_dynanyval._my_ptr, "struct or union");
  r_code = RDI_RTRet_TYPE_MISMATCH;
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_assoc_usC2u(RDI_StructuredEvent* evp) {
  RDI_DECL_iter_i;
  RDI_DECL_seq_len;
  RDI_DECL_da_seq;
  RDI_DECL_da_match;
  RDI_DECL_da_elt;
  RDI_DECL_da_string;
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  if (r_code != RDI_RTRet_UNDEFINED) RDI_NEXT_OP; // skipping parts to find an 'end' op 
  if (_stack[_top]._tckind != RDI_rtk_dynany) {
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "nvpair sequence");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  da_seq = WRAPPED_DYNANY_MODULE::DynSequence::_narrow(_stack[_top]._v_dynanyval._my_ptr);
  if (CORBA::is_nil(da_seq)) {
    RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], _stack[_top]._v_dynanyval._my_ptr, "nvpair sequence");
    r_code = RDI_RTRet_NONE_SUCH;
    RDI_NEXT_OP;
  }
  char* nm = _op[_PC]._arg._v_sc;
  CORBA::Boolean scan_error = 0;
  CORBA::Boolean type_mismatch = 0;
  da_match = WRAPPED_DYNANY_MODULE::DynAny::_nil();
  seq_len = 0;
  try {
    seq_len = WRAPPED_DYNSEQUENCE_GET_LENGTH(da_seq);
  } catch (...) { } // catch exception due to length
  try {
    for (iter_i = 0; iter_i < seq_len; iter_i++) {
      if (!da_seq->seek(iter_i)) {
	break; // end of seq
      }
      da_elt = da_seq->current_component();
      da_elt->seek(0); // necessary if we extracted from this struct before
      da_string = da_elt->current_component();
      RDI_RVM_DbgLog("da_string typecode " << *(da_string->type()));
      char* eltnm = da_string->get_string();
      RDI_RVM_DbgLog("eltnm = " << (eltnm ? eltnm : ""));
      CORBA::Boolean found_it = (eltnm && RDI_STR_EQ(nm, eltnm));
      if (eltnm) {
	CORBA_STRING_FREE(eltnm);
      }
      if (found_it) {
	da_elt->next();
	da_match = da_elt->current_component();
	RDI_RVM_DbgLog("da_match typecode " << *(da_match->type()));
	break;
      }
    }
  } catch (...) { // catch exception due to seek, current_component, get_string
    scan_error = 1;
    da_match = WRAPPED_DYNANY_MODULE::DynAny::_nil();
  } 
  if (! CORBA::is_nil(da_match)) {
    RDI_RVM_DbgLog("Found " << nm);
    _stack[_top].set_dynany(da_match, NULL, 0); // stack slot not in charge of destroying a "top" dynany
    _stack[_top].simplify();
    RDI_RVM_DbgLog("<top> set to: " << _stack[_top]);
    RDI_NEXT_OP;
  }
  if (type_mismatch) {
    RDI_RVM_DbgLog("assoc: sequence elements are not name/value pairs");
    r_code = RDI_RTRet_NONE_SUCH;
    RDI_NEXT_OP;
  }
  if (scan_error) { // something is wrong
    RDI_RVM_DbgLog("unexpected failure during scan of a dynsequence, did not finish loop");
    // fall through to NONE_SUCH
  }
  RDI_RVM_DbgLog("Did Not Find " << nm);
  r_code = RDI_RTRet_NONE_SUCH;
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_index_ulC2u(RDI_StructuredEvent* evp) {
  RDI_DECL_da_seq;
  RDI_DECL_da_array;
  RDI_DECL_da_elt;
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  if (r_code != RDI_RTRet_UNDEFINED) RDI_NEXT_OP; // skipping parts to find an 'end' op 
  CORBA::Long num = _op[_PC]._arg._v_nc_l;
  if (_stack[_top]._tckind != RDI_rtk_dynany) {
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "array or sequence");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
  // try sequence
  da_seq = WRAPPED_DYNANY_MODULE::DynSequence::_narrow(_stack[_top]._v_dynanyval._my_ptr);
  if (! CORBA::is_nil(da_seq)) {
    da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
    try {
      if (da_seq->seek(num)) {
	da_elt = da_seq->current_component();
      }
    } catch (...) { } // catch exception due to seek or current_component
    // fall through to da_elt check
  } else {
    // try array
    da_array = WRAPPED_DYNANY_MODULE::DynArray::_narrow(_stack[_top]._v_dynanyval._my_ptr);
    if (CORBA::is_nil(da_array)) {
      RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], _stack[_top]._v_dynanyval._my_ptr, "array or sequence");
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_NEXT_OP;
    }
    try {
      if (da_array->seek(num)) {
	da_elt = da_array->current_component();
      }
    } catch (...) { } // catch exception due to seek or current_component
  }
  if (CORBA::is_nil(da_elt)) {
    RDI_RVM_DbgLog("either " << num << " is not a legal seq/array offset, or an unexpected error occured");
    r_code = RDI_RTRet_NONE_SUCH;
    RDI_NEXT_OP;
  }
  _stack[_top].set_dynany(da_elt, NULL, 0); // stack slot not in charge of destroying a "top" dynany
  _stack[_top].simplify();
  RDI_RVM_DbgLog("<top> [" << num << "] : <top> set to: " << _stack[_top]);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_tagdef_u2u(RDI_StructuredEvent* evp) {
  RDI_DECL_da_union;
  RDI_DECL_da_elt;
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  if (r_code != RDI_RTRet_UNDEFINED) RDI_NEXT_OP; // skipping parts to find an 'end' op 
  if (_stack[_top]._tckind != RDI_rtk_dynany) {
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "union");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  da_union = WRAPPED_DYNANY_MODULE::DynUnion::_narrow(_stack[_top]._v_dynanyval._my_ptr);
  if (CORBA::is_nil(da_union)) {
    RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], _stack[_top]._v_dynanyval._my_ptr, "union");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
  try {
    if (! WRAPPED_DYNUNION_IS_SET_TO_DEFAULT_MEMBER(da_union)) {
      RDI_RVM_DbgLog("<top> . ( ) : discriminator is not default");
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_NEXT_OP;
    }
    da_elt = da_union->member();
  } catch (...) { } // catch exception due to is_set_to_default_member, member
  if (CORBA::is_nil(da_elt)) {
    RDI_RVM_DbgLog("unexpected failure to extract member value from a dynunion");
    r_code = RDI_RTRet_NONE_SUCH;
    RDI_NEXT_OP;
  }
  _stack[_top].set_dynany(da_elt, NULL, 0); // stack slot not in charge of destroying a "top" dynany
  _stack[_top].simplify();
  RDI_RVM_DbgLog("<top> . ( ) : <top> set to " << _stack[_top]);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_tagid_usC2u(RDI_StructuredEvent* evp) {
  RDI_DECL_da_union;
  RDI_DECL_da_enum;
  RDI_DECL_da_match;
  RDI_DECL_da_elt;
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  if (r_code != RDI_RTRet_UNDEFINED) RDI_NEXT_OP; // skipping parts to find an 'end' op 
  if (_stack[_top]._tckind != RDI_rtk_dynany) {
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "union");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  char* nm = _op[_PC]._arg._v_sc;
  da_union = WRAPPED_DYNANY_MODULE::DynUnion::_narrow(_stack[_top]._v_dynanyval._my_ptr);
  if (CORBA::is_nil(da_union)) {
    RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], _stack[_top]._v_dynanyval._my_ptr, "union");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
  try {
    da_match = WRAPPED_DYNUNION_GET_DISCRIMINATOR(da_union);
    if (CORBA::is_nil(da_match)) {
      RDI_RVM_DbgLog("unexpected failure to extract discriminator value from a dynunion");
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_NEXT_OP;
    }
    da_enum = WRAPPED_DYNANY_MODULE::DynEnum::_narrow(da_match);
    if (CORBA::is_nil(da_enum)) {
      RDI_RVM_DbgLog("<top> . (id) : discriminator kind not enum");
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_NEXT_OP;
    }
    char* tag_id = WRAPPED_DYNENUM_GET_AS_STRING(da_enum);
    if (tag_id == NULL) {
      RDI_RVM_DbgLog("unexpected failure extracting string from dynenum");
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_NEXT_OP;
    }
    if (!RDI_STR_EQ(nm, tag_id)) {
      RDI_RVM_DbgLog("<top> . (id) : id " << nm << " does not match actual tag " << tag_id);
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_NEXT_OP;
    }
    // nm is the correct tag
    da_elt = da_union->member();
  } catch (...) { } // catch exceptions due to discriminator_kind, discriminator, get_as_string, member
  if (CORBA::is_nil(da_elt)) {
    RDI_RVM_DbgLog("unexpected failure testing tag or extracting member value from a dynunion");
    r_code = RDI_RTRet_NONE_SUCH;
    RDI_NEXT_OP;
  }
  _stack[_top].set_dynany(da_elt, NULL, 0); // stack slot not in charge of destroying a "top" dynany
  _stack[_top].simplify();
  RDI_RVM_DbgLog("<top> . (" << nm << ") : <top> set to " << _stack[_top]);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_tagnum_ulC2u(RDI_StructuredEvent* evp) {
  RDI_DECL_da_union;
  RDI_DECL_da_match;
  RDI_DECL_da_elt;
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  if (r_code != RDI_RTRet_UNDEFINED) RDI_NEXT_OP; // skipping parts to find an 'end' op 
  if (_stack[_top]._tckind != RDI_rtk_dynany) {
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "union");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  CORBA::Double num = (CORBA::Double) _op[_PC]._arg._v_nc_l;
  da_union = WRAPPED_DYNANY_MODULE::DynUnion::_narrow(_stack[_top]._v_dynanyval._my_ptr);
  if (CORBA::is_nil(da_union)) {
    RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], _stack[_top]._v_dynanyval._my_ptr, "union");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
  try {
    da_match = WRAPPED_DYNUNION_GET_DISCRIMINATOR(da_union);
    if (CORBA::is_nil(da_match)) {
      RDI_RVM_DbgLog("unexpected failure to extract discriminator value from a dynunion");
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_NEXT_OP;
    }
    CORBA::Double d_dbl;
    RDI_RTVal d_val;
    d_val.set_dynany(da_match, NULL, 0); // not in charge of destroying a "top" dynany 
    d_val.simplify(); // not necessary (what are legal tag kinds?)
    if (d_val.cvt2dbl(d_dbl)) {
      RDI_RVM_DbgLog("<top> . (#) : discriminator value not numeric");
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_NEXT_OP;
    }
    if (num != d_dbl) {
      RDI_RVM_DbgLog("<top> . (#) : num " << (CORBA::Long)num << " does not match actual tag " << (CORBA::Long)d_dbl);
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_NEXT_OP;
    }
    // num is the correct tag
    da_elt = da_union->member();
  } catch (...) { } // catch exceptions due to discriminator_kind, discriminator, member
  if (CORBA::is_nil(da_elt)) {
    RDI_RVM_DbgLog("unexpected failure testing tag or extracting member value from a dynunion");
    r_code = RDI_RTRet_NONE_SUCH;
    RDI_NEXT_OP;
  }
  _stack[_top].set_dynany(da_elt, NULL, 0); // stack slot not in charge of destroying a "top" dynany
  _stack[_top].simplify();
  RDI_RVM_DbgLog("<top> . (" << (CORBA::Long)num << ") : <top> set to " << _stack[_top]);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_tagchar_usC2u(RDI_StructuredEvent* evp) {
  RDI_DECL_da_union;
  RDI_DECL_da_match;
  RDI_DECL_da_elt;
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  if (r_code != RDI_RTRet_UNDEFINED) RDI_NEXT_OP; // skipping parts to find an 'end' op 
  if (_stack[_top]._tckind != RDI_rtk_dynany) {
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "union");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  CORBA::Double num = ((CORBA::Double) _op[_PC]._arg._v_sc[0]);
  da_union = WRAPPED_DYNANY_MODULE::DynUnion::_narrow(_stack[_top]._v_dynanyval._my_ptr);
  if (CORBA::is_nil(da_union)) {
    RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], _stack[_top]._v_dynanyval._my_ptr, "union");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
  try {
    da_match = WRAPPED_DYNUNION_GET_DISCRIMINATOR(da_union);
    if (CORBA::is_nil(da_match)) {
      RDI_RVM_DbgLog("unexpected failure to extract discriminator value from a dynunion");
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_NEXT_OP;
    }
    CORBA::Double d_dbl;
    RDI_RTVal d_val;
    d_val.set_dynany(da_match, NULL, 0); // not in charge of destroying a "top" dynany 
    d_val.simplify(); // not necessary (what are legal tag kinds?)
    if (d_val.cvt2dbl(d_dbl)) {
      RDI_RVM_DbgLog("<top> . (#) : discriminator value not numeric");
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_NEXT_OP;
    }
    if (num != d_dbl) {
      RDI_RVM_DbgLog("<top> . (#) : num " << (CORBA::Long)num << " does not match actual tag " << (CORBA::Long)d_dbl);
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_NEXT_OP;
    }
    // num is the correct tag
    da_elt = da_union->member();
  } catch (...) { } // catch exceptions due to discriminator_kind, discriminator, member
  if (CORBA::is_nil(da_elt)) {
    RDI_RVM_DbgLog("unexpected failure testing tag or extracting member value from a dynunion");
    r_code = RDI_RTRet_NONE_SUCH;
    RDI_NEXT_OP;
  }
  _stack[_top].set_dynany(da_elt, NULL, 0); // stack slot not in charge of destroying a "top" dynany
  _stack[_top].simplify();
  RDI_RVM_DbgLog("<top> . (" << (CORBA::Long)num << ") : <top> set to " << _stack[_top]);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_tagbool_ubC2u(RDI_StructuredEvent* evp) {
  RDI_DECL_da_union;
  RDI_DECL_da_match;
  RDI_DECL_da_elt;
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  if (r_code != RDI_RTRet_UNDEFINED) RDI_NEXT_OP; // skipping parts to find an 'end' op 
  if (_stack[_top]._tckind != RDI_rtk_dynany) {
    RDI_RVM_DbgLog_Op_TC_Mismatch(_PC, _op[_PC], _stack[_top]._tckind, "union");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  CORBA::Double num = (_op[_PC]._arg._v_bc ? ((CORBA::Double)1) : ((CORBA::Double)0));
  da_union = WRAPPED_DYNANY_MODULE::DynUnion::_narrow(_stack[_top]._v_dynanyval._my_ptr);
  if (CORBA::is_nil(da_union)) {
    RDI_RVM_DbgLog_Op_DA_Mismatch(_PC, _op[_PC], _stack[_top]._v_dynanyval._my_ptr, "union");
    r_code = RDI_RTRet_TYPE_MISMATCH;
    RDI_NEXT_OP;
  }
  da_elt = WRAPPED_DYNANY_MODULE::DynAny::_nil();
  try {
    da_match = WRAPPED_DYNUNION_GET_DISCRIMINATOR(da_union);
    if (CORBA::is_nil(da_match)) {
      RDI_RVM_DbgLog("unexpected failure to extract discriminator value from a dynunion");
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_NEXT_OP;
    }
    CORBA::Double d_dbl;
    RDI_RTVal d_val;
    d_val.set_dynany(da_match, NULL, 0); // not in charge of destroying a "top" dynany 
    d_val.simplify(); // not necessary (what are legal tag kinds?)
    if (d_val.cvt2dbl(d_dbl)) {
      RDI_RVM_DbgLog("<top> . (#) : discriminator value not numeric");
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_NEXT_OP;
    }
    if (num != d_dbl) {
      RDI_RVM_DbgLog("<top> . (#) : num " << (CORBA::Long)num << " does not match actual tag " << (CORBA::Long)d_dbl);
      r_code = RDI_RTRet_NONE_SUCH;
      RDI_NEXT_OP;
    }
    // num is the correct tag
    da_elt = da_union->member();
  } catch (...) { } // catch exceptions due to discriminator_kind, discriminator, member
  if (CORBA::is_nil(da_elt)) {
    RDI_RVM_DbgLog("unexpected failure testing tag or extracting member value from a dynunion");
    r_code = RDI_RTRet_NONE_SUCH;
    RDI_NEXT_OP;
  }
  _stack[_top].set_dynany(da_elt, NULL, 0); // stack slot not in charge of destroying a "top" dynany
  _stack[_top].simplify();
  RDI_RVM_DbgLog("<top> . (" << (CORBA::Long)num << ") : <top> set to " << _stack[_top]);
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_debug1(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_DbgLog("==> ?debug1? <==");
  RDI_NEXT_OP;
}
// -------------------------------------------------------------------------------------------------
CORBA::Boolean RDI_RVM::_eval_debug2(RDI_StructuredEvent* evp) {
  RDI_RVM_DbgLog_Op(_PC, _op[_PC], "=>BEGIN");
  RDI_RVM_DbgLog("==> ?debug2? <==");
  RDI_NEXT_OP;
}


//////////////////////////////////////////////////////////////////////
//  eval:  Evaluate the constraint expression(s) on the input event
//////////////////////////////////////////////////////////////////////

#define RDI_EVAL_CASE(op) case RDI_OpCode_ ## op : if (_eval_ ## op(evp)) return; break

void RDI_RVM::eval(RDI_StructuredEvent* evp) 
{
  // Reset state: _PC is set to 0 for legal opseq, else -1
  //              _stack is cleared
  //              r_code is set to RDI_RTRet_UNDEFINED
  //              r_bool is set to FALSE
  this->reset();
  RDI_RVM_Assert(_PC == 0, "");
  while ( 1 ) {
    switch (_op[_PC]._code) {
      RDI_EVAL_CASE(nop);
      RDI_EVAL_CASE(return_b);
      RDI_EVAL_CASE(signal_N);
      RDI_EVAL_CASE(push_cC2c);
      RDI_EVAL_CASE(push_sC2s);
      RDI_EVAL_CASE(push_iC2i);
      RDI_EVAL_CASE(push_bC2b);
      RDI_EVAL_CASE(push_NC2N);
      RDI_EVAL_CASE(push_nC2n_s);
      RDI_EVAL_CASE(push_nC2n_ul);
      RDI_EVAL_CASE(push_nC2n_l);
      RDI_EVAL_CASE(push_nC2n_ull);
      RDI_EVAL_CASE(push_nC2n_ll);
      RDI_EVAL_CASE(push_nC2n_f);
      RDI_EVAL_CASE(push_nC2n_d);
      RDI_EVAL_CASE(ctelt_NC2n);
      RDI_EVAL_CASE(swap_uu2uu);
      RDI_EVAL_CASE(pop_u);
      RDI_EVAL_CASE(pop_uu);
      RDI_EVAL_CASE(cvt_u2b);
      RDI_EVAL_CASE(cvt_u2s);
      RDI_EVAL_CASE(cvt_u2n);
      RDI_EVAL_CASE(or_bb2b);
      RDI_EVAL_CASE(and_bb2b);
      RDI_EVAL_CASE(not_b2b);
      RDI_EVAL_CASE(in_uu2b);
      RDI_EVAL_CASE(add_nn2n);
      RDI_EVAL_CASE(sub_nn2n);
      RDI_EVAL_CASE(mul_nn2n);
      RDI_EVAL_CASE(div_nn2n);
      RDI_EVAL_CASE(rem_nn2n);
      RDI_EVAL_CASE(substr_ss2b);
      RDI_EVAL_CASE(cmp_uu2n);
      RDI_EVAL_CASE(eqz_n2b);
      RDI_EVAL_CASE(nez_n2b);
      RDI_EVAL_CASE(lez_n2b);
      RDI_EVAL_CASE(ltz_n2b);
      RDI_EVAL_CASE(gez_n2b);
      RDI_EVAL_CASE(gtz_n2b);
      RDI_EVAL_CASE(ifT_b2b);
      RDI_EVAL_CASE(ifF_b2b);
      RDI_EVAL_CASE(goto);
      RDI_EVAL_CASE(label);
      RDI_EVAL_CASE(wkp_NC2u);
      RDI_EVAL_CASE(special_sC2u);
      RDI_EVAL_CASE(compend);
      RDI_EVAL_CASE(default_X2b);
      RDI_EVAL_CASE(exist_X2b);
      RDI_EVAL_CASE(dot_len_u2n_l);
      RDI_EVAL_CASE(dot_d_u2u);
      RDI_EVAL_CASE(dot_tid_u2s);
      RDI_EVAL_CASE(dot_rid_u2s);
      RDI_EVAL_CASE(dot_id_usC2u);
      RDI_EVAL_CASE(dot_num_ulC2u);
      RDI_EVAL_CASE(assoc_usC2u);
      RDI_EVAL_CASE(index_ulC2u);
      RDI_EVAL_CASE(tagdef_u2u);
      RDI_EVAL_CASE(tagid_usC2u);
      RDI_EVAL_CASE(tagnum_ulC2u);
      RDI_EVAL_CASE(tagchar_usC2u);
      RDI_EVAL_CASE(tagbool_ubC2u);
      RDI_EVAL_CASE(debug1);
      RDI_EVAL_CASE(debug2);
    } // end opcode switch
  }  // end while 
}

