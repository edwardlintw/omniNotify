// Make sure we are using POA
#ifdef COS_USES_BOA
#error "COS_USES_BOA should not be set for this example"
#endif

#include "orb_init_name.h"
#include "parse_cmd_line.h"
#include "get_channel.h"

#include "sample_clients.h"
#define SUPPORTS_FILTERS 1

int main(int argc, char** argv) {
  char pname[256];
  strcpy(pname, argv[0]);
  CosN::EventTypeSeq evs;
  evs.length(0);
  char* constraint_expr = 0;
  char* channel_name = 0;
  char* ior_file = 0; 
  char* cfact_name = 0;
  char* cfact_ior_file = 0; 
  char* proxy_ior_file = 0; 
  char* admin_ior_file = 0;
  CORBA::ULong num_events = 0, num_con_events = 0, batch_size = 0, millisecs = 0, delay_millisecs = 0;
  CORBA::Boolean verbose = 0;
  
  DO_ORB_INIT(orb);
  try {
    CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
    PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);

    // (1) parse the command line arguments
    if (parse_cmd_line(argc, argv, pname, SUPPORTS_FILTERS,
		       num_events, batch_size, millisecs, delay_millisecs, verbose,
		       channel_name, ior_file, cfact_name, cfact_ior_file,
		       proxy_ior_file, admin_ior_file, evs, constraint_expr)) {
      goto error_return; // command line parsing failed
    }
    // since there are 6 suppliers, must consume 6 x num_events
    num_con_events = 6 * num_events;

    // (2) Obtain reference to notification channel
    if (verbose) cout << "Obtaining reference to notification channel" << endl;
    CosNA::EventChannel_ptr channel = getchan_from_ns(orb, channel_name, verbose);
    if (CORBA::is_nil(channel)) { // name service lookup failed
      channel = getchan_from_iorfile(orb, ior_file, verbose);
      if (CORBA::is_nil(channel)) {
	goto error_return; // both methods failed
      }
    }

    // (3) Create all 12 CosNotify-style clients
    if (verbose) cout << "Creating 12 clients" << endl;

    // (3.1) any_push_consumer
    PushConsumer_i* a_push_c =
      PushConsumer_i::create(orb, channel, num_con_events, batch_size, "any_push_consumer",
			     "", "", sample_consume_any_fn, sample_offer_change_fn,
			     &evs, constraint_expr, millisecs, delay_millisecs, verbose);
    if (! a_push_c) {
      cerr << "Constructing any_push_consumer failed" << endl;
      goto error_return; // failed to create a client
    }
    CosNotifyComm::PushConsumer_var my_a_push_c = a_push_c->_this();
    a_push_c->_remove_ref(); // enables POA cleanup of servant

    // (3.2) struct_push_consumer
    StructuredPushConsumer_i* s_push_c =
      StructuredPushConsumer_i::create(orb, channel, num_con_events, batch_size, "struct_push_consumer",
				       "", "", sample_consume_structured_fn, sample_offer_change_fn,
				       &evs, constraint_expr, millisecs, delay_millisecs, verbose);
    if (! s_push_c) {
      cerr << "Constructing struct_push_consumer failed" << endl;
      goto error_return; // failed to create a client
    }
    CosNotifyComm::StructuredPushConsumer_var my_s_push_c = s_push_c->_this();
    s_push_c->_remove_ref(); // enables POA cleanup of servant

    // (3.3) batch_push_consumer
    SequencePushConsumer_i* b_push_c =
      SequencePushConsumer_i::create(orb, channel, num_con_events, batch_size, "batch_push_consumer",
				     "", "", sample_consume_batch_fn, sample_offer_change_fn,
				     &evs, constraint_expr, millisecs, delay_millisecs, verbose);
    if (! b_push_c) {
      cerr << "Constructing batch_push_consumer failed" << endl;
      goto error_return; // failed to create a client
    }
    CosNotifyComm::SequencePushConsumer_var my_b_push_c = b_push_c->_this();
    b_push_c->_remove_ref(); // enables POA cleanup of servant

    // (3.4) any_pull_consumer
    PullConsumer_i* a_pull_c =
      PullConsumer_i::create(orb, channel, num_con_events, batch_size, "any_pull_consumer",
			     "", "", sample_consume_any_fn, sample_offer_change_fn,
			     &evs, constraint_expr, millisecs, delay_millisecs, verbose);
    if (! a_pull_c) {
      cerr << "Constructing any_pull_consumer failed" << endl;
      goto error_return; // failed to create a client
    }
    CosNotifyComm::PullConsumer_var my_a_pull_c = a_pull_c->_this();
    a_pull_c->_remove_ref(); // enables POA cleanup of servant

    // (3.5) struct_pull_consumer
    StructuredPullConsumer_i* s_pull_c =
      StructuredPullConsumer_i::create(orb, channel, num_con_events, batch_size, "struct_pull_consumer",
				       "", "", sample_consume_structured_fn, sample_offer_change_fn,
				       &evs, constraint_expr, millisecs, delay_millisecs, verbose);
    if (! s_pull_c) {
      cerr << "Constructing struct_pull_consumer failed" << endl;
      goto error_return; // failed to create a client
    }
    CosNotifyComm::StructuredPullConsumer_var my_s_pull_c = s_pull_c->_this();
    s_pull_c->_remove_ref(); // enables POA cleanup of servant

    // (3.6) batch_pull_consumer
    SequencePullConsumer_i* b_pull_c =
      SequencePullConsumer_i::create(orb, channel, num_con_events, batch_size, "batch_pull_consumer",
				     "", "", sample_consume_batch_fn, sample_offer_change_fn,
				     &evs, constraint_expr, millisecs, delay_millisecs, verbose);
    if (! b_pull_c) {
      cerr << "Constructing batch_pull_consumer failed" << endl;
      goto error_return; // failed to create a client
    }
    CosNotifyComm::SequencePullConsumer_var my_b_pull_c = b_pull_c->_this();
    b_pull_c->_remove_ref(); // enables POA cleanup of servant

    // (3.7) any_push_supplier
    PushSupplier_i* a_push_s =
      PushSupplier_i::create(orb, channel, num_events, batch_size, "any_push_supplier",
			     "", "", sample_supply_any_fn, sample_subscription_change_fn,
			     &evs, constraint_expr, millisecs, delay_millisecs, verbose);
    if (! a_push_s) {
      cerr << "Constructing any_push_supplier failed" << endl;
      goto error_return; // failed to create a client
    }
    CosNotifyComm::PushSupplier_var my_a_push_s = a_push_s->_this();
    a_push_s->_remove_ref(); // enables POA cleanup of servant

    // (3.8) struct_push_supplier
    StructuredPushSupplier_i* s_push_s =
      StructuredPushSupplier_i::create(orb, channel, num_events, batch_size, "struct_push_supplier",
				       "", "", sample_supply_structured_fn, sample_subscription_change_fn,
				       &evs, constraint_expr, millisecs, delay_millisecs, verbose);
    if (! s_push_s) {
      cerr << "Constructing struct_push_supplier failed" << endl;
      goto error_return; // failed to create a client
    }
    CosNotifyComm::StructuredPushSupplier_var my_s_push_s = s_push_s->_this();
    s_push_s->_remove_ref(); // enables POA cleanup of servant

    // (3.9) batch_push_supplier
    SequencePushSupplier_i* b_push_s =
      SequencePushSupplier_i::create(orb, channel, num_events, batch_size, "batch_push_supplier",
				     "", "", sample_supply_batch_fn, sample_subscription_change_fn,
				     &evs, constraint_expr, millisecs, delay_millisecs, verbose);
    if (! b_push_s) {
      cerr << "Constructing batch_push_supplier failed" << endl;
      goto error_return; // failed to create a client
    }
    CosNotifyComm::SequencePushSupplier_var my_b_push_s = b_push_s->_this();
    b_push_s->_remove_ref(); // enables POA cleanup of servant

    // (3.10) any_pull_supplier
    PullSupplier_i* a_pull_s =
      PullSupplier_i::create(orb, channel, num_events, batch_size, "any_pull_supplier",
			     "", "", sample_supply_any_fn, sample_subscription_change_fn,
			     &evs, constraint_expr, millisecs, delay_millisecs, verbose);
    if (! a_pull_s) {
      cerr << "Constructing any_pull_supplier failed" << endl;
      goto error_return; // failed to create a client
    }
    CosNotifyComm::PullSupplier_var my_a_pull_s = a_pull_s->_this();
    a_pull_s->_remove_ref(); // enables POA cleanup of servant

    // (3.11) struct_pull_supplier
    StructuredPullSupplier_i* s_pull_s =
      StructuredPullSupplier_i::create(orb, channel, num_events, batch_size, "struct_pull_supplier",
				       "", "", sample_supply_structured_fn, sample_subscription_change_fn,
				       &evs, constraint_expr, millisecs, delay_millisecs, verbose);
    if (! s_pull_s) {
      cerr << "Constructing struct_pull_supplier failed" << endl;
      goto error_return; // failed to create a client
    }
    CosNotifyComm::StructuredPullSupplier_var my_s_pull_s = s_pull_s->_this();
    s_pull_s->_remove_ref(); // enables POA cleanup of servant

    // (3.12) batch_pull_supplier
    SequencePullSupplier_i* b_pull_s =
      SequencePullSupplier_i::create(orb, channel, num_events, batch_size, "batch_pull_supplier",
				     "", "", sample_supply_batch_fn, sample_subscription_change_fn,
				     &evs, constraint_expr, millisecs, delay_millisecs, verbose);
    if (! b_pull_s) {
      cerr << "Constructing batch_pull_supplier failed" << endl;
      goto error_return; // failed to create a client
    }
    CosNotifyComm::SequencePullSupplier_var my_b_pull_s = b_pull_s->_this();
    b_pull_s->_remove_ref(); // enables POA cleanup of servant

    // (4) Tell POA we are ready to go
    if (verbose) cout << "Activating POA manager" << endl;
    PortableServer::POAManager_var pman = poa->the_POAManager();
    pman->activate();

    // (5) Connect clients to their proxies -- consumers first
    if (verbose) cout << "Connecting consumer clients to proxies" << endl;

    if (a_push_c->connect()) {
      cerr << "Connecting any_push_consumer failed" << endl;
      goto error_return;
    }
    if (s_push_c->connect()) {
      cerr << "Connecting struct_push_consumer failed" << endl;
      goto error_return;
    }
    if (b_push_c->connect()) {
      cerr << "Connecting batch_push_consumer failed" << endl;
      goto error_return;
    }

    if (a_pull_c->connect()) {
      cerr << "Connecting any_pull_consumer failed" << endl;
      goto error_return;
    }
    if (s_pull_c->connect()) {
      cerr << "Connecting struct_pull_consumer failed" << endl;
      goto error_return;
    }
    if (b_pull_c->connect()) {
      cerr << "Connecting batch_pull_consumer failed" << endl;
      goto error_return;
    }

    if (verbose) cout << "Connecting supplier clients to proxies" << endl;

    if (a_push_s->connect()) {
      cerr << "Connecting any_push_supplier failed" << endl;
      goto error_return;
    }
    if (s_push_s->connect()) {
      cerr << "Connecting struct_push_supplier failed" << endl;
      goto error_return;
    }
    if (b_push_s->connect()) {
      cerr << "Connecting batch_push_supplier failed" << endl;
      goto error_return;
    }

    if (a_pull_s->connect()) {
      cerr << "Connecting any_pull_supplier failed" << endl;
      goto error_return;
    }
    if (s_pull_s->connect()) {
      cerr << "Connecting struct_pull_supplier failed" << endl;
      goto error_return;
    }
    if (b_pull_s->connect()) {
      cerr << "Connecting batch_pull_supplier failed" << endl;
      goto error_return;
    }

    if (verbose) cout << "Waiting for all 12 clients to finish" << endl;
    CORBA::Boolean a_push_c_err = a_push_c->wait_done();
    CORBA::Boolean s_push_c_err = s_push_c->wait_done();
    CORBA::Boolean b_push_c_err = b_push_c->wait_done();

    CORBA::Boolean a_pull_c_err = a_pull_c->wait_done();
    CORBA::Boolean s_pull_c_err = s_pull_c->wait_done();
    CORBA::Boolean b_pull_c_err = b_pull_c->wait_done();

    CORBA::Boolean a_push_s_err = a_push_s->wait_done();
    CORBA::Boolean s_push_s_err = s_push_s->wait_done();
    CORBA::Boolean b_push_s_err = b_push_s->wait_done();

    CORBA::Boolean a_pull_s_err = a_pull_s->wait_done();
    CORBA::Boolean s_pull_s_err = s_pull_s->wait_done();
    CORBA::Boolean b_pull_s_err = b_pull_s->wait_done();

    CORBA::Boolean com_err =
      a_push_c_err ||
      s_push_c_err ||
      b_push_c_err ||
      a_pull_c_err ||
      s_pull_c_err ||
      b_pull_c_err ||
      a_push_s_err ||
      s_push_s_err ||
      b_push_s_err ||
      a_pull_s_err ||
      s_pull_s_err ||
      b_pull_s_err;

    if (com_err && (num_events > 0)) {
      if (verbose) cout << "Communication error -- probably did not process desired # of events" << endl;
    }

    if (verbose) cout << "Cleaning up" << endl;

    if (!a_push_c_err) { a_push_c->cleanup(); }
    if (!s_push_c_err) { s_push_c->cleanup(); }
    if (!b_push_c_err) { b_push_c->cleanup(); }

    if (!a_pull_c_err) { a_pull_c->cleanup(); }
    if (!s_pull_c_err) { s_pull_c->cleanup(); }
    if (!b_pull_c_err) { b_pull_c->cleanup(); }

    if (!a_push_s_err) { a_push_s->cleanup(); }
    if (!s_push_s_err) { s_push_s->cleanup(); }
    if (!b_push_s_err) { b_push_s->cleanup(); }

    if (!a_pull_s_err) { a_pull_s->cleanup(); }
    if (!s_pull_s_err) { s_pull_s->cleanup(); }
    if (!b_pull_s_err) { b_pull_s->cleanup(); }

    goto normal_return;
  }
  catch(CORBA::SystemException&) {                     
    cerr << "main caught CORBA::SystemException." << endl;  
  }                                                    
  catch(CORBA::Exception&) {                           
    cerr << "main caught CORBA::Exception." << endl;        
  }                                                    
#if defined(__OMNIORB3__) || defined(__OMNIORB4__)
  catch(omniORB::fatalException& fe) {                 
    cerr << "main caught omniORB::fatalException:" << endl; 
    cerr << "  file: " << fe.file() << endl;           
    cerr << "  line: " << fe.line() << endl;           
    cerr << "  mesg: " << fe.errmsg() << endl;         
  }                                                    
#endif
  catch(...) {                                         
    // nameclt comment says it is a bad idea to report an error here 
  }
  cerr << "main caught an exception." << endl;       
 error_return:
  cerr << "QUITTING due to error" << endl;
  // orb->destroy();
  return -1;
 normal_return:
  // orb->destroy();
  return 0;
}

