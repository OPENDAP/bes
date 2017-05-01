
# Automake file for fileout_gdal
#
# 11/1/12 jhrg

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

SUBDIRS = . tests
# DIST_SUBDIRS = tests

# Set the module version here, in the spec file and in configure.ac
M_NAME=fileout_gdal
M_VER=0.10.1

if DAP_MODULES
AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(GDAL_CFLAGS) $(DAP_CFLAGS)
LIBADD = $(GDAL_LDFLAGS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
else
AM_CPPFLAGS = $(BES_CPPFLAGS) $(GDAL_CFLAGS)
LIBADD = $(GDAL_LDFLAGS) $(BES_DAP_LIBS)
endif

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libfong_module.la

libfong_module_la_SOURCES = $(FONG_SRC) $(FONG_HDR)
# libfong_module_la_CPPFLAGS = $(BES_CPPFLAGS) $(GDAL_CFLAGS)
libfong_module_la_LDFLAGS = -avoid-version -module 
libfong_module_la_LIBADD = $(LIBADD)

FONG_SRC = GeoTiffTransmitter.cc JPEG2000Transmitter.cc FONgRequestHandler.cc	\
	FONgModule.cc FONgTransform.cc FONgType.cc

FONG_HDR = GeoTiffTransmitter.h JPEG2000Transmitter.h FONgRequestHandler.h	\
	FONgModule.h FONgTransform.h FONgBaseType.h FONgGrid.h

EXTRA_DIST = data COPYING fong.conf.in doxy.conf

if !DAP_MODULES
EXTRA_DIST += fileout_gdal.spec 
endif

CLEANFILES = *~ fong.conf

install-data-local: fong.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/fong.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/fong.conf $(DESTDIR)$(sysconfdir)/bes/modules/fong.conf.$$currdate; \
	$(INSTALL_DATA) fong.conf $(DESTDIR)$(sysconfdir)/bes/modules/fong.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/fong.conf

fong.conf: fong.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > fong.conf

.PHONY: docs
docs:
	srcdir=$(srcdir) doxygen $(srcdir)/doxy.conf
	(cd docs && tar -czf html.tar.gz html)

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(FONG_SRC) $(FONG_HDR)

###########################################################################

# Fortify targets.

.PHONY: fortify
fortify:
	sourceanalyzer -b @PACKAGE@ $(MAKE)
	sourceanalyzer -b @PACKAGE@ -scan -f @PACKAGE@-@PACKAGE_VERSION@.fpr

# Use this to clean the fortify project.
.PHONY: fortifyclean
fortifyclean:
	sourceanalyzer -b @PACKAGE@ -clean

###########################################################################
#
# Build linux RPMs

srpm: dist
	rpmbuild -ts --clean $(RPM_OPTIONS) @PACKAGE@-@PACKAGE_VERSION@.tar.gz

rpm: dist
	rpmbuild -tb --clean $(RPM_OPTIONS) @PACKAGE@-@PACKAGE_VERSION@.tar.gz

###########################################################################

# Build OS/X Packages. The strange operations with configure and make
# are there so that the values built into dap-config will match the mac
# osx install dirs and not the temp directory used to build the packages

PACKAGEMAKER=/Developer/usr/bin/packagemaker
PKG_CONF_FLAGS=

clean-pkg:
	-rm -rf mac_osx @PACKAGE@-@PACKAGE_VERSION@.pkg

pkg-build: clean-pkg
	./configure --prefix=$(prefix) --with-gdal=$(GDAL_PATH) --disable-dependency-tracking $(PKG_CONF_FLAGS)
	make clean all
	DESTDIR=`pwd`/mac_osx make install

pkg-main: pkg-build
	./OSX_Resources/update_mac_package_contents.pl README
	cat ./OSX_Resources/InstallationCheck.proto | sed -e "s^_PREFIX_^$(prefix)^g" > foo
	mv foo ./OSX_Resources/InstallationCheck
	cat ./OSX_Resources/Info.plist.proto | \
	   sed -e "s^_PREFIX_^$(prefix)^g" \
	       -e "s^_FULL_VERSION_^@PACKAGE_VERSION@^g" \
	       -e "s^_MAJOR_VERSION_^@PACKAGE_MAJOR_VERSION@^g" \
	       -e "s^_MINOR_VERSION_^@PACKAGE_MINOR_VERSION@^g" \
	       > foo
	mv foo ./OSX_Resources/Info.plist
	${PACKAGEMAKER} --root mac_osx --id org.opendap.@PACKAGE@ \
		--title "@PACKAGE@ @PACKAGE_VERSION@" --version @PACKAGE_VERSION@ \
		--out @PACKAGE@-@PACKAGE_VERSION@.pkg --resources OSX_Resources

pkg-dmg: pkg-main
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@.dmg
	mkdir @PACKAGE@-@PACKAGE_VERSION@
	cp -r @PACKAGE@-@PACKAGE_VERSION@.pkg @PACKAGE@-@PACKAGE_VERSION@
	cp README NEWS @PACKAGE@-@PACKAGE_VERSION@
	dropdmg -i --sanitize-for-servers --format zlib @PACKAGE@-@PACKAGE_VERSION@
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@

pkg: pkg-main 






