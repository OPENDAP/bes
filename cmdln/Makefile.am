
# Build bescmdln, part of the BES code.

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = -I$(top_srcdir)/ppt -I$(top_srcdir)/xmlcommand -I$(top_srcdir)/dispatch $(XML2_CFLAGS)
AM_CXXFLAGS =

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
# CXXFLAGS = 
endif

CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align 
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

if BUILD_HYRAX_DEBUG
AM_CXXFLAGS += $(CXXFLAGS_DEBUG)
endif

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
