include $(MAKEFILE_INC_DIR)../DEBUG.mk
include $(MAKEFILE_INC_DIR)libdefs.mk
include $(MAKEFILE_INC_DIR)../trilib.mk

MDFLAGS += -p static/ -p shared/

DIR_CPPFLAGS += $(COS_CPPFLAGS) -D_OMNINOTIFY_LIBRARY -DUSE_stub_in_nt_dll

# omniORB3 does not have full support for long long in anys
majorVersion := $(word 1,$(subst ., ,$(OMNIORB_VERSION)))
ifeq ($(majorVersion),3)
DIR_CPPFLAGS += -DTIMEBASE_NOLONGLONG
endif

ifeq ($(platform),sun4_sosV_5.5)
DIR_CPPFLAGS += -D__SOSV_MINOR_VER__=55
endif

NFYLIB_NAME    = COSNotify$(COS_OA)

CXXSRCS = $(NFYLIB_CXXSRCS)
CSRCS = $(NFYLIB_CSRCS)

all:: export_useful
ifndef DontBuildStaticLibrary
all:: mkstatic mkshared
else
all:: mkshared
endif

export:: export_useful
ifndef DontBuildStaticLibrary
export:: mkstatic mkshared
else
export:: mkshared
endif

# export the library include file to <omni_top>/include
export_useful:: ../include/omniNotify.h
	@(for i in $^; do \
            file="$$i"; \
            dir="$(EXPORT_TREE)/$(INCDIR)"; \
		$(ExportFileToDir) \
          done )

# export .h files useful for writing omniNotify daemon / omniNotify client programs
# to <omni_top>/src/services/include/omniNotify
export_useful:: ../include/CosNotifyShorthands.h ../include/RDIStringDefs.h ../include/RDIstrstream.h ../include/RDIOSWrappers.h ../include/thread_wrappers.h ../include/omnithread_thread_wrappers.h ../include/RDIInteractiveMode.h ../include/corba_wrappers.h ../include/corba_wrappers_impl.h ../include/omniorb_common_wrappers.h  ../include/omniorb_boa_wrappers.h ../include/omniorb_poa_wrappers.h ../include/omniorb_boa_wrappers_impl.h ../include/omniorb_poa_wrappers_impl.h ../include/RDIsysdep.h
	@(for i in $^; do \
            file="$$i"; \
            dir="$(EXPORT_TREE)/src/services/include/omniNotify"; \
		$(ExportFileToDir) \
          done )

##############################################################################
# Build Static library
##############################################################################

version  := $(word 1,$(subst ., ,$(OMNIORB_VERSION)))

ifndef DontBuildStaticLibrary
sk = static/$(patsubst %,$(LibNoDebugPattern),$(NFYLIB_NAME)$(version))

mkstatic::
	@(dir=static; $(CreateDir))

mkstatic:: $(sk)

$(sk): $(patsubst %, static/%, $(NFYLIB_OBJS))
	@$(StaticLinkLibrary)

export:: $(sk)
	@$(ExportLibrary)

ifdef INSTALLTARGET
install:: $(sk)
	@$(InstallLibrary)

veryclean::
	@(dir="$(INSTALLLIBDIR)"; \
	  base=`basename $(sk)`; \
	  echo $(RM) $$dir/$$base; \
	  $(RM) $$dir/$$base )
else
veryclean::
	@(dir="$(EXPORT_TREE)/$(LIBDIR)"; \
	  echo $(RM) $$dir/$(sk); \
	  $(RM) $$dir/$(sk) )

endif

clean::
	$(RM) static/*.o
	$(RM) $(sk)
endif

##############################################################################
# Build Shared library
##############################################################################
ifdef BuildSharedLibrary

sharedversion = $(OMNIORB_VERSION)

sknamespec    = $(subst ., ,$(NFYLIB_NAME).$(sharedversion))
skshared      = shared/$(shell $(SharedLibraryFullName) $(sknamespec))

mkshared::
	@(dir=shared; $(CreateDir))

mkshared:: $(skshared)

$(skshared): $(patsubst %, shared/%, $(NFYLIB_OBJS))
	@(namespec="$(sknamespec)" \
          extralibs="$(OMNIORB_LIB) $(COS_LIB) $(ATTN_LIB)"; \
          $(MakeCXXSharedLibrary))

export:: $(skshared)
	@(namespec="$(sknamespec)"; \
         $(ExportSharedLibrary))

ifdef INSTALLTARGET
install:: $(skshared)
	@(namespec="$(sknamespec)"; \
         $(InstallSharedLibrary))

veryclean::
	@(namespec="$(sknamespec)"; \
	  dir="$(INSTALLLIBDIR)"; \
	  $(ParseNameSpec); \
          fullsoname=$(SharedLibraryFullNameTemplate); \
	  soname=$(SharedLibrarySoNameTemplate); \
	  libname=$(SharedLibraryLibNameTemplate); \
	  echo $(RM) $$dir/$$fullsoname $$dir/$$soname $$dir/$$libname; \
	  $(RM) $$dir/$$fullsoname $$dir/$$soname $$dir/$$libname)
else
veryclean::
	@(namespec="$(sknamespec)"; \
	  dir="$(EXPORT_TREE)/$(LIBDIR)"; \
	  $(ParseNameSpec); \
          fullsoname=$(SharedLibraryFullNameTemplate); \
	  soname=$(SharedLibrarySoNameTemplate); \
	  libname=$(SharedLibraryLibNameTemplate); \
	  echo $(RM) $$dir/$$fullsoname $$dir/$$soname $$dir/$$libname; \
	  $(RM) $$dir/$$fullsoname $$dir/$$soname $$dir/$$libname)

endif

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

skdbug = debug/$(patsubst %,$(LibDebugPattern),$(NFYLIB_NAME)$(dbugversion))

mkstaticdbug::
	@(dir=debug; $(CreateDir))

mkstaticdbug:: $(skdbug)

$(skdbug): $(patsubst %, debug/%, $(NFYLIB_OBJS))
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

sknamespec      = $(subst ., ,$(NFYLIB_NAME).$(shareddbugversion))
skshareddbug    = shareddebug/$(shell $(SharedLibraryDebugFullName) $(sknamespec))

dbugimps  := $(patsubst $(DLLNoDebugSearchPattern),$(DLLDebugSearchPattern), \
               $(OMNIORB_LIB) $(COS_LIB) $(ATTN_LIB))

mkshareddbug::
	@(dir=shareddebug; $(CreateDir))

mkshareddbug:: $(skshareddbug)

$(skshareddbug): $(patsubst %, shareddebug/%, $(NFYLIB_OBJS))
	(namespec="$(sknamespec)" debug=1 \
         extralibs="$(dbugimps)"; \
         $(MakeCXXSharedLibrary))

export:: $(skshareddbug)
	@(namespec="$(sknamespec)" debug=1; \
         $(ExportSharedLibrary))

clean::
	$(RM) shareddebug/*.o
	@(dir=shareddebug; $(CleanSharedLibrary))

endif

