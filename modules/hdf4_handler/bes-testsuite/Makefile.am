
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror

noinst_DATA = bes.conf bes.with_hdfeos2.conf bes.with_hdfeos2_cfdmr.conf

CLEANFILES = bes.conf bes.with_hdfeos2.conf bes.with_hdfeos2_cfdmr.conf

EXTRA_DIST = $(srcdir)/hdf4_handlerTest.with_hdfeos2 \
$(srcdir)/hdf4_handlerTest.with_hdfeos2.at $(srcdir)/hdf4_handlerTest \
$(srcdir)/hdf4_handlerTest.at $(srcdir)/package.m4 atlocal.in \
bes.conf.in bes.with_hdfeos2.conf.in bes.with_hdfeos2_cfdmr.conf.in bes.conf.modules.in \
bes.with_hdfeos2.conf.modules.in generate_metadata_baseline.sh.in \
h4 h4.cf h4.nasa.with_hdfeos2 h4.with_hdfeos2

DISTCLEANFILES = generate_metadata_baseline.sh atconfig

if DAP_BUILTIN_MODULES
BES_CONF_IN = bes.conf.modules.in
BES_HDFEOS_CONF_IN = bes.with_hdfeos2.conf.modules.in
#No 'modules' version for this one.
BES_HDFEOS_CFDMR_CONF_IN = bes.with_hdfeos2_cfdmr.conf.in
else
BES_CONF_IN = bes.conf.in
BES_HDFEOS_CONF_IN = bes.with_hdfeos2.conf.in
BES_HDFEOS_CFDMR_CONF_IN = bes.with_hdfeos2_cfdmr.conf.in
endif

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes.conf: $(BES_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`${PYTHON} -c "import os; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.conf

bes.with_hdfeos2.conf: $(BES_HDFEOS_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`${PYTHON} -c "import os; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.with_hdfeos2.conf

bes.with_hdfeos2_cfdmr.conf: $(BES_HDFEOS_CFDMR_CONF_IN) $(top_srcdir)/configure.ac  
	@clean_abs_top_srcdir=`${PYTHON} -c "import os; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.with_hdfeos2_cfdmr.conf


############## Autotest follows #####################

AUTOM4TE = autom4te

if HDFEOS2 
TESTSUITE = $(srcdir)/hdf4_handlerTest.with_hdfeos2
else
TESTSUITE = $(srcdir)/hdf4_handlerTest
endif

check-local: atconfig atlocal $(srcdir)/package.m4 $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	-rm -f $(TESTSUITE) $(srcdir)/package.m4 

AUTOTEST = $(AUTOM4TE) --language=autotest

% : %.at  $(srcdir)/package.m4
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at

# The `:;' works around a Bash 3.2 bug when the output is not writable.
$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	:;{ \
	echo '# Signature of the current package.' && \
	echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])' && \
	echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])' && \
	echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])' && \
	echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])' && \
	echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	} >'$(srcdir)/package.m4'
