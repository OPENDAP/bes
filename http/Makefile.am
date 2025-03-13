
# Build libbes_http, part of the BES code.
# NOTE: This is a convenience library - it can only be used
# by code that is part of the BES repository.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = $(OPENSSL_INC) -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/rapidjson $(DAP_CFLAGS)

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = . unit-tests tests

noinst_LTLIBRARIES = libbes_http.la

libbes_http_la_SOURCES = $(SRCS) $(HDRS)
libbes_http_la_LDFLAGS =
libbes_http_la_LIBADD = $(BES_DISPATCH_LIB) $(DAP_CLIENT_LIBS) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS)

pkginclude_HEADERS = $(HDRS)

pkgdata_DATA =

EXTRA_DIST = http.conf.in data

DISTCLEANFILES = http.conf

##############################################################################
#
# Sources and Headers

SRCS = CurlUtils.cc \
    HttpError.cc \
    RemoteResource.cc \
    HttpUtils.cc \
    ProxyConfig.cc \
    EffectiveUrlCache.cc \
    url_impl.cc \
    EffectiveUrl.cc \
    AllowedHosts.cc \
    AccessCredentials.cc \
    CredentialsManager.cc \
    awsv4.cc

HDRS = CurlUtils.h \
    HttpError.h \
    RemoteResource.h \
    HttpUtils.h \
    ProxyConfig.h \
    HttpNames.h \
    EffectiveUrlCache.h \
    url_impl.h \
    EffectiveUrl.h \
    AllowedHosts.h \
    AccessCredentials.h \
    CredentialsManager.h \
    awsv4.h

# Removed jhrg 2/26/24 HttpCache.h     HttpCache.cc

moduledir = $(sysconfdir)/bes/modules
module_DATA = http.conf

http.conf: http.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > http.conf

C4_DB=$(C4_DIR)/http.db
C4_HTML=$(C4_dir)/http.html
C4_XM=$(C4_dir)/http.xml

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	-mkdir $(C4_DIR)
	cccc --outdir=$(C4_DIR) $(SRCS)$(HDRS)
