# Like cos.mk with these modifications:
#
# Provides NFY_LIB, NFY_LIB_DEPEND,
#          ATTN_LIB, ATTN_LIB_DEPEND
#
# Also provides combinations:
#     BOTH_LIB* = ATTN_LIB* + COS_LIB* + CORBA_LIB*
#     TRI_LIB*  = NFY_LIB* + BOTH_LIB*
#
# To use the COS idls in application IDLs
#   DIR_IDLFLAGS += $(COS_IDLFLAGS)
#
# To compile the application stubs:
#   DIR_CPPFLAGS += $(COS_CPPFLAGS)

# XXX UNCOMMENT THE FOLLOWING TO USE BOA
# XXX (we do not plan to support this forever, but it should work for now)
# XXX CosUsesBoa = 1

include $(BASE_OMNI_TREE)/mk/cos.mk

ifndef Win32Platform

nfy_libname        = COSNotify$(COS_OA)$(word 1,$(subst ., ,$(COS_VERSION)))
NFY_LIB    = $(patsubst %,$(LibSearchPattern),$(nfy_libname))

nfylib     = $(patsubst %,$(LibPattern),$(nfy_libname))
lib_depend := $(nfylib)
NFY_LIB_DEPEND := $(GENERATE_LIB_DEPEND)

attn_libname = AttNotification$(COS_OA)$(word 1,$(subst ., ,$(COS_VERSION)))
ATTN_LIB     = $(patsubst %,$(LibSearchPattern),$(attn_libname))

attnlib     = $(patsubst %,$(LibPattern),$(attn_libname))
lib_depend := $(attnlib)
ATTN_LIB_DEPEND := $(GENERATE_LIB_DEPEND)

else

ifndef BuildDebugBinary

nfy_dlln := $(shell $(SharedLibraryFullName) $(subst ., ,COSNotify$(COS_OA).$(COS_VERSION)))
attn_dlln := $(shell $(SharedLibraryFullName) $(subst ., ,AttNotification$(COS_OA).$(COS_VERSION)))

else

nfy_dlln := $(shell $(SharedLibraryDebugFullName) $(subst ., ,COSNotify$(COS_OA).$(COS_VERSION)))
attn_dlln := $(shell $(SharedLibraryDebugFullName) $(subst ., ,AttNotification$(COS_OA).$(COS_VERSION)))

endif

NFY_LIB     = $(nfy_dlln)
lib_depend := $(nfy_dlln)
NFY_LIB_DEPEND := $(GENERATE_LIB_DEPEND)

ATTN_LIB     = $(attn_dlln)
lib_depend := $(attn_dlln)
ATTN_LIB_DEPEND := $(GENERATE_LIB_DEPEND)

endif

ifdef CosUsesBoa
# BOA RULES -- includes workaround for a problem with putting DynSK in COSBOA library
ifdef COSDynamicBOACompatible
BOTH_LIB        = $(ATTN_LIB) $(COS_LIB)
BOTH_LIB_DEPEND = $(ATTN_LIB_DEPEND) $(COS_LIB_DEPEND)
else
BOTH_LIB        = $(ATTN_LIB_NODYN) $(COS_LIB_NODYN)
BOTH_LIB_DEPEND = $(ATTN_LIB_NODYN_DEPEND) $(COS_LIB_NODYN_DEPEND)
endif
else
# POA RULES
BOTH_LIB        = $(ATTN_LIB) $(COS_LIB)
BOTH_LIB_DEPEND = $(ATTN_LIB_DEPEND) $(COS_LIB_DEPEND)
endif

BOTH_LIB        += $(CORBA_LIB)       
BOTH_LIB_DEPEND += $(CORBA_LIB_DEPEND)

TRI_LIB         = $(NFY_LIB)          $(BOTH_LIB)
TRI_LIB_DEPEND  = $(NFY_LIB_DEPEND)   $(BOTH_LIB_DEPEND)

# XXX only works for unix platforms?
BOTH_STATIC_LIB = $(BOTH_LIB_DEPEND)
BOTH_STATIC_LIB += $(OMNITHREAD_LIB) $(SOCKET_LIB)

TRI_STATIC_LIB  = $(TRI_LIB_DEPEND)
TRI_STATIC_LIB += $(OMNITHREAD_LIB) $(SOCKET_LIB)


