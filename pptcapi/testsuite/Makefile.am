
# Build pptcapi test program

AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -I$(top_srcdir)

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

bin_PROGRAMS = pptcapi_test

pptcapi_test_SOURCES = pptcapi_test.c
pptcapi_test_LDADD = ../libbes_pptcapi.la

# This is the DejaGNU target copied from the Makefile but with my own name
# and some of the 'Deja' environment variables substituted since automake
# will complain if we use those without 'dejagnu' in the options at the of
# the file. I removed the dejagnu option because I don't want these tests
# running during the regular build, only after all the handlers have built
# and installed.

bes-pptcapi-tests: pptcapi_test
	srcdir=`$(am__cd) $(srcdir) && pwd`; export srcdir; \
	EXPECT=$(EXPECT); export EXPECT; \
	if $(SHELL) -c "runtest --version" > /dev/null 2>&1; then \
	  runtest --tool pptcapi_test --srcdir .; \
	  /bin/rm -f ./*/*.bescmd.output \
	else \
	  echo "WARNING: could not find \`runtest'" 1>&2; :;\
	fi

