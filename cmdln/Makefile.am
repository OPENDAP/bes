
# Build bescmdln, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/ppt -I$(top_srcdir)/dispatch $(XML2_CFLAGS)

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

SUBDIRS = unit-tests
DIST_SUBDIRS = unit-tests

bin_PROGRAMS = bescmdln

bescmdln_SOURCES = CmdApp.cc CmdClient.cc CmdApp.h CmdClient.h 		\
		CmdTranslation.cc CmdTranslation.h

bescmdln_LDADD = ../dispatch/libbes_dispatch.la ../ppt/libbes_ppt.la $(openssl_libs) $(READLINE_LIBS) $(XML2_LIBS)

EXTRA_DIST = testsuite common_tests.exp

# DEJATOOL = bescmdln
# RUNTESTDEFAULTFLAGS = --tool $$tool --srcdir testsuite

# This is the DejaGNU target copied from the Makefile but with my own name
# and some of the 'Deja' environment variables substituted since automake
# will complain it we use those without 'dejagnu' in the options at the of
# the file. I removed the dejagnu option because I don't want these tests
# running during the regular build, only after all the handlers have built
# and installed. I've also added some code to make sure a bes is running
# before we start the tests. jhrg 3/31/07

bes-configuration-tests:
	srcdir=`$(am__cd) $(srcdir) && pwd`; export srcdir; \
	EXPECT=$(EXPECT); export EXPECT; \
	if $(SHELL) -c "runtest --version" > /dev/null 2>&1; then \
	  runtest --tool bescmdln --srcdir testsuite; \
	  /bin/rm -f testsuite/*/*.bescmd.output \
	else \
	  echo "WARNING: could not find \`runtest'" 1>&2; :;\
	fi

#	@(ps -eA | grep beslistener > /dev/null 2>&1;\
#	  if test $$? != 0; then \
#		echo;\
#		echo "Starting bes using the default configuration!";\
#		echo;\
#		besctl start;\
#	  fi)
#	@(ps -el | grep beslistener > /dev/null 2>&1;\
#	  if test $$? = 0; then \
#		echo;\
#		echo "Stopping the bes";\
#		echo;\
#		besctl stop;\
#	  fi)

