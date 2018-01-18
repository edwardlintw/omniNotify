// Make sure we are using POA
#ifdef COS_USES_BOA
#error COS_USES_BOA should not be set for this example
#endif

#include <stdlib.h>
#include <iostream>
using namespace std;

#include "orb_init_name.h"
#include "get_channel.h"

#include "sample_clients.h"
#include "legacy_clients.h"
#define SUPPORTS_FILTERS 1

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifndef HAVE_GETOPT
#  include "getopt_impl.h"
#endif

static void usage(const char* pname)
{
  cout << "Usage: " << pname << " -u client_type [-u client_type ...] [-c #] [-s #] [-b #] [-m #] [-M #] [-n name] [-f file] [-v] [-t types] [-e expr]" << endl;
  cout << "  -u type  : use the specified client type (one client of that type is used)." << endl;
  cout << "             Specify as many as you like, one -u option for each.  Types:" << endl;
  cout << "                lpushc : legacy push consumer" << endl;   
  cout << "                apushc : any push consumer" << endl; 
  cout << "                spushc : structured push consumer" << endl; 
  cout << "                bpushc : batch push consumer" << endl; 
  cout << endl;
  cout << "                lpullc : legacy pull consumer" << endl;   
  cout << "                apullc : any pull consumer" << endl; 
  cout << "                spullc : structured pull consumer" << endl; 
  cout << "                bpullc : batch pull consumer" << endl; 
  cout << endl;
  cout << "                lpushs : legacy push supplier" << endl;   
  cout << "                apushs : any push supplier" << endl; 
  cout << "                spushs : structured push supplier" << endl; 
  cout << "                bpushs : batch push supplier" << endl; 
  cout << endl;
  cout << "                lpulls : legacy pull supplier" << endl;   
  cout << "                apulls : any pull supplier" << endl; 
  cout << "                spulls : structured pull supplier" << endl; 
  cout << "                bpulls : batch pull supplier" << endl; 
  cout << endl;
  cout << "  -s #     : for suppliers: disconnect after # events supplied [default 0 means never disconnect]" << endl;
  cout << "  -c #     : for consumers: disconnect after # events consumed [default 0 means never disconnect]" << endl;
  cout << "             **  N.B. You should take # of suppliers into account:" << endl;
  cout << "                 if 3 suppliers supply 30 events, each consumer should consume 90 events" << endl; 
  cout << "  -b #     : batch size (only relevant for batch supplier programs) [default 5]" << endl;
  cout << "               => must be >= 1" << endl;
  cout << "  -m #     : # of milliseconds to pause between each push/pull [default 0]" << endl;
  cout << "  -M #     : # of milliseconds to pause during incoming push, pull, try_pull [default 0]" << endl;
  cout << "             (-M only applies to push consumers and pull suppliers)" << endl;
  cout << "  -n name  : channel name" << endl;
  cout << "               => use event service to lookup this name" << endl;
  cout << "  -f file  : file containing channel IOR" << endl;
  cout << "               => use string_to_object on the IOR" << endl;
  cout << "  -v       : verbose output" << endl;
  cout << endl;
  cout << "Options for adding a filter **:" << endl;
  cout << "  -t types : add a filter for the specified types, which should be a" << endl;
  cout << "               comma-separated list of types of the form D::T" << endl;
  cout << "  -e expr  : add a filter with the specified boolean expression" << endl;
  cout << "** N.B. Only add filters that always evaluate to TRUE to get proper program termination" << endl;
  cout << endl;
  cout << "If only -t specified, the expression \"true\" is used for the filter" << endl;
  cout << "If only -e specified, the event type \"*::*\" is used for the filter" << endl;
  cout << endl;
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

int main(int argc, char** argv) {
  char pname[256];
  strcpy(pname, argv[0]);
  CORBA::Boolean USE_L_PUSH_C = 0;
  CORBA::Boolean USE_A_PUSH_C = 0;
  CORBA::Boolean USE_S_PUSH_C = 0;
  CORBA::Boolean USE_B_PUSH_C = 0;
  CORBA::Boolean USE_L_PULL_C = 0;
  CORBA::Boolean USE_A_PULL_C = 0;
  CORBA::Boolean USE_S_PULL_C = 0;
  CORBA::Boolean USE_B_PULL_C = 0;
  CORBA::Boolean USE_L_PUSH_S = 0;
  CORBA::Boolean USE_A_PUSH_S = 0;
  CORBA::Boolean USE_S_PUSH_S = 0;
  CORBA::Boolean USE_B_PUSH_S = 0;
  CORBA::Boolean USE_L_PULL_S = 0;
  CORBA::Boolean USE_A_PULL_S = 0;
  CORBA::Boolean USE_S_PULL_S = 0;
  CORBA::Boolean USE_B_PULL_S = 0;
  int num_con = 0, num_sup = 0;

  CosN::EventTypeSeq evs;
  evs.length(0);
  char* constraint_expr = (char*)"";
  char* channel_name = (char*)"";
  char* ior_file = (char*)""; 
  CORBA::ULong num_sup_events = 0, num_con_events = 0, batch_size = 5, millisecs = 0, delay_millisecs = 0;
  CORBA::Boolean verbose = 0;
  
  DO_ORB_INIT(orb);
  try {
    CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
    PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);

    // (1) parse the command line arguments
    int c;
    long tmp_long = 0;
    const char* etype_list = "";
    const char* opts = "u:s:c:b:m:M:n:f:t:e:v";
    while ( (c = getopt(argc, argv, opts)) != EOF ) {
      switch (c) {
      case 'u':
	if (strcmp(optarg, "lpushc") == 0) {
	  USE_L_PUSH_C = 1; num_con++;
	} else if (strcmp(optarg, "apushc") == 0) {
	  USE_A_PUSH_C = 1; num_con++;
	} else if (strcmp(optarg, "spushc") == 0) {
	  USE_S_PUSH_C = 1; num_con++;
	} else if (strcmp(optarg, "bpushc") == 0) {
	  USE_B_PUSH_C = 1; num_con++;
	} else if (strcmp(optarg, "lpullc") == 0) {
	  USE_L_PULL_C = 1; num_con++;
	} else if (strcmp(optarg, "apullc") == 0) {
	  USE_A_PULL_C = 1; num_con++;
	} else if (strcmp(optarg, "spullc") == 0) {
	  USE_S_PULL_C = 1; num_con++;
	} else if (strcmp(optarg, "bpullc") == 0) {
	  USE_B_PULL_C = 1; num_con++;
	} else if (strcmp(optarg, "lpushs") == 0) {
	  USE_L_PUSH_S = 1; num_sup++;
	} else if (strcmp(optarg, "apushs") == 0) {
	  USE_A_PUSH_S = 1; num_sup++;
	} else if (strcmp(optarg, "spushs") == 0) {
	  USE_S_PUSH_S = 1; num_sup++;
	} else if (strcmp(optarg, "bpushs") == 0) {
	  USE_B_PUSH_S = 1; num_sup++;
	} else if (strcmp(optarg, "lpulls") == 0) {
	  USE_L_PULL_S = 1; num_sup++;
	} else if (strcmp(optarg, "apulls") == 0) {
	  USE_A_PULL_S = 1; num_sup++;
	} else if (strcmp(optarg, "spulls") == 0) {
	  USE_S_PULL_S = 1; num_sup++;
	} else if (strcmp(optarg, "bpulls") == 0) {
	  USE_B_PULL_S = 1; num_sup++;
	} else {
	  cout << endl << "Bad client type (" << optarg << ") for -u option" << endl;
	  usage(pname);
	  goto error_return;
	}
	continue;
      case 's' :
	tmp_long = atol(optarg);
	if (tmp_long < 0) { usage(pname); goto error_return; }
	num_sup_events = (CORBA::ULong)tmp_long;
	continue;
      case 'c' :
	tmp_long = atol(optarg);
	if (tmp_long < 0) { usage(pname); goto error_return; }
	num_con_events = (CORBA::ULong)tmp_long;
	continue;
      case 'b' :
	tmp_long = atol(optarg);
	if (tmp_long < 1) { usage(pname); goto error_return; }
	batch_size = (CORBA::ULong)tmp_long;
	continue;
      case 'm' :
	tmp_long = atol(optarg);
	if (tmp_long < 0) { usage(pname); goto error_return; }
	millisecs = (CORBA::ULong)tmp_long;
	continue;
      case 'M' :
	tmp_long = atol(optarg);
	if (tmp_long < 0) { usage(pname); goto error_return; }
	delay_millisecs = (CORBA::ULong)tmp_long;
	continue;
      case 'n' :
	channel_name = optarg;
	continue;
      case 'f' :
	ior_file = optarg;
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
	usage(pname);
	goto error_return;
      }
    }
    if (! (strlen(channel_name) || strlen(ior_file)) ) {
      channel_name = (char*)"EventChannel";
      ior_file = (char*)"/tmp/rdichan.ior";
    }
    if (strlen(etype_list) && parse_etype_list(etype_list, evs)) {
      cout << "Event type list for -t option must be comma-separated list of D::T" << endl << endl; 
      usage(pname);
      goto error_return;
    }
    if ( evs.length() && (! strlen(constraint_expr))) {
      constraint_expr = (char*)"true";
    }
    if (num_con + num_sup == 0) {
      cout << endl << "** You must use at least one -u option to specify a client **" << endl;
      usage(pname);
      goto error_return;
    }
    if (num_con && num_sup && num_con_events && num_sup_events && (num_con_events != (num_sup * num_sup_events))) {
      cout << endl << "** WARNING: Unless filtering or external clients change the count, your math is wrong" << endl;
      cout << "    num_con_events != num_sup * num_sup_events" << endl;
      cout << "    ( " << num_con_events << " != " << num_sup << " * " << num_sup_events << " )" << endl;
      cout << "Proceeding anyway" << endl;
    }

    // (2) Obtain reference to notification channel
    if (verbose) cout << "Obtaining reference to notification channel" << endl;
    CosNA::EventChannel_ptr channel = getchan_from_ns(orb, channel_name, verbose);
    if (CORBA::is_nil(channel)) { // name service lookup failed
      channel = getchan_from_iorfile(orb, ior_file, verbose);
      if (CORBA::is_nil(channel)) {
	goto error_return; // both methods failed
      }
    }

    // (3) Create clients

    LegacyPushConsumer_i* l_push_c = 0;
    PushConsumer_i* a_push_c = 0;
    StructuredPushConsumer_i* s_push_c = 0;
    SequencePushConsumer_i* b_push_c = 0;

    LegacyPullConsumer_i* l_pull_c = 0;
    PullConsumer_i* a_pull_c = 0;
    StructuredPullConsumer_i* s_pull_c = 0;
    SequencePullConsumer_i* b_pull_c = 0;

    LegacyPushSupplier_i* l_push_s = 0;
    PushSupplier_i* a_push_s = 0;
    StructuredPushSupplier_i* s_push_s = 0;
    SequencePushSupplier_i* b_push_s = 0;

    LegacyPullSupplier_i* l_pull_s = 0;
    PullSupplier_i* a_pull_s = 0;
    StructuredPullSupplier_i* s_pull_s = 0;
    SequencePullSupplier_i* b_pull_s = 0;

    CosEvC::PushConsumer_var my_l_push_c = CosEvC::PushConsumer::_nil();
    CosNotifyComm::PushConsumer_var my_a_push_c = CosNotifyComm::PushConsumer::_nil();
    CosNotifyComm::StructuredPushConsumer_var my_s_push_c = CosNotifyComm::StructuredPushConsumer::_nil();
    CosNotifyComm::SequencePushConsumer_var my_b_push_c = CosNotifyComm::SequencePushConsumer::_nil();

    CosEvC::PullConsumer_var my_l_pull_c = CosEvC::PullConsumer::_nil();
    CosNotifyComm::PullConsumer_var my_a_pull_c = CosNotifyComm::PullConsumer::_nil();
    CosNotifyComm::StructuredPullConsumer_var my_s_pull_c = CosNotifyComm::StructuredPullConsumer::_nil();
    CosNotifyComm::SequencePullConsumer_var my_b_pull_c = CosNotifyComm::SequencePullConsumer::_nil();

    CosEvC::PushSupplier_var my_l_push_s = CosEvC::PushSupplier::_nil();
    CosNotifyComm::PushSupplier_var my_a_push_s = CosNotifyComm::PushSupplier::_nil();
    CosNotifyComm::StructuredPushSupplier_var my_s_push_s = CosNotifyComm::StructuredPushSupplier::_nil();
    CosNotifyComm::SequencePushSupplier_var my_b_push_s = CosNotifyComm::SequencePushSupplier::_nil();

    CosEvC::PullSupplier_var my_l_pull_s = CosEvC::PullSupplier::_nil();
    CosNotifyComm::PullSupplier_var my_a_pull_s = CosNotifyComm::PullSupplier::_nil();
    CosNotifyComm::StructuredPullSupplier_var my_s_pull_s = CosNotifyComm::StructuredPullSupplier::_nil();
    CosNotifyComm::SequencePullSupplier_var my_b_pull_s = CosNotifyComm::SequencePullSupplier::_nil();

    // (3.1) legacy_push_consumer
    if (USE_L_PUSH_C) {
      cout << "Constructing legacy_push_consumer, num_con_events = " << num_con_events << endl;
      l_push_c =
	LegacyPushConsumer_i::create(channel, num_con_events, "legacy_push_consumer",
				     sample_consume_any_fn, millisecs, delay_millisecs, verbose);
      if (! l_push_c) {
	cerr << "Constructing legacy_push_consumer failed" << endl;
	goto error_return; // failed to create a client
      }
      my_l_push_c = l_push_c->_this();
      l_push_c->_remove_ref(); // enables POA cleanup of servant
    }

      // (3.2) legacy_pull_consumer
    if (USE_L_PULL_C) {
      cout << "Constructing legacy_pull_consumer, num_con_events = " << num_con_events << endl;
      l_pull_c =
	LegacyPullConsumer_i::create(channel, num_con_events, "legacy_pull_consumer",
				     sample_consume_any_fn, millisecs, delay_millisecs, verbose);
      if (! l_pull_c) {
	cerr << "Constructing legacy_pull_consumer failed" << endl;
	goto error_return; // failed to create a client
      }
      my_l_pull_c = l_pull_c->_this();
      l_pull_c->_remove_ref(); // enables POA cleanup of servant
    }

    // (3.3) any_push_consumer
    if (USE_A_PUSH_C) {
      cout << "Constructing any_push_consumer, num_con_events = " << num_con_events << endl;
      a_push_c =
	PushConsumer_i::create(orb, channel, num_con_events, batch_size, "any_push_consumer",
			       "", "", sample_consume_any_fn, sample_offer_change_fn,
			       &evs, constraint_expr, millisecs, delay_millisecs, verbose);
      if (! a_push_c) {
	cerr << "Constructing any_push_consumer failed" << endl;
	goto error_return; // failed to create a client
      }
      my_a_push_c = a_push_c->_this();
      a_push_c->_remove_ref(); // enables POA cleanup of servant
    }

    // (3.4) struct_push_consumer
    if (USE_S_PUSH_C) {
      cout << "Constructing struct_push_consumer, num_con_events = " << num_con_events << endl;
      s_push_c =
	StructuredPushConsumer_i::create(orb, channel, num_con_events, batch_size, "struct_push_consumer",
					 "", "", sample_consume_structured_fn, sample_offer_change_fn,
					 &evs, constraint_expr, millisecs, delay_millisecs, verbose);
      if (! s_push_c) {
	cerr << "Constructing struct_push_consumer failed" << endl;
	goto error_return; // failed to create a client
      }
      my_s_push_c = s_push_c->_this();
      s_push_c->_remove_ref(); // enables POA cleanup of servant
    }

    // (3.5) batch_push_consumer
    if (USE_B_PUSH_C) {
      cout << "Constructing batch_push_consumer, num_con_events = " << num_con_events << endl;
      b_push_c =
	SequencePushConsumer_i::create(orb, channel, num_con_events, batch_size, "batch_push_consumer",
				       "", "", sample_consume_batch_fn, sample_offer_change_fn,
				       &evs, constraint_expr, millisecs, delay_millisecs, verbose);
      if (! b_push_c) {
	cerr << "Constructing batch_push_consumer failed" << endl;
	goto error_return; // failed to create a client
      }
      my_b_push_c = b_push_c->_this();
      b_push_c->_remove_ref(); // enables POA cleanup of servant
    }

    // (3.6) any_pull_consumer
    if (USE_A_PULL_C) {
      cout << "Constructing any_pull_consumer, num_con_events = " << num_con_events << endl;
      a_pull_c =
	PullConsumer_i::create(orb, channel, num_con_events, batch_size, "any_pull_consumer",
			       "", "", sample_consume_any_fn, sample_offer_change_fn,
			       &evs, constraint_expr, millisecs, delay_millisecs, verbose);
      if (! a_pull_c) {
	cerr << "Constructing any_pull_consumer failed" << endl;
	goto error_return; // failed to create a client
      }
      my_a_pull_c = a_pull_c->_this();
      a_pull_c->_remove_ref(); // enables POA cleanup of servant
    }

    // (3.7) struct_pull_consumer
    if (USE_S_PULL_C) {
      cout << "Constructing struct_pull_consumer, num_con_events = " << num_con_events << endl;
      s_pull_c =
	StructuredPullConsumer_i::create(orb, channel, num_con_events, batch_size, "struct_pull_consumer",
					 "", "", sample_consume_structured_fn, sample_offer_change_fn,
					 &evs, constraint_expr, millisecs, delay_millisecs, verbose);
      if (! s_pull_c) {
	cerr << "Constructing struct_pull_consumer failed" << endl;
	goto error_return; // failed to create a client
      }
      my_s_pull_c = s_pull_c->_this();
      s_pull_c->_remove_ref(); // enables POA cleanup of servant
    }

    // (3.8) batch_pull_consumer
    if (USE_B_PULL_C) {
      cout << "Constructing batch_pull_consumer, num_con_events = " << num_con_events << endl;
      b_pull_c =
	SequencePullConsumer_i::create(orb, channel, num_con_events, batch_size, "batch_pull_consumer",
				       "", "", sample_consume_batch_fn, sample_offer_change_fn,
				       &evs, constraint_expr, millisecs, delay_millisecs, verbose);
      if (! b_pull_c) {
	cerr << "Constructing batch_pull_consumer failed" << endl;
	goto error_return; // failed to create a client
      }
      my_b_pull_c = b_pull_c->_this();
      b_pull_c->_remove_ref(); // enables POA cleanup of servant
    }

    // (3.9) legacy_push_supplier
    if (USE_L_PUSH_S) {
      cout << "Constructing legacy_push_supplier, num_sup_events = " << num_sup_events << endl;
      l_push_s =
	LegacyPushSupplier_i::create(channel, num_sup_events, "legacy_push_supplier",
				     sample_supply_any_fn, millisecs, delay_millisecs, verbose);
      if (! l_push_s) {
	cerr << "Constructing legacy_push_supplier failed" << endl;
	goto error_return; // failed to create a client
      }
      my_l_push_s = l_push_s->_this();
      l_push_s->_remove_ref(); // enables POA cleanup of servant
    }

    // (3.10) legacy_pull_supplier
    if (USE_L_PULL_S) {
      cout << "Constructing legacy_pull_supplier, num_sup_events = " << num_sup_events <<  endl;
      l_pull_s =
	LegacyPullSupplier_i::create(channel, num_sup_events, "legacy_pull_supplier",
				     sample_supply_any_fn, millisecs, delay_millisecs, verbose);
      if (! l_pull_s) {
	cerr << "Constructing legacy_pull_supplier failed" << endl;
	goto error_return; // failed to create a client
      }
      my_l_pull_s = l_pull_s->_this();
      l_pull_s->_remove_ref(); // enables POA cleanup of servant
    }

    // (3.11) any_push_supplier
    if (USE_A_PUSH_S) {
      cout << "Constructing any_push_supplier, num_sup_events = " << num_sup_events <<  endl;
      a_push_s =
	PushSupplier_i::create(orb, channel, num_sup_events, batch_size, "any_push_supplier",
			       "", "", sample_supply_any_fn, sample_subscription_change_fn,
			       &evs, constraint_expr, millisecs, delay_millisecs, verbose);
      if (! a_push_s) {
	cerr << "Constructing any_push_supplier failed" << endl;
	goto error_return; // failed to create a client
      }
      my_a_push_s = a_push_s->_this();
      a_push_s->_remove_ref(); // enables POA cleanup of servant
    }

    // (3.12) struct_push_supplier
    if (USE_S_PUSH_S) {
      cout << "Constructing struct_push_supplier, num_sup_events = " << num_sup_events <<  endl;
      s_push_s =
	StructuredPushSupplier_i::create(orb, channel, num_sup_events, batch_size, "struct_push_supplier",
					 "", "", sample_supply_structured_fn, sample_subscription_change_fn,
					 &evs, constraint_expr, millisecs, delay_millisecs, verbose);
      if (! s_push_s) {
	cerr << "Constructing struct_push_supplier failed" << endl;
	goto error_return; // failed to create a client
      }
      my_s_push_s = s_push_s->_this();
      s_push_s->_remove_ref(); // enables POA cleanup of servant
    }

    // (3.13) batch_push_supplier
    if (USE_B_PUSH_S) {
      cout << "Constructing batch_push_supplier, num_sup_events = " << num_sup_events <<  endl;
      b_push_s =
	SequencePushSupplier_i::create(orb, channel, num_sup_events, batch_size, "batch_push_supplier",
				       "", "", sample_supply_batch_fn, sample_subscription_change_fn,
				       &evs, constraint_expr, millisecs, delay_millisecs, verbose);
      if (! b_push_s) {
	cerr << "Constructing batch_push_supplier failed" << endl;
	goto error_return; // failed to create a client
      }
      my_b_push_s = b_push_s->_this();
      b_push_s->_remove_ref(); // enables POA cleanup of servant
    }

    // (3.14) any_pull_supplier
    if (USE_A_PULL_S) {
      cout << "Constructing any_pull_supplier, num_sup_events = " << num_sup_events <<  endl;
      a_pull_s =
	PullSupplier_i::create(orb, channel, num_sup_events, batch_size, "any_pull_supplier",
			       "", "", sample_supply_any_fn, sample_subscription_change_fn,
			       &evs, constraint_expr, millisecs, delay_millisecs, verbose);
      if (! a_pull_s) {
	cerr << "Constructing any_pull_supplier failed" << endl;
	goto error_return; // failed to create a client
      }
      my_a_pull_s = a_pull_s->_this();
      a_pull_s->_remove_ref(); // enables POA cleanup of servant
    }

    // (3.15) struct_pull_supplier
    if (USE_S_PULL_S) {
      cout << "Constructing struct_pull_supplier, num_sup_events = " << num_sup_events <<  endl;
      s_pull_s =
	StructuredPullSupplier_i::create(orb, channel, num_sup_events, batch_size, "struct_pull_supplier",
					 "", "", sample_supply_structured_fn, sample_subscription_change_fn,
					 &evs, constraint_expr, millisecs, delay_millisecs, verbose);
      if (! s_pull_s) {
	cerr << "Constructing struct_pull_supplier failed" << endl;
	goto error_return; // failed to create a client
      }
      my_s_pull_s = s_pull_s->_this();
      s_pull_s->_remove_ref(); // enables POA cleanup of servant
    }

    // (3.16) batch_pull_supplier
    if (USE_B_PULL_S) {
      cout << "Constructing batch_pull_supplier, num_sup_events = " << num_sup_events <<  endl;
      b_pull_s =
	SequencePullSupplier_i::create(orb, channel, num_sup_events, batch_size, "batch_pull_supplier",
				       "", "", sample_supply_batch_fn, sample_subscription_change_fn,
				       &evs, constraint_expr, millisecs, delay_millisecs, verbose);
      if (! b_pull_s) {
	cerr << "Constructing batch_pull_supplier failed" << endl;
	goto error_return; // failed to create a client
      }
      my_b_pull_s = b_pull_s->_this();
      b_pull_s->_remove_ref(); // enables POA cleanup of servant
    }

    // (4) Tell POA we are ready to go
    if (verbose) cout << "Activating POA manager" << endl;
    PortableServer::POAManager_var pman = poa->the_POAManager();
    pman->activate();

    // (5) Connect clients to their proxies -- consumers first
    if (verbose) cout << "Connecting consumer clients to proxies" << endl;

    if (USE_L_PUSH_C) {
      if (l_push_c->connect()) {
	cerr << "Connecting legacy_push_consumer failed" << endl;
	goto error_return;
      }
    }
    if (USE_A_PUSH_C) {
      if (a_push_c->connect()) {
	cerr << "Connecting any_push_consumer failed" << endl;
	goto error_return;
      }
    }
    if (USE_S_PUSH_C) {
      if (s_push_c->connect()) {
	cerr << "Connecting struct_push_consumer failed" << endl;
	goto error_return;
      }
    }
    if (USE_B_PUSH_C) {
      if (b_push_c->connect()) {
	cerr << "Connecting batch_push_consumer failed" << endl;
	goto error_return;
      }
    }

    if (USE_L_PULL_C) {
      if (l_pull_c->connect()) {
	cerr << "Connecting legacy_pull_consumer failed" << endl;
	goto error_return;
      }
    }
    if (USE_A_PULL_C) {
      if (a_pull_c->connect()) {
	cerr << "Connecting any_pull_consumer failed" << endl;
	goto error_return;
      }
    }
    if (USE_S_PULL_C) {
      if (s_pull_c->connect()) {
	cerr << "Connecting struct_pull_consumer failed" << endl;
	goto error_return;
      }
    }
    if (USE_B_PULL_C) {
      if (b_pull_c->connect()) {
	cerr << "Connecting batch_pull_consumer failed" << endl;
	goto error_return;
      }
    }

    if (verbose) cout << "Connecting supplier clients to proxies" << endl;

    if (USE_L_PUSH_S) {
      if (l_push_s->connect()) {
	cerr << "Connecting legacy_push_supplier failed" << endl;
	goto error_return;
      }
    }
    if (USE_A_PUSH_S) {
      if (a_push_s->connect()) {
	cerr << "Connecting any_push_supplier failed" << endl;
	goto error_return;
      }
    }
    if (USE_S_PUSH_S) {
      if (s_push_s->connect()) {
	cerr << "Connecting struct_push_supplier failed" << endl;
	goto error_return;
      }
    }
    if (USE_B_PUSH_S) {
      if (b_push_s->connect()) {
	cerr << "Connecting batch_push_supplier failed" << endl;
	goto error_return;
      }
    }

    if (USE_L_PULL_S) {
      if (l_pull_s->connect()) {
	cerr << "Connecting legacy_pull_supplier failed" << endl;
	goto error_return;
      }
    }
    if (USE_A_PULL_S) {
      if (a_pull_s->connect()) {
	cerr << "Connecting any_pull_supplier failed" << endl;
	goto error_return;
      }
    }
    if (USE_S_PULL_S) {
      if (s_pull_s->connect()) {
	cerr << "Connecting struct_pull_supplier failed" << endl;
	goto error_return;
      }
    }
    if (USE_B_PULL_S) {
      if (b_pull_s->connect()) {
	cerr << "Connecting batch_pull_supplier failed" << endl;
	goto error_return;
      }
    }

    if (verbose) cout << "Waiting for all 16 clients to finish" << endl;

    CORBA::Boolean l_push_c_err = 0, l_pull_c_err = 0, l_push_s_err = 0, l_pull_s_err = 0;
    CORBA::Boolean a_push_c_err = 0, a_pull_c_err = 0, a_push_s_err = 0, a_pull_s_err = 0;
    CORBA::Boolean s_push_c_err = 0, s_pull_c_err = 0, s_push_s_err = 0, s_pull_s_err = 0;
    CORBA::Boolean b_push_c_err = 0, b_pull_c_err = 0, b_push_s_err = 0, b_pull_s_err = 0;

    if (USE_L_PUSH_C) {
      l_push_c_err = l_push_c->wait_done();
    }
    if (USE_A_PUSH_C) {
      a_push_c_err = a_push_c->wait_done();
    }
    if (USE_S_PUSH_C) {
      s_push_c_err = s_push_c->wait_done();
    }
    if (USE_B_PUSH_C) {
      b_push_c_err = b_push_c->wait_done();
    }

    if (USE_L_PULL_C) {
      l_pull_c_err = l_pull_c->wait_done();
    }
    if (USE_A_PULL_C) {
      a_pull_c_err = a_pull_c->wait_done();
    }
    if (USE_S_PULL_C) {
      s_pull_c_err = s_pull_c->wait_done();
    }
    if (USE_B_PULL_C) {
      b_pull_c_err = b_pull_c->wait_done();
    }

    if (USE_L_PUSH_S) {
      l_push_s_err = l_push_s->wait_done();
    }
    if (USE_A_PUSH_S) {
      a_push_s_err = a_push_s->wait_done();
    }
    if (USE_S_PUSH_S) {
      s_push_s_err = s_push_s->wait_done();
    }
    if (USE_B_PUSH_S) {
      b_push_s_err = b_push_s->wait_done();
    }

    if (USE_L_PULL_S) {
      l_pull_s_err = l_pull_s->wait_done();
    }
    if (USE_A_PULL_S) {
      a_pull_s_err = a_pull_s->wait_done();
    }
    if (USE_S_PULL_S) {
      s_pull_s_err = s_pull_s->wait_done();
    }
    if (USE_B_PULL_S) {
      b_pull_s_err = b_pull_s->wait_done();
    }

    CORBA::Boolean con_com_err =
      l_push_c_err ||
      a_push_c_err ||
      s_push_c_err ||
      b_push_c_err ||
      l_pull_c_err ||
      a_pull_c_err ||
      s_pull_c_err ||
      b_pull_c_err;
    CORBA::Boolean sup_com_err =
      l_push_s_err ||
      a_push_s_err ||
      s_push_s_err ||
      b_push_s_err ||
      l_pull_s_err ||
      a_pull_s_err ||
      s_pull_s_err ||
      b_pull_s_err;
    if (sup_com_err && (num_sup_events > 0)) {
      if (verbose) cout << "Supplier communication error -- probably did not supply desired # of events" << endl;
    }
    if (con_com_err && (num_con_events > 0)) {
      if (verbose) cout << "Consumer communication error -- probably did not consume desired # of events" << endl;
    }

    if (verbose) cout << "Cleaning up" << endl;

    if (USE_L_PUSH_C) {
      if (!l_push_c_err) { l_push_c->cleanup(); }
    }
    if (USE_A_PUSH_C) {
      if (!a_push_c_err) { a_push_c->cleanup(); }
    }
    if (USE_S_PUSH_C) {
      if (!s_push_c_err) { s_push_c->cleanup(); }
    }
    if (USE_B_PUSH_C) {
      if (!b_push_c_err) { b_push_c->cleanup(); }
    }

    if (USE_L_PULL_C) {
      if (!l_pull_c_err) { l_pull_c->cleanup(); }
    }
    if (USE_A_PULL_C) {
      if (!a_pull_c_err) { a_pull_c->cleanup(); }
    }
    if (USE_S_PULL_C) {
      if (!s_pull_c_err) { s_pull_c->cleanup(); }
    }
    if (USE_B_PULL_C) {
      if (!b_pull_c_err) { b_pull_c->cleanup(); }
    }

    if (USE_L_PUSH_S) {
      if (!l_push_s_err) { l_push_s->cleanup(); }
    }
    if (USE_A_PUSH_S) {
      if (!a_push_s_err) { a_push_s->cleanup(); }
    }
    if (USE_S_PUSH_S) {
      if (!s_push_s_err) { s_push_s->cleanup(); }
    }
    if (USE_B_PUSH_S) {
      if (!b_push_s_err) { b_push_s->cleanup(); }
    }

    if (USE_L_PULL_S) {
      if (!l_pull_s_err) { l_pull_s->cleanup(); }
    }
    if (USE_A_PULL_S) {
      if (!a_pull_s_err) { a_pull_s->cleanup(); }
    }
    if (USE_S_PULL_S) {
      if (!s_pull_s_err) { s_pull_s->cleanup(); }
    }
    if (USE_B_PULL_S) {
      if (!b_pull_s_err) { b_pull_s->cleanup(); }
    }

    goto normal_return;
  }
  catch(CORBA::SystemException&) {                     
    cerr << "main caught CORBA::SystemException." << endl;  
  }                                                    
  catch(CORBA::Exception&) {                           
    cerr << "main caught CORBA::Exception." << endl;        
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
 error_return:
  cerr << "QUITTING due to error" << endl;
  // orb->destroy();
  return -1;
 normal_return:
  // orb->destroy();
  return 0;
}

