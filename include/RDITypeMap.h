// -*- Mode: C++; -*-
//                              File      : RDITypeMap.h
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
 
#ifndef _RDI_TYPE_MAP_H_
#define _RDI_TYPE_MAP_H_

#include "thread_wrappers.h"
#include "RDIHash.h"
#include "CosNotifyShorthands.h"
#include "CosNotifyFilter_i.h"
#include "CosNotifyChannelAdmin_i.h"

/** RDI_TypeMap
  *
  * This object is responsible for maintaining the mapping between
  * filters, which are registered with ConsumerAdmin/ProxySupplier
  * objects, and the event types present in the constraints of the
  * filters.
  */

class RDI_TypeMap {
public:
  // Support data structures required for implementing the mapping

  struct FNode_t {
    Filter_i* _fltr;
    FNode_t*  _next;
    FNode_t(Filter_i* fltr=0) : _fltr(fltr), _next(0) {;}
  };

  struct FList_t {
    FNode_t* _star_star;
    FNode_t* _domn_star;
    FNode_t* _star_type;
    FNode_t* _domn_type;
    FList_t() : _star_star(0),_domn_star(0),_star_type(0),_domn_type(0) {;}
  };

  RDI_TypeMap(EventChannel_i* channel, unsigned int init_hash_size=128);
  ~RDI_TypeMap();

  CORBA::Boolean update(RDI_LocksHeld&             held,
			const CosN::EventTypeSeq&  added,
			const CosN::EventTypeSeq&  deled,
			ConsumerAdmin_i*           admin,
			Filter_i*                  filter);
  CORBA::Boolean update(RDI_LocksHeld&             held,
			const CosN::EventTypeSeq&  added,
			const CosN::EventTypeSeq&  deled,
			RDIProxySupplier*          proxy,
			Filter_i*                  filter);

  // Retrieve the list of all event types registered with the map

  CosN::EventTypeSeq* obtain_subscription_types();
  CosN::EventTypeSeq* pxy_obtain_subscription_types(RDIProxyConsumer* pxy, CosNA::ObtainInfoMode mode);

  // Locate all filters for a given event type and a ConsumerAdmin
  // or ProxySupplier object.  Obtaining a consistent snapshot of
  // all filters is achieved by locking in read mode, which should
  // always be done using the following code template:
  //
  // { // introduce a read lock scope for _type_map
  //   RDI_TYPEMAP_READ_SCOPE_LOCK_TRACK(typemap_lock, held, _type_map, WHATFN);
  //   if (!held) { handler; }
  //   _type_map->lookup(...args...);
  //   ... use the lookup results ...
  // } // end of read lock scope for _type_map
  //

  void lookup(const char* domain_name, const char* type_name, 
	      ConsumerAdmin_i* admin, FList_t& filters);

  void lookup(const char* domain_name, const char* type_name,
	      RDIProxySupplier* proxy, FList_t& filters);

  struct ANode_t {
    ConsumerAdmin_i* _admn;
    FNode_t*         _fltr;
    ANode_t*         _next;
    ANode_t(ConsumerAdmin_i* admn=0, FNode_t* fltr=0) :
      _admn(admn), _fltr(fltr), _next(0) {;}
  };
  struct PNode_t {
    RDIProxySupplier* _prxy;
    FNode_t*          _fltr;
    PNode_t*          _next;
    PNode_t(RDIProxySupplier* prxy=0, FNode_t* fltr=0) :
      _prxy(prxy), _fltr(fltr), _next(0) {;}
  };
  struct VNode_t {
    ANode_t* _admn;
    PNode_t* _prxy;
    VNode_t(ANode_t* admn=0, PNode_t* prxy=0) : 
      _admn(admn), _prxy(prxy) {;}
  };

  RDIstrstream& log_output(RDIstrstream& str);

  // N.B. only the lock macro below should call this function
  TW_RWMutex& macro_get_rw_mutex() { return _lock; }

private:
  TW_RWMutex                          _lock;
  EventChannel_i*                     _channel;
  RDI_Hash<CosN::EventType, VNode_t>  _tmap;
};

// Use of 0 below means write lock
#define RDI_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(nm, held, tmap, whatfn) \
  TW_SCOPE_LOCK_RW_TRACK(nm, held, (tmap)->macro_get_rw_mutex(), 0, "typemap", whatfn)

// Use of 1 below means read lock
#define RDI_TYPEMAP_READ_SCOPE_LOCK_TRACK(nm, held, tmap, whatfn) \
  TW_SCOPE_LOCK_RW_TRACK(nm, held, (tmap)->macro_get_rw_mutex(), 1, "typemap", whatfn)

// Macros for typemap locking for objs that only have a channel ref

// Use of 0 below means write lock
#define RDI_CHAN_TYPEMAP_WRITE_SCOPE_LOCK_TRACK(nm, held, chan, whatfn) \
  TW_SCOPE_LOCK_RW_TRACK(nm, held, ((chan)->macro_get_typemap_ptr())->macro_get_rw_mutex(), 0, "typemap", whatfn)

// Use of 1 below means read lock
#define RDI_CHAN_TYPEMAP_READ_SCOPE_LOCK_TRACK(nm, held, chan, whatfn) \
  TW_SCOPE_LOCK_RW_TRACK(nm, held, ((chan)->macro_get_typemap_ptr())->macro_get_rw_mutex(), 1, "typemap", whatfn)

////////////////////////////////////////
// Logging

inline RDIstrstream& operator<< (RDIstrstream& str, RDI_TypeMap& map) { return map.log_output(str); }

#endif
