
# Build the Back End Server Apache Module.
#
# jhrg 9/2/05

AUTOMAKE_OPTIONS = foreign check-news
ACLOCAL_AMFLAGS = -I conf
AM_CPPFLAGS = $(BES_CPPFLAGS) -I$(APACHE_DIR)/include

SUBDIRS = unit-tests

lib_LTLIBRARIES = libbes_apache_module.la

libbes_apache_module_la_SOURCES = $(SRCS) $(HDRS)
libbes_apache_module_la_LDFLAGS = -version-info $(BES_APACHE_VERSION)
# Defined here because configure does not have the value of $sysconfdir. jhrg

##############################################################################
#
# Sources and Headers

SRCS = BESApacheInterface.cc BESApacheRequests.cc BESApacheWrapper.cc mod_opendap.cc

HDRS = BESApacheInterface.h BESApacheRequests.h BESApacheWrapper.h

EXTRA_DIST = 

