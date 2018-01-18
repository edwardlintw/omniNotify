#!/usr/bin/env python

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

from omniORB import CORBA
import sys
import CosNaming
import AttNotification
import CosNotifyFilter
if (len(sys.argv) != 2):
    print "\nUsage:", sys.argv[0], "<ior-file>\n"
    sys.exit(-1)
try:
    ior_file = open(sys.argv[1], "r")
except:
    print "\nCould not open ior file", sys.argv[1],"\n"
    sys.exit(-1)
ior = ior_file.read()
ior_file.close()
orb = CORBA.ORB_init()
poa = orb.resolve_initial_references("RootPOA")
x = orb.string_to_object(ior)
if (x == None):
    print "\nIOR file", sys.argv[1], "does not specify a valid object\n"
    sys.exit(-1)
fadmin = x._narrow(CosNotifyFilter.FilterAdmin)
if (fadmin == None):
    print "\nIOR file", sys.argv[1], "does not specify a CosNotifyFilter.FilterAdmin object\n"
    sys.exit(-1)
interact = x._narrow(AttNotification.Interactive)
if (interact == None):
    print "\nIOR file", sys.argv[1], "does not specify an AttN::Interactive object\n"
    sys.exit(-1)
print "\nFound object with FilterAdmin, name =" ,interact.my_name() , "\n"
ids = fadmin.get_all_filters()
print "\n# Filters =", len(ids), "\n"
for id in ids:
    print "\nGetting filter with FID", id, "\n"
    try:
        filter = fadmin.get_filter(id)
        print "Getting IOR of filter object\n"
        fior = orb.object_to_string(filter)
        print "Filter IOR = ", fior , "\n"
    except:
        print "Caught exception getting filter\n"

print "\nDONE\n"
