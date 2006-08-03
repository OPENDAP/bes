
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
sbin_SCRIPTS = besctl

bes_SOURCES = BESServerHandler.cc ServerApp.cc BESServerHandler.h \
		ServerApp.h ServerExitConditions.h 

bes_LDADD = $(SERVER_LIBS) $(openssl_libs)

besd_SOURCES = daemon.cc

besd_LDADD = $(SERVER_LIBS) $(openssl_libs)

SERVER_LIBS = -L../ppt -lppt -L../command -lopendap_command -L../dispatch -ldispatch

EXTRA_DIST = besctl

install-data-local:
	test -z "$(prefix)/run" || $(mkdir_p) "$(DESTDIR)$(prefix)/run"

