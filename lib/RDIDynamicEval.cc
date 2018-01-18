// -*- Mode: C++; -*-
//                              File      : RDIDynamicEval.cc
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
//    Implementation of support functionality for EXTENDED_TCL parser 
//

#include "corba_wrappers.h"
#include "RDI.h"
#include "RDITimeWrappers.h"
#include "RDIDynamicEvalDefs.h"

////////////////////////////////////////////////////
// useful TABLES

const char* RDI_RTValKind2string[] = {
  "RDI_rtk_null",
  "RDI_rtk_void",       
  "RDI_rtk_ushort",     
  "RDI_rtk_short",      
  "RDI_rtk_ulong",      
  "RDI_rtk_long",       
  "RDI_rtk_ulonglong",      
  "RDI_rtk_longlong",      
  "RDI_rtk_float",      
  "RDI_rtk_double",     
  "RDI_rtk_boolean",    
  "RDI_rtk_char",       
  "RDI_rtk_octet",      
  "RDI_rtk_enum_ident",       
  "RDI_rtk_enum_val",       
  "RDI_rtk_string",     
  "RDI_rtk_char_or_string",     
  "RDI_rtk_abstime",    
  "RDI_rtk_reltime",    
  "RDI_rtk_dynany"     
};

/////////////////////////////////////////////
// RTVal methods

void RDI_RTVal::init_from_any(CORBA::Any& a, CORBA::Boolean dup_n_free) {
  try {
    CORBA::TypeCode_var tmp_tcp = a.type();
    if (CORBA::_tc_null->equivalent(tmp_tcp)) {
      set_null();
      return;
    }
    if (CORBA::_tc_void->equivalent(tmp_tcp)) {
      set_void();
      return;
    }
    if (CORBA::_tc_ushort->equivalent(tmp_tcp)) {
      CORBA::UShort c; a >>= c;
      set_ushort(c);
      return;
    }
    if (CORBA::_tc_short->equivalent(tmp_tcp)) {
      CORBA::Short c; a >>= c;
      set_short(c);
      return;
    }
    if (CORBA::_tc_ulong->equivalent(tmp_tcp)) {
      CORBA::ULong c; a >>= c;
      set_ulong(c);
      return;
    }
    if (CORBA::_tc_long->equivalent(tmp_tcp)) {
      CORBA::Long c; a >>= c;
      set_long(c);
      return;
    }
#ifndef TIMEBASE_NOLONGLONG
    if (CORBA::_tc_ulonglong->equivalent(tmp_tcp)) {
      CORBA::ULongLong c; a >>= c;
      set_ulonglong(c);
      return;
    }
    if (CORBA::_tc_longlong->equivalent(tmp_tcp)) {
      CORBA::LongLong c; a >>= c;
      set_longlong(c);
      return;
    }
#endif
    if (CORBA::_tc_float->equivalent(tmp_tcp)) {
      CORBA::Float c; a >>= c;
      set_float(c);
      return;
    }
    if (CORBA::_tc_double->equivalent(tmp_tcp)) {
      CORBA::Double c; a >>= c;
      set_double(c);
      return;
    }
    if (CORBA::_tc_boolean->equivalent(tmp_tcp)) {
      CORBA::Boolean c; CORBA::Any::to_boolean to_c(c); a >>= to_c;
      set_bool(c);
      return;
    }
    if (CORBA::_tc_char->equivalent(tmp_tcp)) {
      CORBA::Char c; CORBA::Any::to_char to_c(c); a >>= to_c;
      set_char(c);
      return;
    }
    if (CORBA::_tc_octet->equivalent(tmp_tcp)) {
      CORBA::Octet c; CORBA::Any::to_octet to_c(c); a >>= to_c;
      set_octet(c);
      return;
    }
    if (CORBA::_tc_string->equivalent(tmp_tcp)) {
      const char* c = 0; a >>= c;
      if (dup_n_free) {
	char* c2 = CORBA::string_dup(c);
	set_string(c2, 1);
      } else {
	set_string((char*)c, 0);
      }
      return;
    }
    // Generic case: create a dynany object, set_dynany, then simplify.
    // (Could use generic case for all above cases as well, but that would be inefficient.)
    //
    // In this case dup_n_free is not relevant -- create_dyn_any creates
    // a completely independent object.  (It seems string is the only case
    // where dup_n_free false saves some copying.)
    WRAPPED_DYNANY_MODULE::DynAny_var da = WRAPPED_CREATE_DYN_ANY(a);
    set_dynany(da, da, 1); // this is in charge of destroying a "top" dynany da
    simplify();
    return;
  } catch (...) {
    RDIDbgEvalLog("RDI_RTVal::init_from_any encountered an exception -- initializing rtval to NULL\n");
    set_null();
    return;
  }
}

void RDI_RTVal::simplify(void) {
  WRAPPED_DYNANY_MODULE::DynAny_var unwrap_da = WRAPPED_DYNANY_MODULE::DynAny::_nil();
  CORBA::Any_var unwrap_any;

  if (_tckind != RDI_rtk_dynany) return;

  CORBA::TypeCode_var tmp_tcp = _v_dynanyval._my_ptr->type();

  if (CORBA::_tc_null->equivalent(tmp_tcp)) {
    set_null();
    return;
  }
  if (CORBA::_tc_void->equivalent(tmp_tcp)) {
    set_void();
    return;
  }
  if (CORBA::_tc_ushort->equivalent(tmp_tcp)) {
    try {
      CORBA::UShort us = _v_dynanyval._my_ptr->get_ushort();
      set_ushort(us);
    } catch (...) {;}
    return;
  }
  if (CORBA::_tc_short->equivalent(tmp_tcp)) {
    try {
      CORBA::Short s = _v_dynanyval._my_ptr->get_short();
      set_short(s);
    } catch (...) {;}
    return;
  }
  if (CORBA::_tc_ulong->equivalent(tmp_tcp)) {
    try {
      CORBA::ULong ul =  _v_dynanyval._my_ptr->get_ulong();
      set_ulong(ul);
    } catch (...) {;}
    return;
  }
  if (CORBA::_tc_long->equivalent(tmp_tcp)) {
    try {
      CORBA::Long l =  _v_dynanyval._my_ptr->get_long();
      set_long(l);
    } catch (...) {;}
    return;
  }
#ifndef TIMEBASE_NOLONGLONG
  if (CORBA::_tc_ulonglong->equivalent(tmp_tcp)) {
    try {
      CORBA::ULongLong ull =  _v_dynanyval._my_ptr->get_ulonglong();
      set_ulonglong(ull);
    } catch (...) {;}
    return;
  }
  if (CORBA::_tc_longlong->equivalent(tmp_tcp)) {
    try {
      CORBA::LongLong ll =  _v_dynanyval._my_ptr->get_longlong();
      set_longlong(ll);
    } catch (...) {;}
    return;
  }
#endif
  if (CORBA::_tc_float->equivalent(tmp_tcp)) {
    try {
      CORBA::Float f =  _v_dynanyval._my_ptr->get_float();
      set_float(f);
    } catch (...) {;}
    return;
  }
  if (CORBA::_tc_double->equivalent(tmp_tcp)) {
    try {
      CORBA::Double d =  _v_dynanyval._my_ptr->get_double();
      set_double(d);
    } catch (...) {;}
    return;
  }
  if (CORBA::_tc_boolean->equivalent(tmp_tcp)) {
    try {
      CORBA::Boolean b = _v_dynanyval._my_ptr->get_boolean();
      set_bool(b);
    } catch (...) {;}
    return;
  }
  if (CORBA::_tc_char->equivalent(tmp_tcp)) {
    try {
      CORBA::Char c = _v_dynanyval._my_ptr->get_char();
      set_char(c);
    } catch (...) {;}
    return;
  }
  if (CORBA::_tc_octet->equivalent(tmp_tcp)) {
    try {
      CORBA::Octet o = _v_dynanyval._my_ptr->get_octet();
      set_octet(o);
    } catch (...) {;}
    return;
  }
  if (CORBA::_tc_string->equivalent(tmp_tcp)) {
    try {
      char* s = _v_dynanyval._my_ptr->get_string();
      set_string(s, 1); // 'this' is respondible for calling COBRA_STRING_FREE on s
    } catch (...) {;}
    return;
  }
  // each enum has its own typecode; only way to check for enum
  // (accounting for possible aliasing) is to try to do a narrow
  try {
    WRAPPED_DYNANY_MODULE::DynEnum_var de = WRAPPED_DYNANY_MODULE::DynEnum::_narrow(_v_dynanyval._my_ptr);
    if (! (CORBA::is_nil(de))) {
      dynany2dynenum(de);
      return;
    }
  } catch (...) {;} // not an enum
  if (CORBA::_tc_any->equivalent(tmp_tcp)) { // unwrap it
    try {
      unwrap_any = _v_dynanyval._my_ptr->get_any();
    } catch (...) {
      RDI_Fatal("XXX failed to unwrap a contained any -- get_any failed");
    }
    RDI_RTVal unwrapped_rtv;
    unwrapped_rtv.init_from_any(unwrap_any, 1); // 1 means dup if necessary (e.g., for string)
    swap(unwrapped_rtv);
    // 'this' now has the unwrapped/simplified value
    //  unwrapped_rtv now has the original dynany value
    unwrapped_rtv.clear(); // destroys a top dynany if necessary
    return;
  }
}

// return true if ERROR, else set dbl and return false
// NB assumes simplify has already been called
CORBA::Boolean RDI_RTVal::cvt2dbl(CORBA::Double& dbl) {
  switch (_tckind) {
  case RDI_rtk_ushort:          dbl = ((CORBA::Double)_v_ushort);        return 0;
  case RDI_rtk_short:           dbl = ((CORBA::Double)_v_short);         return 0;
  case RDI_rtk_ulong:           dbl = ((CORBA::Double)_v_ulong);         return 0;
  case RDI_rtk_long:            dbl = ((CORBA::Double)_v_long);          return 0;
#ifdef _MSC_VER
  case RDI_rtk_ulonglong:       dbl = ((CORBA::Double)(CORBA::LongLong)_v_ulonglong);     return 0;
#else
  case RDI_rtk_ulonglong:       dbl = ((CORBA::Double)_v_ulonglong);     return 0;
#endif
  case RDI_rtk_longlong:        dbl = ((CORBA::Double)_v_longlong);      return 0;
  case RDI_rtk_float:           dbl = ((CORBA::Double)_v_float);         return 0;
  case RDI_rtk_double:          dbl = ((CORBA::Double)_v_double);        return 0;
  case RDI_rtk_boolean:         dbl = ((CORBA::Double)_v_boolean);       return 0;
  case RDI_rtk_char:            dbl = ((CORBA::Double)_v_char);          return 0;
  case RDI_rtk_octet:           dbl = ((CORBA::Double)_v_octet);         return 0;
  case RDI_rtk_char_or_string:  dbl = ((CORBA::Double)_v_string_ptr[0]); return 0;
  case RDI_rtk_enum_val:
    try {
      dbl = ((CORBA::Double)WRAPPED_DYNENUM_GET_AS_ULONG(_v_enumval._my_ptr));
      return 0;
    } catch (...) { }
    RDIDbgEvalLog("RDI_RTVal::cvt2dbl: DynEnum::get_as_ulong failed\n");
    return 1; // ERROR
  default: break; // fall through
  }
  return 1; // ERROR
}

////////////////////////////////////////////////////
// Logging

RDIstrstream&
operator<< (RDIstrstream& str, const RDI_RTValKind& tckind) {
  switch (tckind) {
  case RDI_rtk_null:           return str << "null";
  case RDI_rtk_void:           return str << "void";
  case RDI_rtk_ushort:         return str << "ushort";
  case RDI_rtk_short:          return str << "short";
  case RDI_rtk_ulong:          return str << "ulong";
  case RDI_rtk_long:           return str << "long";
  case RDI_rtk_ulonglong:      return str << "ulonglong";
  case RDI_rtk_longlong:       return str << "longlong";
  case RDI_rtk_float:          return str << "float";
  case RDI_rtk_double:         return str << "double";
  case RDI_rtk_boolean:        return str << "bool";
  case RDI_rtk_char:           return str << "char";
  case RDI_rtk_octet:          return str << "octet";
  case RDI_rtk_enum_ident:     return str << "enum_id";
  case RDI_rtk_enum_val:       return str << "enum_val";
  case RDI_rtk_string:         return str << "string";
  case RDI_rtk_char_or_string: return str << "char_or_string";
  case RDI_rtk_abstime:        return str << "abstime";
  case RDI_rtk_reltime:        return str << "reltime";
  case RDI_rtk_dynany:         return str << "dynany";
  }
  return str << "(XXX unexpected RDI_TCKind value)" << (unsigned long)tckind;
}

RDIstrstream& RDI_RTVal::log_output(RDIstrstream& str) const {
  str << "[tckind: " << _tckind << ", val: ";
  switch (_tckind) {
  case RDI_rtk_null: {
    break;
  }
  case RDI_rtk_void: {
    break;
  }
  case RDI_rtk_ushort: {
    str << _v_ushort;
    break;
  }
  case RDI_rtk_short: {
    str << _v_short;
    break;
  }
  case RDI_rtk_ulong: {
    str << _v_ulong;
    break;
  }
  case RDI_rtk_long: {
    str << _v_long;
    break;
  }
  case RDI_rtk_ulonglong: {
    str << _v_ulonglong;
    break;
  }
  case RDI_rtk_longlong: {
    str << _v_longlong;
    break;
  }
  case RDI_rtk_float: {
    str << _v_float;
    break;
  }
  case RDI_rtk_double: {
    str << _v_double;
    break;
  }
  case RDI_rtk_boolean: {
    str << (_v_boolean ? "TRUE" : "FALSE");
    break;
  }
  case RDI_rtk_char: {
    str << '\'' << _v_char << '\'' ;
    break;
  }
  case RDI_rtk_octet: {
    str << _v_octet;
    break;
  }
  case RDI_rtk_enum_ident: {
    str << _v_string_ptr;
    break;
  }
  case RDI_rtk_enum_val: {
    // expand the typecode?
    char* id = 0;
    CORBA::ULong ul = 0;
    try {
      id = WRAPPED_DYNENUM_GET_AS_STRING(_v_enumval._my_ptr);
      ul = WRAPPED_DYNENUM_GET_AS_ULONG (_v_enumval._my_ptr);
      str << "(as_ulong: " << ul << "   as_ident: " << (id ? id : "") << ")";
    } catch (...) {
      str << "(XXX could not extract ident or ulong from enum value)"; 
    }
    if (id) CORBA_STRING_FREE(id);
    break;
  }
  case RDI_rtk_string: {
    str << '\"' << _v_string_ptr << '\"' ;
    break;
  }
  case RDI_rtk_char_or_string: {
    str << '\'' << _v_string_ptr << '\'' ;
    break;
  }
  case RDI_rtk_abstime: {
    str << _v_abstime;
    break;
  }
  case RDI_rtk_reltime: {
    str << _v_reltime;
    break;
  }
  case RDI_rtk_dynany: {
    CORBA::TypeCode_var tmp_tcp = _v_dynanyval._my_ptr->type();
    str << "(ptr:" << (void*)_v_dynanyval._my_ptr << ", kind: " << (*tmp_tcp) << ")";
    break;
  }
  } // endsw
  return str << "]";
}
