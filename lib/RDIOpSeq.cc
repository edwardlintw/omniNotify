// -*- Mode: C++; -*-
//                              File      : RDIOpSeq.cc
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
//    Implementation of RDI_OpSeq
//
 
#include "RDI.h"
#include "RDIStringDefs.h"
#include "RDIEvalDefs.h"

///////////////////////////////////////////////////////////////////////////////////////////////
// RDI_OpSeq

// finalize (re)computes relative offset of goto targets, adds final return op (if missing)
// and do final sanity checks
void RDI_OpSeq::finalize(void) {
  int i = 0;
  if (_op[_opstail]._code != RDI_OpCode_return_b) {
    { RDI_Op op(RDI_OpCode_return_b); append(op); }
  }
  // if you want to compute an initial guess (rather than using offset 0 == uninitialized)
  //    try to pick an offset which should be >= the actual label location
  for (i = 0; i <= _opstail; i++) {
    if (_op[i]._argT == RDI_OpArgT_lbl) {
      char* lbl = _op[i]._arg._v_lbl._lbl;
      RDI_Assert(lbl && RDI_STRLEN(lbl) > 0, "invalid target label in (conditional) goto op\n");
      int start_offset = _op[i]._arg._v_lbl._offset;
      if ((i+start_offset < 0) || (i+start_offset > _opstail)) {
	start_offset = 0; // bogus value, so use uninit value
      }
      CORBA::Boolean done = 0;
      int offset = start_offset;
      int bump = ((offset == 0) ? 1 : -1 ); // uninit, look forward first; otherwise look backward first
      for (; (i+offset >= 0) && (i+offset <= _opstail); offset += bump) {
	if ((_op[i+offset]._code == RDI_OpCode_label) && RDI_STR_EQ(lbl, _op[i+offset]._arg._v_sc)) {
	  done = 1; break;
	}
      }
      if (!done) {
	bump = - bump;
	offset = start_offset + bump;
	for (; (i+offset >= 0) && (i+offset <= _opstail); offset += bump) {
	  if ((_op[i+offset]._code == RDI_OpCode_label) && RDI_STR_EQ(lbl, _op[i+offset]._arg._v_sc)) {
	    done = 1; break;
	  }
	}
      }
      RDI_Assert(done, "Op sequence is missing target label " << lbl);
      // rather than jump to a label (a no-op) jump to next real op
      while (_op[i+offset]._code == RDI_OpCode_label) {
	offset++;
      }
      _op[i]._arg._v_lbl._offset = offset;
    }
  }
  // sanity checks
  for (i = 0; i <= _opstail; i++) {
    RDI_Assert(RDI_Op2ArgT[_op[i]._code] == _op[i]._argT, "bad argument type in op code");
  }
}

// append op and update the type stack as appropriate
void RDI_OpSeq::append(RDI_Op& op) {
  check_pre(op._code); // internal safety check
  ++_opstail; RDI_Assert(_opstail < RDI_Eval_OPSEQ_SIZE, "recompile with greater RDI_Eval_OPSEQ_SIZE\n");
  _op[_opstail].clear(1); // 1 means free any strings in the op XXX is this call needed?
  RDI_Op& targ = _op[_opstail];
  targ._code = op._code;
  targ._argT = op._argT;
  switch (op._argT) {
  case RDI_OpArgT_none:
    targ._arg._v_none = NULL;
    break;
  case RDI_OpArgT_sc:
    targ._arg._v_sc = RDI_BUILD_STRING(op._arg._v_sc);
    break;
  case RDI_OpArgT_bc:
    targ._arg._v_bc = op._arg._v_bc;
    break;
  case RDI_OpArgT_nc_us:
    targ._arg._v_nc_us = op._arg._v_nc_us;
    break;
  case RDI_OpArgT_nc_s:
    targ._arg._v_nc_s = op._arg._v_nc_s;
    break;
  case RDI_OpArgT_nc_ul:
    targ._arg._v_nc_ul = op._arg._v_nc_ul;
    break;
  case RDI_OpArgT_nc_l:
    targ._arg._v_nc_l = op._arg._v_nc_l;
    break;
  case RDI_OpArgT_nc_ull:
    targ._arg._v_nc_ull = op._arg._v_nc_ull;
    break;
  case RDI_OpArgT_nc_ll:
    targ._arg._v_nc_ll = op._arg._v_nc_ll;
    break;
  case RDI_OpArgT_nc_f:
    targ._arg._v_nc_f = op._arg._v_nc_f;
    break;
  case RDI_OpArgT_nc_d:
    targ._arg._v_nc_d = op._arg._v_nc_d;
    break;
  case RDI_OpArgT_lbl:
    targ._arg._v_lbl._offset = op._arg._v_lbl._offset;
    targ._arg._v_lbl._lbl    = RDI_BUILD_STRING(op._arg._v_lbl._lbl);
    break;
  default:
    RDI_Fatal("should not get here");
  }
  model_effect(op._code); // update the types on stack
  if (RDIDbgTst(RDIDbgEval)) {
    dbg_record_stackT();
  }
}

void RDI_OpSeq::append(RDI_OpSeq* s) {
  for (int i = 0; i <= s->_opstail; i++) {
    append(s->_op[i]);
  }
}

// ----------------------

// append can check the validity of the stack types, but it cannot produce good error msgs.
// these methods verify that the top type is correct; if not they set err to true and
// put an appropriate error message is errbuf, where error_context is used as a prefix.
//

//////////////
//  Requirement methods
//

// for these methods, StaticT_u is not an acceptable top type, thus the appropriate
// runtime test / conversion op (a _cvt_ op) must be generated.
//   (cvt_u2b opcode used for b_required)
//   (cvt_u2s opcode used for s_required)
//   (cvt_u2n opcode used for n_required)

void RDI_OpSeq::n_required(const char* error_context, CORBA::Boolean& err, char* errbuf) {
  RDI_Assert(_top >= 0, "\n");
  switch (_stackT[_top]) {
  case RDI_StaticT_n:
  case RDI_StaticT_N: {
    return; // OK
  }
  case RDI_StaticT_u: { // use runtime conversion op
    { RDI_Op op(RDI_OpCode_cvt_u2n); append(op); }
    return;
  }
  case RDI_StaticT_b: {
    if (_op[_opstail]._code == RDI_OpCode_push_bC2b) {
      // peephole optimization; change push bool to push of ushort
      CORBA::UShort us = (CORBA::UShort)(_op[_opstail]._arg._v_bc ? 1 : 0);
      _op[_opstail]._code = RDI_OpCode_push_NC2N;
      _op[_opstail]._argT = RDI_OpArgT_nc_us;
      _op[_opstail]._arg._v_nc_us = us;
      _stackT[_top] = RDI_StaticT_n;
      _stackT_descr[_opstail][_top] = 'N'; // capital N means forced to numeric
    } else { // use runtime conversion op
      { RDI_Op op(RDI_OpCode_cvt_u2n); append(op); }
    }
    return;
  }
  case RDI_StaticT_c: {
    // change to a ushort const and return
    RDI_Assert(_op[_opstail]._code == RDI_OpCode_push_cC2c, "\n");
    CORBA::UShort us = (CORBA::UShort)(_op[_opstail]._arg._v_sc[0]);
    _op[_opstail].clear(1); // free the string
    _op[_opstail]._code = RDI_OpCode_push_NC2N;
    _op[_opstail]._argT = RDI_OpArgT_nc_us;
    _op[_opstail]._arg._v_nc_us = us;
    _stackT[_top] = RDI_StaticT_n;
    _stackT_descr[_opstail][_top] = 'N'; // capital N means forced to numeric
    return;
  }
  case RDI_StaticT_i: {
    sprintf(errbuf, "%s: expecting number, found enum ident %s", error_context, _op[_opstail]._arg._v_sc);
    err = 1; return;
  }
  case RDI_StaticT_s: {
    sprintf(errbuf, "%s: expecting number, found string %s (with length > 1)", error_context, _op[_opstail]._arg._v_sc);
    err = 1; return;
  }
  }
}

void RDI_OpSeq::s_required(const char* error_context, CORBA::Boolean& err, char* errbuf) {
  RDI_Assert(_top >= 0, "\n");
  switch (_stackT[_top]) {
  case RDI_StaticT_s: {
    return; // OK
  }
  case RDI_StaticT_u: { // use runtime conversion op
    { RDI_Op op(RDI_OpCode_cvt_u2s); append(op); }
    return;
  }
  case RDI_StaticT_c: {
    // force static type to string constant and return
    RDI_Assert(_op[_opstail]._code == RDI_OpCode_push_cC2c, "\n");
    _op[_opstail]._code = RDI_OpCode_push_sC2s;
    _stackT[_top] = RDI_StaticT_s;
    _stackT_descr[_opstail][_top] = 'S'; // capital S means forced from c to s
    return;
  }
  case RDI_StaticT_i: {
    sprintf(errbuf, "%s: expecting string, found enum ident %s", error_context, _op[_opstail]._arg._v_sc);
    err = 1; return;
  }
  case RDI_StaticT_b: {
    sprintf(errbuf, "%s: expecting string, found boolean", error_context);
    err = 1; return;
  }
  case RDI_StaticT_n:
  case RDI_StaticT_N: {
    sprintf(errbuf, "%s: expecting string, found number", error_context);
    err = 1; return;
  }
  }
}

void RDI_OpSeq::b_required(const char* error_context, CORBA::Boolean& err, char* errbuf) {
  RDI_Assert(_top >= 0, "\n");
  switch (_stackT[_top]) {
  case RDI_StaticT_b: {
    return; // OK
  }
  case RDI_StaticT_u: { // use runtime conversion op
    { RDI_Op op(RDI_OpCode_cvt_u2b); append(op); }
    return;
  }
  case RDI_StaticT_i: {
    sprintf(errbuf, "%s: expecting bool, found enum ident %s", error_context, _op[_opstail]._arg._v_sc);
    err = 1; return;
  }
  case RDI_StaticT_c: {
    sprintf(errbuf, "%s: expecting bool, found char", error_context);
    err = 1; return;
  }
  case RDI_StaticT_s: {
    sprintf(errbuf, "%s: expecting bool, found string", error_context);
    err = 1; return;
  }
  case RDI_StaticT_n:
  case RDI_StaticT_N: {
    sprintf(errbuf, "%s: expecting bool, found number", error_context);
    err = 1; return;
  }
  }
}

// ----------------------

void RDI_OpSeq::dbg_record_stackT(void) {
  for (int typei = 0; typei < RDI_Eval_STACK_SIZE; typei++) {
    char& loc = _stackT_descr[_opstail][typei];
    if (typei > _top) {
      loc = ' '; continue;
    }
    switch (_stackT[typei]) {
    case RDI_StaticT_b:
      loc = 'b'; break;
    case RDI_StaticT_c:
      loc = 'c'; break;
    case RDI_StaticT_i:
      loc = 'i'; break;
    case RDI_StaticT_n:
      loc = 'n'; break;
    case RDI_StaticT_N:
      loc = 'N'; break;
    case RDI_StaticT_s:
      loc = 's'; break;
    case RDI_StaticT_u:
      loc = 'u'; break;
    }
  }
}

void RDI_OpSeq::dbg_output_stackT(RDIstrstream& str) {
  for (int i1 = 0; i1 <= _opstail; i1++) {
    str <<  "--";
  }
  str <<  "\n";
  for (int typei = _maxtop; typei >= 0; typei--) { 
    for (int opi = 0; opi <= _opstail; opi++) {
      str <<  _stackT_descr[opi][typei] << " ";
    }
    str <<  "\n";
  }
  for (int i2 = 0; i2 <= _opstail; i2++) {
    str <<  "--";
  }
  str << "\n";
}

void RDI_OpSeq::dbg_output_opcodes(RDIstrstream& str) {
  for (int i = 0; i <= _opstail; i++) {
    CORBA::Boolean signal_const = ((i < _opstail) &&
			 (_op[i]._code   == RDI_OpCode_push_NC2N) &&
			 (_op[i+1]._code == RDI_OpCode_signal_N));
    _op[i].dbg_output(str, signal_const);
    str << '\n';
  }
}

//////////////////////////////////
// check_pre and model_effect

// check type preconditions.  if fails causes RDI_Fatal
// Note that Statict_c does not need to be one of the legal cases for
//   number or string because we force to long or string const for these cases.
void RDI_OpSeq::check_pre(RDI_OpCode code) {
  switch (RDI_Op2StackPre[code]) {
  case RDI_StackPre_none:
    return;
  case RDI_StackPre_X:
    RDI_Assert((_top >= 0), "at least one stack elt required\n");
    return;
  case RDI_StackPre_XX:
    RDI_Assert((_top >= 1), "at least two stack elts required\n");
    return;
  case RDI_StackPre_Xu:
    RDI_Assert((_top >= 1), "at least two stack elts required\n");
    RDI_Assert((_stackT[_top] == RDI_StaticT_u), "top must be u\n");
    return;
  case RDI_StackPre_b:
    RDI_Assert((_top >= 0), "at least one stack elt required\n");
    RDI_Assert((_stackT[_top] == RDI_StaticT_b), "top must be b\n");
    return;
  case RDI_StackPre_n:
    RDI_Assert((_top >= 0), "at least one stack elt required\n");
    RDI_Assert((_stackT[_top] == RDI_StaticT_n) ||
	       (_stackT[_top] == RDI_StaticT_N), "top must be n or N\n");
    return;
  case RDI_StackPre_N:
    RDI_Assert((_top >= 0), "at least one stack elt required\n");
    RDI_Assert((_stackT[_top] == RDI_StaticT_N), "top must be ushort (N)\n");
    return;
  case RDI_StackPre_u:
    RDI_Assert((_top >= 0), "at least one stack elt required\n");
    RDI_Assert((_stackT[_top] == RDI_StaticT_u), "top must be u\n");
    return;

  case RDI_StackPre_bb:
    RDI_Assert((_top >= 1), "at least two stack elts required\n");
    RDI_Assert((_stackT[_top-1] == RDI_StaticT_b), "(top-1) must be b\n");
    RDI_Assert((_stackT[_top] == RDI_StaticT_b), "top must be b\n");
    return;
  case RDI_StackPre_ss:
    RDI_Assert((_top >= 1), "at least two stack elts required\n");
    RDI_Assert((_stackT[_top-1] == RDI_StaticT_s), "(top-1) must be s\n");
    RDI_Assert((_stackT[_top] == RDI_StaticT_s), "top must be s\n");
    return;
  case RDI_StackPre_nn:
    RDI_Assert((_top >= 1), "at least two stack elts required\n");
    RDI_Assert((_stackT[_top-1] == RDI_StaticT_n) ||
	       (_stackT[_top-1] == RDI_StaticT_N), "(top-1) must be n or N\n");
    RDI_Assert((_stackT[_top] == RDI_StaticT_n) ||
	       (_stackT[_top] == RDI_StaticT_N), "top must be n or N\n");
    return;
  }
}

// update the types on stack
void RDI_OpSeq::model_effect(RDI_OpCode code) {
  RDI_StaticT tmp;
  switch (RDI_Op2StackEffect[code]) {
  case RDI_StackEffect_none:
    return;
  case RDI_StackEffect_X:
    RDI_Assert(_top >= 0, "\n");
    _top--;
    return;
  case RDI_StackEffect_XX:
    RDI_Assert(_top >= 1, "\n");
    _top -= 2;
    return;
  case RDI_StackEffect_swap:
    RDI_Assert(_top >= 1, "\n");
    tmp = _stackT[_top];
    _stackT[_top]   = _stackT[_top-1];
    _stackT[_top-1] = tmp;
    return;
  case RDI_StackEffect_2b:
    _top++; RDI_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE\n");
    _maxtop = (_top > _maxtop) ? _top : _maxtop;
    _stackT[_top] = RDI_StaticT_b;
    return;
  case RDI_StackEffect_2c:
    _top++; RDI_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE\n");
    _maxtop = (_top > _maxtop) ? _top : _maxtop;
    _stackT[_top] = RDI_StaticT_c;
    return;
  case RDI_StackEffect_2i:
    _top++; RDI_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE\n");
    _maxtop = (_top > _maxtop) ? _top : _maxtop;
    _stackT[_top] = RDI_StaticT_i;
    return;
  case RDI_StackEffect_2n:
    _top++; RDI_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE\n");
    _maxtop = (_top > _maxtop) ? _top : _maxtop;
    _stackT[_top] = RDI_StaticT_n;
    return;
  case RDI_StackEffect_2N:
    _top++; RDI_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE\n");
    _maxtop = (_top > _maxtop) ? _top : _maxtop;
    _stackT[_top] = RDI_StaticT_N;
    return;
  case RDI_StackEffect_2s:
    _top++; RDI_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE\n");
    _maxtop = (_top > _maxtop) ? _top : _maxtop;
    _stackT[_top] = RDI_StaticT_s;
    return;
  case RDI_StackEffect_2u:
    _top++; RDI_Assert(_top < RDI_Eval_STACK_SIZE, "recompile with greater RDI_Eval_STACK_SIZE\n");
    _maxtop = (_top > _maxtop) ? _top : _maxtop;
    _stackT[_top] = RDI_StaticT_u;
    return;
  case RDI_StackEffect_X2b:
    RDI_Assert(_top >= 0, "\n");
    _stackT[_top] = RDI_StaticT_b;
    return;
  case RDI_StackEffect_X2n:
    RDI_Assert(_top >= 0, "\n");
    _stackT[_top] = RDI_StaticT_n;
    return;
  case RDI_StackEffect_X2s:
    RDI_Assert(_top >= 0, "\n");
    _stackT[_top] = RDI_StaticT_s;
    return;
  case RDI_StackEffect_X2u:
    RDI_Assert(_top >= 0, "\n");
    _stackT[_top] = RDI_StaticT_u;
    return;
  case RDI_StackEffect_XX2b:
    RDI_Assert(_top >= 1, "\n");
    _top--;
    _stackT[_top] = RDI_StaticT_b;
    return;
  case RDI_StackEffect_XX2n:
    RDI_Assert(_top >= 1, "\n");
    _top--;
    _stackT[_top] = RDI_StaticT_n;
    return;
  }
} 

