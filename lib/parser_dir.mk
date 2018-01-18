include $(MAKEFILE_INC_DIR)../defs.mk

CXXSRCS = $(PARSER_CXXSRCS)
CXXOBJS = $(PARSER_CXXOBJS)

DIR_CPPFLAGS += -DYYDEBUG=1 -DYYERROR_VERBOSE

all::    $(CXXOBJS)
	@ echo Parser objs are now up-to-date

export:: $(CXXOBJS)
	@ echo Parser objs are now up-to-date

clean::
	$(RM) *.d

mkobjs: $(CXXOBJS)
	@ echo Parser objs are now up-to-date
