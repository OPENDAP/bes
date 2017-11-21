
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

SUBDIRS = . unit-tests tests

# DIST_SUBDIRS = unit-tests testsuite

bin_PROGRAMS = bescmdln

bescmdln_SOURCES = CmdApp.cc CmdClient.cc CmdTranslation.cc \
CmdApp.h CmdClient.h CmdTranslation.h

# Including these as dependencies breaks the distclean target.
# I replaced them with a library include and made sure that the
# library's directory is built first.
# ../xmlcommand/BESXMLUtils.cc	../xmlcommand/BESXMLUtils.h

bescmdln_LDADD = $(top_builddir)/dispatch/libbes_dispatch.la $(top_builddir)/ppt/libbes_ppt.la \
$(top_builddir)/xmlcommand/libbes_xml_command.la $(openssl_libs) $(READLINE_LIBS) $(XML2_LIBS)

server-tests:
	besctl start && \
	(cd testsuite && make server-tests) && \
	besctl stop
