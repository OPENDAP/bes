
# Build libdispatch, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/command

# AM_CPPFLAGS =
# AM_CXXFLAGS =

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

lib_LTLIBRARIES = libtestmodule.la

libtestmodule_la_SOURCES = $(SRCS) $(HDRS)

pkginclude_HEADERS = $(HDRS)

##############################################################################
#
# Sources and Headers for libtestmodule.

SRCS:=TestCommand.cc TestEhmResponseHandler.cc TestSigResponseHandler.cc \
	TestRequestHandler.cc TestException.cc TestModule.cc 

HDRS:=TestCommand.h TestEhmResponseHandler.h TestSigResponseHandler.h \
	TestRequestHandler.h TestException.h TestModule.h TestNames.h

