NFYLIB_CXXSRCS = \
  RDIInteractive.cc \
  ChannelAdmin_i.cc \
  corba_wrappers_impl.cc \
  CosEventProxy.cc \
  CosNfyUtils.cc \
  CosNotification.cc \
  EventChannel_i.cc \
  FilterAdmin_i.cc \
  Filter_i.cc \
  omniNotify.cc \
  ProxyConsumer.cc \
  ProxySupplier.cc \
  RDI.cc \
  RDIstrstream.cc \
  RDIChannelUtil.cc \
  RDIConfig.cc \
  RDIConstraint.cc \
  RDIDynamicEval.cc \
  RDIEval.cc \
  RDIEventQueue.cc \
  RDINotifQueue.cc \
  RDINotifServer.cc \
  RDIOplocks.cc \
  RDIOpSeq.cc  \
  RDIParser_l.cc \
  RDIParser_y.cc \
  RDIRVM.cc \
  RDIRVMPool.cc \
  RDIStaticEval.cc \
  RDITimeWrappers.cc \
  RDITypeMap.cc

NFYLIB_CXXOBJS = $(NFYLIB_CXXSRCS:.cc=.o)

NFYLIB_OBJS = $(NFYLIB_CXXOBJS)

# omniORB3 does not yet support long long passed in ANY values,
# hence, we need to pass the -DNOLONGLONG flag so that TimeBase::TimeT
# values can be sent to/from the channel as property values.

OMNIORB_IDL_ANY_FLAGS  += -DNOLONGLONG
OMNIORB2_IDL_ANY_FLAGS += -DNOLONGLONG 

# add services/omniNotify/include:
IMPORT_CPPFLAGS += $(patsubst %,-I%/src/services/omniNotify/include,$(IMPORT_TREES))

DIR_CPPFLAGS  = $(CORBA_CPPFLAGS)

# -mt does not belong in CFLAGS
CFLAGS := $(patsubst -mt,,$(CFLAGS))

ifdef CosUsesBoa
# we need to specify -WbBOA for omniORB3 when we use BOA
OMNIORB_IDL_ANY_FLAGS  += -WbBOA

ifndef COSDynamicBOACompatible
# workaround for a problem with putting DynSK in COSBOA library
vpath %.idl $(IMPORT_TREES:%=%/idl/COS)
DIR_IDLFLAGS += -I. $(patsubst %,-I%/idl/COS,$(IMPORT_TREES))
DIR_IDLFLAGS  += -Wbuse_quotes
DIR_IDLFLAGS  += -DNOLONGLONG
DIR_CPPFLAGS = $(CORBA_CPPFLAGS) -D_COS_LIBRARY 
DIR_IDLFLAGS += -WbBOA
NFYLIB_CXXSRCS += CosNotificationDynSK.cc TimeBaseDynSK.cc RDITestTypesDynSK.cc

%DynSK.cc: %.idl
	$(CORBA_IDL) $(DIR_IDLFLAGS) $^	

.PRECIOUS: %DynSK.cc
endif
endif
