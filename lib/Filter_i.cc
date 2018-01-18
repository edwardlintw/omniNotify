// -*- Mode: C++; -*-
//                              File      : Filter_i.cc
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
//    Implementation of Filter_i and MappingFilter_i
//

#include "corba_wrappers.h"
#include "RDILimits.h"
#include "CosNfyUtils.h"
#include "RDIRVMPool.h"
#include "RDIStringDefs.h"
#include "RDIUtil.h"
#include "RDIList.h"
#include "CosNotifyChannelAdmin_i.h"
#include "CosNotifyFilter_i.h"
#include "RDIOplocksMacros.h"

////////////////////////////////////////////////////////////////////

// A couple of sequence types used by filter impl
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(RDINotifySubscribe_ptr,      RDINotifySubscribeSeq);
WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNC::NotifySubscribe_ptr,  CosNotifySubscribeSeq);

////////////////////////////////////////////////////////////////////

ConstraintImpl* ConstraintImpl::create(const CosNF::ConstraintExp& constraint)
{
  ConstraintImpl* cimpl = new ConstraintImpl();
  RDI_AssertAllocThrowNo(cimpl, "Memory allocation failed for ConstraintImpl\n");
  if ( RDI_STR_EQ_I(constraint.constraint_expr, "true") ) {
    cimpl->just_types = 1;
    cimpl->node = 0;
  } else {
    cimpl->node = new RDI_PCState();
    RDI_AssertAllocThrowNo(cimpl->node, "Memory allocation failed for RDI_PCState\n");
    cimpl->just_types = 0;
    cimpl->node->parse_string(constraint.constraint_expr);
    if ( cimpl->node->e ) {
      RDIDbgFiltLog("Parsing error: " << cimpl->node->b << '\n');
      delete cimpl;
      return 0;
    }
    cimpl->node->r_ops->finalize();
  }
  return cimpl;
}

////////////////////////////////////////////////////////////////////

TW_Mutex         Filter_i::_classlock;
CORBA::Long      Filter_i::_classctr = 0;
RDIFilterKeyMap* Filter_i::_class_keymap = 
  new RDIFilterKeyMap(RDI_CorbaSLongHash, RDI_CorbaSLongRank,128,20);

#undef WHATFN
#define WHATFN "Filter_i::Filter_i"
Filter_i::Filter_i(const char* grammar, FilterFactory_i* factory) :
  _oplockptr(0), _disposed(0), _my_name(factory->L_my_name()),
  _factory(factory), _idcounter(0), _hashvalue(0),
  _constraint_grammar(CORBA_STRING_DUP(grammar)), 
  _constraints(0), _constraint_impls(0),
  _callback_serial(1), _callback_i_serial(1),
  _callbacks(RDI_CorbaULongHash, RDI_CorbaULongRank),
  _callbacks_i(RDI_CorbaULongHash, RDI_CorbaULongRank),
  _flt_dom_ev_types(RDI_EventType::hash, RDI_EventType::rank),
  _flt_all_ev_types(RDI_EventType::hash, RDI_EventType::rank)
{
  RDI_OPLOCK_INIT("filter");
#ifndef NDEBUG
  _dbgout = 1;
#endif
  _constraints      = new CosNF::ConstraintInfoSeq();
  _constraint_impls = new ConstraintImplSeq();
  RDI_AssertAllocThrowNo(_constraints, "Memory allocation failed - ConstraintInfoSeq\n");
  RDI_AssertAllocThrowNo(_constraint_impls, "Memory allocation failed - ConstraintImplSeq\n");
  _constraints->length(0);
  _constraint_impls->length(0);
  { // introduce filter class lock scope
    TW_SCOPE_LOCK(class_lock, _classlock, "filter_class", WHATFN);
      while (1) {
	// loop until we find a filter ID that is not in use
	++_classctr;
	if (_classctr < 0) { // wrapped
	  _classctr = 0;
	}
	_fid = _classctr;
	Filter_i *temp=0;
	_class_keymap->lookup(_fid, temp);
	if (temp == 0) {
	  // _fid is not in use
	  _class_keymap->insert(_fid, this);
	  break;
	}
      }
  } // end filter class lock scope
  char buf[20];
  sprintf(buf, "filter%ld", (long)_fid);
  _my_name.length(_my_name.length()+1);
  _my_name[_my_name.length()-1] = (const char*)buf;
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

Filter_i::~Filter_i()
{
  // RDIDbgForceLog("XXX_REMOVE Filter_i::~Filter_i called for filter with fid " << _fid << '\n');
  RDI_OPLOCK_DESTROY_CHECK("Filter_i");
}

#undef WHATFN
#define WHATFN "Filter_i::obj_gc_all"
void
Filter_i::obj_gc_all(RDI_TimeT curtime, CORBA::ULong deadFilter)
{
  RDIFilterKeyMapCursor   cursor;
  FilterPtrSeq            f_list;
  CosNF_FilterPtrSeq      f_reflist;
  CORBA::ULong            i, len = 0;
  { // introduce filter class lock scope
    TW_SCOPE_LOCK(class_lock, _classlock, "filter_class", WHATFN);
    f_list.length(_class_keymap->length()); // pay once now, rather than doing incremental buffer growth
    f_reflist.length(_class_keymap->length()); // ditto
    for ( cursor = _class_keymap->cursor(); cursor.is_valid(); ++cursor) {
      Filter_i* f1 = cursor.val();
      if (f1->_callbacks_i.length() == 0 && f1->_callbacks.length() == 0) {
	f_list[len]    = f1;
	// bump refcount of f1 so that its storage cannot be deallocated before we call obj_gc on it
	f_reflist[len] = WRAPPED_IMPL2OREF(CosNF::Filter, f1);
	len++;
      }
    }
  } // end filter class lock scope
  for (i = 0; i < len; i++) {
    f_list[i]->obj_gc(curtime, deadFilter);
    CORBA::release(f_reflist[i]);
  }
}

#undef WHATFN
#define WHATFN "Filter_i::obj_gc"
CORBA::Boolean
Filter_i::obj_gc(RDI_TimeT curtime, CORBA::ULong deadFilter)
{
  // note: only called when deadFilter > 0
  CORBA::Boolean res = 0;
  RDI_LocksHeld held = { 0 };
  { // introduce bump lock scope
    RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(filter_lock, held.filter, WHATFN);
    if (!held.filter) { return 0; }
    if (RDI_TIMET_LT_BY_SECS(_last_use, curtime, deadFilter)) {
      res = cleanup_and_dispose(held, 1, filter_lock.dispose_info); // 1 == only on cb zero
    }
  } // end bump scope
  if (res) {
    RDIDbgFiltLog("obj_gc called on filter " << (void*)this << " with fid " << _fid << " -- destroyed\n");
  } else {
    RDIDbgFiltLog("obj_gc called on filter " << (void*)this << " with fid " << _fid << " -- not destroyed\n");
  }
  return res;
}

#undef WHATFN
#define WHATFN "Filter_i::destroy"
void
Filter_i::destroy( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  { // introduce bump lock scope
    RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(filter_lock, held.filter, WHATFN);
    if (!held.filter) { RDI_THROW_INV_OBJREF; }
    RDIDbgFiltLog("destroy called on filter " << (void*)this << " with fid " << _fid << " \n");
    cleanup_and_dispose(held, 0, filter_lock.dispose_info);
  } // end bump scope
}

#undef WHATFN
#define WHATFN "Filter_i::destroy_i"
CORBA::Boolean
Filter_i::destroy_i(CORBA::Boolean only_on_cb_zero)
{
  CORBA::Boolean res;
  RDI_LocksHeld held = { 0 };
  { // introduce bump lock scope
    RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(filter_lock, held.filter, WHATFN);
    if (!held.filter) { return 0; }
    RDIDbgFiltLog("destroy_i called on filter " << (void*)this << " with fid " << _fid << " \n");
    res = cleanup_and_dispose(held, only_on_cb_zero, filter_lock.dispose_info);
  } // end bump scope
  return res;
}

#undef WHATFN
#define WHATFN "Filter_i::cleanup_and_dispose"
// caller should obtain bumped scope lock
CORBA::Boolean
Filter_i::cleanup_and_dispose(RDI_LocksHeld&            held,
			      CORBA::Boolean            only_on_cb_zero,
			      WRAPPED_DISPOSEINFO_PTR&  dispose_info)
{
  RDI_HashCursor<CosNF::CallbackID, RDINfyCB> curs;
  if (only_on_cb_zero && (_callbacks_i.length() || _callbacks.length())) {
    return 0; // not disposed
  }
  if (_disposed) {
    RDIDbgFiltLog("Filter_i::cleanup_and_dispose() called more than once\n");
    return 0; // not disposed by this call
  }
  _disposed = 1; // acts as a guard: the following is only executed by one thread

  // First, remove all constraints.  This will force notifications
  // to be sent and updates to take place at the TypeMap.
  _remove_all_constraints(held);

  // Remove filter from the global hash table used for mapping the
  // CosN::Filter objects to Filter_i objects
  { // introduce filter class lock scope
    TW_SCOPE_LOCK(class_lock, _classlock, "filter_class", WHATFN);
    _class_keymap->remove(_fid);
  } // end filter class lock scope

  // Notify all READY callback subscribers about the destruction 
  for (curs=_callbacks_i.cursor(); curs.is_valid(); ++curs) {
    curs.val().callback->filter_destroy_i(this);
  }

  CORBA_STRING_FREE(_constraint_grammar);
  if ( _constraints ) {
    delete _constraints;
  }
  if ( _constraint_impls ) {
    for (CORBA::ULong ix=0; ix < _constraint_impls->length(); ix++) {
      delete (*_constraint_impls)[ix];
      (*_constraint_impls)[ix] = 0;
    }
    delete _constraint_impls;
  }
  _constraints = 0;
  _constraint_impls = 0;
  _callbacks.clear();
  _callbacks_i.clear();
  _flt_dom_ev_types.clear();
  _flt_all_ev_types.clear();
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
  return 1; // disposed
} 

#undef WHATFN
#define WHATFN "Filter_i::add_constraints"
CosNF::ConstraintInfoSeq* 
Filter_i::add_constraints(const CosNF::ConstraintExpSeq& clist  WRAPPED_IMPLARG )
{
  RDI_LocksHeld       held = { 0 };
  CORBA::ULong        size = clist.length();
  CORBA::ULong        base = 0;
  CORBA::ULong        ix   = 0;
  CosN::EventTypeSeq  add_types;
  CosN::EventTypeSeq  del_types;
  CosN::EventTypeSeq  star_star;

  CosNF::ConstraintInfoSeq* const_res = new CosNF::ConstraintInfoSeq();
  ConstraintImpl** impl_cseq = new ConstraintImpl* [ size ];

  RDI_AssertAllocThrowNo(const_res, "Memory allocation failed - ConstraintInfoSeq\n");
  RDI_AssertAllocThrowNo(impl_cseq, "Memory allocation failed - ConstraintImplSeq\n");

  const_res->length(size);
  add_types.length(0);
  del_types.length(0);
  star_star.length(1);
  star_star[0].domain_name = CORBA_STRING_DUP("*");
  star_star[0].type_name   = CORBA_STRING_DUP("*");

  { // introduce bump lock scope for filter
    RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(filter_lock, held.filter, WHATFN);
    if (!held.filter) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
    _last_use.set_curtime();
#endif

    // To guarantee atomic modification of the filter object, we first 
    // parse the provided constraints and validate their expressions

    for ( ix = 0; ix < size; ix++ ) {
      if ( ! (impl_cseq[ix] = ConstraintImpl::create(clist[ix])) ) {
	delete const_res; 
	delete [] impl_cseq;
	throw CosNF::InvalidConstraint(clist[ix]);
      }
    }

    // Being here implies that all constraints were valid,  and we can
    // now install them in the filter.  First, we need to increase the
    // sizes of the sequences we use to track the constraints

    base = _constraints->length();
    _constraints->length(base + size);
    _constraint_impls->length(base + size);

    for ( ix = 0; ix < size; ix++ ) {
      (*_constraints)[base+ix].constraint_id = _idcounter;
      (*_constraints)[base+ix].constraint_expression.event_types =
	clist[ix].event_types.length()?clist[ix].event_types:star_star;
      (*_constraints)[base+ix].constraint_expression.constraint_expr =
	CORBA_STRING_DUP(clist[ix].constraint_expr);
      (*_constraint_impls)[base+ix] = impl_cseq[ix];
      _update_ev_tables( (*_constraints)[base+ix].constraint_expression, 
			 add_types, del_types);
      (*const_res)[ix].constraint_id = _idcounter++;
      (*const_res)[ix].constraint_expression.event_types = clist[ix].event_types;
      (*const_res)[ix].constraint_expression.constraint_expr = 
	CORBA_STRING_DUP(clist[ix].constraint_expr);
    }

    // Notify all internal entities about any updates in the event types
    // referenced in the constraints used by this filter -- this will in
    // turn update the TypeMap

    if ( add_types.length() || del_types.length() ) {
      notify_subscribers_i(held, add_types, del_types);
    }
  } // end bump lock scope
  delete [] impl_cseq;

  return const_res;
}

#undef WHATFN
#define WHATFN "Filter_i::modify_constraints"
void
Filter_i::modify_constraints(const CosNF::ConstraintIDSeq&   del_list,
			     const CosNF::ConstraintInfoSeq& mod_list  WRAPPED_IMPLARG )
{
  RDI_LocksHeld         held      = { 0 };
  ConstraintImpl**      impl_cseq = new ConstraintImpl* [ mod_list.length() ];
  CosN::EventTypeSeq    add_types;
  CosN::EventTypeSeq    del_types;
  CosN::EventTypeSeq    star_star;
  CORBA::ULong          indx, ix;

  RDI_AssertAllocThrowNo(impl_cseq, "Memory allocation failed for ConstraintImpl[]\n");

  add_types.length(0);
  del_types.length(0);
  star_star.length(1);
  star_star[0].domain_name = CORBA_STRING_DUP("*");
  star_star[0].type_name   = CORBA_STRING_DUP("*");

  { // introduce bump lock scope
    RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(filter_lock, held.filter, WHATFN);
    if (!held.filter) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
    _last_use.set_curtime();
#endif

    // To achieve the atomicity semantics stated in the specification, 
    // we need to make sure that all constraint IDs referenced in both
    // lists are valid and the constraint expressions are valid too

    for ( ix = 0; ix < del_list.length(); ix++ ) {
      if ( ! _exists_constraint(del_list[ix], indx) ) {
	delete [] impl_cseq;
	RDIDbgFiltLog("Invalid contraint ID " << del_list[ix] << '\n');
	throw CosNF::ConstraintNotFound(del_list[ix]);
      }
    }
    for ( ix = 0; ix < mod_list.length(); ix++ ) {
      const CosNF::ConstraintExp& const_expr = mod_list[ix].constraint_expression;
      if ( ! _exists_constraint(mod_list[ix].constraint_id, indx) ) {
	delete [] impl_cseq;
	RDIDbgFiltLog("Invalid contraint ID " << mod_list[ix].constraint_id << '\n');
	throw CosNF::ConstraintNotFound(mod_list[ix].constraint_id);
      }
      if ( ! (impl_cseq[ix] = ConstraintImpl::create(const_expr)) ) {
	delete [] impl_cseq;
	throw CosNF::InvalidConstraint(const_expr);
      }
    }

    // Being here implies that the provided arguments are valid. Thus,
    // we can go ahead and update the constrains. First, we remove any
    // constraints specified in 'del_list' and then we update those in
    // 'mod_list'

    for ( ix = 0; ix < del_list.length(); ix++ ) {
      RDIDbgFiltLog("Removing contraint " << del_list[ix] << '\n');
      _remove_constraint(del_list[ix], add_types, del_types);
    }

    for ( ix = 0; ix < mod_list.length(); ix++ ) {
      const CosNF::ConstraintExp& cexpr = mod_list[ix].constraint_expression;
      const CosNF::ConstraintID&  cstid = mod_list[ix].constraint_id;

      _remove_constraint(cstid, add_types, del_types);
      indx = _constraints->length();
      _constraints->length(indx + 1);
      (*_constraints)[indx].constraint_id = cstid;
      (*_constraints)[indx].constraint_expression.event_types = 
	cexpr.event_types.length()?cexpr.event_types:star_star;
      (*_constraints)[indx].constraint_expression.constraint_expr =
	CORBA_STRING_DUP(cexpr.constraint_expr);
      _update_ev_tables((*_constraints)[indx].constraint_expression, add_types, del_types);
      _constraint_impls->length(indx + 1);
      (*_constraint_impls)[indx] = impl_cseq[ix];
    }

    // Notify all internal entities about any updates in the event types
    // referenced in the constraints used by this filter -- this will in
    // turn update the TypeMap

    notify_subscribers_i(held, add_types, del_types);
  } // end bump lock scope
  delete [] impl_cseq;
}

#undef WHATFN
#define WHATFN "Filter_i::get_constraints"
CosNF::ConstraintInfoSeq*
Filter_i::get_constraints(const CosNF::ConstraintIDSeq& id_list  WRAPPED_IMPLARG )
{
  CORBA::ULong indx, spos, size=id_list.length();
  CosNF::ConstraintInfoSeq* clst = new CosNF::ConstraintInfoSeq();

  RDI_AssertAllocThrowNo(clst, "Memory allocation failed - CosNF::ConstraintInfoSeq object\n");
  clst->length(size);
  RDI_OPLOCK_SCOPE_LOCK(filter_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  for (indx = 0; indx < size; indx++) {
    if ( ! _exists_constraint(id_list[indx], spos) ) {
      delete clst;
      throw CosNF::ConstraintNotFound(id_list[indx]);
    }
    (*clst)[indx].constraint_id = 
      id_list[indx];
    (*clst)[indx].constraint_expression.event_types = 
      (*_constraints)[spos].constraint_expression.event_types;
    (*clst)[indx].constraint_expression.constraint_expr =
      (*_constraints)[spos].constraint_expression.constraint_expr;
  }
  return clst;
}

#undef WHATFN
#define WHATFN "Filter_i::get_all_constraints"
CosNF::ConstraintInfoSeq*
Filter_i::get_all_constraints( WRAPPED_IMPLARG_VOID )
{
  CosNF::ConstraintInfoSeq* clst=new CosNF::ConstraintInfoSeq();
  CORBA::ULong  indx, size=0;

  RDI_AssertAllocThrowNo(clst, "Memory allocation failed - CosNF::ConstraintInfoSeq object\n");
  RDI_OPLOCK_SCOPE_LOCK(filter_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  size = _constraints->length();
  clst->length(size);

  for ( indx = 0; indx < size; indx++ ) {
    (*clst)[indx].constraint_id = 
      (*_constraints)[indx].constraint_id;
    (*clst)[indx].constraint_expression.event_types = 
      (*_constraints)[indx].constraint_expression.event_types;
    (*clst)[indx].constraint_expression.constraint_expr =
      (*_constraints)[indx].constraint_expression.constraint_expr;
  }
  return clst;
}

#undef WHATFN
#define WHATFN "Filter_i::remove_all_constraints"
void
Filter_i::remove_all_constraints( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(filter_lock, held.filter, WHATFN);
  if (!held.filter) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _remove_all_constraints(held);
}

// does the real work; caller should obtain bumped scope lock
void
Filter_i::_remove_all_constraints(RDI_LocksHeld& held)
{
  CosNF::ConstraintIDSeq cstridseq;
  CosN::EventTypeSeq    add_types;
  CosN::EventTypeSeq    rem_types;
  CORBA::ULong     indx;

  // First, collect all constraint IDs into a list
  cstridseq.length( _constraints->length() );
  for ( indx = 0; indx < _constraints->length(); indx++ ) {
    cstridseq[indx] = (*_constraints)[indx].constraint_id;
  }

  // Next, iterate over this list and remove the constraints
  add_types.length(0);
  rem_types.length(0);
  for ( indx = 0; indx < cstridseq.length(); indx++ ) {
    _remove_constraint(cstridseq[indx], add_types, rem_types);
  }

  // Finally, notify subscriber about the event type changes 
  notify_subscribers_i(held, add_types, rem_types);
}

// This is the external attach_callback.
#undef WHATFN
#define WHATFN "Filter_i::attach_callback"
CosNF::CallbackID
Filter_i::attach_callback(CosNC::NotifySubscribe_ptr callback  WRAPPED_IMPLARG )
{
  CosNF::CallbackID cbkid;
  RDI_OPLOCK_SCOPE_LOCK(filter_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  cbkid = _callback_serial++;
  _callbacks.insert(cbkid, callback);
  return cbkid;
}

#undef WHATFN
#define WHATFN "Filter_i::detach_callback"
void
Filter_i::detach_callback(CosNF::CallbackID callbackID  WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(filter_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  _callbacks.remove(callbackID);
}

#undef WHATFN
#define WHATFN "Filter_i::get_callbacks"
CosNF::CallbackIDSeq*
Filter_i::get_callbacks( WRAPPED_IMPLARG_VOID )
{
  RDI_HashCursor<CosNF::CallbackID, CosNC::NotifySubscribe_ptr> curs;
  CosNF::CallbackIDSeq* cb_list = new CosNF::CallbackIDSeq();
  CORBA::ULong indx = 0;

  RDI_AssertAllocThrowNo(cb_list, "Memory allocation failed - CosNF::CallbackIDSeq object\n");
  RDI_OPLOCK_SCOPE_LOCK(filter_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  cb_list->length(_callbacks.length());
  for ( curs = _callbacks.cursor(); curs.is_valid(); ++curs ) {
    (*cb_list)[indx++] = curs.key();
    indx += 1;
  }
  return cb_list;
}

// We do not perform any lookups in the following method since we 
// assume that it is called only once by a given admin or proxy object

#undef WHATFN
#define WHATFN "Filter_i::attach_callback_i"
CosNF::CallbackID
Filter_i::attach_callback_i(RDI_LocksHeld&          held,
			    RDINotifySubscribe_ptr  callback,
			    CORBA::Boolean          need_schange)
{
  RDINfyCB                              cb = { callback, need_schange };
  RDI_HashCursor<RDI_EventType, void *> curs;
  CosN::EventTypeSeq                    add_types;
  CosN::EventTypeSeq                    del_types;
  CosNF::CallbackID                     cbkid = 0;
  CORBA::ULong                          indx  = 0;

  { // introduce filter lock scope
    RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(filter_lock, held.filter, WHATFN);
    if (!held.filter) { RDI_THROW_INV_OBJREF; }
#ifndef NO_OBJ_GC
    _last_use.set_curtime();
#endif

    if ((cbkid = _callback_i_serial++) == 0) { // do not hand out zero as an id 
      cbkid = _callback_i_serial++;
    }
    _callbacks_i.insert(cbkid, cb);
    if (need_schange) {
      // We need to invoke the callback with the event types present
      // in the dominating event types list for this filter ........
      add_types.length(_flt_dom_ev_types.length());
      del_types.length(0);
      for (curs = _flt_dom_ev_types.cursor(); curs.is_valid(); ++curs) {
	add_types[indx].domain_name = curs.key().domain_name;
	add_types[indx++].type_name = curs.key().type_name;
      }
    }
  }
  // end filter lock scope
  //   (do not hold filter lock across calls to propagate_schange)
  if (need_schange) {
    callback->propagate_schange(held, add_types, del_types, this);
  }
  return cbkid;
}

#undef WHATFN
#define WHATFN "Filter_i::detach_callback_i"
void
Filter_i::detach_callback_i(CosNF::CallbackID  callbackID)
{
  RDI_OPLOCK_SCOPE_LOCK(filter_lock, WHATFN, return);

  if (_callbacks_i.length()) {
    _callbacks_i.remove(callbackID);
    if (_callbacks_i.length() == 0) {
      _last_detach.set_curtime();
    }
  }
}

// This filter is being removed from an fadmin.
// If callbackID is non-zero we need to unregister the callback.
// If filter_holder is non_NULL we need to invoke that holder's
// propagate_schange method one last time to inform it
// about the types that are being 'lost' for this filter.
#undef WHATFN
#define WHATFN "Filter_i::fadmin_removal_i"
void
Filter_i::fadmin_removal_i(RDI_LocksHeld&          held,
			   CosNF::CallbackID       callbackID,
			   RDINotifySubscribe_ptr  filter_holder)
{
  RDI_HashCursor<RDI_EventType, void *> curs;
  CORBA::ULong cntr = 0;
  CosN::EventTypeSeq addseq;
  CosN::EventTypeSeq delseq;

  // RDIDbgForceLog("XXX_REMOVE " << WHATFN << " called, callbackID " << callbackID << " filter_holder " << (void*)filter_holder);
  { // introduce filter lock scope
    RDI_OPLOCK_COND_SCOPE_LOCK_TRACK(filter_lock, held.filter, WHATFN);
    if (!held.filter) { RDI_THROW_INV_OBJREF; }

    /* remove callback */
    if (callbackID && _callbacks_i.length()) {
      _callbacks_i.remove(callbackID);
      if (_callbacks_i.length() == 0) {
	_last_detach.set_curtime();
      }
    }

    if (filter_holder) {
      addseq.length(0);
      delseq.length( _flt_dom_ev_types.length() );
      for ( curs = _flt_dom_ev_types.cursor(); curs.is_valid(); ++curs ) {
	delseq[cntr].domain_name = CORBA_STRING_DUP(curs.key().domain_name);
	delseq[cntr++].type_name = CORBA_STRING_DUP(curs.key().type_name);
      }
    }
  } // end filter lock scope
  //   (do not hold filter lock across calls to propagate_schange)
  if (filter_holder) {
    filter_holder->propagate_schange(held, addseq, delseq, this);
  }
}

#undef WHATFN
#define WHATFN "Filter_i::notify_subscribers_i"
void
Filter_i::notify_subscribers_i(RDI_LocksHeld&             held,
			       const CosN::EventTypeSeq&  add_seq, 
			       const CosN::EventTypeSeq&  del_seq)
{
  RDI_HashCursor<CosNF::CallbackID, RDINfyCB>                     curs1;
  RDI_HashCursor<CosNF::CallbackID, CosNC::NotifySubscribe_ptr>   curs2;
  CosN::EventTypeSeq new_add_seq;
  CosN::EventTypeSeq new_del_seq;
  CORBA::ULong ix, iz, sz;

  new_add_seq.length(0);
  new_del_seq.length(0);

  // The two lists may contain common entries. Before we notify all
  // registered entities about the changes in the subscription,  we
  // clean up the two lists first

  for (ix=0; ix < add_seq.length(); ix++) {
    for (iz=0; iz < del_seq.length(); iz++) {
      if ( RDI_STR_EQ(add_seq[ix].type_name, del_seq[iz].type_name) &&
	   RDI_STR_EQ(add_seq[ix].domain_name, del_seq[iz].domain_name) ) {
	break;
      }
    }
    if ( iz == del_seq.length() ) {
      sz = new_add_seq.length();
      new_add_seq.length(sz + 1);
      new_add_seq[sz] = add_seq[ix];
    }
  }

  for (ix=0; ix < del_seq.length(); ix++) {
    for (iz=0; iz < add_seq.length(); iz++) {
      if ( RDI_STR_EQ(del_seq[ix].type_name, add_seq[iz].type_name) &&
	   RDI_STR_EQ(del_seq[ix].domain_name, add_seq[iz].domain_name) ) { 
	break;
      }
    }
    if ( iz == add_seq.length() ) {
      sz = new_del_seq.length();
      new_del_seq.length(sz + 1);
      new_del_seq[sz] = del_seq[ix]; 
    }
  }

  if ( new_add_seq.length() != 0 || new_del_seq.length() != 0 ) {
    // Copy the contents of _callbacks_i and _callbacks so that we can release
    // the filter lock (cannot traverse _callbacks_i and _callbacks
    // without holding the lock).
    unsigned int i, act_i_len = 0;
    unsigned int i_len = _callbacks_i.length();
    unsigned int x_len = _callbacks.length();
    RDINotifySubscribeSeq i_seq;
    CosNotifySubscribeSeq x_seq;
    i_seq.length(i_len);
    x_seq.length(x_len);
    for (i = 0, curs1 = _callbacks_i.cursor(); curs1.is_valid(); ++i, ++curs1) {
      RDINfyCB &cb = curs1.val();
      if (cb.need_schange) {
	i_seq[act_i_len++] = cb.callback;
      }
    }
    for (i = 0, curs2 = _callbacks.cursor(); curs2.is_valid(); ++i, ++curs2) {
      x_seq[i] = curs2.val();
    }
    // Temporarily release the bumped filter scope lock so that it is not held
    // across calls to propagate_schange / subscription_change
    {
      RDI_OPLOCK_SCOPE_RELEASE_TRACK(held.filter, WHATFN);
      for (i = 0; i < act_i_len; i++) {
	i_seq[i]->propagate_schange(held, new_add_seq, new_del_seq, this);
      }
      for (i = 0; i < x_len; i++) {
	x_seq[i]->subscription_change(new_add_seq, new_del_seq);
      }
    } // end of release scope
  }
}

////////////////////////////////////////////////////////////////////

CORBA::Boolean
Filter_i::match(const CORBA::Any& a  WRAPPED_IMPLARG )
{
  return match_chan(a, 0);
}

CORBA::Boolean
Filter_i::match_chan(const CORBA::Any& a, EventChannel_i* channel)
{
  // construct event rev with "%ANY" wrapper, a as remainder_of_body
  RDI_StructuredEvent* rev = new RDI_StructuredEvent(a);
  // match against rev
  CORBA::Boolean res = 0;
  try { res = rdi_match(rev, channel); }
  catch (...) { throw; }
  delete rev;
  return res;
}

CORBA::Boolean
Filter_i::match_structured(const CosN::StructuredEvent& event  WRAPPED_IMPLARG )
{
  return match_structured_chan(event, 0);
}

CORBA::Boolean
Filter_i::match_structured_chan(const CosN::StructuredEvent& event, EventChannel_i* channel)
{
  RDI_StructuredEvent* rev = new RDI_StructuredEvent(event);
  CORBA::Boolean res = 0;
  try { res = rdi_match(rev, channel); }
  catch (...) { throw; }
  delete rev;
  return res;
}

CORBA::Boolean
Filter_i::match_typed(const CosN::PropertySeq& event  WRAPPED_IMPLARG )
{
  return match_typed_chan(event, 0);
}

CORBA::Boolean
Filter_i::match_typed_chan(const CosN::PropertySeq& event, EventChannel_i* channel)
{
  RDIDbgForceLog("Warning: match_typed not implemented yet -- filter always fails\n");
  return 0;
}

#undef WHATFN
#define WHATFN "Filter_i::rdi_match"
CORBA::Boolean
Filter_i::rdi_match(RDI_StructuredEvent* se, EventChannel_i* channel) 
{
  RDI_RVM        rvm; // placing on stack more efficient than alloc
  unsigned int   num     = 0;

  int filter_held = 0;
  if ( channel ) channel->incr_num_rdi_match();

  RDI_OPLOCK_SCOPE_LOCK_TRACK(filter_lock, filter_held, WHATFN);
  if (!filter_held) {
    RDIDbgForceLog("XXX SHOULD_NOT_HAPPEN Filter_i::rdi_match called on destroyed filter " << (void*)this);
    return 0;
  }
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif

  { // introduce se lock scope

    // Lock the event so that only one thread evaluates the event
    // at a time.  This may be too restrictive in some cases and,
    // hence,  we should either use different copies of the event
    // or synchronize the extraction from Any/DynAny only .......

    RDI_SEVENT_SCOPE_LOCK(event_lock, se, WHATFN);
    
#ifndef NDEBUG
    if (RDIDbgFilt) {
      RDIDbgLogger(l, RDIDbgFilt_nm);
      l.str << "rdi_match called for filter " << _fid << " event " << (void*)se << '\n';
      if ( _dbgout ) {
	l.str << *this << '\n';
      }
      _dbgout=0;
    }
#endif

    // In case the filter does not have any constraints attached
    // to it, we assume that the filter evaluates to TRUE ......

    if ( ! _constraint_impls || ! _constraint_impls->length() ) {
      RDIDbgFiltLog("No constraints -- filter eval returns TRUE\n");
      return 1;
    }

    for ( CORBA::ULong idx = 0; idx < _constraints->length(); idx++ ) {
      RDIDbgFiltLog("Eval constraint at idx " << idx << '\n');
      CORBA::Boolean isdominated = 0;
      const CosN::EventTypeSeq& types = 
	(*_constraints)[idx].constraint_expression.event_types;
      // Need to check if the event type is dominated by the event
      // types, if any, specified in the constraint

      isdominated = 0;
      for (CORBA::ULong i = 0; i < types.length() ; i++) {
	if ( RDI_EventType::dominates(types[i], se->get_type()) ) {
	  isdominated = 1;
	  break;
	}
      }

      if ( ! isdominated )
	continue;

      if ( (*_constraint_impls)[idx]->just_types ) {
	RDIDbgFiltLog("Dominating event type and no constraint -- filter eval returns TRUE\n");
	return 1;
      }

      while ( num <= 3 ) {
	try {
	  rvm.init((*_constraint_impls)[idx]->node->r_ops);
	  rvm.cexprs = 
	    (*_constraints)[idx].constraint_expression.constraint_expr;
	  rvm.eval(se);
	  if ( channel )
	    channel->incr_num_rvm_eval();
	  break;
	} 
	catch (...) {
	  RDIDbgForceLog("XXX exception in rvm.init or rvm.eval: " << num << '\n');
	  if ( num == 0 ) {
	    CosNF::ConstraintExp& cxpr= (*_constraints)[idx].constraint_expression;
	    RDIDbgForceLog("XXX constraint "<<(const char*)(cxpr.constraint_expr) << '\n');
	  }
	  if ( ++num == 3 ) {
	    rvm.r_code = RDI_RTRet_OK;
	    rvm.r_bool = 0;
	    break;
	  }
	}
      }

      if (rvm.r_code == RDI_RTRet_OK) {
	if ( rvm.r_bool ) {
	  RDIDbgFiltLog("Constraint eval returned OK -- filter eval returns TRUE\n");
	  return 1;
	}
      } else {
	// We interpret non OK value as false, not as an exception 
	// and, thus, we do nothing - we actually return true only 
	// if the r_bool is true AND the return code is OK. In the
	// future we might want to distinguish some of these codes
	// and do something more useful with this information.
      }
      RDIDbgFiltLog("Constraint eval not OK -- filter eval continues\n");
    }
    RDIDbgFiltLog("No more constraints -- filter eval returns FALSE\n");

  } // end se lock scope 
  return 0;
}

////////////////////////////////////////////////////////////////////

CORBA::Boolean
Filter_i::_exists_constraint(const CosNF::ConstraintID& cstid,
			     CORBA::ULong& position) const
{
  for (unsigned int ix = 0; ix < _constraints->length(); ix++) {
    if ( (*_constraints)[ix].constraint_id == cstid ) {
      position = ix;
      return 1;
    }
  }
  return 0;
}

// Update the tables that maintain information about the event types
// which are referenced in the constraints of the filter.  As a side
// effect,  compute information about dominating event types and any
// updates that resulted from the addition of the new constraint ...

void
Filter_i::_update_ev_tables(const CosNF::ConstraintExp& cexpr,
			    CosN::EventTypeSeq& add_ev_types,
			    CosN::EventTypeSeq& rem_ev_types)
{
  RDI_HashCursor<RDI_EventType, void *> curs;
  RDI_EventType etype;
  CORBA::ULong  value=0;
  CosN::EventTypeSeq deltypes;

  // We need to keep track of the event types that are removed from
  // the dominating event type table.  We do not use 'rem_ev_types'
  // since this list may not be empty.
  deltypes.length(0);

  for (unsigned int ix=0; ix < cexpr.event_types.length(); ix++) {
    etype = cexpr.event_types[ix];
    // If this event type has been used in a previous constraint,
    // increment its reference counter and continue with next one
    if ( _flt_all_ev_types.lookup(etype, value) ) {
      _flt_all_ev_types.replace(etype, ++value);
      continue;
    }

    // New event type: insert it into the appropriate hash table,
    // and check if it is dominated by an existing event type
    value = 1;
    if ( _flt_all_ev_types.insert(etype, value) ) {
      RDI_Fatal("Failed to upadate event type hash table\n");
    }
    if ( _event_is_dominated(etype) )
      continue;

    CORBA::Boolean dstar = RDI_STR_EQ(etype.domain_name, "*") ? 1 : 0;
    CORBA::Boolean tstar = RDI_STR_EQ(etype.type_name, "*") ? 1 : 0;

    if ( dstar && tstar ) {		// Case 1: "*::*"
      // All existing entries are removed from the dominating list
      for (curs=_flt_dom_ev_types.cursor(); curs.is_valid(); ++curs) {
	_add_ev_type(deltypes, curs.key());
      }
    } else if ( dstar ) {		// Case 2: " *::T"
      // Entries with 'domain_name!=* && type_name==T' are removed
      for (curs=_flt_dom_ev_types.cursor(); curs.is_valid(); ++curs) {
	if ( RDI_STR_NEQ(curs.key().domain_name, "*") &&
	     RDI_STR_EQ(curs.key().type_name, etype.type_name) ) {
	  _add_ev_type(deltypes, curs.key());
	}
      }
    } else if ( tstar ) { 		// Case 3: "D::*"
      // Entries with 'domain_name==D && type_name!=*' are removed
      for (curs=_flt_dom_ev_types.cursor(); curs.is_valid(); ++curs) {
	if ( RDI_STR_EQ(curs.key().domain_name, etype.domain_name) &&
	     RDI_STR_NEQ(curs.key().type_name, "*") ) {
	  _add_ev_type(deltypes, curs.key());
	}
      }
    }

    // Remove all event type entries that are present in the list
    // containing the deleted dominating event types and add this
    // event type to the dominating list and the added list
    for ( unsigned int ix=0; ix < deltypes.length(); ix++) {
      _flt_dom_ev_types.remove(deltypes[ix]);
      _add_ev_type(rem_ev_types, deltypes[ix]);
    }
    _flt_dom_ev_types.insert(etype, 0);
    _add_ev_type(add_ev_types, etype);
  }
}

// Remove an existing constraint from the filter. The removal of the
// constraint may result in the update of the dominating event types
// table.  The 'add_ev_types' and  'rem_ev_types' sequences are used
// to record changes that occur at the dominating event types table.

void
Filter_i::_remove_constraint(const CosNF::ConstraintID& cstrid,
			     CosN::EventTypeSeq& add_ev_types,
			     CosN::EventTypeSeq& rem_ev_types)
{
  RDI_HashCursor<RDI_EventType, CORBA::ULong> acurs;
  CosN::EventTypeSeq deltypes;
  CosN::EventTypeSeq null_evseq;
  RDI_EventType etype;
  unsigned int  cix, idx;
  CORBA::ULong  rfctr=0;
  CORBA::ULong  numet=0;
  void*         nullv=0;
  CORBA::Boolean dstar=0;
  CORBA::Boolean tstar=0;

  // Locate the entry of the constraint into the constraint list
  for (cix = 0; cix < _constraints->length(); cix++) {
    if ( (*_constraints)[cix].constraint_id == cstrid )
      break;
  }
  RDI_Assert((cix != _constraints->length()), "Corrupted data structures\n");
  deltypes.length(0);
  null_evseq.length(0);

  // Remove the event types referenced in the removed constraint
  // from '_flt_all_ev_types'.  If the deletion of an event type
  // from this table triggers the removal of the event type from
  // '_flt_dom_ev_types', add the event type to 'deltypes'

  numet = (*_constraints)[cix].constraint_expression.event_types.length();
  for (idx = 0; idx < numet; idx++) {
    etype = (*_constraints)[cix].constraint_expression.event_types[idx];
    if ( ! _flt_all_ev_types.lookup(etype, rfctr) ) {
      RDI_Fatal("Corrupted data structures -- did not find entry\n");
    }
    if ( --rfctr > 0 ) {
      _flt_all_ev_types.replace(etype, rfctr);
      continue;
    } else {
      _flt_all_ev_types.remove(etype);
      if ( _flt_dom_ev_types.lookup(etype, nullv) ) {
	_flt_dom_ev_types.remove(etype);
	_add_ev_type(deltypes, etype);
      }
    }
  }

  // If the 'deltypes' list is not empty,  we may need to update
  // the event types in '_flt_dom_ev_types'.  Updates take place
  // only when "*::*", "*::T",  or "D::*" belongs to 'deltypes'.
  // NOTE: if "*::*" belongs to 'deltypes', no other type should
  //       exist due to the construction of '_flt_dom_ev_types'.

  for ( idx = 0; idx < deltypes.length(); idx++) {
    etype = deltypes[idx];
    dstar = RDI_STR_EQ(etype.domain_name, "*") ? 1 : 0;
    tstar = RDI_STR_EQ(etype.type_name, "*") ? 1 : 0;

    if ( dstar && tstar ) {
      // First we will add all "D::*" and "*::T" types
      // and then the dominating "D::T" types
      acurs = _flt_all_ev_types.cursor();
      while ( acurs.is_valid() ) {
	if ( RDI_STR_EQ(acurs.key().type_name, "*") ||
	     RDI_STR_EQ(acurs.key().domain_name, "*") ) {
	  _flt_dom_ev_types.insert(acurs.key(), 0);
	  _add_ev_type(add_ev_types, acurs.key());
	}
	acurs++;
      }
      acurs = _flt_all_ev_types.cursor();
      while ( acurs.is_valid() ) {
	if ( RDI_STR_NEQ(acurs.key().type_name, "*") &&
	     RDI_STR_NEQ(acurs.key().domain_name, "*") &&
	     ! _event_is_dominated(acurs.key()) ) {
	  _flt_dom_ev_types.insert(acurs.key(), 0);
	  _add_ev_type(add_ev_types, acurs.key());
	}
	acurs++;
      }
    } else if ( dstar ) {
      // Add all "D::type_name" event type entries
      acurs = _flt_all_ev_types.cursor();
      while ( acurs.is_valid() ) {
	if ( RDI_STR_EQ(acurs.key().type_name,etype.type_name) ) {
	  _flt_dom_ev_types.insert(acurs.key(), 0);
	  _add_ev_type(add_ev_types, acurs.key());
	}
	acurs++;
      }
    } else if ( tstar ) {
      // Add all "domain_name::T" event type entries
      acurs = _flt_all_ev_types.cursor();
      while ( acurs.is_valid() ) {
	if ( RDI_STR_EQ(acurs.key().domain_name,etype.domain_name) ) {
	  _flt_dom_ev_types.insert(acurs.key(), 0);
	  _add_ev_type(add_ev_types, acurs.key());
	}
	acurs++;
      }
    }

    _add_ev_type(rem_ev_types, etype);
  }

  // Remove the entries that corresponds to the deleted constraint 
  // from the two constraint lists -- if the deleted constraint is
  // not the last entry in these lists, we ``move'' the last entry
  // into the location of the constraint.

  idx = _constraints->length() - 1;
  if ( cix != idx ) {
    (*_constraints)[cix].constraint_id = 
      (*_constraints)[idx].constraint_id;
    (*_constraints)[cix].constraint_expression.event_types =
      (*_constraints)[idx].constraint_expression.event_types;
    (*_constraints)[cix].constraint_expression.constraint_expr =
      (*_constraints)[idx].constraint_expression.constraint_expr;
    delete (*_constraint_impls)[cix];
    (*_constraint_impls)[cix] = (*_constraint_impls)[idx];
    (*_constraint_impls)[idx] = 0;
  } else {
    delete (*_constraint_impls)[cix];
    (*_constraint_impls)[cix] = 0;
  }

  // Null-out the last entry of the constraint sequence to avoid 
  // holding references to unused objects

  (*_constraints)[idx].constraint_expression.event_types = null_evseq;
  (*_constraints)[idx].constraint_expression.constraint_expr = (const char*)"";

  _constraints->length(idx);
  _constraint_impls->length(idx);
}

// Check if the given event type is dominated by at least one entry
// in the dominating event type table '_flt_dom_ev_types'

CORBA::Boolean
Filter_i::_event_is_dominated(const CosN::EventType& etype)
{
  RDI_HashCursor<RDI_EventType, void *> curs;
  for (curs = _flt_dom_ev_types.cursor(); curs.is_valid(); ++curs) {
    if ( RDI_EventType::dominates(curs.key(), etype) )
      return 1;
  }
  return 0;
}

void
Filter_i::_add_ev_type(CosN::EventTypeSeq& type_seq, 
		       const RDI_EventType& type)
{
  CORBA::ULong indx = type_seq.length();
  type_seq.length(indx + 1);
  type_seq[indx] = type;
}

#undef WHATFN
#define WHATFN "Filter_i::Filter2Filter_i"
Filter_i*
Filter_i::Filter2Filter_i(CosNF::Filter_ptr f)
{
  Filter_i* res = 0;
  AttN::Filter_var fvar = AttN::Filter::_narrow(f);
  if (CORBA::is_nil(fvar)) {
    return res; // not a Filter_i
  }
  { // introduce filter class lock scope
    TW_SCOPE_LOCK(class_lock, _classlock, "filter_class", WHATFN);
    _class_keymap->lookup(fvar->MyFID(), res);
  } // end filter class lock scope
  return res;
}

////////////////////////////////////////////////////////////////////

TW_Mutex         MappingFilter_i::_classlock;
CORBA::Long      MappingFilter_i::_classctr = 0;

// THIS CLASS NOT SUPPORTED YET
#undef WHATFN
#define WHATFN "MappingFilter_i::MappingFilter_i"
MappingFilter_i::MappingFilter_i(const char* grammar, const CORBA::Any& value,
				 FilterFactory_i* factory) :
  _oplockptr(0), _disposed(0), _my_name(factory->L_my_name()),
  _constraint_grammar(CORBA_STRING_DUP(grammar)), _def_value(value)
{ 
  char buf[30];
  { // introduce mappingfilter class lock scope
    TW_SCOPE_LOCK(class_lock, _classlock, "mappingfilter_class", WHATFN);
    _classctr++;
    sprintf(buf, "mapfilter%ld", (long)_classctr);
  } // end mappingfilter class lock scope
  _my_name.length(_my_name.length()+1);
  _my_name[_my_name.length()-1] = (const char*)buf;
  RDI_OPLOCK_INIT("mapfilter");
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

MappingFilter_i::~MappingFilter_i()
{
  RDI_OPLOCK_DESTROY_CHECK("MappingFilter_i");
}

#undef WHATFN
#define WHATFN "MappingFilter_i::destroy"
void
MappingFilter_i::destroy( WRAPPED_IMPLARG_VOID )
{
  RDI_LocksHeld held = { 0 };
  { // introduce bump lock scope
    RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(filter_lock, held.filter, WHATFN);
    if (!held.filter) { RDI_THROW_INV_OBJREF; }
    RDIDbgFiltLog("destroy called on mapping filter with fid XXX \n");
    cleanup_and_dispose(held, 0, filter_lock.dispose_info);
  } // end bump scope lock
}

#undef WHATFN
#define WHATFN "MappingFilter_i::cleanup_and_dispose"
// caller should obtain bumped scope lock
CORBA::Boolean
MappingFilter_i::cleanup_and_dispose(RDI_LocksHeld&            held,
				     CORBA::Boolean            only_on_cb_zero,
				     WRAPPED_DISPOSEINFO_PTR&  dispose_info)
{
  // NOT SUPPORTED YET
  if (_disposed) {
    RDIDbgFiltLog("MappingFilter_i::cleanup_and_dispose() called more than once\n");
    return 0; // not disposed by this call
  }
  _disposed = 1; // acts as a guard: the following is only executed by one thread
  CORBA_STRING_FREE(_constraint_grammar);
  RDI_OPLOCK_SET_DISPOSE_INFO(dispose_info);
  return 1; // disposed
}

CosNF::MappingConstraintInfoSeq*
MappingFilter_i::add_mapping_constraints(const CosNF::MappingConstraintPairSeq& pair_list)
{
  RDI_THROW_INV_OBJREF; // NOT SUPPORTED YET
  return 0;
}

void
MappingFilter_i::modify_mapping_constraints(const CosNF::ConstraintIDSeq& del_list,
					    const CosNF::MappingConstraintInfoSeq& mod_list  WRAPPED_IMPLARG )
{ 
  RDI_THROW_INV_OBJREF; // NOT SUPPORTED YET
}

CosNF::MappingConstraintInfoSeq*
MappingFilter_i::get_mapping_constraints(const CosNF::ConstraintIDSeq& id_list  WRAPPED_IMPLARG )
{ 
  RDI_THROW_INV_OBJREF; // NOT SUPPORTED YET
  return 0;
}

CosNF::MappingConstraintInfoSeq*
MappingFilter_i::get_all_mapping_constraints( WRAPPED_IMPLARG_VOID )
{ 
  RDI_THROW_INV_OBJREF; // NOT SUPPORTED YET
  return 0;
}

void
MappingFilter_i::remove_all_mapping_constraints( WRAPPED_IMPLARG_VOID )
{
  RDI_THROW_INV_OBJREF; // NOT SUPPORTED YET
}

CORBA::Boolean
MappingFilter_i::match(const CORBA::Any& event, 
		       WRAPPED_OUTARG_TYPE(CORBA::Any) result_to_set  WRAPPED_IMPLARG )
{
  return 0;  // NOT SUPPORTED YET
}

CORBA::Boolean
MappingFilter_i::match_structured(const CosN::StructuredEvent& event,
				  WRAPPED_OUTARG_TYPE(CORBA::Any) result_to_set  WRAPPED_IMPLARG )
{
  return 0;  // NOT SUPPORTED YET
}


CORBA::Boolean
MappingFilter_i::match_typed(const CosN::PropertySeq& event,
			     WRAPPED_OUTARG_TYPE(CORBA::Any) result_to_set  WRAPPED_IMPLARG )
{
  return 0;  // NOT SUPPORTED YET
}

////////////////////////////////////////
// Logging

RDIstrstream&
Filter_i::log_output(RDIstrstream& str) const
{
  CosNF::ConstraintInfoSeq& cons = *_constraints;
  ConstraintImplSeq&  conimpls  = *_constraint_impls;

  str << "[" << _my_name << "] #constraints = " << cons.length() << '\n';
  for (unsigned int i = 0; i < cons.length(); i++) {
    str << "  Constraint ";
    str.setw(5); str << cons[i].constraint_id << 
      " Types " << cons[i].constraint_expression.event_types << '\n';
    if ( conimpls[i]->just_types ) {
      str << "\tJUST_TYPES (cexpr: TRUE)\n";
    } else {
      str << "\tExpression: " << 
	(const char*)(cons[i].constraint_expression.constraint_expr) << 
	'\n';
    }
  }
  return str;
}

////////////////////////////////////////
// Interactive

#undef WHATFN
#define WHATFN "Filter_i::my_name"
AttN::NameSeq*
Filter_i::my_name( WRAPPED_IMPLARG_VOID ) 
{
  RDI_OPLOCK_SCOPE_LOCK(filter_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  AttN::NameSeq* res = new AttN::NameSeq(_my_name);
  if ( res == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  return res;
}

AttN::NameSeq*
Filter_i::child_names( WRAPPED_IMPLARG_VOID ) 
{
  RDI_OPLOCK_SCOPE_LOCK(filter_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  AttN::NameSeq* names = new AttN::NameSeq;
  if ( names == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  names->length(0);
  return names;
}

AttN::IactSeq*
Filter_i::children(CORBA::Boolean only_cleanup_candidates  WRAPPED_IMPLARG ) 
{
  RDI_OPLOCK_SCOPE_LOCK(filter_lock, WHATFN, RDI_THROW_INV_OBJREF);
#ifndef NO_OBJ_GC
  _last_use.set_curtime();
#endif
  AttN::IactSeq* ren = new AttN::IactSeq;
  if ( ren == (AttN::IactSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  ren->length(0);
  return ren;
}

CORBA::Boolean
Filter_i::safe_cleanup( WRAPPED_IMPLARG_VOID ) 
{
  CORBA::Boolean   res;
  RDI_LocksHeld    held = { 0 };

  { // introduce bump lock scope
    RDI_OPLOCK_BUMP_SCOPE_LOCK_TRACK(filter_lock, held.filter, WHATFN);
    if (!held.filter) { return 0; }
    res = cleanup_and_dispose(held, 1, filter_lock.dispose_info); // 1 means only dispose if there are no callbacks
  } // end bump lock scope
  return res;
}

// static class functions

#undef WHATFN
#define WHATFN "Filter_i::all_children"
AttN::IactSeq*
Filter_i::all_children( CORBA::Boolean only_cleanup_candidates ) 
{
  TW_SCOPE_LOCK(class_lock, _classlock, "filter_class", WHATFN);
  AttN::IactSeq* ren = new AttN::IactSeq;
  if ( ren == (AttN::IactSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  ren->length(0);
  RDIFilterKeyMapCursor cursor;
  unsigned int idx = 0;
  for ( cursor = _class_keymap->cursor(); cursor.is_valid(); ++cursor) {
    Filter_i* f = cursor.val();
    if (!only_cleanup_candidates || (f->_callbacks_i.length() == 0 && f->_callbacks.length() == 0)) {
      idx++;
      ren->length(idx);
      (*ren)[idx-1] = WRAPPED_IMPL2OREF(AttN::Interactive, f);
    }
  }
  return ren;
}

#undef WHATFN
#define WHATFN "Filter_i::all_filter_names"
AttN::NameSeq* 
Filter_i::all_filter_names() 
{
  TW_SCOPE_LOCK(class_lock, _classlock, "filter_class", WHATFN);
  AttN::NameSeq* names = new AttN::NameSeq;
  if ( names == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  names->length(_class_keymap->length());
  RDIFilterKeyMapCursor cursor;
  unsigned int idx = 0;
  for ( cursor = _class_keymap->cursor(); cursor.is_valid(); ++cursor) {
    char buf[20];
    sprintf(buf, "filter%ld", (long)cursor.key());
    (*names)[idx++] = (const char*)buf;
  }
  return names;
}

#undef WHATFN
#define WHATFN "Filter_i::find_filter"
Filter_i* 
Filter_i::find_filter(const char* fname) 
{
  if ((RDI_STRLEN(fname) < 7) || (! RDI_STRN_EQ_I(fname, "filter", 6))) {
    return 0; // invalid filter name
  }
  const char* fidstr = fname + 6;
  char* delim  = 0;
  CORBA::Long fid = (CORBA::Long)RDI_STRTOL(fidstr, &delim);
  if ( (delim == 0) || (delim == fidstr) || (*delim != '\0') ) {
    return 0; // invalid filter name
  }
  // do lookup on fid
  Filter_i* res = 0;
  { // introduce filter class lock scope
    TW_SCOPE_LOCK(class_lock, _classlock, "filter_class", WHATFN);
    _class_keymap->lookup(fid, res);
  } // end filter class lock scope
  return res;
}

#if 0
// XXX_OBSOLETE ???
#undef WHATFN
#define WHATFN "Filter_i::cleanup_zero_cbs"
void
Filter_i::cleanup_zero_cbs(RDIstrstream& str) 
{
  RDIFilterKeyMapCursor cursor;
  CORBA::ULong len = 0, num_destroyed = 0;
  str << "\nDestroying all filters not attached to proxy or admin\n";
  CosNF::FilterIDSeq fids;
  { // introduce filter class lock scope
    TW_SCOPE_LOCK(class_lock, _classlock, "filter_class", WHATFN);
    fids.length(_class_keymap->length()); // pay once now, rather than doing incremental buffer growth
    for ( cursor = _class_keymap->cursor(); cursor.is_valid(); ++cursor) {
      Filter_i* f1 = cursor.val();
      if (f1->_callbacks_i.length() == 0 && f1->_callbacks.length() == 0) {
	fids[len] = f1->_fid;
	len++;
      }
    }
  } // end filter class lock scope
  for (CORBA::ULong i = 0; i < len; i++) {
    Filter_i* f2 = 0;
    _class_keymap->lookup(fids[i], f2);
    if ( (f2 != 0) && f2->destroy_i(1) ) {
      str << "Destroyed filter with FilterID " << fids[i] << '\n';
      num_destroyed++;
    }
  }
  str << "Total destroyed: " << num_destroyed <<  '\n';
}
#endif

void 
Filter_i::out_info_filter(RDIstrstream& str, const char* fname) 
{
  Filter_i* f = find_filter(fname);
  if (f) {
    f->out_info_descr(str);
  } else {
    str << "Invalid name: " << fname << " is not a filter name\n";
    str << "  (Use 'children' for list of valid filter names)\n";
  }
}

#undef WHATFN
#define WHATFN "Filter_i::out_info_all_filters"
void 
Filter_i::out_info_all_filters(RDIstrstream& str) 
{
  TW_SCOPE_LOCK(class_lock, _classlock, "filter_class", WHATFN);
  RDIFilterKeyMapCursor cursor;
  str << "\nAll non-destroyed filters attached to at least one proxy or admin or with an external callback\n";
  CORBA::Boolean none_found = 1;
  for ( cursor = _class_keymap->cursor(); cursor.is_valid(); ++cursor) {
    Filter_i* f = cursor.val();
    if (f->_callbacks_i.length() || f->_callbacks.length()) {
      none_found = 0;
      f->out_short_descr(str);
    }
  }
  if (none_found) {
    str << "(NONE)\n";
  }
  str << "\nAll non-destroyed filters not attached to a proxy or admin and no external callback\n";
  str << "  (normally means a client forgot to destroy a filter;\n";
  str << "   sometimes filter not yet added to a proxy or admin)\n";
  none_found = 1; 
  for ( cursor = _class_keymap->cursor(); cursor.is_valid(); ++cursor) {
    Filter_i* f = cursor.val();
    if (f->_callbacks_i.length() == 0 && f->_callbacks.length() == 0) {
      none_found = 0;
      f->out_short_descr(str);
      if (f->_last_detach.time == 0) {
	str << "  ** Never attached to a proxy or admin\n";
      } else {
	str << "  ** Last removed from proxy or admin at: ";
	f->_last_detach.out_local(str);
	str << "\n";
      }
    }
  }
  if (none_found) {
    str << "(NONE)\n";
  }
}

// these are instance methods
void 
Filter_i::out_short_descr(RDIstrstream& str) 
{
  log_output(str);
}

void 
Filter_i::out_info_descr(RDIstrstream& str) 
{
  log_output(str);
  str << "This filter attached to " << _callbacks_i.length() << " proxies or admins, "
      << _callbacks.length() << " external callbacks.\n";
  if (_callbacks_i.length() == 0 && _callbacks.length() == 0) {
    if (_last_detach.time == 0) {
      str << "  (filter never attached to a proxy or admin)\n";
  } else {
      str << "  (normally means a client forgot to destroy a filter)\n";
    }
  }
}

void
Filter_i::out_commands(RDIstrstream& str) 
{
  str << "omniNotify Filter commands:\n"
      << "  info              : show description of this filter\n"
      << "  up                : change target to filtfact\n";
}

char*
Filter_i::do_command(const char* cmnd, CORBA::Boolean& success,
		     CORBA::Boolean& target_changed,
		     AttN_Interactive_outarg next_target  WRAPPED_IMPLARG ) 
{
  RDIParseCmd p(cmnd);
  success = 1;
  target_changed = 0;
  if (p.argc == 0) { return CORBA_STRING_DUP("");  }

  RDIstrstream str;
  if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "help")) {
    out_commands(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "info")) {
    out_info_descr(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "up")) {
    target_changed = 1;
    next_target = WRAPPED_IMPL2OREF(AttN::FilterFactory, _factory);
    str << "\nomniNotify: new target ==> filtfact\n";
  } else {
    str << "Invalid command: " << cmnd << "\n";
    success = 0;
  }
  RDIRptInteractiveLog(_my_name << " received command: " << cmnd << "\nResult:\n" << str.buf());
  // this is the only safe way to return a string?
  return CORBA_STRING_DUP(str.buf());
}

//////////////////////////////////////////////////////////////////////

char*
MappingFilter_i::do_command(const char* cmnd, CORBA::Boolean& success,
			    CORBA::Boolean& target_changed,
			    AttN_Interactive_outarg next_target  WRAPPED_IMPLARG ) 
{
  // NOT SUPPORTED YET
  return CORBA_STRING_DUP("");
}


AttN::NameSeq* 
MappingFilter_i::child_names( WRAPPED_IMPLARG_VOID ) 
{
  // NOT SUPPORTED YET
  AttN::NameSeq* names = new AttN::NameSeq;
  if ( names == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  names->length(0);
  return names;
}

AttN::NameSeq* 
MappingFilter_i::my_name( WRAPPED_IMPLARG_VOID ) 
{
  // NOT SUPPORTED YET
  AttN::NameSeq* res = new AttN::NameSeq;
  if ( res == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  res->length(0);
  return res;
}

AttN::IactSeq* 
MappingFilter_i::children(CORBA::Boolean only_cleanup_candidates  WRAPPED_IMPLARG ) 
{
  AttN::IactSeq* ren = new AttN::IactSeq;
  if ( ren == (AttN::IactSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  ren->length(0);
  return ren;
}

CORBA::Boolean 
MappingFilter_i::safe_cleanup( WRAPPED_IMPLARG_VOID ) 
{
  // NOT SUPPORTED YET
  return 0;
}

