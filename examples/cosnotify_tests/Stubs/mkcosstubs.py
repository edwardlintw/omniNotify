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

import os

idlfiles = ["TimeBase", "CosTime", "CosEventComm", "CosEventChannelAdmin", "CosTypedEventComm", "CosTypedEventChannelAdmin", "CosTimerEvent", "CosNotification", "CosNotifyComm", "CosNotifyFilter", "CosNotifyChannelAdmin", "CosTypedNotifyComm", "CosTypedNotifyChannelAdmin"]

# Assumes costnotify_tests can be found at <top>/src/services/omniNotify/examples/cosnotify_tests
d = "../../../../../../idl"    # <top>/idl

for x in idlfiles:
    y = x + ".idl"
    print x
    cmd = "omniidl -bpython -I%s -I%s/COS -DNOLONGLONG %s/COS/%s" % (d, d, d, y)
    print cmd
    os.system(cmd)

