// -*- Mode: C++; -*-
//                              File      : RDIStaticEval.cc
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
//    Implementation of RDI_PCState
//
 
#include "RDI.h"
#include "RDIStaticEvalDefs.h"
#include "RDIStringDefs.h"

extern int yyparse(void*);

#if 0
typedef union{
  /* the type of yylval */
  void* noval;     /* should be set to NULL */
  char* sb;
  RDI_Constraint* cexpr;
} YYSTYPE;
#endif

#include "ModFlexLexer.h"

RDI_PCState::RDI_PCState(void) : e(0), r_tree(NULL) {
  r_ops = new RDI_OpSeq();
  lexer = new yyFlexLexer(NULL, NULL); // use null for [i/o]stream???
}

RDI_PCState::~RDI_PCState(void) 
{
  RDI_DELNULL(r_ops);
  RDI_DELNULL(lexer);
  deltree();
}

int RDI_PCState::next_token(void* lvalp) {
  return lexer->yylex((YYSTYPE*) lvalp, this);
}

void RDI_PCState::deltree(void) 
{
  if (r_tree) {
    r_tree->destruct_tree();
    r_tree = 0;
  }
}

int RDI_PCState::lexer_input(char* buf, int max_size) {
  if (_parse_str == NULL) return -1; // ERROR
  if (_remaining_chars == 0) return 0; // should be YY_NULL
  int xfer_chars = (max_size < _remaining_chars) ? max_size : _remaining_chars;
  RDI_STRNCPY(buf, &(_parse_str[_str_offset]), xfer_chars);
  _remaining_chars -= xfer_chars;
  _str_offset += xfer_chars;
  return xfer_chars;
}

//  After call, ps->e is error code, if TRUE then ps->b contains error message
//  if false (no error) then ps->r is the result.
void RDI_PCState::parse_string(const char* s) 
{
  e = 0;
  r_ops->clearops(); // must clear ps->_ops for each new parse
  deltree(); // if tree exists, destroy it since we will build a new one
  if (!s) {
    e = 1;
    sprintf(b, "XXX RDI_Parse_String called with null string");
    return;
  }
  // init private state
  _regc_tail = -1;
  _regs_tail = -1;
  _str_offset = 0;
  _parse_str = CORBA_STRING_DUP(s);
  _remaining_chars = RDI_STRLEN(_parse_str);

  // do parse
  yyparse((void*)this);
  if (!e) {
    assert_none_reg();
  } else {
    delete_all_reg();
  }
  if (!e) {
    if (r_tree) {
      // OK so far; generate the code
      r_tree->do_codegen(this, 1);
      if (e) { // error found during code gen
#ifndef NDEBUG
	if (RDIDbgTst(RDIDbgEval)) {
	  RDIDbgLogger(l, RDIDbgEval_nm);
	  // don't return a tree when there is an error 
	  l.str << "XXX Error During Code Gen; Output Tree for Debug Purposes\n";
	  r_tree->dbg_output_tree(l.str);
	  l.str << "OpCodes:\n";
	  r_ops->dbg_output_opcodes(l.str);
	  l.str << "Type Stack:\n";
	  r_ops->dbg_output_stackT(l.str);
	  l.str << '\n';
	}
#endif
	deltree(); // probably not necessary
      } else {
#if 0
	// NORMALLY DISABLED -- TOO MUCH INFO
	RDIDbgLogger(l, RDIDbgEval_nm);
	// don't return a tree when there is an error 
	l.str << "Code Gen Successful\n";
	r_tree->dbg_output_tree(l.str);
	l.str << "OpCodes:\n";
	r_ops->dbg_output_opcodes(l.str);
	l.str << "Type Stack:\n";
	r_ops->dbg_output_stackT(l.str);
	l.str << '\n';
#endif
      }
    } else {
      // a generic syntax error caught by yacc; no error msg generated yet
      e = 1;
      sprintf(b, "Parse failed: badly formed constraint expression");
    }
  }
  if (e && r_tree) {
    // after top-level reduction, extra tokens caused an error, (e.g.,  1 == 1 'a')
    sprintf(b, "Parse failed: badly formed constraint expression");
    deltree(); // probably not necessary
  }
  // cleanup
  CORBA_STRING_FREE(_parse_str);
  _parse_str = 0;	// don't track string beyond this call
}

void RDI_PCState::regstr(char *s) {
  if (s == NULL) return;
  ++_regs_tail; RDI_Assert(_regs_tail < RDI_MAX_PARSE_STACK, "recompile with larger RDI_MAX_PARSE_STACK");
  _regs[_regs_tail] = s;
}

void RDI_PCState::unregstr(char *s) {
  if (s == NULL) return;
  for (int i = _regs_tail; i >= 0; i--) {
    if (_regs[i] == s) {
      for (int j = i+1; j <= _regs_tail; j++) {
        _regs[j-1] = _regs[j];
      }
      _regs_tail--;
      return;
    }
  }
}

void RDI_PCState::unregdelstr(char *s) {
  if (s == NULL) return;
  unregstr(s);
  CORBA_STRING_FREE(s);
}

void RDI_PCState::regcexpr(RDI_Constraint *c) {
  if (c == NULL) return;
  ++_regc_tail; RDI_Assert(_regc_tail < RDI_MAX_PARSE_STACK, "recompile with larger RDI_MAX_PARSE_STACK");
  _regc[_regc_tail] = c;
}

void RDI_PCState::unregcexpr(RDI_Constraint *c) {
  if (c == NULL) return;
  for (int i = _regc_tail; i >= 0; i--) {
    if (_regc[i] == c) {
      for (int j = i+1; j <= _regc_tail; j++) {
        _regc[j-1] = _regc[j];
      }
      _regc_tail--;
      return;
    }
  }
}

void RDI_PCState::unregdelcexpr(RDI_Constraint *c) {
  if (c == NULL) return;
  unregcexpr(c);
  c->destruct_tree();
}

void RDI_PCState::delete_all_reg() {

  while (_regc_tail >= 0) {
    _regc[_regc_tail]->destruct_tree();
    _regc_tail--;
  }

  while (_regs_tail >= 0) {
    CORBA_STRING_FREE(_regs[_regs_tail]);
    _regs_tail--;
  }
}

void RDI_PCState::assert_none_reg() {
  RDI_Assert(_regs_tail == -1, "all alloc'd strings should be unregistered");
  RDI_Assert(_regc_tail == -1, "all RDI_Constraint objs should be unregistered");
}

