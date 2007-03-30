
# Build the Back End Server.
#
# jhrg 9/2/05

AUTOMAKE_OPTIONS = foreign check-news
ACLOCAL_AMFLAGS = -I conf

SUBDIRS = dispatch command ppt cmdln server docs bin templates

bin_SCRIPTS = bes-config

EXTRA_DIST = apache hello_world csv-handler doxy.conf bes.spec

#########################################################################

bes-configuration-tests:
	cd cmdln && make bes-configuration-tests

#########################################################################

# docs

.PHONY: docs
docs:
	srcdir=$(srcdir) doxygen $(srcdir)/doxy.conf

###########################################################################

# Build linux RPMs

srpm: dist
	rpmbuild -ts @PACKAGE@-@PACKAGE_VERSION@.tar.gz

rpm: dist
	rpmbuild -tb @PACKAGE@-@PACKAGE_VERSION@.tar.gz

###########################################################################

# Build OS/X Packages. The strange operations with configure and dap-config
# are there so that the values built into dap-config will match the mac
# osx install dirs and not the temp directory used to build the packages

pkg:
	./configure --prefix=/usr
	cp bes-config bes-config.macosx
	./configure --prefix=`pwd`/mac_osx
	make clean install
	mv bes-config.macosx mac_osx/bin/bes-config

