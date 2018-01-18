// -*- Mode: C++; -*-

// -------------------------------------------------------------- 
// See comment at top of sample_clients.h
// -------------------------------------------------------------- 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <iomanip>
using namespace std;

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "thread_wrappers.h"
#include "sample_clients.h"


///////////////////////////////////////////////////////////////////
//                 Optimization Notes                            //
///////////////////////////////////////////////////////////////////
//
// A number of methods have the following form:
//
// <result_type> method_name (...) {
//    if (_done) return <something>;
//    ...
//    TW_SCOPE_LOCK(lock, _oplock, "resource_type", "method_name");
//    if (_done) return <something>;
//    ...
// }
//
// -OR-
//
// void method_name (...) {
//    if (_done) return;
//    ...
//    TW_SCOPE_LOCK(lock, _oplock, "resource_type", "method_name");
//    if (_done) return;
//    ...
// }
//
// The _done check prior to obtaining the lock is an optimization;
// it could be removed and the code would still be correct.
// It is useful because it avoids unnecessary synchronization
// during shutdown/cleanup of the client.
//
// Because _done can change while the lock is not held, any time
// the code first acquires (or releases then re-acquires) _oplock,
// it must do another _done check.  So the 2nd check, which looks
// redundant, is actually required!


///////////////////////////////////////////////////////////////////
//                  HELPER ROUTINES                              //
///////////////////////////////////////////////////////////////////

void destroy_filters(FilterSeq& seq) {
  for (unsigned int i = 0; i < seq.length(); i++) {
    try {
      seq[i]->destroy();
      seq[i] = CosNF::Filter::_nil();
    } catch (...) { }
  } 
  seq.length(0);
}

// This helper routine informs channel that type ANY will be supplied
void offer_any(CosNC::NotifyPublish_ptr proxy, const char* objnm, CORBA::Boolean verbose) {
  if (verbose) cout << objnm << ": offering type %ANY" << endl;
  CosN::EventTypeSeq added, deled;
  added.length(1);
  added[0].domain_name = CORBA::string_dup("");
  added[0].type_name = CORBA::string_dup("%ANY");
  deled.length(0);
  try {
    proxy->offer_change(added, deled);
  } catch (...) {
    if (verbose) cout << "** registration failed **" << endl;
  }
}

// This helper routine informs channel that types Stock::Ticker and Bond::Ticker will be supplied
void offer_ticker(CosNC::NotifyPublish_ptr proxy, const char* objnm, CORBA::Boolean verbose) {
  if (verbose) cout << objnm << ": offering types Stock::Ticker and Bond::Ticker" << endl;
  CosN::EventTypeSeq added, deled;
  added.length(2);
  added[0].domain_name = CORBA::string_dup("Stock");
  added[0].type_name = CORBA::string_dup("Ticker");
  added[1].domain_name = CORBA::string_dup("Bond");
  added[1].type_name = CORBA::string_dup("Ticker");
  deled.length(0);
  try {
    proxy->offer_change(added, deled);
  } catch (...) {
    if (verbose) cout << "** registration failed **" << endl;
  }
}

// This helper routines writes an object's IOR to a file
void write_ior_to_file(CORBA::ORB_ptr orb, CORBA::Object_ptr o,
		       const char* filenm, CORBA::Boolean verbose) {
  if (strlen(filenm) == 0) {
    if (verbose) cout << "ior filename empty -- skipping" << endl;
    return;
  }
  ofstream file(filenm, ios::out);
  if ( ! file ) {
    if (verbose) cerr << "Failed to open IOR file: " << filenm << endl;
    return;
  }
  char* ior_name = orb->object_to_string(o);
  file << ior_name;
  file.close();
  delete [] ior_name;
  if (verbose) cout << "wrote IOR to file: " << filenm << endl;
}

// These 2 helper routines are used to obtain one of the
// 12 kinds of notification channel proxies from a channel;
// they return a nil reference if a failure occurs

CosNA::ProxyConsumer_ptr get_proxy_consumer(CORBA::ORB_ptr orb,
					   CosNA::EventChannel_ptr channel,
					   CosNA::ClientType ctype,
					   CORBA::Boolean push_proxy,
					   const char* admin_ior_file,
					   CosNA::SupplierAdmin_ptr& admin, // out param
					   CORBA::Boolean verbose) {
  CosNA::ProxyConsumer_ptr generic_proxy = CosNA::ProxyConsumer::_nil();
  CosNA::InterFilterGroupOperator ifoper = CosNA::AND_OP;
  //  CosNA::InterFilterGroupOperator ifoper = CosNA::OR_OP;
  CosNA::AdminID admID;
  admin = CosNA::SupplierAdmin::_nil();
  try {
    admin = channel->new_for_suppliers(ifoper, admID);
    if ( CORBA::is_nil(admin) ) {
      cerr << "Failed to obtain admin" << endl;
      return generic_proxy; // failure
    }
  } catch (...) {
    cerr << "Failed to obtain admin" << endl;
    return generic_proxy;  // failure
  }
  if (verbose) cout << "Obtained admin from the channel" << endl;

  CosNA::ProxyID prxID;
  try {
    if (push_proxy) {
      generic_proxy = admin->obtain_notification_push_consumer(ctype, prxID);
    } else {
      generic_proxy = admin->obtain_notification_pull_consumer(ctype, prxID);
    }
  } catch (...) {  }
  if (CORBA::is_nil(generic_proxy)) {
    cerr << "Failed to obtain proxy" << endl;
    try {
      admin->destroy();
    } catch (...) { }
    admin = CosNA::SupplierAdmin::_nil();
    return generic_proxy;
  }
  if (verbose) cout << "Obtained proxy from admin" << endl;
  write_ior_to_file(orb, admin, admin_ior_file, verbose);
  return generic_proxy; // success if generic_proxy is non-nil, otherwise failure
}

CosNA::ProxySupplier_ptr get_proxy_supplier(CORBA::ORB_ptr orb,
					   CosNA::EventChannel_ptr channel,
					   CosNA::ClientType ctype,
					   CORBA::Boolean push_proxy,
					   const char* admin_ior_file,
					   CosNA::ConsumerAdmin_ptr& admin, // out param
					   CORBA::Boolean verbose) {
  CosNA::ProxySupplier_ptr generic_proxy = CosNA::ProxySupplier::_nil();
  CosNA::InterFilterGroupOperator ifoper = CosNA::AND_OP;
  //  CosNA::InterFilterGroupOperator ifoper = CosNA::OR_OP;
  CosNA::AdminID admID;
  admin = CosNA::ConsumerAdmin::_nil();
  try {
    admin = channel->new_for_consumers(ifoper, admID);
    if ( CORBA::is_nil(admin) ) {
      cerr << "Failed to obtain admin" << endl;
      return generic_proxy; // failure
    }
  } catch (...) {
    cerr << "Failed to obtain admin" << endl;
    return generic_proxy;  // failure
  }
  if (verbose) cout << "Obtained admin from the channel" << endl;

  CosNA::ProxyID prxID;
  try {
    if (push_proxy) {
      generic_proxy = admin->obtain_notification_push_supplier(ctype, prxID);
    } else {
      generic_proxy = admin->obtain_notification_pull_supplier(ctype, prxID);
    }
  } catch (...) {  }
  if (CORBA::is_nil(generic_proxy)) {
    cerr << "Failed to obtain proxy" << endl;
    try {
      admin->destroy();
    } catch (...) { }
    admin = CosNA::ConsumerAdmin::_nil();
    return generic_proxy;
  }
  if (verbose) cout << "Obtained proxy from admin" << endl;
  write_ior_to_file(orb, admin, admin_ior_file, verbose);
  return generic_proxy; // success if generic_proxy is non-nil, otherwise failure
}

// This helper routine adds a filter to a FilterAdmin object
// (which could be a supplier or consumer proxy or admin).
// Returns pointer to filter, or nil on error
CORBA::Boolean
sample_add_filter(CosNA::EventChannel_ptr channel, 
		  CosNF::FilterAdmin_ptr fadmin,
		  CosN::EventTypeSeq& evs,
		  const char* constraint_expr,
		  const char* obj_name,
		  CosNF::Filter_ptr& filter, // out param
		  CORBA::Boolean verbose) {
  // if evs and constraint expr are empty, we ignore them + do not add a filter
  if ( (evs.length() == 0) && (strlen(constraint_expr) == 0) ) {
    if (verbose) cout << obj_name << ": (no filter used)" << endl;
    return 0; // OK
  }
  // Obtain a reference to the default filter factory; create a filter object 
  CosNF::FilterFactory_ptr ffp;
  filter = CosNF::Filter::_nil();
  try {
    if (verbose) cout << obj_name << ": Obtaining default filter factory" << endl;
    ffp    = channel->default_filter_factory();  
    filter = ffp->create_filter("EXTENDED_TCL");
  } catch (CORBA::COMM_FAILURE& ex) {
    cerr << obj_name << ": Caught COMM_FAILURE obtaining filter object" << endl;
    return 1; // error
  } catch (...) {
    cerr << obj_name << ": Caught exception obtaining filter object" << endl;
    return 1; // error
  }
  if (verbose) cout << obj_name << ": Obtained filter from default filter factory" << endl;

  // Construct a simple constraint expression; add it to fadmin
  CosNF::ConstraintExpSeq   exp;
  exp.length(1);
  exp[0].event_types = evs;
  exp[0].constraint_expr = CORBA::string_dup(constraint_expr);
  CORBA::Boolean res = 0; // OK
  try {
    if (verbose) cout << obj_name << ": Adding constraints to filter" << endl;
    filter->add_constraints(exp);
    if (verbose) cout << obj_name << ": Adding filter to target" << endl;
    fadmin->add_filter(filter);
    if (verbose) {
      if (evs.length()) {
	cout << obj_name << ": Added filter for types ";
	for (unsigned int j = 0; j < evs.length(); j++) { 
	  cout << (const char*)evs[j].domain_name << "::" << (const char*)evs[j].type_name;
	  if ((j+1) < evs.length())
	    cout << ", ";
	}
      } else {
	cout << obj_name << ": Added filter for type *::* ";
      }
      cout << " and constraint expression \"" << constraint_expr << "\" " << endl;
    }
  }
  catch(CosNF::InvalidConstraint& _exobj1) {
    cerr << obj_name << ": Exception thrown : Invalid constraint given "
	 << (const char *)constraint_expr << endl;
    res = 1; // error
  }
  catch (...) {
    cerr << obj_name << ": Exception thrown while adding constraint " 
	 << (const char *)constraint_expr << endl; 
    res = 1; // error
  }
  if (res == 1) { // error so destroy filter
    try {
      filter->destroy();
    } catch (...) { }
    filter = CosNF::Filter::_nil();
    return 1; // error
  }
  // OK
  return 0; // OK
}

// This helper routine adds a filter to a FilterAdmin object
// (which could be a supplier or consumer proxy or admin)
// where the filter contains 512 constraint exprs.
// Returns pointer to filter, or nil on error
CORBA::Boolean
add512_filter(CosNA::EventChannel_ptr channel, 
	      CosNF::FilterAdmin_ptr fadmin,
	      const char* obj_name,
	      CosNF::Filter_ptr& filter, // out param
	      CORBA::Boolean verbose)
{
  // Obtain a reference to the default filter factory; create a filter object 
  CosNF::FilterFactory_ptr ffp;
  filter = CosNF::Filter::_nil();
  try {
    if (verbose) cout << obj_name << ": Obtaining default filter factory" << endl;
    ffp    = channel->default_filter_factory();  
    filter = ffp->create_filter("EXTENDED_TCL");
  } catch (CORBA::COMM_FAILURE& ex) {
    cerr << obj_name << ": Caught COMM_FAILURE obtaining filter object" << endl;
    return 1; // error
  } catch (...) {
    cerr << obj_name << ": Caught exception obtaining filter object" << endl;
    return 1; // error
  }
  if (verbose) cout << obj_name << ": Obtained filter from default filter factory" << endl;

  // Construct a simple constraint expression; add it to fadmin
  CosNF::ConstraintExpSeq   exp;
  exp.length(1);
  exp[0].event_types.length(1);
  exp[0].event_types[0].domain_name = CORBA::string_dup("Test512");
  exp[0].event_types[0].type_name = CORBA::string_dup("Type_XXX");
  exp[0].constraint_expr = CORBA::string_dup("true");
  CORBA::Boolean res = 0; // OK
  try {
    if (verbose) cout << obj_name << ": Adding initial dummy constraint to filter" << endl;
    if (verbose) {
      cout << obj_name << ": Adding constraint with dummy type Type_XXX and constraint \"true\" to filter" << endl;
    }
    filter->add_constraints(exp);
    if (verbose) cout << obj_name << ": Adding filter to target" << endl;
    fadmin->add_filter(filter);
    for (int i = 0; i < 512; i++) {
      char buf[1024];
      sprintf(buf, "Type_%03d", i);
      exp[0].event_types[0].type_name = CORBA::string_dup(buf);
      if (verbose) {
	cout << obj_name << ": Adding constraint with type " << buf << " and constraint \"true\" to filter" << endl;
      }
      filter->add_constraints(exp);
    }
  }
  catch(CosNF::InvalidConstraint& _exobj1) {
    cerr << obj_name << ": Exception thrown : Invalid constraint given" << endl;
    res = 1; // error
  }
  catch (...) {
    cerr << obj_name << ": Exception thrown while adding constraint/filter" << endl; 
    res = 1; // error
  }

  if (res == 1) { // error so destroy filter
    try {
      filter->destroy();
    } catch (...) { }
    filter = CosNF::Filter::_nil();
    return 1; // error
  }
  // OK
  return 0; // OK
}

///////////////////////////////////////////////////////////////////
//                   PUSH  CONSUMER  EXAMPLES                    //
///////////////////////////////////////////////////////////////////

// ---------------- CosNotifyComm::PushConsumer ---------------- //


PushConsumer_i::
PushConsumer_i(CosNA::ProxyPushSupplier_ptr proxy,
	       CosNA::ConsumerAdmin_ptr admin,
	       CosNF::Filter_ptr filter,
	       CORBA::ULong max_events,const char* objnm,
	       consume_any_fn* consume_fn, type_change_fn* change_fn,
	       CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _my_admin(admin), _my_filters(0),
  _obj_name(objnm), _consume_fn(consume_fn), _change_fn(change_fn),
  _num_changes(0), _num_events(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose), _done(0), _com_err(0),
  _oplock(), _finish(&_oplock), _worker(0)
{
  if (! CORBA::is_nil(filter)) {
    _my_filters.length(1);
    _my_filters[0] = filter;
  }
}

PushConsumer_i*
PushConsumer_i::create(CORBA::ORB_ptr orb,
		       CosNA::EventChannel_ptr channel,
		       CORBA::ULong max_events,
		       CORBA::ULong batch_size,
		       const char* objnm,
		       const char* proxy_ior_file,
		       const char* admin_ior_file,
		       consume_any_fn* consume_fn,
		       type_change_fn* change_fn,
		       CosN::EventTypeSeq* evs_ptr,
		       const char* constraint_expr,
		       CORBA::ULong millisecs,
		       CORBA::ULong delay_millisecs,
		       CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosNA::ConsumerAdmin_ptr admin = CosNA::ConsumerAdmin::_nil();
  CosNA::ProxySupplier_var generic_proxy =
    get_proxy_supplier(orb, channel, CosNA::ANY_EVENT, 1, admin_ior_file, admin, verbose); // 1 is push 0 is pull
  CosNA::ProxyPushSupplier_ptr proxy = CosNA::ProxyPushSupplier::_narrow(generic_proxy);
  if ( CORBA::is_nil(proxy) ) {
    return 0; // get_proxy_supplier failed
  }
  // If evs or constraint_expr are non-empty, add a filter to proxy
  CosNF::Filter_ptr filter = CosNF::Filter::_nil();
  if (evs_ptr) {
    CORBA::Boolean filt_err = sample_add_filter(channel, proxy, *evs_ptr, constraint_expr, objnm, filter, verbose);
    if (filt_err) {
      try {
	admin->destroy();
      } catch (...) { }
      return 0; // adding filter failed
    }
  }
  // write proxy IOR to file
  write_ior_to_file(orb, proxy, proxy_ior_file, verbose);
  // Construct a client
  PushConsumer_i* client =
    new PushConsumer_i(proxy, admin, filter, max_events, objnm, consume_fn, change_fn, millisecs, delay_millisecs, verbose);
  return client;
}

CORBA::Boolean PushConsumer_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "PushConsumer", "PushConsumer_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_any_push_consumer(_this());
    if (_change_fn) {
      _my_proxy->obtain_offered_types(CosNA::NONE_NOW_UPDATES_ON);
    } else {
      _my_proxy->obtain_offered_types(CosNA::NONE_NOW_UPDATES_OFF);
    }
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEvCA::AlreadyConnected& ex) {
    cerr << _obj_name << ": Already connected" << endl;
    return 1; // error
  } catch (...) {
    cerr << _obj_name << ": Failed to connect" << endl;
    return 1; // error
  }
  if (_verbose) cout << _obj_name << ": Connected to proxy, ready to consume events" << endl;
  // if _millisecs is set, spawn a thread to ping the proxy
  if (_millisecs)
    _worker = new GenericBoundWorkerThread(this);
  return 0; // OK
}

void PushConsumer_i::cleanup() {
  CosNA::ProxyPushSupplier_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "PushConsumer", "PushConsumer_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    if (CORBA::is_nil(_my_proxy)) {
      cerr << "Coding error: cleanup already called?" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy
    proxy = _my_proxy;
    _my_proxy = CosNA::ProxyPushSupplier::_nil();
  } // end lock scope
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_push_supplier();
  } catch(...) {}
  try {
    _my_admin->destroy();
  } catch (...) { }
  _my_admin = CosNA::ConsumerAdmin::_nil();
  destroy_filters(_my_filters);
}

// While _millisecs is positive, pings proxy every _millisecs milliseconds.
// Sets error code and sets _done to 1 if there is an error during ping.
void* PushConsumer_i::start_working(void *) {
  CosNC::PushConsumer_var bump_my_refcount_during_outcalls = _this();

  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "PushConsumer", "PushConsumer_i::start_working");
    if (_verbose && (!_done) && _millisecs) cout << _obj_name << ": Spawned thread entering ping loop" << endl;
    while ( 1 ) {
      if (_done || (_millisecs == 0)) break;
      unsigned long t_secs = 0, t_nanosecs = 0;
      unsigned long d_secs = 0, d_nanosecs = 0;
      d_secs = _millisecs / 1000;
      d_nanosecs = (_millisecs % 1000) * 1000000;
      { // introduce temporary lock release scope; do not hold oplock across ping
	TW_SCOPE_RELEASE(lock);
	try {
	  _my_proxy->MyType();
	} catch (...) {
	  goto com_fail;
	}
      } // end temporary lock release scope
      if (_done) break; // must have disconnected during pull
      // sleep for specified interval
      TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
      _finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      continue; // continue ping loop
    com_fail:
      if (_done) break;
      if (_verbose) cout << _obj_name << ": communication error while pinging proxy using MyType()" << endl;
      _done = 1;
      _finish.broadcast();
      _com_err = 1;
      break;
    } // end while loop
  } // end lock scope
  // done
  return 0;
}

void PushConsumer_i::push(const CORBA::Any& data)
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "PushConsumer", "PushConsumer_i::push");
  if (_done) return;
  _num_events++;
  if (_consume_fn)
    (*_consume_fn)(data, _obj_name, _num_events, _verbose);
  else if (_verbose) cout << _obj_name << ": event count = " << _num_events << endl;
  if (_max_events && (_num_events >= _max_events)) {
    if (_verbose) cout << _obj_name << ": DONE [max_events reached]" << endl;
    _done = 1;
    _finish.broadcast();
  }
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying push response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

void PushConsumer_i::disconnect_push_consumer()
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "PushConsumer", "PushConsumer_i::disconnect_push_consumer");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean PushConsumer_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "PushConsumer", "PushConsumer_i::wait_done");
    while (!_done) {
      _finish.wait();
    }
    // using tmp_worker ensures only one wait_done call tries join()
    tmp_worker = _worker;
    _worker = 0;
  } // end lock scope
  if (tmp_worker) {
    tmp_worker->join(0);
  }
  return _com_err;
}

void PushConsumer_i::offer_change(const CosN::EventTypeSeq& added,
				  const CosN::EventTypeSeq& deled)
{
  if (_done) return; // see "Optimization Notes" at top
  TW_SCOPE_LOCK(lock, _oplock, "PushConsumer", "PushConsumer_i::offer_change");
  if (_done) return;
  _num_changes++;
  if (_change_fn) (*_change_fn)(added, deled, _obj_name, _num_changes, _verbose);
  else if (_verbose) cout << _obj_name << ": offer_change received [# " << _num_changes << "]" << endl;
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying offer_change response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

// -------------- CosNotifyComm::StructuredPushConsumer -------------- //

StructuredPushConsumer_i::
StructuredPushConsumer_i(CosNA::StructuredProxyPushSupplier_ptr proxy,
			 CosNA::ConsumerAdmin_ptr admin,
			 CosNF::Filter_ptr filter,
			 CORBA::ULong max_events, const char* objnm,
			 consume_structured_fn* consume_fn, type_change_fn* change_fn,
			 CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _my_admin(admin), _my_filters(0),
  _obj_name(objnm), _consume_fn(consume_fn), _change_fn(change_fn),
  _num_changes(0), _num_events(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose),
  _done(0), _com_err(0), _oplock(), _finish(&_oplock), _worker(0)
{
  if (! CORBA::is_nil(filter)) {
    _my_filters.length(1);
    _my_filters[0] = filter;
  }
}

StructuredPushConsumer_i*
StructuredPushConsumer_i::create(CORBA::ORB_ptr orb,
				 CosNA::EventChannel_ptr channel,
				 CORBA::ULong max_events,
				 CORBA::ULong batch_size,
				 const char* objnm,
				 const char* proxy_ior_file,
				 const char* admin_ior_file,
				 consume_structured_fn* consume_fn,
				 type_change_fn* change_fn,
				 CosN::EventTypeSeq* evs_ptr,
				 const char* constraint_expr,
				 CORBA::ULong millisecs,
				 CORBA::ULong delay_millisecs,
				 CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosNA::ConsumerAdmin_ptr admin = CosNA::ConsumerAdmin::_nil();
  CosNA::ProxySupplier_var generic_proxy =
    get_proxy_supplier(orb, channel, CosNA::STRUCTURED_EVENT, 1, admin_ior_file, admin, verbose); // 1 is push 0 is pull
  CosNA::StructuredProxyPushSupplier_ptr proxy = CosNA::StructuredProxyPushSupplier::_narrow(generic_proxy);
  if ( CORBA::is_nil(proxy) ) {
    return 0; // get_proxy_supplier failed
  }
  // If evs or constraint_expr are non-empty, add a filter to proxy
  CosNF::Filter_ptr filter = CosNF::Filter::_nil();
  if (evs_ptr) {
    CORBA::Boolean filt_err = sample_add_filter(channel, proxy, *evs_ptr, constraint_expr, objnm, filter, verbose);
    if (filt_err) {
      try {
	admin->destroy();
      } catch (...) { }
      return 0; // adding filter failed
    }
  }
  // write proxy IOR to file
  write_ior_to_file(orb, proxy, proxy_ior_file, verbose);
  // Construct a client
  StructuredPushConsumer_i* client =
    new StructuredPushConsumer_i(proxy, admin, filter, max_events, objnm, consume_fn, change_fn, millisecs, delay_millisecs, verbose);
  return client;
}


CORBA::Boolean StructuredPushConsumer_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "StructuredPushConsumer", "StructuredPushConsumer_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_structured_push_consumer(_this());
    if (_change_fn) {
      _my_proxy->obtain_offered_types(CosNA::NONE_NOW_UPDATES_ON);
    } else {
      _my_proxy->obtain_offered_types(CosNA::NONE_NOW_UPDATES_OFF);
    }
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEvCA::AlreadyConnected& ex) {
    cerr << _obj_name << ": Already connected" << endl;
    return 1; // error
  } catch (...) {
    cerr << _obj_name << ": Failed to connect" << endl;
    return 1; // error
  }
  if (_verbose) cout << _obj_name << ": Connected to proxy, ready to consume events" << endl; 
  // if _millisecs is set, spawn a thread to ping the proxy
  if (_millisecs) 
    _worker = new GenericBoundWorkerThread(this);
  return 0; // OK
}

void StructuredPushConsumer_i::cleanup() {
  CosNA::StructuredProxyPushSupplier_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "StructuredPushConsumer", "StructuredPushConsumer_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    if (CORBA::is_nil(_my_proxy)) {
      cerr << "Coding error: cleanup already called?" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosNA::StructuredProxyPushSupplier::_nil();
  } // end lock scope
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_structured_push_supplier();
  } catch(...) {}
  try {
    _my_admin->destroy();
  } catch (...) { }
  _my_admin = CosNA::ConsumerAdmin::_nil();
  destroy_filters(_my_filters);
}

// While _millisecs is positive, pings proxy every _millisecs milliseconds.
// Sets error code and sets _done to 1 if there is an error during ping.
void* StructuredPushConsumer_i::start_working(void *) {
  CosNC::StructuredPushConsumer_var bump_my_refcount_during_outcalls = _this();

  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "StructuredPushConsumer", "StructuredPushConsumer_i::start_working");

    if (_verbose && (!_done) && _millisecs) cout << _obj_name << ": Spawned thread entering ping loop" << endl;
    while ( 1 ) {
      if (_done || (_millisecs == 0)) break;
      unsigned long t_secs = 0, t_nanosecs = 0;
      unsigned long d_secs = 0, d_nanosecs = 0;
      d_secs = _millisecs / 1000;
      d_nanosecs = (_millisecs % 1000) * 1000000;
      { // introduce temporary lock release scope; do not hold oplock across ping
	TW_SCOPE_RELEASE(lock);
	try {
	  _my_proxy->MyType();
	} catch (...) {
	  goto com_fail;
	}
      } // end temporary lock release scope
      if (_done) break; // must have disconnected during pull
      // sleep for specified interval
      TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
      _finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      continue; // continue ping loop
    com_fail:
      if (_done) break;
      if (_verbose) cout << _obj_name << ": communication error while pinging proxy using MyType()" << endl;
      _done = 1;
      _finish.broadcast();
      _com_err = 1;
      break;
    } // end while loop
  } // end lock scope
  return 0;
}

void StructuredPushConsumer_i::push_structured_event(const CosN::StructuredEvent& data)
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "StructuredPushConsumer", "StructuredPushConsumer_i::push_structured_event");
  if (_done) return;
  _num_events++;
  if (_consume_fn)
    (*_consume_fn)(data, _obj_name, _num_events, _verbose);
  else if (_verbose) cout << _obj_name << ": event count = " << _num_events << endl;
  if (_max_events && (_num_events >= _max_events)) {
    if (_verbose) cout << _obj_name << ": DONE [max_events reached]" << endl;
    _done = 1;
    _finish.broadcast();
  }
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying push_structured_event response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

void StructuredPushConsumer_i::disconnect_structured_push_consumer()
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "StructuredPushConsumer", "StructuredPushConsumer_i::disconnect_structured_push_consumer");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean StructuredPushConsumer_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "StructuredPushConsumer", "StructuredPushConsumer_i::wait_done");
    while (!_done) {
      _finish.wait();
    }
    // using tmp_worker ensures only one wait_done call tries join()
    tmp_worker = _worker;
    _worker = 0;
  } // end lock scope
  if (tmp_worker) {
    tmp_worker->join(0);
  }
  return _com_err;
}

void StructuredPushConsumer_i::offer_change(const CosN::EventTypeSeq& added,
					    const CosN::EventTypeSeq& deled)
{
  if (_done) return; // see "Optimization Notes" at top
  TW_SCOPE_LOCK(lock, _oplock, "StructuredPushConsumer", "StructuredPushConsumer_i::offer_change");
  if (_done) return;
  _num_changes++;
  if (_change_fn) (*_change_fn)(added, deled, _obj_name, _num_changes, _verbose);
  else if (_verbose) cout << _obj_name << ": offer_change received [# " << _num_changes << "]" << endl;
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying offer_change response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

// 512 version //

Structured512PushConsumer_i::
Structured512PushConsumer_i(CosNA::StructuredProxyPushSupplier_ptr proxy,
			    CosNA::ConsumerAdmin_ptr admin,
			    CosNF::Filter_ptr filter,
			    CORBA::ULong max_events, const char* objnm,
			    consume_structured_fn* consume_fn, type_change_fn* change_fn,
			    CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _my_admin(admin), _my_filters(0),
  _obj_name(objnm), _consume_fn(consume_fn), _change_fn(change_fn),
  _num_changes(0), _num_events(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose),
  _done(0), _com_err(0), _oplock(), _finish(&_oplock), _worker(0)
{
  if (! CORBA::is_nil(filter)) {
    _my_filters.length(1);
    _my_filters[0] = filter;
  }
}

Structured512PushConsumer_i*
Structured512PushConsumer_i::create(CORBA::ORB_ptr orb,
				    CosNA::EventChannel_ptr channel,
				    CORBA::ULong max_events,
				    CORBA::ULong batch_size,
				    const char* objnm,
				    const char* proxy_ior_file,
				    const char* admin_ior_file,
				    consume_structured_fn* consume_fn,
				    type_change_fn* change_fn,
				    CosN::EventTypeSeq* evs_ptr,
				    const char* constraint_expr,
				    CORBA::ULong millisecs,
				    CORBA::ULong delay_millisecs,
				    CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosNA::ConsumerAdmin_ptr admin = CosNA::ConsumerAdmin::_nil();
  CosNA::ProxySupplier_var generic_proxy =
    get_proxy_supplier(orb, channel, CosNA::STRUCTURED_EVENT, 1, admin_ior_file, admin, verbose); // 1 is push 0 is pull
  CosNA::StructuredProxyPushSupplier_ptr proxy = CosNA::StructuredProxyPushSupplier::_narrow(generic_proxy);
  if ( CORBA::is_nil(proxy) ) {
    return 0; // get_proxy_supplier failed
  }
#if 0
  // If evs or constraint_expr are non-empty, add a filter to proxy
  CosNF::Filter_ptr filter = CosNF::Filter::_nil();
  if (evs_ptr) {
    CORBA::Boolean filt_err = sample_add_filter(channel, proxy, *evs_ptr, constraint_expr, objnm, filter, verbose);
    if (filt_err) {
      try {
	admin->destroy();
      } catch (...) { }
      return 0; // adding filter failed
    }
  }
#endif
  // Add a single filter that subscribes to all 512 event types.
  // Use filter->add_constraint because this is what Cameron's clients use.
  CosNF::Filter_ptr filter = CosNF::Filter::_nil();
  CORBA::Boolean filt_err = add512_filter(channel, proxy, objnm, filter, verbose);
  if (filt_err) {
    try {
      admin->destroy();
    } catch (...) { }
    return 0; // adding filter failed
  }
  // write proxy IOR to file
  write_ior_to_file(orb, proxy, proxy_ior_file, verbose);
  // Construct a client
  Structured512PushConsumer_i* client =
    new Structured512PushConsumer_i(proxy, admin, filter, max_events, objnm, consume_fn, change_fn, millisecs, delay_millisecs, verbose);
  return client;
}

CORBA::Boolean Structured512PushConsumer_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "Structured512PushConsumer", "Structured512PushConsumer_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_structured_push_consumer(_this());
    if (_change_fn) {
      _my_proxy->obtain_offered_types(CosNA::NONE_NOW_UPDATES_ON);
    } else {
      _my_proxy->obtain_offered_types(CosNA::NONE_NOW_UPDATES_OFF);
    }
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEvCA::AlreadyConnected& ex) {
    cerr << _obj_name << ": Already connected" << endl;
    return 1; // error
  } catch (...) {
    cerr << _obj_name << ": Failed to connect" << endl;
    return 1; // error
  }
  if (_verbose) cout << _obj_name << ": Connected to proxy, ready to consume events" << endl; 
  // if _millisecs is set, spawn a thread to ping the proxy
  if (_millisecs) 
    _worker = new GenericBoundWorkerThread(this);
  return 0; // OK
}

void Structured512PushConsumer_i::cleanup() {
  CosNA::StructuredProxyPushSupplier_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "Structured512PushConsumer", "Structured512PushConsumer_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    if (CORBA::is_nil(_my_proxy)) {
      cerr << "Coding error: cleanup already called?" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosNA::StructuredProxyPushSupplier::_nil();
  } //end lock scope 
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_structured_push_supplier();
  } catch(...) {}
  try {
    _my_admin->destroy();
  } catch (...) { }
  _my_admin = CosNA::ConsumerAdmin::_nil();
  destroy_filters(_my_filters);
}

// While _millisecs is positive, pings proxy every _millisecs milliseconds.
// Sets error code and sets _done to 1 if there is an error during ping.
void* Structured512PushConsumer_i::start_working(void *) {
  CosNC::StructuredPushConsumer_var bump_my_refcount_during_outcalls = _this();

  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "Structured512PushConsumer", "Structured512PushConsumer_i::start_working");

    if (_verbose && (!_done) && _millisecs) cout << _obj_name << ": Spawned thread entering ping loop" << endl;
    // invariant: _oplock held at top of loop
    while ( 1 ) {
      if (_done || (_millisecs == 0)) break;
      unsigned long t_secs = 0, t_nanosecs = 0;
      unsigned long d_secs = 0, d_nanosecs = 0;
      d_secs = _millisecs / 1000;
      d_nanosecs = (_millisecs % 1000) * 1000000;
      { // introduce temporary unlock scope
	TW_SCOPE_RELEASE(lock);
	try {
	  _my_proxy->MyType();
	} catch (...) {
	  goto com_fail;
	}
      } // end temporary unlock scope
      if (_done) break; // must have disconnected during pull
      // sleep for specified interval
      TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
      _finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      continue; // continue ping loop
    com_fail:
      if (_done) break;
      if (_verbose) cout << _obj_name << ": communication error while pinging proxy using MyType()" << endl;
      _done = 1;
      _finish.broadcast();
      _com_err = 1;
      break; // break from while loop -- done
    } // end while loop
  } // end lock scope
  return 0;
}

void Structured512PushConsumer_i::push_structured_event(const CosN::StructuredEvent& data)
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "Structured512PushConsumer", "Structured512PushConsumer_i::push_structured_event");
  if (_done) return;
  _num_events++;
  if (_consume_fn)
    (*_consume_fn)(data, _obj_name, _num_events, _verbose);
  else if (_verbose) cout << _obj_name << ": event count = " << _num_events << endl;
  if (_max_events && (_num_events >= _max_events)) {
    if (_verbose) cout << _obj_name << ": DONE [max_events reached]" << endl;
    _done = 1;
    _finish.broadcast();
  }
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying push_structured_event response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

void Structured512PushConsumer_i::disconnect_structured_push_consumer()
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "Structured512PushConsumer", "Structured512PushConsumer_i::disconnect_structured_push_consumer");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean Structured512PushConsumer_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "Structured512PushConsumer", "Structured512PushConsumer_i::wait_done");
    while (!_done) {
      _finish.wait();
    }
    // using tmp_worker ensures only one wait_done call tries join()
    tmp_worker = _worker;
    _worker = 0;
  } // end lock scope
  if (tmp_worker) {
    tmp_worker->join(0);
  }
  return _com_err;
}

void Structured512PushConsumer_i::offer_change(const CosN::EventTypeSeq& added,
					       const CosN::EventTypeSeq& deled)
{
  if (_done) return; // see "Optimization Notes" at top
  TW_SCOPE_LOCK(lock, _oplock, "Structured512PushConsumer", "Structured512PushConsumer_i::offer_change");
  if (_done) return;
  _num_changes++;
  if (_change_fn) (*_change_fn)(added, deled, _obj_name, _num_changes, _verbose);
  else if (_verbose) cout << _obj_name << ": offer_change received [# " << _num_changes << "]" << endl;
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying offer_change response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

// ------------- CosNotifyComm::SequencePushConsumer ------------- //

SequencePushConsumer_i::
SequencePushConsumer_i(CosNA::SequenceProxyPushSupplier_ptr proxy,
		       CosNA::ConsumerAdmin_ptr admin,
		       CosNF::Filter_ptr filter,
		       CORBA::ULong max_events, const char* objnm,
		       consume_batch_fn* consume_fn, type_change_fn* change_fn,
		       CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _my_admin(admin), _my_filters(0),
  _obj_name(objnm), _consume_fn(consume_fn), _change_fn(change_fn),
  _num_changes(0), _num_events(0), _num_batches(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose), _done(0), _com_err(0),
  _oplock(), _finish(&_oplock), _worker(0)
{
  if (! CORBA::is_nil(filter)) {
    _my_filters.length(1);
    _my_filters[0] = filter;
  }
}

SequencePushConsumer_i*
SequencePushConsumer_i::create(CORBA::ORB_ptr orb,
			       CosNA::EventChannel_ptr channel,
			       CORBA::ULong max_events,
			       CORBA::ULong batch_size,
			       const char* objnm,
			       const char* proxy_ior_file,
			       const char* admin_ior_file,
			       consume_batch_fn* consume_fn,
			       type_change_fn* change_fn,
			       CosN::EventTypeSeq* evs_ptr,
			       const char* constraint_expr,
			       CORBA::ULong millisecs,
			       CORBA::ULong delay_millisecs,
			       CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosNA::ConsumerAdmin_ptr admin = CosNA::ConsumerAdmin::_nil();
  CosNA::ProxySupplier_var generic_proxy =
    get_proxy_supplier(orb, channel, CosNA::SEQUENCE_EVENT, 1, admin_ior_file, admin, verbose); // 1 is push 0 is pull
  CosNA::SequenceProxyPushSupplier_ptr proxy = CosNA::SequenceProxyPushSupplier::_narrow(generic_proxy);
  if ( CORBA::is_nil(proxy) ) {
    return 0; // get_proxy_supplier failed
  }
  // If evs or constraint_expr are non-empty, add a filter to proxy
  CosNF::Filter_ptr filter = CosNF::Filter::_nil();
  if (evs_ptr) {
    CORBA::Boolean filt_err = sample_add_filter(channel, proxy, *evs_ptr, constraint_expr, objnm, filter, verbose);
    if (filt_err) {
      try {
	admin->destroy();
      } catch (...) { }
      return 0; // adding filter failed
    }
  }
  // write proxy IOR to file
  write_ior_to_file(orb, proxy, proxy_ior_file, verbose);
  // Construct a client
  SequencePushConsumer_i* client =
    new SequencePushConsumer_i(proxy, admin, filter, max_events, objnm, consume_fn, change_fn, millisecs, delay_millisecs, verbose);
  return client;
}

CORBA::Boolean SequencePushConsumer_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "SequencePushConsumer", "SequencePushConsumer_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_sequence_push_consumer(_this());
    if (_change_fn) {
      _my_proxy->obtain_offered_types(CosNA::NONE_NOW_UPDATES_ON);
    } else {
      _my_proxy->obtain_offered_types(CosNA::NONE_NOW_UPDATES_OFF);
    }
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEvCA::AlreadyConnected& ex) {
    cerr << _obj_name << ": Already connected" << endl;
    return 1; // error
  } catch (...) {
    cerr << _obj_name << ": Failed to connect" << endl;
    return 1; // error
  }
  if (_verbose) cout << _obj_name << ": Connected to proxy, ready to consume events" << endl; 
  // if _millisecs is set, spawn a thread to ping the proxy
  if (_millisecs)
    _worker = new GenericBoundWorkerThread(this);
  return 0; // OK
}

void SequencePushConsumer_i::cleanup() {
  CosNA::SequenceProxyPushSupplier_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "SequencePushConsumer", "SequencePushConsumer_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    if (CORBA::is_nil(_my_proxy)) {
      cerr << "Coding error: cleanup already called?" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosNA::SequenceProxyPushSupplier::_nil();
  }
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_sequence_push_supplier();
  } catch(...) {}
  try {
    _my_admin->destroy();
  } catch (...) { }
  _my_admin = CosNA::ConsumerAdmin::_nil();
  destroy_filters(_my_filters);
}

// While _millisecs is positive, pings proxy every _millisecs milliseconds.
// Sets error code and sets _done to 1 if there is an error during ping.
void* SequencePushConsumer_i::start_working(void *) {
  CosNC::SequencePushConsumer_var bump_my_refcount_during_outcalls = _this();

  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "SequencePushConsumer", "SequencePushConsumer_i::start_working");

    if (_verbose && (!_done) && _millisecs) cout << _obj_name << ": Spawned thread entering ping loop" << endl;
    // invariant: _oplock held at top of loop
    while ( 1 ) {
      if (_done || (_millisecs == 0)) break;
      unsigned long t_secs = 0, t_nanosecs = 0;
      unsigned long d_secs = 0, d_nanosecs = 0;
      d_secs = _millisecs / 1000;
      d_nanosecs = (_millisecs % 1000) * 1000000;
      { // introduce temporary lock release scope
	TW_SCOPE_RELEASE(lock);
	try {
	  _my_proxy->MyType();
	} catch (...) {
	  goto com_fail;
	}
      } // end temporary lock release scope
      if (_done) break; // must have disconnected during pull
      // sleep for specified interval
      TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
      _finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      continue; // continue ping loop
    com_fail:
	if (_done) break;
	if (_verbose) cout << _obj_name << ": communication error while pinging proxy using MyType()" << endl;
	_done = 1;
	_finish.broadcast();
	_com_err = 1;
	break; // break from while loop -- done
    } // end while loop
  } // end lock scope
  return 0;
}

void SequencePushConsumer_i::push_structured_events(const CosN::EventBatch& data)
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "SequencePushConsumer", "SequencePushConsumer_i::push_structured_events");
  if (_done) return;
  _num_batches++;
  _num_events += data.length();
  if (_consume_fn)
    (*_consume_fn)(data, _obj_name, _num_events, _num_batches, _verbose);
  else if (_verbose) cout << _obj_name << ": event count = " << _num_events << " batch count = " << _num_batches << endl;
  if (_max_events && (_num_events >= _max_events)) {
    if (_verbose) cout << _obj_name << ": DONE [max_events reached]" << endl;
    _done = 1;
    _finish.broadcast();
  }
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying push_structured_events response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

void SequencePushConsumer_i::disconnect_sequence_push_consumer()
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "SequencePushConsumer", "SequencePushConsumer_i::disconnect_sequence_push_consumer");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean SequencePushConsumer_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "SequencePushConsumer", "SequencePushConsumer_i::wait_done");
    while (!_done) {
      _finish.wait();
    }
    // using tmp_worker ensures only one wait_done call tries join()
    tmp_worker = _worker;
    _worker = 0;
  } // end lock scope
  if (tmp_worker) {
    tmp_worker->join(0);
  }
  return _com_err;
}

void SequencePushConsumer_i::offer_change(const CosN::EventTypeSeq& added,
					  const CosN::EventTypeSeq& deled)
{
  if (_done) return; // see "Optimization Notes" at top
  TW_SCOPE_LOCK(lock, _oplock, "SequencePushConsumer", "SequencePushConsumer_i::offer_change");
  if (_done) return;
  _num_changes++;
  if (_change_fn) (*_change_fn)(added, deled, _obj_name, _num_changes, _verbose);
  else if (_verbose) cout << _obj_name << ": offer_change received [# " << _num_changes << "]" << endl;
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying offer_change response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

///////////////////////////////////////////////////////////////////
//                   PULL  CONSUMER  EXAMPLES                    //
///////////////////////////////////////////////////////////////////

// ---------------- CosNotifyComm::PullConsumer ---------------- //

PullConsumer_i::
PullConsumer_i(CosNA::ProxyPullSupplier_ptr proxy,
	       CosNA::ConsumerAdmin_ptr admin,
	       CosNF::Filter_ptr filter,
	       CORBA::ULong max_events, const char* objnm,
	       consume_any_fn* consume_fn, type_change_fn* change_fn,
	       CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _my_admin(admin), _my_filters(0),
  _obj_name(objnm), _consume_fn(consume_fn), _change_fn(change_fn),
  _num_changes(0), _num_events(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose), _done(0), _com_err(0),
  _oplock(), _finish(&_oplock), _worker(0)
{
  if (! CORBA::is_nil(filter)) {
    _my_filters.length(1);
    _my_filters[0] = filter;
  }
}

PullConsumer_i*
PullConsumer_i::create(CORBA::ORB_ptr orb,
		       CosNA::EventChannel_ptr channel,
		       CORBA::ULong max_events,
		       CORBA::ULong batch_size,
		       const char* objnm,
		       const char* proxy_ior_file,
		       const char* admin_ior_file,
		       consume_any_fn* consume_fn,
		       type_change_fn* change_fn,
		       CosN::EventTypeSeq* evs_ptr,
		       const char* constraint_expr,
		       CORBA::ULong millisecs,
		       CORBA::ULong delay_millisecs,
		       CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosNA::ConsumerAdmin_ptr admin = CosNA::ConsumerAdmin::_nil();
  CosNA::ProxySupplier_var generic_proxy =
    get_proxy_supplier(orb, channel, CosNA::ANY_EVENT, 0, admin_ior_file, admin, verbose); // 1 is push 0 is pull
  CosNA::ProxyPullSupplier_ptr proxy = CosNA::ProxyPullSupplier::_narrow(generic_proxy);
  if ( CORBA::is_nil(proxy) ) {
    return 0; // get_proxy_supplier failed
  }
  // If evs or constraint_expr are non-empty, add a filter to proxy
  CosNF::Filter_ptr filter = CosNF::Filter::_nil();
  if (evs_ptr) {
    CORBA::Boolean filt_err = sample_add_filter(channel, proxy, *evs_ptr, constraint_expr, objnm, filter, verbose);
    if (filt_err) {
      try {
	admin->destroy();
      } catch (...) { }
      return 0; // adding filter failed
    }
  }
  // write proxy IOR to file
  write_ior_to_file(orb, proxy, proxy_ior_file, verbose);
  // Construct a client
  PullConsumer_i* client =
    new PullConsumer_i(proxy, admin, filter, max_events, objnm, consume_fn, change_fn, millisecs, delay_millisecs, verbose);
  return client;
}

CORBA::Boolean PullConsumer_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "PullConsumer", "PullConsumer_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_any_pull_consumer(_this());
    if (_change_fn) {
      _my_proxy->obtain_offered_types(CosNA::NONE_NOW_UPDATES_ON);
    } else {
      _my_proxy->obtain_offered_types(CosNA::NONE_NOW_UPDATES_OFF);
    }
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEvCA::AlreadyConnected& ex) {
    cerr << _obj_name << ": Already connected" << endl;
    return 1; // error
  } catch (...) {
    cerr << _obj_name << ": Failed to connect" << endl;
    return 1; // error
  }
  // spawn a thread to do pulling
  if (_verbose) cout << _obj_name << ": Connected to proxy, ready to consume events" << endl; 
  _worker = new GenericBoundWorkerThread(this);
  return 0; // OK
}

void PullConsumer_i::cleanup() {
  CosNA::ProxyPullSupplier_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "PullConsumer", "PullConsumer_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    if (CORBA::is_nil(_my_proxy)) {
      cerr << "Coding error: cleanup already called?" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosNA::ProxyPullSupplier::_nil();
  }  // end lock scope
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_pull_supplier();
  } catch(...) {}
  try {
    _my_admin->destroy();
  } catch (...) { }
  _my_admin = CosNA::ConsumerAdmin::_nil();
  destroy_filters(_my_filters);
}

// Note: oplock is *not* held during pull(), yield(), and timedwait() calls
void* PullConsumer_i::start_working(void *) {
  CosNC::PullConsumer_var bump_my_refcount_during_outcalls = _this();

  unsigned long t_secs = 0, t_nanosecs = 0;
  unsigned long d_secs = 0, d_nanosecs = 0;
  if (_millisecs) {
    d_secs = _millisecs / 1000;
    d_nanosecs = (_millisecs % 1000) * 1000000;
  }
  CORBA::Any* data = 0;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "PullConsumer", "PullConsumer_i::start_working");

    if (_verbose && (!_done)) cout << _obj_name << ": Spawned thread entering main pull loop" << endl;
    while ( 1 ) {
      if (_done) break; // must have disconnected
      { // introduce temporary lock release scope; do not hold oplock across pull
	TW_SCOPE_RELEASE(lock);
	try {
	  data = _my_proxy->pull();
	} catch (...) {
	  goto com_fail;
	}
      } // end temporary lock release scope
      if (_done) break; // must have disconnected during pull
      if (!data) {
	if (_verbose) cout << _obj_name << ": strange failure: pull() returned nil" << endl;
	_com_err = 1;
	break; // break from while loop -- done
      }
      // got an event
      _num_events++;
      if (_consume_fn)
	(*_consume_fn)(*data, _obj_name, _num_events, _verbose);
      else if (_verbose) cout << _obj_name << ": event count = " << _num_events << endl;
      delete data;
      data = 0;
      if (_max_events && (_num_events >= _max_events)) {
	if (_verbose) cout << _obj_name << ": DONE [max_events reached]" << endl;
	break; // done 
      }
      if (_millisecs) { // sleep for specified interval
	TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
	_finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      } else { // use yield to let other threads have a shot at _oplock
	{ // introduce temporary lock release scope
	  TW_SCOPE_RELEASE(lock);
	  TW_YIELD();
	} // end temporary lock release scope
      }
      continue; // continue pulling
    com_fail:
      if (_done) break;
      if (_verbose) cout << _obj_name << ": communication error while calling pull()" << endl;
      _com_err = 1;
      break;
    } // end while loop
    delete data;
    if (!_done) {
      _done = 1;
      _finish.broadcast();
    }
  } // end lock scope
  return 0;
}


void PullConsumer_i::disconnect_pull_consumer()
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "PullConsumer", "PullConsumer_i::disconnect_pull_consumer");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean PullConsumer_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "PullConsumer", "PullConsumer_i::wait_done");
    while (!_done) {
      _finish.wait();
    }
    // using tmp_worker ensures only one wait_done call tries join()
    tmp_worker = _worker;
    _worker = 0;
  } // end lock scopen
  if (tmp_worker) {
    tmp_worker->join(0);
  }
  return _com_err;
}

void PullConsumer_i::offer_change(const CosN::EventTypeSeq& added,
				  const CosN::EventTypeSeq& deled)
{
  if (_done) return; // see "Optimization Notes" at top
  TW_SCOPE_LOCK(lock, _oplock, "PullConsumer", "PullConsumer_i::offer_change");
  if (_done) return;
  _num_changes++;
  if (_change_fn) (*_change_fn)(added, deled, _obj_name, _num_changes, _verbose);
  else if (_verbose) cout << _obj_name << ": offer_change received [# " << _num_changes << "]" << endl;
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying offer_change response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

// ---------------- CosNotifyComm::StructuredPullConsumer ---------------- //

StructuredPullConsumer_i::
StructuredPullConsumer_i(CosNA::StructuredProxyPullSupplier_ptr proxy,
			 CosNA::ConsumerAdmin_ptr admin,
			 CosNF::Filter_ptr filter,
			 CORBA::ULong max_events, const char* objnm,
			 consume_structured_fn* consume_fn, type_change_fn* change_fn,
			 CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _my_admin(admin), _my_filters(0),
  _obj_name(objnm), _consume_fn(consume_fn), _change_fn(change_fn),
  _num_changes(0), _num_events(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose),
  _done(0), _com_err(0), _oplock(), _finish(&_oplock), _worker(0)
{
  if (! CORBA::is_nil(filter)) {
    _my_filters.length(1);
    _my_filters[0] = filter;
  }
}

StructuredPullConsumer_i*
StructuredPullConsumer_i::create(CORBA::ORB_ptr orb,
				 CosNA::EventChannel_ptr channel,
				 CORBA::ULong max_events,
				 CORBA::ULong batch_size,
				 const char* objnm,
				 const char* proxy_ior_file,
				 const char* admin_ior_file,
				 consume_structured_fn* consume_fn,
				 type_change_fn* change_fn,
				 CosN::EventTypeSeq* evs_ptr,
				 const char* constraint_expr,
				 CORBA::ULong millisecs,
				 CORBA::ULong delay_millisecs,
				 CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosNA::ConsumerAdmin_ptr admin = CosNA::ConsumerAdmin::_nil();
  CosNA::ProxySupplier_var generic_proxy =
    get_proxy_supplier(orb, channel, CosNA::STRUCTURED_EVENT, 0, admin_ior_file, admin, verbose); // 1 is push 0 is pull
  CosNA::StructuredProxyPullSupplier_ptr proxy = CosNA::StructuredProxyPullSupplier::_narrow(generic_proxy);
  if ( CORBA::is_nil(proxy) ) {
    return 0; // get_proxy_supplier failed
  }
  // If evs or constraint_expr are non-empty, add a filter to proxy
  CosNF::Filter_ptr filter = CosNF::Filter::_nil();
  CORBA::Boolean filt_err = 0;
  if (evs_ptr) {
    filt_err = sample_add_filter(channel, proxy, *evs_ptr, constraint_expr, objnm, filter, verbose);
    if (filt_err) {
      try {
	admin->destroy();
      } catch (...) { }
      return 0; // adding filter failed
    }
  }
  // write proxy IOR to file
  write_ior_to_file(orb, proxy, proxy_ior_file, verbose);
  // Construct a client
  StructuredPullConsumer_i* client =
    new StructuredPullConsumer_i(proxy, admin, filter, max_events, objnm, consume_fn, change_fn, millisecs, delay_millisecs, verbose);
  return client;
}


CORBA::Boolean StructuredPullConsumer_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "StructuredPullConsumer", "StructuredPullConsumer_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_structured_pull_consumer(_this());
    if (_change_fn) {
      _my_proxy->obtain_offered_types(CosNA::NONE_NOW_UPDATES_ON);
    } else {
      _my_proxy->obtain_offered_types(CosNA::NONE_NOW_UPDATES_OFF);
    }
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEvCA::AlreadyConnected& ex) {
    cerr << _obj_name << ": Already connected" << endl;
    return 1; // error
  } catch (...) {
    cerr << _obj_name << ": Failed to connect" << endl;
    return 1; // error
  }
  // spawn a thread to do pulling
  if (_verbose) cout << _obj_name << ": Connected to proxy, ready to consume events" << endl; 
  _worker = new GenericBoundWorkerThread(this);
  return 0; // OK
}

void StructuredPullConsumer_i::cleanup() {
  CosNA::StructuredProxyPullSupplier_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "StructuredPullConsumer", "StructuredPullConsumer_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    if (CORBA::is_nil(_my_proxy)) {
      cerr << "Coding error: cleanup already called?" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosNA::StructuredProxyPullSupplier::_nil();
  } // end lock scope
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_structured_pull_supplier();
  } catch(...) {}
  try {
    _my_admin->destroy();
  } catch (...) { }
  _my_admin = CosNA::ConsumerAdmin::_nil();
  destroy_filters(_my_filters);
}

// Note: lock is *not* held during pull_structured_event(), yield(), and timedwait() calls
void* StructuredPullConsumer_i::start_working(void *) {
  CosNC::StructuredPullConsumer_var bump_my_refcount_during_outcalls = _this();

  unsigned long t_secs = 0, t_nanosecs = 0;
  unsigned long d_secs = 0, d_nanosecs = 0;
  if (_millisecs) {
    d_secs = _millisecs / 1000;
    d_nanosecs = (_millisecs % 1000) * 1000000;
  }
  CosN::StructuredEvent* data = 0;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "StructuredPullConsumer", "StructuredPullConsumer_i::start_working");

    if (_verbose && (!_done)) cout << _obj_name << ": Spawned thread entering main pull loop" << endl;
    while ( 1 ) {
      if (_done) break; // must have disconnected
      { // introduce temporary lock release scope; do not hold oplock across pull
	TW_SCOPE_RELEASE(lock);
	try {
	  data = _my_proxy->pull_structured_event();
	} catch (...) {
	  goto com_fail;
	}
      } // end temporary lock release scope
      if (_done) break; // must have disconnected during pull
      if (!data) {
	if (_verbose) cout << _obj_name << ": strange failure: pull_structured_event() returned nil" << endl;
	_com_err = 1;
	break; // break from while loop -- done
      }
      // got an event
      _num_events++;
      if (_consume_fn)
	(*_consume_fn)(*data, _obj_name, _num_events, _verbose);
      else if (_verbose) cout << _obj_name << ": event count = " << _num_events << endl;
      delete data;
      data = 0;
      if (_max_events && (_num_events >= _max_events)) {
	if (_verbose) cout << _obj_name << ": DONE [max_events reached]" << endl;
	break; // done
      }
      if (_millisecs) { // sleep for specified interval
	TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
	_finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      } else { // use yield to let other threads have a shot at _oplock
	{ // introduce temporary lock release scope
	  TW_SCOPE_RELEASE(lock);
	  TW_YIELD();
	} // end temporary lock release scope
      }
      continue; // continue pulling
    com_fail:
      if (_done) break; // must have disconnected during pull
      if (_verbose) cout << _obj_name << ": communication error while calling pull_structured_event()" << endl;
      _com_err = 1;
      break; // break from while loop -- done
    } // end while loop
    // done
    delete data;
    if (!_done) {
      _done = 1;
      _finish.broadcast();
    }
  } // end lock scope
  return 0;
}


void StructuredPullConsumer_i::disconnect_structured_pull_consumer()
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "StructuredPullConsumer", "StructuredPullConsumer_i::disconnect_structured_pull_consumer");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean StructuredPullConsumer_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "StructuredPullConsumer", "StructuredPullConsumer_i::wait_done");
    while (!_done) {
      _finish.wait();
    }
    // using tmp_worker ensures only one wait_done call tries join()
    tmp_worker = _worker;
    _worker = 0;
  } // end lock scope
  if (tmp_worker) {
    tmp_worker->join(0);
  }
  return _com_err;
}

void StructuredPullConsumer_i::offer_change(const CosN::EventTypeSeq& added,
					    const CosN::EventTypeSeq& deled)
{
  if (_done) return; // see "Optimization Notes" at top
  TW_SCOPE_LOCK(lock, _oplock, "StructuredPullConsumer", "StructuredPullConsumer_i::offer_change");
  if (_done) return;
  _num_changes++;
  if (_change_fn) (*_change_fn)(added, deled, _obj_name, _num_changes, _verbose);
  else if (_verbose) cout << _obj_name << ": offer_change received [# " << _num_changes << "]" << endl;
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying offer_change response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

// ---------------- CosNotifyComm::SequencePullConsumer ---------------- //

SequencePullConsumer_i::
SequencePullConsumer_i(CosNA::SequenceProxyPullSupplier_ptr proxy,
		       CosNA::ConsumerAdmin_ptr admin,
		       CosNF::Filter_ptr filter,
		       CORBA::ULong max_events, const char* objnm,
		       consume_batch_fn* consume_fn, type_change_fn* change_fn,
		       CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _my_admin(admin), _my_filters(0),
  _obj_name(objnm), _consume_fn(consume_fn), _change_fn(change_fn),
  _num_changes(0), _num_events(0), _num_batches(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose), _done(0), _com_err(0),
  _oplock(), _finish(&_oplock), _worker(0)
{
  if (! CORBA::is_nil(filter)) {
    _my_filters.length(1);
    _my_filters[0] = filter;
  }
}

SequencePullConsumer_i*
SequencePullConsumer_i::create(CORBA::ORB_ptr orb,
			       CosNA::EventChannel_ptr channel,
			       CORBA::ULong max_events,
			       CORBA::ULong batch_size,
			       const char* objnm,
			       const char* proxy_ior_file,
			       const char* admin_ior_file,
			       consume_batch_fn* consume_fn,
			       type_change_fn* change_fn,
			       CosN::EventTypeSeq* evs_ptr,
			       const char* constraint_expr,
			       CORBA::ULong millisecs,
			       CORBA::ULong delay_millisecs,
			       CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosNA::ConsumerAdmin_ptr admin = CosNA::ConsumerAdmin::_nil();
  CosNA::ProxySupplier_var generic_proxy =
    get_proxy_supplier(orb, channel, CosNA::SEQUENCE_EVENT, 0, admin_ior_file, admin, verbose); // 1 is push 0 is pull
  CosNA::SequenceProxyPullSupplier_ptr proxy = CosNA::SequenceProxyPullSupplier::_narrow(generic_proxy);
  if ( CORBA::is_nil(proxy) ) {
    return 0; // get_proxy_supplier failed
  }
  // If evs or constraint_expr are non-empty, add a filter to proxy
  CosNF::Filter_ptr filter = CosNF::Filter::_nil();
  if (evs_ptr) {
    CORBA::Boolean filt_err = sample_add_filter(channel, proxy, *evs_ptr, constraint_expr, objnm, filter, verbose);
    if (filt_err) {
      try {
	admin->destroy();
      } catch (...) { }
      return 0; // adding filter failed
    }
  }
  // write proxy IOR to file
  write_ior_to_file(orb, proxy, proxy_ior_file, verbose);
  // Construct a client
  SequencePullConsumer_i* client =
    new SequencePullConsumer_i(proxy, admin, filter, max_events, objnm, consume_fn, change_fn, millisecs, delay_millisecs, verbose);
  return client;
}


CORBA::Boolean SequencePullConsumer_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "SequencePullConsumer", "SequencePullConsumer_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_sequence_pull_consumer(_this());
    if (_change_fn) {
      _my_proxy->obtain_offered_types(CosNA::NONE_NOW_UPDATES_ON);
    } else {
      _my_proxy->obtain_offered_types(CosNA::NONE_NOW_UPDATES_OFF);
    }
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEvCA::AlreadyConnected& ex) {
    cerr << _obj_name << ": Already connected" << endl;
    return 1; // error
  } catch (...) {
    cerr << _obj_name << ": Failed to connect" << endl;
    return 1; // error
  }
  // spawn a thread to do pulling
  if (_verbose) cout << _obj_name << ": Connected to proxy, ready to consume events" << endl; 
  _worker = new GenericBoundWorkerThread(this);
  return 0; // OK
}

void SequencePullConsumer_i::cleanup() {
  CosNA::SequenceProxyPullSupplier_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "SequencePullConsumer", "SequencePullConsumer_i::connect");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    if (CORBA::is_nil(_my_proxy)) {
      cerr << "Coding error: cleanup already called?" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosNA::SequenceProxyPullSupplier::_nil();
  } // end lock scope
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_sequence_pull_supplier();
  } catch(...) {}
  try {
    _my_admin->destroy();
  } catch (...) { }
  _my_admin = CosNA::ConsumerAdmin::_nil();
  destroy_filters(_my_filters);
}

// Note: lockis *not* held during pull_structured_events(), yield(), and timedwait() calls
void* SequencePullConsumer_i::start_working(void *) {
  CosNC::SequencePullConsumer_var bump_my_refcount_during_outcalls = _this();

  unsigned long t_secs = 0, t_nanosecs = 0;
  unsigned long d_secs = 0, d_nanosecs = 0;
  if (_millisecs) {
    d_secs = _millisecs / 1000;
    d_nanosecs = (_millisecs % 1000) * 1000000;
  }
  CosN::EventBatch* data = 0;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "SequencePullConsumer", "SequencePullConsumer_i::start_working");

    if (_verbose && (!_done)) cout << _obj_name << ": Spawned thread entering main pull loop" << endl;
    while ( 1 ) {
      if (_done) break; // must have disconnected
      { // introduce temporary lock release scope; do not hold oplock across pull
	TW_SCOPE_RELEASE(lock);
	try {
	  data = _my_proxy->pull_structured_events((CORBA::ULong)10);
	} catch (...) {
	  goto com_fail;
	}
      } // end temporary lock release scope
      if (_done) break; // must have disconnected during pull
      if (!data) {
	if (_verbose) cout << _obj_name << ": strange failure: pull_structured_events() returned nil" << endl;
	_com_err = 1;
	break; // break from while loop -- done
      }
      // got an event
      _num_batches++;
      _num_events += data->length();
      if (_consume_fn)
	(*_consume_fn)(*data, _obj_name, _num_events, _num_batches, _verbose);
      else if (_verbose) cout << _obj_name << ": event count = " << _num_events << " batch count = " << _num_batches << endl;
      delete data;
      data = 0;
      if (_max_events && (_num_events >= _max_events)) {
	if (_verbose) cout << _obj_name << ": DONE [max_events reached]" << endl;
	break; // done
      }
      if (_millisecs) { // sleep for specified interval
	TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
	_finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      } else { // use yield to let other threads have a shot at _oplock
	{ // introduce temporary lock release scope
	  TW_SCOPE_RELEASE(lock);
	  TW_YIELD();
	}
      }
      continue; // continue pulling
    com_fail:
      if (_done) break; // must have disconnected during pull
      if (_verbose) cout << _obj_name << ": communication error while calling pull_structured_events()" << endl;
      _com_err = 1;
      break; // break from while loop -- done
    } // end while loop
    delete data;
    if (!_done) {
      _done = 1;
      _finish.broadcast();
    }
  } // end lock scopen
  return 0;
}


void SequencePullConsumer_i::disconnect_sequence_pull_consumer()
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "SequencePullConsumer", "SequencePullConsumer_i::disconnect_sequence_pull_consumer");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean SequencePullConsumer_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "SequencePullConsumer", "SequencePullConsumer_i::wait_done");
    while (!_done) {
      _finish.wait();
    }
    // using tmp_worker ensures only one wait_done call tries join()
    tmp_worker = _worker;
    _worker = 0;
  } // end lock scope
  if (tmp_worker) {
    tmp_worker->join(0);
  }
  return _com_err;
}

void SequencePullConsumer_i::offer_change(const CosN::EventTypeSeq& added,
					  const CosN::EventTypeSeq& deled)
{
  if (_done) return; // see "Optimization Notes" at top
  TW_SCOPE_LOCK(lock, _oplock, "SequencePullConsumer", "SequencePullConsumer_i::offer_change");
  if (_done) return;
  _num_changes++;
  if (_change_fn) (*_change_fn)(added, deled, _obj_name, _num_changes, _verbose);
  else if (_verbose) cout << _obj_name << ": offer_change received [# " << _num_changes << "]" << endl;
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying offer_change response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

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

PushSupplier_i::
PushSupplier_i(CosNA::ProxyPushConsumer_ptr proxy,
	       CosNA::SupplierAdmin_ptr admin,
	       CosNF::Filter_ptr filter,
	       CORBA::ULong max_events, const char* objnm,
	       supply_any_fn* supply_fn, type_change_fn* change_fn,
	       CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _my_admin(admin), _my_filters(0),
  _obj_name(objnm), _supply_fn(supply_fn), _change_fn(change_fn),
  _num_changes(0), _num_events(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose), _done(0), _com_err(0),
  _oplock(), _finish(&_oplock), _worker(0)
{
  // providing explict NULL for supply_fn is not OK -- must have a valid function
  if (!_supply_fn) _supply_fn = sample_supply_any_fn;
  if (! CORBA::is_nil(filter)) {
    _my_filters.length(1);
    _my_filters[0] = filter;
  }
}

PushSupplier_i*
PushSupplier_i::create(CORBA::ORB_ptr orb,
		       CosNA::EventChannel_ptr channel,
		       CORBA::ULong max_events,
		       CORBA::ULong batch_size,
		       const char* objnm,
		       const char* proxy_ior_file,
		       const char* admin_ior_file,
		       supply_any_fn* supply_fn,
		       type_change_fn* change_fn,
		       CosN::EventTypeSeq* evs_ptr,
		       const char* constraint_expr,
		       CORBA::ULong millisecs,
		       CORBA::ULong delay_millisecs,
		       CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosNA::SupplierAdmin_ptr admin = CosNA::SupplierAdmin::_nil();
  CosNA::ProxyConsumer_var generic_proxy =
    get_proxy_consumer(orb, channel, CosNA::ANY_EVENT, 1, admin_ior_file, admin, verbose); // 1 means push 0 means pull
  CosNA::ProxyPushConsumer_ptr proxy = CosNA::ProxyPushConsumer::_narrow(generic_proxy);
  if ( CORBA::is_nil(proxy) ) {
    return 0; // get_proxy_consumer failed
  }
  // If evs or constraint_expr are non-empty, add a filter to proxy
  CosNF::Filter_ptr filter = CosNF::Filter::_nil();
  if (evs_ptr) {
    CORBA::Boolean filt_err = sample_add_filter(channel, proxy, *evs_ptr, constraint_expr, objnm, filter, verbose);
    if (filt_err) {
      try {
	admin->destroy();
      } catch (...) { }
      return 0; // adding filter failed
    }
  }
  // write proxy IOR to file
  write_ior_to_file(orb, proxy, proxy_ior_file, verbose);
  // Construct a client
  PushSupplier_i* client =
    new PushSupplier_i(proxy, admin, filter, max_events, objnm, supply_fn, change_fn, millisecs, delay_millisecs, verbose);
  return client;
}

CORBA::Boolean PushSupplier_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "PushSupplier", "PushSupplier_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_any_push_supplier(_this());
    if (_change_fn) {
      _my_proxy->obtain_subscription_types(CosNA::NONE_NOW_UPDATES_ON);
    } else {
      _my_proxy->obtain_subscription_types(CosNA::NONE_NOW_UPDATES_OFF);
    }
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEvCA::AlreadyConnected& ex) {
    cerr << _obj_name << ": Already connected" << endl;
    return 1; // error
  } catch (...) {
    cerr << _obj_name << ": Failed to connect" << endl;
    return 1; // error
  }
  // register the types to be supplied
  offer_any(_my_proxy, _obj_name, _verbose);
  // spawn a thread to do pushing
  if (_verbose) cout << _obj_name << ": Connected to proxy, ready to supply events" << endl; 
  _worker = new GenericBoundWorkerThread(this);
  return 0; // OK
}

void PushSupplier_i::cleanup() {
  CosNA::ProxyPushConsumer_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "PushSupplier", "PushSupplier_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    if (CORBA::is_nil(_my_proxy)) {
      cerr << "Coding error: cleanup already called?" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosNA::ProxyPushConsumer::_nil();
  }
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_push_consumer();
  } catch(...) {}
  try {
    _my_admin->destroy();
  } catch (...) { }
  _my_admin = CosNA::SupplierAdmin::_nil();
  destroy_filters(_my_filters);
}

// Note: lock is *not* held during push(), yield() and timedwait() calls
void* PushSupplier_i::start_working(void *) {
  CosNC::PushSupplier_var bump_my_refcount_during_outcalls = _this();

  unsigned long t_secs = 0, t_nanosecs = 0;
  unsigned long d_secs = 0, d_nanosecs = 0;
  if (_millisecs) {
    d_secs = _millisecs / 1000;
    d_nanosecs = (_millisecs % 1000) * 1000000;
  }
  CORBA::Any* data = new CORBA::Any;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "PushSupplier", "PushSupplier_i::start_working");

    if (_verbose && (!_done)) cout << _obj_name << ": Spawned thread entering main push loop" << endl;
    while ( 1 ) {
      if (_done) break; // must have disconnected
      if ((*_supply_fn)(*data, _obj_name, _num_events+1, _verbose)) {
	{ // introduce temporary lock release scope; do not hold oplock across push
	  TW_SCOPE_RELEASE(lock);
	  try {
	    _my_proxy->push(*data);
	  } catch (...) {
	    goto com_fail;
	  }
	} // end temporary lock release scope
	if (_done) break; // must have disconnected during push
	_num_events++;
	if (_max_events && (_num_events >= _max_events)) {
	  if (_verbose) cout << _obj_name << ": DONE [max_events reached]" << endl;
	  break; // done
	}
      }
      // wait or yield then try again
      if (_millisecs) { // sleep for specified interval
	TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
	_finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      } else { // use yield to let other threads have a shot at _oplock
	{ // introduce temporary lock release scope
	  TW_SCOPE_RELEASE(lock);
	  TW_YIELD();
	} // end temporary lock release scope
      }
      continue; // continue pushing
    com_fail:
      if (_done) break; // must have disconnected during push
      if (_verbose) cout << _obj_name << ": communication error while calling push()" << endl;
      _com_err = 1;
      break; // break from while loop -- done
    } // end while loop
    // done
    delete data;
    if (!_done) {
      _done = 1;
      _finish.broadcast();
    }
  } // end lock scope
  return 0;
}

void PushSupplier_i::disconnect_push_supplier()
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "PushSupplier", "PushSupplier_i::disconnect_push_supplier");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean PushSupplier_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "PushSupplier", "PushSupplier_i::wait_done");
    while (!_done) {
      _finish.wait();
    }
    // using tmp_worker ensures only one wait_done call tries join()
    tmp_worker = _worker;
    _worker = 0;
  } // end lock scope
  if (tmp_worker) {
    tmp_worker->join(0);
  }
  return _com_err;
}

void PushSupplier_i::subscription_change(const CosN::EventTypeSeq& added,
					 const CosN::EventTypeSeq& deled)
{
  if (_done) return; // see "Optimization Notes" at top
  TW_SCOPE_LOCK(lock, _oplock, "PushSupplier", "PushSupplier_i::subscription_change");
  if (_done) return;
  _num_changes++;
  if (_change_fn) (*_change_fn)(added, deled, _obj_name, _num_changes, _verbose);
  else if (_verbose) cout << _obj_name << ": subscription_change received [# " << _num_changes << "]" << endl;
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying subscription_change response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

// ---------------- CosNotifyComm::StructuredPushSupplier ---------------- //

StructuredPushSupplier_i::
StructuredPushSupplier_i(CosNA::StructuredProxyPushConsumer_ptr proxy,
			 CosNA::SupplierAdmin_ptr admin,
			 CosNF::Filter_ptr filter,
			 CORBA::ULong max_events, const char* objnm,
			 supply_structured_fn* supply_fn, type_change_fn* change_fn,
			 CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _my_admin(admin), _my_filters(0),
  _obj_name(objnm), _supply_fn(supply_fn), _change_fn(change_fn),
  _num_changes(0), _num_events(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose), _done(0), _com_err(0),
  _oplock(), _finish(&_oplock), _worker(0)
{
  // providing explict NULL for supply_fn is not OK -- must have a valid function
  if (!_supply_fn) _supply_fn = sample_supply_structured_fn;
  if (! CORBA::is_nil(filter)) {
    _my_filters.length(1);
    _my_filters[0] = filter;
  }
}

StructuredPushSupplier_i*
StructuredPushSupplier_i::create(CORBA::ORB_ptr orb,
				 CosNA::EventChannel_ptr channel,
				 CORBA::ULong max_events,
				 CORBA::ULong batch_size,
				 const char* objnm,
				 const char* proxy_ior_file,
				 const char* admin_ior_file,
				 supply_structured_fn* supply_fn,
				 type_change_fn* change_fn,
				 CosN::EventTypeSeq* evs_ptr,
				 const char* constraint_expr,
				 CORBA::ULong millisecs,
				 CORBA::ULong delay_millisecs,
				 CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosNA::SupplierAdmin_ptr admin = CosNA::SupplierAdmin::_nil();
  CosNA::ProxyConsumer_var generic_proxy =
    get_proxy_consumer(orb, channel, CosNA::STRUCTURED_EVENT, 1, admin_ior_file, admin, verbose); // 1 means push 0 means pull
  CosNA::StructuredProxyPushConsumer_ptr proxy = CosNA::StructuredProxyPushConsumer::_narrow(generic_proxy);
  if ( CORBA::is_nil(proxy) ) {
    return 0; // get_proxy_consumer failed
  }
  // If evs or constraint_expr are non-empty, add a filter to proxy
  CosNF::Filter_ptr filter = CosNF::Filter::_nil();
  if (evs_ptr) {
    CORBA::Boolean filt_err = sample_add_filter(channel, proxy, *evs_ptr, constraint_expr, objnm, filter, verbose);
    if (filt_err) {
      try {
	admin->destroy();
      } catch (...) { }
      return 0; // adding filter failed
    }
  }
  // write proxy IOR to file
  write_ior_to_file(orb, proxy, proxy_ior_file, verbose);
  // Construct a client
  StructuredPushSupplier_i* client =
    new StructuredPushSupplier_i(proxy, admin, filter, max_events, objnm, supply_fn, change_fn, millisecs, delay_millisecs, verbose);
  return client;
}

CORBA::Boolean StructuredPushSupplier_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "StructuredPushSupplier", "StructuredPushSupplier_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_structured_push_supplier(_this());
    if (_change_fn) {
      _my_proxy->obtain_subscription_types(CosNA::NONE_NOW_UPDATES_ON);
    } else {
      _my_proxy->obtain_subscription_types(CosNA::NONE_NOW_UPDATES_OFF);
    }
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEvCA::AlreadyConnected& ex) {
    cerr << _obj_name << ": Already connected" << endl;
    return 1; // error
  } catch (...) {
    cerr << _obj_name << ": Failed to connect" << endl;
    return 1; // error
  }
  // register the types to be supplied
  offer_ticker(_my_proxy, _obj_name, _verbose);
  // spawn a thread to do pushing
  if (_verbose) cout << _obj_name << ": Connected to proxy, ready to supply events" << endl; 
  _worker = new GenericBoundWorkerThread(this);
  return 0; // OK
}

void StructuredPushSupplier_i::cleanup() {
  CosNA::StructuredProxyPushConsumer_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "StructuredPushSupplier", "StructuredPushSupplier_i::connect");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    if (CORBA::is_nil(_my_proxy)) {
      cerr << "Coding error: cleanup already called?" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosNA::StructuredProxyPushConsumer::_nil();
  } // end lock scope
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_structured_push_consumer();
  } catch(...) {}
  try {
    _my_admin->destroy();
  } catch (...) { }
  _my_admin = CosNA::SupplierAdmin::_nil();
  destroy_filters(_my_filters);
}

// Note: lock is *not* held during push_structured_event(), yield() and timedwait() calls
void* StructuredPushSupplier_i::start_working(void *) {
  CosNC::StructuredPushSupplier_var bump_my_refcount_during_outcalls = _this();

  unsigned long t_secs = 0, t_nanosecs = 0;
  unsigned long d_secs = 0, d_nanosecs = 0;
  if (_millisecs) {
    d_secs = _millisecs / 1000;
    d_nanosecs = (_millisecs % 1000) * 1000000;
  }
  CosN::StructuredEvent* data = new CosN::StructuredEvent;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "StructuredPushSupplier", "StructuredPushSupplier_i::start_working");

    if (_verbose && (!_done)) cout << _obj_name << ": Spawned thread entering main push loop" << endl;
    while ( 1 ) {
      if (_done) break; // must have disconnected
      if ((*_supply_fn)(*data, _obj_name, _num_events+1, _verbose)) {
	{ // introduce temporary lock release scope; do not hold oplock across push
	  TW_SCOPE_RELEASE(lock);
	  try {
	    _my_proxy->push_structured_event(*data);
	  } catch (...) {
	    goto com_fail;
	  }
	} // end temporary lock release scope
	if (_done) break; // must have disconnected during push
	_num_events++;
	if (_max_events && (_num_events >= _max_events)) {
	  if (_verbose) cout << _obj_name << ": DONE [max_events reached]" << endl;
	  break; // done
	}
      }
      // wait or yield then try again
      if (_millisecs) { // sleep for specified interval
	TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
	_finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      } else { // use yield to let other threads have a shot at _oplock
	{ // introduce temporary lock release scope
	  TW_SCOPE_RELEASE(lock);
	  TW_YIELD();
	} // end temporary lock release scope
      }
      continue; // continue pushing
    com_fail:
      if (_done) break; // must have disconnected during push
      if (_verbose) cout << _obj_name << ": communication error while calling push_structured_event()" << endl;
      _com_err = 1;
      break; // break from while loop -- done
    } // end while loop
    // done
    delete data;
    if (!_done) {
      _done = 1;
      _finish.broadcast();
    }
  } // end lock scope
  return 0;
}

void StructuredPushSupplier_i::disconnect_structured_push_supplier()
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "StructuredPushSupplier", "StructuredPushSupplier_i::disconnect_structured_push_supplier");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean StructuredPushSupplier_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "StructuredPushSupplier", "StructuredPushSupplier_i::wait_done");
    while (!_done) {
      _finish.wait();
    }
    // using tmp_worker ensures only one wait_done call tries join()
    tmp_worker = _worker;
    _worker = 0;
  } // end lock scope
  if (tmp_worker) {
    tmp_worker->join(0);
  }
  return _com_err;
}

void StructuredPushSupplier_i::subscription_change(const CosN::EventTypeSeq& added,
						   const CosN::EventTypeSeq& deled)
{
  if (_done) return; // see "Optimization Notes" at top
  TW_SCOPE_LOCK(lock, _oplock, "StructuredPushSupplier", "StructuredPushSupplier_i::subscription_change");
  if (_done) return;
  _num_changes++;
  if (_change_fn) (*_change_fn)(added, deled, _obj_name, _num_changes, _verbose);
  else if (_verbose) cout << _obj_name << ": subscription_change received [# " << _num_changes << "]" << endl;
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying subscription_change response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

// 512 version //

Structured512PushSupplier_i::
Structured512PushSupplier_i(CosNA::StructuredProxyPushConsumer_ptr proxy,
			    CosNA::SupplierAdmin_ptr admin,
			    CosNF::Filter_ptr filter,
			    CORBA::ULong max_events, const char* objnm,
			    supply_structured_fn* supply_fn, type_change_fn* change_fn,
			    CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _my_admin(admin), _my_filters(0),
  _obj_name(objnm), _supply_fn(supply_fn), _change_fn(change_fn),
  _num_changes(0), _num_events(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose), _done(0), _com_err(0),
  _oplock(), _finish(&_oplock), _worker(0)
{
  // providing explict NULL for supply_fn is not OK -- must have a valid function
  if (!_supply_fn) _supply_fn = sample_supply_structured_fn;
  if (! CORBA::is_nil(filter)) {
    _my_filters.length(1);
    _my_filters[0] = filter;
  }
}

Structured512PushSupplier_i*
Structured512PushSupplier_i::create(CORBA::ORB_ptr orb,
				    CosNA::EventChannel_ptr channel,
				    CORBA::ULong max_events,
				    CORBA::ULong batch_size,
				    const char* objnm,
				    const char* proxy_ior_file,
				    const char* admin_ior_file,
				    supply_structured_fn* supply_fn,
				    type_change_fn* change_fn,
				    CosN::EventTypeSeq* evs_ptr,
				    const char* constraint_expr,
				    CORBA::ULong millisecs,
				    CORBA::ULong delay_millisecs,
				    CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosNA::SupplierAdmin_ptr admin = CosNA::SupplierAdmin::_nil();
  CosNA::ProxyConsumer_var generic_proxy =
    get_proxy_consumer(orb, channel, CosNA::STRUCTURED_EVENT, 1, admin_ior_file, admin, verbose); // 1 means push 0 means pull
  CosNA::StructuredProxyPushConsumer_ptr proxy = CosNA::StructuredProxyPushConsumer::_narrow(generic_proxy);
  if ( CORBA::is_nil(proxy) ) {
    return 0; // get_proxy_consumer failed
  }
  // If evs or constraint_expr are non-empty, add a filter to proxy
  CosNF::Filter_ptr filter = CosNF::Filter::_nil();
  if (evs_ptr) {
    CORBA::Boolean filt_err = sample_add_filter(channel, proxy, *evs_ptr, constraint_expr, objnm, filter, verbose);
    if (filt_err) {
      try {
	admin->destroy();
      } catch (...) { }
      return 0; // adding filter failed
    }
  }
  // write proxy IOR to file
  write_ior_to_file(orb, proxy, proxy_ior_file, verbose);
  // Construct a client
  Structured512PushSupplier_i* client =
    new Structured512PushSupplier_i(proxy, admin, filter, max_events, objnm, supply_fn, change_fn, millisecs, delay_millisecs, verbose);
  return client;
}

CORBA::Boolean Structured512PushSupplier_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "Structured512PushSupplier", "Structured512PushSupplier_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_structured_push_supplier(_this());
    if (_change_fn) {
      _my_proxy->obtain_subscription_types(CosNA::NONE_NOW_UPDATES_ON);
    } else {
      _my_proxy->obtain_subscription_types(CosNA::NONE_NOW_UPDATES_OFF);
    }
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEvCA::AlreadyConnected& ex) {
    cerr << _obj_name << ": Already connected" << endl;
    return 1; // error
  } catch (...) {
    cerr << _obj_name << ": Failed to connect" << endl;
    return 1; // error
  }
  // spawn a thread to do pushing
  if (_verbose) cout << _obj_name << ": Connected to proxy, ready to supply events" << endl; 
  _worker = new GenericBoundWorkerThread(this);
  return 0; // OK
}

void Structured512PushSupplier_i::cleanup() {
  CosNA::StructuredProxyPushConsumer_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "Structured512PushSupplier", "Structured512PushSupplier_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    if (CORBA::is_nil(_my_proxy)) {
      cerr << "Coding error: cleanup already called?" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosNA::StructuredProxyPushConsumer::_nil();
  } // end lock scope
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_structured_push_consumer();
  } catch(...) {}
  try {
    _my_admin->destroy();
  } catch (...) { }
  _my_admin = CosNA::SupplierAdmin::_nil();
  destroy_filters(_my_filters);
}

// Note: lock is *not* held during push_structured_event(), yield() and timedwait() calls
void* Structured512PushSupplier_i::start_working(void *) {
  CosNC::StructuredPushSupplier_var bump_my_refcount_during_outcalls = _this();

  unsigned long t_secs = 0, t_nanosecs = 0;
  unsigned long d_secs = 0, d_nanosecs = 0;
  if (_millisecs) {
    d_secs = _millisecs / 1000;
    d_nanosecs = (_millisecs % 1000) * 1000000;
  }
  CosN::StructuredEvent* data = new CosN::StructuredEvent;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "Structured512PushSupplier", "Structured512PushSupplier_i::start_working");
    if (_verbose && (!_done)) cout << _obj_name << ": Spawned thread entering main push loop" << endl;
    while ( 1 ) {
      if (_done) break; // must have disconnected
      if ((*_supply_fn)(*data, _obj_name, _num_events+1, _verbose)) {
	{ // introduce temporary lock release scope; do not hold oplock across push
	  TW_SCOPE_RELEASE(lock);
	  try {
	    _my_proxy->push_structured_event(*data);
	  } catch (...) {
	    goto com_fail;
	  }
	} // end temporary lock release scope
	if (_done) break; // must have disconnected during push
	_num_events++;
	if (_max_events && (_num_events >= _max_events)) {
	  if (_verbose) cout << _obj_name << ": DONE [max_events reached]" << endl;
	  break; // done
	}
      }
      // wait or yield then try again
      if (_millisecs) { // sleep for specified interval
	TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
	_finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      } else { // use yield to let other threads have a shot at _oplock
	{ // introduce temporary lock release scope
	  TW_SCOPE_RELEASE(lock);
	  TW_YIELD();
	} // end temporary lock release scope
      }
      continue; // continue pushing
    com_fail:
      if (_done) break; // must have disconnected during push
      if (_verbose) cout << _obj_name << ": communication error while calling push_structured_event()" << endl;
      _com_err = 1;
      break; // break from while loop -- done
    } //end while loop
    // done
    delete data;
    if (!_done) {
      _done = 1;
      _finish.broadcast();
    }
  } // end lock scope
  return 0;
}

void Structured512PushSupplier_i::disconnect_structured_push_supplier()
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "Structured512PushSupplier", "Structured512PushSupplier_i::disconnect_structured_push_supplier");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean Structured512PushSupplier_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "Structured512PushSupplier", "Structured512PushSupplier_i::wait_done");
    while (!_done) {
      _finish.wait();
    }
    // using tmp_worker ensures only one wait_done call tries join()
    tmp_worker = _worker;
    _worker = 0;
  } // end lock scope
  if (tmp_worker) {
    tmp_worker->join(0);
  }
  return _com_err;
}

void Structured512PushSupplier_i::subscription_change(const CosN::EventTypeSeq& added,
						      const CosN::EventTypeSeq& deled)
{
  if (_done) return; // see "Optimization Notes" at top
  TW_SCOPE_LOCK(lock, _oplock, "Structured512PushSupplier", "Structured512PushSupplier_i::subscription_change");
  if (_done) return;
  _num_changes++;
  if (_change_fn) (*_change_fn)(added, deled, _obj_name, _num_changes, _verbose);
  else if (_verbose) cout << _obj_name << ": subscription_change received [# " << _num_changes << "]" << endl;
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying subscription_change response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

// ---------------- CosNotifyComm::SequencePushSupplier ---------------- //

SequencePushSupplier_i::
SequencePushSupplier_i(CosNA::SequenceProxyPushConsumer_ptr proxy,
		       CosNA::SupplierAdmin_ptr admin,
		       CosNF::Filter_ptr filter,
		       CORBA::ULong max_events,
		       CORBA::ULong batch_size,
		       const char* objnm,
		       supply_batch_fn* supply_fn,
		       type_change_fn* change_fn,
		       CORBA::ULong millisecs,
		       CORBA::ULong delay_millisecs,
		       CORBA::Boolean verbose) :
  _my_proxy(proxy), _my_admin(admin), _my_filters(0),
  _obj_name(objnm), _supply_fn(supply_fn), _change_fn(change_fn),
  _num_changes(0), _num_events(0), _batch_size(batch_size), _num_batches(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose), _done(0), _com_err(0),
  _oplock(), _finish(&_oplock), _worker(0)
{
  // providing explict NULL for supply_fn is not OK -- must have a valid function
  if (!_supply_fn) _supply_fn = sample_supply_batch_fn;
  if (! CORBA::is_nil(filter)) {
    _my_filters.length(1);
    _my_filters[0] = filter;
  }
}

SequencePushSupplier_i*
SequencePushSupplier_i::create(CORBA::ORB_ptr orb,
			       CosNA::EventChannel_ptr channel,
			       CORBA::ULong max_events,
			       CORBA::ULong batch_size,
			       const char* objnm,
			       const char* proxy_ior_file,
			       const char* admin_ior_file,
			       supply_batch_fn* supply_fn,
			       type_change_fn* change_fn,
			       CosN::EventTypeSeq* evs_ptr,
			       const char* constraint_expr,
			       CORBA::ULong millisecs,
			       CORBA::ULong delay_millisecs,
			       CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosNA::SupplierAdmin_ptr admin = CosNA::SupplierAdmin::_nil();
  CosNA::ProxyConsumer_var generic_proxy =
    get_proxy_consumer(orb, channel, CosNA::SEQUENCE_EVENT, 1, admin_ior_file, admin, verbose); // 1 means push 0 means pull
  CosNA::SequenceProxyPushConsumer_ptr proxy = CosNA::SequenceProxyPushConsumer::_narrow(generic_proxy);
  if ( CORBA::is_nil(proxy) ) {
    return 0; // get_proxy_consumer failed
  }
  // If evs or constraint_expr are non-empty, add a filter to proxy
  CosNF::Filter_ptr filter = CosNF::Filter::_nil();
  if (evs_ptr) {
    CORBA::Boolean filt_err = sample_add_filter(channel, proxy, *evs_ptr, constraint_expr, objnm, filter, verbose);
    if (filt_err) {
      try {
	admin->destroy();
      } catch (...) { }
      return 0; // adding filter failed
    }
  }
  // write proxy IOR to file
  write_ior_to_file(orb, proxy, proxy_ior_file, verbose);
  // Construct a client
  SequencePushSupplier_i* client =
    new SequencePushSupplier_i(proxy, admin, filter, max_events, batch_size, objnm, supply_fn, change_fn, millisecs, delay_millisecs, verbose);
  return client;
}

CORBA::Boolean SequencePushSupplier_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "SequencePushSupplier", "SequencePushSupplier_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_sequence_push_supplier(_this());
    if (_change_fn) {
      _my_proxy->obtain_subscription_types(CosNA::NONE_NOW_UPDATES_ON);
    } else {
      _my_proxy->obtain_subscription_types(CosNA::NONE_NOW_UPDATES_OFF);
    }
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEvCA::AlreadyConnected& ex) {
    cerr << _obj_name << ": Already connected" << endl;
    return 1; // error
  } catch (...) {
    cerr << _obj_name << ": Failed to connect" << endl;
    return 1; // error
  }
  // register the types to be supplied
  offer_ticker(_my_proxy, _obj_name, _verbose);
  // spawn a thread to do pushing
  if (_verbose) cout << _obj_name << ": Connected to proxy, ready to supply events" << endl; 
  _worker = new GenericBoundWorkerThread(this);
  return 0; // OK
}

void SequencePushSupplier_i::cleanup() {
  CosNA::SequenceProxyPushConsumer_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "SequencePushSupplier", "SequencePushSupplier_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    if (CORBA::is_nil(_my_proxy)) {
      cerr << "Coding error: cleanup already called?" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosNA::SequenceProxyPushConsumer::_nil();
  }
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_sequence_push_consumer();
  } catch(...) {}
  try {
    _my_admin->destroy();
  } catch (...) { }
  _my_admin = CosNA::SupplierAdmin::_nil();
  destroy_filters(_my_filters);
}

// Note: lock is *not* held during push_structurd_events(), yield() and timedwait() calls
void* SequencePushSupplier_i::start_working(void *) {
  CosNC::SequencePushSupplier_var bump_my_refcount_during_outcalls = _this();

  unsigned long t_secs = 0, t_nanosecs = 0;
  unsigned long d_secs = 0, d_nanosecs = 0;
  if (_millisecs) {
    d_secs = _millisecs / 1000;
    d_nanosecs = (_millisecs % 1000) * 1000000;
  }
  CosN::EventBatch* data = new CosN::EventBatch;
  data->length(0);
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "SequencePushSupplier", "SequencePushSupplier_i::start_working");

    if (_verbose && (!_done)) cout << _obj_name << ": Spawned thread entering main push loop" << endl;
    while ( 1 ) {
      if (_done) break; // must have disconnected
      data->length(0);
      if ( ((*_supply_fn)(*data, _obj_name, _batch_size, _num_events+1, _num_batches+1, _verbose)) && data->length() ) {
	{ // introduce temporary lock release scope; do not hold oplock across push
	  TW_SCOPE_RELEASE(lock);
	  try {
	    _my_proxy->push_structured_events(*data);
	  } catch (...) {
	    goto com_fail;
	  }
	} // end temporary lock release scope
	if (_done) break; // must have disconnected during push
	_num_batches++;
	_num_events += data->length();
	if (_max_events && (_num_events >= _max_events)) {
	  if (_verbose) cout << _obj_name << ": DONE [max_events reached]" << endl;
	  break; // done
	}
      }
      // wait or yield then try again
      if (_millisecs) { // sleep for specified interval
	TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
	_finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      } else { // use yield to let other threads have a shot at _oplock
	{ // introduce temporary lock release scope
	  TW_SCOPE_RELEASE(lock);
	  TW_YIELD();
	}
      }
      continue; // continue pushing
    com_fail:
      if (_done) break; // must have disconnected during push
      if (_verbose) cout << _obj_name << ": communication error while calling push_structured_events()" << endl;
      _com_err = 1;
      break; // break from while loop -- done
    } // end while loop
    // done
    delete data;
    if (!_done) {
      _done = 1;
      _finish.broadcast();
    }
  } // end lock scope
  return 0;
}

void SequencePushSupplier_i::disconnect_sequence_push_supplier()
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "SequencePushSupplier", "SequencePushSupplier_i::disconnect_sequence_push_supplier");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean SequencePushSupplier_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "SequencePushSupplier", "SequencePushSupplier_i::wait_done");
    while (!_done) {
      _finish.wait();
    }
    // using tmp_worker ensures only one wait_done call tries join()
    tmp_worker = _worker;
    _worker = 0;
  } // end lock scope
  if (tmp_worker) {
    tmp_worker->join(0);
  }
  return _com_err;
}

void SequencePushSupplier_i::subscription_change(const CosN::EventTypeSeq& added,
						 const CosN::EventTypeSeq& deled)
{
  if (_done) return; // see "Optimization Notes" at top
  TW_SCOPE_LOCK(lock, _oplock, "SequencePushSupplier", "SequencePushSupplier_i::subscription_change");
  if (_done) return;
  _num_changes++;
  if (_change_fn) (*_change_fn)(added, deled, _obj_name, _num_changes, _verbose);
  else if (_verbose) cout << _obj_name << ": subscription_change received [# " << _num_changes << "]" << endl;
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying subscription_change response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}


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

PullSupplier_i::
PullSupplier_i(CosNA::ProxyPullConsumer_ptr proxy,
	       CosNA::SupplierAdmin_ptr admin,
	       CosNF::Filter_ptr filter,
	       CORBA::ULong max_events, const char* objnm,
	       supply_any_fn* supply_fn, type_change_fn* change_fn,
	       CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _my_admin(admin), _my_filters(0),
  _obj_name(objnm), _supply_fn(supply_fn), _change_fn(change_fn),
  _num_changes(0), _num_events(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose), _done(0), _com_err(0),
  _oplock(), _finish(&_oplock), _worker(0)
{
  // providing explict NULL for supply_fn is not OK -- must have a valid function
  if (!_supply_fn) _supply_fn = sample_supply_any_fn;
  if (! CORBA::is_nil(filter)) {
    _my_filters.length(1);
    _my_filters[0] = filter;
  }
}

PullSupplier_i*
PullSupplier_i::create(CORBA::ORB_ptr orb,
		       CosNA::EventChannel_ptr channel,
		       CORBA::ULong max_events,
		       CORBA::ULong batch_size,
		       const char* objnm,
		       const char* proxy_ior_file,
		       const char* admin_ior_file,
		       supply_any_fn* supply_fn,
		       type_change_fn* change_fn,
		       CosN::EventTypeSeq* evs_ptr,
		       const char* constraint_expr,
		       CORBA::ULong millisecs,
		       CORBA::ULong delay_millisecs,
		       CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosNA::SupplierAdmin_ptr admin = CosNA::SupplierAdmin::_nil();
  CosNA::ProxyConsumer_var generic_proxy =
    get_proxy_consumer(orb, channel, CosNA::ANY_EVENT, 0, admin_ior_file, admin, verbose); // 1 means push 0 means pull
  CosNA::ProxyPullConsumer_ptr proxy = CosNA::ProxyPullConsumer::_narrow(generic_proxy);
  if ( CORBA::is_nil(proxy) ) {
    return 0; // get_proxy_consumer failed
  }
  // If evs or constraint_expr are non-empty, add a filter to proxy
  CosNF::Filter_ptr filter = CosNF::Filter::_nil();
  if (evs_ptr) {
    CORBA::Boolean filt_err = sample_add_filter(channel, proxy, *evs_ptr, constraint_expr, objnm, filter, verbose);
    if (filt_err) {
      try {
	admin->destroy();
      } catch (...) { }
      return 0; // adding filter failed
    }
  }
  // write proxy IOR to file
  write_ior_to_file(orb, proxy, proxy_ior_file, verbose);
  // Construct a client
  PullSupplier_i* client =
    new PullSupplier_i(proxy, admin, filter, max_events, objnm, supply_fn, change_fn, millisecs, delay_millisecs, verbose);
  return client;
}

CORBA::Boolean PullSupplier_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "PullSupplier", "PullSupplier_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_any_pull_supplier(_this());
    if (_change_fn) {
      _my_proxy->obtain_subscription_types(CosNA::NONE_NOW_UPDATES_ON);
    } else {
      _my_proxy->obtain_subscription_types(CosNA::NONE_NOW_UPDATES_OFF);
    }
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEvCA::AlreadyConnected& ex) {
    cerr << _obj_name << ": Already connected" << endl;
    return 1; // error
  } catch (...) {
    cerr << _obj_name << ": Failed to connect" << endl;
    return 1; // error
  }
  if (_verbose) cout << _obj_name << ": Connected to proxy, ready to supply events" << endl; 
  // register the types to be supplied
  offer_any(_my_proxy, _obj_name, _verbose);
  // if _millisecs is set, spawn a thread to ping the proxy
  if (_millisecs)
    _worker = new GenericBoundWorkerThread(this);
  return 0; // OK
}

void PullSupplier_i::cleanup() {
  CosNA::ProxyPullConsumer_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "PullSupplier", "PullSupplier_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    if (CORBA::is_nil(_my_proxy)) {
      cerr << "Coding error: cleanup already called?" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosNA::ProxyPullConsumer::_nil();
  } // end lock scope
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_pull_consumer();
  } catch(...) {}
  try {
    _my_admin->destroy();
  } catch (...) { }
  _my_admin = CosNA::SupplierAdmin::_nil();
  destroy_filters(_my_filters);
}

// While _millisecs is positive, pings proxy every _millisecs milliseconds.
// Sets error code and sets _done to 1 if there is an error during ping.
void* PullSupplier_i::start_working(void *) {
  CosNC::PullSupplier_var bump_my_refcount_during_outcalls = _this();

  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "PullSupplier", "PullSupplier_i::start_working");

    if (_verbose && (!_done) && _millisecs) cout << _obj_name << ": Spawned thread entering ping loop" << endl;
    while ( 1 ) {
      if (_done || (_millisecs == 0)) break;
      unsigned long t_secs = 0, t_nanosecs = 0;
      unsigned long d_secs = 0, d_nanosecs = 0;
      d_secs = _millisecs / 1000;
      d_nanosecs = (_millisecs % 1000) * 1000000;
      { // introduce temporary lock release scope; do not hold oplock across ping
	TW_SCOPE_RELEASE(lock);
	try {
	  _my_proxy->MyType();
	} catch (...) {
	  goto com_fail;
	}
      } // end temporary lock release scope
      if (_done) break; // must have disconnected during pull
      // sleep for specified interval
      TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
      _finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      continue; // continue ping loop
    com_fail:
      if (_done) break;
      if (_verbose) cout << _obj_name << ": communication error while pinging proxy using MyType()" << endl;
      _done = 1;
      _finish.broadcast();
      _com_err = 1;
      break; // break from while loop -- done
    } // end while loop
    // done
  } // end lock scope
  return 0;
}

CORBA::Any* PullSupplier_i::try_pull(CORBA::Boolean& has_event) {
  CORBA::Any* data = new CORBA::Any;
  has_event = 0;

  if (_done) return data; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "PullSupplier", "PullSupplier_i::try_pull");
  if (_done) return data;
  if (_verbose) cout << _obj_name << ": Channel called try_pull" << endl;

  // If we reached done point during last try_pull, it is broadcast now
  // and we return from this try_pull call without an event.
  // Waiting for one extra try_pull ensures the channel has successfully
  // processed the previous try_pull (the final supplied event) before
  // we disconnect this supplier.
  if (_max_events && (_num_events >= _max_events)) {
    if (_verbose) cout << _obj_name << ": DONE [max_events reached; we waited for 1 extra try_pull]" << endl;
    _done = 1;
    _finish.broadcast();
    if (_verbose) cout << _obj_name << ": NOT returning an event" << endl;
    return data;
  }

  if ((*_supply_fn)(*data, _obj_name, _num_events+1, _verbose)) {
    has_event = 1;
    _num_events++;
  } else {
    if (_verbose) cout << _obj_name << ": NOT returning an event" << endl;
  }
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying try_pull response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
  return data;
}

CORBA::Any* PullSupplier_i::pull() {
  if (_done) // see "Optimization Notes" at top
    throw CORBA::INV_OBJREF(0, CORBA::COMPLETED_NO);

  CORBA::Any* data = new CORBA::Any;
  unsigned long t_secs = 0, t_nanosecs = 0;
  unsigned long d_secs = 0, d_nanosecs = 0;
  if (_millisecs) {
    d_secs = _millisecs / 1000;
    d_nanosecs = (_millisecs % 1000) * 1000000;
  }
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "PullSupplier", "PullSupplier_i::pull");

    if (_verbose && (!_done)) cout << _obj_name << ": Channel called pull" << endl;
    // invariants: _oplock is held entering top of loop and 
    // also whenever we break out of loop
    // (it is *not* held during yield() and timedwait() calls)
    while ( 1 ) {
      if (_done) {
	delete data;
	throw CORBA::INV_OBJREF(0, CORBA::COMPLETED_NO);
      }
      if ((*_supply_fn)(*data, _obj_name, _num_events+1, _verbose)) {
	_num_events++;
	if (_max_events && (_num_events >= _max_events)) {
	  if (_verbose) cout << _obj_name << ": DONE [max_events reached]" << endl;
	  _done = 1;
	  _finish.broadcast();
	}
	break; // return data
      }
      // wait or yield then try again
      if (_millisecs) { // sleep for specified interval
	TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
	_finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      } else { // use yield to let other threads have a shot at _oplock
	{ // introduce temporary lock release scope
	  TW_SCOPE_RELEASE(lock);
	  TW_YIELD();
	} // end temporary lock release scope
      }
      continue;
    } // end while loop
    if (!_done && _delay_millisecs) {
      unsigned long d_secs     =  _delay_millisecs / 1000;
      unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
      if (_verbose) cout << _obj_name << ": delaying pull response for " << _delay_millisecs << " millisecs" << endl;
      unsigned long t_secs, t_nanosecs;
      TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
      _finish.timedwait(t_secs, t_nanosecs);
    }
  } // end lock scope
  return data;
}

void PullSupplier_i::disconnect_pull_supplier()
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "PullSupplier", "PullSupplier_i::disconnect_pull_supplier");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean PullSupplier_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "PullSupplier", "PullSupplier_i::wait_done");
    while (!_done) {
      _finish.wait();
    }
    // using tmp_worker ensures only one wait_done call tries join()
    tmp_worker = _worker;
    _worker = 0;
  } // end lock scope
  if (tmp_worker) {
    tmp_worker->join(0);
  }
  return _com_err;
}

void PullSupplier_i::subscription_change(const CosN::EventTypeSeq& added,
					 const CosN::EventTypeSeq& deled)
{
  if (_done) return; // see "Optimization Notes" at top
  TW_SCOPE_LOCK(lock, _oplock, "PullSupplier", "PullSupplier_i::subscription_change");
  if (_done) return;
  _num_changes++;
  if (_change_fn) (*_change_fn)(added, deled, _obj_name, _num_changes, _verbose);
  else if (_verbose) cout << _obj_name << ": subscription_change received [# " << _num_changes << "]" << endl;
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying subscription_change response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

// ------------ CosNotifyComm::StructuredPullSupplier ------------ //

StructuredPullSupplier_i::
StructuredPullSupplier_i(CosNA::StructuredProxyPullConsumer_ptr proxy,
			 CosNA::SupplierAdmin_ptr admin,
			 CosNF::Filter_ptr filter,
			 CORBA::ULong max_events, const char* objnm,
			 supply_structured_fn* supply_fn, type_change_fn* change_fn,
			 CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _my_admin(admin), _my_filters(0),
  _obj_name(objnm), _supply_fn(supply_fn), _change_fn(change_fn),
  _num_changes(0), _num_events(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose), _done(0), _com_err(0),
  _oplock(), _finish(&_oplock), _worker(0)
{
  // providing explict NULL for supply_fn is not OK -- must have a valid function
  if (!_supply_fn) _supply_fn = sample_supply_structured_fn;
  if (! CORBA::is_nil(filter)) {
    _my_filters.length(1);
    _my_filters[0] = filter;
  }
}

StructuredPullSupplier_i*
StructuredPullSupplier_i::create(CORBA::ORB_ptr orb,
				 CosNA::EventChannel_ptr channel,
				 CORBA::ULong max_events,
				 CORBA::ULong batch_size,
				 const char* objnm,
				 const char* proxy_ior_file,
				 const char* admin_ior_file,
				 supply_structured_fn* supply_fn,
				 type_change_fn* change_fn,
				 CosN::EventTypeSeq* evs_ptr,
				 const char* constraint_expr,
				 CORBA::ULong millisecs,
				 CORBA::ULong delay_millisecs,
				 CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosNA::SupplierAdmin_ptr admin = CosNA::SupplierAdmin::_nil();
  CosNA::ProxyConsumer_var generic_proxy =
    get_proxy_consumer(orb, channel, CosNA::STRUCTURED_EVENT, 0, admin_ior_file, admin, verbose); // 1 means push 0 means pull
  CosNA::StructuredProxyPullConsumer_ptr proxy = CosNA::StructuredProxyPullConsumer::_narrow(generic_proxy);
  if ( CORBA::is_nil(proxy) ) {
    return 0; // get_proxy_consumer failed
  }
  // If evs or constraint_expr are non-empty, add a filter to proxy
  CosNF::Filter_ptr filter = CosNF::Filter::_nil();
  if (evs_ptr) {
    CORBA::Boolean filt_err = sample_add_filter(channel, proxy, *evs_ptr, constraint_expr, objnm, filter, verbose);
    if (filt_err) {
      try {
	admin->destroy();
      } catch (...) { }
      return 0; // adding filter failed
    }
  }
  // write proxy IOR to file
  write_ior_to_file(orb, proxy, proxy_ior_file, verbose);
  // Construct a client
  StructuredPullSupplier_i* client =
    new StructuredPullSupplier_i(proxy, admin, filter, max_events, objnm, supply_fn, change_fn, millisecs, delay_millisecs, verbose);
  return client;
}

CORBA::Boolean StructuredPullSupplier_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "StructuredPullSupplier", "StructuredPullSupplier_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_structured_pull_supplier(_this());
    if (_change_fn) {
      _my_proxy->obtain_subscription_types(CosNA::NONE_NOW_UPDATES_ON);
    } else {
      _my_proxy->obtain_subscription_types(CosNA::NONE_NOW_UPDATES_OFF);
    }
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEvCA::AlreadyConnected& ex) {
    cerr << _obj_name << ": Already connected" << endl;
    return 1; // error
  } catch (...) {
    cerr << _obj_name << ": Failed to connect" << endl;
    return 1; // error
  }
  if (_verbose) cout << _obj_name << ": Connected to proxy, ready to supply events" << endl; 
  // register the types to be supplied
  offer_ticker(_my_proxy, _obj_name, _verbose);
  // if _millisecs is set, spawn a thread to ping the proxy
  if (_millisecs)
    _worker = new GenericBoundWorkerThread(this);
  return 0; // OK
}

void StructuredPullSupplier_i::cleanup() {
  CosNA::StructuredProxyPullConsumer_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "StructuredPullSupplier", "StructuredPullSupplier_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    if (CORBA::is_nil(_my_proxy)) {
      cerr << "Coding error: cleanup already called?" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosNA::StructuredProxyPullConsumer::_nil();
  } // end lock scope
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy))) {
      proxy->disconnect_structured_pull_consumer();
    }
  } catch(...) {}
  try {
    _my_admin->destroy();
  } catch (...) { }
  _my_admin = CosNA::SupplierAdmin::_nil();
  destroy_filters(_my_filters);
}

// While _millisecs is positive, pings proxy every _millisecs milliseconds.
// Sets error code and sets _done to 1 if there is an error during ping.
void* StructuredPullSupplier_i::start_working(void *) {
  CosNC::StructuredPullSupplier_var bump_my_refcount_during_outcalls = _this();

  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "StructuredPullSupplier", "StructuredPullSupplier_i::start_working");

    if (_verbose && (!_done) && _millisecs) cout << _obj_name << ": Spawned thread entering ping loop" << endl;
    while ( 1 ) {
      if (_done || (_millisecs == 0)) break;
      unsigned long t_secs = 0, t_nanosecs = 0;
      unsigned long d_secs = 0, d_nanosecs = 0;
      d_secs = _millisecs / 1000;
      d_nanosecs = (_millisecs % 1000) * 1000000;
      { // introduce temporary lock release scope; do not hold oplock across ping
	TW_SCOPE_RELEASE(lock);
	try {
	  _my_proxy->MyType();
	} catch (...) {
	  goto com_fail;
	}
      } // end temporary lock release scope
      if (_done) break; // must have disconnected during pull
      // sleep for specified interval
      TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
      _finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      continue; // continue ping loop
    com_fail:
      if (_done) break;
      if (_verbose) cout << _obj_name << ": communication error while pinging proxy using MyType()" << endl;
      _done = 1;
      _finish.broadcast();
      _com_err = 1;
      break; // break from while loop -- done
    } // end while loop
    // done
  } // end lock scope
  return 0;
}

CosN::StructuredEvent* StructuredPullSupplier_i::try_pull_structured_event(CORBA::Boolean& has_event) {
  CosN::StructuredEvent* data = new CosN::StructuredEvent;
  has_event = 0;

  if (_done) return data; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "StructuredPullSupplier", "StructuredPullSupplier_i::try_pull_structured_event");
  if (_done) return data;
  if (_verbose) cout << _obj_name << ": Channel called try_pull_structured_event" << endl;

  // If we reached done point during last try_pull, it is broadcast now
  // and we return from this try_pull call without an event.
  // Waiting for one extra try_pull ensures the channel has successfully
  // processed the previous try_pull (the final supplied event) before
  // we disconnect this supplier.
  if (_max_events && (_num_events >= _max_events)) {
    if (_verbose) cout << _obj_name << ": DONE [max_events reached; we waited for 1 extra try_pull]" << endl;
    _done = 1;
    _finish.broadcast();
    if (_verbose) cout << _obj_name << ": NOT returning an event" << endl;
    return data;
  }

  if ((*_supply_fn)(*data, _obj_name, _num_events+1, _verbose)) {
    has_event = 1;
    _num_events++;
  } else {
    if (_verbose) cout << _obj_name << ": NOT returning an event" << endl;
  }
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying try_pull_structured_event response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
  return data;
}

CosN::StructuredEvent* StructuredPullSupplier_i::pull_structured_event() {
  if (_done) // see "Optimization Notes" at top
    throw CORBA::INV_OBJREF(0, CORBA::COMPLETED_NO);

  CosN::StructuredEvent* data = new CosN::StructuredEvent;
  unsigned long t_secs = 0, t_nanosecs = 0;
  unsigned long d_secs = 0, d_nanosecs = 0;
  if (_millisecs) {
    d_secs = _millisecs / 1000;
    d_nanosecs = (_millisecs % 1000) * 1000000;
  }
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "StructuredPullSupplier", "StructuredPullSupplier_i::pull_structured_event");

    if (_verbose && (!_done)) cout << _obj_name << ": Channel called pull_structured_event" << endl;
    // invariants: _oplock is held entering top of loop and 
    // also whenever we break out of loop
    // (it is *not* held during yield() and timedwait() calls)
    while ( 1 ) {
      if (_done) {
	delete data;
	throw CORBA::INV_OBJREF(0, CORBA::COMPLETED_NO);
      }
      if ((*_supply_fn)(*data, _obj_name, _num_events+1, _verbose)) {
	_num_events++;
	if (_max_events && (_num_events >= _max_events)) {
	  if (_verbose) cout << _obj_name << ": DONE [max_events reached]" << endl;
	  _done = 1;
	  _finish.broadcast();
	}
	break; // return data
      }
      // wait or yield then try again
      if (_millisecs) { // sleep for specified interval
	TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
	_finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      } else { // use yield to let other threads have a shot at _oplock
	{ // introduce temporary lock release scope
	  TW_SCOPE_RELEASE(lock);
	  TW_YIELD();
	} // end temporary lock release scope
      }
      continue;
    } // end while loop
    if (!_done && _delay_millisecs) {
      unsigned long d_secs     =  _delay_millisecs / 1000;
      unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
      if (_verbose) cout << _obj_name << ": delaying pull_structured_event response for " << _delay_millisecs << " millisecs" << endl;
      unsigned long t_secs, t_nanosecs;
      TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
      _finish.timedwait(t_secs, t_nanosecs);
    }
  } // end lock scope
  return data;
}

void StructuredPullSupplier_i::disconnect_structured_pull_supplier()
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "StructuredPullSupplier", "StructuredPullSupplier_i::disconnect_structured_pull_supplier");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean StructuredPullSupplier_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "StructuredPullSupplier", "StructuredPullSupplier_i::wait_done");
    while (!_done) {
      _finish.wait();
    }
    // using tmp_worker ensures only one wait_done call tries join()
    tmp_worker = _worker;
    _worker = 0;
  } // end lock scope
  if (tmp_worker) {
    tmp_worker->join(0);
  }
  return _com_err;
}

void StructuredPullSupplier_i::subscription_change(const CosN::EventTypeSeq& added,
						   const CosN::EventTypeSeq& deled)
{
  if (_done) return; // see "Optimization Notes" at top
  TW_SCOPE_LOCK(lock, _oplock, "StructuredPullSupplier", "StructuredPullSupplier_i::subscription_change");
  if (_done) return;
  _num_changes++;
  if (_change_fn) (*_change_fn)(added, deled, _obj_name, _num_changes, _verbose);
  else if (_verbose) cout << _obj_name << ": subscription_change received [# " << _num_changes << "]" << endl;
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying subscription_change response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}

// ------------ CosNotifyComm::SequencePullSupplier ------------ //

SequencePullSupplier_i::
SequencePullSupplier_i(CosNA::SequenceProxyPullConsumer_ptr proxy,
		       CosNA::SupplierAdmin_ptr admin,
		       CosNF::Filter_ptr filter,
		       CORBA::ULong max_events,
		       CORBA::ULong batch_size,
		       const char* objnm,
		       supply_batch_fn* supply_fn,
		       type_change_fn* change_fn,
		       CORBA::ULong millisecs,
		       CORBA::ULong delay_millisecs,
		       CORBA::Boolean verbose) :
  _my_proxy(proxy), _my_admin(admin), _my_filters(0),
  _obj_name(objnm), _supply_fn(supply_fn), _change_fn(change_fn),
  _num_changes(0), _num_events(0), _batch_size(batch_size), _num_batches(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose), _done(0), _com_err(0),
  _oplock(), _finish(&_oplock), _worker(0)
{
  // providing explict NULL for supply_fn is not OK -- must have a valid function
  if (!_supply_fn) _supply_fn = sample_supply_batch_fn;
  if (! CORBA::is_nil(filter)) {
    _my_filters.length(1);
    _my_filters[0] = filter;
  }
}

SequencePullSupplier_i*
SequencePullSupplier_i::create(CORBA::ORB_ptr orb,
			       CosNA::EventChannel_ptr channel,
			       CORBA::ULong max_events,
			       CORBA::ULong batch_size,
			       const char* objnm,
			       const char* proxy_ior_file,
			       const char* admin_ior_file,
			       supply_batch_fn* supply_fn,
			       type_change_fn* change_fn,
			       CosN::EventTypeSeq* evs_ptr,
			       const char* constraint_expr,
			       CORBA::ULong millisecs,
			       CORBA::ULong delay_millisecs,
			       CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosNA::SupplierAdmin_ptr admin = CosNA::SupplierAdmin::_nil();
  CosNA::ProxyConsumer_var generic_proxy =
    get_proxy_consumer(orb, channel, CosNA::SEQUENCE_EVENT, 0, admin_ior_file, admin, verbose); // 1 means push 0 means pull
  CosNA::SequenceProxyPullConsumer_ptr proxy = CosNA::SequenceProxyPullConsumer::_narrow(generic_proxy);
  if ( CORBA::is_nil(proxy) ) {
    return 0; // get_proxy_consumer failed
  }
  // If evs or constraint_expr are non-empty, add a filter to proxy
  CosNF::Filter_ptr filter = CosNF::Filter::_nil();
  if (evs_ptr) {
    CORBA::Boolean filt_err = sample_add_filter(channel, proxy, *evs_ptr, constraint_expr, objnm, filter, verbose);
    if (filt_err) {
      try {
	admin->destroy();
      } catch (...) { }
      return 0; // adding filter failed
    }
  }
  // write proxy IOR to file
  write_ior_to_file(orb, proxy, proxy_ior_file, verbose);
  // Construct a client
  SequencePullSupplier_i* client =
    new SequencePullSupplier_i(proxy, admin, filter, max_events, batch_size, objnm, supply_fn, change_fn, millisecs, delay_millisecs, verbose);
  return client;
}

CORBA::Boolean SequencePullSupplier_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "SequencePullSupplier", "SequencePullSupplier_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_sequence_pull_supplier(_this());
    if (_change_fn) {
      _my_proxy->obtain_subscription_types(CosNA::NONE_NOW_UPDATES_ON);
    } else {
      _my_proxy->obtain_subscription_types(CosNA::NONE_NOW_UPDATES_OFF);
    }
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEvCA::AlreadyConnected& ex) {
    cerr << _obj_name << ": Already connected" << endl;
    return 1; // error
  } catch (...) {
    cerr << _obj_name << ": Failed to connect" << endl;
    return 1; // error
  }
  if (_verbose) cout << _obj_name << ": Connected to proxy, ready to supply events" << endl; 
  // register the types to be supplied
  offer_ticker(_my_proxy, _obj_name, _verbose);
  // if _millisecs is set, spawn a thread to ping the proxy
  if (_millisecs)
    _worker = new GenericBoundWorkerThread(this);
  return 0; // OK
}

void SequencePullSupplier_i::cleanup() {
  CosNA::SequenceProxyPullConsumer_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "SequencePullSupplier", "SequencePullSupplier_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    if (CORBA::is_nil(_my_proxy)) {
      cerr << "Coding error: cleanup already called?" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosNA::SequenceProxyPullConsumer::_nil();
  } // end lock scope
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_sequence_pull_consumer();
  } catch(...) {}
  try {
    _my_admin->destroy();
  } catch (...) { }
  _my_admin = CosNA::SupplierAdmin::_nil();
  destroy_filters(_my_filters);
}

// While _millisecs is positive, pings proxy every _millisecs milliseconds.
// Sets error code and sets _done to 1 if there is an error during ping.
void* SequencePullSupplier_i::start_working(void *) {
  CosNC::SequencePullSupplier_var bump_my_refcount_during_outcalls = _this();

  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "SequencePullSupplier", "SequencePullSupplier_i::start_working");

    if (_verbose && (!_done) && _millisecs) cout << _obj_name << ": Spawned thread entering ping loop" << endl;
    while ( 1 ) {
      if (_done || (_millisecs == 0)) break;
      unsigned long t_secs = 0, t_nanosecs = 0;
      unsigned long d_secs = 0, d_nanosecs = 0;
      d_secs = _millisecs / 1000;
      d_nanosecs = (_millisecs % 1000) * 1000000;
      { // introduce temporary lock release scope; do not hold oplock across ping
	TW_SCOPE_RELEASE(lock);
	try {
	  _my_proxy->MyType();
	} catch (...) {
	  goto com_fail;
	}
      } // end temporary lock release scope
      if (_done) break; // must have disconnected during pull
      // sleep for specified interval
      TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
      _finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      continue; // continue ping loop
    com_fail:
      if (_done) break;
      if (_verbose) cout << _obj_name << ": communication error while pinging proxy using MyType()" << endl;
      _done = 1;
      _finish.broadcast();
      _com_err = 1;
      break; // break from while loop -- done
    } // end while loop
    // done
  } // end lock scope
  return 0;
}

CosN::EventBatch* SequencePullSupplier_i::try_pull_structured_events(CORBA::Long max_number,
								    CORBA::Boolean& has_event) {
  CosN::EventBatch* data = new CosN::EventBatch;
  data->length(0);
  has_event = 0;

  if (_done) return data; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "SequencePullSupplier", "SequencePullSupplier_i::try_pull_structured_events");
  if (_done) return data;
  if (_verbose) cout << _obj_name << ": Channel called try_pull_structured_events" << endl;

  // If we reached done point during last try_pull, it is broadcast now
  // and we return from this try_pull call without an event batch.
  // Waiting for one extra try_pull ensures the channel has successfully
  // processed the previous try_pull (the final supplied event batch) before
  // we disconnect this supplier.
  if (_max_events && (_num_events >= _max_events)) {
    if (_verbose) cout << _obj_name << ": DONE [max_events reached; we waited for 1 extra try_pull]" << endl;
    _done = 1;
    _finish.broadcast();
    if (_verbose) cout << _obj_name << ": NOT returning a batch" << endl;
    return data;
  }

  if ( ((*_supply_fn)(*data, _obj_name, _batch_size, _num_events+1, _num_batches+1, _verbose)) && (data->length())) {
    has_event = 1;
    _num_batches++;
    _num_events += data->length();
  } else {
    if (_verbose) cout << _obj_name << ": NOT returning a batch" << endl;
  }
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying try_pull_structured_events response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
  return data;
}

CosN::EventBatch* SequencePullSupplier_i::pull_structured_events(CORBA::Long max_number) {
  if (_done) // see "Optimization Notes" at top
    throw CORBA::INV_OBJREF(0, CORBA::COMPLETED_NO);

  CosN::EventBatch* data = new CosN::EventBatch;
  data->length(0);
  unsigned long t_secs = 0, t_nanosecs = 0;
  unsigned long d_secs = 0, d_nanosecs = 0;
  if (_millisecs) {
    d_secs = _millisecs / 1000;
    d_nanosecs = (_millisecs % 1000) * 1000000;
  }
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "SequencePullSupplier", "SequencePullSupplier_i::pull_structured_events");

    if (_verbose && (!_done)) cout << _obj_name << ": Channel called pull_structured_events" << endl;
    // invariants: _oplock is held entering top of loop and 
    // also whenever we break out of loop
    // (it is *not* held during yield() and timedwait() calls)
    while ( 1 ) {
      if (_done) {
	delete data;
	throw CORBA::INV_OBJREF(0, CORBA::COMPLETED_NO);
      }
      data->length(0);
      if ( ((*_supply_fn)(*data, _obj_name, _batch_size, _num_events+1, _num_batches+1, _verbose)) && (data->length())) {
	_num_batches++;
	_num_events += data->length();
	if (_max_events && (_num_events >= _max_events)) {
	  if (_verbose) cout << _obj_name << ": DONE [max_events reached]" << endl;
	  _done = 1;
	  _finish.broadcast();
	}
	break; // return data
      }
      // wait or yield then try again
      if (_millisecs) { // sleep for specified interval
	TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
	_finish.timedwait(t_secs, t_nanosecs); // this release _oplock during wait
      } else { // use yield to let other threads have a shot at _oplock
	{ // introduce temporary lock release scope
	  TW_SCOPE_RELEASE(lock);
	  TW_YIELD();
	} // end  temporary lock release scope
      }
      continue;
    } // end while loop
    if (!_done && _delay_millisecs) {
      unsigned long d_secs     =  _delay_millisecs / 1000;
      unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
      if (_verbose) cout << _obj_name << ": delaying pull_structured_events response for " << _delay_millisecs << " millisecs" << endl;
      unsigned long t_secs, t_nanosecs;
      TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
      _finish.timedwait(t_secs, t_nanosecs);
    }
  } // end lock scope
  return data;
}

void SequencePullSupplier_i::disconnect_sequence_pull_supplier()
{
  if (_done) return; // see "Optimization Notes" at top

  TW_SCOPE_LOCK(lock, _oplock, "SequencePullSupplier", "SequencePullSupplier_i::disconnect_sequence_pull_supplier");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean SequencePullSupplier_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "SequencePullSupplier", "SequencePullSupplier_i::wait_done");
    while (!_done) {
      _finish.wait();
    }
    // using tmp_worker ensures only one wait_done call tries join()
    tmp_worker = _worker;
    _worker = 0;
  } // end lock scope
  if (tmp_worker) {
    tmp_worker->join(0);
  }
  return _com_err;
}

void SequencePullSupplier_i::subscription_change(const CosN::EventTypeSeq& added,
 						 const CosN::EventTypeSeq& deled)
{
  if (_done) return; // see "Optimization Notes" at top
  TW_SCOPE_LOCK(lock, _oplock, "SequencePullSupplier", "SequencePullSupplier_i::subscription_change");
  if (_done) return;
  _num_changes++;
  if (_change_fn) (*_change_fn)(added, deled, _obj_name, _num_changes, _verbose);
  else if (_verbose) cout << _obj_name << ": subscription_change received [# " << _num_changes << "]" << endl;
  if (!_done && _delay_millisecs) {
    unsigned long d_secs     =  _delay_millisecs / 1000;
    unsigned long d_nanosecs = (_delay_millisecs % 1000) * 1000000;
    if (_verbose) cout << _obj_name << ": delaying subscription_change response for " << _delay_millisecs << " millisecs" << endl;
    unsigned long t_secs, t_nanosecs;
    TW_GET_TIME(&t_secs, &t_nanosecs, d_secs, d_nanosecs);
    _finish.timedwait(t_secs, t_nanosecs);
  }
}
