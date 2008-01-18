
# Build the Back End Server.
#
# jhrg 9/2/05

AUTOMAKE_OPTIONS = foreign check-news
ACLOCAL_AMFLAGS = -I conf

SUBDIRS = dispatch command ppt cmdln server docs bin templates

bin_SCRIPTS = bes-config bes-config-pkgconfig

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA = bes_dispatch.pc bes_ppt.pc bes_command.pc bes_dap.pc

aclocaldir=$(datadir)/aclocal
dist_aclocal_DATA = conf/bes.m4

EXTRA_DIST = apache hello_world csv-handler doxy.conf bes.spec \
	INSTALL_MacOSX.rtf README_MacOSX.rtf OSX_Resources bes-config-pkgconfig

#########################################################################

bes-configuration-tests:
	cd cmdln && make bes-configuration-tests

#########################################################################

# docs

.PHONY: docs
docs:
	srcdir=$(srcdir) doxygen $(srcdir)/doxy.conf

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

##########################################################################

# Build linux RPMs

srpm: dist
	rpmbuild -ts @PACKAGE@-@PACKAGE_VERSION@.tar.gz

rpm: dist
	rpmbuild -tb @PACKAGE@-@PACKAGE_VERSION@.tar.gz

###########################################################################

# Build OS/X Packages. The strange operations with configure and dap-config
# are there so that the values built into dap-config will match the mac
# osx install dirs and not the temp directory used to build the packages

PACKAGEMAKER=/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker*

pkg:
	rm -rf mac_osx @PACKAGE@-@PACKAGE_VERSION@.pkg
	./configure --prefix=/usr $(PKG_CONFIGURE_FLAGS)
	make clean all
	DESTDIR=${PWD}/mac_osx make install
	${PACKAGEMAKER} -build -p @PACKAGE@-@PACKAGE_VERSION@.pkg -f mac_osx/usr -ds -r OSX_Resources -i OSX_Resources/Info.plist -d OSX_Resources/Description.plist

