
# Build libdispatch, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I../dispatch

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

lib_LTLIBRARIES = libopendap_command.la

libopendap_command_la_SOURCES = $(SRCS) $(HDRS)

libopendap_command_la_LDFLAGS = -version-info $(LIBCOMMAND_VERSION)

pkginclude_HEADERS = $(HDRS)

all-local: $(MODOBJS)

install-data-local: $(MODOBJS)
	test -z "$(pkglibdir)" || $(mkdir_p) "$(DESTDIR)$(pkglibdir)"
	@list='$(MODOBJS)'; for p in $$list; do \
	  if test -f $$p; then \
	    f=$(am__strip_dir) \
	    $(INSTALL) $$p $(DESTDIR)$(pkglibdir)/$$f
	  else :; fi; \
	done

uninstall-local:
	@set -x; list='$(MODOBJS)'; for p in $$list; do \
	  p=$(am__strip_dir) \
	  rm -f '$(DESTDIR)$(pkglibdir)/$$p'; \
	done

# noinst_HEADERS = 

EXTRA_DIST = $(MODSRCS)

CLEANFILES = $(MODOBJS)


##############################################################################
#
# Sources and Headers for 
SRCS:=OPeNDAPCmdInterface.cc OPeNDAPCmdParser.cc OPeNDAPCommand.cc \
	OPeNDAPDefineCommand.cc OPeNDAPDeleteCommand.cc OPeNDAPGetCommand.cc \
	OPeNDAPSetCommand.cc OPeNDAPShowCommand.cc OPeNDAPTermCommand.cc

HDRS:=OPeNDAPCmdInterface.h OPeNDAPCmdParser.h OPeNDAPCommand.h

MODSRCS:=opendap_commands.cc
MODOBJS:=opendap_commands.o

# TEST_SRC:=cmd_test.cc
# TEST_OBJ:=cmd_test.o
# TEST_EXEC:=cmd_test

# $(TEST_EXEC) : $(TEST_OBJ) $(LIB) $(MOD_OBJS)
# 	$(CXX) -o $(TEST_EXEC) $(TEST_OBJ) $(MOD_OBJS) $(LIB) $(LIBS)

