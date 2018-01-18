// -*- Mode: C++; -*-
//                              File      : RDIConstraint.h
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
 
#ifndef _RD_CONSTRAINT_H_
#define _RD_CONSTRAINT_H_

#include "RDIEvalDefs.h"
#include "CosNotifyShorthands.h"

class RDI_PCState;
class RDI_Constraint;

typedef void RDI_CodeGenFun(RDI_PCState*, RDI_Constraint*);

class RDI_Constraint {
public:
  RDI_Constraint(char* d=0, char* x=0) : 
			_lchild(0), _rchild(0), _descr(d), _extra_descr(x) {;}
  ~RDI_Constraint() {
    _lchild = 0; _rchild = 0;
    RDI_STRDELNULL(_descr);
    RDI_STRDELNULL(_extra_descr);
    _cachedop1.clear(1); // frees any strings left in the cached op
  };

  void destruct_tree(void) { // unlike class destructor, destructs entire tree
    if (_lchild) _lchild->destruct_tree();
    if (_rchild) _rchild->destruct_tree();
    delete this;
  }
  // static constructors
  static RDI_Constraint* NewOr(RDI_PCState* ps, 
			       	RDI_Constraint* l, RDI_Constraint* r);
  static RDI_Constraint* NewAnd(RDI_PCState* ps, 
				RDI_Constraint* l, RDI_Constraint* r);
  static RDI_Constraint* NewCompare(RDI_PCState* ps, RDI_CompareOp whichone, 
				RDI_Constraint* l, RDI_Constraint* r);
  static RDI_Constraint* NewIn(RDI_PCState* ps, 
				RDI_Constraint* l, RDI_Constraint* r);
  static RDI_Constraint* NewSubStr(RDI_PCState* ps, 
				RDI_Constraint* l, RDI_Constraint* r);
  static RDI_Constraint* NewArith(RDI_PCState* ps, RDI_ArithOp whichone, 
				RDI_Constraint* l, RDI_Constraint* r);
  static RDI_Constraint* NewNot(RDI_PCState* ps, RDI_Constraint* c);
  static RDI_Constraint* NewBoolConst(RDI_PCState* ps, CORBA::Boolean b);
  static RDI_Constraint* NewCompEnd(RDI_PCState* ps, RDI_Constraint* c);
  static RDI_Constraint* NewExist(RDI_PCState* ps, RDI_Constraint* c);
  static RDI_Constraint* NewDefault(RDI_PCState* ps, RDI_Constraint* c);
  static RDI_Constraint* NewComp(RDI_PCState* ps, char* topident);
  static RDI_Constraint* NewLConst(RDI_PCState* ps, char* numstr);
  static RDI_Constraint* NewDConst(RDI_PCState* ps, char* numstr);
  static RDI_Constraint* NewSConst(RDI_PCState* ps, char* str);
  static RDI_Constraint* NewIdent(RDI_PCState* ps, char* str);
  static RDI_Constraint* NewRuntimeError(RDI_PCState* ps, RDI_RTRetCode ecode);

  // static codegen routines
  static void GenOr          (RDI_PCState* ps, RDI_Constraint* n);
  static void GenAnd         (RDI_PCState* ps, RDI_Constraint* n);
  static void GenNot         (RDI_PCState* ps, RDI_Constraint* n);
  static void GenCompare     (RDI_PCState* ps, RDI_Constraint* n);
  static void GenIN          (RDI_PCState* ps, RDI_Constraint* n);
  static void GenSubStr      (RDI_PCState* ps, RDI_Constraint* n);
  static void GenArith       (RDI_PCState* ps, RDI_Constraint* n);
  static void GenCompEnd     (RDI_PCState* ps, RDI_Constraint* n);
  static void GenExist       (RDI_PCState* ps, RDI_Constraint* n);
  static void GenDefault     (RDI_PCState* ps, RDI_Constraint* n);
  static void GenRuntimeError(RDI_PCState* ps, RDI_Constraint* n);
  static void GenCachedOp1   (RDI_PCState* ps, RDI_Constraint* n);
  static void GenCachedChain (RDI_PCState* ps, RDI_Constraint* n);

  // instance methods...

  // applied to numbers:
  RDI_Constraint* negate(RDI_PCState* ps);

  // applied to components:
  RDI_Constraint* comp_done(RDI_PCState* ps);
  RDI_Constraint* add_dot_ident(RDI_PCState* ps, char* ident);
  RDI_Constraint* add_dot_num(RDI_PCState* ps, char* digits);
  RDI_Constraint* add_index(RDI_PCState* ps, char* digits);
  RDI_Constraint* add_assoc(RDI_PCState* ps, char* ident);
  RDI_Constraint* add_tag_default(RDI_PCState* ps);
  RDI_Constraint* add_tag_bool(RDI_PCState* ps, CORBA::Boolean b);
  RDI_Constraint* add_tag_char(RDI_PCState* ps, char* str);
  RDI_Constraint* add_tag_ident(RDI_PCState* ps, char* ident);
  RDI_Constraint* add_tag_num(RDI_PCState* ps, char* digits, CORBA::Boolean negate);

  // trigger top-level codegen
  void do_codegen(RDI_PCState* ps, CORBA::Boolean requirebool = 0);

  // debugging
  void dbg_output_tree(RDIstrstream& str, char* lbl = 0);

private:
  // private state
  friend void RDI_Parse_String(RDI_PCState* ps, char* s);

  RDI_Constraint*   _lchild;
  RDI_Constraint*   _rchild;
  char*             _descr;
  char*             _extra_descr;

  CORBA::Long       _v_l; 

  RDI_CodeGenFun*   _genfun;
  RDI_Op            _cachedop1; // invariant: cached op owns its string arg

  // private methods (helpers)
  void _append_rightmost(RDI_Constraint* n);
  void _del_rightmost(void);
  RDI_Constraint* _rightmost(void);
  void _assert_not_endpart(RDI_PCState* ps);

  // private stand-alone helpers
  static void _fix_string(RDI_PCState* ps, char* s);
};

#endif  /* _RD_CONSTRAINT_H_  */
