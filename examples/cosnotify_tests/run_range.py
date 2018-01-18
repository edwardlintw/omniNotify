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

import string, sys, os

if (len(sys.argv) < 3):
    print "\nUsage: run_range.py <lo> <hi> [ common run_tests.py args ]\n"
    sys.exit(-1)

for x in range(int(sys.argv[1]), int(sys.argv[2]) + 1):
  cmdargs = [ "./run_tests.py", "-r", repr(x) ]
  cmdargs += sys.argv[3:]
  cmd = string.join(cmdargs, ' ')
  print '\n', cmd, '\n'
  os.system(cmd)
