
# Automake file for the xml_data_handler
#
# 7/27/2010 jhrg

# AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=xml_data_handler
M_VER=1.1.2

if DAP_MODULES
AM_CPPFLAGS = -I$(top_srcdir)/modules/xml_data_handler -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
else
AM_CPPFLAGS = $(BES_CPPFLAGS)
LIBADD = $(BES_DAP_LIBS)      # and ..._LIBADD
endif

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

EXTRA_DIST = COPYING INSTALL NEWS README \
	xml_data_handler.conf.in data

if !DAP_MODULES
EXTRA_DIST += xml_data_handler.spec
endif

CLEANFILES = *~ *.log *.sum
DISTCLEANFILES = xml_data_handler.conf

install-data-local: xml_data_handler.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/xml_data_handler.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/xml_data_handler.conf $(DESTDIR)$(sysconfdir)/bes/modules/xml_data_handler.conf.$$currdate; \
	$(INSTALL_DATA) xml_data_handler.conf $(DESTDIR)$(sysconfdir)/bes/modules/xml_data_handler.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/xml_data_handler.conf

xml_data_handler.conf: xml_data_handler.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > xml_data_handler.conf

C4_DIR=./cccc
C4_XD=--html_outfile=cccc-xml-data.html --xml_outfile=cccc-xml-data.xml --db_outfile=cccc-xml-data.db

.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(C4_XD) $(libxml_data_module_la_SOURCES)

###########################################################################

# Fortify targets.

.PHONY: fortify
fortify:
	sourceanalyzer -b @PACKAGE@ $(MAKE) libxml_data_module.la
	sourceanalyzer -b @PACKAGE@ -scan -f @PACKAGE@-@PACKAGE_VERSION@.fpr

# Use this to clean the fortify project.
.PHONY: fortifyclean
fortifyclean:
	sourceanalyzer -b @PACKAGE@ -clean

###########################################################################

# Build linux RPMs

srpm: dist
	rpmbuild -ts --clean $(RPM_OPTIONS) @PACKAGE@-@PACKAGE_VERSION@.tar.gz

rpm: dist
	rpmbuild -tb --clean $(RPM_OPTIONS) @PACKAGE@-@PACKAGE_VERSION@.tar.gz

###########################################################################

# Build OS/X Packages.

PACKAGEMAKER=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker*

UNIV_ARCH=-arch ppc -arch i386 -arch x86_64
MAC_SYSROOT=Developer/SDKs/MacOSX10.4u.sdk

PKG_CONF_FLAGS=

clean-pkg:
	-rm -rf mac_osx @PACKAGE@-@PACKAGE_VERSION@.pkg @PACKAGE@-@PACKAGE_VERSION@

pkg-build: clean-pkg
	./configure --prefix=/usr --disable-dependency-tracking $(PKG_CONF_FLAGS)
	make clean all
	DESTDIR=${PWD}/mac_osx make install

pkg-main: pkg-build
	./OSX_Resources/update_mac_package_contents.pl README "configure.ac" "mac_osx/usr"
	${PACKAGEMAKER} -build -p @PACKAGE@-@PACKAGE_VERSION@.pkg \
		-f mac_osx/usr \
		-ds -r OSX_Resources \
		-i OSX_Resources/Info.plist \
		-d OSX_Resources/Description.plist
	-rm -rf mac_osx

pkg-dmg: pkg-main
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@.dmg
	mkdir @PACKAGE@-@PACKAGE_VERSION@
	mv @PACKAGE@-@PACKAGE_VERSION@.pkg @PACKAGE@-@PACKAGE_VERSION@
	cp README NEWS @PACKAGE@-@PACKAGE_VERSION@
	dropdmg -i --sanitize-for-servers --format zlib @PACKAGE@-@PACKAGE_VERSION@
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@

pkg: pkg-main 

