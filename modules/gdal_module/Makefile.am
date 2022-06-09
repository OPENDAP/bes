
# Automake file for the gdal_handler
#
# 1/11/12 jhrg
#

AUTOMAKE_OPTIONS = foreign check-news 

SUBDIRS = reader .

# Set the module version here, in the spec file and in configure.ac
M_NAME=gdal_module
M_VER=1.0.0

AM_CPPFLAGS = $(GDAL_CFLAGS) $(DAP_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

AM_CXXFLAGS = 

if COMPILER_IS_GCC
AM_CXXFLAGS += -Wall -W -Wcast-align
endif

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -Wall -W -Wcast-align 

AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

CLEANFILES = *~ gdal.conf

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libgdal_module.la

GDAL_SRCS = GDALModule.cc GDALRequestHandler.cc

GDAL_HDRS = GDALModule.h GDALRequestHandler.h

libgdal_module_la_SOURCES = $(GDAL_SRCS) $(GDAL_HDRS)
libgdal_module_la_LDFLAGS = -avoid-version -module $(GDAL_LDFLAGS)
libgdal_module_la_LIBADD = $(LIBADD)

moduledir = $(sysconfdir)/bes/modules
module_DATA = gdal.conf

gdal.conf: gdal.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > gdal.conf
