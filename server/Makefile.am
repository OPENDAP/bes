
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

bin_PROGRAMS = beslistener besdaemon
dist_bin_SCRIPTS = besctl hyrax

beslistener_SOURCES = BESServerHandler.cc ServerApp.cc BESServerUtils.cc \
		BESServerHandler.h ServerApp.h BESServerUtils.h \
		ServerExitConditions.h

beslistener_LDADD = $(SERVER_LIBS) $(openssl_libs)

besdaemon_SOURCES = daemon.cc BESServerUtils.cc BESServerUtils.h
besdaemon_CPPFLAGS = $(AM_CPPFLAGS) -DBES_BIN_DIR=\"${bindir}\" -DBES_STATE_DIR=\"${localstatedir}\"

besdaemon_LDADD = $(SERVER_LIBS) $(openssl_libs)

SERVER_LIBS = -L../ppt -lbes_ppt -L../command -lbes_command -L../dispatch -lbes_dispatch

install-data-local:
	test -z "$(localstatedir)/run" || $(mkdir_p) "$(DESTDIR)$(localstatedir)/run"
