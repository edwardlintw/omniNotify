// -*- Mode: C++; -*-
//                              File      : RDIDynamicEvalDefs.h
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
//    proprietary interface
//
 
#ifndef _RD_DYNAMICEVALDEFS_H_
#define _RD_DYNAMICEVALDEFS_H_

#include "RDIsysdep.h"
#include "thread_wrappers.h"
#include "RDI.h"
#include "RDIEvalDefs.h" // shared by static and dynamic eval
#include "CosNotifyShorthands.h"

class RDI_StructuredEvent;

// This file contains defs for dynamic eval

// Evaluation of the constraint opcodes is performed by a "simple" (hah!)
//   virtual machine.   The machine could maintain two stacks (a value stack and
//   a type stack); this might make simple binops over the top two elts on the
//   value stack faster (e.g., if both fit in the same word).
//   Right now it has one stack with each stack elt having a type and a value.

// Notes on runtime types (RDI_RTValKind):
//  * tk_dynany is never used as the result of an evaluation of a subexpr
//    if the value can be converted to one of the other value types.
//  * tk_char_or_string is only used for a string constant of length 1.
//    (If an expr such as $foo evaluates to a CORBA::string of length one, it
//     has type tk_string; if it evals to a CORBA::char it has type tk_char.
//     Only a strint constant 'X' appearing in a constraint expression
//     has the privilege of being treated as either a char or a string.)
//  

enum RDI_RTValKind {
  // what is difference between null and void?
  RDI_rtk_null,
  RDI_rtk_void,       
  // simple numeric kinds
  RDI_rtk_ushort,     
  RDI_rtk_short,      
  RDI_rtk_ulong,      
  RDI_rtk_long,       
  RDI_rtk_ulonglong,      
  RDI_rtk_longlong,       
  RDI_rtk_float,      
  RDI_rtk_double,     
#if 0
  RDI_rtk_ldouble,     // OMNIORB DOES NOT SUPPORT LONG DOUBLE YET
#endif
  // simple non-numeric kinds 
  RDI_rtk_boolean,    
  RDI_rtk_char,       
  RDI_rtk_octet,      
  RDI_rtk_enum_ident,       
  RDI_rtk_enum_val,       
  RDI_rtk_string,     
  RDI_rtk_char_or_string,     
  // compound kinds
  RDI_rtk_abstime,    
  RDI_rtk_reltime,    
  RDI_rtk_dynany     
};

class RDI_RTVal {
 public:
  RDI_RTVal(void) : _free(0), _tckind(RDI_rtk_null) {;}
  ~RDI_RTVal(void) { clear(); }

  // If dup_n_free is true, extracted contents of any (such as string)
  // is duplicated and 'this' will free the dup'd content; otherwise
  // 'this' will share content with a, and a (or another obj) must
  // be responsible for deleting the shared content.
  // ** dup_n_free false should only be used if lifetime(a) >= lifetime(this)
  void init_from_any(CORBA::Any& a, CORBA::Boolean dup_n_free = 0);

  void set_null(void)               { clear(); _tckind = RDI_rtk_null; }
  void set_void(void)               { clear(); _tckind = RDI_rtk_void; }
  void set_ushort(CORBA::UShort us) { clear(); _tckind = RDI_rtk_ushort;    _v_ushort    = us;  }
  void set_short(CORBA::Short s)    { clear(); _tckind = RDI_rtk_short;     _v_short     = s;   }
  void set_ulong(CORBA::ULong ul)   { clear(); _tckind = RDI_rtk_ulong;     _v_ulong     = ul;  }
  void set_long(CORBA::Long l)      { clear(); _tckind = RDI_rtk_long;      _v_long      = l;   }
  void set_ulonglong(CORBA::ULongLong ull)
                                    { clear(); _tckind = RDI_rtk_ulonglong; _v_ulonglong = ull; }
  void set_longlong(CORBA::LongLong ll)
                                    { clear(); _tckind = RDI_rtk_longlong;  _v_longlong  = ll;  }
  void set_float(CORBA::Float f)    { clear(); _tckind = RDI_rtk_float;     _v_float     = f;   }
  void set_double(CORBA::Double d)  { clear(); _tckind = RDI_rtk_double;    _v_double    = d;   }
  void set_bool(CORBA::Boolean b)   { clear(); _tckind = RDI_rtk_boolean;   _v_boolean   = b;   }
  void set_char(CORBA::Char c)      { clear(); _tckind = RDI_rtk_char;      _v_char      = c;   }
  void set_octet(CORBA::Octet o)    { clear(); _tckind = RDI_rtk_octet;     _v_octet     = o;   }
  void set_abstime(RDI_AbsTime& a)  { clear(); _tckind = RDI_rtk_abstime;   _v_abstime   = a;   }
  void set_reltime(RDI_RelTime& r)  { clear(); _tckind = RDI_rtk_reltime;   _v_reltime   = r;   }

  void set_enum_ident(char* e, CORBA::Boolean f = 0)
    { clear(); _free = f; _tckind = RDI_rtk_enum_ident;     _v_string_ptr   = e;  }
  void set_string(char* s, CORBA::Boolean f = 0)
    { clear(); _free = f; _tckind = RDI_rtk_string;         _v_string_ptr = s;  }
  void set_char_or_string(char* s, CORBA::Boolean f = 0)
    { clear(); _free = f; _tckind = RDI_rtk_char_or_string; _v_string_ptr = s;  }

  // for dynany, we duplicate the references passed in and release on clear, so
  // the caller is responsible for otherwise balancing the refcounts
  // on a clear, if free is true and top_ptr is non-NULL, we destroy top_ptr
  // if ptr and top_ptr are the same you should use
  //        set_dynany(x, x, 1)  to destroy x 
  void set_dynany(WRAPPED_DYNANY_MODULE::DynAny_ptr mptr, WRAPPED_DYNANY_MODULE::DynAny_ptr tptr, CORBA::Boolean f = 0) {
    // if same object, this bumps refcount by 2, but clear de-bumps by 2 
    WRAPPED_DYNANY_MODULE::DynAny_ptr mptr_dup = WRAPPED_DYNANY_MODULE::DynAny::_duplicate(mptr);
    WRAPPED_DYNANY_MODULE::DynAny_ptr tptr_dup = WRAPPED_DYNANY_MODULE::DynAny::_duplicate(tptr);
    clear(); _free = f; _tckind = RDI_rtk_dynany;
    _v_dynanyval._my_ptr = mptr_dup;
    _v_dynanyval._top_ptr = tptr_dup;
  }

  // See comment for set_dynany
  void set_enum_val(WRAPPED_DYNANY_MODULE::DynEnum_ptr mptr, WRAPPED_DYNANY_MODULE::DynAny_ptr tptr, CORBA::Boolean f = 0) {
    // if same object, this bumps refcount by 2, but clear de-bumps by 2 
    WRAPPED_DYNANY_MODULE::DynEnum_ptr mptr_dup = WRAPPED_DYNANY_MODULE::DynEnum::_duplicate(mptr);
    WRAPPED_DYNANY_MODULE::DynAny_ptr  tptr_dup = WRAPPED_DYNANY_MODULE::DynAny::_duplicate(tptr);
    clear(); _free = f; _tckind = RDI_rtk_enum_val;
    _v_enumval._my_ptr  = mptr_dup;
    _v_enumval._top_ptr = tptr_dup;
  }

  // simplify:  if kind is dynany, attempt to convert to a simpler kind
  void simplify(void);
  // return true if ERROR, else set dbl and return false
  CORBA::Boolean cvt2dbl(CORBA::Double& dbl); // NB assumes simplify has already been called

  void clear(void) {
    if ((_tckind == RDI_rtk_string) || (_tckind == RDI_rtk_char_or_string) ||
	(_tckind == RDI_rtk_enum_ident)) {
      if (_free) { CORBA_STRING_FREE(_v_string_ptr); }
      _v_string_ptr = 0;
    } else if (_tckind == RDI_rtk_enum_val) {
      if (_free && !(CORBA::is_nil(_v_enumval._top_ptr))) {
	_v_enumval._top_ptr->destroy();
      }
      CORBA::release(_v_enumval._my_ptr);
      CORBA::release(_v_enumval._top_ptr);
      _v_enumval._my_ptr = 0;
      _v_enumval._top_ptr = 0;
    } else if (_tckind == RDI_rtk_dynany) {
      if (_free && !(CORBA::is_nil(_v_dynanyval._top_ptr))) {
	_v_dynanyval._top_ptr->destroy();
      }
      CORBA::release(_v_dynanyval._my_ptr);
      CORBA::release(_v_dynanyval._top_ptr);
      _v_dynanyval._my_ptr = 0;
      _v_dynanyval._top_ptr = 0;
    }
    // leave in null state
    _tckind = RDI_rtk_null;
    _free = 0;
  }

  void swap(RDI_RTVal& v) {
    // using _v_abstime copies largest possible value
    CORBA::Boolean tmp_free    = _free;
    RDI_RTValKind  tmp_kind    = _tckind;
    RDI_AbsTime    tmp_abstime = _v_abstime;

    _free        = v._free;
    _tckind      = v._tckind;
    _v_abstime   = v._v_abstime;

    v._free      = tmp_free;
    v._tckind    = tmp_kind;
    v._v_abstime = tmp_abstime;
  };

  // convert a dynany to a (contained) dynenum
  void dynany2dynenum(WRAPPED_DYNANY_MODULE::DynEnum_ptr mptr) {
    WRAPPED_DYNANY_MODULE::DynEnum_ptr mptr_dup = WRAPPED_DYNANY_MODULE::DynEnum::_duplicate(mptr);
    // _free does not change
    // _v_dynanyval._top_ptr + _v_enumval._top_ptr are at same location => no change
    // _v_dynanyval._my_ptr  + _v_dynenum._my_ptr are at same location => 
    //   must release the former before setting the latter
    CORBA::release(_v_dynanyval._my_ptr);
    _tckind = RDI_rtk_enum_val;
    _v_enumval._my_ptr = mptr_dup;
  };
  
  // NB For operator=, 'this' is not responsible for storage of data referenced by v
  // e.g., if v is a string, 'this' will not free it on delete.
  // ** operator= should only be used if lifetime(v) >= lifetime(this)

  inline RDI_RTVal& operator= (const RDI_RTVal& v) {
    clear();
    _free        = 0;
    _tckind      = v._tckind;
    switch (_tckind) {
    case RDI_rtk_enum_val:
      // these refs are always released regardless of free so they must be bumped here
      _v_enumval._my_ptr  = WRAPPED_DYNANY_MODULE::DynEnum::_duplicate(v._v_enumval._my_ptr);
      _v_enumval._top_ptr = WRAPPED_DYNANY_MODULE::DynAny::_duplicate(v._v_enumval._top_ptr);
      break;
    case RDI_rtk_dynany:
      // these refs are always released regardless of free so they must be bumped here
      _v_dynanyval._my_ptr  = WRAPPED_DYNANY_MODULE::DynAny::_duplicate(v._v_dynanyval._my_ptr);
      _v_dynanyval._top_ptr = WRAPPED_DYNANY_MODULE::DynAny::_duplicate(v._v_dynanyval._top_ptr);
      break;
    default:
      // using _v_abstime copies largest possible value
      _v_abstime   = v._v_abstime;
      break;
    }
    return *this;
  };

  // caller must release the result (can use _var reference to do it for you)
  CORBA::TypeCode_ptr type(void) {
    CORBA::TypeCode_var tcp;
    switch (_tckind) {
    case RDI_rtk_null:
      return CORBA::TypeCode::_duplicate(CORBA::_tc_null);
    case RDI_rtk_void: 
      return CORBA::TypeCode::_duplicate(CORBA::_tc_void);
    case RDI_rtk_ushort: 
      return CORBA::TypeCode::_duplicate(CORBA::_tc_ushort);
    case RDI_rtk_short:
      return CORBA::TypeCode::_duplicate(CORBA::_tc_short);
    case RDI_rtk_ulong:
      return CORBA::TypeCode::_duplicate(CORBA::_tc_ulong);
    case RDI_rtk_long: 
      return CORBA::TypeCode::_duplicate(CORBA::_tc_long);
#ifdef TIMEBASE_NOLONGLONG
    case RDI_rtk_ulonglong:
      // XXX SOMETHING BETTER ???
      return CORBA::TypeCode::_duplicate(CORBA::_tc_null);
    case RDI_rtk_longlong: 
      // XXX SOMETHING BETTER ???
      return CORBA::TypeCode::_duplicate(CORBA::_tc_null);
#else
    case RDI_rtk_ulonglong:
      return CORBA::TypeCode::_duplicate(CORBA::_tc_ulonglong);
    case RDI_rtk_longlong: 
      return CORBA::TypeCode::_duplicate(CORBA::_tc_longlong);
#endif
    case RDI_rtk_float:
      return CORBA::TypeCode::_duplicate(CORBA::_tc_float);
    case RDI_rtk_double: 
      return CORBA::TypeCode::_duplicate(CORBA::_tc_double);
    case RDI_rtk_boolean:
      return CORBA::TypeCode::_duplicate(CORBA::_tc_boolean);
    case RDI_rtk_char: 
      return CORBA::TypeCode::_duplicate(CORBA::_tc_char);
    case RDI_rtk_octet:
      return CORBA::TypeCode::_duplicate(CORBA::_tc_octet);
    case RDI_rtk_enum_ident: 
      RDI_Fatal("not get here");
    case RDI_rtk_enum_val:
      return ( _v_enumval._my_ptr->type() );
    case RDI_rtk_string: 
      return CORBA::TypeCode::_duplicate(CORBA::_tc_string);
    case RDI_rtk_char_or_string: 
      RDI_Fatal("not get here");
    case RDI_rtk_abstime:
      return CORBA::TypeCode::_duplicate(TimeBase::_tc_UtcT);
    case RDI_rtk_reltime:
      return CORBA::TypeCode::_duplicate(TimeBase::_tc_TimeT);
    case RDI_rtk_dynany:
      return ( _v_dynanyval._my_ptr->type() );
    }
    RDI_Fatal("not get here");
    return 0;
  }

  // type_id and repos_id should only be called on rtval from component expr
  // the string has been duplicated so must be freed
  char* type_id(void) {
    CORBA::TypeCode_var tcp = type();
    const char* nm = tcp->name();
    if ( ! nm ) nm = "";
    return CORBA_STRING_DUP(nm);
  }

  char* repos_id(void) {
    CORBA::TypeCode_var tcp = type();
    const char* id = tcp->id();
    if ( ! id ) id = "";
    return CORBA_STRING_DUP(id);
  }

  // public state
  CORBA::Boolean            _free;
  RDI_RTValKind             _tckind;

  // HP aCC does not like struct type decl inside an anonymous union
  // so I put two struct typedefs here:

  typedef struct {
    WRAPPED_DYNANY_MODULE::DynAny_ptr    _top_ptr;
    WRAPPED_DYNANY_MODULE::DynAny_ptr    _my_ptr;
  } _StructForVDynAnyVal;

  typedef struct {
    WRAPPED_DYNANY_MODULE::DynAny_ptr    _top_ptr;
    WRAPPED_DYNANY_MODULE::DynEnum_ptr   _my_ptr;
  } _StructForVEnumVal;

  union {
    CORBA::UShort        _v_ushort;
    CORBA::Short         _v_short;
    CORBA::ULong         _v_ulong;
    CORBA::Long          _v_long;
    CORBA::ULongLong     _v_ulonglong;
    CORBA::LongLong      _v_longlong;
    CORBA::Float         _v_float;
    CORBA::Double        _v_double;
    CORBA::Boolean       _v_boolean;
    CORBA::Char          _v_char;
    CORBA::Octet         _v_octet;
    RDI_AbsTime          _v_abstime;
    RDI_RelTime          _v_reltime;
    char*                _v_string_ptr;
    _StructForVDynAnyVal _v_dynanyval;
    _StructForVEnumVal   _v_enumval;
  };

  RDIstrstream& log_output(RDIstrstream& str) const;
};

///////////////////////////////////////////
// RVM: READY Virtual Machine
// (omniNotify began life as 'READY')

class RDI_RVM { 
public:
  RDI_RTRetCode  r_code; // result code
  CORBA::Boolean r_bool; // bool result (valid if r_code==RDI_RTRet_OK)
  char*          cexprs; // constraint expression; may be set by filter
  unsigned long  rvmID;	 // serial number of this READY Virtual Machine
public:
  RDI_RVM();
  ~RDI_RVM() 	{ _clear_stack(); _null_ops(); }

  void init(RDI_OpSeq* ops, CORBA::Boolean free_ops=0) 
	{ if (_ops != ops) { _null_ops(); _ops = ops; }
    	  _free_ops = free_ops; reset();
  	}
  void reset() 	{ _clear_stack(); _init_eval_state(); }
  void eval(RDI_StructuredEvent* evp);
private:
  int            _PC;	// program counter; -1 used as invalid value
  int            _top;	// top of stack, -1 if empty
  CORBA::Boolean _free_ops; // is RTState in charge of _ops memory?
  RDI_OpSeq*     _ops;
  RDI_Op*        _op;
  RDI_RTVal      _stack[RDI_Eval_STACK_SIZE];

  CORBA::Boolean _eval_arithop(RDI_ArithOp arithop);

  void _null_ops() 
	{ if (_ops && _free_ops) delete _ops; 
	  _ops = 0; _free_ops = 0; 
	}
  void _clear_stack(void) 
	{ for (int idx = 0; idx <= _top; idx++)
      		_stack[idx].clear(); 	// frees any allocated state
    	  _top = -1;
  	}
  void _init_eval_state() 	// Sets _PC to zero if legal opseq, else -1
	{ r_code = RDI_RTRet_UNDEFINED;
    	  r_bool = 0;
	  _op    = (_ops && _ops->_op) ? _ops->_op : 0;
	  _PC    = (_ops && _ops->_opstail >= 0) ? 0 : -1;
  	}

  // eval helpers
  CORBA::Boolean _eval_nop(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_return_b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_signal_N(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_push_cC2c(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_push_sC2s(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_push_iC2i(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_push_bC2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_push_NC2N(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_push_nC2n_s(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_push_nC2n_ul(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_push_nC2n_l(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_push_nC2n_ull(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_push_nC2n_ll(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_push_nC2n_f(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_push_nC2n_d(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_ctelt_NC2n(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_swap_uu2uu(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_pop_u(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_pop_uu(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_cvt_u2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_cvt_u2s(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_cvt_u2n(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_or_bb2b (RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_and_bb2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_not_b2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_in_uu2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_add_nn2n(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_sub_nn2n(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_mul_nn2n(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_div_nn2n(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_rem_nn2n(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_substr_ss2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_cmp_uu2n(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_eqz_n2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_nez_n2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_lez_n2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_ltz_n2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_gez_n2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_gtz_n2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_ifT_b2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_ifF_b2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_goto(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_label(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_wkp_NC2u(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_special_sC2u(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_compend(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_default_X2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_exist_X2b(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_dot_len_u2n_l(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_dot_d_u2u(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_dot_tid_u2s(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_dot_rid_u2s(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_dot_id_usC2u(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_dot_num_ulC2u(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_assoc_usC2u(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_index_ulC2u(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_tagdef_u2u(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_tagid_usC2u(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_tagnum_ulC2u(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_tagchar_usC2u(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_tagbool_ubC2u(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_debug1(RDI_StructuredEvent* evp);
  CORBA::Boolean _eval_debug2(RDI_StructuredEvent* evp);

  // helpers
  void _widenAndResultKinds(RDI_RTValKind lhskind, RDI_RTValKind rhskind, 
			    RDI_RTValKind& widenkind, RDI_RTValKind& resultkind);
  void _force2num(RDI_RTVal& v);

  // Mutex and Serial for atomic Assignment of RVM serial numbers
  static _nfy_attr TW_Mutex      _cls_lock;
  static _nfy_attr unsigned long _rvmIDctr;
};

////////////////////////////////////////////////////
// useful TABLES

extern const char* RDI_RTValKind2string[]; // map RDI_RTValKind to string

////////////////////////////////////////////////////
// Logging
RDIstrstream& operator<< (RDIstrstream& str, const RDI_RTValKind& tckind);
inline RDIstrstream& operator<< (RDIstrstream& str, const RDI_RTVal& res) { return res.log_output(str); }

#endif
