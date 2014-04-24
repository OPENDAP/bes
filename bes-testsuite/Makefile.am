
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)

# AM_CPPFLAGS =
# AM_CXXFLAGS =

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

noinst_DATA = bes.conf bes.with_hdfeos2.conf

CLEANFILES = *.log *.sum site.exp

EXTRA_DIST = hdf4_handlerTest.at hdf4_handlerTest.with_hdfeos2.at	\
	 hdf4_handlerTest.nasa.with_hdfeos2.at h4 h4.with_hdfeos2	\
	 h4.nasa.with_hdfeos2 $(srcdir)/package.m4 atlocal.in		\
	 bes.conf.in bes.with_hdfeos2.conf.in				\
	 generate_metadata_baseline.sh.in $(TESTSUITE)

DISTCLEANFILES = bes.conf $(srcdir)/package.m4 hdf4_handlerTest	\
	generate_metadata_baseline.sh bes.with_hdfeos2.conf	\
	hdf4_handlerTest.with_hdfeos2

bes.conf: bes.conf.in ../config.status
	@my_topdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
		-e "s%[@]modulesdir[@]%${BES_MODULE_DIR}%" \
		-e "s%[@]sysconfdir[@]%${sysconfdir}%" \
		-e "s%[@]libdir[@]%${libdir}%" \
		-e "s%[@]datadir[@]%${datadir}%" \
		-e "s%[@]abs_top_srcdir[@]%$$my_topdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" \
		-e "s%[@]bindir[@]%${bindir}%" $< > bes.conf

bes.with_hdfeos2.conf: bes.with_hdfeos2.conf.in ../config.status
	@my_topdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
		-e "s%[@]modulesdir[@]%${BES_MODULE_DIR}%" \
		-e "s%[@]sysconfdir[@]%${sysconfdir}%" \
		-e "s%[@]libdir[@]%${libdir}%" \
		-e "s%[@]datadir[@]%${datadir}%" \
		-e "s%[@]abs_top_srcdir[@]%$$my_topdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" \
		-e "s%[@]bindir[@]%${bindir}%" $< > bes.with_hdfeos2.conf

############## Autotest follows #####################

AUTOM4TE = autom4te

if HDFEOS2 
TESTSUITE = $(srcdir)/hdf4_handlerTest.with_hdfeos2
else
TESTSUITE = $(srcdir)/hdf4_handlerTest
endif

check-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean

distclean-local:
	-rm atconfig
	-rm $(srcdir)/hdf4_handlerTest.with_hdfeos2
	-rm $(srcdir)/hdf4_handlerTest

AUTOTEST = $(AUTOM4TE) --language=autotest

% : %.at  $(srcdir)/package.m4
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at

# The `:;' works around a Bash 3.2 bug when the output is not writeable.
$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	:;{ \
	echo '# Signature of the current package.' && \
	echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])' && \
	echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])' && \
	echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])' && \
	echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])' && \
	echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	} >'$(srcdir)/package.m4'
