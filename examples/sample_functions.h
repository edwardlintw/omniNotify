// -*- Mode: C++; -*-

// -------------------------------------------------------------- //
// This file contains SAMPLE user-defined functions for use       //
// with the client classes in sample_clients.h and                // 
// legacy_clients.h                                               //
// -------------------------------------------------------------- //

#ifndef _SAMPLE_FUNCTIONS_H_
#define _SAMPLE_FUNCTIONS_H_

#include "CosNotifyShorthands.h"

// ------------------------------------------------------------- //
// The user-defined function types -- decls of examples follow
// ------------------------------------------------------------- //

// 3 kinds of 'consume' functions
typedef void
consume_any_fn        (const CORBA::Any& event,
		       const char* obj_name,
		       const CORBA::ULong event_num,
		       CORBA::Boolean verbose
		       );
typedef void
consume_structured_fn (const CosN::StructuredEvent& event,
		       const char* obj_name,
		       const CORBA::ULong event_num,
		       CORBA::Boolean verbose
		       );
typedef void
consume_batch_fn      (const CosN::EventBatch& events,
		       const char* obj_name,
		       const CORBA::ULong event_num,
		       const CORBA::ULong batch_num,
		       CORBA::Boolean verbose
		       );

// 3 kinds of 'supply' functions:
//    either fill in event(s) and return TRUE [event(s) supplied)]
//    or return FALSE [event(s) not supplied]
typedef CORBA::Boolean
supply_any_fn         (CORBA::Any& event,
		       const char* obj_name,
		       const CORBA::ULong event_num,
		       CORBA::Boolean verbose
		       );
typedef CORBA::Boolean
supply_structured_fn  (CosN::StructuredEvent& event,
		       const char* obj_name,
		       const CORBA::ULong event_num,
		       CORBA::Boolean verbose
		       );
typedef CORBA::Boolean
supply_batch_fn       (CosN::EventBatch& events,
		       const char* obj_name,
		       const CORBA::ULong batch_size,
		       const CORBA::ULong event_num,
		       const CORBA::ULong batch_num,
		       CORBA::Boolean verbose
		       );

// 1 kind of 'change' function
typedef void
type_change_fn        (const CosN::EventTypeSeq& added,
		       const CosN::EventTypeSeq& deled,
		       const char* obj_name,
		       CORBA::ULong num_changes,
		       CORBA::Boolean verbose
		       );

///////////////////////////////////////////////////////////////////
//  DECLARATIONS OF USER-DEFINED FUNCTION EXAMPLES
//  See sample_functions.cc for implementations.
///////////////////////////////////////////////////////////////////

// This pair of examples uses a CORBA::ULong inside the CORBA::Any event kind

CORBA::Boolean
sample_supply_any_fn (CORBA::Any& event,
		      const char* obj_name,
		      const CORBA::ULong event_num,
		      CORBA::Boolean verbose
		      );
void
sample_consume_any_fn (const CORBA::Any& event,
		       const char* obj_name,
		       const CORBA::ULong event_num,
		       CORBA::Boolean verbose
		       );

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
			      );

CORBA::Boolean
sample_supply_structured_fn (CosN::StructuredEvent& event,
			     const char* obj_name,
			     const CORBA::ULong event_num,
			     CORBA::Boolean verbose
			     );

// This pair of examples also uses type Stock::Ticker.  It supplies
// 5 events per batch which is an interesting # releative
// to the default MaximumBatchSize of 8

CORBA::Boolean
sample_supply_batch_fn (CosN::EventBatch& events,
			const char* obj_name,
			const CORBA::ULong batch_size,
			const CORBA::ULong event_num,
			const CORBA::ULong batch_num,
			CORBA::Boolean verbose
			);
void
sample_consume_batch_fn (const CosN::EventBatch& events,
			 const char* obj_name,
			 const CORBA::ULong event_num,
			 const CORBA::ULong batch_num,
			 CORBA::Boolean verbose
			 );

// This pair uses 512 different event types

void
consume512_structured_fn (const CosN::StructuredEvent& event,
			  const char* obj_name,
			  const CORBA::ULong event_num,
			  CORBA::Boolean verbose
			  );

CORBA::Boolean
supply512_structured_fn (CosN::StructuredEvent& event,
			 const char* obj_name,
			 const CORBA::ULong event_num,
			 CORBA::Boolean verbose
			 );

// Two examples that describe the contents 
// of an offer_change or subscription_change msg

void
sample_offer_change_fn(const CosN::EventTypeSeq& added,
		       const CosN::EventTypeSeq& deled,
		       const char* obj_name,
		       CORBA::ULong num_changes,
		       CORBA::Boolean verbose
		       );

void
sample_subscription_change_fn(const CosN::EventTypeSeq& added,
			      const CosN::EventTypeSeq& deled,
			      const char* obj_name,
			      CORBA::ULong num_changes,
			      CORBA::Boolean verbose
			      );
#endif
