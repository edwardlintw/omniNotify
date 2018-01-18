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
  unsigned int i;
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

    // (1) Tell POA we are ready to go
    if (verbose) cout << "Activating POA manager" << endl;
    PortableServer::POAManager_var pman = poa->the_POAManager();
    pman->activate();

    // (2) parse the command line arguments
    if (parse_cmd_line(argc, argv, pname, SUPPORTS_FILTERS,
		       num_events, batch_size, millisecs, delay_millisecs, verbose,
		       channel_name, ior_file, cfact_name, cfact_ior_file,
		       proxy_ior_file, admin_ior_file, evs, constraint_expr)) {
      goto error_return; // command line parsing failed
    }
    num_con_events = num_events;

    // (3) Obtain reference to notification channel
    if (verbose) cout << "Obtaining reference to notification channel" << endl;
    CosNA::EventChannel_ptr channel = getchan_from_ns(orb, channel_name, verbose);
    if (CORBA::is_nil(channel)) { // name service lookup failed
      channel = getchan_from_iorfile(orb, ior_file, verbose);
      if (CORBA::is_nil(channel)) {
	goto error_return; // both methods failed
      }
    }

    // (4) Create all 12 CosNotify-style clients
    if (verbose) cout << "Creating 10 any_pull_consumer clients" << endl;

    PullConsumer_i* a_pull_c[10];
    CosNotifyComm::PullConsumer_var my_a_pull_c[10];
    char buf[100];
    for (i = 0; i < 10; i++) {
      sprintf(buf, "any_pull_consumer%d", i);
      if (verbose) cout << "Creating " << buf << endl;
      a_pull_c[i] =
	PullConsumer_i::create(orb, channel, num_con_events, batch_size, CORBA::string_dup(buf),
			       "", "", sample_consume_any_fn, sample_offer_change_fn,
			       &evs, constraint_expr, millisecs, delay_millisecs, verbose);
      if (! a_pull_c[i]) {
	goto error_return; // failed to create a client
      }
      my_a_pull_c[i] = a_pull_c[i]->_this();
      a_pull_c[i]->_remove_ref(); // enables POA cleanup of servant
    }

    // (5) Connect clients to their proxies -- consumers first
    if (verbose) cout << "Connecting consumer clients to proxies" << endl;

    for (i = 0; i < 10; i++) {
      if (verbose) cout << "Connecting Client # " << i+1 << endl;
      if (a_pull_c[i]->connect()) { goto error_return; }
    }

    if (verbose) cout << "Waiting for all 10 clients to finish" << endl;

    CORBA::Boolean a_com_err[10];
    CORBA::Boolean com_err = 0;
    for (i = 0; i < 10; i++) {
      a_com_err[i] = a_pull_c[i]->wait_done();
      if (a_com_err[i]) {
	com_err = 1;
      }
    }

    if (com_err && (num_events > 0)) {
      if (verbose) cout << "Communication error -- probably did not process desired # of events" << endl;
    }

    if (verbose) cout << "Cleaning up" << endl;

    for (i = 0; i < 10; i++) {
      if (!a_com_err[i]) { a_pull_c[i]->cleanup(); }
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
  // orb->destroy();
  return -1;
 normal_return:
  // orb->destroy();
  return 0;
}
