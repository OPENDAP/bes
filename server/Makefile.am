
# Build opendapcmdln, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/ppt -I$(top_srcdir)/dispatch -I$(top_srcdir)/command

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

bin_PROGRAMS = opendapserver opendapdaemon

opendapserver_SOURCES = OPeNDAPServerHandler.cc ServerApp.cc \
	OPeNDAPServerHandler.h  ServerApp.h  ServerExitConditions.h 

opendapserver_LDADD = $(MODS) $(SERVER_LIBS)

opendapdaemon_SOURCES = daemon.cc

opendapdaemon_LDADD = $(AUTH_MOD) $(SERVER_LIBS)

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

# Maybe the source files for these should be added to this directory.
MODS = 

MODOBJS=OPeNDAPServerHandler.o ServerApp.o

SERVER_LIBS=-L../ppt -lppt -L../command -lopendap_command -L../dispatch -ldispatch

