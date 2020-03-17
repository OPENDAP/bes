
AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I conf

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
AM_CXXFLAGS =

BES_STANDALONE_LIB = -L$(top_builddir)/standalone -lbes_standalone

# FIXME Remove this hack. Set these with configure. jhrg 11/25/19
OPENSSL_LIBS=-lcrypto
OPENSSL_INC=-I/usr/local/opt/openssl/include
OPENSSL_LDFLAGS=-L/usr/local/opt/openssl/lib

AM_CPPFLAGS += $(OPENSSL_INC)

# Set the module version here, in the spec file and in configure.ac
M_NAME=dmrpp_module
M_VER=1.1.1

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
# ./configure --disable-shared CXXFLAGS=...
# or ./configure --enable-developer --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align 
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

SUBDIRS = . unit-tests tests data

BUILT_SOURCES = bes_default_conf.h

# Build the bes_default_conf script so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes_default_conf.h: $(srcdir)/bes_default_conf_template.h.in $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	cat $(srcdir)/bes_default_conf_template.h.in | sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" \
		-e "s%[@]modulesdir[@]%${modulesdir}%" > bes_default_conf.h

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libdmrpp_module.la

BES_SRCS = DMRpp.cc DmrppCommon.cc Chunk.cc CurlHandlePool.cc DmrppByte.cc DmrppArray.cc \
DmrppFloat32.cc DmrppFloat64.cc DmrppInt16.cc DmrppInt32.cc DmrppInt64.cc \
DmrppInt8.cc DmrppUInt16.cc DmrppUInt32.cc DmrppUInt64.cc DmrppStr.cc  \
DmrppStructure.cc DmrppUrl.cc DmrppD4Enum.cc DmrppD4Group.cc DmrppD4Opaque.cc \
DmrppD4Sequence.cc  DmrppTypeFactory.cc DmrppParserSax2.cc DmrppMetadataStore.cc \
CredentialsManager.cc AccessCredentials.cc NgapS3Credentials.cc \
awsv4.cc url_parser.cc \
curl_utils.cc

BES_HDRS = DMRpp.h DmrppCommon.h Chunk.h  CurlHandlePool.h DmrppByte.h \
DmrppArray.h DmrppFloat32.h DmrppFloat64.h DmrppInt16.h DmrppInt32.h \
DmrppInt64.h DmrppInt8.h DmrppUInt16.h DmrppUInt32.h DmrppUInt64.h \
DmrppStr.h DmrppStructure.h DmrppUrl.h DmrppD4Enum.h DmrppD4Group.h \
DmrppD4Opaque.h DmrppD4Sequence.h DmrppTypeFactory.h DmrppParserSax2.h \
CredentialsManager.h AccessCredentials.h NgapS3Credentials.h \
DmrppMetadataStore.h awsv4.h url_parser.h \
curl_utils.h

DMRPP_MODULE = DmrppModule.cc DmrppRequestHandler.cc DmrppModule.h DmrppRequestHandler.h

BUILD_DMRPP = DmrppRequestHandler.cc DmrppRequestHandler.h

libdmrpp_module_la_SOURCES = $(BES_HDRS) $(BES_SRCS) $(DMRPP_MODULE)
libdmrpp_module_la_LDFLAGS = -avoid-version -module
libdmrpp_module_la_LIBADD = $(BES_DISPATCH_LIB) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) $(H5_LDFLAGS) $(H5_LIBS) \
$(OPENSSL_LDFLAGS) $(OPENSSL_LIBS) -ltest-types

bin_PROGRAMS = build_dmrpp ngap_build_dmrpp

# build_dmrpp config
build_dmrpp_CPPFLAGS = $(AM_CPPFLAGS) $(H5_CPPFLAGS)

build_dmrpp_SOURCES = $(BES_SRCS) $(BES_HDRS) $(BUILD_DMRPP) build_dmrpp.cc

build_dmrpp_LDADD = $(abs_top_builddir)/dap/.libs/libdap_module.a $(BES_DISPATCH_LIB) $(BES_EXTRA_LIBS) \
$(H5_LDFLAGS) $(H5_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS) $(XML2_LIBS) -lz


ngap_build_dmrpp_CPPFLAGS = $(AM_CPPFLAGS) $(H5_CPPFLAGS)  -I$(top_srcdir)/standalone

ngap_build_dmrpp_SOURCES = $(BES_SRCS) $(BES_HDRS) $(BUILD_DMRPP) bes_default_conf.h ngap_build_dmrpp.cc 

ngap_build_dmrpp_LDADD =  \
    $(abs_top_builddir)/cmdln/CmdTranslation.o \
    $(abs_top_builddir)/xmlcommand/libbes_xml_command.la \
    $(abs_top_builddir)/dap/.libs/libdap_module.a \
    $(BES_STANDALONE_LIB)  $(BES_DISPATCH_LIB) \
    $(BES_XML_CMD_LIB) \
    $(abs_top_builddir)/modules/hdf5_handler/gctp/src/libGctp.a \
    $(BES_EXTRA_LIBS) $(H5_LDFLAGS) $(H5_LIBS) $(DAP_SERVER_LIBS) \
    $(DAP_CLIENT_LIBS) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS) \
    $(XML2_LIBS) $(READLINE_LIBS) -lz
#    $(HDF5_MODULE_OBJS)


# Cruft
# $(HDF5_MODULE_OBJS)
#   $(abs_top_builddir)/modules/hdf5_handler/libhdf5_module.la
#

EXTRA_PROGRAMS = 

# Adding the xml2json dir here is all we need to get it in the dist tar ball.
# If it was added to DIST_SUBDIRS, some targets would be run, within the directory,
# by automake and that would break since the dir does not have automake files.
# jhrg 12/30/19
EXTRA_DIST = dmrpp.conf.in xml2json rapidjson

# removed from EXTRA_DIST: libdmrpp_module.la

CLEANFILES = *~ bes_default_conf.h dmrpp.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = dmrpp.conf

dmrpp.conf: dmrpp.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > dmrpp.conf

distclean-local:

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libdmrpp_module_la_SOURCES)

HDF5_MODULE_OBJS = \
$(abs_top_builddir)/modules/hdf5_handler/HDF5Array.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5BaseArray.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5Byte.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CF.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CFArray.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CFByte.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CFFloat32.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CFFloat64.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CFGeoCF1D.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CFGeoCFProj.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CFInt16.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CFInt32.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CFInt64.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CFModule.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CFStr.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CFUInt16.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CFUInt32.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CFUInt64.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5CFUtil.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5DiskCache.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5Float32.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5Float64.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5GCFProduct.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5GMCF.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5GMCFFillIndexArray.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5GMCFMissLLArray.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5GMCFMissNonLLCVArray.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5GMCFSpecialCVArray.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5GMSPCFArray.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5Int16.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5Int32.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5Int64.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5Int8.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5Module.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5PathFinder.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5RequestHandler.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5Sequence.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5Str.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5Structure.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5UInt16.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5UInt32.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5UInt64.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5Url.o \
$(abs_top_builddir)/modules/hdf5_handler/HDF5_DataMemCache.o \
$(abs_top_builddir)/modules/hdf5_handler/HDFEOS5CF.o \
$(abs_top_builddir)/modules/hdf5_handler/HDFEOS5CFMissLLArray.o \
$(abs_top_builddir)/modules/hdf5_handler/HDFEOS5CFMissNonLLCVArray.o \
$(abs_top_builddir)/modules/hdf5_handler/HDFEOS5CFSpecialCVArray.o \
$(abs_top_builddir)/modules/hdf5_handler/HE5Checker.o \
$(abs_top_builddir)/modules/hdf5_handler/HE5Parser.o \
$(abs_top_builddir)/modules/hdf5_handler/h5cfdap.o \
$(abs_top_builddir)/modules/hdf5_handler/h5cfdaputil.o \
$(abs_top_builddir)/modules/hdf5_handler/h5common.o \
$(abs_top_builddir)/modules/hdf5_handler/h5commoncfdap.o \
$(abs_top_builddir)/modules/hdf5_handler/h5das.o \
$(abs_top_builddir)/modules/hdf5_handler/h5dds.o \
$(abs_top_builddir)/modules/hdf5_handler/h5dmr.o \
$(abs_top_builddir)/modules/hdf5_handler/h5get.o \
$(abs_top_builddir)/modules/hdf5_handler/h5gmcfdap.o \
$(abs_top_builddir)/modules/hdf5_handler/he5das.tab.o \
$(abs_top_builddir)/modules/hdf5_handler/he5dds.tab.o \
$(abs_top_builddir)/modules/hdf5_handler/heos5cfdap.o \
$(abs_top_builddir)/modules/hdf5_handler/lex.he5das.o \
$(abs_top_builddir)/modules/hdf5_handler/lex.he5dds.o
