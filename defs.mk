faketarget::
	@ echo "SPECIFY TARGET (e.g., 'all','export','clean','veryclean',...)"
	@ echo " "
	@ exit 1

# -mt does not belong in CFLAGS
CFLAGS := $(patsubst -mt,,$(CFLAGS))

COMMON_CXXSRCS = \
    RDIDebug.cc        RDITime.cc         Filter_i.cc        FilterAdmin_i.cc \
    RDITypeMap.cc      RDI.cc             RDIFileSys.cc      RDIConfig.cc \
    RDIFileIO.cc       RDILog.cc          CosNaming_i.cc
COMMON_CXXOBJS = $(COMMON_CXXSRCS:.cc=.o)

COMMON_CSRCS = RDIDummyAstError.c
COMMON_COBJS = RDIDummyAstError.o

PARSER_CXXSRCS = \
    RDIParser_y.cc     RDIParser_l.cc     RDIRVM.cc          RDIEval.cc \
    RDIStaticEval.cc   RDIDynamicEval.cc  RDIConstraint.cc   RDIOpSeq.cc 

PARSER_CXXOBJS = \
    RDIParser_y.o      RDIParser_l.o      RDIRVM.o           RDIEval.o \
    RDIStaticEval.o    RDIDynamicEval.o   RDIConstraint.o    RDIOpSeq.o

SERVER_CXXSRCS = \
    ChannelAdmin_i.cc  PullConsumer_i.cc  PushConsumer_i.cc  EventChannel_i.cc \
    PullSupplier_i.cc  PushSupplier_i.cc  RDIChannelUtil.cc  RDIEventQueue.cc \
    CosEventProxy.cc   CosNotification.cc omniNotify.cc 
SERVER_CXXOBJS = $(SERVER_CXXSRCS:.cc=.o)

COMMON_OBJS    = $(patsubst %,../common/%,$(COMMON_CXXOBJS)) $(patsubst %,../common/%,$(COMMON_COBJS))
PARSER_OBJS    = $(patsubst %,../parser/%,$(PARSER_CXXOBJS))
SERVER_OBJS    = $(patsubst %,../server/%,$(SERVER_CXXOBJS))

# sometimes we need subsets of COMMON_OBJS
COMMON_OBJS_MINUS_DUMMYASTERROR = $(patsubst ../common/RDIDummyAstError.o,,$(COMMON_OBJS))
COMMON_OBJS_MINUS_TYPEMAP = $(patsubst ../common/RDITypeMap.o,,$(COMMON_OBJS))
COMMON_OBJS_MINUS_TYPEMAP_N_FILESYS = $(patsubst ../common/RDIFileSys.o,,$(COMMON_OBJS_MINUS_TYPEMAP))

# omniidl/omniidl2 do not support long long and, hence, we need
# to pass the -DNOLONGLONG flag for TimeBase.idl.  In addition,
# we need to specify -WbBOA for omniORB3 since we still use BOA

OMNIORB_IDL_ANY_FLAGS  += -DNOLONGLONG -WbBOA
OMNIORB2_IDL_ANY_FLAGS += -DNOLONGLONG 

# add services/include and services/omniNotify/include:
IMPORT_CPPFLAGS += $(patsubst %,-I%/src/services/include,$(IMPORT_TREES))
IMPORT_CPPFLAGS += $(patsubst %,-I%/src/services/omniNotify/include,$(IMPORT_TREES))

DIR_CPPFLAGS  = $(CORBA_CPPFLAGS)

# Naming and bootstrap are built into omniORB

# CORBA_INTERFACES = TimeBase CosTime CosEventComm CosEventChannelAdmin \
#                    CosNotification CosNotifyComm CosNotifyFilter \
#                    CosNotifyChannelAdmin CosTimerEvent \
# 		     RDITestTypes AttNotification

CORBA_INTERFACES = 

ALL_DEVTEST_HELPER_OBJS = $(COMMON_OBJS_MINUS_TYPEMAP_N_FILESYS) $(PARSER_OBJS) $(CORBA_STUB_OBJS)
ALL_SERVER_HELPER_OBJS  = $(COMMON_OBJS)                         $(PARSER_OBJS) $(CORBA_STUB_OBJS)
ALL_RUX_HELPER_OBJS     = $(COMMON_OBJS_MINUS_DUMMYASTERROR)     $(PARSER_OBJS) $(CORBA_STUB_OBJS) $(SERVER_OBJS)
ALL_FTNOTIF_HELPER_OBJS = $(COMMON_OBJS)                         $(PARSER_OBJS) $(CORBA_STUB_OBJS) $(SERVER_OBJS)

COS_LIB    = $(patsubst %,$(LibSearchPattern),COS_BOA)
COS_DYNLIB = $(patsubst %,$(LibSearchPattern),COSdynamic)
coslib     = $(patsubst %,$(LibPattern),COS_BOA)
cosdynlib  = $(patsubst %,$(LibPattern),COSdynamic)

lib_depend := $(coslib) 
COS_LIB_DEPEND := $(GENERATE_LIB_DEPEND)
lib_depend := $(cosdynlib) 
COS_DYNLIB_DEPEND := $(GENERATE_LIB_DEPEND)

# XXX TEMPORARILY USE STATIC COS LIBRARIES
#BOTH_LIB        = $(CORBA_LIB)        $(COS_LIB_DEPEND) $(COS_DYNLIB_DEPEND)
BOTH_LIB        = $(CORBA_LIB)        $(COS_LIB)        $(COS_DYNLIB)

BOTH_LIB_DEPEND = $(CORBA_LIB_DEPEND) $(COS_LIB_DEPEND) $(COS_DYNLIB_DEPEND)

