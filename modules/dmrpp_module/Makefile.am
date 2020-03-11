
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

build_dmrpp_LDADD = $(BES_DISPATCH_LIB) $(DAP_MODULE_OBJS) $(BES_EXTRA_LIBS) \
$(H5_LDFLAGS) $(H5_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS) $(XML2_LIBS) -lz

# ngap_build_dmrpp config
#bin_PROGRAMS = ngap_build_dmrpp 

ngap_build_dmrpp_CPPFLAGS = $(AM_CPPFLAGS) $(H5_CPPFLAGS)

ngap_build_dmrpp_SOURCES = $(BES_SRCS) $(BES_HDRS) $(BUILD_DMRPP) bes_default_conf.h ngap_build_dmrpp.cc 

ngap_build_dmrpp_LDADD =  \
    $(HDF5_MODULE_OBJS) \
    $(abs_top_builddir)/cmdln/CmdTranslation.o \
    $(abs_top_builddir)/xmlcommand/libbes_xml_command.la \
    $(BES_STANDALONE_LIB)  $(BES_DISPATCH_LIB) \
    $(DAP_MODULE_OBJS) $(BES_EXTRA_LIBS) \
    $(H5_LDFLAGS) $(H5_LIBS) $(DAP_SERVER_LIBS) \
    $(DAP_CLIENT_LIBS) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS) \
    $(XML2_LIBS) $(READLINE_LIBS) -lz

EXTRA_PROGRAMS = 

# Adding the xml2json dir here is all we need to get it in the dist tar ball.
# If it was added to DIST_SUBDIRS, some targets would be run, within the directory,
# by automake and that would break since the dir does not have automake files.
# jhrg 12/30/19
EXTRA_DIST = dmrpp.conf.in xml2json

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

DAP_MODULE_OBJS = $(top_builddir)/dap/BESDASResponseHandler.o \
$(top_builddir)/dap/BESDDSResponseHandler.o \
$(top_builddir)/dap/BESDDXResponseHandler.o \
$(top_builddir)/dap/BESDataResponseHandler.o \
$(top_builddir)/dap/BESDataDDXResponseHandler.o \
$(top_builddir)/dap/BESDMRResponseHandler.o \
$(top_builddir)/dap/BESDap4ResponseHandler.o \
$(top_builddir)/dap/BESDapRequestHandler.o \
$(top_builddir)/dap/BESDapTransmit.o \
$(top_builddir)/dap/BESDapResponse.o \
$(top_builddir)/dap/BESDASResponse.o \
$(top_builddir)/dap/BESDDSResponse.o \
$(top_builddir)/dap/BESDataDDSResponse.o \
$(top_builddir)/dap/BESDMRResponse.o \
$(top_builddir)/dap/BESDapError.o \
$(top_builddir)/dap/BESDapErrorInfo.o \
$(top_builddir)/dap/BESDapService.o \
$(top_builddir)/dap/BESDapResponseBuilder.o \
$(top_builddir)/dap/BESDapFunctionResponseCache.o \
$(top_builddir)/dap/BESStoredDapResultCache.o \
$(top_builddir)/dap/DapFunctionUtils.o \
$(top_builddir)/dap/CachedSequence.o \
$(top_builddir)/dap/CacheTypeFactory.o \
$(top_builddir)/dap/TempFile.o \
$(top_builddir)/dap/CacheMarshaller.o \
$(top_builddir)/dap/CacheUnMarshaller.o \
$(top_builddir)/dap/ObjMemCache.o \
$(top_builddir)/dap/ShowPathInfoResponseHandler.o \
$(top_builddir)/dap/GlobalMetadataStore.o

HDF5_MODULE_OBJS = \
$(top_builddir)/modules/hdf5_handler/gctp/src/bceafor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/equiinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/isinusinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/psinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/eqconinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/sininv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/inv_init.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/alberinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/merinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/vandginv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/somfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/molwinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/wviiinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/stplnfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/azimfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/polyfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/imolwinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/sterfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/untfz.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/alconfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/utmfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/wivinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/orthfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/goodinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/obleqfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/tminv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/millfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/hamfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/gnominv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/sphdz.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/robfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/lamazfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/lamccfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/gvnspfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/ceainv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/omerinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/tmfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/orthinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/wivfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/obleqinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/goodfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/utminv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/haminv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/for_init.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/millinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/report.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/robinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/gnomfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/omerfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/ceafor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/br_gctp.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/gvnspinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/lamccinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/lamazinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/cproj.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/paksz.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/alberfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/psfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/isinusfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/sinfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/eqconfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/equifor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/bceainv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/gctp.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/sominv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/molwfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/vandgfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/merfor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/stplninv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/aziminv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/polyinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/wviifor.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/alconinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/sterinv.o \
$(top_builddir)/modules/hdf5_handler/gctp/src/imolwfor.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5DiskCache.o \
$(top_builddir)/modules/hdf5_handler/.libs/he5dds.tab.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5GMCFFillIndexArray.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CFStr.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CFInt32.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5RequestHandler.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CFUInt32.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CFGeoCF1D.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5_DataMemCache.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDFEOS5CF.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CFArray.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5GMCFSpecialCVArray.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CF.o \
$(top_builddir)/modules/hdf5_handler/.libs/lex.he5das.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5GMSPCFArray.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5Array.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5BaseArray.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5GCFProduct.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5Int32.o \
$(top_builddir)/modules/hdf5_handler/.libs/lex.he5dds.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CFFloat32.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5Float32.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5GMCF.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CFGeoCFProj.o \
$(top_builddir)/modules/hdf5_handler/.libs/HE5Parser.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5UInt32.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5Url.o \
$(top_builddir)/modules/hdf5_handler/.libs/h5das.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5Sequence.o \
$(top_builddir)/modules/hdf5_handler/.libs/h5gmcfdap.o \
$(top_builddir)/modules/hdf5_handler/.libs/h5common.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5GMCFMissLLArray.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CFUInt64.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CFInt64.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5Int16.o \
$(top_builddir)/modules/hdf5_handler/.libs/h5dds.o \
$(top_builddir)/modules/hdf5_handler/.libs/HE5Checker.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5UInt16.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5Structure.o \
$(top_builddir)/modules/hdf5_handler/.libs/h5cfdap.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CFModule.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CFUtil.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDFEOS5CFSpecialCVArray.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDFEOS5CFMissNonLLCVArray.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDFEOS5CFMissLLArray.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CFFloat64.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5Float64.o \
$(top_builddir)/modules/hdf5_handler/.libs/heos5cfdap.o \
$(top_builddir)/modules/hdf5_handler/.libs/h5cfdaputil.o \
$(top_builddir)/modules/hdf5_handler/.libs/h5dmr.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5UInt64.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5Int64.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5PathFinder.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5Str.o \
$(top_builddir)/modules/hdf5_handler/.libs/h5get.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CFInt16.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CFUInt16.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5CFByte.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5Byte.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5Module.o \
$(top_builddir)/modules/hdf5_handler/.libs/h5commoncfdap.o \
$(top_builddir)/modules/hdf5_handler/.libs/he5das.tab.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5Int8.o \
$(top_builddir)/modules/hdf5_handler/.libs/HDF5GMCFMissNonLLCVArray.o
