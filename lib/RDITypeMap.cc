// -*- Mode: C++; -*-
//                              File      : RDITypeMap.cc
//                              Package   : omniNotify-Library
//                              Created on: 1-Jan-1999
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
//    Implementation of RDI_TypeMap
//
 
#include "RDITypeMap.h"

RDI_TypeMap::RDI_TypeMap(EventChannel_i* channel, unsigned int hsize) :
  _lock(), _channel(channel), 
  _tmap(RDI_EventType::hash, RDI_EventType::rank, hsize, 20)
{;}

#undef WHATFN
#define WHATFN "RDI_TypeMap::~RDI_TypeMap"
RDI_TypeMap::~RDI_TypeMap()
{
  RDI_HashCursor<CosN::EventType, VNode_t> curs;
  { // introduce lock scope so that unlock occurs before destructor terminates
    TW_SCOPE_LOCK_RW(typemap_lock, _lock, 0, "typemap", WHATFN); // 0 means write lock
    for ( curs = _tmap.cursor(); curs.is_valid(); curs++ ) {
      VNode_t& value = curs.val();
      while ( value._admn ) {
	ANode_t* node = value._admn;
	while ( node->_fltr ) {
	  FNode_t* fltr = node->_fltr;
	  node->_fltr   = fltr->_next;
	  delete fltr;
	}
	value._admn = node->_next;
	delete node;
      }
      while ( value._prxy ) {
	PNode_t* node = value._prxy;
	while ( node->_fltr ) {
	  FNode_t* fltr = node->_fltr;
	  node->_fltr   = fltr->_next;
	  delete fltr;
	}
	value._prxy = node->_next;
	delete node;
      }
    }
    _tmap.clear();
    _channel = 0;
  } // end lock scope
}

// Update the mapping between event types and the filter registered
// at the ConsumerAdmin level.  Since this operation may be invoked 
// due to a 'subscription_change()' call, the filter may be NULL.

#undef WHATFN
#define WHATFN "RDI_TypeMap::update"
CORBA::Boolean
RDI_TypeMap::update(RDI_LocksHeld&             held,
		    const CosN::EventTypeSeq&  added,
		    const CosN::EventTypeSeq&  deled,
		    ConsumerAdmin_i*           admin,
		    Filter_i*                  filter)
{
  VNode_t value;
  CosN::EventTypeSeq gadded;
  CosN::EventTypeSeq gdeled;
  CORBA::ULong size;
  unsigned int indx;
  ANode_t *ncurr, *nprev;
  FNode_t *fcurr, *fprev;

  TW_COND_SCOPE_LOCK_RW_TRACK(typemap_lock, held.typemap, _lock, 0, "typemap", WHATFN); // 0 means write lock

  RDI_Assert(admin, "NULL ConsumerAdmin_i object was provided");
  gadded.length(0);
  gdeled.length(0);

  // Remove entries for the event types in the deleted list
  for ( indx = 0; indx < deled.length(); indx++ ) {
    if ( _tmap.lookup(deled[indx], value) == 0 )
      continue;
    ncurr = value._admn; nprev = 0;	// Locate admin
    while ( ncurr && (ncurr->_admn != admin) ) {
      nprev = ncurr;
      ncurr = ncurr->_next;
    }
    if ( ! ncurr )
      continue;
    fcurr = ncurr->_fltr; fprev = 0;	// Locate filter
    while ( fcurr && (fcurr->_fltr != filter) ) {
      fprev = fcurr;
      fcurr = fcurr->_next;
    }
    if ( ! fcurr )
      continue;
    // RDIDbgForceLog("XXX_REMOVE TypeMapping deleting an entry for filter " << (void*)filter << " \n");
    if ( ! fprev )			// Remove filter entry
      ncurr->_fltr = fcurr->_next;
    else
      fprev->_next = fcurr->_next;
    delete fcurr;
	
    if ( ! ncurr->_fltr ) {		// Remove admin entry
      if ( ! nprev ) {
	value._admn = ncurr->_next;
	_tmap.replace(deled[indx], value);
      } else {
	nprev->_next = ncurr->_next;
      }
      delete ncurr;
    }
    if ( ! value._admn && ! value._prxy ) {
      size = gdeled.length(); gdeled.length(size + 1);
      gdeled[size].domain_name = deled[indx].domain_name;
      gdeled[size].type_name   = deled[indx].type_name;
      _tmap.remove(deled[indx]);
    }
  }
  // Insert entries for the event types in the added list
  for ( indx = 0; indx < added.length(); indx++ ) {
    fcurr = new FNode_t(filter);
    // RDIDbgForceLog("XXX_REMOVE TypeMapping adding an entry for filter " << (void*)filter << " \n");
    if ( _tmap.lookup(added[indx], value) == 0 ) {
      value._admn = new ANode_t(admin, fcurr);
      value._prxy = 0;
      if ( _tmap.insert(added[indx], value) ) {
	delete fcurr;
	return 0;
      }
      size = gadded.length(); gadded.length(size + 1);
      gadded[size].domain_name = added[indx].domain_name;
      gadded[size].type_name   = added[indx].type_name;
    } else {
      for ( ncurr = value._admn; ncurr; ncurr = ncurr->_next ) {
	if ( ncurr->_admn == admin )
	  break;
      }
      if ( ! ncurr ) {
	ncurr = new ANode_t(admin, fcurr);
	ncurr->_next = value._admn;
	value._admn  = ncurr;
	_tmap.replace(added[indx], value);
      } else {
	fcurr->_next = ncurr->_fltr;
	ncurr->_fltr = fcurr;
      }
    }
  }
  // Notify suppliers about changes in the event types, if any
  if ( _channel && (gadded.length() > 0 || gdeled.length() > 0) ) {
    _channel->propagate_schange(held, gadded, gdeled);
  }
  return 1;
}

#undef WHATFN
#define WHATFN "RDI_TypeMap::update"
CORBA::Boolean
RDI_TypeMap::update(RDI_LocksHeld&             held,
		    const CosN::EventTypeSeq&  added,
		    const CosN::EventTypeSeq&  deled,
		    RDIProxySupplier*          proxy,
		    Filter_i*                  filter)
{
  VNode_t value;
  CosN::EventTypeSeq gadded;
  CosN::EventTypeSeq gdeled;
  CORBA::ULong size;
  unsigned int indx;
  PNode_t *ncurr, *nprev;
  FNode_t *fcurr, *fprev;

  TW_COND_SCOPE_LOCK_RW_TRACK(typemap_lock, held.typemap, _lock, 0, "typemap", WHATFN); // 0 means write lock

  RDI_Assert(proxy, "NULL RDIProxySupplier ref was provided");
  gadded.length(0);
  gdeled.length(0);

  // Remove entries for the event types in the deleted list
  for ( indx = 0; indx < deled.length(); indx++ ) {
    if ( _tmap.lookup(deled[indx], value) == 0 )
      continue;
    ncurr = value._prxy; nprev = 0;	// Locate proxy
    while ( ncurr && ncurr->_prxy != proxy ) {
      nprev = ncurr;
      ncurr = ncurr->_next;
    }
    if ( ! ncurr )
      continue;

    fcurr = ncurr->_fltr; fprev = 0;
    while ( fcurr && (fcurr->_fltr != filter) ) {
      fprev = fcurr;
      fcurr = fcurr->_next;
    }
    if ( ! fcurr )
      continue;
    // RDIDbgForceLog("XXX_REMOVE TypeMapping deleting an entry for filter " << (void*)filter << " \n");
    if ( ! fprev )			// Remove filter entry
      ncurr->_fltr = fcurr->_next;
    else
      fprev->_next = fcurr->_next;
    delete fcurr;
	
    if ( ! ncurr->_fltr ) {		// Remove proxy entry
      if ( ! nprev ) {
	value._prxy = ncurr->_next;
	_tmap.replace(deled[indx], value);
      } else {
	nprev->_next = ncurr->_next;
      }
      delete ncurr;
    }

    if ( ! value._admn && ! value._prxy ) {
      size = gdeled.length(); gdeled.length(size + 1);
      gdeled[size].domain_name = deled[indx].domain_name;
      gdeled[size].type_name   = deled[indx].type_name;
      _tmap.remove(deled[indx]);
    }
  }
  // Insert entries for the event types in the added list
  for ( indx = 0; indx < added.length(); indx++ ) {
    fcurr = new FNode_t(filter);
    // RDIDbgForceLog("XXX_REMOVE TypeMapping adding an entry for filter " << (void*)filter << " \n");
    if ( _tmap.lookup(added[indx], value) == 0 ) {
      value._prxy = new PNode_t(proxy, fcurr);
      value._admn = 0;
      if ( _tmap.insert(added[indx], value) ) {
	delete fcurr;
	return 0;
      }
      size = gadded.length(); gadded.length(size + 1);
      gadded[size].domain_name = added[indx].domain_name;
      gadded[size].type_name   = added[indx].type_name;
    } else {
      for ( ncurr = value._prxy; ncurr; ncurr = ncurr->_next ) {
	if ( ncurr->_prxy == proxy )
	  break;
      }
      if ( ! ncurr ) {
	ncurr = new PNode_t(proxy, fcurr);
	ncurr->_next = value._prxy;
	value._prxy  = ncurr;
	_tmap.replace(added[indx], value);
      } else {
	fcurr->_next = ncurr->_fltr;
	ncurr->_fltr = fcurr;
      }
    }
  }
  // Notify suppliers about changes in the event types, if any
  if ( _channel && (gadded.length() > 0 || gdeled.length() > 0) ) {
    _channel->propagate_schange(held, gadded, gdeled);
  }
  return 1;
}

void
RDI_TypeMap::lookup(const char* dname, const char*    tname,
		    ConsumerAdmin_i*                  admin, 
		    RDI_TypeMap::FList_t&             filters)
{
  VNode_t value;
  ANode_t* node;
  CosN::EventType evtype;

  filters._star_star = filters._domn_star = 0;
  filters._star_type = filters._domn_type = 0;

  evtype.domain_name = (const char *) "*"; 
  evtype.type_name   = (const char *) "*";
  if ( _tmap.lookup(evtype, value) == 1 ) {
    for ( node = value._admn; node; node = node->_next ) {
      if ( node->_admn == admin ) {
	filters._star_star = node->_fltr;
	break;
      }
    }
  }

  evtype.domain_name = (const char *) "*"; 
  evtype.type_name   = tname;
  if ( _tmap.lookup(evtype, value) == 1 ) {
    for ( node = value._admn; node; node = node->_next ) {
      if ( node->_admn == admin ) { 
	filters._star_type = node->_fltr;
	break;
      }
    }
  }

  evtype.domain_name = dname;
  evtype.type_name   = (const char *) "*";
  if ( _tmap.lookup(evtype, value) == 1 ) {
    for ( node = value._admn; node; node = node->_next ) {
      if ( node->_admn == admin ) { 
	filters._domn_star = node->_fltr;
	break;
      }
    }
  }

  evtype.domain_name = dname;
  evtype.type_name   = tname;
  if ( _tmap.lookup(evtype, value) == 1 ) {
    for ( node = value._admn; node; node = node->_next ) {
      if ( node->_admn == admin ) { 
	filters._domn_type = node->_fltr;
	break;
      }
    }
  }
}

void
RDI_TypeMap::lookup(const char*                      dname,
		    const char*                      tname,
		    RDIProxySupplier*                proxy,
		    RDI_TypeMap::FList_t&            filters)
{
  VNode_t value;
  PNode_t* node;
  CosN::EventType evtype;

  filters._star_star = filters._domn_star = 0;
  filters._star_type = filters._domn_type = 0;

  evtype.domain_name = (const char *) "*"; 
  evtype.type_name   = (const char *) "*";
  if ( _tmap.lookup(evtype, value) == 1 ) {
    for ( node = value._prxy; node; node = node->_next ) {
      if ( node->_prxy == proxy ) {
	filters._star_star = node->_fltr;
	break;
      }
    }
  }

  evtype.domain_name = (const char *) "*"; 
  evtype.type_name   = tname;
  if ( _tmap.lookup(evtype, value) == 1 ) {
    for ( node = value._prxy; node; node = node->_next ) {
      if ( node->_prxy == proxy ) {
	filters._star_type = node->_fltr;
	break;
      }
    }
  }

  evtype.domain_name = dname;
  evtype.type_name   = (const char *) "*";
  if ( _tmap.lookup(evtype, value) == 1 ) {
    for ( node = value._prxy; node; node = node->_next ) {
      if ( node->_prxy == proxy ) {
	filters._domn_star = node->_fltr;
	break;
      }
    }
  }

  evtype.domain_name = dname;
  evtype.type_name   = tname;
  if ( _tmap.lookup(evtype, value) == 1 ) {
    for ( node = value._prxy; node; node = node->_next ) {
      if ( node->_prxy == proxy ) {
	filters._domn_type = node->_fltr;
	break;
      }
    }
  }
}

#undef WHATFN
#define WHATFN "RDI_TypeMap::obtain_subscription_types"
CosN::EventTypeSeq*
RDI_TypeMap::obtain_subscription_types() 
{
  RDI_HashCursor<CosN::EventType, VNode_t> curs;
  CosN::EventTypeSeq* typeseq = new CosN::EventTypeSeq;
  RDI_Assert(typeseq, "Memory allocation failure -- EventTypeSeq");

  TW_SCOPE_LOCK_RW(typemap_lock, _lock, 1, "typemap", WHATFN); // 1 means read lock

  CORBA::ULong size = 0;
  typeseq->length(size);
  for ( curs = _tmap.cursor(); curs.is_valid(); curs++ ) {
    typeseq->length(size + 1);
    (*typeseq)[size].domain_name = curs.key().domain_name;	
    (*typeseq)[size++].type_name = curs.key().type_name;	
  }
  return typeseq;
}

#undef WHATFN
#define WHATFN "RDI_TypeMap::pxy_obtain_subscription_types"
CosN::EventTypeSeq*
RDI_TypeMap::pxy_obtain_subscription_types(RDIProxyConsumer* pxy, CosNA::ObtainInfoMode mode) 
{
  RDI_HashCursor<CosN::EventType, VNode_t> curs;
  CosN::EventTypeSeq* typeseq = new CosN::EventTypeSeq;
  RDI_Assert(typeseq, "Memory allocation failure -- EventTypeSeq");

  TW_SCOPE_LOCK_RW(typemap_lock, _lock, 1, "typemap", WHATFN); // 1 means read lock

  CORBA::ULong size = 0;
  typeseq->length(0);
  if ( (mode == CosNA::ALL_NOW_UPDATES_OFF) || (mode == CosNA::ALL_NOW_UPDATES_ON) ) {
    // fill in typeseq
    for ( curs = _tmap.cursor(); curs.is_valid(); curs++ ) {
      typeseq->length(size + 1);
      (*typeseq)[size].domain_name = curs.key().domain_name;	
      (*typeseq)[size].type_name = curs.key().type_name;
      size++;
    }
  }
  if ( (mode == CosNA::NONE_NOW_UPDATES_OFF) || (mode == CosNA::ALL_NOW_UPDATES_OFF) ) {
    pxy->_disable_updates();
  } else {
    pxy->_enable_updates();
  }
  return typeseq;
}

////////////////////////////////////////
// Logging

#undef WHATFN
#define WHATFN "RDI_TypeMap::log_output"
RDIstrstream&
RDI_TypeMap::log_output(RDIstrstream& str)
{
  VNode_t  value;
  ANode_t* anode;
  PNode_t* pnode;
  FNode_t* fnode;
  RDI_HashCursor<CosN::EventType, VNode_t> curs;
  str << "-------\nTypeMap\n-------\n";

  TW_SCOPE_LOCK_RW(typemap_lock, _lock, 1, "typemap", WHATFN); // 1 means read lock

  if (_tmap.length() == 0) {
    str << "\t(no entries)\n";
  } else {
    for ( curs = _tmap.cursor(); curs.is_valid(); curs++ ) {
      str << curs.key().domain_name << "::" << curs.key().type_name;
      anode = curs.val()._admn;
      pnode = curs.val()._prxy;
      while ( anode ) {
	str << "\n\tA " << anode->_admn << " : ";
	for ( fnode = anode->_fltr; fnode; fnode = fnode->_next )
	  str << fnode->_fltr << " ";
	anode = anode->_next;
      }
      while ( pnode ) {
	str << "\n\tP " << pnode->_prxy << " : ";
	for ( fnode = pnode->_fltr; fnode; fnode = fnode->_next )
	  str << fnode->_fltr << " ";
	pnode = pnode->_next;
      }
      str << '\n';
    }
  }
  return str;
}
