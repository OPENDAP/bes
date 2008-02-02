
# Build bescmdln, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/ppt -I$(top_srcdir)/command -I$(top_srcdir)/dispatch

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

bin_PROGRAMS = besstandalone

besstandalone_SOURCES = StandAloneApp.cc StandAloneClient.cc \
			StandAloneApp.h StandAloneClient.h
besstandalone_LDADD = ../dispatch/libbes_dispatch.la ../command/libbes_command.la $(READLINE_LIBS)

