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

# Module Con_StockBondTicker_tnameeq

# Notes: sets up a consumer for CosNotification::StructuredEvent that contain
# Stock::Ticker and Bond::Ticker events.  Uses constraint
#   $domain_name == 'Stock'
# which should match 1/2 of all events supplied by Sup_StockBondTicker

from omniORB import CORBA
import CosNotification
from Utils import CliUtils, StockBondUtils

# public state
Tdoc = "Consume CosNotification::StructuredEvent vals containing Stock::Ticker events"
Tname = "Con_StockBondTicker_dnameeq"

_constraint1 = [ [ CosNotification.EventType("*","Ticker") ],  "$domain_name == 'Stock'" ]
con_constraints = [ _constraint1 ]
con_chfn = None
con_msec = 0
con_pcnt = .50

# maps indexed by idx arg
con_total = {}
con_valid = {}

def con_evfn(batch, objnm, idx, num_events, num_batches, verbose):
    "consume event, verify it contains the appropriate value"
    # initialize consume stats for idx, if necessary
    if (not con_valid.has_key(idx)):
	con_valid[idx] = 0
    if (not con_total.has_key(idx)):
	con_total[idx] = 0
    # the rest of the consume method
    if ((verbose > 0) and (num_batches != 0)):
	print objnm, ": ------------------------------------------------------------"
	print objnm, ": con_evfn received batch #", num_batches
    # process like a batch regardless
    for ev in batch:
	con_total[idx] += 1
	if ((ev.header.fixed_header.event_type.domain_name != "Stock") or
	    (ev.header.fixed_header.event_type.type_name != "Ticker")):
	    if (verbose > 0):
		print objnm, ": con_evfn: unexpected value consumed (not a Stock::Ticker event)"
	    continue
	con_valid[idx] += 1
	if (verbose > 0):
	    StockBondUtils.describe_event(objnm, ev, con_total[idx])
    if (verbose > 0):
	print objnm, ": con_evfn: Valid events consumed now = ", con_valid[idx]
	if (num_batches != 0):
	    print objnm, ": ------------------------------------------------------------"
