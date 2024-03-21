
# Build libdebug_functions, part of libdap.

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=debug_functions
M_VER=1.1.5

AM_CPPFLAGS = $(GF_CFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/http $(DAP_CFLAGS)
LIBADD = $(GF_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) $(BES_HTTP_LIB)

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -Wcast-align

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS =  . unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libdebug_functions.la

SRCS =  DebugFunctions.cc 

HDRS =  DebugFunctions.h 

libdebug_functions_la_SOURCES = $(SRCS) $(HDRS)
# libdebug_functions_la_CPPFLAGS = $(GF_CFLAGS) $(XML2_CFLAGS)
libdebug_functions_la_LDFLAGS = -avoid-version -module 
libdebug_functions_la_LIBADD = $(LIBADD) 

EXTRA_DIST = data debug_functions.conf.in

package_version=$(PACKAGE_MAJOR_VERSION).$(PACKAGE_MINOR_VERSION).$(PACKAGE_SUBMINOR_VERSION)

CLEANFILES = *~ debug_functions.conf debug_functions-*.tar.gz

moduledir = $(sysconfdir)/bes/modules
module_DATA = debug_functions.conf

debug_functions.conf: debug_functions.conf.in $(top_builddir)/config.status
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
                -e "s%[@]sysconfdir[@]%${sysconfdir}%" \
                -e "s%[@]pkglibdir[@]%${pkglibdir}%" \
                -e "s%[@]datarootdir[@]%${datarootdir}%" \
                -e "s%[@]datadir[@]%${datadir}%" \
                -e "s%[@]bindir[@]%${bindir}%" \
                -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > debug_functions.conf

                # Not nearly as clean as it could be, but this removes .svn directories
C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libdebug_functions_la_SOURCES)
