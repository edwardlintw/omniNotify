// -*- Mode: C++; -*-

// -------------------------------------------------------------- //
// This file contains SAMPLE implementation of push and pull      //
// suppliers and consumers that use the old-style CosEvent        //
// interfaces.  If you do not need to use the CosEvent interfaces //
// for backward-compatibility reasons, you can ignore these       //
// examples.                                                      //
//                                                                //
// The classes are similar to the classes in sample_clients.h     //
// (see the comment at the top of that file) with 2 exceptions:   //
// there is no change function parameter to the constructor, and  //
// there is no support for filters.  CosEvents does not support   //
// these features.                                                // 
// -------------------------------------------------------------- //

#ifndef _LEGACY_CLIENTS_H_
#define _LEGACY_CLIENTS_H_

#include "thread_wrappers.h"
#include "CosNotifyShorthands.h"
#include "sample_functions.h"

#define CosEA CosEventChannelAdmin

///////////////////////////////////////////////////////////////////
// Helper Classes                                                //
///////////////////////////////////////////////////////////////////

#ifndef __GenericWorker__
#define __GenericWorker__
class GenericWorker {
public:
  GenericWorker() {;}
  virtual void* start_working(void*) = 0;
};

class GenericBoundWorkerThread : public TW_Thread {
public:
  GenericBoundWorkerThread(GenericWorker* worker)
    : TW_Thread(), _worker(worker)
  { start_undetached(); }
  void* run_undetached(void *x)	{ return _worker->start_working(x); }
private:
  GenericWorker* _worker;
  GenericBoundWorkerThread()  {;}
}; 
#endif /*  __GenericWorker__  */

// ----------------- CosEvC::PushConsumer ---------------- //

// millisecs is ignored for push consumers
class LegacyPushConsumer_i :
  public GenericWorker,
  public POA_CosEventComm::PushConsumer,
  public PortableServer::RefCountServantBase
{
public:
  LegacyPushConsumer_i(CosEA::ProxyPushSupplier_ptr proxy, CORBA::ULong max_events,
		       const char* objnm, consume_any_fn* consume_fn,
		       CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static LegacyPushConsumer_i*
  create(CosEA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 const char* objnm = "LegacyPushConsumer_i",
	 consume_any_fn* consume_fn = sample_consume_any_fn,
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods 
  void push(const CORBA::Any & data);
  void disconnect_push_consumer();

  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosEA::ProxyPushSupplier_var _my_proxy;
  const char*                  _obj_name;
  consume_any_fn*              _consume_fn;
  CORBA::ULong                 _num_events;
  CORBA::ULong                 _max_events;
  CORBA::ULong                 _millisecs;
  CORBA::ULong                 _delay_millisecs;
  CORBA::Boolean               _verbose;
  CORBA::Boolean               _done;
  CORBA::Boolean               _com_err;
  TW_Mutex                     _oplock;
  TW_Condition                 _finish;
  GenericBoundWorkerThread*    _worker;
};

// ----------------- CosEvC::PullConsumer ---------------- //

class LegacyPullConsumer_i :
  public GenericWorker,
  public POA_CosEventComm::PullConsumer,
  public PortableServer::RefCountServantBase
{
public:
  LegacyPullConsumer_i(CosEA::ProxyPullSupplier_ptr proxy, CORBA::ULong max_events, 
		       const char* objnm, consume_any_fn* consume_fn,
		       CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static LegacyPullConsumer_i*
  create(CosEA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 const char* objnm = "LegacyPullConsumer_i",
	 consume_any_fn* consume_fn = sample_consume_any_fn,
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  void disconnect_pull_consumer();

  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosEA::ProxyPullSupplier_var _my_proxy;
  const char*                  _obj_name;
  consume_any_fn*              _consume_fn;
  CORBA::ULong                 _num_events;
  CORBA::ULong                 _max_events;
  CORBA::ULong                 _millisecs;
  CORBA::ULong                 _delay_millisecs;
  CORBA::Boolean               _verbose;
  CORBA::Boolean               _done;
  CORBA::Boolean               _com_err;
  TW_Mutex                     _oplock;
  TW_Condition                 _finish;
  GenericBoundWorkerThread*    _worker;
};

// ------------------ CosEvC::PushSupplier ---------------- //

class LegacyPushSupplier_i :
  public GenericWorker,
  public POA_CosEventComm::PushSupplier,
  public PortableServer::RefCountServantBase
{
public:
  LegacyPushSupplier_i(CosEA::ProxyPushConsumer_ptr proxy, CORBA::ULong max_events,
		       const char* objnm, supply_any_fn* supply_fn,
		       CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static LegacyPushSupplier_i*
  create(CosEA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 const char* objnm = "LegacyPushSupplier_i",
	 supply_any_fn* supply_fn = sample_supply_any_fn,
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  void disconnect_push_supplier();

  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosEA::ProxyPushConsumer_var _my_proxy;
  const char*                  _obj_name;
  supply_any_fn*               _supply_fn;
  CORBA::ULong                 _num_events;
  CORBA::ULong                 _max_events;
  CORBA::ULong                 _millisecs;
  CORBA::ULong                 _delay_millisecs;
  CORBA::Boolean               _verbose;
  CORBA::Boolean               _done;
  CORBA::Boolean               _com_err;
  TW_Mutex                     _oplock;
  TW_Condition                 _finish;
  GenericBoundWorkerThread*    _worker;
};

// ------------------- CosEvC::PullSupplier ---------------- //

class LegacyPullSupplier_i :
  public GenericWorker,
  public POA_CosEventComm::PullSupplier,
  public PortableServer::RefCountServantBase
{
public:
  LegacyPullSupplier_i(CosEA::ProxyPullConsumer_ptr proxy, CORBA::ULong max_events,
		       const char* objnm, supply_any_fn* supply_fn,
		       CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static LegacyPullSupplier_i*
  create(CosEA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 const char* objnm = "LegacyPullSupplier_i",
	 supply_any_fn* supply_fn = sample_supply_any_fn,
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  CORBA::Any* pull();
  CORBA::Any* try_pull(CORBA::Boolean& has_event);
  void        disconnect_pull_supplier();

  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosEA::ProxyPullConsumer_var _my_proxy;
  const char*                  _obj_name;
  supply_any_fn*               _supply_fn;
  CORBA::ULong                 _num_events;
  CORBA::ULong                 _max_events;
  CORBA::ULong                 _millisecs;
  CORBA::ULong                 _delay_millisecs;
  CORBA::Boolean               _verbose;
  CORBA::Boolean               _done;
  CORBA::Boolean               _com_err;
  TW_Mutex                     _oplock;
  TW_Condition                 _finish;
  GenericBoundWorkerThread*    _worker;
};

#endif
