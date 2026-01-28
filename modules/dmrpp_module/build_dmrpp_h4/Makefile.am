# Automake file for cmr_module
#
# 07/25/18
#
AUTOMAKE_OPTIONS = foreign subdir-objects

dist_bin_SCRIPTS = get_dmrpp_h4

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=builddmrpp_h4
M_VER=0.1.0

AM_CPPFLAGS = $(HDF4_CFLAGS) $(HDFEOS2_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/modules/dmrpp_module \
    -I$(top_srcdir)/pugixml/src -I$(top_srcdir)/http -I$(top_srcdir)/modules/dmrpp_module/ngap_container -I$(top_srcdir)/dap \
    $(DAP_CFLAGS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

bin_PROGRAMS = build_dmrpp_h4

build_dmrpp_h4_SOURCES = $(SRC) $(HDR)

SRC = build_dmrpp_h4.cc build_dmrpp_util_h4.cc

HDR = build_dmrpp_util_h4.h

build_dmrpp_h4_CPPFLAGS = $(AM_CPPFLAGS)

build_dmrpp_h4_LDFLAGS = $(HDFEOS2_LDFLAGS) $(HDF4_LDFLAGS) $(BES_DAP_LIB_LDFLAGS)

# TODO Fix this kludge: $(builddir)/../.libs/libdmrpp_module.a. The code above in dmrpp_module
# should build the various type classes into a convenience library and then link that with
# DmrppRequestHandler and DmrppModule to make the module so file. That convenience library
# should then be used here to make build_dmrpp_h4 (and likely to make other programs). jhrg 1/13/26

build_dmrpp_h4_LDADD = $(top_builddir)/modules/dmrpp_module/.libs/libdmrpp_module.a \
-L$(top_builddir)/modules/dmrpp_module/ngap_container -lngap \
$(BES_DISPATCH_LIB) $(BES_DAP_LIB) $(BES_HTTP_LIB) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) $(OPENSSL_LDFLAGS) \
$(OPENSSL_LIBS) $(XML2_LIBS) $(BYTESWAP_LIBS) $(HDFEOS2_LIBS) $(HDF4_LIBS)

lib_besdir=$(libdir)/bes

EXTRA_DIST = get_dmrpp_h4

CLEANFILES = *~

moduledir = $(sysconfdir)/bes/modules
module_DATA =

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(CMR_SRC) $(CMR_HDR)


