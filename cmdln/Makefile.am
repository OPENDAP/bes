
# Build opendapcmdln, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/ppt -I$(top_srcdir)/dispatch

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

bin_PROGRAMS = opendapcmdln

opendapcmdln_SOURCES = CmdApp.cc CmdClient.cc CmdApp.h CmdClient.h

opendapcmdln_LDADD = $(AUTH_MOD) -L../dispatch -ldispatch -L../ppt -lppt

AUTH_MOD = ../dispatch/test_authenticator.o
