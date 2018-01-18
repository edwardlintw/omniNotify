// -*- Mode: C++; -*-
//                              File      : RDINotifQueue.h
//                              Package   : omniNotify-Library
//                              Created on: 1-Mar-2001
//                              Authors   : ding&gruber
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
//    RDINotifQueue proprietary interface
//
 
#ifndef _RDI_NOTIF_QUEUE_H_
#define _RDI_NOTIF_QUEUE_H_

#include "RDIEvent.h"
#include "CosNotification_i.h"

// ================================ PriorityQueue ==========================================

// Priority queue with primary and secondary sort keys.
// Supports pri_head, sec_head, remove_pri_head, remove_sec_head.
// Lookup/delete using secondary sort key is currently linear time.

struct RDIPriorityQueueEntry {
  RDI_StructuredEvent* event;
  CORBA::ULongLong     prival;
  CORBA::ULongLong     secval;
};

class RDIPriorityQueue {
public:
  // for both sort keys, default is "lowest first",
  // i.e. remove_foo_head produces lowest-valued item
  RDIPriorityQueue(CORBA::ULong init_size,
		   CORBA::Boolean pri_lowest_first,
		   CORBA::Boolean sec_lowest_first);
  RDIPriorityQueue(const RDIPriorityQueue& q);
  ~RDIPriorityQueue();

  CORBA::ULong length() const;
  CORBA::ULong curr_size() const;

  int insert(RDI_StructuredEvent* ev, CORBA::ULongLong prival, CORBA::ULongLong secval);

  RDI_StructuredEvent* get_pri_head();
  RDI_StructuredEvent* remove_pri_head();

  RDI_StructuredEvent* get_sec_head();
  RDI_StructuredEvent* remove_sec_head();

  void pri_lowest_first(CORBA::Boolean b);
  void sec_lowest_first(CORBA::Boolean b);

  // for testing
  RDI_StructuredEvent* get_event(CORBA::ULong i);
  CORBA::Boolean pri_lowest_first();
  CORBA::Boolean sec_lowest_first();

private:
  static CORBA::ULong _parent(CORBA::ULong i);
  static CORBA::ULong _left(CORBA::ULong i);
  static CORBA::ULong _right(CORBA::ULong i);

  RDI_StructuredEvent* _remove(CORBA::ULong i);

  void          _swap(CORBA::ULong i, CORBA::ULong j);
  int           _resize();
  void          _drain();
  CORBA::ULong  _sec_head_index();

  CORBA::Boolean          _pri_lowest_first;
  CORBA::Boolean          _sec_lowest_first;
  CORBA::ULong            _num_items;
  CORBA::ULong            _curr_size;
  RDIPriorityQueueEntry*  _entry;
};

// ================================== NotifQueue ===========================================

class RDINotifQueue { 
public:
  RDINotifQueue(CORBA::ULong init_size=128);
  RDINotifQueue(const RDINotifQueue& q);
  ~RDINotifQueue();

  CORBA::ULong         length() const;
  int                  insert(RDI_StructuredEvent* event);
  RDI_StructuredEvent* get_pri_head();
  RDI_StructuredEvent* remove_pri_head();
  RDI_StructuredEvent* get_sec_head();
  RDI_StructuredEvent* remove_sec_head();
  void                 qos_changed(RDI_NotifQoS* qosprop);

  // for testing 
  CORBA::Short orderPolicy();
  CORBA::Short discardPolicy();

private:
  CORBA::ULongLong   _sort_val(CORBA::Short order_policy, RDI_StructuredEvent* event);

  CORBA::ULongLong   _def_timeout;
  CORBA::Short       _def_priority;
  CORBA::Short       _order_policy;
  CORBA::Short       _discard_policy;
  RDIPriorityQueue*  _queue;
};

#endif  //_RDI_NOTIF_QUEUE_H_

