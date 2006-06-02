
# Build opendapcmdln, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/ppt -I$(top_srcdir)/dispatch -I$(top_srcdir)/command

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

SUBDIRS = test
DIST_SUBDIRS = test

sbin_PROGRAMS = bes besd

bes_SOURCES = OPeNDAPServerHandler.cc ServerApp.cc \
	OPeNDAPServerHandler.h  ServerApp.h  ServerExitConditions.h 

bes_LDADD = $(SERVER_LIBS) $(openssl_libs)

besd_SOURCES = daemon.cc

besd_LDADD = $(SERVER_LIBS) $(openssl_libs)

# Maybe the source files for these should be added to this directory.
SERVER_LIBS=-L../ppt -lppt -L../command -lopendap_command -L../dispatch -ldispatch

SERVER_INSTALL_FILES=bes.conf
install-data-local: $(SERVER_INSTALL_FILES)
	test -z "$(sysconfdir)/bes" || $(mkdir_p) "$(DESTDIR)$(sysconfdir)/bes"
	list='$(SERVER_INSTALL_FILES)'; for p in $$list; do \
	  if test -f $$p; then \
	    f=$(am__strip_dir) \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(sysconfdir)/bes/$$f; \
	  fi; \
	done

uninstall-local:
	@set -x; list='$(SERVER_INSTALL_FILES)'; for p in $$list; do \
	  f=$(am__strip_dir) \
	  rm -f "$(DESTDIR)$(sysconfdir)/bes/$$f"; \
	done

