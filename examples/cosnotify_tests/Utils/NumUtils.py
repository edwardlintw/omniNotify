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

# module NumUtils

from omniORB import CORBA

def any2number(a):
    """a should be a CORBA::Any value
    attempt to convert it to a number; return None if cannot be done
    """
    for tc in [ CORBA._tc_short, CORBA._tc_ushort, CORBA._tc_long, CORBA._tc_ulong,
		CORBA._tc_float, CORBA._tc_double, CORBA._tc_longlong, CORBA._tc_ulonglong ]:
	v = a.value(tc)
	if (v != None):
            if (tc == CORBA._tc_double):
                # if very close to a nice number of the form x.y,
                # round to nearest tenth (e.g., 5.500000434 => 5.5)
                # this works around the problem that Linux<->Sun
                # double interchange seems to introduce small errors
                vdiff = (v*10) - int(v*10)
                if (vdiff < 0):
                    vdiff = - vdiff
                if ( vdiff < .01):
                    vnew = int(v*10)/10.0
                    if ( (vnew - int(vnew)) < .01):
                        vnew = int(vnew)
                    if (v != vnew):
                        print "XXX Changing ", v, " to ", vnew
                        v = vnew
	    return v
    return None
