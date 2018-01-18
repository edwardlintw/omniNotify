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

##############################################################
# Module PropUtils
##############################################################

from omniORB import CORBA
import CosNotification,TimeBase

def print_property_or_properror(p):
    nm = p.name
    # if there is an error code, print it
    try:
	if (p.code == CosNotification.BAD_TYPE):
	    print "BAD_TYPE used for param", nm
	    return # for bad type, no further info supplied
	elif (p.code == CosNotification.BAD_VALUE):
	    print "BAD_VALUE used for param", nm
	elif (p.code == CosNotification.BAD_PROPERTY):
	    print "BAD_PROPERTY for param", nm
	    return # for bad property, no further info supplied
	elif (p.code == CosNotification.UNAVAILABLE_PROPERTY):
	    print "UNAVAILABLE_PROPERTY for param", nm
	    return # for unavailable property, no further info supplied
	elif (p.code == CosNotification.UNSUPPORTED_PROPERTY):
	    print "UNSUPPORTED_PROPERTY for param", nm
	    return # for unsupported property, no further info supplied
	elif (p.code == CosNotification.UNSUPPORTED_VALUE):
	    print "UNSUPPORTED_VALUE for param", nm
	elif (p.code == CosNotification.UNAVAILABLE_VALUE):
	    print "UNAVAILABLE_VALUE for param", nm
    except:
	pass
    # if there a value, print it
    try:
	aval = p.value
	val = aval.value(aval.typecode())
	try:
	    val_lo = long(val.low)
	    val_hi = long(val.high)
	    shiftval = long(32)
	    tm = val_lo + ( val_hi << shiftval)
	    print "Name:", nm, "Val:", tm
	except:
	    print "Name:", nm, "Val:", val
    except:
	pass
    # if there is a range or available_range, print it
    lo = None
    hi = None
    try:
	lo = p.range.low_val
	hi = p.range.high_val
    except:
	pass
    if (lo is None):
	try:
	    lo = p.available_range.low_val
	    hi = p.available_range.high_val
	except:
	    pass
    if (lo != None):
	loval = lo.value(lo.typecode())
	hival = hi.value(hi.typecode())
	try:
	    loval_lo = long(loval.low)
	    loval_hi = long(loval.high)
	    hival_lo = long(hival.low)
	    hival_hi = long(hival.high)
	    shiftval = long(32)
	    lotm = loval_lo + ( loval_hi << shiftval)
	    hitm = hival_lo + ( hival_hi << shiftval)
	    print "Available Range for", nm, "Min:", lotm, "Max:", hitm
	except:
	    print "Available Range for", nm, "Min:", loval, "Max:", hival
    # done
    return

def print_propseq(pseq):
    for p in pseq:
	print_property_or_properror(p)

