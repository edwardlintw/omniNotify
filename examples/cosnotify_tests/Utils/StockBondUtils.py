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

# Module StockBondUtil

from omniORB import CORBA

def describe_event(objnm, ev, evnum):
    d = ev.header.fixed_header.event_type.domain_name
    t = ev.header.fixed_header.event_type.type_name
    n = ev.filterable_data[0].value.value(CORBA._tc_string)
    v = ev.filterable_data[1].value.value(CORBA._tc_ulong)
    q = ev.remainder_of_body.value(CORBA._tc_ulong)
    print objnm, ": %4d : %s::%s  --  Name: %s  Value: %d  Quote#: %d" % (evnum, d, t, n, v, q)

