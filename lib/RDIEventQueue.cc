// -*- Mode: C++; -*-
//                              File      : RDIEventQueue.cc
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
//    Implementation of RDI_EventQueue [Channel's event queue]
//

#include "thread_wrappers.h"
#include "RDIThreadPriority.h"
#include "RDI.h"
#include "RDIEventQueue.h"

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// Notes
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

// Discarding events already in the global queue is based on 3 things:
//    _rejectNewEvents, _maxQueueLength, _discardPolicy
// (passed to queue when it is constructed, updated with qos_changed)
// The rules are:
//
// 1. If rejectNewEvents is true, always discard an incoming event
//    rather than an event already in the queue.
//
// 2. Otherwise, if maxQueueLength is zero, always accept new events
//    since there is no limit on queue size.
//
// 3. Otherwise, if maxQueueLength is reached, pick an event to discard
//    based on discardPolicy().
//
// ** At the moment, omniNotify only supports cases 1+2, i.e.,
// the channel can only be configured such that either rejectNewEvents
// is true or maxQueueLength is zero.

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
//              Support class used for garbage collection                //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

class EventQueueWorker : public TW_Thread {
public:
  typedef void (RDI_EventQueue::*QueueMethod)(void);

  EventQueueWorker(RDI_EventQueue* queue, QueueMethod method,
                   TW_PRIORITY_T prio = RDI_EVENT_QUEUE_PRIORITY) :
    TW_Thread(NULL, prio), _equeue(queue), _method(method) {;}
  void run(void *)      { (_equeue->*_method)(); }
private:
  RDI_EventQueue* _equeue;
  QueueMethod     _method;
  EventQueueWorker()  {;}
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

RDI_EventQueue::RDI_EventQueue(CORBA::UShort  numAdminThreads,
			       CORBA::ULong   queueGCPeriod,
			       CORBA::ULong   maxQueueLength,
			       CORBA::Boolean rejectNewEvents,
			       CORBA::Short   discardPolicy) :
  _oplock(), _qempty(&_oplock), _gclock(), _qclean(&_gclock),
  _gcdone(1), _finish(0), _worker(0), _evhead(0),
  _evtail(0), _length(0),
  _announ(0), _nmdrop(0), _numreg(numAdminThreads), _numblk(0),
  _maxQueueLength(maxQueueLength), _queueGCPeriod(queueGCPeriod), _discardPolicy(discardPolicy),
  _rejectNewEvents(rejectNewEvents)
{
  _worker = new EventQueueWorker(this, & RDI_EventQueue::garbage_collect);
  _worker->start();
}

#undef WHATFN
#define WHATFN "RDI_EventQueue::~RDI_EventQueue"
RDI_EventQueue::~RDI_EventQueue()
{
  { // introduce oplock scope so that lock release occurs before destructor terminates
    TW_SCOPE_LOCK(evqueue_lock, _oplock, "evqueue", WHATFN);
    RDIDbgEvQLog("   - event queue is shutting down\n");
    if (!_finish) {
      // Set '_finish' flag to true and signal the garbage collection and
      // any other blocked threads. Wait for _numblk zero and gcdone
      _finish = 1;
      _qclean.signal();
      _qempty.broadcast();
      while (_numblk != 0 || ! _gcdone) {
	RDIDbgEvQLog("~RDI_EventQueue waiting on " << _numblk << " blocked threads [gcdone = " << (int)_gcdone << "]\n");
	_qclean.signal();
	_qempty.broadcast();
	{ // introduce unlock scope
	  TW_SCOPE_RELEASE(evqueue_lock);
	  TW_YIELD();
	} // end unlock scope
      }
      RDIDbgEvQLog("~RDI_EventQueue: _numblk is zero and gc thread done\n");
    }
    while ( _evhead ) {	// Delete all enqueued events
      RDI_StructuredEvent* tmpevnt = _evhead;
      _evhead = tmpevnt->_next;
      delete tmpevnt;
    }
    // Need to release the mutex to avoid EBUSY errors during 
    // destruction of the mutex object
    _length = _numreg = 0;
    _evhead = _evtail = 0;
  } // end oplock scope
}

#undef WHATFN
#define WHATFN "RDI_EventQueue::qos_changed"
void
RDI_EventQueue::qos_changed(CORBA::ULong   queueGCPeriod,
			    CORBA::ULong   maxQueueLength,
			    CORBA::Boolean rejectNewEvents,
			    CORBA::Short   discardPolicy)
{
  TW_SCOPE_LOCK(evqueue_lock, _oplock, "evqueue", WHATFN);
  RDIDbgEvQLog("RDI_EventQueue::qos_changed called with:"
	       << "\n  queueGCPeriod   = " << queueGCPeriod 
	       << "\n  maxQueueLength  = " << maxQueueLength 
	       << "\n  rejectNewEvents = " << rejectNewEvents 
	       << "\n  discardPolicy   = " << discardPolicy << '\n');
  _maxQueueLength  = maxQueueLength;
  _queueGCPeriod   = queueGCPeriod;
  _discardPolicy   = discardPolicy;
  _rejectNewEvents = rejectNewEvents;
  _qclean.signal();
}

#undef WHATFN
#define WHATFN "RDI_EventQueue::set_finished"
// Set '_finish' flag to true and signal the garbage collection and
// any other blocked threads.  If wait_on_threads is true, wait until
// there are no blocked threads and gcdone before returning.
void
RDI_EventQueue::set_finished(CORBA::Boolean wait_on_threads)
{
  TW_SCOPE_LOCK(evqueue_lock, _oplock, "evqueue", WHATFN);
  RDIDbgEvQLog("RDI_EventQueue::set_finished called\n");
  if (_finish) {
    RDIDbgEvQLog("RDI_EventQueue::set_finished called more than once\n");
    return;
  }
  _finish = 1;
  _qclean.signal();
  _qempty.broadcast();
  RDIDbgEvQLog("RDI_EventQueue::set_finished: _numblk = " << _numblk << " [gcdone = " << (int)_gcdone << "]\n");
  if (wait_on_threads) {
    while (_numblk != 0 || ! _gcdone) {
      RDIDbgEvQLog("RDI_EventQueue::set_finished waiting on " << _numblk << " blocked threads [gcdone = " << (int)_gcdone << "]\n");
      _qclean.signal();
      _qempty.broadcast();
      { // introduce unlock scope
	TW_SCOPE_RELEASE(evqueue_lock);
	TW_YIELD();
      } // end unlock scope
    }
    RDIDbgEvQLog("RDI_EventQueue::set_finished: _numblk is zero and _gcdone is true\n");
  }
}

#undef WHATFN
#define WHATFN "RDI_EventQueue::insert"
int
RDI_EventQueue::insert(RDI_StructuredEvent* event)
{
  TW_SCOPE_LOCK(evqueue_lock, _oplock, "evqueue", WHATFN);
  _announ += 1;

  if ( _finish ) {		// Are we shutting down???
    return -1;
  }
  if ( _maxQueueLength && (_length >= _maxQueueLength) && gc1() ) {
    // out of space
    if (_rejectNewEvents) { // reject the incoming event
      return -1;
    }
    // try to use discard policy
    if (apply_discard_policy()) {
      // could not free up space
      return -1;
    }
    // apply_discard_policy did free up some space
  }
  // The reference counter of the event MUST by 1 at this point
  RDI_Assert((event->ref_counter()==1), "Invalid event refCnt\n");

  // RECORD ARRIVAL TIME
  event->set_arrival_time();

  // Increment event reference counter by 'concurrency' level.
  // Not very flexible since we cannot update the concurrency
  // level during run time.  Do not need to use locking yet
  // because event is not visible to other threads until it
  // is inserted below.
  event->n_incr_ref_counter_lock_held(_numreg);

  _length     += 1;
  event->_next = 0;
  if ( _evtail ) { _evtail->_next = event; _evtail = event; } 
  else           { _evtail = event;        _evhead = event; }

  if ( _numblk != 0 )	// Signal threads waiting for new event
    _qempty.broadcast();
  if ( run_garbage_collect() && _gcdone ) // Garbage collection
    _qclean.signal();

  return 0;
}

#undef WHATFN
#define WHATFN "RDI_EventQueue::next_event"
// Return the next event to be processed, given that the previously
// processed event is 'eprev'. The assumption made in the following
// method is that  accessing a pointer or a four-byte integer is an
// atomic operation and, hence,  we do not need to acquire the lock
// on the queue unless we have already processed all events .......
RDI_StructuredEvent* 
RDI_EventQueue::next_event(RDI_StructuredEvent* eprev, CORBA::Boolean block)
{
  RDI_StructuredEvent* event = 0;

  if ( _finish )  {	// We are shutting down
    return 0;
  }

  if ( ! eprev ) {
    // First time in the method; we assume that only future
    // events are of interest and, thus, we use '_evtail'
    if ( (event = _evtail) ) {
      if ( event->get_state() == RDI_StructuredEvent::NEWBORN )
	event->set_state(RDI_StructuredEvent::PENDING);
      return event;
    } else if ( ! block ) {
      return 0;
    } else {
      // To avoid missing events we use '_evhead'. In
      // addition,  we acquire '_oplock' since we will
      // update the queue state.

      { // introduce oplock scope
	TW_SCOPE_LOCK(evqueue_lock, _oplock, "evqueue", WHATFN);
	_numblk += 1;
	while ( ! _finish && ! _evhead ) {
	  TW_CV_WAIT(_qempty, _oplock, "evqueue", WHATFN);
	}
	_numblk -= 1;
	if ( _finish ) {	// We are shutting down
	  return 0;
	}
	event = _evhead;
      } // end oplock scope

      if ( event->get_state() == RDI_StructuredEvent::NEWBORN )
	event->set_state(RDI_StructuredEvent::PENDING);
      return event;
    }
  } else {
    // We decrement the reference counter of an event which
    // has already being accessed ONLY AFTER a new event is
    // available to 'prevent' any race condition with the
    // garbage collection worker thread
    if ( eprev->_next ) {
      event = eprev->_next;
      RDI_SEVENT_DECR_REF_COUNTER(eprev, WHATFN);
      if ( event->get_state() == RDI_StructuredEvent::NEWBORN )
	event->set_state(RDI_StructuredEvent::PENDING);
      return event;
    } else if ( ! block ) {
      return 0;
    } else {
      { // introduce oplock scope
	TW_SCOPE_LOCK(evqueue_lock, _oplock, "evqueue", WHATFN);
	_numblk += 1;
	while ( ! _finish && ! eprev->_next ) {
	  TW_CV_WAIT(_qempty, _oplock, "evqueue", WHATFN);
	}
	_numblk -= 1;
	if ( _finish ) {        // We are shutting down
	  return 0;
	}
	event = eprev->_next;
      } // end oplock scope
      RDI_SEVENT_DECR_REF_COUNTER(eprev, WHATFN);
      if ( event->get_state() == RDI_StructuredEvent::NEWBORN )
	event->set_state(RDI_StructuredEvent::PENDING);
      return event;
    }
  }
  return 0;
}

// quick GC mechanism, to see if single space can be freed up.
// It returns 0 is space is freed up, otherwise -1
int
RDI_EventQueue::gc1()
{
  RDI_StructuredEvent* tmpevnt = 0;

  if ( (_evhead->ref_counter() > 1) ||
       (_evhead->get_state() == RDI_StructuredEvent::NEWBORN) )
    return -1;
  tmpevnt = _evhead;
  _evhead = _evhead->_next;
  delete tmpevnt;
  _nmdrop += 1;
  return 0;
}

// Not implemented.  See NOTES section at top.
// Returns 0 if some space is freed up, otherwise -1
int
RDI_EventQueue::apply_discard_policy()
{
  RDIRptForceLog("Global queue does not currently implement a discard policy -- rejecting incoming event instead\n");
  return -1;
}

#undef WHATFN
#define WHATFN "RDI_EventQueue::garbage_collect"
// Garbage collect processed events that are not needed any longer.
// These events have a reference counter==1, and their state is not
// RDI_StructuredEvent::NEWBORN. Garbage collection starts from the
// head of the queue and stops once an event that does not meet the
// above criteria is encountered 
void
RDI_EventQueue::garbage_collect()
{
  RDI_StructuredEvent*  tmpevnt;
  CORBA::ULong cursize, evncntr;
  unsigned long secs, nsecs;
  unsigned long tid = TW_ID();

  while ( 1 ) {
    { // introduce gc lock scope
      TW_SCOPE_LOCK(evqueue_lock, _gclock, "evqueue_gc", WHATFN);
      if ( _finish ) {
	RDIDbgEvQLog("\tGC thread " << tid << " exits\n");
	goto evqueue_gc_exit;
      }
      TW_GET_TIME(&secs, &nsecs, _queueGCPeriod, 0);
      TW_CV_TIMEDWAIT(_qclean, secs, nsecs, _gclock, "evqueue_gc", WHATFN);
      if ( _finish ) {
	RDIDbgEvQLog("\tGC thread " << tid << " exits\n");
	goto evqueue_gc_exit;
      }
    } // end gc lock scope

    // Remember the current length of the queue and set the
    // garbage collection flag to indicate work in progress

    { // introduce oplock scope
      TW_SCOPE_LOCK(evqueue_lock, _oplock, "evqueue", WHATFN);
      cursize = _length;
      _gcdone = 0;		// Garbage collection started
    } // end oplock scope
    evncntr = 0;
    while ( --cursize && _evhead && (_evhead->ref_counter() == 1) &&
	    (_evhead->get_state() != RDI_StructuredEvent::NEWBORN) ) {
      tmpevnt = _evhead;
      _evhead = _evhead->_next;
      delete tmpevnt;
      if ( ++evncntr % 100 == 0 ) 
	TW_YIELD();
    }
    { // introduce oplock scope
      TW_SCOPE_LOCK(evqueue_lock, _oplock, "evqueue", WHATFN);
      if ( evncntr ) {
	RDIDbgEvQLog("GC: disposed "<<evncntr<<" of "<<_length<<" events\n");
      }
      _length -= evncntr;
      _gcdone  = 1;		// Garbage collection finished
    } // end oplock scope
    TW_YIELD();
  }

 evqueue_gc_exit:
  TW_EXIT();
}

void
RDI_EventQueue::log_stats(CORBA::Boolean force)
{
  if (force) {
    RDIRptLogger1(l);
    out_stats(l.str);
  } else if (RDIRptTst(RDIRptQSizeStats)) {
    RDIRptLogger(l,RDIRptQSizeStats_nm);
    out_stats(l.str);
  }
}

#undef WHATFN
#define WHATFN "RDI_EventQueue::out_debug_info"
void
RDI_EventQueue::out_debug_info(RDIstrstream& str, CORBA::Boolean show_events)
{
  TW_SCOPE_LOCK(evqueue_lock, _oplock, "evqueue", WHATFN);
  str << "--------------\nRDI_EventQueue\n--------------\n";
  if ( show_events ) {
    RDI_StructuredEvent* evnode = _evhead;
    while ( evnode ) {
      str << "\t" << evnode << " - " << evnode->get_domain_name() << "::" <<
	evnode->get_type_name() << " [" << evnode->get_event_name()  <<
	"] " << " refCnt " << evnode->ref_counter();
      switch ( evnode->get_state() ) {
      case RDI_StructuredEvent::INVALID   : str <<" INVALID\n"; break;
      case RDI_StructuredEvent::NEWBORN   : str <<" NEWBORN\n"; break;
      case RDI_StructuredEvent::DISPATCHED: str <<" DISPATCHED\n"; break;
      case RDI_StructuredEvent::PENDING   : str <<" PENDING\n"; break;
      }
      evnode = evnode->_next;
    }
  }
  str << "Size " << _length << " [Max " << _maxQueueLength << "] #announced " << 
    _announ << " #dropped " << _nmdrop << " period " << _queueGCPeriod << '\n';
}

#undef WHATFN
#define WHATFN "RDI_EventQueue::out_stats"
void
RDI_EventQueue::out_stats(RDIstrstream& str)
{
  TW_SCOPE_LOCK(evqueue_lock, _oplock, "evqueue", WHATFN);
  str << "--------------\nRDI_EventQueue\n--------------\n";
  str << "Size " << _length << " [Max " << _maxQueueLength << "] #announced " << 
    _announ << " #dropped " << _nmdrop << " period " << _queueGCPeriod << '\n';
}
