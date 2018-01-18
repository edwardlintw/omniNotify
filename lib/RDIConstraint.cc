// -*- Mode: C++; -*-
//                              File      : RDIConstraint.cc
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
//    Implementation of RDI_Constraint -- support class for Filter_i
//
 
#include <stdlib.h>

#include "RDIStringDefs.h"
#include "RDI.h"
#include "RDIStaticEvalDefs.h"

///////////////////////////////////////////////////////////////////////////////////////////////
// some tables

const char* RDI_ERT2string[] = {
  "ERT_bad",
  "ERT_string",
  "ERT_ident",
  "ERT_bool",
  "ERT_long",
  "ERT_double",
  "ERT_abstime",
  "ERT_reltime",
  "ERT_dynany"
};

///////////////////////////////////////////////////////////////////////////////////////////////
// Notes on codegen for component eval
//
// 2 'start' ops:
//     RDI_OpCode_wkp_NC2u,      // <well know part> nC -> u    $,  $event_name, etc. (ushort nC is RDI_WellKnownPart)
//     RDI_OpCode_special_sC2u,  // $foo             sC -> u    signals NONE_SUCH [foo is not well-known]
//
// Followed by zero or more of these comp-part ops:
//
//     RDI_OpCode_dot_id_usC2u,  // "<top>.foo"      u x sC -> u  signals TYPE_MISMATCH, NONE_SUCH
//     RDI_OpCode_dot_num_ulC2u, // "<top>.#  "      u x lC -> u  signals TYPE_MISMATCH, NONE_SUCH
//     RDI_OpCode_assoc_usC2u,   // "<top>(foo)"     u x sC -> u  signals TYPE_MISMATCH, NONE_SUCH
//     RDI_OpCode_index_ulC2u,   // "<top>[#]"       u x lC -> u  signals TYPE_MISMATCH, NONE_SUCH
//     RDI_OpCode_tagdef_u2u,    // "<top>.()"       u      -> u  signals TYPE_MISMATCH, NONE_SUCH
//     RDI_OpCode_tagid_usC2u,   // "<top>.(foo)"    u x sC -> u  signals TYPE_MISMATCH, NONE_SUCH
//     RDI_OpCode_tagnum_ulC2u,  // "<top>.([+|-]#)" u x lC -> u  signals TYPE_MISMATCH, NONE_SUCH
//     RDI_OpCode_tagchar_usC2u, // "<top>.('X')"    u x sC -> u  signals TYPE_MISMATCH, NONE_SUCH
//     RDI_OpCode_tagbool_ubC2u, // "<top>.(true)"   u x bC -> u  signals TYPE_MISMATCH, NONE_SUCH
//
//   * RDI_OpCode_dot_len_u2n_l, // <top>._length    u -> n   signals TYPE_MISMATCH
//   * RDI_OpCode_dot_d_u2u,     // <top>._d         u -> u   signals TYPE_MISMATCH
//   * RDI_OpCode_dot_tid_u2s,   // <top>._type_id   u -> s   signals NONE_SUCH
//   * RDI_OpCode_dot_rid_u2s,   // <top>._repos_id  u -> s   signals NONE_SUCH
//
// Followed by exactly one 'end' op, one of the following 3 ops:
//     RDI_OpCode_compend,       // marks end of component that is not otherwise marked by an 'end' op
//     RDI_OpCode_default_X2b,   // default <top>    X -> b   signals TYPE_MISMATCH
//     RDI_OpCode_exist_X2b,     // exist <top>      X -> b   signals TYPE_MISMATCH
//
// * These four comp-part ops could also be used as end markers.   For now they are not.
//
// NB compend must be used for a component that is part of an expression, including
// a component that appears on the lhs or rhs of the 'in' operator.

///////////////////////////////////////////////////////////////////////////////////////////////
// RDI_Constraint

// trigger top-level codegen
void RDI_Constraint::do_codegen(RDI_PCState* ps, CORBA::Boolean requirebool) {
  _genfun(ps, this);
  if (ps->e) return; // already have an error message
  if (requirebool) {
    ps->r_ops->b_required("final constraint result", ps->e, ps->b);
  }
}

// static constructors and associated code gen routines

RDI_Constraint*
RDI_Constraint::NewOr(RDI_PCState* ps, RDI_Constraint* l, RDI_Constraint* r) {
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("Or"));
  n->_lchild = l;  n->_rchild = r;  n->_genfun = RDI_Constraint::GenOr;
  return n;
}

void RDI_Constraint::GenOr(RDI_PCState* ps, RDI_Constraint* n) {
  char lblbuf[RDI_Eval_LBLBUF_LEN];
  RDI_Op::newlblnm("OrTrue", lblbuf);
  n->_lchild->_genfun(ps, n->_lchild);                           if (ps->e) return;
  ps->r_ops->b_required("LHS of OR", ps->e, ps->b);              if (ps->e) return;
  { RDI_Op op(RDI_OpCode_ifT_b2b, lblbuf, 0); ps->r_ops->append(op); } // if true, skip RHS eval (leaves true on stack)
  { RDI_Op op(RDI_OpCode_pop_u); ps->r_ops->append(op); }              // discard the false val
  n->_rchild->_genfun(ps, n->_rchild);                           if (ps->e) return;
  ps->r_ops->b_required("RHS of OR", ps->e, ps->b);              if (ps->e) return;
  { RDI_Op op(RDI_OpCode_label, lblbuf); ps->r_ops->append(op); }      // jump target
}

RDI_Constraint*
RDI_Constraint::NewAnd(RDI_PCState* ps, RDI_Constraint* l, RDI_Constraint* r) {
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("And"));
  n->_lchild = l;  n->_rchild = r;  n->_genfun = RDI_Constraint::GenAnd;
  return n;
}

void RDI_Constraint::GenAnd(RDI_PCState* ps, RDI_Constraint* n) {
  char lblbuf[RDI_Eval_LBLBUF_LEN];
  RDI_Op::newlblnm("AndFalse", lblbuf);
  n->_lchild->_genfun(ps, n->_lchild);                           if (ps->e) return;
  ps->r_ops->b_required("LHS of AND", ps->e, ps->b);             if (ps->e) return;
  { RDI_Op op(RDI_OpCode_ifF_b2b, lblbuf, 0); ps->r_ops->append(op); } // if false, skip RHS eval (leaves false on stack)
  { RDI_Op op(RDI_OpCode_pop_u); ps->r_ops->append(op); }              // discard the true val
  n->_rchild->_genfun(ps, n->_rchild);                           if (ps->e) return;
  ps->r_ops->b_required("RHS of AND", ps->e, ps->b);             if (ps->e) return;
  { RDI_Op op(RDI_OpCode_label, lblbuf); ps->r_ops->append(op); }      // jump target
}

RDI_Constraint*
RDI_Constraint::NewNot(RDI_PCState* ps, RDI_Constraint* c) {
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("NOT"));
  n->_lchild = c;  n->_genfun = RDI_Constraint::GenNot;
  return n;
}

void RDI_Constraint::GenNot(RDI_PCState* ps, RDI_Constraint* n) {
  n->_lchild->_genfun(ps, n->_lchild);                           if (ps->e) return;
  ps->r_ops->b_required("NOT", ps->e, ps->b);                    if (ps->e) return;
  { RDI_Op op(RDI_OpCode_not_b2b); ps->r_ops->append(op); }
}

// private tables used by NewCompare and GenCompare
static const char* RDI_CompareOp2str[] = {
  "==",  "!=",  "<=",  "<",  ">=",  ">"
};

#if 0
static const char* RDI_CompareOp2lhs_str[] = {
  "LHS of ==",  "LHS of !=",  "LHS of <=",  "LHS of <",  "LHS of >=",  "LHS >"
};
static const char* RDI_CompareOp2rhs_str[] = {
  "RHS of ==",  "RHS of !=",  "RHS of <=",  "RHS of <",  "RHS of >=",  "RHS >"
};
#endif

static const RDI_OpCode RDI_CompareOpZCmpCode[] = {
  RDI_OpCode_eqz_n2b,
  RDI_OpCode_nez_n2b,
  RDI_OpCode_lez_n2b,
  RDI_OpCode_ltz_n2b,
  RDI_OpCode_gez_n2b,
  RDI_OpCode_gtz_n2b
};

RDI_Constraint*
RDI_Constraint::NewCompare(RDI_PCState* ps, RDI_CompareOp whichone, RDI_Constraint* l, RDI_Constraint* r) {
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING(RDI_CompareOp2str[whichone]));
  n->_v_l = (CORBA::Long)whichone;
  n->_lchild = l;  n->_rchild = r;  n->_genfun = RDI_Constraint::GenCompare;
  return n;
}

// XXX could check the stack types after generating both lhs and rhs ops
void RDI_Constraint::GenCompare(RDI_PCState* ps, RDI_Constraint* n) {
  n->_lchild->_genfun(ps, n->_lchild);                                            if (ps->e) return;
  n->_rchild->_genfun(ps, n->_rchild);                                            if (ps->e) return;
  { RDI_Op op(RDI_OpCode_cmp_uu2n); ps->r_ops->append(op); }                      if (ps->e) return;
  { RDI_Op op(RDI_CompareOpZCmpCode[n->_v_l]);  ps->r_ops->append(op); }
}

RDI_Constraint*
RDI_Constraint::NewIn(RDI_PCState* ps, RDI_Constraint* l, RDI_Constraint* r) {
  if (r->_cachedop1._code == RDI_OpCode_ctelt_NC2n) {
    ps->e = 1;
    sprintf(ps->b, "rhs of op 'in' cannot be $curtime component (RHS must be sequence)'");
    return NULL;
  }
  r->_assert_not_endpart(ps);
  if (ps->e) { sprintf((ps->b + RDI_STRLEN(ps->b)), ", cannot be RHS of in operator (RHS must be sequence)"); return NULL; }
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("IN"));
  n->_lchild = l;  n->_rchild = r;  n->_genfun = RDI_Constraint::GenIN;
  return n;
}

void RDI_Constraint::GenIN(RDI_PCState* ps, RDI_Constraint* n) {
  n->_lchild->_genfun(ps, n->_lchild);                           if (ps->e) return;
  // note: no requirements at all on LHS (only place you can have an ident?) 
  n->_rchild->_genfun(ps, n->_rchild);                           if (ps->e) return;
  { RDI_Op op(RDI_OpCode_in_uu2b);  ps->r_ops->append(op); }
}

RDI_Constraint*
RDI_Constraint::NewSubStr(RDI_PCState* ps, RDI_Constraint* l, RDI_Constraint* r) {
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("SUBSTR"));
  n->_lchild = l;  n->_rchild = r;  n->_genfun = RDI_Constraint::GenSubStr;
  return n;
}

void RDI_Constraint::GenSubStr(RDI_PCState* ps, RDI_Constraint* n) {
  n->_lchild->_genfun(ps, n->_lchild);                           if (ps->e) return;
  ps->r_ops->s_required("RHS of ~", ps->e, ps->b);               if (ps->e) return;
  n->_rchild->_genfun(ps, n->_rchild);                           if (ps->e) return;
  ps->r_ops->s_required("RHS of ~", ps->e, ps->b);               if (ps->e) return;
  { RDI_Op op(RDI_OpCode_substr_ss2b);  ps->r_ops->append(op); }
}

// private tables used by NewArith and GenArith
static const char* RDI_ArithOp2str[] = {
  "+",  "-",  "*",  "/", "CMP"
};
static const char* RDI_ArithOp2lhs_str[] = {
  "LHS of +",  "LHS of -",  "LHS of *",  "LHS of /", "LHS of CMP"
};
static const char* RDI_ArithOp2rhs_str[] = {
  "RHS of +",  "RHS of -",  "RHS of *",  "RHS of /", "LHS of CMP"
};
static const RDI_OpCode RDI_ArithOp_opcode[] = {
  RDI_OpCode_add_nn2n,
  RDI_OpCode_sub_nn2n,
  RDI_OpCode_mul_nn2n,
  RDI_OpCode_div_nn2n,
  RDI_OpCode_rem_nn2n,
  RDI_OpCode_cmp_uu2n
};

RDI_Constraint*
RDI_Constraint::NewArith(RDI_PCState* ps, RDI_ArithOp whichone, RDI_Constraint* l, RDI_Constraint* r) {
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING(RDI_ArithOp2str[whichone]));
  n->_v_l = (CORBA::Long)whichone;
  n->_lchild = l;  n->_rchild = r;  n->_genfun = RDI_Constraint::GenArith;
  return n;
}

void RDI_Constraint::GenArith(RDI_PCState* ps, RDI_Constraint* n) {
  n->_lchild->_genfun(ps, n->_lchild);                                 if (ps->e) return;
  ps->r_ops->n_required(RDI_ArithOp2lhs_str[n->_v_l], ps->e, ps->b);   if (ps->e) return;
  n->_rchild->_genfun(ps, n->_rchild);                                 if (ps->e) return;
  ps->r_ops->n_required(RDI_ArithOp2rhs_str[n->_v_l], ps->e, ps->b);   if (ps->e) return;
  { RDI_Op op(RDI_ArithOp_opcode[n->_v_l]);  ps->r_ops->append(op); }
}

RDI_Constraint*
RDI_Constraint::NewCompEnd(RDI_PCState* ps, RDI_Constraint* c) {
  if (c->_cachedop1._code == RDI_OpCode_ctelt_NC2n) {
    // curtime elt case requires no compend
    return c;
  }
  // all other cases
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("COMPEND"));
  n->_lchild = c;  n->_genfun = RDI_Constraint::GenCompEnd;
  return n;
}

void RDI_Constraint::GenCompEnd(RDI_PCState* ps, RDI_Constraint* n) {
  n->_lchild->_genfun(ps, n->_lchild);                            if (ps->e) return;
  { RDI_Op op(RDI_OpCode_compend); ps->r_ops->append(op); }
}

RDI_Constraint*
RDI_Constraint::NewExist(RDI_PCState* ps, RDI_Constraint* c) {
  if (c->_cachedop1._code == RDI_OpCode_ctelt_NC2n) {
    CORBA::Boolean result = (((RDI_CurTimeElt)c->_cachedop1._arg._v_nc_us) != RDI_CurTime_error);
    RDI_Constraint* n_bconst = RDI_Constraint::NewBoolConst(ps, result);
    c->destruct_tree();    // c is no longer used
    return n_bconst;
  }
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("EXIST"));
  n->_lchild = c;  n->_genfun = RDI_Constraint::GenExist;
  return n;
}

void RDI_Constraint::GenExist(RDI_PCState* ps, RDI_Constraint* n) {
  n->_lchild->_genfun(ps, n->_lchild);                            if (ps->e) return;
  { RDI_Op op(RDI_OpCode_exist_X2b); ps->r_ops->append(op); }
}

RDI_Constraint*
RDI_Constraint::NewDefault(RDI_PCState* ps, RDI_Constraint* c) {
  if (c->_cachedop1._code == RDI_OpCode_ctelt_NC2n) {
    ps->e = 1;
    sprintf(ps->b, "default cannot be applied to $curtime component (no union part)'");
    return NULL;
  }
  if (c->_rightmost()->_cachedop1._code != RDI_OpCode_dot_d_u2u) {
    ps->e = 1;
    sprintf(ps->b, "default can only be applied to a component ending in '._d'");
    return NULL;
  }
  // remove dot_d since we want to apply default to component, not its discriminator
  c->_del_rightmost();
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("DEFAULT"));
  n->_lchild = c;  n->_genfun = RDI_Constraint::GenDefault;
  return n;
}

void RDI_Constraint::GenDefault(RDI_PCState* ps, RDI_Constraint* n) {
  n->_lchild->_genfun(ps, n->_lchild);                            if (ps->e) return;
  { RDI_Op op(RDI_OpCode_default_X2b); ps->r_ops->append(op); }
}

RDI_Constraint*
RDI_Constraint::NewComp(RDI_PCState* ps, char* topident) {
  RDI_Constraint* n = NULL;
  RDI_Constraint* n2 = NULL; // if n2 is used, we are converting $foo to $.foo
  if (topident == NULL) {
    n = new RDI_Constraint(RDI_BUILD_STRING("COMP"), RDI_BUILD_STRING("[DOLLAR]"));
    n->_cachedop1 = RDI_Op(RDI_OpCode_wkp_NC2u, ((CORBA::UShort)RDI_WKP_dollar));
  } else if (RDI_STR_EQ_I(topident, "curtime")) {
    n = new RDI_Constraint(RDI_BUILD_STRING("COMP"), RDI_BUILD_STRING("*CURTIME*")); 
    n->_cachedop1 = RDI_Op(RDI_OpCode_ctelt_NC2n);
  } else if (RDI_STR_EQ_I(topident, "_d")) {
    n2 = new RDI_Constraint(RDI_BUILD_STRING("DOT_D"), RDI_BUILD_STRING(topident));
    n2->_cachedop1 = RDI_Op(RDI_OpCode_dot_d_u2u);
  } else if (RDI_STR_EQ_I(topident, "_length")) {
    n2 = new RDI_Constraint(RDI_BUILD_STRING("DOT_LENGTH"), RDI_BUILD_STRING(topident));
    n2->_cachedop1 = RDI_Op(RDI_OpCode_dot_len_u2n_l);
  } else if (RDI_STR_EQ_I(topident, "_type_id")) {
    n2 = new RDI_Constraint(RDI_BUILD_STRING("DOT_TYPE_ID"), RDI_BUILD_STRING(topident));
    n2->_cachedop1 = RDI_Op(RDI_OpCode_dot_tid_u2s);
  } else if (RDI_STR_EQ_I(topident, "_repos_id")) {
    n2 = new RDI_Constraint(RDI_BUILD_STRING("DOT_REPOS_ID"), RDI_BUILD_STRING(topident));
    n2->_cachedop1 = RDI_Op(RDI_OpCode_dot_rid_u2s);
  } else {
    n = new RDI_Constraint(RDI_BUILD_STRING("COMP"), RDI_BUILD_STRING(topident));
    n->_cachedop1 = RDI_Op(RDI_OpCode_special_sC2u, RDI_BUILD_STRING(topident));
  }
  if (n2) {
    n = new RDI_Constraint(RDI_BUILD_STRING("COMP"), RDI_BUILD_STRING("[DOLLAR]"));
    n->_cachedop1 = RDI_Op(RDI_OpCode_wkp_NC2u, ((CORBA::UShort)RDI_WKP_dollar));
    n->_append_rightmost(n2);
  }
  n->_genfun = RDI_Constraint::GenCachedChain;
  if (n2) n2->_genfun = RDI_Constraint::GenCachedChain;
  return n;
}

RDI_Constraint*
RDI_Constraint::NewBoolConst(RDI_PCState* ps, CORBA::Boolean b) {
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("BConst"),
					   RDI_BUILD_STRING(b ? "True" : "False"));
  n->_cachedop1 = RDI_Op(RDI_OpCode_push_bC2b, b);
  n->_genfun = RDI_Constraint::GenCachedOp1;
  return n;
}

RDI_Constraint*
RDI_Constraint::NewLConst(RDI_PCState* ps, char* numstr) {
  char* delim = 0;
  CORBA::ULongLong ull = RDI_STRTOULL(numstr, &delim);
  if ((delim == 0) || (delim == numstr) || (*delim != '\0')) {
    ps->e = 1;
    sprintf(ps->b, "Overflow Error for Integer Constant %s", numstr); 
    return NULL;
  }
  if (ull <= RDI_ULONG_MAX) {
    // Value fits in a CORBA::ULong  --> push a ULong
    RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("+LConst_ULong"), RDI_BUILD_STRING(numstr));
    n->_cachedop1 = RDI_Op(RDI_OpCode_push_nC2n_ul, (CORBA::ULong)ull);
    n->_genfun = RDI_Constraint::GenCachedOp1;
    return n;
  }
  // Value fits in a CORBA::ULongLong  --> push a ULongLong
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("+LConst_ULongLong"), RDI_BUILD_STRING(numstr));
  n->_cachedop1 = RDI_Op(RDI_OpCode_push_nC2n_ull, ull);
  n->_genfun = RDI_Constraint::GenCachedOp1;
  return n;
}

RDI_Constraint*
RDI_Constraint::NewDConst(RDI_PCState* ps, char* numstr) {
  char* delim = 0;
  CORBA::Double vd = RDI_STRTOD(numstr, &delim);
  if ((delim == 0) || (delim == numstr) || (*delim != '\0')) {
    ps->e = 1;
    sprintf(ps->b, "Underflow/Overflow/Range Error for Floating Point Constant %s", numstr); 
    return NULL;
  }
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("+DConst"), RDI_BUILD_STRING(numstr));
  n->_cachedop1 = RDI_Op(RDI_OpCode_push_nC2n_d, vd);
  n->_genfun = RDI_Constraint::GenCachedOp1;
  return n;
}

RDI_Constraint*
RDI_Constraint::NewSConst(RDI_PCState* ps, char* str) {
  RDI_Constraint::_fix_string(ps, str);                           if (ps->e) return NULL;
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("SConst"), RDI_BUILD_STRING(str));
  if (RDI_STRLEN(str) == 1) {
    n->_cachedop1 = RDI_Op(RDI_OpCode_push_cC2c, RDI_BUILD_STRING(str));
  } else {
    n->_cachedop1 = RDI_Op(RDI_OpCode_push_sC2s, RDI_BUILD_STRING(str));
  }
  n->_genfun = RDI_Constraint::GenCachedOp1;
  return n;
}

RDI_Constraint*
RDI_Constraint::NewIdent(RDI_PCState* ps, char* str) {
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("EnumIdent"), RDI_BUILD_STRING(str));
  n->_cachedop1 = RDI_Op(RDI_OpCode_push_iC2i, RDI_BUILD_STRING(str));
  n->_genfun = RDI_Constraint::GenCachedOp1;
  return n;
}

RDI_Constraint*
RDI_Constraint::NewRuntimeError(RDI_PCState* ps, RDI_RTRetCode errcode) {
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("RUN_TIME_ERROR"));
  n->_cachedop1 = RDI_Op(RDI_OpCode_push_NC2N, (CORBA::UShort)errcode);
  n->_genfun = RDI_Constraint::GenRuntimeError;
  return n;
}

void RDI_Constraint::GenRuntimeError(RDI_PCState* ps, RDI_Constraint* n) {
  RDI_Constraint::GenCachedOp1(ps, n);                       // cached op pushes the error code
  { RDI_Op op(RDI_OpCode_signal_N); ps->r_ops->append(op); }
}

// instance methods...

// applied to numbers:
RDI_Constraint* RDI_Constraint::negate(RDI_PCState* ps) {
  // only applied to LConst or DConst
  if (_cachedop1._code == RDI_OpCode_push_nC2n_ul) {
    if (_cachedop1._arg._v_nc_ul >= RDI_LONG_MAX) {
      // too big for signed long, switch to signed long long
      _cachedop1._code = RDI_OpCode_push_nC2n_ll;
      _cachedop1._argT = RDI_OpArgT_nc_ll;
      _cachedop1._arg._v_nc_ll = - ((CORBA::LongLong)(_cachedop1._arg._v_nc_ul));
      CORBA_STRING_FREE(_descr);
      _descr = RDI_BUILD_STRING("-LConst_LongLong");
    } else {
      // switch to signed long
      _cachedop1._code = RDI_OpCode_push_nC2n_l;
      _cachedop1._argT = RDI_OpArgT_nc_l;
      _cachedop1._arg._v_nc_l = - ((CORBA::Long)(_cachedop1._arg._v_nc_ul));
      CORBA_STRING_FREE(_descr);
      _descr = RDI_BUILD_STRING("-LConst_Long");
    }
  } else if (_cachedop1._code == RDI_OpCode_push_nC2n_ull) {
    if (_cachedop1._arg._v_nc_ull >= RDI_LONGLONG_MAX) {
      // too big for signed long long -- overflow!
      ps->e = 1;
      sprintf(ps->b, "Overflow Error negating Integer Constant %s", _extra_descr);
      return NULL;
    } else {
      // switch to signed long long
      _cachedop1._code = RDI_OpCode_push_nC2n_ll;
      _cachedop1._argT = RDI_OpArgT_nc_ll;
      _cachedop1._arg._v_nc_ll = - ((CORBA::LongLong)(_cachedop1._arg._v_nc_ull));
      CORBA_STRING_FREE(_descr);
      _descr = RDI_BUILD_STRING("-LConst_LongLong");
    }
  } else { // DConst
    RDI_Assert(_cachedop1._code == RDI_OpCode_push_nC2n_d, "\n");
    _cachedop1._arg._v_nc_d = - _cachedop1._arg._v_nc_d;
    CORBA_STRING_FREE(_descr);
    _descr = RDI_BUILD_STRING("-DConst");
  }
  // XXX eventually get rid of modification of _extra_descr ???
  char* tmp = _extra_descr;
  _extra_descr = RDI_BUILD_STRING("-", tmp);
  CORBA_STRING_FREE(tmp);
  return this;
}

// applied to components:

RDI_Constraint*
RDI_Constraint::add_dot_ident(RDI_PCState* ps, char* ident) {
  _assert_not_endpart(ps);
  if (ps->e) { sprintf((ps->b + RDI_STRLEN(ps->b)), ", cannot be be followed by .%s", ident); return NULL; }
  RDI_Constraint* n = NULL;
  if (RDI_STR_EQ_I(ident, "_d")) {
    n = new RDI_Constraint(RDI_BUILD_STRING("DOT_D"), RDI_BUILD_STRING(ident));
    n->_cachedop1 = RDI_Op(RDI_OpCode_dot_d_u2u);
  } else if (RDI_STR_EQ_I(ident, "_length")) {
    n = new RDI_Constraint(RDI_BUILD_STRING("DOT_LENGTH"), RDI_BUILD_STRING(ident));
    n->_cachedop1 = RDI_Op(RDI_OpCode_dot_len_u2n_l);
  } else if (RDI_STR_EQ_I(ident, "_type_id")) {
    n = new RDI_Constraint(RDI_BUILD_STRING("DOT_TYPE_ID"), RDI_BUILD_STRING(ident));
    n->_cachedop1 = RDI_Op(RDI_OpCode_dot_tid_u2s);
  } else if (RDI_STR_EQ_I(ident, "_repos_id")) {
    n = new RDI_Constraint(RDI_BUILD_STRING("DOT_REPOS_ID"), RDI_BUILD_STRING(ident));
    n->_cachedop1 = RDI_Op(RDI_OpCode_dot_rid_u2s);
  } else {
    n = new RDI_Constraint(RDI_BUILD_STRING("DOT_IDENT"), RDI_BUILD_STRING(ident));
    n->_cachedop1 = RDI_Op(RDI_OpCode_dot_id_usC2u, RDI_BUILD_STRING(ident));
  }
  n->_genfun = RDI_Constraint::GenCachedChain;
  _append_rightmost(n);
  return this;
}

RDI_Constraint*
RDI_Constraint::add_dot_num(RDI_PCState* ps, char* digits) {
  _assert_not_endpart(ps);
  if (ps->e) { sprintf((ps->b + RDI_STRLEN(ps->b)), ", cannot be be followed by .%s", RDI_BUILD_STRING(digits)); return NULL; }
  char* delim = 0;
  CORBA::Long vl = RDI_STRTOL(digits, &delim);
  if ((delim == 0) || (delim == digits) || (*delim != '\0')) {
    ps->e = 1;
    sprintf(ps->b, "<expr>.<digits> : Overflow Error for <digits> field %s", digits);
    return NULL;
  }
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("DOT_NUM"), RDI_BUILD_STRING(digits));
  n->_v_l = vl;
  n->_cachedop1 = RDI_Op(RDI_OpCode_dot_num_ulC2u, n->_v_l);
  n->_genfun = RDI_Constraint::GenCachedChain;
  _append_rightmost(n);
  return this;
}

RDI_Constraint*
RDI_Constraint::add_index(RDI_PCState* ps, char* digits) {
  _assert_not_endpart(ps);
  if (ps->e) { sprintf((ps->b + RDI_STRLEN(ps->b)), ", cannot be be followed by [%s]", digits); return NULL; }
  char* delim = 0;
  CORBA::Long vl = RDI_STRTOL(digits, &delim);
  if ((delim == 0) || (delim == digits) || (*delim != '\0')) {
    ps->e = 1;
    sprintf(ps->b, "<expr>[<digits>] : Overflow Error for <digits> field %s", digits);
    return NULL;
  }
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("ARRAY_INDEX"), RDI_BUILD_STRING(digits));
  n->_v_l = vl;
  n->_cachedop1 = RDI_Op(RDI_OpCode_index_ulC2u, n->_v_l);
  n->_genfun = RDI_Constraint::GenCachedChain;
  _append_rightmost(n);
  return this;
}

RDI_Constraint*
RDI_Constraint::add_assoc(RDI_PCState* ps, char* ident) {
  _assert_not_endpart(ps);
  if (ps->e) { sprintf((ps->b + RDI_STRLEN(ps->b)), ", cannot be be followed by (%s)", ident); return NULL; }
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("NVPAIR_LOOKUP"), RDI_BUILD_STRING(ident));
  n->_cachedop1 = RDI_Op(RDI_OpCode_assoc_usC2u, RDI_BUILD_STRING(ident));
  n->_genfun = RDI_Constraint::GenCachedChain;
  _append_rightmost(n);
  return this;
}

RDI_Constraint*
RDI_Constraint::add_tag_default(RDI_PCState* ps) {
  _assert_not_endpart(ps);
  if (ps->e) { sprintf((ps->b + RDI_STRLEN(ps->b)), ", cannot be be followed by .()"); return NULL; }
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("TAG_DEFAULT"));
  n->_cachedop1 = RDI_Op(RDI_OpCode_tagdef_u2u);
  n->_genfun = RDI_Constraint::GenCachedChain;
  _append_rightmost(n);
  return this;
}

RDI_Constraint*
RDI_Constraint::add_tag_bool(RDI_PCState* ps, CORBA::Boolean b) {
  _assert_not_endpart(ps);
  if (ps->e) { sprintf((ps->b + RDI_STRLEN(ps->b)), ", cannot be be followed by .(%s)", b ? "TRUE" : "FALSE"); return NULL; }
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("TAG_BOOL"),
					   RDI_BUILD_STRING(b ? "True" : "False"));
  n->_cachedop1 = RDI_Op(RDI_OpCode_tagbool_ubC2u, b);
  n->_genfun = RDI_Constraint::GenCachedChain;
  _append_rightmost(n);
  return this;
}

RDI_Constraint*
RDI_Constraint::add_tag_char(RDI_PCState* ps, char* str) {
  _assert_not_endpart(ps);
  if (ps->e) { sprintf((ps->b + RDI_STRLEN(ps->b)), ", cannot be be followed by .('%s')", str); return NULL; }
  RDI_Constraint::_fix_string(ps, str);                           if (ps->e) return NULL;
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("TAG_CHAR"), RDI_BUILD_STRING(str));
  n->_cachedop1 = RDI_Op(RDI_OpCode_tagchar_usC2u, RDI_BUILD_STRING(str));
  n->_genfun = RDI_Constraint::GenCachedChain;
  _append_rightmost(n);
  return this;
}

RDI_Constraint*
RDI_Constraint::add_tag_ident(RDI_PCState* ps, char* ident) {
  _assert_not_endpart(ps);
  if (ps->e) { sprintf((ps->b + RDI_STRLEN(ps->b)), ", cannot be be followed by .(%s)", ident); return NULL; }
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("TAG_IDENT"), RDI_BUILD_STRING(ident));
  n->_cachedop1 = RDI_Op(RDI_OpCode_tagid_usC2u, RDI_BUILD_STRING(ident));
  n->_genfun = RDI_Constraint::GenCachedChain;
  _append_rightmost(n);
  return this;
}

RDI_Constraint*
RDI_Constraint::add_tag_num(RDI_PCState* ps, char* digits, CORBA::Boolean negate) {
  _assert_not_endpart(ps);
  if (ps->e) { sprintf((ps->b + RDI_STRLEN(ps->b)), ", cannot be be followed by .(%s%s)", negate ? "-" : "", digits); return NULL; }
  char* delim = 0;
  CORBA::Long vl = RDI_STRTOL(digits, &delim);
  if ((delim == 0) || (delim == digits) || (*delim != '\0')) {
    ps->e = 1;
    sprintf(ps->b, "<expr>.(<digits>) : Overflow Error for <digits> field %s%s", negate ? "-" : "", digits);
    return NULL;
  }
  RDI_Constraint* n = new RDI_Constraint(RDI_BUILD_STRING("TAG_NUM"),
					 RDI_BUILD_STRING((negate ? " - " : ""), digits));
  n->_v_l = vl;
  if (negate) n->_v_l = - n->_v_l;
  n->_cachedop1 = RDI_Op(RDI_OpCode_tagnum_ulC2u, n->_v_l);
  n->_genfun = RDI_Constraint::GenCachedChain;
  _append_rightmost(n);
  return this;
}

// comp_done is called to 'finalize' component construction
// If an error case is detected, ps->e is set to true and the node is left unchanged (caller must destroy it).
// Otherwise, the result can be the same node or a newly created node
// (in the latter case 'this' has been destroyed).
RDI_Constraint*
RDI_Constraint::comp_done(RDI_PCState* ps) {

  // ---------------------------- well-known component parts ----------------------------------------
  RDI_WellKnownPart part = RDI_WKP_dollar; // if this remains unchanged, do not change 'this'
  if (_cachedop1._code == RDI_OpCode_special_sC2u) {
    if (RDI_STR_EQ_I(_cachedop1._arg._v_sc, "header")) {
      part = RDI_WKP_hdr;             // $header
    } else if (RDI_STR_EQ_I(_cachedop1._arg._v_sc, "filterable_data")) {
      part = RDI_WKP_fdata;           // $filterable_data
    } else if (RDI_STR_EQ_I(_cachedop1._arg._v_sc, "remainder_of_body")) {
      part = RDI_WKP_rob;             // $remainder_of_body
    } else if (RDI_STR_EQ_I(_cachedop1._arg._v_sc, "event_name")) {
      part = RDI_WKP_ename;           // $event_name
    } else if (RDI_STR_EQ_I(_cachedop1._arg._v_sc, "type_name")) {
      part = RDI_WKP_tname;           // $type_name
    } else if (RDI_STR_EQ_I(_cachedop1._arg._v_sc, "domain_name")) {
      part = RDI_WKP_dname;           // $domain_name
    } else if (RDI_STR_EQ_I(_cachedop1._arg._v_sc, "event_type")) {
      part = RDI_WKP_etype;           // $event_type
    } else if (RDI_STR_EQ_I(_cachedop1._arg._v_sc, "fixed_header")) {
      part = RDI_WKP_fhdr;            // $fixed_header
    } else if (RDI_STR_EQ_I(_cachedop1._arg._v_sc, "variable_header")) {
      part = RDI_WKP_vhdr;            // $variable_header
    }
  } else if (_rchild && (_cachedop1._code == RDI_OpCode_wkp_NC2u) && 
	     (_rchild->_cachedop1._code == RDI_OpCode_dot_id_usC2u)) {
    if (RDI_STR_EQ_I(_rchild->_cachedop1._arg._v_sc, "remainder_of_body")) {
      part = RDI_WKP_rob;          // $.remainder_of_body
      RDI_Constraint* tmp = _rchild; _rchild = tmp->_rchild; delete tmp; // remove one rchild
    } else if (RDI_STR_EQ_I(_rchild->_cachedop1._arg._v_sc, "filterable_data")) {
      part = RDI_WKP_fdata;        // $.filterable_data
      RDI_Constraint* tmp = _rchild; _rchild = tmp->_rchild; delete tmp; // remove one rchild
    } else if (RDI_STR_EQ_I(_rchild->_cachedop1._arg._v_sc, "header")) {
      part = RDI_WKP_hdr;          // $.header
      RDI_Constraint* tmp = _rchild; _rchild = tmp->_rchild; delete tmp; // remove one rchild
    }
  }
  if (part != RDI_WKP_dollar) { // found a well-known part
    _cachedop1.clear(1); // frees any strings left in the cached op
    _cachedop1._code         = RDI_OpCode_wkp_NC2u;
    _cachedop1._argT         = RDI_OpArgT_nc_us;
    _cachedop1._arg._v_nc_us = (CORBA::UShort)part;
    return this;
  }

  // ----------------------------- $curtime special cases -------------------------------------------
  // Only 7 possible cases, including an error case.  Encode using an enum, discard the parts
  else if (_cachedop1._code == RDI_OpCode_ctelt_NC2n) {
    RDI_CurTimeElt eltkind = RDI_CurTime_error;
    const char* eltdesc = "*BAD CURTIME COMPONENT*";
    if (!(_rchild)) {                                                             // $curtime
      eltkind = RDI_CurTime_itself;
      eltdesc = "CURTIME COMPONENT: $curtime";
    }
    else if (!(_rchild->_rchild)) {
      if (_rchild->_cachedop1._code == RDI_OpCode_dot_id_usC2u) {
	if (RDI_STR_EQ_I(_rchild->_cachedop1._arg._v_sc, "time")) {               //  $curtime.time
	  eltkind = RDI_CurTime_time;
	  eltdesc = "CURTIME COMPONENT: $curtime.time";
	} else if (RDI_STR_EQ_I(_rchild->_cachedop1._arg._v_sc, "inacclo")) {     //  $curtime.inacclo
	  eltkind = RDI_CurTime_inacclo;
	  eltdesc = "CURTIME COMPONENT: $curtime.inacclo";
	} else if (RDI_STR_EQ_I(_rchild->_cachedop1._arg._v_sc, "inacchi")) {     //  $curtime.inacchi
	  eltkind = RDI_CurTime_inacchi;
	  eltdesc = "CURTIME COMPONENT: $curtime.inacchi";
	} else if (RDI_STR_EQ_I(_rchild->_cachedop1._arg._v_sc, "tdf")) {         //  $curtime.tdf 
	  eltkind = RDI_CurTime_tdf;
	  eltdesc = "CURTIME COMPONENT: $curtime.tdf";
	}
      }
    } else if (!(_rchild->_rchild->_rchild)) {
      if ((_rchild->_cachedop1._code == RDI_OpCode_dot_id_usC2u) &&
	  (RDI_STR_EQ_I(_rchild->_cachedop1._arg._v_sc, "time")) &&
	  (_rchild->_rchild->_cachedop1._code == RDI_OpCode_dot_id_usC2u)) {
	if (RDI_STR_EQ_I(_rchild->_rchild->_cachedop1._arg._v_sc, "low")) {       //  $curtime.time.low
	  eltkind = RDI_CurTime_time_low;
	  eltdesc = "CURTIME COMPONENT: $curtime.time.low";
	} else if (RDI_STR_EQ_I(_rchild->_rchild->_cachedop1._arg._v_sc, "hi")) { //  $curtime.time.high
	  eltkind = RDI_CurTime_time_high;
	  eltdesc = "CURTIME COMPONENT: $curtime.time.high";
	}
      }
    }
    // XXX Could generate a syntax error if eltkind == RDI_CurTime_error
    while (_rchild) _del_rightmost(); // the comp-parts are no longer needed
    _cachedop1._argT = RDI_OpArgT_nc_us;
    _cachedop1._arg._v_nc_us = (CORBA::UShort)eltkind;
    CORBA_STRING_FREE(_extra_descr);
    _extra_descr = RDI_BUILD_STRING(eltdesc);
    return this;
  }
  // all special cases have been handled; just return 'this' with no changes
  return this;
}

void RDI_Constraint::GenCachedOp1(RDI_PCState* ps, RDI_Constraint* n) {
  ps->r_ops->append(n->_cachedop1);
}

void RDI_Constraint::GenCachedChain(RDI_PCState* ps, RDI_Constraint* n) {
  ps->r_ops->append(n->_cachedop1);
  if (n->_rchild) {
    n->_rchild->_genfun(ps, n->_rchild);
  }
}

///////////////////////////////////////////////////////////////////////////////
// helpers

void RDI_Constraint::_append_rightmost(RDI_Constraint* n) {
  // if I were using a sequence I would not need to do this
  RDI_Constraint* rightmost = this;
  while (rightmost->_rchild) {
    rightmost = rightmost->_rchild;
  }
  rightmost->_rchild = n;
}

void RDI_Constraint::_del_rightmost(void) {
  // if I were using a sequence I would not need to do this
  RDI_Assert(this->_rchild, "only call del_rightmost on component with at least one subpart");
  RDI_Constraint* next2last = this;
  while (next2last->_rchild->_rchild) {
    next2last = next2last->_rchild;
  }
  delete next2last->_rchild;
  next2last->_rchild = NULL;
}

RDI_Constraint* RDI_Constraint::_rightmost(void) {
  RDI_Constraint* rightmost = this;
  while (rightmost->_rchild) {
    rightmost = rightmost->_rchild;
  }
  return rightmost;
}

void RDI_Constraint::_assert_not_endpart(RDI_PCState* ps) {
  RDI_OpCode opcode = _rightmost()->_cachedop1._code;
  if (opcode == RDI_OpCode_dot_len_u2n_l) {
    ps->e = 1;
    sprintf(ps->b, "<expr>._length, a number");
  } else if (opcode == RDI_OpCode_dot_d_u2u) {
    ps->e = 1;
    sprintf(ps->b, "<expr>._d, a discriminator value (number, bool, char, or enum)");
  } else if (opcode == RDI_OpCode_dot_tid_u2s) {
    ps->e = 1;
    sprintf(ps->b, "<expr>._type_id, a string");
  } else if (opcode == RDI_OpCode_dot_rid_u2s) {
    ps->e = 1;
    sprintf(ps->b, "<expr>._repos_id, a string");
  }
}

void RDI_Constraint::_fix_string(RDI_PCState* ps, char* s)
/* 
 * EFFECT:    remove enclosing quotes and internal backslash escapes
 *            On error, set ps->e and put error msg in ps->b
 * ASSUMES:   ending quote char is last character before \0
 * MODIFIES:  s
 */
{
  int prev_slash = 0;
  char* loc = s;
  char* ptr = s+1;
  while (*ptr != '\0') {
      if ((!prev_slash) && (*ptr == '\\')) {
          prev_slash = 1;
          ptr++;
          continue;
      }
      if (prev_slash && !((*ptr == '\\') || (*ptr == '\''))) {
	// bad string lit: put \ back in 
	*loc++ = '\\';
      }
      prev_slash = 0;
      *loc++ = *ptr++;
  }
  *(loc-1) = '\0';  // remove trailing quote
}

// ------------------------

void RDI_Constraint::dbg_output_tree(RDIstrstream& str, char* lbl) {
  if (lbl == NULL) lbl = RDI_BUILD_STRING("T"); // TOP
  // use label length to determine indent
  str.setw(RDI_STRLEN(lbl)*2);
  str << "[(" << lbl << ") "
    << (_descr ? _descr : "")
    << " {" << (_extra_descr ? _extra_descr : "") << "}"
    << "]\n";
  if (_lchild) _lchild->dbg_output_tree(str, RDI_BUILD_STRING(lbl, ".L"));
  if (_rchild) _rchild->dbg_output_tree(str, RDI_BUILD_STRING(lbl, ".R"));
  CORBA_STRING_FREE(lbl);
}

// ------------------------
