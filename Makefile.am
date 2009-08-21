
# Build the Back End Server.
#
# jhrg 9/2/05

AUTOMAKE_OPTIONS = foreign check-news
ACLOCAL_AMFLAGS = -I conf

SUBDIRS = dispatch xmlcommand ppt cmdln server standalone docs bin templates

bin_SCRIPTS = bes-config bes-config-pkgconfig

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA = bes_dispatch.pc bes_ppt.pc bes_command.pc bes_dap.pc

aclocaldir=$(datadir)/aclocal
dist_aclocal_DATA = conf/bes.m4

EXTRA_DIST = apache hello_world csv-handler doxy.conf bes.spec \
		OSX_Resources bes-config-pkgconfig

##############################################################################

bes-configuration-tests:
	cd cmdln && make bes-configuration-tests

#########################################################################

# docs

.PHONY: docs
docs:
	srcdir=$(srcdir) doxygen $(srcdir)/doxy.conf
	(cd docs && tar -czf html.tar.gz html)

###########################################################################

# Fortify targets.

# SUBDIRS = dispatch command xmlcommand ppt cmdln server standalone docs bin templates
# In the server: server, ppt, dispatch, xmlcommand
# There is porbably a better way to do this. Focus the analysis on the parts
# used to build the server, not on the test drivers and documentation.

.PHONY: fortify
fortify:
	(cd dispatch && sourceanalyzer -b @PACKAGE@ $(MAKE))
	(cd command && $(MAKE))
	(cd xmlcommand && sourceanalyzer -b @PACKAGE@ $(MAKE))
	(cd ppt && sourceanalyzer -b @PACKAGE@ $(MAKE))
	(cd cmdln && $(MAKE))
	(cd server && sourceanalyzer -b @PACKAGE@ $(MAKE))
	(cd standalone && $(MAKE))
	(cd docs && $(MAKE))
	(cd bin && $(MAKE))
	(cd templates && $(MAKE))

	sourceanalyzer -b @PACKAGE@ -scan -f @PACKAGE@-@PACKAGE_VERSION@.fpr

# Use this to clean the fortify project.
.PHONY: fortifyclean
fortifyclean:
	sourceanalyzer -b @PACKAGE@ -clean

##########################################################################

# Build linux RPMs

srpm: dist
	rpmbuild -ts @PACKAGE@-@PACKAGE_VERSION@.tar.gz

rpm: dist
	rpmbuild -tb @PACKAGE@-@PACKAGE_VERSION@.tar.gz

###########################################################################

# Build OS/X Packages. The strange operations with configure and make
# are there so that the values built into bes-config will match the mac
# osx install dirs and not the temp directory used to build the packages

PACKAGEMAKER=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker*

UNIV_ARCH=-arch ppc -arch i386
MAC_SYSROOT=Developer/SDKs/MacOSX10.4u.sdk

PKG_CONF_FLAGS=

clean-pkg:
	-rm -rf mac_osx @PACKAGE@-@PACKAGE_VERSION@.pkg @PACKAGE@-@PACKAGE_VERSION@

pkg-build:
	./configure --prefix=/usr --disable-dependency-tracking $(PKG_CONF_FLAGS)
	make clean all
	DESTDIR=${PWD}/mac_osx make install

pkg-main:
	./OSX_Resources/update_mac_package_contents.pl README "configure.ac" "mac_osx/usr"
	${PACKAGEMAKER} -build -p @PACKAGE@-@PACKAGE_VERSION@.pkg \
		-f mac_osx/usr \
		-ds -r OSX_Resources \
		-i OSX_Resources/Info.plist \
		-d OSX_Resources/Description.plist
	-rm -rf mac_osx

pkg-dmg:
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@.dmg
	mkdir @PACKAGE@-@PACKAGE_VERSION@
	mv @PACKAGE@-@PACKAGE_VERSION@.pkg @PACKAGE@-@PACKAGE_VERSION@
	cp README NEWS @PACKAGE@-@PACKAGE_VERSION@
	dropdmg -i --sanitize-for-servers --format zlib @PACKAGE@-@PACKAGE_VERSION@
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@

pkg: clean-pkg pkg-build pkg-main pkg-dmg

