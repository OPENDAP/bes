
# Automake file for the xml_data_handler
#
# 7/27/2010 jhrg

# AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=xml_data_handler
M_VER=1.2.5

AM_CPPFLAGS = -I$(top_srcdir)/modules/xml_data_handler -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

# These are not used by automake but are often useful for certain types of
# debugging.
CXXFLAGS_DEBUG = -g3 -O0 -Wall 
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

SUBDIRS = . unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libxml_data_module.la

libxml_data_module_la_SOURCES = $(XD_SOURCES) $(BES_SOURCES)
# libxml_data_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libxml_data_module_la_LDFLAGS = -avoid-version -module 
libxml_data_module_la_LIBADD = $(LIBADD)

# Update this in unit-tests/Makefile.am too
XD_SOURCES = XDArray.cc XDInt16.cc XDStructure.cc		   \
		XDInt32.cc XDUInt16.cc XDByte.cc XDOutput.cc   \
		XDUInt32.cc XDFloat32.cc XDUrl.cc XDFloat64.cc \
		XDSequence.cc  XDGrid.cc XDStr.cc XDArray.h	   \
		XDInt32.h XDUInt32.h XDByte.h XDOutput.h	   \
		XDUrl.h XDFloat32.h XDSequence.h XDFloat64.h   \
		XDStr.h XDGrid.h XDStructure.h XDInt16.h	   \
		XDUInt16.h XDOutputFactory.cc XDOutputFactory.h   \
		get_xml_data.cc get_xml_data.h 

BES_SOURCES = BESXDModule.cc BESXDResponseHandler.cc		   \
	      BESXDTransmit.cc BESXDRequestHandler.cc		   \
	      BESXDModule.h BESXDResponseHandler.h		   \
	      BESXDTransmit.h BESXDRequestHandler.h BESXDNames.h

EXTRA_DIST = xml_data_handler.conf.in data

CLEANFILES = *~ *.log *.sum
DISTCLEANFILES = xml_data_handler.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = xml_data_handler.conf

xml_data_handler.conf: xml_data_handler.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > xml_data_handler.conf

C4_DIR=./cccc
C4_XD=--html_outfile=cccc-xml-data.html --xml_outfile=cccc-xml-data.xml --db_outfile=cccc-xml-data.db

.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(C4_XD) $(libxml_data_module_la_SOURCES)

