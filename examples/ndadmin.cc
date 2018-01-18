//
// Use the AttN::Interact API to interact with notifd
//
// Make sure we are using POA
#ifdef COS_USES_BOA
#error COS_USES_BOA should not be set for this example
#endif

#include <iostream>
using namespace std;

#include "orb_init_name.h"
#include "get_channel.h"
#include "RDIStringDefs.h"
#include "RDIInteractiveMode.h"

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifndef HAVE_GETOPT
#  include "getopt_impl.h"
#endif

static void usage(const char* pname)
{
  cout << "Usage: " << pname << " [-n name] [-f file] [-v]" << endl;
  cout << "  -n name  : channel name" << endl;
  cout << "               => use event service to lookup this name" << endl;
  cout << "  -f file  : file containing channel IOR" << endl;
  cout << "               => use string_to_object on the IOR" << endl;
  cout << "  -v       : verbose output" << endl;
  cout << "NOTE: The server has a default channel; " << pname << " finds this channel" << endl;
  cout << "      and then goes 'UP' to find the notifd server" << endl;
  cout << endl;
}

int main(int argc, char** argv) {
  CORBA::Boolean success, target_changed;
  CORBA::String_var cmdres;
  AttN::Interactive_var target = AttN::Interactive::_nil();
  AttN::Interactive_var next_target = AttN::Interactive::_nil();
  CosNA::EventChannel_var channel = CosNA::EventChannel::_nil();
  AttN::Server_var server = AttN::Server::_nil();
  CORBA::Boolean docmd_problem = 0;
  char pname[256];
  strcpy(pname, argv[0]);
  char* channel_name = (char*)"";
  char* ior_file = (char*)""; 
  CORBA::Boolean verbose = 0;
  
  DO_ORB_INIT(orb);
  try {
    CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
    PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);

    // (1) parse the command line arguments
    int c;
    const char* opts = "n:f:v";
    while ( (c = getopt(argc, argv, opts)) != EOF ) {
      switch (c) {
      case 'n' :
	channel_name = optarg;
	continue;
      case 'f' :
	ior_file = optarg;
	continue;
      case 'v' :
	verbose = 1;
	continue;
      default :
	usage(pname);
	goto err_return;
      }
    }
    if (! (strlen(channel_name) || strlen(ior_file)) ) {
      channel_name = (char*)"EventChannel";
      ior_file = (char*)"/tmp/rdichan.ior";
    }

    // (2) Obtain reference to notification channel
    if (verbose) cout << "Obtaining reference to notification channel" << endl;
    channel = getchan_from_ns(orb, channel_name, verbose);
    if (CORBA::is_nil(channel)) { // name service lookup failed
      channel = getchan_from_iorfile(orb, ior_file, verbose);
      if (CORBA::is_nil(channel)) {
	if (verbose) cout << "Could not find server's default channel" << endl;
	goto err_return; // both methods failed
      }
    }

    // (3) Find server: must go up twice
    target = AttN::Interactive::_narrow(channel);
    try {
      cmdres = target->do_command("up", success, target_changed, next_target);
    }
    catch ( CORBA::INV_OBJREF& e ) { docmd_problem = 1; } \
    catch ( CORBA::OBJECT_NOT_EXIST& e ) { docmd_problem = 1; } \
    catch ( CORBA::COMM_FAILURE& e ) { docmd_problem = 1; }
    if (!success || !target_changed || docmd_problem) {
      if (verbose) cout << "Could not find server's default channel" << endl;
      goto err_return;
    }
    target = next_target;
    try {
      cmdres = target->do_command("up", success, target_changed, next_target);
    }
    catch ( CORBA::INV_OBJREF& e ) { docmd_problem = 1; } \
    catch ( CORBA::OBJECT_NOT_EXIST& e ) { docmd_problem = 1; } \
    catch ( CORBA::COMM_FAILURE& e ) { docmd_problem = 1; }
    if (!success || !target_changed || docmd_problem) {
      if (verbose) cout << "Could not find server's default channel" << endl;
      goto err_return;
    }
    target = AttN::Interactive::_duplicate(next_target);
    server = AttN::Server::_narrow(target);

    // (4) Process user input
    RDI_Interactive_Mode(server, 0); // do not spawn thread
    goto norm_return;
  }
  catch(CORBA::SystemException&) {                     
    cerr << pname << " main caught CORBA::SystemException." << endl;  
  }                                                    
  catch(CORBA::Exception&) {                           
    cerr << pname << " main caught CORBA::Exception." << endl;        
  }                                                    
  catch(omniORB::fatalException& fe) {                 
    cerr << "main caught omniORB::fatalException:" << endl; 
    cerr << "  file: " << fe.file() << endl;           
    cerr << "  line: " << fe.line() << endl;           
    cerr << "  mesg: " << fe.errmsg() << endl;         
  }                                                    
  catch(...) {                                         
    // nameclt comment says it is a bad idea to report an error here 
  }
  cerr << "main caught an exception." << endl;
 err_return:
  cerr << "QUITTING due to error" << endl;
  return -1;
 norm_return:
  return 0;
}
