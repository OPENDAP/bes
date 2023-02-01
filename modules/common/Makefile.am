
# Build libmodules_common, part of the BES code.
# NOTE: This is a convenience library - it can only be used
# by code that is part of the BES repository.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch
AM_LDFLAGS = -L$(top_builddir)/dispatch

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

AM_CXXFLAGS=

include $(top_srcdir)/coverage.mk

noinst_LTLIBRARIES = libmodules_common.la

libmodules_common_la_SOURCES = $(SRCS) $(HDRS)
libmodules_common_la_LDFLAGS =
libmodules_common_la_LIBADD =

pkginclude_HEADERS = $(HDRS) 

pkgdata_DATA =

EXTRA_DIST = handler_tests_macros.m4

DISTCLEANFILES =

SRCS = read_test_baseline.cc

HDRS = read_test_baseline.h run_tests_cppunit.h

C4_DB=$(C4_DIR)/modules_common.db
C4_HTML=$(C4_dir)/modules_common.html
C4_XM=$(C4_dir)/modules_common.xml

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	-mkdir $(C4_DIR)
	cccc --outdir=$(C4_DIR) $(SRCS)$(HDRS)
