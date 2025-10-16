
# Build libbes_aws, part of the BES code.
# NOTE: This is a convenience library - it can only be used
# by code that is part of the BES repository.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = . unit-tests

noinst_LTLIBRARIES = libbes_aws.la

# AWS_SDK_C_LIBS = -laws-c-common -laws-c-cal -laws-c-compression -laws-c-event-stream
#                 -laws-c-http -laws-c-io -laws-c-mqtt -laws-c-s3 -laws-c-auth -laws-c-sdkutils
# AWS_SDK_CPP_LIBS = -laws-crt-cpp -laws-cpp-sdk-s3 -laws-cpp-sdk-core -laws-checksums

libbes_aws_la_SOURCES = $(SRCS) $(HDRS)
libbes_aws_la_LDFLAGS =
# -L/Users/jimg/src/opendap/hyrax/build/deps/lib
libbes_aws_la_LIBADD =
#-L/Users/jimg/src/opendap/hyrax/build/deps/lib $(AWS_SDK_CPP_LIBS) $(AWS_SDK_C_LIBS)

# TODO how to deal with the convenience lib - can the AWS libs be linked to it? Prob not,
# but worth checking. jhrg 10/15/25

pkginclude_HEADERS = $(HDRS)

pkgdata_DATA =

EXTRA_DIST =

DISTCLEANFILES =

moduledir = $(sysconfdir)/bes/modules
module_DATA =

%.conf: %.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > $@

C4_DB=$(C4_DIR)/bes_aws.db
C4_HTML=$(C4_dir)/bes_aws.html
C4_XM=$(C4_dir)/bes_aws.xml

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	-mkdir $(C4_DIR)
	cccc --outdir=$(C4_DIR) $(SRCS) $(HDRS)

SRCS = AWS_SDK.cc
HDRS = AWS_SDK.h IAWS_SDK.h
