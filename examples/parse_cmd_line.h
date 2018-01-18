// -*- Mode: C++; -*-

#ifndef _PARSE_CMD_LINE_H
#define _PARSE_CMD_LINE_H

#include <stdlib.h>
#include <iostream>
using namespace std;

#include "CosNotifyShorthands.h"

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifndef HAVE_GETOPT
#  include "getopt_impl.h"
#endif

static void usage(const char* pname, CORBA::Boolean supports_filters)
{
  cout << "Usage: " << pname << " [-d #] [-b #] [-m #] [-M #] [-n name] [-f file] [-N name] [-F file] [-p file] [-a file] [-v]";
  if (supports_filters) {
    cout << " [-t types] [-e expr]" << endl;
  } else {
    cout << endl;
  }
  cout << "  -d #     : disconnect after # events [default 0 means never disconnect]" << endl;
  cout << "  -b #     : batch size (only relevant for batch supplier programs) [default 5]" << endl;
  cout << "               => must be >= 1" << endl;
  cout << "  -m #     : # of milliseconds to pause between each push/pull [default 0]" << endl;
  cout << "  -M #     : # of milliseconds to pause during incoming push, pull, try_pull [default 0]" << endl;
  cout << "             (-M only applies to push consumers and pull suppliers)" << endl;
  cout << "  -n name  : channel name" << endl;
  cout << "               => use event service to lookup this name" << endl;
  cout << "  -f file  : file containing channel IOR" << endl;
  cout << "               => use string_to_object on the IOR" << endl;
  cout << "  -N name  : factory name" << endl;
  cout << "               => use event service to lookup this name" << endl;
  cout << "  -F file  : file containing channel factory IOR" << endl;
  cout << "               => use string_to_object on the IOR" << endl;
  cout << "  -p file  : file to store IOR of the proxy used by this client" << endl;
  cout << "                (used by demo_add_filter, demo_offer_change, or demo_subscription_chang)" << endl;
  cout << "  -a file  : file to store IOR of the admin used by this client" << endl;
  cout << "                (used by demo_add_filter, demo_offer_change, or demo_subscription_chang)" << endl;
  cout << "  -v       : verbose output" << endl;
  cout << endl;
  if (supports_filters) {
    cout << "Options for adding a filter:" << endl;
    cout << "  -t types : add a filter for the specified types, which should be a" << endl;
    cout << "               comma-separated list of types of the form D::T" << endl;
    cout << "  -e expr  : add a filter with the specified boolean expression" << endl;
    cout << endl;
    cout << "If only -t specified, the expression \"true\" is used for the filter" << endl;
    cout << "If only -e specified, the event type \"*::*\" is used for the filter" << endl;
    cout << endl;
  }
  cout << "If both -n and -f are specified, the -n option is tried first" << endl;
  cout << endl;
  cout << "If neither -n or -f is specified, program first tries name service" << endl;
  cout << "using name \"EventChannel\", then default IOR file /tmp/rdichan.ior" << endl;
  cout << endl;
}

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

// Return TRUE if there is an error, false if OK
CORBA::Boolean parse_cmd_line(int argc, char** argv,
			      char* pname,
			      CORBA::Boolean supports_filters,
			      CORBA::ULong& num_events,
			      CORBA::ULong& batch_size,
			      CORBA::ULong& millisecs,
			      CORBA::ULong& delay_millisecs,
			      CORBA::Boolean& verbose,
			      char*& channel_name,
			      char*& ior_file,
			      char*& cfact_name,
			      char*& cfact_ior_file,
			      char*& proxy_ior_file,
			      char*& admin_ior_file,
			      CosN::EventTypeSeq& evs,
			      char*& constraint_expr
			      )
{
  batch_size = 5;
  channel_name = (char*)"";
  cfact_name = (char*)"";
  ior_file = (char*)"";
  cfact_ior_file = (char*)"";
  proxy_ior_file = (char*)"";
  admin_ior_file = (char*)"";
  constraint_expr = (char*)"";
  evs.length(0);
  const char* etype_list = "";
  int c;
  long tmp_long = 0;

  // Process command line arguments
  const char* opts = "d:b:m:M:n:f:N:F:a:p:t:e:v";
  while ( (c = getopt(argc, argv, opts)) != EOF ) {
    switch (c) {
    case 'd' :
      tmp_long = atol(optarg);
      if (tmp_long < 0) { usage(pname, supports_filters); return 1; }
      num_events = (CORBA::ULong)tmp_long;
      continue;
    case 'b' :
      tmp_long = atol(optarg);
      if (tmp_long < 1) { usage(pname, supports_filters); return 1; }
      batch_size = (CORBA::ULong)tmp_long;
      continue;
    case 'm' :
      tmp_long = atol(optarg);
      if (tmp_long < 0) { usage(pname, supports_filters); return 1; }
      millisecs = (CORBA::ULong)tmp_long;
      continue;
    case 'M' :
      tmp_long = atol(optarg);
      if (tmp_long < 0) { usage(pname, supports_filters); return 1; }
      delay_millisecs = (CORBA::ULong)tmp_long;
      continue;
    case 'n' :
      channel_name = optarg;
      continue;
    case 'f' :
      ior_file = optarg;
      continue;
    case 'N' :
      cfact_name = optarg;
      continue;
    case 'F' :
      cfact_ior_file = optarg;
      continue;
    case 'p' :
      proxy_ior_file = optarg;
      continue;
    case 'a' :
      admin_ior_file = optarg;
      continue;
    case 't' :
      etype_list = optarg;
      continue;
    case 'e' :
      constraint_expr = optarg;
      continue;
    case 'v' :
      verbose = 1;
      continue;
    default :
      usage(pname, supports_filters);
      return 1;
    }
  }
  if (! (strlen(channel_name) || strlen(ior_file)) ) {
    channel_name = (char*)"EventChannel";
    ior_file = (char*)"/tmp/rdichan.ior";
  }
  if (! (strlen(cfact_name) || strlen(cfact_ior_file)) ) {
    cfact_name = (char*)"ChannelFactory";
    cfact_ior_file = (char*)"/tmp/rdifact.ior";
  }
  if ( (!supports_filters) && ( strlen(etype_list) || strlen(constraint_expr) ) ) {
    cout << "Filter options not supported for this client" << endl;
    usage(pname, supports_filters);
    return 1; // error
  }
  if (strlen(etype_list) && parse_etype_list(etype_list, evs)) {
    cout << "Event type list for -t option must be comma-separated list of D::T" << endl << endl; 
    usage(pname, supports_filters);
    return 1;
  }
  if ( evs.length() && (! strlen(constraint_expr))) {
    constraint_expr = (char*)"true";
  }
  return 0; // OK
}

#endif
