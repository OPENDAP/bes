
# Build libdispatch, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/dispatch

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
	list='$(MODOBJS)'; for p in $$list; do \
	  if test -f $$p; then \
	    f=$(am__strip_dir) \
	    $(INSTALL) $$p $(DESTDIR)$(pkglibdir)/$$f; \
	  fi; \
	done

uninstall-local:
	@set -x; list='$(MODOBJS)'; for p in $$list; do \
	  f=$(am__strip_dir) \
	  rm -f "$(DESTDIR)$(pkglibdir)/$$f"; \
	done

EXTRA_DIST = $(MODSRCS)

CLEANFILES = $(MODOBJS)

##############################################################################
#
# Sources and Headers
#
##############################################################################
SRCS:=OPeNDAPCmdInterface.cc OPeNDAPCmdParser.cc OPeNDAPCommand.cc	     \
	OPeNDAPDefineCommand.cc OPeNDAPDeleteCommand.cc OPeNDAPGetCommand.cc \
	OPeNDAPSetCommand.cc OPeNDAPShowCommand.cc OPeNDAPTermCommand.cc     \
	OPeNDAPCatalogCommand.cc OPeNDAPParserException.cc \
	OPeNDAPTokenizer.cc OPeNDAPCommandModule.cc opendap_commands.cc

HDRS:=OPeNDAPCmdInterface.h OPeNDAPCommand.h OPeNDAPTokenizer.h OPeNDAPParserException.h opendap_commands.h

MODSRCS:=

MODOBJS:=

