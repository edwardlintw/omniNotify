// -*- Mode: C++; -*-

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
using namespace std;
#include "sample_functions.h"

///////////////////////////////////////////////////////////////////
//             USER-DEFINED FUNCTION EXAMPLES                    //
///////////////////////////////////////////////////////////////////

// This pair of examples uses a CORBA::ULong inside the CORBA::Any event kind

CORBA::Boolean
sample_supply_any_fn (CORBA::Any& event,
		      const char* obj_name,
		      const CORBA::ULong event_num,
		      CORBA::Boolean verbose
		      )
{
  event <<= event_num;
  if (verbose) cout << "Supplied event # " << event_num << endl;
  return 1; // OK
}

void
sample_consume_any_fn (const CORBA::Any& event,
		       const char* obj_name,
		       const CORBA::ULong event_num,
		       CORBA::Boolean verbose
		       )
{
  CORBA::ULong ul;
  if (event >>= ul) {
    if (verbose) cout << obj_name << ": event data = " << ul << ", event count = " << event_num << endl;
  } else {
    if (verbose) cout << obj_name << ": event data not known (not a ULong), event count = " << event_num << endl;
  }
}

// This pair of examples uses types Stock::Ticker and Bond::Ticker.
// The supplier generates stock price events for 5 stocks (A, B, C, D, E)
// and bond price events for 5 bonds (F, G, H, I, J). Each value
// is integral and follows a random walk, changing its value by -1, 0,
// or +1 each time an event for that stock or bond is supplied.
// There are Name and Value fields, thus you can use constraints such as 
//    $Name == 'A' and $Value > 40
// when creating filters.  For event types, you can specify
// *::Ticker to receive both kinds of events, or specify just
// Stock::Ticker or just Bond::Ticker.
// The event_num is placed in the remainder_of_body.

void
sample_consume_structured_fn (const CosN::StructuredEvent& event,
			      const char* obj_name,
			      const CORBA::ULong event_num,
			      CORBA::Boolean verbose
			      )
{
  if (verbose) {
    cout << setw(10) << obj_name << " : " <<
      setw(4) << event_num << " : " <<
      setw(5) << event.header.fixed_header.event_type.domain_name << "::" <<
      event.header.fixed_header.event_type.type_name << " -- ";
    const char* stock_or_bond_name = "";
    CORBA::ULong value = 0;
    if (((strcmp(event.header.fixed_header.event_type.domain_name, "Stock") == 0) ||
	 (strcmp(event.header.fixed_header.event_type.domain_name, "Bond") == 0)) &&
	(strcmp(event.header.fixed_header.event_type.type_name, "Ticker") == 0) &&
	(event.filterable_data.length() > 1) &&
	(event.filterable_data[0].value >>= stock_or_bond_name) &&
	(event.filterable_data[1].value >>= value)) {
      cout << "Name: " << stock_or_bond_name << " Value: " << setw(4) << value << " Quote# "; 
      CORBA::ULong l = 0;
      if (event.remainder_of_body >>= l) {
	cout << setw(4) << l << endl;
      } else {
	cout << "????" << endl;
      }
    } else {
      cout << "(not a Stock or Bond quote -- ignoring)" << endl;
    }
  }
}

// Names and initial values for the 5 stocks and 5 bonds
static const char*   sample_stockbond_nm[10] = 
{ "A", "B", "C", "D", "E", "F", "G", "H", "I", "J" };
static unsigned long sample_stockbond_val[10] =
{ 26, 45, 100, 65, 87, 250, 120, 33, 102, 15 };

CORBA::Boolean
sample_supply_structured_fn (CosN::StructuredEvent& event,
			     const char* obj_name,
			     const CORBA::ULong event_num,
			     CORBA::Boolean verbose
			     )
{
  int i = rand() % 10;
  const char* c = sample_stockbond_nm[i];
  const char* d = "";
  if (i < 5)
    d = "Stock";
  else
    d = "Bond";
  // we are nice and do not let values drop below 5
  if (sample_stockbond_val[i] <= 5) {
    sample_stockbond_val[i] = sample_stockbond_val[i] + (rand() % 2);
  } else {
    sample_stockbond_val[i] = sample_stockbond_val[i] + (rand() % 3) - 1;
  }
  event.header.fixed_header.event_type.domain_name = 
    CORBA::string_dup(d);
  event.header.fixed_header.event_type.type_name   = 
    CORBA::string_dup("Ticker");
  event.header.variable_header.length( 0 );
  event.filterable_data.length(2);
  event.filterable_data[0].name = (const char*) "Name";
  event.filterable_data[0].value <<= (const char*)c;
  event.filterable_data[1].name = (const char*) "Value";
  event.filterable_data[1].value <<= (CORBA::ULong) sample_stockbond_val[i];
  event.remainder_of_body <<= (CORBA::ULong) event_num;
  if (verbose) cout << "Supplied event # " << event_num << endl;
  return 1;
}

// This pair of examples also uses type Stock::Ticker.  It supplies
// batch_size events per batch.  The default is 5,
// which is an interesting # relative to the default
// MaximumBatchSize of 8.  Override with -b option.

CORBA::Boolean
sample_supply_batch_fn (CosN::EventBatch& events,
			const char* obj_name,
			const CORBA::ULong batch_size,
			const CORBA::ULong event_num,
			const CORBA::ULong batch_num,
			CORBA::Boolean verbose
			)
{
  CORBA::ULong evnum = event_num;
  // We re-use the singleton example function
  if (verbose) cout << "--------- " << obj_name << " Supplying Event Batch " <<
		 batch_num << " -------------" << endl;
  events.length(batch_size);
  for (unsigned int i = 0; i < batch_size; i++) {
    sample_supply_structured_fn(events[i], obj_name, evnum++, verbose);
  }
  return 1;
}

void
sample_consume_batch_fn (const CosN::EventBatch& events,
			 const char* obj_name,
			 const CORBA::ULong event_num,
			 const CORBA::ULong batch_num,
			 CORBA::Boolean verbose
			 )
{
  // We re-use the singleton example function
  CORBA::ULong evnum = (event_num - events.length()) + 1;
  if (verbose) cout << "--------- " << obj_name << " Got Event Batch " <<
		 batch_num << " -------------" << endl;
  for (unsigned int i = 0; i < events.length(); i++) {
    sample_consume_structured_fn(events[i], "", evnum++, verbose);
  }
}

/* This pair uses 512 different event types */

void
consume512_structured_fn (const CosN::StructuredEvent& event,
			  const char* obj_name,
			  const CORBA::ULong event_num,
			  CORBA::Boolean verbose
			  )
{
  if (verbose) {
    cout << setw(10) << obj_name << " : " <<
      setw(4) << event_num << " : " <<
      setw(5) << event.header.fixed_header.event_type.domain_name << "::" <<
      event.header.fixed_header.event_type.type_name << " -- ";
    CORBA::ULong l = 0;
    if (event.remainder_of_body >>= l) {
      cout << setw(4) << l << endl;
    } else {
      cout << "????" << endl;
    }
  }
}

CORBA::Boolean
supply512_structured_fn (CosN::StructuredEvent& event,
			 const char* obj_name,
			 const CORBA::ULong event_num,
			 CORBA::Boolean verbose
			 )
{
  char buf[1024];
  int i = rand() % 512;
  sprintf(buf, "Type_%03d", i);
  event.header.fixed_header.event_type.domain_name = 
    CORBA::string_dup("Test512");
  event.header.fixed_header.event_type.type_name   = 
    CORBA::string_dup(buf);
  event.header.variable_header.length( 0 );
  event.filterable_data.length(1);
  event.filterable_data[0].name = (const char*) "TypeNum";
  event.filterable_data[0].value <<= (CORBA::ULong) i;
  event.remainder_of_body <<= (CORBA::ULong) event_num;
  if (verbose) cout << "Supplied event # " << event_num << endl;
  return 1;
}

// Helper functions

ostream& operator<< (ostream& out, const CosN::EventType & t) {
  const char* dname = (t.domain_name ? (const char*)t.domain_name : "*");
  const char* tname = (t.type_name ? (const char*)t.type_name : "*");
  return out << dname << "::" << tname;
}

void
sample_describe_change_fn(const CosN::EventTypeSeq& added,
			  const CosN::EventTypeSeq& deled)
{
  CORBA::ULong indx;
  CosN::EventType type;
  for (indx = 0; indx < added.length(); indx++) {
    cout << "\t+ " << added[indx] << endl;
  }
  for (indx = 0; indx < deled.length(); indx++) {
    cout << "\t- " << deled[indx] << endl;
  }
}

// Two examples that describe the contents 
// of an offer_change or subscription_change msg

void
sample_offer_change_fn(const CosN::EventTypeSeq& added,
		       const CosN::EventTypeSeq& deled,
		       const char* obj_name,
		       CORBA::ULong num_changes,
		       CORBA::Boolean verbose
		       )
{
  if (verbose) {
    cout << "Object " << obj_name <<
      " received offer_change msg [# " << num_changes << "]:" << endl;
    sample_describe_change_fn(added, deled);
  }
}

void
sample_subscription_change_fn(const CosN::EventTypeSeq& added,
			      const CosN::EventTypeSeq& deled,
			      const char* obj_name,
			      CORBA::ULong num_changes,
			      CORBA::Boolean verbose
		       )
{
  if (verbose) {
    cout << "Object " << obj_name << 
      " received subscription_change msg [# " << num_changes << "]:" << endl;
    sample_describe_change_fn(added, deled);
  }
}

