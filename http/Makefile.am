
# Build libbes_dispatch and libdap_module, part of the BES code.

AUTOMAKE_OPTIONS = foreign

# I added a new command to and put the ResponseHandler in xmlcommand to
# keep all of the files together. jhrg 2/8/18
AM_CPPFLAGS = -I$(top_srcdir)/dispatch $(DAP_CFLAGS)
AM_CXXFLAGS = 

# FIXME Remove this hack. Set these with configure. jhrg 11/25/19
OPENSSL_LIBS=-lcrypto
OPENSSL_INC=-I/usr/local/opt/openssl/include
OPENSSL_LDFLAGS=-L/usr/local/opt/openssl/lib

AM_CPPFLAGS += $(OPENSSL_INC)

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

# SUBDIRS = . unit-tests tests

noinst_LTLIBRARIES = libbes_http.la

libbes_http_la_SOURCES = $(SRCS) $(HDRS)
libbes_http_la_LDFLAGS =
libbes_http_la_LIBADD = $(BES_DISPATCH_LIB) $(DAP_CLIENT_LIBS) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS)

pkginclude_HEADERS = $(HDRS) 

pkgdata_DATA =

EXTRA_DIST =

DISTCLEANFILES =

##############################################################################
#
# Sources and Headers

SRCS = BESCurlUtils.cc \
    BESRemoteCache.cc \
    BESRemoteHttpResource.cc \
    BESRemoteUtils.cc

HDRS = BESCurlUtils.h \
    BESRemoteCache.h \
    BESRemoteHttpResource.h \
    BESRemoteUtils.h \
    BESProxyNames.h



C4_DB=$(C4_DIR)/http.db
C4_HTML=$(C4_dir)/http.html
C4_XM=$(C4_dir)/http.xml

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	-mkdir $(C4_DIR)
	cccc --outdir=$(C4_DIR) $(SRCS)$(HDRS)
