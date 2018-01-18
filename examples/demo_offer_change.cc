#include <stdlib.h>
#include <iostream>
using namespace std;
#include <stdio.h>

#include "CosNotifyShorthands.h"
#include "orb_init_name.h"
#include "get_channel.h"

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifndef HAVE_GETOPT
#  include "getopt_impl.h"
#endif

// return 0 if etype_list is a valid comma-separated event type list
// (and fill in evs with the event types); else return 1 (1 means error)
CORBA::Boolean parse_etype_list(const char* etype_list, CosN::EventTypeSeq& evs)
{
  char *elist;
  CORBA::Long len = 0;

  if ( ! etype_list )
    return 1; // error
  elist = CORBA::string_dup(etype_list);
  char* b = elist;
  while (b != 0) {
    char* e = strchr(b, ',');
    if (e != NULL) {
      *e = 0;
      e++;
    }
    char* m = strchr(b, ':');
    if (m == NULL) {
      return 1; // error
    }
    *m = 0;
    m++;
    if (*m != ':') {
      return 1; // error
    }
    *m = 0;
    m++;
    char* domain_name = CORBA::string_dup(b);
    char* type_name = CORBA::string_dup(m);
    b = e;
    if (strlen(domain_name) && strlen(type_name)) {
      len++;
      evs.length(len);
      evs[len-1].domain_name = domain_name;
      evs[len-1].type_name = type_name;
    } else {
      return 1; // error
    }
  }
  return ((len > 0) ? 0 : 1);
}

void usage(char* pname) {
  cerr << "Usage: " << pname << " -p file" << endl << endl;
  cout << "  -p file  : file with stringified IOR of proxy to be" << endl;
  cout << "               sent offer_change messages (REQUIRED)" << endl;
}

int main(int argc, char** argv)
{
  char pname[256];
  strcpy(pname, argv[0]);
  char* proxy_ior_file = (char*)""; 
  int c;

  DO_ORB_INIT(orb);

  // Process command line arguments
  while ( (c = getopt(argc, argv,"n:f:p:")) != EOF ) {
    switch (c) {
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

  try {
    CosN::EventTypeSeq added, deled;
    added.length(0);
    deled.length(0);
    char buf[8096];
    char buf2[8096];
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
    CosNC::NotifyPublish_ptr proxy;
    try {
      CORBA::Object_var proxy_ref = orb->string_to_object(buf);
      if ( CORBA::is_nil(proxy_ref) ) {
	cerr << "Failed to turn IOR in file " << proxy_ior_file << " into object" << endl;
	goto cleanup;
      }
      proxy = CosNC::NotifyPublish::_narrow(proxy_ref);
      if ( CORBA::is_nil(proxy) ) {
	cerr << "Failed to narrow object from IOR in file " << proxy_ior_file << endl;
	goto cleanup;
      }
    } catch (...) {
      cerr << "Failed to convert to object from IOR in file " << proxy_ior_file << endl;
      goto cleanup;
    }

    while (1) {
      added.length(0);
      deled.length(0);
      cout << endl << "Enter list of removals (or empty for none): " << endl <<  flush;
      if (!(fgets(buf, 8090, stdin))) break;
      if (strlen(buf) < 1) break;
      buf[strlen(buf)-1] = '\0'; // strip newline from end
      if (strcmp(buf, "exit") == 0) break;
      if (strlen(buf)) {
	if (parse_etype_list(buf, deled)) {
	  cerr << "Failed to parse the input as an event type list" << endl;
	  continue;
	}
      }

      cout << endl << "Enter list of additions (or empty for none): " << endl <<  flush;
      if (!(fgets(buf2, 8090, stdin))) break;
      if (strlen(buf2) < 1) break;
      buf2[strlen(buf2)-1] = '\0'; // strip newline from end
      if (strcmp(buf2, "exit") == 0) break;
      if (strlen(buf2)) {
	if (parse_etype_list(buf2, added)) {
	  cerr << "Failed to parse the input as an event type list" << endl;
	  continue;
	}
      }

      cout << "Calling offer_change on the proxy" << endl;
      proxy->offer_change(added, deled);
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
