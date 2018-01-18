#include <stdlib.h>
#include <iostream>
using namespace std;
#include <stdio.h>

#include "CosNotifyShorthands.h"
#include "orb_init_name.h"
#include "get_channel.h"
#include "sample_clients.h"

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifndef HAVE_GETOPT
#  include "getopt_impl.h"
#endif

// decl of a helper function in sample_clients.cc
CORBA::Boolean sample_add_filter(CosNA::EventChannel_ptr channel, 
				 CosNF::FilterAdmin_ptr fadmin,
				 CosN::EventTypeSeq& evs,
				 const char* constraint_expr,
				 const char* obj_name,
				 CosNF::Filter_ptr& filter, // out param
				 CORBA::Boolean verbose);

void usage(char* pname) {
  cerr << "Usage: " << pname << " [-n name] [-f file] -p file" << endl << endl;
  cout << "  -n name  : channel name" << endl;
  cout << "               => use event service to lookup this name" << endl;
  cout << "  -f file  : file containing channel IOR" << endl;
  cout << "               => use string_to_object on the IOR" << endl;
  cout << "  -p file  : file with stringified IOR of proxy to be" << endl;
  cout << "               sent add_filter messages (REQUIRED)" << endl;
  cout << "(The channel is needed to obtain its default filter factory)" << endl;
}


int main(int argc, char** argv)
{
  char pname[256];
  strcpy(pname, argv[0]);
  char* channel_name = (char*)"";
  char* ior_file = (char*)""; 
  char* proxy_ior_file = (char*)""; 
  int c;

  DO_ORB_INIT(orb);

  // Process command line arguments
  while ( (c = getopt(argc, argv,"n:f:p:")) != EOF ) {
    switch (c) {
    case 'n' :
      channel_name = optarg;
      continue;
    case 'f' :
      ior_file = optarg;
      continue;
    case 'p' :
      proxy_ior_file = optarg;
      continue;
    default :
      usage(pname);
      goto cleanup;
    }
  }
  if (! strlen(proxy_ior_file) )  {
    usage(pname);
    goto cleanup;
  } 
  if (! (strlen(channel_name) || strlen(ior_file)) ) {
    channel_name = (char*)"EventChannel";
    ior_file = (char*)"/tmp/rdichan.ior";
  }

  try {
    CosNA::EventChannel_ptr channel = getchan_from_ns(orb, channel_name, 1);
    if (CORBA::is_nil(channel)) { // name service lookup failed
      channel = getchan_from_iorfile(orb, ior_file, 1);
      if (CORBA::is_nil(channel)) {
	goto cleanup;
      }
    }
    char buf[8096];
    FILE* ifile;
    if (! (ifile = fopen(proxy_ior_file, "r")) ) {
      cerr << "Failed to open file " << proxy_ior_file << " for reading" << endl;
      goto cleanup;
    }
    if (fscanf(ifile, "%s", buf) != 1) {
      cerr << "Failed to get an IOR from file " << proxy_ior_file << endl;
      fclose(ifile);
      goto cleanup;
    }
    fclose(ifile);
    CosNF::FilterAdmin_ptr proxy;
    try {
      CORBA::Object_var proxy_ref = orb->string_to_object(buf);
      if ( CORBA::is_nil(proxy_ref) ) {
	cerr << "Failed to turn IOR in file " << proxy_ior_file << " into object" << endl;
	goto cleanup;
      }
      proxy = CosNF::FilterAdmin::_narrow(proxy_ref);
      if ( CORBA::is_nil(proxy) ) {
	cerr << "Failed to narrow object from IOR in file " << proxy_ior_file << endl;
	goto cleanup;
      }
    } catch (...) {
      cerr << "Failed to convert to object from IOR in file " << proxy_ior_file << endl;
      goto cleanup;
    }

    CosNF::Filter_ptr filter = CosNF::Filter::_nil();
    while (1) {
      cout << endl << "Enter a constraint ('exit' to quit, 'remove' to remove all filters): " << endl <<  flush;
      if (!(fgets(buf, 8090, stdin))) break;
      if (strlen(buf) < 2) continue; // skip empty lines
      buf[strlen(buf)-1] = '\0'; // strip newline from end
      if (strcmp(buf, "exit") == 0) {
	break;
      }
      cout << "Calling remove_all_filters on the proxy" << endl;
      proxy->remove_all_filters();
      if (!CORBA::is_nil(filter)) {
	cout << "Destroying the previous filter, which is no longer in use" << endl;
	filter->destroy();
	filter = CosNF::Filter::_nil();
      }
      if (strcmp(buf, "remove") == 0) {
	// do nothing else
      } else {
	// assume it is a constraint expr
	CosN::EventTypeSeq evs;
	evs.length(0);
	sample_add_filter(channel, proxy, evs, buf, "demo_add_filter", filter, 1);
      }
      cout << endl << "---------------------------------------------------------" << endl;
    }
    orb->destroy();
    return 0; // success
  }
  catch(CORBA::SystemException&) {                     
    cerr << "Caught CORBA::SystemException." << endl;  
  }                                                    
  catch(CORBA::Exception&) {                           
    cerr << "Caught CORBA::Exception." << endl;        
  }                                                    
#if defined(__OMNIORB3__) || defined(__OMNIORB4__)
  catch(omniORB::fatalException& fe) {                 
    cerr << "Caught omniORB::fatalException:" << endl; 
    cerr << "  file: " << fe.file() << endl;           
    cerr << "  line: " << fe.line() << endl;           
    cerr << "  mesg: " << fe.errmsg() << endl;         
  }                                                    
#endif
  catch(...) {                                         
    cerr << "Caught unknown exception." << endl;       
  }
 cleanup:
  orb->destroy();
  return -1; // failure
}
