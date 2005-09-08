
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

opendapserver_LDADD = $(MODS) $(AUTH_MOD) $(SERVER_LIBS)

opendapdaemon_SOURCES = daemon.cc

opendapdaemon_LDADD = $(AUTH_MOD) $(SERVER_LIBS)

# Maybe the source files for these should be added to this directory.
MODS = ../command/opendap_commands.o ../dispatch/dods_module.o
AUTH_MOD = ../dispatch/test_authenticator.o

# The original code uses MODULES and PACKAGES to include more object files.
# I'll add this here as a reminder...
MODULES=
PACKAGES=

SERVER_LIBS=-L../dispatch -ldispatch -L../ppt -lppt -L../command -lopendap_command
