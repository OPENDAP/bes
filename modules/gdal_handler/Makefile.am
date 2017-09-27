
# Automake file for the gdal_handler
#
# 1/11/12 jhrg
#

AUTOMAKE_OPTIONS = foreign check-news 

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=gdal_handler
M_VER=1.1.1

if DAP_MODULES
AM_CPPFLAGS = $(GDAL_CFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
else
AM_CPPFLAGS = $(BES_CPPFLAGS) $(DAP_CFLAGS) $(GDAL_CFLAGS)
LIBADD = $(BES_DAP_LIBS) $(DAP_LIBS)
endif

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
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

if BUILD_HYRAX_DEBUG
AM_CXXFLAGS += $(CXXFLAGS_DEBUG)
endif

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libgdal_module.la

SUBDIRS = . tests

GDAL_SRCS = GDALModule.cc GDALRequestHandler.cc GDALArray.cc GDALGrid.cc gdal_utils.cc

GDAL_HDRS = GDALModule.h GDALRequestHandler.h GDALTypes.h gdal_utils.h

libgdal_module_la_SOURCES = $(GDAL_SRCS) $(GDAL_HDRS)
libgdal_module_la_LDFLAGS = -avoid-version -module $(GDAL_LDFLAGS)
libgdal_module_la_LIBADD = $(LIBADD)

EXTRA_PROGRAMS = 

EXTRA_DIST = gdal.conf.in data

CLEANFILES = *~ gdal.conf

# Sample data primaries for install
sample_datadir = $(datadir)/hyrax/data/gdal
sample_data_DATA = data/README \
    data/Atlantic.wind.grb.bz2 \
    data/Caribbean.wind.grb.bz2 \
    data/EastAtlantic.wind.grb.bz2 \
    data/CentralAtlantic.wind.grb.bz2 \
    data/cea.tif \
    data/cea.txt \
    data/DIGEST_Example_2.20120926112517.tif \
    data/SP27GTIF.TIF \
    data/SP27GTIF.lgo \
    data/DIGEST_Example_2.20120926112501.jp2

install-data-local: gdal.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/gdal.conf \
		&& mv -f $(DESTDIR)$(sysconfdir)/bes/modules/gdal.conf $(DESTDIR)$(sysconfdir)/bes/modules/gdal.conf.$$currdate; \
	$(INSTALL_DATA) gdal.conf $(DESTDIR)$(sysconfdir)/bes/modules/gdal.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/gdal.conf

gdal.conf: gdal.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > gdal.conf
