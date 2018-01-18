// -*- Mode: C++; -*-

// -------------------------------------------------------------- 
// sample_clients.h / .cc contains SAMPLE implementations of the  
// 12 kinds of push/pull suppliers/consumers.  The classes can    
// can be extended for use in real applications: you can copy
// and edit one of the classes to suit your needs, or you can
// define a class that inherits from one of the classes and  
// override the behavior you want to change.  
//    
// => Look at the example programs for simple uses of
//    these classes, including how to instantiate them.
//    Also, the file READING_THE_CODE has more details. 
//    
// Each class has a static `create' member that should be used
// to construct instances of the class.  It takes many arguments,
// most of them optional.  (We apoligize for this ugly API -- we
// wanted a flexible example, but perhaps there is a better
// aproach!)  The arguments:
//
//  + (required) a reference to the ORB
//
//  + (required) a reference to a notification channel 
//
//  + (optional) a max number of events to push/pull   
//    [default zero means push or pull indefinetely]   
//
//  + (optional) a batch size (only relevant for batch suppliers)
//    [default is five]   
//
//  + (optional) a string name for the object
//    [default is the class name]  
//
//  + (optional) a file pathname for storing the IOR of the proxy
//    for this client (default empty means do not save IOR to file)
//
//  + (optional) a function ptr for user-defined push/pull.
//    For consumers the function 'consumes' an event / batch,    
//    for suppliers the function 'fills in' an event / batch.    
//    We pass in the object name and an event count to these
//    functions to make it easy to generate unique events or to  
//    report on the event count.   
//    [default is to use the example version of the appropriate  
//    user-defined function from sample_functions.h   
//
//  + (optional) a function ptr for user-defined offer_change    
//    or subscription_change handling. We pass in object name.   
//    [default is to use the example version of the appropriate  
//    user-defined function from sample_functions.h.]
//    If an explicit NULL ptr is specified, then no change   
//    function is used (the client turns off the change messages).
//
//  + (optional) a pointer to an event type sequence --
//    used to construct a filter to be attached to the client's proxy
//    [default nil means ignore]
//  + (optional) a constraint expression ptr --
//    used to construct a filter to be attached to the client's proxy
//    [default nil means ignore]
//
//  + (optional) a time in milliseconds to wait between each
//    push/pull (for push suppliers / pull consumers)
//    [default zero means push/pull as fast as possible]
//    For pull suppliers / push consumers, milliseconds is
//    used to control the interval between pings for a background
//    thread which keeps track of whether the client's proxy
//    is still alive.  [In this case default zero means
//    do not use a ping thread.]
//
//  + (optional) a verbose flag which if true will cause simple  
//    descriptive messages to go to cout
//    [default false means do not output these messages]    
//    
// During construction, the client obtains an appropriate proxy   
// object from the channel but does not connect itself to this    
// proxy yet. The type sequence and constraint expression are used
// to specify a filter to be added to the proxy.  If both are 
// empty, they are ignored (no filter is used).
//    
// A connect call is used to connect the client to its proxy and  
// spawn a thread for doing push/pull work or for doing pinging.
// A return result of true means there was an immediate error,
// e.g., failure to connect.  False means the connect succeeded.
//
// Once a client c is connected, use
//    CORBA::Boolean com_error = c->wait_done();
// to wait for the max_events to be reached. If max_events is
// zero this will be an infinite wait (unless an error occurs
// causing the client to give up on further use of the channel).
// The return result indicates whether a communication error
// (appears to have) occurred while invoking a method on
// the client's proxy.  If the result is true, this means the
// desired # of events was probably not processed successfully.
//    
// The object and POA should be activated before calling connect.
// -------------------------------------------------------------- 

#ifndef _SAMPLE_CLIENTS_H_
#define _SAMPLE_CLIENTS_H_

#include "corba_wrappers.h"
#include "thread_wrappers.h"
#include "CosNotifyShorthands.h"
#include "sample_functions.h"

WRAPPED_DECL_UNBOUNDED_SEQUENCE_TYPE(CosNF::Filter_var, FilterSeq);

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

///////////////////////////////////////////////////////////////////
//                   PUSH  CONSUMER  EXAMPLES                    //
///////////////////////////////////////////////////////////////////

// ---------------- CosNotifyComm::PushConsumer ---------------- //

class PushConsumer_i :
  public GenericWorker,
  public POA_CosNotifyComm::PushConsumer,
  public PortableServer::RefCountServantBase
{
public:
  PushConsumer_i(CosNA::ProxyPushSupplier_ptr proxy, CosNA::ConsumerAdmin_ptr admin, CosNF::Filter_ptr filter,
		 CORBA::ULong max_events,
		 const char* objnm, consume_any_fn* consume_fn, type_change_fn* change_fn,
		 CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static PushConsumer_i* 
  create(CORBA::ORB_ptr orb,
	 CosNA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 CORBA::ULong batch_size = 0,
	 const char* objnm = "PushConsumer_i",
	 const char* proxy_ior_file = "",
	 const char* admin_ior_file = "",
	 consume_any_fn* consume_fn = sample_consume_any_fn,
	 type_change_fn* change_fn = sample_offer_change_fn,
	 CosN::EventTypeSeq* evs_ptr = 0,
	 const char* constraint_expr = "",
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  void push(const CORBA::Any & data);
  void disconnect_push_consumer();
  void offer_change(const CosN::EventTypeSeq& added,
		    const CosN::EventTypeSeq& deled);

  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosNA::ProxyPushSupplier_var _my_proxy;
  CosNA::ConsumerAdmin_var     _my_admin;
  FilterSeq                   _my_filters;
  const char*                 _obj_name;
  consume_any_fn*             _consume_fn;
  type_change_fn*             _change_fn;
  CORBA::ULong                _num_changes;
  CORBA::ULong                _num_events;
  CORBA::ULong                _max_events;
  CORBA::ULong                _millisecs;
  CORBA::ULong                _delay_millisecs;
  CORBA::Boolean              _verbose;
  CORBA::Boolean              _done;
  CORBA::Boolean              _com_err;
  TW_Mutex                    _oplock;
  TW_Condition                _finish;
  GenericBoundWorkerThread*   _worker;
};

// -------------- CosNotifyComm::StructuredPushConsumer -------------- //

class StructuredPushConsumer_i :
  public GenericWorker,
  public POA_CosNotifyComm::StructuredPushConsumer,
  public PortableServer::RefCountServantBase
{
public:
  StructuredPushConsumer_i(CosNA::StructuredProxyPushSupplier_ptr proxy, CosNA::ConsumerAdmin_ptr admin, CosNF::Filter_ptr filter,
			   CORBA::ULong max_events,
			   const char* objnm, consume_structured_fn* consume_fn, type_change_fn* change_fn,
			   CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static StructuredPushConsumer_i*
  create(CORBA::ORB_ptr orb,
	 CosNA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 CORBA::ULong batch_size = 0,
	 const char* objnm = "StructuredPushConsumer_i",
	 const char* proxy_ior_file = "",
	 const char* admin_ior_file = "",
	 consume_structured_fn* consume_fn = sample_consume_structured_fn,
	 type_change_fn* change_fn = sample_offer_change_fn,
	 CosN::EventTypeSeq* evs_ptr = 0,
	 const char* constraint_expr = "",
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  void push_structured_event(const CosN::StructuredEvent & data);
  void disconnect_structured_push_consumer();
  void offer_change(const CosN::EventTypeSeq& added,
		    const CosN::EventTypeSeq& deled);

  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosNA::StructuredProxyPushSupplier_var _my_proxy;
  CosNA::ConsumerAdmin_var               _my_admin;
  FilterSeq                             _my_filters;
  const char*                           _obj_name;
  consume_structured_fn*                _consume_fn;
  type_change_fn*                       _change_fn;
  CORBA::ULong                          _num_changes;
  CORBA::ULong                          _num_events;
  CORBA::ULong                          _max_events;
  CORBA::ULong                          _millisecs;
  CORBA::ULong                          _delay_millisecs;
  CORBA::Boolean                        _verbose;
  CORBA::Boolean                        _done;
  CORBA::Boolean                        _com_err;
  TW_Mutex                              _oplock;
  TW_Condition                          _finish;
  GenericBoundWorkerThread*             _worker;
};

// 512 version //

class Structured512PushConsumer_i :
  public GenericWorker,
  public POA_CosNotifyComm::StructuredPushConsumer,
  public PortableServer::RefCountServantBase
{
public:
  Structured512PushConsumer_i(CosNA::StructuredProxyPushSupplier_ptr proxy, CosNA::ConsumerAdmin_ptr admin, CosNF::Filter_ptr filter,
			      CORBA::ULong max_events,
			      const char* objnm, consume_structured_fn* consume_fn, type_change_fn* change_fn,
			      CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static Structured512PushConsumer_i*
  create(CORBA::ORB_ptr orb,
	 CosNA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 CORBA::ULong batch_size = 0,
	 const char* objnm = "Structured512PushConsumer_i",
	 const char* proxy_ior_file = "",
	 const char* admin_ior_file = "",
	 consume_structured_fn* consume_fn = consume512_structured_fn,
	 type_change_fn* change_fn = sample_offer_change_fn,
	 CosN::EventTypeSeq* evs_ptr = 0,
	 const char* constraint_expr = "",
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  void push_structured_event(const CosN::StructuredEvent & data);
  void disconnect_structured_push_consumer();
  void offer_change(const CosN::EventTypeSeq& added,
		    const CosN::EventTypeSeq& deled);

  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosNA::StructuredProxyPushSupplier_var _my_proxy;
  CosNA::ConsumerAdmin_var               _my_admin;
  FilterSeq                             _my_filters;
  const char*                           _obj_name;
  consume_structured_fn*                _consume_fn;
  type_change_fn*                       _change_fn;
  CORBA::ULong                          _num_changes;
  CORBA::ULong                          _num_events;
  CORBA::ULong                          _max_events;
  CORBA::ULong                          _millisecs;
  CORBA::ULong                          _delay_millisecs;
  CORBA::Boolean                        _verbose;
  CORBA::Boolean                        _done;
  CORBA::Boolean                        _com_err;
  TW_Mutex                              _oplock;
  TW_Condition                          _finish;
  GenericBoundWorkerThread*             _worker;
};

// ------------- CosNotifyComm::SequencePushConsumer ------------- //

class SequencePushConsumer_i :
  public GenericWorker,
  public POA_CosNotifyComm::SequencePushConsumer,
  public PortableServer::RefCountServantBase
{
public:
  SequencePushConsumer_i(CosNA::SequenceProxyPushSupplier_ptr proxy, CosNA::ConsumerAdmin_ptr admin, CosNF::Filter_ptr filter,
			 CORBA::ULong max_events,
			 const char* objnm, consume_batch_fn* consume_fn, type_change_fn* change_fn,
			 CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static SequencePushConsumer_i*
  create(CORBA::ORB_ptr orb,
	 CosNA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 CORBA::ULong batch_size = 0,
	 const char* objnm = "SequencePushConsumer_i",
	 const char* proxy_ior_file = "",
	 const char* admin_ior_file = "",
	 consume_batch_fn* consume_fn = sample_consume_batch_fn,
	 type_change_fn* change_fn = sample_offer_change_fn,
	 CosN::EventTypeSeq* evs_ptr = 0,
	 const char* constraint_expr = "",
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  void push_structured_events(const CosN::EventBatch& data);
  void disconnect_sequence_push_consumer();
  void offer_change(const CosN::EventTypeSeq& added,
		    const CosN::EventTypeSeq& deled);

  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosNA::SequenceProxyPushSupplier_var _my_proxy;
  CosNA::ConsumerAdmin_var             _my_admin;
  FilterSeq                           _my_filters;
  const char*                         _obj_name;
  consume_batch_fn*                   _consume_fn;
  type_change_fn*                     _change_fn;
  CORBA::ULong                        _num_changes;
  CORBA::ULong                        _num_events;
  CORBA::ULong                        _num_batches;
  CORBA::ULong                        _max_events;
  CORBA::ULong                        _millisecs;
  CORBA::ULong                        _delay_millisecs;
  CORBA::Boolean                      _verbose;
  CORBA::Boolean                      _done;
  CORBA::Boolean                      _com_err;
  TW_Mutex                            _oplock;
  TW_Condition                        _finish;
  GenericBoundWorkerThread*           _worker;
};

///////////////////////////////////////////////////////////////////
//                   PULL  CONSUMER  EXAMPLES                    //
///////////////////////////////////////////////////////////////////

// ---------------- CosNotifyComm::PullConsumer ---------------- //

class PullConsumer_i :
  public GenericWorker,
  public POA_CosNotifyComm::PullConsumer,
  public PortableServer::RefCountServantBase
{
public:
  PullConsumer_i(CosNA::ProxyPullSupplier_ptr proxy, CosNA::ConsumerAdmin_ptr admin, CosNF::Filter_ptr filter,
		 CORBA::ULong max_events,
		 const char* objnm, consume_any_fn* consume_fn, type_change_fn* change_fn,
		 CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static PullConsumer_i*
  create(CORBA::ORB_ptr orb,
	 CosNA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 CORBA::ULong batch_size = 0,
	 const char* objnm = "PullConsumer_i",
	 const char* proxy_ior_file = "",
	 const char* admin_ior_file = "",
	 consume_any_fn* consume_fn = sample_consume_any_fn,
	 type_change_fn* change_fn = sample_offer_change_fn,
	 CosN::EventTypeSeq* evs_ptr = 0,
	 const char* constraint_expr = "",
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  void disconnect_pull_consumer();
  void offer_change(const CosN::EventTypeSeq& added,
		    const CosN::EventTypeSeq& deled);

  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosNA::ProxyPullSupplier_var _my_proxy;
  CosNA::ConsumerAdmin_var     _my_admin;
  FilterSeq                   _my_filters;
  const char*                 _obj_name;
  consume_any_fn*             _consume_fn;
  type_change_fn*             _change_fn;
  CORBA::ULong                _num_changes;
  CORBA::ULong                _num_events;
  CORBA::ULong                _max_events;
  CORBA::ULong                _millisecs;
  CORBA::ULong                _delay_millisecs;
  CORBA::Boolean              _verbose;
  CORBA::Boolean              _done;
  CORBA::Boolean              _com_err;
  TW_Mutex                    _oplock;
  TW_Condition                _finish;
  GenericBoundWorkerThread*   _worker;
};

// ---------------- CosNotifyComm::StructuredPullConsumer ---------------- //

class StructuredPullConsumer_i :
  public GenericWorker,
  public POA_CosNotifyComm::StructuredPullConsumer,
  public PortableServer::RefCountServantBase
{
public:
  StructuredPullConsumer_i(CosNA::StructuredProxyPullSupplier_ptr proxy, CosNA::ConsumerAdmin_ptr admin, CosNF::Filter_ptr filter,
			   CORBA::ULong max_events,
			   const char* objnm, consume_structured_fn* consume_fn, type_change_fn* change_fn,
			   CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static StructuredPullConsumer_i*
  create(CORBA::ORB_ptr orb,
	 CosNA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 CORBA::ULong batch_size = 0,
	 const char* objnm = "StructuredPullConsumer_i",
	 const char* proxy_ior_file = "",
	 const char* admin_ior_file = "",
	 consume_structured_fn* consume_fn = sample_consume_structured_fn,
	 type_change_fn* change_fn = sample_offer_change_fn,
	 CosN::EventTypeSeq* evs_ptr = 0,
	 const char* constraint_expr = "",
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  void disconnect_structured_pull_consumer();
  void offer_change(const CosN::EventTypeSeq& added,
		    const CosN::EventTypeSeq& deled);

  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosNA::StructuredProxyPullSupplier_var _my_proxy;
  CosNA::ConsumerAdmin_var               _my_admin;
  FilterSeq                             _my_filters;
  const char*                           _obj_name;
  consume_structured_fn*                _consume_fn;
  type_change_fn*                       _change_fn;
  CORBA::ULong                          _num_changes;
  CORBA::ULong                          _num_events;
  CORBA::ULong                          _max_events;
  CORBA::ULong                          _millisecs;
  CORBA::ULong                          _delay_millisecs;
  CORBA::Boolean                        _verbose;
  CORBA::Boolean                        _done;
  CORBA::Boolean                        _com_err;
  TW_Mutex                              _oplock;
  TW_Condition                          _finish;
  GenericBoundWorkerThread*             _worker;
};

// ---------------- CosNotifyComm::SequencePullConsumer ---------------- //

class SequencePullConsumer_i :
  public GenericWorker,
  public POA_CosNotifyComm::SequencePullConsumer,
  public PortableServer::RefCountServantBase
{
public:
  SequencePullConsumer_i(CosNA::SequenceProxyPullSupplier_ptr proxy, CosNA::ConsumerAdmin_ptr admin, CosNF::Filter_ptr filter,
			 CORBA::ULong max_events,
			 const char* objnm, consume_batch_fn* consume_fn, type_change_fn* change_fn,
			 CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static SequencePullConsumer_i*
  create(CORBA::ORB_ptr orb,
	 CosNA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 CORBA::ULong batch_size = 0,
	 const char* objnm = "SequencePullConsumer_i",
	 const char* proxy_ior_file = "",
	 const char* admin_ior_file = "",
	 consume_batch_fn* consume_fn = sample_consume_batch_fn,
	 type_change_fn* change_fn = sample_offer_change_fn,
	 CosN::EventTypeSeq* evs_ptr = 0,
	 const char* constraint_expr = "",
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  void disconnect_sequence_pull_consumer();
  void offer_change(const CosN::EventTypeSeq& added,
		    const CosN::EventTypeSeq& deled);

  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosNA::SequenceProxyPullSupplier_var _my_proxy;
  CosNA::ConsumerAdmin_var             _my_admin;
  FilterSeq                           _my_filters;
  const char*                         _obj_name;
  consume_batch_fn*                   _consume_fn;
  type_change_fn*                     _change_fn;
  CORBA::ULong                        _num_changes;
  CORBA::ULong                        _num_events;
  CORBA::ULong                        _num_batches;
  CORBA::ULong                        _max_events;
  CORBA::ULong                        _millisecs;
  CORBA::ULong                        _delay_millisecs;
  CORBA::Boolean                      _verbose;
  CORBA::Boolean                      _done;
  CORBA::Boolean                      _com_err;
  TW_Mutex                            _oplock;
  TW_Condition                        _finish;
  GenericBoundWorkerThread*           _worker;
};


////////////////////////////////////////////////////////////////////
//                   PUSH  SUPPLIER  EXAMPLES                     //
//                                                                //
// N.B.: For these push suppliers, the push thread does           //
// the equivalent of a local try_pull loop, attempting to get     //
// an event/batch from the user-defiend supply function every     //
// millisecs milliseconds. Setting millisecs to zero does rapid   //
// polling; a bad idea unless your supply function always returns //
// TRUE with a valid event, so that millisecs doesn't matter.     //
// (millisecs is also used as period between successful pushes)   //
//                                                                //
////////////////////////////////////////////////////////////////////

// ---------------- CosNotifyComm::PushSupplier ---------------- //

class PushSupplier_i :
  public GenericWorker,
  public POA_CosNotifyComm::PushSupplier,
  public PortableServer::RefCountServantBase
{
public:
  PushSupplier_i(CosNA::ProxyPushConsumer_ptr proxy, CosNA::SupplierAdmin_ptr admin, CosNF::Filter_ptr filter,
		 CORBA::ULong max_events,
		 const char* objnm, supply_any_fn* supply_fn, type_change_fn* change_fn,
		 CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static PushSupplier_i*
  create(CORBA::ORB_ptr orb,
	 CosNA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 CORBA::ULong batch_size = 0,
	 const char* objnm = "PushSupplier_i",
	 const char* proxy_ior_file = "",
	 const char* admin_ior_file = "",
	 supply_any_fn* supply_fn = sample_supply_any_fn,
	 type_change_fn* change_fn = sample_subscription_change_fn,
	 CosN::EventTypeSeq* evs_ptr = 0,
	 const char* constraint_expr = "",
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  void disconnect_push_supplier();
  void subscription_change(const CosN::EventTypeSeq& added,
			   const CosN::EventTypeSeq& deled);
  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosNA::ProxyPushConsumer_var _my_proxy;
  CosNA::SupplierAdmin_var     _my_admin;
  FilterSeq                   _my_filters;
  const char*                 _obj_name;
  supply_any_fn*              _supply_fn;
  type_change_fn*             _change_fn;
  CORBA::ULong                _num_changes;
  CORBA::ULong                _num_events;
  CORBA::ULong                _max_events;
  CORBA::ULong                _millisecs;
  CORBA::ULong                _delay_millisecs;
  CORBA::Boolean              _verbose;
  CORBA::Boolean              _done;
  CORBA::Boolean              _com_err;
  TW_Mutex                    _oplock;
  TW_Condition                _finish;
  GenericBoundWorkerThread*   _worker;
};

// ---------------- CosNotifyComm::StructuredPushSupplier ---------------- //

class StructuredPushSupplier_i :
  public GenericWorker,
  public POA_CosNotifyComm::StructuredPushSupplier,
  public PortableServer::RefCountServantBase
{
public:
  StructuredPushSupplier_i(CosNA::StructuredProxyPushConsumer_ptr proxy, CosNA::SupplierAdmin_ptr admin, CosNF::Filter_ptr filter,
			   CORBA::ULong max_events,
			   const char* objnm, supply_structured_fn* supply_fn, type_change_fn* change_fn,
			   CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static StructuredPushSupplier_i*
  create(CORBA::ORB_ptr orb,
	 CosNA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 CORBA::ULong batch_size = 0,
	 const char* objnm = "StructuredPushSupplier_i",
	 const char* proxy_ior_file = "",
	 const char* admin_ior_file = "",
	 supply_structured_fn* supply_fn = sample_supply_structured_fn,
	 type_change_fn* change_fn = sample_subscription_change_fn,
	 CosN::EventTypeSeq* evs_ptr = 0,
	 const char* constraint_expr = "",
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  void disconnect_structured_push_supplier();
  void subscription_change(const CosN::EventTypeSeq& added,
			   const CosN::EventTypeSeq& deled);
  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosNA::StructuredProxyPushConsumer_var _my_proxy;
  CosNA::SupplierAdmin_var               _my_admin;
  FilterSeq                             _my_filters;
  const char*                           _obj_name;
  supply_structured_fn*                 _supply_fn;
  type_change_fn*                       _change_fn;
  CORBA::ULong                          _num_changes;
  CORBA::ULong                          _num_events;
  CORBA::ULong                          _max_events;
  CORBA::ULong                          _millisecs;
  CORBA::ULong                          _delay_millisecs;
  CORBA::Boolean                        _verbose;
  CORBA::Boolean                        _done;
  CORBA::Boolean                        _com_err;
  TW_Mutex                              _oplock;
  TW_Condition                          _finish;
  GenericBoundWorkerThread*             _worker;
};

// 512 version //

class Structured512PushSupplier_i :
  public GenericWorker,
  public POA_CosNotifyComm::StructuredPushSupplier,
  public PortableServer::RefCountServantBase
{
public:
  Structured512PushSupplier_i(CosNA::StructuredProxyPushConsumer_ptr proxy, CosNA::SupplierAdmin_ptr admin, CosNF::Filter_ptr filter,
			      CORBA::ULong max_events,
			      const char* objnm, supply_structured_fn* supply_fn, type_change_fn* change_fn,
			      CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static Structured512PushSupplier_i*
  create(CORBA::ORB_ptr orb,
	 CosNA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 CORBA::ULong batch_size = 0,
	 const char* objnm = "Structured512PushSupplier_i",
	 const char* proxy_ior_file = "",
	 const char* admin_ior_file = "",
	 supply_structured_fn* supply_fn = sample_supply_structured_fn,
	 type_change_fn* change_fn = sample_subscription_change_fn,
	 CosN::EventTypeSeq* evs_ptr = 0,
	 const char* constraint_expr = "",
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  void disconnect_structured_push_supplier();
  void subscription_change(const CosN::EventTypeSeq& added,
			   const CosN::EventTypeSeq& deled);
  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosNA::StructuredProxyPushConsumer_var _my_proxy;
  CosNA::SupplierAdmin_var               _my_admin;
  FilterSeq                             _my_filters;
  const char*                           _obj_name;
  supply_structured_fn*                 _supply_fn;
  type_change_fn*                       _change_fn;
  CORBA::ULong                          _num_changes;
  CORBA::ULong                          _num_events;
  CORBA::ULong                          _max_events;
  CORBA::ULong                          _millisecs;
  CORBA::ULong                          _delay_millisecs;
  CORBA::Boolean                        _verbose;
  CORBA::Boolean                        _done;
  CORBA::Boolean                        _com_err;
  TW_Mutex                              _oplock;
  TW_Condition                          _finish;
  GenericBoundWorkerThread*             _worker;
};

// ---------------- CosNotifyComm::SequencePushSupplier ---------------- //

class SequencePushSupplier_i :
  public GenericWorker,
  public POA_CosNotifyComm::SequencePushSupplier,
  public PortableServer::RefCountServantBase
{
public:
  SequencePushSupplier_i(CosNA::SequenceProxyPushConsumer_ptr proxy, CosNA::SupplierAdmin_ptr admin, CosNF::Filter_ptr filter,
			 CORBA::ULong max_events, CORBA::ULong batch_size,
			 const char* objnm, supply_batch_fn* supply_fn, type_change_fn* change_fn,
			 CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static SequencePushSupplier_i*
  create(CORBA::ORB_ptr orb,
	 CosNA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 CORBA::ULong batch_size = 0,
	 const char* objnm = "SequencePushSupplier_i",
	 const char* proxy_ior_file = "",
	 const char* admin_ior_file = "",
	 supply_batch_fn* supply_fn = sample_supply_batch_fn,
	 type_change_fn* change_fn = sample_subscription_change_fn,
	 CosN::EventTypeSeq* evs_ptr = 0,
	 const char* constraint_expr = "",
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  void disconnect_sequence_push_supplier();
  void subscription_change(const CosN::EventTypeSeq& added,
			   const CosN::EventTypeSeq& deled);
  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosNA::SequenceProxyPushConsumer_var _my_proxy;
  CosNA::SupplierAdmin_var             _my_admin;
  FilterSeq                           _my_filters;
  const char*                         _obj_name;
  supply_batch_fn*                    _supply_fn;
  type_change_fn*                     _change_fn;
  CORBA::ULong                        _num_changes;
  CORBA::ULong                        _num_events;
  CORBA::ULong                        _batch_size;
  CORBA::ULong                        _num_batches;
  CORBA::ULong                        _max_events;
  CORBA::ULong                        _millisecs;
  CORBA::ULong                        _delay_millisecs;
  CORBA::Boolean                      _verbose;
  CORBA::Boolean                      _done;
  CORBA::Boolean                      _com_err;
  TW_Mutex                            _oplock;
  TW_Condition                        _finish;
  GenericBoundWorkerThread*           _worker;
};

////////////////////////////////////////////////////////////////////
//                   PULL  SUPPLIER  EXAMPLES                     //
//                                                                //
// N.B.: For these pull suppliers, the pull() variants do         //
// the equivalent of a local try_pull loop, attempting to get     //
// an event/batch from the user-defined supply function every     //
// millisecs milliseconds. Setting millisecs to zero does rapid   //
// polling; a bad idea unless your supply function always returns //
// TRUE with a valid event, so that millisecs doesn't matter.     //
//                                                                //
////////////////////////////////////////////////////////////////////

// ---------------- CosNotifyComm::PullSupplier ---------------- //

class PullSupplier_i :
  public GenericWorker,
  public POA_CosNotifyComm::PullSupplier,
  public PortableServer::RefCountServantBase
{
public:
  PullSupplier_i(CosNA::ProxyPullConsumer_ptr proxy, CosNA::SupplierAdmin_ptr admin, CosNF::Filter_ptr filter,
		 CORBA::ULong max_events,
		 const char* objnm, supply_any_fn* supply_fn, type_change_fn* change_fn,
		 CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static PullSupplier_i*
  create(CORBA::ORB_ptr orb,
	 CosNA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 CORBA::ULong batch_size = 0,
	 const char* objnm = "PullSupplier_i",
	 const char* proxy_ior_file = "",
	 const char* admin_ior_file = "",
	 supply_any_fn* supply_fn = sample_supply_any_fn,
	 type_change_fn* change_fn = sample_subscription_change_fn,
	 CosN::EventTypeSeq* evs_ptr = 0,
	 const char* constraint_expr = "",
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  CORBA::Any* pull();
  CORBA::Any* try_pull(CORBA::Boolean& has_event);
  void        disconnect_pull_supplier();
  void        subscription_change(const CosN::EventTypeSeq& added,
				  const CosN::EventTypeSeq& deled);
  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosNA::ProxyPullConsumer_var _my_proxy;
  CosNA::SupplierAdmin_var     _my_admin;
  FilterSeq                   _my_filters;
  const char*                 _obj_name;
  supply_any_fn*              _supply_fn;
  type_change_fn*             _change_fn;
  CORBA::ULong                _num_changes;
  CORBA::ULong                _num_events;
  CORBA::ULong                _max_events;
  CORBA::ULong                _millisecs;
  CORBA::ULong                _delay_millisecs;
  CORBA::Boolean              _verbose;
  CORBA::Boolean              _done;
  CORBA::Boolean              _com_err;
  TW_Mutex                    _oplock;
  TW_Condition                _finish;
  GenericBoundWorkerThread*   _worker;
};

// ------------ CosNotifyComm::StructuredPullSupplier ------------ //

class StructuredPullSupplier_i :
  public GenericWorker,
  public POA_CosNotifyComm::StructuredPullSupplier,
  public PortableServer::RefCountServantBase
{
public:
  StructuredPullSupplier_i(CosNA::StructuredProxyPullConsumer_ptr proxy, CosNA::SupplierAdmin_ptr admin, CosNF::Filter_ptr filter,
			   CORBA::ULong max_events,
			   const char* objnm, supply_structured_fn* supply_fn, type_change_fn* change_fn,
			   CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static StructuredPullSupplier_i*
  create(CORBA::ORB_ptr orb,
	 CosNA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 CORBA::ULong batch_size = 0,
	 const char* objnm = "StructuredPullSupplier_i",
	 const char* proxy_ior_file = "",
	 const char* admin_ior_file = "",
	 supply_structured_fn* supply_fn = sample_supply_structured_fn,
	 type_change_fn* change_fn = sample_subscription_change_fn,
	 CosN::EventTypeSeq* evs_ptr = 0,
	 const char* constraint_expr = "",
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  CosN::StructuredEvent* pull_structured_event();
  CosN::StructuredEvent* try_pull_structured_event(CORBA::Boolean& );
  void                  disconnect_structured_pull_supplier();
  void                  subscription_change(const CosN::EventTypeSeq& added,
					    const CosN::EventTypeSeq& deled);
  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosNA::StructuredProxyPullConsumer_var _my_proxy;
  CosNA::SupplierAdmin_var               _my_admin;
  FilterSeq                             _my_filters;
  const char*                           _obj_name;
  supply_structured_fn*                 _supply_fn;
  type_change_fn*                       _change_fn;
  CORBA::ULong                          _num_changes;
  CORBA::ULong                          _num_events;
  CORBA::ULong                          _max_events;
  CORBA::ULong                          _millisecs;
  CORBA::ULong                          _delay_millisecs;
  CORBA::Boolean                        _verbose;
  CORBA::Boolean                        _done;
  CORBA::Boolean                        _com_err;
  TW_Mutex                              _oplock;
  TW_Condition                          _finish;
  GenericBoundWorkerThread*             _worker;
};

// ------------ CosNotifyComm::SequencePullSupplier ------------ //

class SequencePullSupplier_i :
  public GenericWorker,
  public POA_CosNotifyComm::SequencePullSupplier,
  public PortableServer::RefCountServantBase
{
public:
  SequencePullSupplier_i(CosNA::SequenceProxyPullConsumer_ptr proxy, CosNA::SupplierAdmin_ptr admin, CosNF::Filter_ptr filter,
			 CORBA::ULong max_events, CORBA::ULong batch_size,
			 const char* objnm, supply_batch_fn* supply_fn, type_change_fn* change_fn,
			 CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose);

  static SequencePullSupplier_i*
  create(CORBA::ORB_ptr orb,
	 CosNA::EventChannel_ptr channel,
	 CORBA::ULong max_events = 0,
	 CORBA::ULong batch_size = 0,
	 const char* objnm = "SequencePullSupplier_i",
	 const char* proxy_ior_file = "",
	 const char* admin_ior_file = "",
	 supply_batch_fn* supply_fn = sample_supply_batch_fn,
	 type_change_fn* change_fn = sample_subscription_change_fn,
	 CosN::EventTypeSeq* evs_ptr = 0,
	 const char* constraint_expr = "",
	 CORBA::ULong millisecs = 0,
	 CORBA::ULong delay_millisecs = 0,
	 CORBA::Boolean verbose = 0);

  // IDL methods
  CosN::EventBatch* pull_structured_events(CORBA::Long max_number);
  CosN::EventBatch* try_pull_structured_events(CORBA::Long max_number,
					      CORBA::Boolean& has_event);
  void             disconnect_sequence_pull_supplier();
  void             subscription_change(const CosN::EventTypeSeq& added,
				       const CosN::EventTypeSeq& deled);
  // Local methods
  CORBA::Boolean connect();
  CORBA::Boolean wait_done();
  void  cleanup();
  void* start_working(void *);

protected:
  CosNA::SequenceProxyPullConsumer_var _my_proxy;
  CosNA::SupplierAdmin_var             _my_admin;
  FilterSeq                           _my_filters;
  const char*                         _obj_name;
  supply_batch_fn*                    _supply_fn;
  type_change_fn*                     _change_fn;
  CORBA::ULong                        _num_changes;
  CORBA::ULong                        _num_events;
  CORBA::ULong                        _batch_size;
  CORBA::ULong                        _num_batches;
  CORBA::ULong                        _max_events;
  CORBA::ULong                        _millisecs;
  CORBA::ULong                        _delay_millisecs;
  CORBA::Boolean                      _verbose;
  CORBA::Boolean                      _done;
  CORBA::Boolean                      _com_err;
  TW_Mutex                            _oplock;
  TW_Condition                        _finish;
  GenericBoundWorkerThread*           _worker;
};

#endif
