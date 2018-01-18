#    Copyright (C) 2003 AT&T Laboratories -- Research
#
#    This file is part of a set of test programs distributed with
#    the omniNotify release (under the cosnotify_tests directory).
#
#    The test programs are free software; you can redistribute and/or modify
#    them under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    The tests are distributed in the hope that they will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this file; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
#    USA.

# Module Sup_ULEvent

# Notes: sets up a supplier of CosNotification::StructuredEvent
# containing ULTest::UL events, with a single field, $ul
# which has an unsigned long with the event #

# Some facts about the supplied values:
#   ul field ranges from 1 .. max_events
#   (so does remainder_of_body)

from omniORB import CORBA
import CosNotification
from Utils import CliUtils

# public state
Tdoc = "Supply CosNotification::StructuredEvent vals containing ULTest::UL events"
Tname = "Sup_ULEvent"

sup_offr = [ CosNotification.EventType("ULTest","UL") ] 
sup_constraints = None
sup_chfn = None
sup_msec = 0

# maps indexed by idx arg
sup_total = {} 

def sup_evfn(objnm, idx, bsize, num_events, num_batches, verbose):
    "Supply CosNotification::StructuredEvent vals containing ULTest::UL events"
    # initialize supply stats for idx, if necessary
    if (not sup_total.has_key(idx)):
	sup_total[idx] = 0
    # the rest of the supply method
    if ((verbose > 0) and (num_batches != 0)):
	print objnm, ": ------------------------------------------------------------"
	print objnm, ": sup_evfn supplying batch #", num_batches
    batch = [ ]
    while (len(batch) < bsize):
	if (verbose > 0):
	    print objnm, ": sup_evfn: supplying event #", num_events
	sup_total[idx] += 1
	# build an event
	etype = CosNotification.EventType("ULTest", "UL")
	ename = "foo"
	fhdr = CosNotification.FixedEventHeader(etype, ename)
	vhdr = [ ]
	hdr = CosNotification.EventHeader(fhdr, vhdr)
	f1pair = CosNotification.Property("ul",  CORBA.Any(CORBA._tc_ulong, num_events))
	fdata = [ f1pair ]
	rob = CORBA.Any(CORBA._tc_ulong, num_events)
	ev = CosNotification.StructuredEvent(hdr, fdata, rob)
	if (verbose > 0):
	    print objnm, ": sup_evfn: event with type ULTest::UL with $ul == ", num_events
	num_events += 1
	batch.append(ev)
    if (verbose > 0):
	print objnm, ": sup_evfn: returning event batch of length", len(batch)
	if (num_batches != 0):
	    print objnm, ": ------------------------------------------------------------"
    return batch
