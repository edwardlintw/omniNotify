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

import sys
import RunTests

def usage():
    print "\nrun_tests.py [-n] [-r <region>] [-c <name>]"
    print "    -n : do not run, just print the test names"
    print "    -r <region> : specify either 0 (run all regions) or"
    print "                  one of the 10 regions (numbered 1 to 10)\n"
    print "    -c <name>   : specify the name to use to lookup the ChannelFactory"

cfactory_name = "ChannelFactory"
run_region = int(0)
norun = 0
i = 1
while (i < len(sys.argv)):
    if (sys.argv[i] == "-n"):
	norun = 1
	i += 1
	continue
    if (sys.argv[i] == "-c"):
        i += 1
        if (i < len(sys.argv)):
            cfactory_name = sys.argv[i]
            i += 1
            continue
        print "-c must be followed by a channel factory name"
        usage()
        sys.exit(-1)
    if (sys.argv[i] == "-r"):
	i += 1
	if (i < len(sys.argv)):
	    try:
		run_region = int(sys.argv[i])
	    except:
		print "-r must be followed by an integer (", sys.argv[i], " is not an int)"
		usage()
		sys.exit(-1)
	else:
	    print "-r must be followed by an integer"
	    usage()
	    sys.exit(-1)
	i += 1
	continue
    print "invalid argument:", sys.argv[i]
    usage()
    sys.exit(-1)

RunTests.run(run_region, norun, cfactory_name)

