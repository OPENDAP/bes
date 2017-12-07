
AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = $(DAP_CFLAGS) $(BES_CPPFLAGS) -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch
ACLOCAL_AMFLAGS = -I conf
AM_CXXFLAGS = 

# Set the module version here, in the spec file and in configure.ac
M_NAME=dmrpp_module
M_VER=0.4.0

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
# ./configure --disable-shared CXXFLAGS=...
# or ./configure --enable-developer --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align 
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

SUBDIRS = . unit-tests data

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libdmrpp_module.la

BES_SRCS = DmrppCommon.cc H4ByteStream.cc \
DmrppModule.cc DmrppRequestHandler.cc DmrppByte.cc DmrppArray.cc \
DmrppFloat32.cc DmrppFloat64.cc DmrppInt16.cc DmrppInt32.cc DmrppInt64.cc \
DmrppInt8.cc DmrppUInt16.cc DmrppUInt32.cc DmrppUInt64.cc DmrppStr.cc  \
DmrppStructure.cc DmrppUrl.cc DmrppD4Enum.cc DmrppD4Group.cc DmrppD4Opaque.cc \
DmrppD4Sequence.cc  DmrppTypeFactory.cc DmrppParserSax2.cc DmrppUtil.cc \
Odometer.cc 

BES_HDRS = DmrppCommon.h H4ByteStream.h \
DmrppModule.h DmrppRequestHandler.h DmrppByte.h \
DmrppArray.h DmrppFloat32.h DmrppFloat64.h DmrppInt16.h DmrppInt32.h \
DmrppInt64.h DmrppInt8.h DmrppUInt16.h DmrppUInt32.h DmrppUInt64.h \
DmrppStr.h DmrppStructure.h DmrppUrl.h DmrppD4Enum.h DmrppD4Group.h \
DmrppD4Opaque.h DmrppD4Sequence.h DmrppTypeFactory.h DmrppParserSax2.h \
DmrppUtil.h Odometer.h 

# hack. This is probably not needed and is currently not used, but
# I'm leaving it in for now. If there are run-time issues with this
# module, add '$(BES_LIBS) $(OPENSSL_LIBS)' to LIBADD below.
# jhrg 10/13/14 
BES_LIBS = $(top_builddir)/xmlcommand/libbes_xml_command.la \
$(top_builddir)/ppt/libbes_ppt.la \
$(top_builddir)/dispatch/libbes_dispatch.la

libdmrpp_module_la_SOURCES = $(BES_HDRS) $(BES_SRCS)
libdmrpp_module_la_LDFLAGS = -avoid-version -module 
libdmrpp_module_la_LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) -ltest-types 

EXTRA_PROGRAMS = 

EXTRA_DIST = dmrpp.conf.in

CLEANFILES = *~ dmrpp.conf

# sample_datadir = $(datadir)/hyrax/data/dmrpp
# sample_data_DATA = $(SAMPLE_DATA_FILES)

install-data-local: dmrpp.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/dmrpp.conf \
	    && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/dmrpp.conf $(DESTDIR)$(sysconfdir)/bes/modules/dmrpp.conf.$$currdate; \
	$(INSTALL_DATA) dmrpp.conf $(DESTDIR)$(sysconfdir)/bes/modules/dmrpp.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/dmrpp.conf

dmrpp.conf: dmrpp.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > dmrpp.conf

distclean-local:

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libdmrpp_module_la_SOURCES)

SAMPLE_DATA_FILES = data/dmrpp/README \
data/dmrpp/a2_local_twoD.h5.dmrpp \
data/dmrpp/a3_local_twoD.h5.dmrpp \
data/dmrpp/bstream_3980492644c726c40d98833e277e02da.bs \
data/dmrpp/bstream_52310773dfe8f711ca3ad4a95cbd65d8.bs \
data/dmrpp/bstream_63004b2080d84a81469dc0f113a3c843.bs \
data/dmrpp/bstream_a6fee7e3efc727e4b43ae9daf881cb58.bs \
data/dmrpp/chunked_fourD.h5 \
data/dmrpp/chunked_fourD.h5.dmrpp \
data/dmrpp/chunked_gzipped_fourD.h5 \
data/dmrpp/chunked_gzipped_fourD.h5.dmrpp \
data/dmrpp/chunked_gzipped_oneD.h5 \
data/dmrpp/chunked_gzipped_oneD.h5.dmrpp \
data/dmrpp/chunked_gzipped_threeD.h5 \
data/dmrpp/chunked_gzipped_threeD.h5.dmrpp \
data/dmrpp/chunked_gzipped_twoD.h5 \
data/dmrpp/chunked_gzipped_twoD.h5.dmrpp \
data/dmrpp/chunked_oneD.h5 \
data/dmrpp/chunked_oneD.h5.dmrpp \
data/dmrpp/chunked_oneD_uneven.h5 \
data/dmrpp/chunked_oneD_uneven.h5.dmrpp \
data/dmrpp/chunked_shuffled_fourD.h5 \
data/dmrpp/chunked_shuffled_fourD.h5.dmrpp \
data/dmrpp/chunked_shuffled_oneD.h5 \
data/dmrpp/chunked_shuffled_oneD.h5.dmrpp \
data/dmrpp/chunked_shuffled_threeD.h5 \
data/dmrpp/chunked_shuffled_threeD.h5.dmrpp \
data/dmrpp/chunked_shuffled_twoD.h5 \
data/dmrpp/chunked_shuffled_twoD.h5.dmrpp \
data/dmrpp/chunked_shufzip_fourD.h5 \
data/dmrpp/chunked_shufzip_fourD.h5.dmrpp \
data/dmrpp/chunked_shufzip_oneD.h5 \
data/dmrpp/chunked_shufzip_oneD.h5.dmrpp \
data/dmrpp/chunked_shufzip_threeD.h5 \
data/dmrpp/chunked_shufzip_threeD.h5.dmrpp \
data/dmrpp/chunked_shufzip_twoD.h5 \
data/dmrpp/chunked_shufzip_twoD.h5.dmrpp \
data/dmrpp/chunked_threeD.h5 \
data/dmrpp/chunked_threeD.h5.dmrpp \
data/dmrpp/chunked_threeD_asymmetric.h5 \
data/dmrpp/chunked_threeD_asymmetric.h5.dmrpp \
data/dmrpp/chunked_threeD_asymmetric_uneven.h5 \
data/dmrpp/chunked_threeD_asymmetric_uneven.h5.dmrpp \
data/dmrpp/chunked_twoD.h5 \
data/dmrpp/chunked_twoD.h5.dmrpp \
data/dmrpp/chunked_twoD_asymmetric.h5 \
data/dmrpp/chunked_twoD_asymmetric.h5.dmrpp \
data/dmrpp/chunked_twoD_uneven.h5 \
data/dmrpp/chunked_twoD_uneven.h5.dmrpp \
data/dmrpp/coads_climatology.bin \
data/dmrpp/coads_climatology.dmrpp \
data/dmrpp/d_int.h5 \
data/dmrpp/d_int.h5.dmrpp \
data/dmrpp/grid_1_2d.h5 \
data/dmrpp/grid_1_2d.h5.dmrpp \
data/dmrpp/grid_2_2d.h5 \
data/dmrpp/grid_2_2d.h5.dmrpp \
data/dmrpp/http_d_int.h5.dmrpp \
data/dmrpp/http_t_int_scalar.h5.dmrpp \
data/dmrpp/nc4_group_atomic.h5 \
data/dmrpp/nc4_group_atomic.h5.dmrpp \
data/dmrpp/t_float.h5 \
data/dmrpp/t_float.h5.dmrpp \
data/dmrpp/t_int_scalar.h5 \
data/dmrpp/t_int_scalar.h5.dmrpp
