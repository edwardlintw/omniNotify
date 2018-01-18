// -*- Mode: C++; -*-
//                              File      : RDIEvalDefs.h
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
 
#ifndef _RD_EVALDEFS_H_
#define _RD_EVALDEFS_H_

#include "CosNotifyShorthands.h"
#include "RDIstrstream.h"
#include "RDIStringDefs.h"

typedef TimeBase::UtcT   RDI_AbsTime;
typedef TimeBase::TimeT  RDI_RelTime;

#define RDI_Eval_OPSEQ_SIZE       2048
#define RDI_Eval_STACK_SIZE         10
#define RDI_Eval_LBLBUF_LEN        128

// more special cases?
enum RDI_WellKnownPart {
  RDI_WKP_dollar,       // $
  RDI_WKP_hdr,          // $header                 or $.header
  RDI_WKP_fdata,        // $filterable_data        or $.filterable_data
  RDI_WKP_rob,          // $remainder_of_body      or $.remainder_of_body
  RDI_WKP_fhdr,         // $fixed_header           or $.header.fixed_header
  RDI_WKP_vhdr,         // $variable_header        or $.header.variable_header
  RDI_WKP_ename,        // $event_name             or $.header.fixed_header.event_name
  RDI_WKP_etype,        // $event_type             or $.header.fixed_header.event_type
  RDI_WKP_tname,        // $type_name              or $.header.fixed_header.event_type.type_name
  RDI_WKP_dname         // $domain_name            or $.header.fixed_header.event_type.domain_name
};

enum RDI_CurTimeElt {
  RDI_CurTime_error,     //  some erroneous $curtime expr (e.g., $curtime[27])
  RDI_CurTime_itself,    //  $curtime            // not legal by itself in an expr, except for exist $curtime
  RDI_CurTime_time,      //  $curtime.time       // not legal by itself in an expr, except for exist $curtime.time
  RDI_CurTime_time_low,  //  $curtime.time.low   (type CORBA::ULong)
  RDI_CurTime_time_high, //  $curtime.time.high  (type CORBA::ULong)
  RDI_CurTime_inacclo,   //  $curtime.inacclo    (type CORBA::ULong)
  RDI_CurTime_inacchi,   //  $curtime.inacchi    (type CORBA::UShort) 
  RDI_CurTime_tdf        //  $curtime.tdf        (type CORBA::Short)
};

enum RDI_CompareOp {
  RDI_CompareOp_eq,
  RDI_CompareOp_ne,
  RDI_CompareOp_le,
  RDI_CompareOp_lt,
  RDI_CompareOp_ge,
  RDI_CompareOp_gt
};

enum RDI_ArithOp {
  RDI_ArithOp_add,
  RDI_ArithOp_sub,
  RDI_ArithOp_mul,
  RDI_ArithOp_div,
  RDI_ArithOp_rem,
  RDI_ArithOp_cmp    // result is -1, 0, or 1
};

// Possible return codes
enum RDI_RTRetCode {
  RDI_RTRet_OK = 0, // no error; normal return result can be used
  RDI_RTRet_UNDEFINED,
  RDI_RTRet_DIVIDE_BY_ZERO,
  RDI_RTRet_OVERFLOW,
  RDI_RTRet_OUT_OF_MEMORY,
  RDI_RTRet_TYPE_MISMATCH, // wrong or unexpected type is on the stack
  RDI_RTRet_NONE_SUCH,     // for a component expr; component does not exist
  RDI_RTRet_NOT_SUPPORTED
};

// Static typing
//   Similar to dynamic typing, except it is hard to type certain subexpressions
//   statically.  For example, $foo + 10 could result in a long or a double,
//   and for the component $foo in  $foo == 'a' it would be legal for $foo
//   to evaluate to a number or a string (since 'a' can be treated as a character,
//   and a char can be compared to a number).

//   Component has static type unknown
//   literals have static type char, string, ident, numeric, bool
//    ... char can be used where either a number or a string is expected
//   :UtcT:(expr), $curtime, $StartTime, $StopTime have static type abstime
//   :TimeT:(expr), $Timeout have static type reltime
//   Various tests have static type bool (comparisons, default, exist)
//   Arith exprs have static type number
//   <comp> . _type_id has static type string
//   <comp> . _repos_id has static type string
//   <comp> . _length has static type numeric (error if <comp> is not an array or sequence)
//   <comp> . _d has static type unknown
//        (could be numeric, char, bool, or enum)

enum RDI_StaticT {
  RDI_StaticT_b,      // <b>ool
  RDI_StaticT_c,      // <c>har (actually char_or_string)
  RDI_StaticT_i,      // <i>dent
  RDI_StaticT_n,      // <n>umeric value
  RDI_StaticT_N,      // <N>umeric value of type unsigned short
  RDI_StaticT_s,      // <s>string
  RDI_StaticT_u       // <u>nknown, e.g., component
};

// In the opcode names below, the use of 'n' as an argument indicator
// means a number must be present at runtime.

// The use of 'u' means any runtime type might be valid, depending
// on, e.g., the runtime type of another argument.  E.g., the uu_cmp
// comparison op is valid if
//     1. both are valid numbers
//     2. both are strings
//     3. both are relative times // XXX not implemented
//     4. both are absolute times // XXX not implemented
//     5. the op is == and one side is a dynamic enum, the other
//           a dynic enum or an enum ident literal
// (cannot compare 2 enum ident literals; we use a runtime error NOT_SUPPORTED)
// (an enum ident literal can be compared to a component that evals to an enum
//    or used with the in operator against a sequence of enum)

// The use of more specific indicators (N, b) means exactly the
// type specified must be present at runtime.

// char forced conversion:  where an s is expected,
// a single-char string constant is given static type string.
// Where a number is expected, a single-char string constant
// is converted to an unsigned short

// in notation below a single type letter indicates a
// value on the stack of the specified type, while a
// a letter plus a C indicates a constant of the specified
// type [which will be supplied with the op itself].
// The resulting stack elt is given after a '2'.
//
// When numeric values are pushed, the final n
// can be qualified with a more specific type, e.g.,
// push_nC2n_ul specifies that an unsigned long
// numeric constant is pushed onto the stack.

// Examples:
//    pop two bools, push bool result:
//        opname_bb2b   (b x b -> b)
//    pop two numbers, put a number on stack
//        opname_nn2n   (n x n -> n)
//    push a bool const (no stack elt required)
//        opname_bC2b
//    1 or 2 vals popped off stack, none pushed:
//        opname_u
//        opname_uu
//    no stack elt used, none pushed
//        opname (no suffix)

// On labels and goto ops:
//
// Initially a target is a label [<label_name>, 0]... there are no one-instruction infinite loops!
// Once the final PC offset is known, the label is changed to [<label_name>, relative_offset].
// We leave the label ops in the op sequence so that we can easily do code modification.

enum RDI_OpCode {
  RDI_OpCode_nop,  // NO-OP

  // Normal and Exceptional RETURN
  RDI_OpCode_return_b,      // <top> must be a bool; it is returned as final value
  RDI_OpCode_signal_N,      // <top> must be N (ushort) = an RDI_RTRetCode

  // PUSH a CONSTANT
  RDI_OpCode_push_cC2c,     // push char const                  cC -> c   ** really c_or_s
  RDI_OpCode_push_sC2s,     // push string const                sC -> s
  RDI_OpCode_push_iC2i,     // push enum ident const            iC -> i
  RDI_OpCode_push_bC2b,     // push bool const                  bC -> b
  RDI_OpCode_push_NC2N,     // push numeric (ushort) const      NC -> N
  RDI_OpCode_push_nC2n_s,   // push numeric (short) const       nC -> n
  RDI_OpCode_push_nC2n_ul,  // push numeric (ulong) const       nC -> n
  RDI_OpCode_push_nC2n_l,   // push numeric (long) const        nC -> n
  RDI_OpCode_push_nC2n_ull, // push numeric (ulonglong) const   nC -> n
  RDI_OpCode_push_nC2n_ll,  // push numeric (longlong) const    nC -> n
  RDI_OpCode_push_nC2n_f,   // push numeric (float) const       nC -> n
  RDI_OpCode_push_nC2n_d,   // push numeric (double) const      nC -> n

  // special PUSH
  RDI_OpCode_ctelt_NC2n,    // push a $curtime element          NC -> n   NC is really of type RDI_CurTimeElt

  // SIMPLE STACK OPS
  RDI_OpCode_swap_uu2uu,    // swap top 2 elts    ..,v0,v1,v2 -> ..,v0,v2,v1
  RDI_OpCode_pop_u,         // pop 1 elt          ..,v0,v1,v2 -> ..,v0,v1
  RDI_OpCode_pop_uu,        // pop 2 elt          ..,v0,v1,v2 -> ..,v0

  // CONVERSION OPS
  RDI_OpCode_cvt_u2b,       // covert unknown to bool    u -> b    signals TYPE_MISMATCH
  RDI_OpCode_cvt_u2s,       // covert unknown to string  u -> s    signals TYPE_MISMATCH
  RDI_OpCode_cvt_u2n,       // covert unknown to numeric u -> n    signals TYPE_MISMATCH

  // BOOLEAN OPS
  RDI_OpCode_or_bb2b,       // boolean or            b x b -> b
  RDI_OpCode_and_bb2b,      // boolean and           b x b -> b
  RDI_OpCode_not_b2b,       // boolean not           b -> b

  RDI_OpCode_in_uu2b,       // is uknown in seq      u x u -> b   signals TYPE_MISMATCH

  // ARITH OPS
  RDI_OpCode_add_nn2n,      // add 2 nums        n x n -> n   signals TYPE_MISMATCH
  RDI_OpCode_sub_nn2n,      // sub 2 nums        n x n -> n   signals TYPE_MISMATCH
  RDI_OpCode_mul_nn2n,      // mul 2 nums        n x n -> n   signals TYPE_MISMATCH
  RDI_OpCode_div_nn2n,      // div 2 nums        n x n -> n   signals TYPE_MISMATCH, DIVIDE_BY_ZERO
  RDI_OpCode_rem_nn2n,      // rem 2 nums        n x n -> n   signals TYPE_MISMATCH, DIVIDE_BY_ZERO

  // STRING OPS
  // RDI_OpCode_ss_concat,  // concat 2 strings        s x s -> s
  RDI_OpCode_substr_ss2b,   // lhs contained in rhs?   s x s -> b

  // COMPARISON, CONDITIONAL GOTO
  RDI_OpCode_cmp_uu2n,      // compare 2 unknowns,  u x u -> n  signals TYPE_MISMATCH 
  //  result is numeric short:   -1   if   <top-1>  <  <top>
  //                              0   if   <top-1>  == <top>
  //                             +1   if   <top-1>  >  <top>

  RDI_OpCode_eqz_n2b,       // n -> b    n == 0 ?
  RDI_OpCode_nez_n2b,       // n -> b    n != 0 ?
  RDI_OpCode_lez_n2b,       // n -> b    n <= 0 ?
  RDI_OpCode_ltz_n2b,       // n -> b    n <  0 ?
  RDI_OpCode_gez_n2b,       // n -> b    n >= 0 ?
  RDI_OpCode_gtz_n2b,       // n -> b    n >  0 ?

  // all conditional goto ops leave <top> as-is; you must generate pop_u
  // (in each branch where you want to discard it)
  RDI_OpCode_ifT_b2b,       // conditional goto      if (top == true)  goto location
  RDI_OpCode_ifF_b2b,       // conditional goto      if (top == false) goto location

  // GOTO and DISPATCH
  RDI_OpCode_goto,          // unconditional goto location
  RDI_OpCode_label,         // a label is a 'no-op' placeholder
  // RDI_OpCode_tdisp,      // dispatch on top type, provide K labels

  // COMPONENT 'start' ops
  RDI_OpCode_wkp_NC2u,      // <well know part>  nC -> u    $,  $event_name, etc. (NC is RDI_WellKnownPart)
  RDI_OpCode_special_sC2u,  // $foo              sC -> u    signals NONE_SUCH [foo is not well-known]

  // COMPONENT 'end' ops
  RDI_OpCode_compend,       // marks end of component that is not otherwise marked by an 'end' op
  RDI_OpCode_default_X2b,   // default <top>   X -> b   signals TYPE_MISMATCH
  RDI_OpCode_exist_X2b,     // exist <top>     X -> b   signals TYPE_MISMATCH

  // These *could* be 'end' ops but are currently 'part' ops
  RDI_OpCode_dot_len_u2n_l, // <top>._length   u -> n   signals TYPE_MISMATCH
  RDI_OpCode_dot_d_u2u,     // <top>._d        u -> u   signals TYPE_MISMATCH
  RDI_OpCode_dot_tid_u2s,   // <top>._type_id  u -> s   signals NONE_SUCH
  RDI_OpCode_dot_rid_u2s,   // <top>._repos_id u -> s   signals NONE_SUCH

  // COMPONENT 'part' ops
  RDI_OpCode_dot_id_usC2u,  // "<top>.foo"      u x sC -> u  signals TYPE_MISMATCH, NONE_SUCH
  RDI_OpCode_dot_num_ulC2u, // "<top>.#  "      u x lC -> u  signals TYPE_MISMATCH, NONE_SUCH
  RDI_OpCode_assoc_usC2u,   // "<top>(foo)"     u x sC -> u  signals TYPE_MISMATCH, NONE_SUCH
  RDI_OpCode_index_ulC2u,   // "<top>[#]"       u x lC -> u  signals TYPE_MISMATCH, NONE_SUCH
  RDI_OpCode_tagdef_u2u,    // "<top>.()"       u      -> u  signals TYPE_MISMATCH, NONE_SUCH
  RDI_OpCode_tagid_usC2u,   // "<top>.(foo)"    u x sC -> u  signals TYPE_MISMATCH, NONE_SUCH
  RDI_OpCode_tagnum_ulC2u,  // "<top>.([+|-]#)" u x lC -> u  signals TYPE_MISMATCH, NONE_SUCH
  RDI_OpCode_tagchar_usC2u, // "<top>.('X')"    u x sC -> u  signals TYPE_MISMATCH, NONE_SUCH
  RDI_OpCode_tagbool_ubC2u, // "<top>.(true)"   u x bC -> u  signals TYPE_MISMATCH, NONE_SUCH

  // a couple of extra ops for debugging
  RDI_OpCode_debug1,        // used for debugging
  RDI_OpCode_debug2         // used for debugging
};

///////////// Some notes:
// 
// Upon completion of return or signal the stack is discarded
//
// NB for expr 'default <comp>._d' generate opcodes to put <comp> on top of stack
//  then use opcode 'RDI_OpCode_default' to test whether the top elt
//  (which must be a discriminated union) has discr value = the default
//  discriminator value (if the union type has such a default)
//  signals TYPE_MISMATCH if <comp> is not a union val; returns false if
//  there is no default or if the discr value is not the default

/////////////

struct RDI_OpLabel {
  int _offset; // offset in opseq of op w/ this label (-1 = not assigned yet)
  char* _lbl;  // label name
};

// what argument is provided along with a give opcode
enum RDI_OpArgT {
  RDI_OpArgT_none,
  RDI_OpArgT_sc,     // string constant
  RDI_OpArgT_bc,     // bool constant
  RDI_OpArgT_nc_us,  // numeric constant (unsigned short)
  RDI_OpArgT_nc_s,   // numeric constant (short)
  RDI_OpArgT_nc_ul,  // numeric constant (unsigned long)
  RDI_OpArgT_nc_l,   // numeric constant (long)
  RDI_OpArgT_nc_ull, // numeric constant (unsigned long long)
  RDI_OpArgT_nc_ll,  // numeric constant (long long)
  RDI_OpArgT_nc_f,   // numeric constant (float)
  RDI_OpArgT_nc_d,   // numeric constant (double)
  RDI_OpArgT_lbl     // label
};

// the type of an argument
typedef union {
    void*             _v_none;
    char*             _v_sc;
    CORBA::Boolean    _v_bc;
    CORBA::UShort     _v_nc_us;
    CORBA::Short      _v_nc_s;
    CORBA::ULong      _v_nc_ul;
    CORBA::Long       _v_nc_l;
    CORBA::ULongLong  _v_nc_ull;
    CORBA::LongLong   _v_nc_ll;
    CORBA::Float      _v_nc_f;
    CORBA::Double     _v_nc_d;
    RDI_OpLabel       _v_lbl;
  } RDI_OpArg;

enum RDI_StackPre {
  RDI_StackPre_none,      // no requirement

  RDI_StackPre_X,    // >= 1 val,  no type requirement
  RDI_StackPre_XX,   // >= 2 vals, no type requirements
  RDI_StackPre_Xu,   // >= 2 vals, no lhs requirement, rhs must be StaticT_u

  RDI_StackPre_b,    // >= 1 val,  type  must be StaticT_b
  RDI_StackPre_n,    // >= 1 val,  type  must be numeric
  RDI_StackPre_N,    // >= 1 val,  type  must be ushort
  RDI_StackPre_u,    // >= 1 val,  type  must be StaticT_u

  RDI_StackPre_bb,   // >= 2 vals, types must be StaticT_b
  RDI_StackPre_ss,   // >= 2 vals, types must be StaticT_s
  RDI_StackPre_nn    // >= 2 vals, types must be valid numeric types
};

// effect of an op on type of top elt(s) on the stack
enum RDI_StackEffect {
  RDI_StackEffect_none,        // no change

  RDI_StackEffect_X,           // top 2 elts removed, nothing pushed
  RDI_StackEffect_XX,          // top 2 elts removed, nothing pushed 
  RDI_StackEffect_swap,        // top 2 elts/types swapped

  RDI_StackEffect_2b,          // nothing removed, b pushed (bool)
  RDI_StackEffect_2c,          // nothing removed, c pushed (char or string)
  RDI_StackEffect_2i,          // nothing removed, i pushed (identifier)
  RDI_StackEffect_2n,          // nothing removed, n pushed (numeric)
  RDI_StackEffect_2N,          // nothing removed, N pushed (Numeric ushort)
  RDI_StackEffect_2s,          // nothing removed, s pushed (string)
  RDI_StackEffect_2u,          // nothing removed, u pushed (unknown)

  RDI_StackEffect_X2b,         // top elt removed, b pushed (bool)
  RDI_StackEffect_X2n,         // top elt removed, n pushed (numeric)
  RDI_StackEffect_X2s,         // top elt removed, s pushed (string)
  RDI_StackEffect_X2u,         // top elt removed, u pushed (unknown)

  RDI_StackEffect_XX2b,        // top 2 elts removed, b pushed (bool)
  RDI_StackEffect_XX2n         // top 2 elts removed, n pushed (numeric)
};

////////////////////////////////////////////////////
// useful TABLES

extern const char* RDI_WKP2string[];               // WellKnownPart -> string name 
extern const char* RDI_RTRetCode2string[];         // RTRetCode -> string name
extern const char* RDI_OpArgT2string[];            // OpArgT -> string name
extern const char* RDI_OpCode2string[];            // OpCode -> string name
extern const RDI_StackPre RDI_Op2StackPre[];       // OpCode -> stack prereq
extern const RDI_StackEffect RDI_Op2StackEffect[]; // OpCode -> change in top of stack
extern const RDI_OpArgT RDI_Op2ArgT[];             // OpCode -> required arg type

////////////////////////////////////////////////////
// RDI_Op

// NB: RDI_Op does not manage the storage of the strings in
//     the string constant or label args.
//  (whereas RDI_OpSeq performs this management for the ops in a sequence)
class RDI_Op {
 public:
  RDI_Op(RDI_OpCode c = RDI_OpCode_nop);
  RDI_Op(RDI_OpCode c, char* s);
  RDI_Op(RDI_OpCode c, CORBA::Boolean b);
  RDI_Op(RDI_OpCode c, CORBA::UShort us);
  RDI_Op(RDI_OpCode c, CORBA::Short s);
  RDI_Op(RDI_OpCode c, CORBA::ULong ul);
  RDI_Op(RDI_OpCode c, CORBA::Long l);
  RDI_Op(RDI_OpCode c, CORBA::ULongLong ull);
  RDI_Op(RDI_OpCode c, CORBA::LongLong ll);
  RDI_Op(RDI_OpCode c, CORBA::Float f);
  RDI_Op(RDI_OpCode c, CORBA::Double d);
  RDI_Op(RDI_OpCode c, char* s, int offset);
  ~RDI_Op(void);

  void clear(CORBA::Boolean freestrings = 0);

  void dbg_output(RDIstrstream& str, CORBA::Boolean signal_const = 0);

  RDI_OpCode      _code;
  RDI_OpArgT      _argT;
  RDI_OpArg       _arg;

  // simple label generator  NB buf assumed to have len RDI_Eval_LBLBUF_LEN
  static void newlblnm(const char* s, char buf[]);

  RDIstrstream& log_output(RDIstrstream& str) const;

private:
  static int   _newlblctr;
};

class RDI_OpSeq {
 public:
  RDI_OpSeq() : _opstail(-1), _top(-1), _maxtop(-1) {;}
  ~RDI_OpSeq(void) { clearops(); }
  void clearops(void) {
    for (int idx = 0; idx <= _opstail; idx++) {
      _op[idx].clear(1); // 1 means free any strings in the op
    }
    _opstail = -1;
    _top = -1;
    _maxtop = -1;
  }

  // unchecked!
  //   RDI_Op& get (unsigned int idx) { return _op[idx]; }
  //   RDI_Op& tail (void)            { return _op[_opstail]; }

  // any string present in op will be copied; only the copys storage is managed by OpSeq
  void append(RDI_Op& op);
  void append(RDI_OpSeq* s);

  void model_effect(RDI_OpCode code); // update the types on stack

  // check_pre checks the validity of each append, but it cannot produce good error msgs,
  // so it is only used as an internal sanity (safety) check. 
  void check_pre(RDI_OpCode code); // if fails causes RDI_Fatal

  // These methods verify that the top type is correct; if not they either
  // generate a runtime check/cvt ocode or if this is not possible they set err to true and
  // put an appropriate error message is errbuf, where error_context is used as a prefix.
  void n_required(const char* error_context, CORBA::Boolean& err, char* errbuf);
  void b_required(const char* error_context, CORBA::Boolean& err, char* errbuf);
  void s_required(const char* error_context, CORBA::Boolean& err, char* errbuf);

  // finalize (re)computes relative offset of goto targets, adds final return op (if missing)
  void finalize(void);

  // debugging
  void dbg_record_stackT();
  void dbg_output_opcodes(RDIstrstream& str);
  void dbg_output_stackT(RDIstrstream& str);

 private:
  friend class RDI_RVM;

  int         _opstail;                     // current tail of _op    (-1 = empty) 
  int         _top;                         // current top of _stackT (-1 = empty) 
  RDI_Op      _op[RDI_Eval_OPSEQ_SIZE];     // opcodes that eval the constraint are collected here
  RDI_StaticT _stackT[RDI_Eval_STACK_SIZE]; // types that will be on the stack due to ops in _op

  // for debugging stackT
  int         _maxtop;
  char _stackT_descr[RDI_Eval_OPSEQ_SIZE][RDI_Eval_STACK_SIZE];
};

///////////////////////////////////////////////////////////////
// Logging

inline RDIstrstream& operator<< (RDIstrstream& str, const RDI_Op& op) { return op.log_output(str); }

#endif
