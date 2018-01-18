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

# Module Sup_StockBondTicker

# Notes: sets up a supplier of CosNotification::StructuredEvent
# containing both Stock::Ticker and Bond::Ticker events.

# The supplier generates stock price events for 5 stocks (A, B, C, D, E)
# and bond price events for 5 bonds (F, G, H, I, J). Each value
# is integral and follows a random walk, changing its value by -1, 0,
# or +1 each time an event for that stock or bond is supplied.
# There are Name and Value fields, thus you can use constraints such as 
#    $Name == 'A' and $Value > 40
# when creating filters.  For event types, you can specify
# *::Ticker to receive both kinds of events, or specify just
# Stock::Ticker or just Bond::Ticker.
# The event_num is placed in the remainder_of_body.

# The quotes cycle through all 10 stocks/bonds so $Name == 'A' will match
# 1/10 events, while $domain_name == 'Stock' will match 5/10 events

from omniORB import CORBA
import CosNotification
from Utils import CliUtils
import random

# public state
Tdoc = "Supply CosNotification::StructuredEvent vals containing Stock::Ticker and Bond::Ticker events"
Tname = "Sup_StockBondTicker"

sup_offr = [ CosNotification.EventType("Stock","Ticker"), CosNotification.EventType("Bond","Ticker") ] 
sup_constraints = None
sup_chfn = None
sup_msec = 0

# maps indexed by idx arg
sup_total = {} 

# private state
# Names and initial values for the 5 stocks and 5 bonds
__sample_stockbond_nm = [ "A", "B", "C", "D", "E", "F", "G", "H", "I", "J" ]
__sample_stockbond_val = [ 26, 45, 100, 65, 87, 250, 120, 33, 102, 15 ]

def sup_evfn(objnm, idx, bsize, num_events, num_batches, verbose):
    "Supply CosNotification::StructuredEvent vals w/ Stock::Ticker,Bond::Ticker events"
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
	counter = sup_total[idx] % 10
	sup_total[idx] += 1
	d = "Bond"
	if (counter < 5):
	    d = "Stock"
	name = __sample_stockbond_nm[counter]
	val = __sample_stockbond_val[counter]
	# Update value -- we are nice and do not let the value drop below 5
	if (val <= 5):
	    __sample_stockbond_val[counter] += random.choice([0, 1])
	else:
	    __sample_stockbond_val[counter] += random.choice([-1, 0, 1])
	# build an event
	etype = CosNotification.EventType(d, "Ticker")
	ename = "foo"
	fhdr = CosNotification.FixedEventHeader(etype, ename)
	vhdr = [ ]
	hdr = CosNotification.EventHeader(fhdr, vhdr)
	f1pair = CosNotification.Property("Name", CORBA.Any(CORBA._tc_string, name))
	f2pair = CosNotification.Property("Value", CORBA.Any(CORBA._tc_ulong, val))
	fdata = [ f1pair, f2pair ]
	rob = CORBA.Any(CORBA._tc_ulong, num_events)
	ev = CosNotification.StructuredEvent(hdr, fdata, rob)
	num_events += 1
	if (verbose > 0):
	    print objnm, ": sup_evfn: event with type", d + "::Ticker, Company", name, "Value", val
	batch.append(ev)
    if (verbose > 0):
	print objnm, ": sup_evfn: returning event batch of length", len(batch)
	if (num_batches != 0):
	    print objnm, ": ------------------------------------------------------------"
    return batch
