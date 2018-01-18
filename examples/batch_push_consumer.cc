// This application implements a push consumer which consumes
// event batches (sequences of structured events).
//
// We have chosen to group code examples according to their
// function.  This file simply puts together various examples
// into a working program.  See the README file for a list of
// command-line options supported.  See the READING_THE_CODE
// file for an explanation of the example code.

#define CLIENT_IMPL_CLASS  SequencePushConsumer_i
#define CLIENT_CLASS_VAR   CosNotifyComm::SequencePushConsumer_var
#define CLIENT_NAME        "batch_push_consumer"

#define SUPPLY_OR_CONSUME_FN sample_consume_batch_fn
#define CHANGE_FN            sample_offer_change_fn
//#define CHANGE_FN            0

#include "main_program.h"
