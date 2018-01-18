#if !defined(CLIENT_IMPL_CLASS) || !defined(CLIENT_CLASS_VAR) || !defined(CLIENT_NAME) || !defined(SUPPLY_OR_CONSUME_FN)
#error "define CLIENT_IMPL_CLASS, CLIENT_CLASS_VAR, CLIENT_NAME, SUPPLY_OR_CONSUME_FN before including main_program.h"
#endif

#if !defined(LEGACY_CLIENT) && !defined(CHANGE_FN)
#error "define CHANGE_FN before including main_program.h"
#endif

// Make sure we are using POA
#ifdef COS_USES_BOA
#error COS_USES_BOA should not be set for this example
#endif

#include "orb_init_name.h"
#include "parse_cmd_line.h"
#include "get_channel.h"

#ifndef LEGACY_CLIENT
#include "sample_clients.h"
#define SUPPORTS_FILTERS 1
#else
#include "legacy_clients.h"
#define SUPPORTS_FILTERS 0
#endif

// uncomment the following to create a new channel
// #define CREATE_CHANNEL
// uncomment the following to destroy the created channel
// #define DESTROY_CHANNEL

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
  CORBA::ULong num_events = 0, batch_size = 0, millisecs = 0, delay_millisecs = 0;
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

#ifndef CREATE_CHANNEL
    // (2) Obtain reference to notification channel
    if (verbose) cout << "Obtaining reference to notification channel" << endl;
    CosNA::EventChannel_ptr channel = getchan_from_ns(orb, channel_name, verbose);
    if (CORBA::is_nil(channel)) { // name service lookup failed
      channel = getchan_from_iorfile(orb, ior_file, verbose);
      if (CORBA::is_nil(channel)) {
	goto error_return; // both methods failed
      }
    }
#else
    // (2) Create a new notification channel
    if (verbose) cout << "Creating a new notification channel" << endl;
    CosNA::EventChannel_ptr channel = getnewchan_use_ns(orb, cfact_name, verbose);
    if (CORBA::is_nil(channel)) { // name service lookup failed
      channel = getnewchan_use_iorfile(orb, cfact_ior_file, verbose);
      if (CORBA::is_nil(channel)) {
	goto error_return; // both methods failed
      }
    }
#endif

    // (3) Create a client
    if (verbose) cout << "Creating client" << endl;
#ifndef LEGACY_CLIENT
    CLIENT_IMPL_CLASS* client =
      CLIENT_IMPL_CLASS::create(orb, channel, num_events, batch_size, CLIENT_NAME, proxy_ior_file, admin_ior_file,
				SUPPLY_OR_CONSUME_FN, CHANGE_FN,
				&evs, constraint_expr, millisecs, delay_millisecs, verbose);
#else
    CLIENT_IMPL_CLASS* client =
      CLIENT_IMPL_CLASS::create(channel, num_events, CLIENT_NAME,
				SUPPLY_OR_CONSUME_FN, millisecs, delay_millisecs, verbose);
#endif
    if (! client) {
      goto error_return; // failed to create client
    }
    // Obtain reference to the external client object
    if (verbose) cout << "Obtaining CORBA object ref to client" << endl;
    CLIENT_CLASS_VAR myref = client->_this(); // implicit activation
    client->_remove_ref(); // enables POA cleanup of servant

    // (4) Tell POA we are ready to go
    if (verbose) cout << "Activating POA manager" << endl;
    PortableServer::POAManager_var pman = poa->the_POAManager();
    pman->activate();

    // (5) Connect client to its proxy, wait for desired # of events
    if (verbose) cout << "Connecting client to its proxy" << endl;
    if (client->connect()) {
      // an immediate error occurred
    } else {
      if (verbose) cout << "Waiting for desired # of events" << endl;
      // Wait until num_events has been reached
      CORBA::Boolean communication_error = client->wait_done();
      if (communication_error && (num_events > 0)) {
	if (verbose) cout << "Communication error -- probably did not process desired # of events" << endl;
      }
      if (verbose) cout << "Cleaning up" << endl;
      client->cleanup();
    }
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
#ifdef CREATE_CHANNEL
#ifdef DESTROY_CHANNEL
  try {
    if (!CORBA::is_nil(channel)) {
      if (verbose) cout << "Destroying the channel we created" << endl;
      channel->destroy();
    }
  } catch(...) { }
#endif
#endif
  // orb->destroy();
  return -1;
 normal_return:
#ifdef CREATE_CHANNEL
#ifdef DESTROY_CHANNEL
  try {
    if (!CORBA::is_nil(channel)) {
      if (verbose) cout << "Destroying the channel we created" << endl;
      channel->destroy();
    }
  } catch(...) { }
#endif
#endif
  // orb->destroy();
  return 0;
}
