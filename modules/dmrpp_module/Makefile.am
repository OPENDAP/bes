
AUTOMAKE_OPTIONS = foreign subdir-objects

ACLOCAL_AMFLAGS = -I conf

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/xmlcommand \
    -I$(top_srcdir)/http -I$(top_srcdir)/modules/dmrpp_module/types -I$(top_srcdir)/modules/ngap_module \
    -I$(top_srcdir)/pugixml/src $(DAP_CFLAGS)
AM_CXXFLAGS = -Wno-vla-extension -Wno-inconsistent-missing-override

# FIXME Remove this hack. Set these with configure. jhrg 11/25/19
OPENSSL_LIBS = -lcrypto
OPENSSL_INC = -I/usr/local/opt/openssl/include
OPENSSL_LDFLAGS = -L/usr/local/opt/openssl/lib

AM_CPPFLAGS += $(OPENSSL_INC)

# Set the module version here, in the spec file and in configure.ac
M_NAME=dmrpp_module
M_VER=1.1.6

AM_CPPFLAGS += -DMODULE_VERSION=\"$(M_VER)\"

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
# ./configure --disable-shared CXXFLAGS=...
# or ./configure --enable-developer --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align 

AM_LDFLAGS = 
include $(top_srcdir)/coverage.mk

SUBDIRS = types writer . unit-tests tests data tests_build_dmrpp

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libdmrpp_module.la

libdmrpp_module_la_SOURCES = $(DMRPP_MODULE)
libdmrpp_module_la_LDFLAGS = -avoid-version -module
libdmrpp_module_la_LIBADD = types/libdmrpp_types.la writer/libdmrpp_writer.la \
    $(BES_DISPATCH_LIB) $(BES_HTTP_LIB) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) \
    $(H5_LDFLAGS) $(H5_LIBS) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS) -ltest-types

DMRPP_MODULE = DmrppModule.cc DmrppModule.h DmrppRequestHandler.cc DmrppRequestHandler.h

bin_PROGRAMS = build_dmrpp check_dmrpp merge_dmrpp reduce_mdf

BUILT_SOURCES = h5common.cc h5common.h

h5common.cc: Makefile
	cp $(top_srcdir)/modules/hdf5_handler/h5common.cc ./h5common.cc

h5common.h: Makefile
	cp $(top_srcdir)/modules/hdf5_handler/h5common.h ./h5common.h

build_dmrpp_CPPFLAGS = $(AM_CPPFLAGS) $(H5_CPPFLAGS) -I$(top_srcdir)/modules/hdf5_handler
build_dmrpp_SOURCES = DmrppRequestHandler.cc DmrppRequestHandler.h build_dmrpp.cc build_dmrpp_util.cc build_dmrpp_util.h h5common.cc h5common.h
build_dmrpp_LDFLAGS = types/libdmrpp_types.la $(top_builddir)/dap/.libs/libdap_module.a
build_dmrpp_LDADD = types/libdmrpp_types.la $(BES_DISPATCH_LIB) $(BES_HTTP_LIB) $(H5_LDFLAGS) \
    $(H5_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS) \
    $(XML2_LIBS) $(BYTESWAP_LIBS) -lz

# check_dmrpp config
check_dmrpp_CPPFLAGS = $(AM_CPPFLAGS) $(H5_CPPFLAGS)
check_dmrpp_SOURCES = check_dmrpp.cc

# merge_dmrpp config
merge_dmrpp_CPPFLAGS = $(AM_CPPFLAGS)
merge_dmrpp_SOURCES = merge_dmrpp.cc

# reduce_mdf config
reduce_mdf_CPPFLAGS = $(AM_CPPFLAGS)
reduce_mdf_SOURCES = reduce_mdf.cc
reduce_mdf_LDADD = $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS) -lz

EXTRA_PROGRAMS =

EXTRA_DIST = dmrpp.conf.in

CLEANFILES = *~ *.gcda *.gcno *.gcov dmrpp.conf h5common.cc h5common.h

moduledir = $(sysconfdir)/bes/modules
module_DATA = dmrpp.conf

dmrpp.conf: dmrpp.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > dmrpp.conf

distclean-local:

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libdmrpp_module_la_SOURCES)
