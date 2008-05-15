
# Tests

AUTOMAKE_OPTIONS = foreign

# Arrange to build with the backward compatibility mode enabled.
AM_CPPFLAGS = -I$(top_srcdir)/dispatch $(BES_CPPFLAGS)

# These are not used by automake but are often useful for certain types of
# debugging. 
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

check_PROGRAMS = $(TESTS)

# This determines what gets run by 'make check.'
TESTS = requestsT wrapperT

CLEANFILES = *.log *.sum real*

############################################################################

requestsT_SOURCES = requestsT.cc
requestsT_LDADD =  ../.libs/BESApacheRequests.o $(BES_LIBS)
wrapperT_SOURCES = wrapperT.cc
wrapperT_LDADD = ../.libs/BESApacheWrapper.o ../.libs/BESApacheRequests.o ../.libs/BESApacheInterface.o -L$(BES_MODULE_DIR) -ldap_module -ldap_cmd_module $(BES_DAP_LIBS)

