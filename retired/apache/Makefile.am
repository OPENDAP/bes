
# Build the Back End Server Apache Module.
#
# jhrg 9/2/05

AUTOMAKE_OPTIONS = foreign check-news
ACLOCAL_AMFLAGS = -I conf
AM_CPPFLAGS = $(BES_CPPFLAGS) -I$(APACHE_DIR)/src/include -I$(APACHE_DIR)/src/os/unix

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = unit-tests

noinst_LTLIBRARIES = libbes_apache_module.la

noinst_DATA = libopendap.module Makefile.tmpl

libbes_apache_module_la_SOURCES = $(SRCS) $(HDRS)
libbes_apache_module_la_LDFLAGS = -version-info $(BES_APACHE_VERSION)

##############################################################################
#
# Sources and Headers
#
##############################################################################
SRCS = BESApacheInterface.cc BESApacheRequests.cc BESApacheWrapper.cc \
mod_opendap.cc BESProcessEncodedString.cc

HDRS = BESApacheInterface.h BESApacheRequests.h BESApacheWrapper.h \
BESProcessEncodedString.h

APACHE_CONFIG = Makefile.libdir Makefile.tmpl libopendap.module

EXTRA_DIST = Makefile.libdir Makefile.tmpl libopendap.module.in Makefile.tmpl.in

DISTCLEANFILES = libopendap.module Makefile.tmpl

libopendap.module: libopendap.module.in config.status
	sed -e "s%[@]BES_PREFIX[@]%${BES_PREFIX}%g" $< > libopendap.module

Makefile.tmpl: Makefile.tmpl.in config.status
	sed -e "s%[@]BES_PREFIX[@]%${BES_PREFIX}%g" $< > Makefile.tmpl

install-data-local: libopendap.module
	test -z "$(APACHE_DIR)/src/modules/opendap" || $(mkdir_p) "$(APACHE_DIR)/src/modules/opendap"
	list='$(SRCS) $(HDRS) $(APACHE_CONFIG)'; for p in $$list; do \
	  if test -f $$p; then \
	    f=$(am__strip_dir) \
	    $(INSTALL) $$p $(APACHE_DIR)/src/modules/opendap/$$f; \
	    chmod -x $(APACHE_DIR)/src/modules/opendap/$$f/$$p; \
	  fi; \
	done

