// This application implements a pull supplier which consumes
// structured events.
//
// We have chosen to group code examples according to their
// function.  This file simply puts together various examples
// into a working program.  See the README file for a list of
// command-line options supported.  See the READING_THE_CODE
// file for an explanation of the example code.

#define CLIENT_IMPL_CLASS   StructuredPullSupplier_i
#define CLIENT_CLASS_VAR    CosNotifyComm::StructuredPullSupplier_var
#define CLIENT_NAME         "struct_pull_supplier"

#define SUPPLY_OR_CONSUME_FN sample_supply_structured_fn
#define CHANGE_FN            sample_subscription_change_fn
//#define CHANGE_FN            0

#include "main_program.h"
