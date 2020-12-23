
# Tests

AUTOMAKE_OPTIONS = foreign

# Arrange to build with the backward compatibility mode enabled.
AM_CPPFLAGS = -I$(top_srcdir) $(BES_CPPFLAGS)

# These are not used by automake but are often useful for certain types of
# debugging. 
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

check_PROGRAMS = $(TESTS)

noinst_DATA = opendap.ini

# This determines what gets run by 'make check.'
TESTS = requestsT wrapperT encodeT 

CLEANFILES = *.log *.sum real*

EXTRA_DIST = opendap.ini.in

DISTCLEANFILES = opendap.ini

opendap.ini: opendap.ini.in
	sed -e "s%[@]srcdir[@]%${srcdir}%" $< > opendap.ini

############################################################################

requestsT_SOURCES = requestsT.cc
requestsT_LDADD =  ../.libs/BESApacheRequests.o $(BES_LIBS)

wrapperT_SOURCES = wrapperT.cc
wrapperT_LDADD = ../.libs/BESApacheWrapper.o ../.libs/BESApacheRequests.o \
../.libs/BESApacheInterface.o -L$(BES_MODULE_DIR) -ldap_module \
-ldap_cmd_module $(BES_DAP_LIBS)

encodeT_SOURCES = encodeT.cc
