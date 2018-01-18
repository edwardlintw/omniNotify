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

# Module StructPushSup defines class StructPushSup.C + constructor SupPushSup.Create

import CosNotifyChannelAdmin # import stubs
import CosNotifyComm__POA # import skeletons
import CliMixin, SupMixin, PushSupMixin
from Utils import CliUtils

# public module state
Tdoc = "Using CosNotifyComm::StructuredPushSupplier Client"
Tname = "StructPushSup"

class C(CosNotifyComm__POA.StructuredPushSupplier, CliMixin.C, SupMixin.C, PushSupMixin.C):
    "push supplier of CosNotification::StructuredEvent"

    # IDL methods
    def disconnect_structured_push_supplier(self):
        "channel informs of disconnect"
	return self.L_disconnect()

    # Three helpers are required

    def L_connect_helper(self):
	"connect self to my_proxy"
	self.__my_proxy.connect_structured_push_supplier(self._this())

    def L_cleanup_helper(self, proxy):
	"disconnect self from proxy"
	proxy.disconnect_structured_push_consumer()

    def L_push_helper(self, batch):
	"push event(s) to channel"
	for data in batch:
	    self.__my_proxy.push_structured_event(data)

# Stand-Alone Constructor
def Create(chan, maxevs, bsize, objnm, idx, efn, tfn, ofs, constraints, mil, opr, vb):
    "stand-alone constructor"
    if (bsize != 1):
	print "For StructPushSup, batch size must be 1"
	bsize = 1
    admin, proxy = CliUtils.get_proxy_for_sup(CosNotifyChannelAdmin.STRUCTURED_EVENT, "push", opr,
					      CosNotifyChannelAdmin.StructuredProxyPushConsumer,
					      chan, objnm, ofs, vb)
    if (proxy == None):
	return None
    client = C(chan, admin, proxy, maxevs, bsize, objnm, idx, efn, tfn, mil, vb)
    b = client.L_add_filter("proxy", constraints)
    if (b == 1):
	try:
	    admin.destroy()
	except:
	    pass
	return None
    return client
