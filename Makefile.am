
# Build the Back End Server.
#
# jhrg 9/2/05
#
# Parallel versus serial build times:
# A full build using 'make' on a four core single-processor laptop took:
# real    2m7.363s
# user    1m51.920s
# sys     0m13.313s
#
# on the same machine, the same build using 'make -j6' took:
# real    0m48.181s
# user    3m29.876s
# sys     0m23.857s

AUTOMAKE_OPTIONS = foreign check-news subdir-objects
ACLOCAL_AMFLAGS = -I conf
AM_CXXFLAGS = 

if WITH_DEPENDENCIES
# I think this will enable distcheck to pickup the --with-dependencies
# flag on OSX but not use it on Linux (where we should be getting all
# the dependencies from RPMs). jhrg 3/18/15
#
# Not always true - some deps come from here, even on linux jhrg 4/5/15
AM_DISTCHECK_CONFIGURE_FLAGS=--with-dependencies=@ac_bes_dependencies_prefix@
endif

SUBDIRS = dispatch xmlcommand ppt server cmdln standalone docs bin templates 
DIST_SUBDIRS = $(SUBDIRS)

if LIBDAP
SUBDIRS += dap dapreader functions
endif
if DAP_MODULES
SUBDIRS += modules
endif

bin_SCRIPTS = bes-config bes-config-pkgconfig

# Yes, even though its `init_d_SCRIPTS`, we use `init_ddir`,
# with-out a second underscore.
init_ddir = $(sysconfdir)/rc.d/init.d
init_d_SCRIPTS = besd

logrotate_ddir = $(sysconfdir)/logrotate.d
logrotate_d_DATA = besd.logrotate

CLEANFILES = $(init_d_SCRIPTS)

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA = bes_dispatch.pc bes_ppt.pc bes_xml_command.pc

aclocaldir=$(datadir)/aclocal
dist_aclocal_DATA = conf/bes.m4

EXTRA_DIST = apache hello_world doxy.conf.in bes.spec \
OSX_Resources bes-config-pkgconfig besd.logrotate

server-tests:
	cd cmdln && make server-tests

.PHONY: docs
docs: doxy.conf
	doxygen doxy.conf
	(cd docs && tar -czf html.tar.gz html)

# Not needed. jhrg 1/4/16
# doxy.conf: doxy.conf.in config.status
# 	./config.status --file=doxy.conf

# cccc computes metrics like Lines of code and McCabe. It'a available
# on the web...
# For the BES, compute metrics for: dispatch xmlcommand ppt server
.PHONY: cccc
cccc:	
	for d in dispatch xmlcommand ppt server; \
		do $(MAKE) -C $$d $(MFLAGS) cccc; \
	done

###########################################################################

# Fortify targets.

# SUBDIRS = dispatch command xmlcommand ppt cmdln server standalone docs bin templates
# In the server: server, ppt, dispatch, xmlcommand
# There is porbably a better way to do this. Focus the analysis on the parts
# used to build the server, not on the test drivers and documentation.

.PHONY: fortify
fortify:
	(cd dispatch && sourceanalyzer -b @PACKAGE@ $(MAKE) $(MFLAGS))
	(cd xmlcommand && sourceanalyzer -b @PACKAGE@ $(MAKE) $(MFLAGS))
	(cd ppt && sourceanalyzer -b @PACKAGE@ $(MAKE) $(MFLAGS))
	(cd server && sourceanalyzer -b @PACKAGE@ $(MAKE) $(MFLAGS))
	(cd cmdln && $(MAKE) $(MFLAGS))
	(cd standalone && $(MAKE) $(MFLAGS))
	(cd docs && $(MAKE) $(MFLAGS))
	(cd bin && $(MAKE) $(MFLAGS))
	(cd templates && $(MAKE) $(MFLAGS))
	(cd dap && sourceanalyzer -b @PACKAGE@ $(MAKE) $(MFLAGS))
	(cd dapreader && sourceanalyzer -b @PACKAGE@ $(MAKE) $(MFLAGS))
	(cd functions && sourceanalyzer -b @PACKAGE@ $(MAKE) $(MFLAGS))
	(cd modules && sourceanalyzer -b @PACKAGE@ $(MAKE) $(MFLAGS))
	sourceanalyzer -b @PACKAGE@ -scan -f @PACKAGE@-@PACKAGE_VERSION@.fpr

# Use this to clean the fortify project.
.PHONY: fortifyclean
fortifyclean:
	sourceanalyzer -b @PACKAGE@ -clean

##########################################################################

# This hack strips out the individual handlers' spec files from the
# big tar and lets us use the old target again, which means I don't
# have to code the rpmbuild root into a target.
dist-hook:
	rm -rf `find $(distdir) -name '*.spec' -a ! -name bes.spec `

# Build linux RPMs

srpm: dist
	rpmbuild -ts --clean $(RPM_OPTIONS) @PACKAGE@-@PACKAGE_VERSION@.tar.gz

# NB: 'dist' builds a tar.gz package using automake in the CWD.
rpm: dist
	rpmbuild -tb --clean $(RPM_OPTIONS) @PACKAGE@-@PACKAGE_VERSION@.tar.gz

# This target is pretty specific to our need to build a RPM for use at
# GSFC and JPL. We are assuming that the user has the rpmbuild dirs.
#
# Note: NASA is now using the all-static-rpm build and this is deprecated.
# jhrg 1/25/16
nasarpm: dist
	cp @PACKAGE@-@PACKAGE_VERSION@.tar.gz ~/rpmbuild/SOURCES
	rpmbuild -bb $(RPM_OPTIONS) bes.spec.nasa

# This target is pretty specific to our need to build a RPM for use at
# GSFC and JPL. We are assuming that the user has the rpmbuild dirs.
# This will link with everything it can find in the local deps dir,
# removing the need for EPEL
all-static-rpm: dist
	cp @PACKAGE@-@PACKAGE_VERSION@.tar.gz ~/rpmbuild/SOURCES
	rpmbuild -bb $(RPM_OPTIONS) bes.spec.all_static

# cp @PACKAGE@-@PACKAGE_VERSION@.tar.gz ~/rpmbuild/SOURCES
# rpmbuild -bb $(RPM_OPTIONS) bes.spec
#
# Old comment: 
# We used to use -tb and feed it the source tar.gz but with the new
# 'modules' build of the bes there are multiple .spec files in the resulting
# tar.gz and that confuses rpmbuild. Running rpmbuild with more than one
# spec file in the tar.gz results in the somewhat inscrutable error: 'two
# %prep lines'. As a work-around (and because I don't want to remove the
# handler's spec files, I'm using the modified rpm rule above). jhrg 3/18/15
#
# Update: I used dist-hook to filter out the modules' *.spec files. jhrg 
# 3/20/15


###########################################################################

# Build OS/X Packages. The strange operations with configure and make
# are there so that the values built into bes-config will match the mac
# osx install dirs and not the temp directory used to build the packages

PACKAGEMAKER=/Developer/usr/bin/packagemaker
PKG_CONF_FLAGS=

clean-pkg:
	-rm -rf mac_osx @PACKAGE@-@PACKAGE_VERSION@.pkg @PACKAGE@-@PACKAGE_VERSION@

pkg-build: clean-pkg
	./configure --prefix=$(prefix) --disable-dependency-tracking $(PKG_CONF_FLAGS)
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

