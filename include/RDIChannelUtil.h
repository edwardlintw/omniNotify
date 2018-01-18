// -*- Mode: C++; -*-
//                              File      : RDIChannelUtil.h
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
 
#ifndef _RDI_CHANNEL_UTIL_H_
#define _RDI_CHANNEL_UTIL_H_

#include "RDIUtil.h"
#include "RDISafeList.h"

#include "thread_wrappers.h"
#include "RDIThreadPriority.h"
#include "CosNotifyShorthands.h"
#include "CosNotification_i.h"
#include "RDISeqDecls.h"

class RDI_RVM;

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// Event Channel Statistics related structures                           //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

typedef struct RDI_ThStat {
  TW_Mutex     _lock;
  CORBA::ULong _num_rdi_match;
  CORBA::ULong _num_rvm_eval;
  CORBA::ULong _num_announcements;
  CORBA::ULong _num_notifications;
  CORBA::ULong _qsize_acum;
  CORBA::ULong _qsize_ctr;
  CORBA::ULong _dummy[16]; 	// 64 bytes to force cache line separation?
} RDI_ThStat;

#define RDI_TH_ARRAY_SZ 32
#define RDI_TH_INDEX() ( TW_ID() % RDI_TH_ARRAY_SZ)

// report a delta every 1000 annoucements and/or notifications
// (the delta ctr increments every time a single thread has done 100 of these)
#define RDI_STATS_MINOR_INCREMENT 100
#define RDI_STATS_DELTA_INCREMENT 10

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// Various Support Classes and Utility Functions                         //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

/** RDIProxyPullConsumer and RDIProxyPushSupplier
 *
 * Support classes that are need by the pool of threads we create to
 * push notifications to consumers and pull events from suppliers...
 */

class RDIProxyPullConsumer {
public:
  virtual void pull_event(CORBA::Boolean& invalid) = 0;
  virtual CORBA::Boolean is_available(unsigned long*, unsigned long*) = 0;
};

class RDIProxyPushSupplier {
public:
  virtual void push_event(CORBA::Boolean& invalid) = 0;
  virtual CORBA::Boolean has_events(unsigned long*, unsigned long*) = 0;
};

/** The state of a proxy can be one of the following:
 *   - RDI_NotConnected : no supplier/consumer has been connected yet
 *   - RDI_Connected    : a supplier/consumer  is interacting with it
 *   - RDI_Disconnected : the supplier/consumer broke  the connection
 *   - RDI_Exception    : exception caught  during client interaction
 */

enum RDI_ProxyState { RDI_UnknownState, RDI_NotConnected, RDI_Connected, 
		      RDI_Disconnected, RDI_Exception };

/** The following is used during event dispatching to record the state
 * of filter evaluation at the ConsumerAdmin level
 */

enum RDI_FilterState_t { NoFilters, OrMatch, AndMatch, OrFailed };

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// Event Channel Thread related structures                               //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

/** A number of threads are used by each Event Channel for event filtering.
 * The total number of created threads and the work carried out by each of
 * them are set during channel creation.  The following class  is used for
 * this purpose.
 */

typedef void (EventChannel_i::*EventChannelMethod)(void);

class EventChannelDispatch {
public:
  EventChannelDispatch(EventChannel_i*    event_channel,
                       unsigned int       num_admin_threads,
                       EventChannelMethod admin_method,
                       unsigned int       num_proxy_threads,
                       EventChannelMethod proxy_method);
  ~EventChannelDispatch();
private:
  unsigned int            _num_athreads;
  TW_Thread**             _admin_thread;
  unsigned int            _num_pthreads;
  TW_Thread**             _proxy_thread;
  EventChannel_i*         _evnt_channel;

  EventChannelDispatch()        {;}
  EventChannelDispatch(const EventChannelDispatch&)     {;}
};

/** Admin and proxy objects may by grouped together for several reasons.
 * Here, we group them so that only one thread can evaluate filters for
 * all entries in a given group.
 */

typedef RDI_SafeList<ConsumerAdmin_i> CAdminGroup;

class CAdminGroupMgr {
public:
  CAdminGroupMgr(unsigned int numGroups, unsigned int numThreads);
  ~CAdminGroupMgr();

  // Insert/Remove an entry into/from a group -- group selection is
  // based on the number of groups and the ID assigned to 'entry'

  void insert(ConsumerAdmin_i* entry);
  void remove(ConsumerAdmin_i* entry);

  // Remove all entries and fill seq_out with the removed admins
  void remove_all(ConsumerAdminPtrSeq& seq_out);

  // The groups are accessed by a number of threads.  This number
  // is supplied to the above constructor.  Each admin thread MUST call
  // 'allocate_range()' ONCE to get an inclusive range of groups that
  // it will be responsible for accessing using the [] operator.

  CORBA::Boolean allocate_range(unsigned int& lo, unsigned int &hi);

  inline CAdminGroup* operator[] (unsigned int grpix) { return (grpix >= _length) ? 0 : & _groups[grpix]; }

  RDIstrstream& log_output(RDIstrstream& str);

private:
  TW_Mutex           _oplock;
  CAdminGroup*       _groups;
  unsigned int       _length;
  unsigned int       _nextgr;
  unsigned int       _numthr;
  unsigned int       _rmgrps;
  unsigned int       _rmthrs;
};

/** The number of pull suppliers and push consumers connected to a channel
 * may be large and,  hence, we should use a pool of threads instead of a
 * thread per client for pulling events from suppliers or pushing matched
 * events to consumers.  The following support classes are used for this.
 */

class RDI_PullSupplier {
public:
  RDI_PullSupplier(unsigned int numThreads=1, unsigned int periodMSecs=100);
  ~RDI_PullSupplier()           { this->destroy(); }

  void destroy();

  // Change in proxy state allowing pulling => wake waiting threads
  void signal_pull_threads() { _nonempty.signal(); }

  // Register/Deregister a ProxyConsumer so that created threads can pull
  // its connected supplier for available events
  void insert_proxy(RDIProxyPullConsumer* proxy);
  void remove_proxy(RDIProxyPullConsumer* proxy);

  // Update the pull period --- We do not have to get the lock here since
  // up-to-date information about '_period' is not crucial
  void set_pull_period(unsigned long nmsecs=0)	{ _period = nmsecs; }

  // The member function that gets executed by all created threads
  void pull_event();

  // Support structures needed for the implementation of the class
  struct ProxyEntry_t {
    RDIProxyPullConsumer* _proxy;
    CORBA::Boolean        _inuse;
    CORBA::Boolean        _deled;
    ProxyEntry_t*         _next;
    ProxyEntry_t(RDIProxyPullConsumer* prx=0) :
      _proxy(prx), _inuse(0), _deled(0), _next(0) {;}
  };
private:
  TW_Mutex                _oplock;
  TW_Condition            _nonempty;
  unsigned long           _period;
  TW_Thread**             _threads;
  unsigned int            _nactive;      // Number of threads still running
  unsigned int            _ndeleted;     // Number of deleted proxy entries
  CORBA::Boolean          _terminate;    // Set to true  during destruction
  ProxyEntry_t*           _entries;      // Registered  push  ProxySupplier
  ProxyEntry_t*           _last_one;     // Last entry accessed by a thread

  // Locate the next entry that is not being worked on by an active
  // thread -- the invocation is protected by the '_oplock' mutex
  ProxyEntry_t* _next_available(unsigned long*, unsigned long*);

  // Destroy the entries that are not used  and have been marked as
  // deleted -- the invocation is protected by the '_oplock' mutex
  void _gcollect();
};

class RDI_NotifyConsumer {
public:
  RDI_NotifyConsumer(unsigned int numThreads=1);
  ~RDI_NotifyConsumer()         { this->destroy(); }

  void destroy();

  // Change in proxy state allowing pushing => wake waiting threads
  void signal_push_threads() { _nonempty.signal(); }

  // Register/Deregister a ProxySupplier so that notifications are
  // sent to its connected consumer by one of the created theads
  void insert_proxy(RDIProxyPushSupplier* proxy);
  void remove_proxy(RDIProxyPushSupplier* proxy);

  // The member function that gets executed by all created threads
  void notify();

  // Support structures needed for the implementation of the class
  struct ProxyEntry_t {
    RDIProxyPushSupplier* _proxy;
    CORBA::Boolean        _inuse;
    CORBA::Boolean        _deled;
    ProxyEntry_t*         _next;
    ProxyEntry_t(RDIProxyPushSupplier* prx=0) :
      _proxy(prx), _inuse(0), _deled(0), _next(0) {;}
  };
private:
  TW_Mutex                _oplock;
  TW_Condition            _nonempty;
  TW_Thread**             _threads;
  unsigned int            _nactive;      // Number of threads still running
  unsigned int            _ndeleted;     // Number of deleted proxy entries
  CORBA::Boolean          _terminate;    // Set to true  during destruction
  ProxyEntry_t*           _entries;      // Registered  push  ProxySupplier
  ProxyEntry_t*           _last_one;     // Last entry accessed by a thread

  // Locate the next entry that is not being worked on by an active
  // thread -- the invocation is protected by the '_oplock' mutex
  ProxyEntry_t*  _next_available(unsigned long*, unsigned long*);

  // Destroy the entries that are not used  and have been marked as
  // deleted -- the invocation is protected by the '_oplock' mutex
  void _gcollect();
};

/** The total number of suppliers and consumers connected to a channel
 * that need subscription_change and offer_change messages, respecitvely,
 * may be large and,  hence, we should use a pool of threads instead of a
 * thread per client for doing subscription_change and offer_change.
 * The following support class is used for this.
 */

typedef enum {
  RDI_ChangeKind_OfferChange,
  RDI_ChangeKind_SubscriptionChange }
RDI_ChangeKind;

class RDI_ChangePool {
public:
  RDI_ChangePool(RDI_ChangeKind kind, unsigned int numThreads=1);
  ~RDI_ChangePool()         { this->destroy(); }

  void destroy();

  // the channel informs us of a new subscription or offer change
  void insert_change(RDI_ChangeKind kind,
		     const CosN::EventTypeSeq& added,
		     const CosN::EventTypeSeq& deled);

  // Register/Deregister a ProxyConsumer -- Only valid
  // for kind RDI_ChangeKind_SubscriptionChange
  void insert_proxy(RDIProxyConsumer* proxy);
  void remove_proxy(RDIProxyConsumer* proxy);

  // Register/Deregister a ProxySupplier -- Only valid
  //for kind RDI_ChangeKind_OfferChange
  void insert_proxy(RDIProxySupplier* proxy);
  void remove_proxy(RDIProxySupplier* proxy);

  // The member function that gets executed by all created threads
  void notify();

  // Support structures needed for the implementation of the class
  struct ProxyEntry_t {
    union {
      RDIProxyConsumer*  _cproxy;
      RDIProxySupplier*  _sproxy;
    };
    CORBA::Boolean        _inuse;
    CORBA::Boolean        _deled;
    ProxyEntry_t*         _pxy_next;
    ProxyEntry_t(RDIProxyConsumer* prx=0) :
      _cproxy(prx), _inuse(0), _deled(0), _pxy_next(0) {;}
    ProxyEntry_t(RDIProxySupplier* prx=0) :
      _sproxy(prx), _inuse(0), _deled(0), _pxy_next(0) {;}
  };

  struct CEntry_t {
    CORBA::Boolean      _valid;        // Are _added/_deleded valid yet?
    unsigned int        _num_inuse;    // Number of nodes on proxy chain in use
    CosN::EventTypeSeq   _added;
    CosN::EventTypeSeq   _deled;
    ProxyEntry_t*       _pxy_head;     // Head of chain of ProxyEntry
    ProxyEntry_t*       _pxy_tail;     // Tail of chain of ProxyEntry
    CEntry_t*           _c_next;       // For linking CEntry
    CEntry_t*           _c_skip;       // For skipping merged added/deled sets

    CEntry_t() :
      _valid(0), _num_inuse(0), _added(), _deled(),
      _pxy_head(0), _pxy_tail(0), _c_next(0), _c_skip(0)
    {
      _added.length(0);
      _deled.length(0);
    }
  };

private:
  TW_Mutex                 _oplock;
  RDI_ChangeKind           _kind;
  TW_Condition             _nonempty;
  TW_Thread**              _threads;
  unsigned long            _nproxies;     // Number of valid (non-deleted) proxy entries
  unsigned long            _ndeleted;     // Number of deleted proxy entries
  unsigned int             _nactive;      // Number of threads still running
  CORBA::Boolean           _terminate;    // Set to true  during destruction
  CEntry_t*                _c_head;       // Head of chain of CEntries
  CEntry_t*                _c_tail;       // Tail of chain -- holding pen for proxies that are up-to-date

  // Locate earliest entry that is not being worked on by an active
  // thread.  ASSUMES _oplock is already held.  Sets *found_entry to the
  // CEntry where the ProxyEntry_t result was found.  Also
  // moves node to next location, sets node->_inuse to 1, and
  // increments (*found_entry)->_num_inuse by 1.
  ProxyEntry_t* _next_available(CEntry_t** found_entry);

  // ASSUMES _oplock is held
  void _gcollect();
};

/** Each event channel uses a number of threads for carrying out filter
 * evaluation, garbage collection etc..  The classes below provide the
 * "wrappers" for creating these threads.
 */

class EventChannelWorker : public TW_Thread {
public:
  EventChannelWorker(EventChannel_i* channel, EventChannelMethod method,
		     TW_PRIORITY_T priority = RDI_CHANNEL_WORKER_PRIORITY);
  void run(void *);
private:
  EventChannel_i*    _channel;
  EventChannelMethod _method;
  EventChannelWorker()  {;}
};

class EventChannelBoundWorker : public TW_Thread {
public:
  EventChannelBoundWorker(EventChannel_i* channel, EventChannelMethod method,
			  TW_PRIORITY_T priority = RDI_CHANNEL_WORKER_PRIORITY);
  void* run_undetached(void *);
private:
  EventChannel_i*    _channel;
  EventChannelMethod _method;
  EventChannelBoundWorker() {;}
};

////////////////////////////////////////
// Logging

RDIstrstream& operator<< (RDIstrstream& str, const RDI_ProxyState& s);
RDIstrstream& operator<< (RDIstrstream& str, const CAdminGroup& agr);
inline RDIstrstream& operator<< (RDIstrstream& str, CAdminGroupMgr& agrm)  { return agrm.log_output(str); }

#endif
