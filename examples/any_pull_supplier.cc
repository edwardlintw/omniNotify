// This application implements a pull supplier which uses the new
// Notification Service interfaces to supply events of type CORBA::Any.
// (As opposed to using the CosEvents interfaces.)
//
// We have chosen to group code examples according to their
// function.  This file simply puts together various examples
// into a working program.  See the README file for a list of
// command-line options supported.  See the READING_THE_CODE
// file for an explanation of the example code.

#define CLIENT_IMPL_CLASS  PullSupplier_i
#define CLIENT_CLASS_VAR   CosNotifyComm::PullSupplier_var
#define CLIENT_NAME        "any_pull_supplier"

#define SUPPLY_OR_CONSUME_FN sample_supply_any_fn
#define CHANGE_FN            sample_subscription_change_fn
//#define CHANGE_FN            0

#include "main_program.h"
