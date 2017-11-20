
AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/xmlcommand $(XML2_CFLAGS)

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

check_LTLIBRARIES = libtestmodule.la

libtestmodule_la_SOURCES = $(SRCS) $(HDRS)
libtestmodule_la_LDFLAGS = -avoid-version -module
libtestmodule_la_LIBADD = ../../xmlcommand/libbes_xml_command.la ../../dispatch/libbes_dispatch.la 
			  
##############################################################################
#
# Sources and Headers for libtestmodule.

SRCS:=TestCommand.cc TestEhmResponseHandler.cc TestSigResponseHandler.cc \
	TestRequestHandler.cc TestException.cc TestModule.cc 

HDRS:=TestCommand.h TestEhmResponseHandler.h TestSigResponseHandler.h \
	TestRequestHandler.h TestException.h TestModule.h TestNames.h

