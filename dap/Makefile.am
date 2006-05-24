
# Build libdispatch, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/dispatch

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

lib_LTLIBRARIES = libdap_module.la

libdap_module_la_SOURCES = $(SRCS) $(HDRS)

pkginclude_HEADERS = $(HDRS)

##############################################################################
#
# Sources and Headers
#
##############################################################################
SRCS:=

HDRS:=

