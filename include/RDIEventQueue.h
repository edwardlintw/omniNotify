// -*- Mode: C++; -*-
//                              File      : RDIEventQueue.h
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
 
#ifndef __RDI_EVENT_QUEUE_H__
#define __RDI_EVENT_QUEUE_H__

#include "thread_wrappers.h"
#include "CosNotifyShorthands.h"
#include "RDIEvent.h"
#include "CosNotification_i.h"

/** Each EventChannel_i maintains a global queue that contains the
  * announced events.  numAdminThreads is used for reference counting the
  * events inserted in the queue.  maxQueueLimit (if non-zero)
  * limits the queue size.  With a limit, if rejectNewEvents is false,
  * discardPolicy would be used to discard an old event.
  * (Currently that property combo is not supported.)
  */

class RDI_EventQueue {
public:
  RDI_EventQueue(CORBA::UShort  numAdminThreads,
		 CORBA::ULong   queueGCPeriod,
		 CORBA::ULong   maxQueueLength,
		 CORBA::Boolean rejectNewEvents,
		 CORBA::Short   discardPolicy);
  ~RDI_EventQueue();

  // Channel informs event queue when any of these properties
  // *may* have changed.  event queue does not necessarily react
  // immediately, e.g., a lower maxQueueLength will not cause
  // an immediate discard to chop the queue down to the new size;
  // instead it will lazily get to the new size as GC occurs.
  void qos_changed(CORBA::ULong   queueGCPeriod,
		   CORBA::ULong   maxQueueLength,
		   CORBA::Boolean rejectNewEvents,
		   CORBA::Short   discardPolicy);

  // Insert a new event into the queue -- reference counter of the
  // event must be 1 at this point. On success, 0 is returned.  If
  // we are shutting down or reached the capacity of the queue and
  // we cannot drop existing events, -1 is returned

  int insert(RDI_StructuredEvent* event);

  // Retrieve the next event based on a previously accessed event.
  // The very first time this is called,  the value of 'prevEvent'
  // MUST be NULL. If no next event exists yet and 'block' is set,
  // the caller is blocked until an event becomes available.  NULL
  // is returned if 'block' is not set and no event is avaialble.
  
  RDI_StructuredEvent* next_event(RDI_StructuredEvent* prevEvent, 
			          CORBA::Boolean         block = 1);

  // The following can be used to view a snapshot of the status of
  // the queue. Since no lock is held, the numbers may not reflect
  // the most up-to-date information. 

  CORBA::ULong  length() const		{ return _length; }
  CORBA::ULong  max_size() const	{ return _maxQueueLength; }
  CORBA::ULong  queue_GC_period() const	{ return _queueGCPeriod; }
  CORBA::ULong  announced() const	{ return _announ; }
  CORBA::ULong  dropped() const		{ return _nmdrop; }

  // Change the maximun size of the queue - it does not affect any
  // of the events already present in the queue

  void set_max_size(CORBA::ULong new_max_size);

  // Debug/stats reporting.  
  // For out_debug_info, when 'showEvents' is set, some
  // information about each entry in queue is also displayed.

  void out_debug_info(RDIstrstream& str, CORBA::Boolean showEvents=0);
  void out_stats(RDIstrstream& str);

  // logs stats to the report log: logging only done if enabled
  // via RDIRptQSizeStats or if force is set.
  void log_stats(CORBA::Boolean force=0);

  // set_finished can be called prior to destroying the queue
  // to force any threads blocked on next_event to receive
  // null replies.  If wait_on_threads is true, waits for _numblk
  // to be zero and gcdone to be true before returning.
  void set_finished(CORBA::Boolean wait_on_threads);

private:
  TW_Mutex              _oplock;
  TW_Condition          _qempty;
  TW_Mutex              _gclock;
  TW_Condition          _qclean;
  CORBA::Boolean        _gcdone; // Set on inactive garbage collector
  CORBA::Boolean        _finish; // Set when queue is beeing disposed
  TW_Thread*            _worker;	// Garbage collection  worker thread
  RDI_StructuredEvent*  _evhead;	// Head of the linked list of events
  RDI_StructuredEvent*  _evtail;	// Tail of the linked list of events
  CORBA::ULong          _length; // Current length  of the event list
  CORBA::ULong          _announ;	// Number of announced events so far
  CORBA::ULong          _nmdrop;	// Number of dropped  events  so far
  unsigned int          _numreg;	// Number of threads accessing queue
  unsigned int          _numblk;	// Number of  blocked access threads
  // the 4 qos props that can change
  CORBA::ULong          _maxQueueLength;
  CORBA::ULong          _queueGCPeriod;
  CORBA::Short          _discardPolicy;
  CORBA::Boolean        _rejectNewEvents;

  // Used when queue appears to be full: see if oldest event is
  // actually garbage, and, if so, remove it.
  int gc1();

  // Used when the queue is full and rejectNewEvents is false:
  // apply discard policy to make room in the queue, if possible
  int  apply_discard_policy();

  // Garbage collect the queue of events. Garbage collection takes
  // place for events having reference counter == 1 and state that
  // is not equal to RDI_StructuredEvent::NEWBORN

  void garbage_collect();

  // The garbage collection thread is activated when the queue size
  // exceeds a threshold

  inline int run_garbage_collect() const 
    { return (((_maxQueueLength == 0) && (_length < 4096)) || 
	      ((_maxQueueLength > 0) && (_length < (3*_maxQueueLength)/4))) ? 0 : 1; }

};

#endif
