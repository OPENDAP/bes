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

AM_DISTCHECK_CONFIGURE_FLAGS=

if WITH_DEPENDENCIES
# With automake 1.14 setting AM_DISTCHECK... means that the user variable
# DISTCHECK_CONFIGURE_FLAGS no longer needs to be used. Earlier versions
# of automake don't support this. jhrg 6/5/17
AM_DISTCHECK_CONFIGURE_FLAGS += --with-dependencies=@ac_bes_dependencies_prefix@ --enable-developer
endif

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = dispatch xmlcommand ppt server http cmdln standalone docs bin templates hello_world

if USE_AWS_SDK
SUBDIRS += aws
endif

if LIBDAP
SUBDIRS += dap dapreader

if DAP_BUILTIN_MODULES
SUBDIRS += modules
# moved functions to the modules directory. jhrg 12.22.21
endif

endif

bin_SCRIPTS = bes-config bes-config-pkgconfig bes-tmpfiles-conf

# Yes, even though its `init_d_SCRIPTS`, we use `init_ddir`,
# without a second underscore.
init_ddir = $(sysconfdir)/rc.d/init.d
init_d_SCRIPTS = besd

logrotate_ddir = $(sysconfdir)/logrotate.d
logrotate_d_DATA = besd.logrotate

CLEANFILES = $(init_d_SCRIPTS)

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA = bes_dispatch.pc bes_ppt.pc bes_xml_command.pc

aclocaldir=$(datadir)/aclocal
dist_aclocal_DATA = conf/bes.m4

EXTRA_DIST = doxy.conf.in bes-config-pkgconfig \
bes-tmpfiles-conf besd.logrotate README.md rapidjson nlohmann pugixml

HTML_DOCS=html
DOXYGEN_CONF=doxy.conf

framework_dirs = dispatch xmlcommand ppt server http aws dap dapreader
.PHONY: framework
framework:
	for d in $(framework_dirs); do $(MAKE) -C $$d $(MFLAGS); done

# Added this as a simple way to test the framework without the modules.
# jhrg 3.18.22
.PHONY: framework-check
framework-check:
	for d in $(framework_dirs); do $(MAKE) check -C $$d $(MFLAGS); done

# This target only builds the documents, it does not move them to
# github. Use the 'gh-docs' target below for that.
.PHONY: docs
docs: ${DOXYGEN_CONF}
	doxygen ${DOXYGEN_CONF}

# GitHub.io docs 
.PHONY: gh-docs 
gh-docs:
	./build-gh-docs.sh

# cccc computes metrics like Lines of code and McCabe. It's available
# on the web...
# For the BES, compute metrics for: dispatch xmlcommand ppt server
.PHONY: cccc
cccc:	
	for d in dispatch xmlcommand ppt server; \
		do $(MAKE) -C $$d $(MFLAGS) cccc; \
	done

# This hack strips out the individual handlers' spec files from the
# big tar and lets us use the old target again, which means I don't
# have to code the rpmbuild root into a target.
dist-hook:
	-rm -rf `find $(distdir) -name '*.spec' -a ! -name bes.spec `
	-rm -rf `find $(distdir) -name 'test_config.h' `

# Build linux RPMs

srpm: dist
	rpmbuild -ts --clean $(RPM_OPTIONS) @PACKAGE@-@PACKAGE_VERSION@.tar.gz

# NB: 'dist' builds a tar.gz package using automake in the CWD.
rpm: dist
	cp @PACKAGE@-@VERSION@.tar.gz ~/rpmbuild/SOURCES
	rpmbuild -bb --clean $(RPM_OPTIONS) bes.spec

# This target is pretty specific to our need to build a RPM for use at
# NASA DAACs. We are assuming that the user has the rpmbuild dirs.
# This will link with everything it can find in the local deps dir,
# removing the need for EPEL
all-static-rpm: dist
	cp @PACKAGE@-@VERSION@.tar.gz ~/rpmbuild/SOURCES
	rpmbuild -bb $(RPM_OPTIONS) bes.spec.all_static

.PHONY: clion-source-discovery
clion-source-discovery:
	-$(MAKE) $(MFLAGS)
	-$(MAKE) $(MFLAGS) check
	true
