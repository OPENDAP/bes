
# Build libbes_ppt, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CFLAGS =

if BES_DEVELOPER
AM_CFLAGS += -DBES_DEVELOPER
endif

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

SUBDIRS = unit-tests
DIST_SUBDIRS = unit-tests

lib_LTLIBRARIES = libbes_pptcapi.la

noinst_HEADERS = pptcapi_utils.h

libbes_pptcapi_la_SOURCES = $(CSRCS) $(CHDRS)
libbes_pptcapi_la_LDFLAGS = -version-info $(LIBPPTCAPI_VERSION)

pkginclude_HEADERS = $(CHDRS)

CSRCS = pptcapi_authenticate.c pptcapi_tcp_connect.c pptcapi_socket_connect.c pptcapi_send.c pptcapi_receive.c pptcapi.c

CHDRS = pptcapi.h

bes-pptcapi-tests: libbes_pptcapi.la
	(cd testsuite && $(MAKE) $(AM_MAKEFLAGS) bes-pptcapi-tests)
