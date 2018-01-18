// -*- Mode: C++; -*-
//                              File      : RDIStaticEvalDefs.h
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
 
#ifndef _RD_STATICEVALDEFS_H_
#define _RD_STATICEVALDEFS_H_


#include "RDIEvalDefs.h"   // shared by static and dynamic eval
#include "RDIConstraint.h" // only doing static parsing ??? [or does this include belong in EvalDefs?]

// XXX could use YYMAXDEPTH but it can be very large
// XXX 500 allows for a lot of nesting
#define RDI_MAX_PARSE_STACK 500

// This file contains defs for static eval (parsing, static checking, static optimizations)

// NB one scanner is created per PCState object, no mutual exclusion ops are used,
// thus either each thread that calls parse_string must have its own PCState obj
// or an external mutex must be used to ensure one parse at a time.

class yyFlexLexer;

class RDI_PCState { // PCState = ParseConstraint State (runtime state of parse)
 public:
  // these members are used in yacc.y and also in RDI_Constraint code
  CORBA::Boolean e;       // e: error?
  char b[1024];           // b: error buf 
  RDI_Constraint* r_tree; // r_tree: resulting parse tree
  RDI_OpSeq*      r_ops;  // r_ops:  resulting op sequence
  yyFlexLexer*    lexer;  // scanner

  RDI_PCState(void);
  ~RDI_PCState(void);

  void deltree(void);
  void parse_string(const char* s);

  int next_token(void*);

  int lexer_input(char* buf, int max_size);

  void regstr(char *);
  void unregstr(char *);
  void unregdelstr(char *);

  void regcexpr(RDI_Constraint *);
  void unregcexpr(RDI_Constraint *);
  void unregdelcexpr(RDI_Constraint *);

  void delete_all_reg();
  void assert_none_reg();


 private:
  // used during parse:

  char*            _parse_str;
  int              _str_offset;
  int              _remaining_chars;
  char*            _regs[RDI_MAX_PARSE_STACK];
  RDI_Constraint*  _regc[RDI_MAX_PARSE_STACK];
  int              _regc_tail;
  int              _regs_tail;
};

#endif
