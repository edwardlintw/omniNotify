include $(MAKEFILE_INC_DIR)../libdefs.mk

# Generate BOA skeleton
DIR_IDLFLAGS += -WbBOA

# Look for .idl files in <top>/idl/COS and <top>/src/services/omniNotify/idl
vpath %.idl $(IMPORT_TREES:%=%/idl/COS) $(IMPORT_TREES:%=%/src/services/omniNotify/idl)
DIR_IDLFLAGS += -I. $(patsubst %,-I%/idl/COS,$(IMPORT_TREES))
DIR_IDLFLAGS += -I. $(patsubst %,-I%/src/services/omniNotify/idl,$(IMPORT_TREES))

# Look for .hh files in <top>/include/COS/BOA
IMPORT_CPPFLAGS += $(patsubst %,-I%/include/COS/BOA,$(IMPORT_TREES))

ATTN_SKLIB_NAME    = AttNotificationBOA

ATTN_SK_OBJS = $(ATTN_INTERFACES:%=%SK.o)
ATTN_SK_SRCS = $(ATTN_INTERFACES:%=%SK.cc)
CXXSRCS = $(ATTN_SK_SRCS) 

all:: mkstatic mkshared

export:: mkstatic mkshared

export:: $(ATTN_INTERFACES:%=%.hh) ATTN_sysdep.h
	@(for i in $^; do \
            file="$$i"; \
            dir="$(EXPORT_TREE)/$(INCDIR)/COS/BOA"; \
		$(ExportFileToDir) \
          done )

ifdef INSTALLTARGET
install:: $(ATTN_INTERFACES:%=%.hh) ATTN_sysdep.h
	@(dir="$(INSTALLINCDIR)/COS/BOA"; \
          for file in $^; do \
            $(ExportFileToDir) \
          done )
endif

veryclean::
	$(RM) $(ATTN_INTERFACES:%=%SK.cc) $(ATTN_INTERFACES:%=%DynSK.cc) \
              $(ATTN_INTERFACES:%=%.hh)

##############################################################################
# Build Static library
##############################################################################

version  := $(word 1,$(subst ., ,$(OMNIORB_VERSION)))

sk = static/$(patsubst %,$(LibNoDebugPattern),$(ATTN_SKLIB_NAME)$(version))

MDFLAGS += -p static/

mkstatic::
	@(dir=static; $(CreateDir))

mkstatic:: $(sk)

$(sk): $(patsubst %, static/%, $(ATTN_SK_OBJS))
	@$(StaticLinkLibrary)

export:: $(sk)
	@$(ExportLibrary)

clean::
	$(RM) static/*.o
	$(RM) $(sk)

##############################################################################
# Build Shared library
##############################################################################
ifdef BuildSharedLibrary

sharedversion = $(OMNIORB_VERSION)

sknamespec    = $(subst ., ,$(ATTN_SKLIB_NAME).$(sharedversion))
skshared      = shared/$(shell $(SharedLibraryFullName) $(sknamespec))

MDFLAGS += -p shared/

ifdef Win32Platform
# in case of Win32 lossage:
imps := $(patsubst $(DLLDebugSearchPattern),$(DLLNoDebugSearchPattern), \
         $(OMNIORB_LIB))
else
imps := $(OMNIORB_LIB)
endif

mkshared::
	@(dir=shared; $(CreateDir))

mkshared:: $(skshared)

$(skshared): $(patsubst %, shared/%, $(ATTN_SK_OBJS))
	@(namespec="$(sknamespec)" extralibs="$(imps)"; \
         $(MakeCXXSharedLibrary))

export:: $(skshared)
	@(namespec="$(sknamespec)"; \
         $(ExportSharedLibrary))

clean::
	$(RM) shared/*.o
	(dir=shared; $(CleanSharedLibrary))

endif

##############################################################################
# Build debug libraries for Win32
##############################################################################
ifdef Win32Platform

all:: mkstaticdbug mkshareddbug

export:: mkstaticdbug mkshareddbug

#####################################################
#      Static debug libraries
#####################################################
dbugversion = $(word 1,$(subst ., ,$(OMNIORB_VERSION)))

skdbug = debug/$(patsubst %,$(LibDebugPattern),$(ATTN_SKLIB_NAME)$(dbugversion))

MDFLAGS += -p debug/

mkstaticdbug::
	@(dir=debug; $(CreateDir))

mkstaticdbug:: $(skdbug)

$(skdbug): $(patsubst %, debug/%, $(ATTN_SK_OBJS))
	@$(StaticLinkLibrary)

export:: $(skdbug)
	@$(ExportLibrary)

clean::
	$(RM) debug/*.o
	$(RM) $(skdbug)

#####################################################
#      DLL debug libraries
#####################################################
shareddbugversion = $(OMNIORB_VERSION)

sknamespec      = $(subst ., ,$(ATTN_SKLIB_NAME).$(shareddbugversion))
skshareddbug    = shareddebug/$(shell $(SharedLibraryDebugFullName) $(sknamespec))

MDFLAGS += -p shareddebug/

dbugimps  := $(patsubst $(DLLNoDebugSearchPattern),$(DLLDebugSearchPattern), \
               $(OMNIORB_LIB))

mkshareddbug::
	@(dir=shareddebug; $(CreateDir))

mkshareddbug:: $(skshareddbug)

$(skshareddbug): $(patsubst %, shareddebug/%, $(ATTN_SK_OBJS))
	(namespec="$(sknamespec)" debug=1 extralibs="$(dbugimps)"; \
         $(MakeCXXSharedLibrary))

export:: $(skshareddbug)
	@(namespec="$(sknamespec)" debug=1; \
         $(ExportSharedLibrary))

clean::
	$(RM) shareddebug/*.o
	@(dir=shareddebug; $(CleanSharedLibrary))

endif


