// -*- Mode: C++; -*-

// -------------------------------------------------------------- //
// See legacy_clients.h
// -------------------------------------------------------------- //

#include <iostream>
#include <iomanip>
using namespace std;
#include "thread_wrappers.h"

#include "CosNotifyShorthands.h"
#include "legacy_clients.h"

// ----------------- CosEvC::PushConsumer ---------------- //

LegacyPushConsumer_i::
LegacyPushConsumer_i(CosEA::ProxyPushSupplier_ptr proxy, CORBA::ULong max_events,
		     const char* objnm, consume_any_fn* consume_fn,
		     CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _obj_name(objnm), _consume_fn(consume_fn),
  _num_events(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose), _done(0), _com_err(0),
  _oplock(), _finish(&_oplock), _worker(0)
{
  // nothing else to do
}

LegacyPushConsumer_i*
LegacyPushConsumer_i::create(CosEA::EventChannel_ptr channel,
			     CORBA::ULong max_events,
			     const char* objnm,
			     consume_any_fn* consume_fn,
			     CORBA::ULong millisecs,
			     CORBA::ULong delay_millisecs,
			     CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosEA::ProxyPushSupplier_ptr proxy = CosEA::ProxyPushSupplier::_nil();
  CosEA::ConsumerAdmin_var admin;
  try {
    admin = channel->for_consumers();
    if ( CORBA::is_nil(admin) ) {
      cerr << "Failed to obtain admin" << endl;
      return 0; // failure
    }
  } catch (...) {
    cerr << "Failed to obtain admin" << endl;
    return 0;  // failure
  }
  if (verbose) cout << "Obtained admin from the channel" << endl;
  try {
    proxy = admin->obtain_push_supplier();
  } catch (...) {  }
  if ( CORBA::is_nil(proxy) ) {
    cerr << "Failed to obtain proxy" << endl;
    return 0;
  }
  if (verbose) cout << "Obtained proxy from admin" << endl;

    // Construct a client
  LegacyPushConsumer_i* client =
    new LegacyPushConsumer_i(proxy, max_events, objnm, consume_fn, millisecs, delay_millisecs, verbose);
  return client;
}

CORBA::Boolean LegacyPushConsumer_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "LegacyPushConsumer", "LegacyPushConsumer_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_push_consumer(_this());
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEA::AlreadyConnected& ex) {
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

void LegacyPushConsumer_i::cleanup() {
  CosEA::ProxyPushSupplier_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "LegacyPushConsumer", "LegacyPushConsumer_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosEA::ProxyPushSupplier::_nil();
  } // end lock scope
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_push_supplier();
  } catch(...) {}
}

// While _millisecs is positive, pings proxy every _millisecs milliseconds.
// Sets error code and sets _done to 1 if there is an error during ping.
void* LegacyPushConsumer_i::start_working(void *) {
  CosEvC::PushConsumer_var bump_my_refcount_during_outcalls = _this();

  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "LegacyPushConsumer", "LegacyPushConsumer_i::start_working");

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
	  _my_proxy->connect_push_consumer(_this());
	} catch (CosEA::AlreadyConnected& ex) {
	  // this is the expected result -- continue pinging
	} catch (...) { // assume everything else is a communication error
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
      if (_verbose) cout << _obj_name << ": communication error while pinging proxy using connect_push_consumer" << endl;
      _done = 1;
      _finish.broadcast();
      _com_err = 1;
      break; // break from while loop -- done
    } // end while loop
    // done
  } // end lock scope
  return 0;
}

void LegacyPushConsumer_i::push(const CORBA::Any& data)
{
  if (_done) return; // see "Optimization Notes" at top of sample_clients.cc

  TW_SCOPE_LOCK(lock, _oplock, "LegacyPushConsumer", "LegacyPushConsumer_i::push");
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

void LegacyPushConsumer_i::disconnect_push_consumer()
{
  if (_done) return; // see "Optimization Notes" at top of sample_clients.cc

  TW_SCOPE_LOCK(lock, _oplock, "LegacyPushConsumer", "LegacyPushConsumer_i::diconnect_push_consumer");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean LegacyPushConsumer_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "LegacyPushConsumer", "LegacyPushConsumer_i::wait_done");
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

// ----------------- CosEvC::PullConsumer ---------------- //

LegacyPullConsumer_i::
LegacyPullConsumer_i(CosEA::ProxyPullSupplier_ptr proxy, CORBA::ULong max_events, 
		     const char* objnm, consume_any_fn* consume_fn,
		     CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _obj_name(objnm), _consume_fn(consume_fn),
  _num_events(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose), _done(0), _com_err(0),
  _oplock(), _finish(&_oplock), _worker(0)
{
  // nothing else to do
}

LegacyPullConsumer_i*
LegacyPullConsumer_i::create(CosEA::EventChannel_ptr channel,
			     CORBA::ULong max_events,
			     const char* objnm,
			     consume_any_fn* consume_fn,
			     CORBA::ULong millisecs,
			     CORBA::ULong delay_millisecs,
			     CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosEA::ProxyPullSupplier_ptr proxy = CosEA::ProxyPullSupplier::_nil();
  CosEA::ConsumerAdmin_var admin;
  try {
    admin = channel->for_consumers();
    if ( CORBA::is_nil(admin) ) {
      cerr << "Failed to obtain admin" << endl;
      return 0; // failure
    }
  } catch (...) {
    cerr << "Failed to obtain admin" << endl;
    return 0;  // failure
  }
  if (verbose) cout << "Obtained admin from the channel" << endl;
  try {
    proxy = admin->obtain_pull_supplier();
  } catch (...) {  }
  if ( CORBA::is_nil(proxy) ) {
    cerr << "Failed to obtain proxy" << endl;
    return 0;
  }
  if (verbose) cout << "Obtained proxy from admin" << endl;

    // Construct a client
  LegacyPullConsumer_i* client =
    new LegacyPullConsumer_i(proxy, max_events, objnm, consume_fn, millisecs, delay_millisecs, verbose);
  return client;
}

CORBA::Boolean LegacyPullConsumer_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "LegacyPullConsumer", "LegacyPullConsumer_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_pull_consumer(_this());
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEA::AlreadyConnected& ex) {
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

void LegacyPullConsumer_i::cleanup() {
  CosEA::ProxyPullSupplier_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "LegacyPullConsumer", "LegacyPullConsumer_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosEA::ProxyPullSupplier::_nil();
  } // end lock scope
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_pull_supplier();
  } catch(...) {}
}

// Note: lock is *not* held during pull(), yield(), and timedwait() calls
void* LegacyPullConsumer_i::start_working(void *) {
  CosEvC::PullConsumer_var bump_my_refcount_during_outcalls = _this();

  unsigned long t_secs = 0, t_nanosecs = 0;
  unsigned long d_secs = 0, d_nanosecs = 0;
  if (_millisecs) {
    d_secs = _millisecs / 1000;
    d_nanosecs = (_millisecs % 1000) * 1000000;
  }
  CORBA::Any* data = 0;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "LegacyPullConsumer", "LegacyPullConsumer_i::start_working");

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


void LegacyPullConsumer_i::disconnect_pull_consumer()
{
  if (_done) return; // see "Optimization Notes" at top of sample_clients.cc

  TW_SCOPE_LOCK(lock, _oplock, "LegacyPullConsumer", "LegacyPullConsumer_i::disconnect_pull_consumer");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean LegacyPullConsumer_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "LegacyPullConsumer", "LegacyPullConsumer_i::wait_done");
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

// ------------------ CosEvC::PushSupplier ---------------- //

LegacyPushSupplier_i::
LegacyPushSupplier_i(CosEA::ProxyPushConsumer_ptr proxy, CORBA::ULong max_events,
		     const char* objnm, supply_any_fn* supply_fn,
		     CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _obj_name(objnm), _supply_fn(supply_fn),
  _num_events(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose), _done(0), _com_err(0),
  _oplock(), _finish(&_oplock), _worker(0)
{
  // providing explict NULL for supply_fn is not OK -- must have a valid function
  if (!_supply_fn) _supply_fn = sample_supply_any_fn;
}

LegacyPushSupplier_i*
LegacyPushSupplier_i::create(CosEA::EventChannel_ptr channel,
			     CORBA::ULong max_events,
			     const char* objnm,
			     supply_any_fn* supply_fn,
			     CORBA::ULong millisecs,
			     CORBA::ULong delay_millisecs,
			     CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosEA::ProxyPushConsumer_ptr proxy = CosEA::ProxyPushConsumer::_nil();
  CosEA::SupplierAdmin_var admin;
  try {
    admin = channel->for_suppliers();
    if ( CORBA::is_nil(admin) ) {
      cerr << "Failed to obtain admin" << endl;
      return 0; // failure
    }
  } catch (...) {
    cerr << "Failed to obtain admin" << endl;
    return 0;  // failure
  }
  if (verbose) cout << "Obtained admin from the channel" << endl;
  try {
    proxy = admin->obtain_push_consumer();
  } catch (...) {  }
  if ( CORBA::is_nil(proxy) ) {
    cerr << "Failed to obtain proxy" << endl;
    return 0; // failure
  }
  if (verbose) cout << "Obtained proxy from admin" << endl;

    // Construct a client
  LegacyPushSupplier_i* client =
    new LegacyPushSupplier_i(proxy, max_events, objnm, supply_fn, millisecs, delay_millisecs, verbose);
  return client;
}

CORBA::Boolean LegacyPushSupplier_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "LegacyPushSupplier", "LegacyPushSupplier_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_push_supplier(_this());
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEA::AlreadyConnected& ex) {
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

void LegacyPushSupplier_i::cleanup() {
  CosEA::ProxyPushConsumer_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "LegacyPushSupplier", "LegacyPushSupplier_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosEA::ProxyPushConsumer::_nil();
  } // end lock scope
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_push_consumer();
  } catch(...) {}
}

// Note: lock is *not* held during push(), yield() and timedwait() calls
void* LegacyPushSupplier_i::start_working(void *) {
  CosEvC::PushSupplier_var bump_my_refcount_during_outcalls = _this();

  unsigned long t_secs = 0, t_nanosecs = 0;
  unsigned long d_secs = 0, d_nanosecs = 0;
  if (_millisecs) {
    d_secs = _millisecs / 1000;
    d_nanosecs = (_millisecs % 1000) * 1000000;
  }
  CORBA::Any* data = new CORBA::Any;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "LegacyPushSupplier", "LegacyPushSupplier_i::start_working");

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
	// [rather than doing the following we let supply_fn report if it wants to]
	// if (_verbose) cout << _obj_name << ": Pushed an event, event count = " << _num_events << endl;
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

void LegacyPushSupplier_i::disconnect_push_supplier()
{
  if (_done) return; // see "Optimization Notes" at top of sample_clients.cc

  TW_SCOPE_LOCK(lock, _oplock, "LegacyPushSupplier", "LegacyPushSupplier_i::disconnect_push_supplier");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean LegacyPushSupplier_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "LegacyPushSupplier", "LegacyPushSupplier_i::wait_done");
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

// ------------------- CosEvC::PullSupplier ---------------- //

LegacyPullSupplier_i::
LegacyPullSupplier_i(CosEA::ProxyPullConsumer_ptr proxy, CORBA::ULong max_events,
		     const char* objnm, supply_any_fn* supply_fn,
		     CORBA::ULong millisecs, CORBA::ULong delay_millisecs, CORBA::Boolean verbose) :
  _my_proxy(proxy), _obj_name(objnm), _supply_fn(supply_fn),
  _num_events(0), _max_events(max_events),
  _millisecs(millisecs), _delay_millisecs(delay_millisecs), _verbose(verbose), _done(0), _com_err(0),
  _oplock(), _finish(&_oplock), _worker(0)
{
  // providing explict NULL for supply_fn is not OK -- must have a valid function
  if (!_supply_fn) _supply_fn = sample_supply_any_fn;
}

LegacyPullSupplier_i*
LegacyPullSupplier_i::create(CosEA::EventChannel_ptr channel,
			     CORBA::ULong max_events,
			     const char* objnm,
			     supply_any_fn* supply_fn,
			     CORBA::ULong millisecs,
			     CORBA::ULong delay_millisecs,
			     CORBA::Boolean verbose)
{
  // Obtain appropriate proxy object
  CosEA::ProxyPullConsumer_ptr proxy = CosEA::ProxyPullConsumer::_nil();
  CosEA::SupplierAdmin_var admin;
  try {
    admin = channel->for_suppliers();
    if ( CORBA::is_nil(admin) ) {
      cerr << "Failed to obtain admin" << endl;
      return 0; // failure
    }
  } catch (...) {
    cerr << "Failed to obtain admin" << endl;
    return 0;  // failure
  }
  if (verbose) cout << "Obtained admin from the channel" << endl;
  try {
    proxy = admin->obtain_pull_consumer();
  } catch (...) {  }
  if ( CORBA::is_nil(proxy) ) {
    cerr << "Failed to obtain proxy" << endl;
    return 0; // failure
  }
  if (verbose) cout << "Obtained proxy from admin" << endl;

    // Construct a client
  LegacyPullSupplier_i* client =
    new LegacyPullSupplier_i(proxy, max_events, objnm, supply_fn, millisecs, delay_millisecs, verbose);
  return client;
}

CORBA::Boolean LegacyPullSupplier_i::connect() {
  TW_SCOPE_LOCK(lock, _oplock, "LegacyPullSupplier", "LegacyPullSupplier_i::connect");
  if (_done) return 0;
  _com_err = 0;
  try {
    _my_proxy->connect_pull_supplier(_this());
  } catch (CORBA::BAD_PARAM& ex) {
    cerr << _obj_name << ": BAD_PARAM Exception while connecting" << endl;
    return 1; // error
  } catch (CosEA::AlreadyConnected& ex) {
    cerr << _obj_name << ": Already connected" << endl;
    return 1; // error
  } catch (...) {
    cerr << _obj_name << ": Failed to connect" << endl;
    return 1; // error
  }
  if (_verbose) cout << _obj_name << ": Connected to proxy, ready to supply events" << endl; 
  // if _millisecs is set, spawn a thread to ping the proxy
  if (_millisecs)
    _worker = new GenericBoundWorkerThread(this);
  return 0; // OK
}

void LegacyPullSupplier_i::cleanup() {
  CosEA::ProxyPullConsumer_var proxy;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "LegacyPullSupplier", "LegacyPullSupplier_i::cleanup");
    if (_worker || (!_done)) {
      cerr << "Coding error: only call c->cleanup() after c->wait_done()" << endl;
      return;
    }
    // this method takes sole ownership of _my_proxy ref
    proxy = _my_proxy;
    _my_proxy = CosEA::ProxyPullConsumer::_nil();
  } // end lock scope
  // do not hold oplock while invoking disconnect
  try {
    if ((!_com_err) && (!CORBA::is_nil(proxy)))
      proxy->disconnect_pull_consumer();
  } catch(...) {}
}

// While _millisecs is positive, pings proxy every _millisecs milliseconds.
// Sets error code and sets _done to 1 if there is an error during ping.
void* LegacyPullSupplier_i::start_working(void *) {
  CosEvC::PullSupplier_var bump_my_refcount_during_outcalls = _this();

  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "LegacyPullSupplier", "LegacyPullSupplier_i::start_working");
    if (_verbose && (!_done) && _millisecs) cout << _obj_name << ": Spawned thread entering ping loop" << endl;
    // invariant: _oplock held at top of loop
    while ( 1 ) {
      if (_done || (_millisecs == 0)) break;
      unsigned long t_secs = 0, t_nanosecs = 0;
      unsigned long d_secs = 0, d_nanosecs = 0;
      d_secs = _millisecs / 1000;
      d_nanosecs = (_millisecs % 1000) * 1000000;
      { // introduce temporary lock release scope; do not hold oplock across ping
	TW_SCOPE_RELEASE(lock);
	try {
	  _my_proxy->connect_pull_supplier(_this());
	} catch (CosEA::AlreadyConnected& ex) {
	  // this is the expected result -- continue pinging
	} catch (...) { // assume everything else is a communication error
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
      if (_verbose) cout << _obj_name << ": communication error while pinging proxy using connect_pull_supplier" << endl;
      _done = 1;
      _finish.broadcast();
      _com_err = 1;
      break; // break from while loop -- done
    } // end while loop
    // done
  } // end lock scope
  return 0;
}

CORBA::Any* LegacyPullSupplier_i::try_pull(CORBA::Boolean& has_event) {
  CORBA::Any* data = new CORBA::Any;
  has_event = 0;

  if (_done) return data; // see "Optimization Notes" at top of sample_clients.cc

  TW_SCOPE_LOCK(lock, _oplock, "LegacyPullSupplier", "LegacyPullSupplier_i::try_pull");
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
    // [rather than doing the following we let supply_fn report if it wants to]
    // if (_verbose) cout << _obj_name << ": Returning an event, event count = " << _num_events << endl;
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

// Note: lock is *not* held during yield() and timedwait() calls
CORBA::Any* LegacyPullSupplier_i::pull() {
  CORBA::Any* data = new CORBA::Any;

  if (_done) return data; // see "Optimization Notes" at top of sample_clients.cc

  unsigned long t_secs = 0, t_nanosecs = 0;
  unsigned long d_secs = 0, d_nanosecs = 0;
  if (_millisecs) {
    d_secs = _millisecs / 1000;
    d_nanosecs = (_millisecs % 1000) * 1000000;
  }
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "LegacyPullSupplier", "LegacyPullSupplier_i::pull");

    if (_verbose && (!_done)) cout << _obj_name << ": Channel called pull" << endl;
    // invariants: _oplock is held entering top of loop and 
    // also whenever we break out of loop
    while ( 1 ) {
      if (_done) break; // must have disconnected
      if ((*_supply_fn)(*data, _obj_name, _num_events+1, _verbose)) {
	_num_events++;
	// [rather than doing the following we let supply_fn report if it wants to]
	// if (_verbose) cout << _obj_name << ": Returning an event, event count = " << _num_events << endl;
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
    }
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

void LegacyPullSupplier_i::disconnect_pull_supplier()
{
  if (_done) return; // see "Optimization Notes" at top of sample_clients.cc

  TW_SCOPE_LOCK(lock, _oplock, "LegacyPullSupplier", "LegacyPullSupplier_i::discconnect_pull_supplier");
  if (_done) return;
  if (_verbose) cout << _obj_name << ": disconnected" << endl;
  _done = 1;
  _finish.broadcast();
}

CORBA::Boolean LegacyPullSupplier_i::wait_done() {
  GenericBoundWorkerThread* tmp_worker;
  { // introduce lock scope
    TW_SCOPE_LOCK(lock, _oplock, "LegacyPullSupplier", "LegacyPullSupplier_i::wait_done");
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

