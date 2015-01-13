
# Build bescmdln, part of the BES code.

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = -I$(top_srcdir)/ppt -I$(top_srcdir)/xmlcommand -I$(top_srcdir)/dispatch $(XML2_CFLAGS)

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

SUBDIRS = . unit-tests testsuite
DIST_SUBDIRS = unit-tests testsuite

bin_PROGRAMS = bescmdln

bescmdln_SOURCES = CmdApp.cc CmdClient.cc CmdTranslation.cc 		\
		../xmlcommand/BESXMLUtils.cc				\
		CmdApp.h CmdClient.h CmdTranslation.h			\
		../xmlcommand/BESXMLUtils.h

bescmdln_LDADD = ../dispatch/libbes_dispatch.la ../ppt/libbes_ppt.la $(openssl_libs) $(READLINE_LIBS) $(XML2_LIBS)

server-tests:
	besctl start && \
	(cd testsuite && make server-tests) && \
	besctl stop
