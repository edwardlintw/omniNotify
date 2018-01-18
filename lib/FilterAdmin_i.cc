// -*- Mode: C++; -*-
//                              File      : FilterAdmin_i.cc
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
//    Implementation of FilterFactory_i and FAdminHelper
//

#include "RDI.h"
#include "RDINotifServer.h"
#include "RDIStringDefs.h"
#include "RDIList.h"
#include "RDICatchMacros.h"
#include "RDIOplocksMacros.h"
#include "CosNfyUtils.h"
#include "CosNotifyFilter_i.h"
#include "CosNotifyChannelAdmin_i.h"

////////////////////////////////////////////////////////////////////

FilterFactory_i::FilterFactory_i(const char* grammar) :
  _oplockptr(0), _disposed(0), _nlangs(0)
{
  RDI_OPLOCK_INIT("filtfact");
  _my_name.length(2);
  _my_name[0] = (const char*)"server";
  _my_name[1] = (const char*)"filtfact";
  for (unsigned int i=0; i < MAXGR; i++) {
    _clangs[i] = 0; 
  }
  if ( ! (_clangs[0] = CORBA_STRING_DUP(grammar)) ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  _nlangs += 1;
  WRAPPED_REGISTER_IMPL2(this, &_my_name);
}

FilterFactory_i::~FilterFactory_i()
{
  RDI_OPLOCK_DESTROY_CHECK("FilterFactory_i");
}

#undef WHATFN
#define WHATFN "FilterFactory_i::cleanup_and_dispose"
void
FilterFactory_i::cleanup_and_dispose()
{
  RDI_OPLOCK_BUMP_SCOPE_LOCK(filtfact_lock, WHATFN, return);
  if (_disposed) {
    RDIDbgFAdminLog("FilterFactory_i::cleanup_and_dispose() called more than once\n");
    return;
  }
  _disposed = 1; // acts as guard: the following is executed by only one thread 
  for (unsigned int i=0; i < MAXGR; i++) {
    CORBA_STRING_FREE(_clangs[i]);
    _clangs[i] = 0;
  }
  _nlangs  = 0;
  RDI_OPLOCK_SET_DISPOSE_INFO(filtfact_lock.dispose_info);
}

#undef WHATFN
#define WHATFN "FilterFactory_i::create_filter"
CosNF::Filter_ptr
FilterFactory_i::create_filter(const char* grammar  WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(filtfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  Filter_i* fltr = 0;
  RDIDbgFAdminLog("Create a new filter using: " << grammar << '\n');
  if ( ! _is_supported(grammar) ) {
    throw CosNF::InvalidGrammar();
  }
  if ( ! (fltr = new Filter_i(grammar, this)) ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  RDI_AssertAllocThrowNo(fltr, "Memory allocation failure -- Filter_i\n");
  return WRAPPED_IMPL2OREF(CosNF::Filter, fltr);
}

#undef WHATFN
#define WHATFN "FilterFactory_i::create_mapping_filter"
CosNF::MappingFilter_ptr
FilterFactory_i::create_mapping_filter(const char* grammar, const CORBA::Any& value   WRAPPED_IMPLARG )
{
  RDI_OPLOCK_SCOPE_LOCK(filtfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  MappingFilter_i* fltr = 0;
  if ( ! _is_supported(grammar) ) {
    throw CosNF::InvalidGrammar();
  }
  if ( ! (fltr = new MappingFilter_i(grammar, value, this)) ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  RDIDbgFAdminLog("Created new MappingFilter using: " << grammar << '\n');
  return WRAPPED_IMPL2OREF(CosNF::MappingFilter, fltr); 
}

#undef WHATFN
#define WHATFN "FilterFactory_i::add_grammar"
int
FilterFactory_i::add_grammar(const char* grammar)
{
  RDI_OPLOCK_SCOPE_LOCK(filtfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  if ( _is_supported(grammar) ) {
    return 0;
  }
  if ( _nlangs == MAXGR ) {
    RDIDbgFAdminLog("reached max number of supported grammars\n");
    return -1;
  }
  for (unsigned int i=0; i < MAXGR; i++) {
    if ( _clangs[i] == (char *) 0 ) {
      if ( ! (_clangs[i] = CORBA_STRING_DUP(grammar)) ) {
	RDIDbgFAdminLog("failed to allocate string for " << grammar << '\n');
	return -1;
      } else {
	RDIDbgFAdminLog("Adding support for: " << grammar << '\n');
	_nlangs += 1;
	return 0;
      }
    }
  }
  RDIDbgForceLog("Internal error -- inconsistent data structures.....\n");
  return -1;
}

#undef WHATFN
#define WHATFN "FilterFactory_i::del_grammar"
void
FilterFactory_i::del_grammar(const char* grammar)
{
  RDI_OPLOCK_SCOPE_LOCK(filtfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  for (unsigned int i=0; i < MAXGR; i++) {
    if ( _clangs[i] && RDI_STR_EQ(_clangs[i], grammar) ) {
      RDIDbgFAdminLog("Deleting support for: " << grammar << '\n');
      CORBA_STRING_FREE(_clangs[i]);
      _nlangs -= 1;
      break;
    }
  }
}

#undef WHATFN
#define WHATFN "FilterFactory_i::is_supported"
CORBA::Boolean
FilterFactory_i::is_supported(const char* grammar)
{
  RDI_OPLOCK_SCOPE_LOCK(filtfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  CORBA::Boolean res = _is_supported(grammar);
  return res;
}

// oplock already acquired; does the real work
CORBA::Boolean
FilterFactory_i::_is_supported(const char* grammar)
{
  for (unsigned int i=0; i < MAXGR; i++) {
    if ( _clangs[i] && RDI_STR_EQ(_clangs[i], grammar) ) {
      return 1;
    }
  }
  return 0;
}

////////////////////////////////////////
// Interactive

#undef WHATFN
#define WHATFN "FilterFactory_i::my_name"
AttN::NameSeq*
FilterFactory_i::my_name( WRAPPED_IMPLARG_VOID )
{
  RDI_OPLOCK_SCOPE_LOCK(filtfact_lock, WHATFN, RDI_THROW_INV_OBJREF);
  AttN::NameSeq* res = new AttN::NameSeq(_my_name);
  if ( res == (AttN::NameSeq*) 0 ) {
    throw CORBA::NO_MEMORY(0, CORBA::COMPLETED_NO);
  }
  return res;
}

AttN::NameSeq*
FilterFactory_i::child_names( WRAPPED_IMPLARG_VOID )
{
  return Filter_i::all_filter_names();
}

AttN::IactSeq*
FilterFactory_i::children(CORBA::Boolean only_cleanup_candidates WRAPPED_IMPLARG )
{
  return Filter_i::all_children(only_cleanup_candidates);
}

CORBA::Boolean
FilterFactory_i::safe_cleanup( WRAPPED_IMPLARG_VOID )
{
  return 0; // not destroyed
}

void
FilterFactory_i::cleanup_all(RDIstrstream& str)
{
  str << "\nDestroying all filters not attached to a proxy or admin\n";
  unsigned int num_destroyed = 0;
  AttN::IactSeq* cleanups = Filter_i::all_children(1);
  if (cleanups) {
    for (unsigned int i = 0; i < cleanups->length(); i++) {
      AttN::NameSeq* nm = 0;
      CORBA::Boolean worked = 0;
      try {
	nm = (*cleanups)[i]->my_name();
	worked = (*cleanups)[i]->safe_cleanup();
      } CATCH_INVOKE_PROBLEM_NOOP;
      if (worked) {
	num_destroyed++;
	str << "Destroyed filter " << *nm << '\n';
      }
      if (nm) delete nm;
    }
    delete cleanups;
  }
  str << "Total filters destroyed: " << num_destroyed <<  '\n';
}

void
FilterFactory_i::out_commands(RDIstrstream& str)
{
  str << "omniNotify FilterFactory commands:\n"
      << "  cleanup           : destroy all filters not attached to a proxy or admin\n"
      << "                        (normally means a client forgot to destroy a filter;\n"
      << "                         sometimes filter not yet added to a proxy or admin)\n"
      << "  info filters      : show brief description of each non-destroyed filter\n"
      << "  info <filt_name>  : show description of a specific filter\n"
      << "  up                : change target to server\n"
      << "  go <filt_name>    : change target to a specific filter\n"
      << "                        ('children' lists filter names)\n";
}

char*
FilterFactory_i::do_command(const char* cmnd, CORBA::Boolean& success,
			    CORBA::Boolean& target_changed,
			    AttN_Interactive_outarg next_target  WRAPPED_IMPLARG )
{
  RDIParseCmd p(cmnd);
  success = 1;
  target_changed = 0;
  if (p.argc == 0) { return CORBA_STRING_DUP("");  }

  RDINotifServer* server = RDI::get_server_i();
  RDIstrstream str;
  if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "help")) {
    out_commands(str);
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "up")) {
    target_changed = 1;
    next_target = RDI::get_server();
    str << "\nomniNotify: new target ==> server\n";
  } else if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "cleanup")) {
    cleanup_all(str);
  } else if ((p.argc == 2) &&
	     RDI_STR_EQ_I(p.argv[0], "info") && RDI_STR_EQ_I(p.argv[1], "filters")) {
    Filter_i::out_info_all_filters(str);
  } else if ((p.argc == 2) &&
	     RDI_STR_EQ_I(p.argv[0], "info")) {
    Filter_i::out_info_filter(str, p.argv[1]);
  } else if ((p.argc == 2) &&
	     RDI_STR_EQ_I(p.argv[0], "go")) {
    Filter_i* f = Filter_i::find_filter(p.argv[1]);
    if (f) {
      target_changed = 1;
      next_target = WRAPPED_IMPL2OREF(AttN::Filter, f);
      str << "\nomniNotify: new target ==> " << p.argv[1] << '\n';
    } else {
      str << "Invalid target: " << p.argv[1] << " is not a filter name\n";
      str << "  (Use 'children' for list of valid filter names)\n";
      success = 0;
    }
  } else {
    str << "Invalid command: " << cmnd << "\n";
    success = 0;
  }
  RDIRptInteractiveLog(_my_name << " received command: " << cmnd << "\nResult:\n" << str.buf());
  // this is the only safe way to return a string?
  return CORBA_STRING_DUP(str.buf());
}

////////////////////////////////////////////////////////////////////

FAdminHelper::FAdminHelper(const char *resty) :
  _resty(resty), _serial(1),
  _filters(RDI_CorbaULongHash, RDI_CorbaULongRank)
{;}

FAdminHelper::~FAdminHelper()
{;}

#undef WHATFN
#define WHATFN "FAdminHelper::add_filter_i"
CosNF::FilterID
FAdminHelper::add_filter_i(RDI_LocksHeld&          held,
			   CosNF::Filter_ptr       new_filter,
			   RDINotifySubscribe_ptr  filter_holder,
			   CORBA::Boolean          need_schange)
{
  FAdminFilterEntry  entry;
  CosNF::FilterID    fltrID;
  Filter_i*          fltr    = Filter_i::Filter2Filter_i(new_filter);

  RDI_Assert(fltr, "Filter was not created by READY\n");

  fltrID            = _serial++;
  entry.filter      = fltr;
  entry.callback_id = fltr->attach_callback_i(held, filter_holder, need_schange);
  if ( _filters.insert(fltrID, entry) ) {
    RDIDbgFAdminLog("Failed to register new filter in hash table\n");
    return 0;
  }
  CosNF::Filter::_duplicate(new_filter);
  RDIDbgFAdminLog("\tFilter " << new_filter << " [" << fltrID << "] for " << filter_holder << '\n');
  return fltrID;
}

// --------------------------------------------------------------------------------
// A filter is being destroyed and it is registered with the object that uses
// this particular FAdminHelper object. To avoid trying to do matching againt a
// destroyed filter, we must remove the filter from the internal hash table.  If
// the fadmin is an schange subscriber, it is already informed of subscription
// change due to destruction because the filter does a remove_all_constraints as
// part of its destruction.
// --------------------------------------------------------------------------------

#undef WHATFN
#define WHATFN "FAdminHelper::rem_filter_i"
void
FAdminHelper::rem_filter_i(Filter_i* fltr)
{
  CosNF::FilterID     fltrID = fltr->getID();
  FAdminFilterEntry   entry;

  if ( ! _filters.lookup(fltrID, entry) ) {
    // RDIDbgForceLog("XXX_REMOVE UNEXPECTED: rem_filter_i failed to find filter, fid " << fltrID << '\n');
    return;
  }
  Filter_i *f = entry.filter;
#ifndef NDEBUG
  // if (f != fltr) {
  //   RDIDbgForceLog("XXX_REMOVE UNEXPECTED: rem_filter_i lookup for fid " << fltrID << " produced different filter\n");
  // }
  return;
#endif
  RDIDbgFAdminLog("FAdminHelper::rem_filter_i: removing filter [" << fltrID << "] from hash table\n");
  _filters.remove(fltrID);
  WRAPPED_RELEASE_IMPL(f);
}

#undef WHATFN
#define WHATFN "FAdminHelper::get_filter"
CosNF::Filter_ptr
FAdminHelper::get_filter(CosNF::FilterID fltrID)
{
  FAdminFilterEntry entry;

  if ( _filters.lookup(fltrID, entry) ) {
    RDIDbgFAdminLog("Get Filter " << entry.filter << " [" << fltrID << "]\n");
    CosNF::Filter_var res = WRAPPED_IMPL2OREF(CosNF::Filter, entry.filter);
    return res;
  }
  throw CosNF::FilterNotFound();
}

#undef WHATFN
#define WHATFN "FAdminHelper::get_all_filters"
CosNF::FilterIDSeq*
FAdminHelper::get_all_filters()
{
  RDI_HashCursor<CosNF::FilterID, FAdminFilterEntry> fcur;
  CORBA::ULong  indx=0;
  CosNF::FilterIDSeq* fseq=new CosNF::FilterIDSeq();
  RDI_AssertAllocThrowNo(fseq, "Memory allocation failure -- FilterIDSeq\n");

  fseq->length(_filters.length());
  for ( fcur = _filters.cursor(); fcur.is_valid(); fcur++, indx++ ) {
    (*fseq)[indx] = fcur.key();
  }
  return fseq;
}

#undef WHATFN
#define WHATFN "FAdminHelper::remove_filter"
void
FAdminHelper::remove_filter(RDI_LocksHeld&          held,
			    CosNF::FilterID         fltrID,
			    RDINotifySubscribe_ptr  filter_holder)
{
  FAdminFilterEntry entry;

  if ( ! _filters.lookup(fltrID, entry) ) {
    throw CosNF::FilterNotFound();
  }
  Filter_i *f = entry.filter;
  f->fadmin_removal_i(held, entry.callback_id, filter_holder);
  RDIDbgFAdminLog("remove_filter: removing filter [" << fltrID << "] from hash table\n");
  _filters.remove(fltrID);
  WRAPPED_RELEASE_IMPL(f);
}

#undef WHATFN
#define WHATFN "FAdminHelper::remove_all_filters"
void
FAdminHelper::remove_all_filters(RDI_LocksHeld&          held,
				 RDINotifySubscribe_ptr  filter_holder)
{
  RDI_HashCursor<CosNF::FilterID, FAdminFilterEntry> fcur;

  for ( fcur = _filters.cursor(); fcur.is_valid(); fcur++ ) {
    FAdminFilterEntry &entry = fcur.val();
    // RDIDbgForceLog("XXX_REMOVE " << WHATFN << " calling fadmin_removal_i(held, entry.callback_id, filter_holder)");
    entry.filter->fadmin_removal_i(held, entry.callback_id, filter_holder);
    WRAPPED_RELEASE_IMPL(entry.filter);
  }
  _filters.clear();
}

#undef WHATFN
#define WHATFN "FAdminHelper::out_info_filters"
void
FAdminHelper::out_info_filters(RDIstrstream& str)
{
  RDI_HashCursor<CosNF::FilterID, FAdminFilterEntry> fcur;

  if (_filters.length() == 0) {
    str << "  (no attached filters)\n";
  } else {
    for ( fcur = _filters.cursor(); fcur.is_valid(); fcur++ ) {
      FAdminFilterEntry &entry = fcur.val();
      entry.filter->out_info_descr(str);
    }
  }
}
