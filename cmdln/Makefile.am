
# Build bescmdln, part of the BES code.

AUTOMAKE_OPTIONS = foreign dejagnu

AM_CPPFLAGS = -I$(top_srcdir)/ppt -I$(top_srcdir)/dispatch

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

bin_PROGRAMS = bescmdln

bescmdln_SOURCES = CmdApp.cc CmdClient.cc CmdApp.h CmdClient.h

bescmdln_LDADD = -L../dispatch -lbes_dispatch -L../ppt -lbes_ppt $(openssl_libs)
DEJATOOL = bescmdln
RUNTESTDEFAULTFLAGS = --tool $$tool --srcdir testsuite

# Add a test to make sure beslister is running; this is only half correct
# since beslistener can be running n single access mode and then the tests
# still won't work... Also, this target is run _after_ 'check:' so we cannot
# use this to start beslistener. jhrg 1/2/07

check-local:
	@(ps -el | grep beslistener > /dev/null 2>&1;\
	  if test $$? != 0; \
	    then echo "beslitener must be running for these tests to work!";\
	  fi)



