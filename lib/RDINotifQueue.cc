// -*- Mode: C++; -*-
//                              File      : RDINotifQueue.cc
//                              Package   : omniNotify-Library
//                              Created on: 1-Mar-2001
//                              Authors   : J.Ding & R.Gruber
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
//    RDINotifQueue implementation
//
 
#include "RDINotifQueue.h"
#include "RDIUtil.h"

// ================================ PriorityQueue ==========================================

RDIPriorityQueue::RDIPriorityQueue(CORBA::ULong init_size,
				   CORBA::Boolean pri_lowest_first,
				   CORBA::Boolean sec_lowest_first) :
  _pri_lowest_first(pri_lowest_first), _sec_lowest_first(sec_lowest_first), _num_items(0)
{
  _curr_size = ((init_size < 4) ? 4 : init_size) + 1;
  _entry = new RDIPriorityQueueEntry[_curr_size];
  // index 0 is not used, null it out just for the heck of it
  _entry[0].event = 0;
  _entry[0].prival = 0;
  _entry[0].secval = 0;
}

RDIPriorityQueue::RDIPriorityQueue(const RDIPriorityQueue& q) :
  _pri_lowest_first(q._pri_lowest_first), _sec_lowest_first(q._sec_lowest_first), _num_items(q._num_items)
{
  _curr_size = q._curr_size;
  _entry = new RDIPriorityQueueEntry[_curr_size];
  // copy elts
  for (CORBA::ULong i = 0; i < _curr_size; i++) {
    _entry[i] = q._entry[i];
  }
}

RDIPriorityQueue::~RDIPriorityQueue()
{
  _drain(); delete [] _entry;
}

CORBA::ULong
RDIPriorityQueue::length() const {
  return _num_items;
}

CORBA::ULong
RDIPriorityQueue::curr_size() const {
  return _curr_size;
}

RDI_StructuredEvent*
RDIPriorityQueue::get_pri_head() {
  if (_num_items < 1) return 0;
  return _entry[1].event;
}

RDI_StructuredEvent*
RDIPriorityQueue::remove_pri_head() {
  if (_num_items < 1) return 0; 
  return _remove(1); 
}

RDI_StructuredEvent* 
RDIPriorityQueue::get_sec_head() {
  if (_num_items < 1) return 0;
  return _entry[_sec_head_index()].event;
}

RDI_StructuredEvent* 
RDIPriorityQueue::remove_sec_head() {
  if (_num_items < 1) return 0;
  return _remove(_sec_head_index()); 
}

void
RDIPriorityQueue::pri_lowest_first(CORBA::Boolean b) {
  _pri_lowest_first = b; 
}

void
RDIPriorityQueue::sec_lowest_first(CORBA::Boolean b) {
  _sec_lowest_first = b; 
}

// for testing
RDI_StructuredEvent* 
RDIPriorityQueue::get_event(CORBA::ULong i) {
  if (i > _num_items) return 0;
  return _entry[i].event;
}

CORBA::Boolean
RDIPriorityQueue::pri_lowest_first() {
  return _pri_lowest_first;
}

CORBA::Boolean 
RDIPriorityQueue::sec_lowest_first() {
  return _sec_lowest_first; 
}

// --------- private ------------

// 3 static helpers
CORBA::ULong RDIPriorityQueue::_parent(CORBA::ULong i) {return (i >> 1);}
CORBA::ULong RDIPriorityQueue::_left(CORBA::ULong i)   {return (2 * i);}
CORBA::ULong RDIPriorityQueue::_right(CORBA::ULong i)  {return ((2 * i) + 1);}

void
RDIPriorityQueue::_drain() {
  while (_num_items) {
    _remove(_num_items);
  }
}

void
RDIPriorityQueue::_swap(CORBA::ULong i, CORBA::ULong j) {
  if (i == j) return;
  RDIPriorityQueueEntry tmp = _entry[i];
  _entry[i] = _entry[j];
  _entry[j] = tmp;
}

RDI_StructuredEvent* 
RDIPriorityQueue::_remove(CORBA::ULong idx) { 
  if (idx > _num_items) return 0;
  RDI_StructuredEvent* res = _entry[idx].event;
  _entry[idx].event = 0;
  _entry[idx].prival = 0;
  _entry[idx].secval = 0;
  _num_items--;
  if (idx == (_num_items + 1)) {
    return res;
  }
  // move last to idx, re-establish heap order
  _entry[idx] = _entry[_num_items + 1];
  CORBA::ULong i = idx;
  CORBA::ULong s = _num_items + 1;
  while (1) {
    CORBA::ULong l = _left(i);
    CORBA::ULong r = _right(i);
    CORBA::ULong best = i;
    if (_pri_lowest_first) {
      if ((l < s) && (_entry[l].prival < _entry[best].prival))
	best = l;
      if ((r < s) && (_entry[r].prival < _entry[best].prival))
	best = r;
    } else { // highest first
      if ((l < s) && (_entry[l].prival > _entry[best].prival))
	best = l;
      if ((r < s) && (_entry[r].prival > _entry[best].prival))
	best = r;
    }
    if (best == i) {
      break;
    }
    _swap(i, best);
    i = best;
  }
  return res;
}

int
RDIPriorityQueue::insert(RDI_StructuredEvent* ev,
			 CORBA::ULongLong pri, CORBA::ULongLong sec) {
  if ( ((_num_items+1) == _curr_size) && (_resize() == -1) )
    return 0;
  _num_items++;
  _entry[_num_items].event = ev;
  _entry[_num_items].prival = pri;
  _entry[_num_items].secval = sec;
  CORBA::ULong pos = _num_items;
  // Re-establish heap order by moving "pos" up
  while (1) {
    if (pos == 1) {
      break;
    }
    CORBA::ULong par = _parent(pos);
    if (_pri_lowest_first) {
      if (_entry[pos].prival >= _entry[par].prival) {
	break;
      }
    } else { // highest first
      if (_entry[pos].prival <= _entry[par].prival) {
	break;
      }
    }
    _swap(pos, par);
    pos = par;
  }
  return 1;
}

int
RDIPriorityQueue::_resize() {
  CORBA::ULong new_size = (2 * (_curr_size-1)) + 1;
  RDIPriorityQueueEntry* tmp_buff = 0;

  if ( (tmp_buff = new RDIPriorityQueueEntry[new_size]) == (RDIPriorityQueueEntry*) 0 )  
    return -1;
  for (CORBA::ULong i=0; i <= _num_items; i++) {
    tmp_buff[i] = _entry[i];
  }
  _curr_size = new_size;
  delete [] _entry;
  _entry = tmp_buff;
  return 0;
}

CORBA::ULong
RDIPriorityQueue::_sec_head_index() {
  if (_num_items < 1) return 0;
  // linear search (for now)
  CORBA::ULong best = 1;
  for (CORBA::ULong i = 2; i <= _num_items; i++) {
    if (_sec_lowest_first) {
      if (_entry[i].secval < _entry[best].secval) {
	best = i;
      }
    } else { // highest first
      if (_entry[i].secval > _entry[best].secval) {
	best = i;
      }
    }
  }
  return best;
}

// ================================== NotifQueue ===========================================

RDINotifQueue::RDINotifQueue(CORBA::ULong init_size) {
  if (init_size < 4) init_size = 4;
  _order_policy    = CosN::FifoOrder;
  _discard_policy  = CosN::FifoOrder;
  _def_timeout     = (CORBA::ULongLong)0;
  _def_priority    = CosN::DefaultPriority;
  CORBA::Boolean pri_lowest_first = (_order_policy   != CosN::PriorityOrder);
  CORBA::Boolean sec_lowest_first = (_discard_policy != CosN::PriorityOrder);
  _queue = new RDIPriorityQueue(init_size, pri_lowest_first, sec_lowest_first);
}

RDINotifQueue::RDINotifQueue(const RDINotifQueue& q) {
  _order_policy = q._order_policy;
  _discard_policy = q._discard_policy;
  _def_timeout = q._def_timeout;
  _def_priority = q._def_priority;
  _queue = new RDIPriorityQueue(*(q._queue));
}

RDINotifQueue::~RDINotifQueue() { delete _queue; }

CORBA::ULong
RDINotifQueue::length() const {
  return _queue->length(); 
}

int
RDINotifQueue::insert(RDI_StructuredEvent* event) {
  CORBA::ULongLong sortval = _sort_val(_order_policy, event);
  if (_order_policy == _discard_policy) {
    return _queue->insert(event, sortval, sortval);
  }
  return _queue->insert(event, sortval, _sort_val(_discard_policy, event));
}

RDI_StructuredEvent*
RDINotifQueue::get_pri_head() {
  return _queue->get_pri_head(); 
}

RDI_StructuredEvent*
RDINotifQueue::remove_pri_head() {
  return _queue->remove_pri_head(); 
}

RDI_StructuredEvent*
RDINotifQueue::get_sec_head() {
  if (_order_policy == _discard_policy) {
    return _queue->get_pri_head(); // more efficient 
  }
  return _queue->get_sec_head(); 
}

RDI_StructuredEvent*
RDINotifQueue::remove_sec_head() {
  if (_order_policy == _discard_policy) {
    return _queue->remove_pri_head(); // more efficient 
  }
  return _queue->remove_sec_head(); 
}

// for testing 
CORBA::Short
RDINotifQueue::orderPolicy() {
  return _order_policy; 
}

CORBA::Short
RDINotifQueue::discardPolicy() {
  return _discard_policy; 
}

void
RDINotifQueue::qos_changed(RDI_NotifQoS* qospr) {
  RDI_StructuredEvent* event;
  CORBA::Short     new_order_policy   = qospr->orderPolicy();
  if (new_order_policy == CosN::AnyOrder)
    new_order_policy = CosN::FifoOrder;
  CORBA::Short     new_discard_policy = qospr->discardPolicy();
  if (new_discard_policy == CosN::AnyOrder)
    new_discard_policy = CosN::FifoOrder;
  CORBA::ULongLong new_def_timeout    = qospr->timeout().time;
  CORBA::Short     new_def_priority   = qospr->priority();
  if ((_order_policy   == new_order_policy) &&
      (_discard_policy == new_discard_policy) &&
      (_def_priority   == new_def_priority) &&
      (_def_timeout    == new_def_timeout)) {
    // no relevant changes
    return;
  }
  // one or more of these changed
  _order_policy   = new_order_policy;
  _discard_policy = new_discard_policy;
  _def_priority   = new_def_priority;
  _def_timeout    = new_def_timeout;

  CORBA::Boolean pri_lowest_first = (_order_policy   != CosN::PriorityOrder);
  CORBA::Boolean sec_lowest_first = (_discard_policy != CosN::PriorityOrder);

  if (_queue->length() == 0) {
    // just need to modify the 2 lowest_first settings
    _queue->pri_lowest_first(pri_lowest_first);
    _queue->sec_lowest_first(sec_lowest_first);
    return;
  }

  // queue must be rebuilt to reflect changes
  RDIPriorityQueue* new_queue = new RDIPriorityQueue(_queue->curr_size(), pri_lowest_first, sec_lowest_first);
  for (CORBA::ULong i = 1; i <= _queue->length(); i++) {
    event = _queue->get_event(i);
    CORBA::ULongLong sortval = _sort_val(_order_policy, event);
    if (_order_policy == _discard_policy) {
      new_queue->insert(event, sortval, sortval);
    } else {
      new_queue->insert(event, sortval, _sort_val(_discard_policy, event));
    }
  }
  delete _queue;
  _queue  = new_queue;
}

// --------- private ------------

CORBA::ULongLong
RDINotifQueue::_sort_val(CORBA::Short policy, RDI_StructuredEvent* event) {
  if (policy == CosN::AnyOrder || policy == CosN::FifoOrder) {
    // sort by event arrival time
    CORBA::ULongLong res = event->get_timestamp();
    return res;
  }
  if (policy == CosN::DeadlineOrder) {
    // sort by (event arrival time + timeout)
    CORBA::ULongLong timeout = _def_timeout;
    const RDI_RTVal* to = event->lookup_vahdr_rtval("Timeout");
    if (to) {
      if (to->_tckind == RDI_rtk_dynany) {
	try {
	  WRAPPED_DYNANY_MODULE::DynStruct_var da_struct = WRAPPED_DYNANY_MODULE::DynStruct::_narrow((to->_v_dynanyval)._top_ptr);
	  if (!CORBA::is_nil(da_struct)) {
	    da_struct->seek(0);
	    CORBA::ULong  t_lo = da_struct->get_ulong();
	    CORBA::UShort t_hi = da_struct->get_ushort();
	    timeout = ((CORBA::ULongLong)t_lo + ((CORBA::ULongLong)t_hi << 32));
	  }
	}
	catch(...)
	  {
	    RDIDbgNotifQueueLog("Exception caught while pulling a timeout value from a DynAny\n");
	  }
      }
    }
    CORBA::ULongLong res =  event->get_timestamp() + timeout;
    return res;
  }
  // policy is CosN::PriorityOrder; sort by priority
  CORBA::Short priority = _def_priority;
  const RDI_RTVal* prio = event->lookup_vahdr_rtval("Priority");
  if (prio) {
    if (prio->_tckind == RDI_rtk_short) {
      priority = prio->_v_short;
    }
  }
  CORBA::ULongLong res = (CORBA::ULongLong) priority;
  return res;
}

