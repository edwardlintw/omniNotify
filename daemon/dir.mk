.PHONY : fooclean

include $(MAKEFILE_INC_DIR)../DEBUG.mk
include $(MAKEFILE_INC_DIR)../trilib.mk

CXXSRCS = ReadyChannel_d.cc
CXXOBJS = ReadyChannel_d.o 

# add services/include/omniNotify
IMPORT_CPPFLAGS += $(patsubst %,-I%/src/services/include/omniNotify,$(IMPORT_TREES))

DIR_CPPFLAGS  = $(CORBA_CPPFLAGS)
DIR_CPPFLAGS += $(COS_CPPFLAGS)
DIR_CPPFLAGS += -DUSE_stub_in_nt_dll

# omniORB3 does not have full support for long long in anys
majorVersion := $(word 1,$(subst ., ,$(OMNIORB_VERSION)))
ifeq ($(majorVersion),3)
DIR_CPPFLAGS += -DTIMEBASE_NOLONGLONG
endif

notifd   = $(patsubst %,$(BinPattern),notifd)

ALL_TARGETS = $(notifd)

all::     $(ALL_TARGETS)

export:: $(ALL_TARGETS)
	@(module="as"; $(ExportExecutable))

ifdef INSTALLTARGET
install:: $(ALL_TARGETS)
	@$(InstallExecutable)

veryclean::
	@(dir=$(INSTALLBINDIR); \
	 for file in $(ALL_TARGETS); do \
		base=`basename $$file`; \
		echo $(RM) $$dir/$$base; \
		$(RM) $$dir/$$base; \
		done;)
else
veryclean::
	@(dir=$(EXPORT_TREE)/$(BINDIR); \
	 for file in $(ALL_TARGETS); do \
		base=`basename $$file`; \
		echo $(RM) $$dir/$$base; \
		$(RM) $$dir/$$base; \
		done;)

endif

clean::
	$(RM) $(ALL_TARGETS) *.d

$(notifd) : $(CXXOBJS) $(TRI_LIB_DEPEND)
	@(libs="$(TRI_LIB)"; $(CXXExecutable))

# STATIC BUILD:
#$(notifd) : $(CXXOBJS) $(TRI_LIB_DEPEND)
#	@(libs="$(TRI_STATIC_LIB)"; $(CXXExecutable))

