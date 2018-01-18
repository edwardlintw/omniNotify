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
import CosNaming
import AttNotification
orb = CORBA.ORB_init()
poa = orb.resolve_initial_references("RootPOA")
# ns = orb.resolve_initial_references("NameService")
ns = orb.string_to_object("corbaloc::bombur.research.att.com:9999/NameService")
rootCon = ns._narrow(CosNaming.NamingContext)
name = [CosNaming.NameComponent("EventChannel","EventChannel")]
echannel_ref = rootCon.resolve(name)
target = echannel_ref._narrow(AttNotification.Interactive)
# (I also tried AttNotification.EventChannel)
result, success, target_change, next_target = target.do_command("help")
print result
if (target_change):
    target = next_target
result, success, target_change, next_target = target.do_command("up")
print result
if (target_change):
    target = next_target

result, success, target_change, next_target = target.do_command("help")
print result
if (target_change):
    target = next_target

result, success, target_change, next_target = target.do_command("up")
print result
if (target_change):
    target = next_target

result, success, target_change, next_target = target.do_command("help")
print result
if (target_change):
    target = next_target

result, success, target_change, next_target = target.do_command("cleanup all")
print result
if (target_change):
    target = next_target
