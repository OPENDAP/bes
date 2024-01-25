
# Automake file for functions
#
# 01/28/2013 Hacked up by jhrg
#

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
# Removed jhrg 6/9/22 $(NC_CPPFLAGS)  $(GDAL_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
# $(DAP_LIBS)  $(NC_LDFLAGS) $(NC_LIBS) $(GDAL_LDFLAGS)
# Removed jhrg 6/9/22

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
# ./configure --disable-shared CXXFLAGS=...
# or ./configure --enable-developer --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
#
# Added -Wno-deprecated to quite warings about auto_ptr<>. jhrg 11/6/19
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Wno-deprecated

AM_CXXFLAGS =
# Removed jhrg 5/31/22 -Wno-c++14-extensions
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

if BES_DEVELOPER
AM_CXXFLAGS += $(CXXFLAGS_DEBUG)
endif

# Set the module version here, in the spec file and in configure.ac
M_NAME=functions
M_VER=1.1.1

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

SUBDIRS = . unit-tests tests
DISTDIRS = unit-tests tests stare

if BUILD_STARE
SUBDIRS += stare
LIBADD += $(H5_LDFLAGS) $(H5_LIBS) $(STARE_LDFLAGS) $(STARE_LIBS) $(NC_LDFLAGS) $(NC_LIBS)
endif

# For now, do not include GDAL. Once the GDAL linking issues on RHEL8 are understood/solved
# add this back into the code. See also DapFunctions.cc. jhrg 6/11/22
# if BUILD_GDAL
# AM_CPPFLAGS +=  $(GDAL_CFLAGS)
# LIBADD +=  $(GDAL_LDFLAGS)
# endif

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libfunctions_module.la

BUILT_SOURCES = gse.tab.cc gse.tab.hh lex.gse.cc

SRCS = grid_utils.cc DapFunctions.cc GeoConstraint.cc			\
GridGeoConstraint.cc gse.tab.cc lex.gse.cc GSEClause.cc			\
GeoGridFunction.cc GridFunction.cc LinearScaleFunction.cc		\
VersionFunction.cc MakeArrayFunction.cc MakeMaskFunction.cc		\
DilateArrayFunction.cc BindNameFunction.cc BindShapeFunction.cc		\
TabularFunction.cc TabularSequence.cc BBoxFunction.cc RoiFunction.cc	\
roi_util.cc BBoxUnionFunction.cc Odometer.cc MaskArrayFunction.cc	\
RangeFunction.cc functions_util.cc BBoxCombFunction.cc IdentityFunction.cc	\
DapFunctionsRequestHandler.cc

HDRS = grid_utils.h DapFunctions.h GeoConstraint.h			\
GridGeoConstraint.h gse.tab.hh gse_parser.h GSEClause.h			\
GeoGridFunction.h GridFunction.h LinearScaleFunction.h			\
VersionFunction.h MakeArrayFunction.h MakeMaskFunction.h		\
DilateArrayFunction.h BindNameFunction.h BindShapeFunction.h		\
TabularFunction.h TabularSequence.h BBoxFunction.h RoiFunction.h	\
roi_util.h BBoxUnionFunction.h Odometer.h MaskArrayFunction.h		\
RangeFunction.h functions_util.h DapFunctionsRequestHandler.h		\
BBoxCombFunction.h TestFunction.h IdentityFunction.h

if BUILD_STARE
SRCS += stare/StareFunctions.cc stare/GeoFile.cc
HDRS += stare/StareFunctions.h stare/GeoFile.h
endif

# See above. jhrg 6/11/22
# if BUILD_GDAL
# SRCS += scale_util.cc ScaleGrid.cc
# HDRS += ScaleGrid.h
# endif

libfunctions_module_la_SOURCES = $(SRCS) $(HDRS)
libfunctions_module_la_LDFLAGS = -avoid-version -module
libfunctions_module_la_LIBADD = $(LIBADD)

# Don't install the headers for this kind of module.
pkginclude_HEADERS = $(HDRS) 

# Build the grid selection sub_expression scanner and parser

lex.gse.cc: gse.lex gse.tab.cc gse.tab.hh
	$(LEX) $(LFLAGS) $<

gse.tab.cc gse.tab.hh: gse.yy
	$(YACC) $(YFLAGS) $<

EXTRA_DIST = functions.conf.in gse.yy gse.lex

CLEANFILES = *~ functions.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = functions.conf

functions.conf: functions.conf.in $(top_builddir)/config.status
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
                -e "s%[@]sysconfdir[@]%${sysconfdir}%" \
                -e "s%[@]pkglibdir[@]%${pkglibdir}%" \
                -e "s%[@]datarootdir[@]%${datarootdir}%" \
                -e "s%[@]datadir[@]%${datadir}%" \
                -e "s%[@]bindir[@]%${bindir}%" \
                -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > functions.conf

distclean-local:
	-rm gse.tab.cc gse.tab.hh lex.gse.cc

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libfunctions_module_la_SOURCES)

