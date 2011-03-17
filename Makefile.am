# Automake file for hdf5_handler
#
# $Id$

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

AM_YFLAGS = -d -v
AM_LFLAGS = -8

BUILT_SOURCES = $(GRAM_SRCS)

SUBDIRS = . bes-testsuite
DIST_SUBDIRS = . bes-testsuite

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libhdf5_module.la

GRAM_SRCS = he5dds.tab.cc he5dds.tab.hh lex.he5dds.cc \
	he5das.tab.cc he5das.tab.hh lex.he5das.cc

HDF5SRC = HDF5Array.cc HDF5Byte.cc HDF5Float64.cc HDF5Grid.cc HDF5UInt32.cc \
	HDF5Int32.cc HDF5Sequence.cc HDF5Str.cc HDF5Structure.cc HDF5Url.cc \
	HDF5UInt16.cc HDF5Int16.cc HDF5Float32.cc \
	HDF5GridEOS.cc HDF5ArrayEOS.cc HE5ShortName.cc HE5CF.cc HE5CFSwath.cc \
	HE5CFGrid.cc

HDF5HDR = HDF5Array.h HDF5Float64.h HDF5Int32.h HDF5Structure.h HDF5Url.h \
	HDF5Byte.h HDF5Grid.h HDF5Sequence.h HDF5UInt16.h HDF5Float32.h \
	HDF5Int16.h HDF5Str.h HDF5UInt32.h HDF5GridEOS.h HDF5ArrayEOS.h \
        HE5ShortName.h HE5CF.h HE5CFSwath.h HE5CFGrid.h

SRCS = h5dds.cc h5das.cc h5get.cc HE5Parser.cc HDF5PathFinder.cc $(GRAM_SRCS)

HDRS = hdf5_handler.h h5dds.h h5das.h h5get.h HE5Parser.h HDF5PathFinder.h

SERVER_SRC = HDF5RequestHandler.cc HDF5Module.cc

SERVER_HDR = HDF5RequestHandler.h HDF5Module.h

libhdf5_module_la_SOURCES = $(HDF5SRC) $(SRCS) $(HDF5HDR)  $(HDRS) $(SERVER_SRC) $(SERVER_HDR)
libhdf5_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libhdf5_module_la_LDFLAGS = -avoid-version -module 
libhdf5_module_la_LIBADD = $(BES_DAP_LIBS) -L${hdf5_path}/lib -lhdf5 -lz

EXTRA_DIST = he5das.y he5dds.y he5das.lex  he5dds.lex OSX_Resources \
	data COPYING hdf5_handler.spec h5.conf.in 

CLEANFILES = *~ *.sum *.log *.output he5das.tab.cc he5das.tab.hh \
	lex.he5das.cc lex.he5dds.cc he5dds.tab.cc he5dds.tab.hh \
	h5.conf

sample_datadir = $(datadir)/hyrax/data/hdf5
sample_data_DATA = data/tcompound.h5

install-data-local: h5.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/h5.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/h5.conf $(DESTDIR)$(sysconfdir)/bes/modules/h5.conf.$$currdate; \
	$(INSTALL_DATA) h5.conf $(DESTDIR)$(sysconfdir)/bes/modules/h5.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/h5.conf

h5.conf: h5.conf.in config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > h5.conf

# Not nearly as clean as it could be, but this removes .svn directories
# in subdirs.
dist-hook:
	rm -rf `find $(distdir) -name .svn`

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libhdf5_module_la_SOURCES)

###########################################################################

# Build the HDF-EOS scanner

lex.he5dds.cc: he5dds.lex he5dds.tab.cc he5dds.tab.hh
	$(LEX) $(LFLAGS) $(AM_LFLAGS) $<


lex.he5das.cc: he5das.lex he5das.tab.cc he5das.tab.hh
	$(LEX) $(LFLAGS) $(AM_LFLAGS) $<

# Build the HDF-EOS parser

he5dds.tab.cc he5dds.tab.hh: he5dds.y HE5Parser.h
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -p he5dds -o he5dds.tab.cc $<
	if test -e he5dds.tab.cc.h; then mv he5dds.tab.cc.h he5dds.tab.hh; fi

he5das.tab.cc he5das.tab.hh: he5das.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -p he5das -o he5das.tab.cc $<
	if test -e he5das.tab.cc.h; then mv he5das.tab.cc.h he5das.tab.hh; fi

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

# Build linux RPMs

srpm: dist
	rpmbuild -ts --clean $(RPM_OPTIONS) @PACKAGE@-@PACKAGE_VERSION@.tar.gz

rpm: dist
	rpmbuild -tb --clean $(RPM_OPTIONS) @PACKAGE@-@PACKAGE_VERSION@.tar.gz

###########################################################################

# Build OS/X Packages. The strange operations with configure and make
# are there so that the values built into bes-config will match the mac
# osx install dirs and not the temp directory used to build the packages

PACKAGEMAKER=/Developer/usr/bin/packagemaker
PKG_CONF_FLAGS=

clean-pkg:
	-rm -rf mac_osx @PACKAGE@-@PACKAGE_VERSION@.pkg @PACKAGE@-@PACKAGE_VERSION@

pkg-build: clean-pkg
	echo "HDF5_PATH: " $(H5_PATH)
	./configure --prefix=$(prefix) --with-hdf5=$(H5_PATH) --disable-dependency-tracking $(PKG_CONF_FLAGS)
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

