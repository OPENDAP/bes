# Automake file for ngap-specific code
#
# 07/25/18
#
AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/xmlcommand \
-I$(top_srcdir)/http -I$(top_srcdir)/pugixml/src -I$(top_srcdir)/aws $(DAP_CFLAGS)

# LDADD = $(abs_top_builddir)/aws/libbes_aws.la $(aws_libs) #TODO-delete this line
LDADD = $(aws_libs)

AM_CXXFLAGS=
AM_LDFLAGS = -L$(aws_libdir)
include $(top_srcdir)/coverage.mk

if DARWIN
AM_LDFLAGS += -Wl,-rpath,$(aws_libdir) -Wl,-rpath,@loader_path
else
AM_LDFLAGS += -Wl,-rpath,$(aws_libdir) -Wl,-rpath,'$$ORIGIN'
endif

SUBDIRS = . unit-tests

noinst_LTLIBRARIES = libngap.la
libngap_la_SOURCES = $(NGAP_SRC) $(NGAP_HDR) 
libngap_la_LDFLAGS =
libngap_la_LIBADD = $(LDADD)

NGAP_SRC = NgapOwnedContainer.cc NgapOwnedContainerStorage.cc  NgapApi.cc

NGAP_HDR = NgapNames.h NgapOwnedContainer.h NgapOwnedContainerStorage.h \
    NgapApi.h MemoryCache.h

EXTRA_DIST = ngap.conf.in

moduledir = $(sysconfdir)/bes/modules
module_DATA = ngap.conf

%.conf: %.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > $@

CLEANFILES = *~ *.gcda *.gcno *.gcov ngap.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(NGAP_SRC) $(NGAP_HDR)


