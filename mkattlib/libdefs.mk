# Edit ATTN_INTERFACES to select which Att idl to build into the stub library.
#
ATTN_INTERFACES = AttNotification

DIR_IDLFLAGS  += -Wbuse_quotes

# omniidl + omniidl2 do not support long long and, hence,
# we need to pass the -DNOLONGLONG flag to generate correct stubs.
#DIR_IDLFLAGS  += -DNOLONGLONG

DIR_CPPFLAGS = $(CORBA_CPPFLAGS) -D_ATTN_LIBRARY

%.DynSK.cc %SK.cc: %.idl
	$(CORBA_IDL) $(DIR_IDLFLAGS) $^	

.PRECIOUS: %.DynSK.cc %SK.cc
