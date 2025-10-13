
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)

AM_CXXFLAGS =

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

noinst_DATA = bes.conf bes.default.conf bes.ignore.conf bes.cfdmr.conf \
bes.e_utf8_false.conf bes.e_utf8_false.default.conf bes.e_utf8_false.dap2dmr.conf bes.e_utf8_false.enableFileID.dap2dmr.conf

CLEANFILES = bes.conf bes.default.conf bes.ignore.conf bes.cfdmr.conf \
bes.e_utf8_false.conf bes.e_utf8_false.default.conf bes.e_utf8_false.dap2dmr.conf bes.e_utf8_false.enableFileID.dap2dmr.conf

EXTRA_DIST = $(TESTSUITE).at $(TESTSUITE) atlocal.in		\
$(srcdir)/package.m4 h5.cf h5.cf.ignore h5.default h5.cf.dmr h5.nasa bes.conf.in \
bes.default.conf.in bes.ignore.conf.in bes.cfdmr.conf.in bes.conf.modules.in \
bes.default.conf.modules.in bes.ignore.conf.modules.in bes.cfdmr.conf.modules.in \
bes.e_utf8_false.conf.in bes.e_utf8_false.default.conf.in bes.e_utf8_false.dap2dmr.conf.in bes.e_utf8_false.enableFileID.dap2dmr.conf.in

# hdf5_handlerTest.nasa.at 
# Do not prefix the DISTCLEANFILES with $(srcdir) because that
# will result in a warning from distcheck
DISTCLEANFILES = atconfig

# TODO We should choose one or the other - the modules/no-modules is left over
#  from a long time ago and results in error prone behavior. DAP_BUILTIN_MODULES
#  is true for a default BES build. jhrg 3/11/22
if DAP_BUILTIN_MODULES
BES_CONF_IN = bes.conf.modules.in
BES_DEFAULT_CONF_IN = bes.default.conf.modules.in
BES_IGNORE_CONF_IN = bes.ignore.conf.modules.in
BES_CFDMR_CONF_IN = bes.cfdmr.conf.modules.in
# No 'modules' version for this one. See above. jhrg 3/12/22
BES_UTF8_FALSE_CONF_IN = bes.e_utf8_false.conf.in
BES_UTF8_FALSE_DEFAULT_CONF_IN = bes.e_utf8_false.default.conf.in
BES_UTF8_FALSE_DAP2DMR_CONF_IN = bes.e_utf8_false.dap2dmr.conf.in
BES_UTF8_FALSE_FILEID_DAP2DMR_CONF_IN = bes.e_utf8_false.enableFileID.dap2dmr.conf.in
else
BES_CONF_IN = bes.conf.in
BES_DEFAULT_CONF_IN = bes.default.conf.in
BES_IGNORE_CONF_IN = bes.ignore.conf.in
BES_CFDMR_CONF_IN = bes.cfdmr.conf.in
BES_UTF8_FALSE_CONF_IN = bes.e_utf8_false.conf.in
BES_UTF8_FALSE_DEFAULT_CONF_IN = bes.e_utf8_false.default.conf.in
BES_UTF8_FALSE_DAP2DMR_CONF_IN = bes.e_utf8_false.dap2dmr.conf.in
BES_UTF8_FALSE_FILEID_DAP2DMR_CONF_IN = bes.e_utf8_false.enableFileID.dap2dmr.conf.in
endif

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes.conf: $(BES_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`${PYTHON} -c "import os; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.conf

bes.default.conf: $(BES_DEFAULT_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`${PYTHON} -c "import os; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.default.conf

bes.ignore.conf: $(BES_IGNORE_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`${PYTHON} -c "import os; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.ignore.conf

bes.cfdmr.conf: $(BES_CFDMR_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`${PYTHON} -c "import os; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.cfdmr.conf

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using
# configure's value for the parameter when running the distcheck target.
#
# Rule for bes conf file production. jhrg 3/12/22
%.conf : %.conf.in
	@clean_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > $@

bes.e_utf8_false.conf: $(top_srcdir)/configure.ac  $(BES_UTF8_FALSE_CONF_IN)
bes.e_utf8_false.default.conf: $(top_srcdir)/configure.ac  $(BES_UTF8_FALSE_DEFAULT_CONF_IN)
bes.e_utf8_false.dap2dmr.conf: $(top_srcdir)/configure.ac  $(BES_UTF8_FALSE_DAP2DMR_CONF_IN)
bes.e_utf8_false.enableFileID.dap2dmr.conf: $(top_srcdir)/configure.ac  $(BES_UTF8_FALSE_FILEID_DAP2DMR_CONF_IN)

############## Autotest follows #####################

AUTOM4TE = autom4te

TESTSUITE = $(srcdir)/hdf5_handlerTest

# Put the bes config file dependency here because the TESTSUITE script needs
# autoconf to build but we cannot require users to have that. The conf file
# can build using only what configure and this Makefile(.am) know, so the
# dependency can go here. jhrg 3/18/22
check-local: atconfig atlocal $(srcdir)/package.m4 $(TESTSUITE) $(builddir)/bes.e_utf8_false.conf
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	-rm -f $(TESTSUITE) $(srcdir)/package.m4 

# more general rule that can be used for the testsuite production.
# jhrg 3/12/22

AUTOTEST = $(AUTOM4TE) --language=autotest
%: %.at
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at

# Only dependencies that are part of srcdir OR are included in the dist tar ball.
# See note above. jhrg 3/18/22
$(TESTSUITE): $(TESTSUITE).at $(srcdir)/package.m4

# $(TESTSUITE): $(srcdir)/hdf5_handlerTest.at $(srcdir)/package.m4
#	$(AUTOTEST) -I '$(srcdir)' -o $@.tmp $@.at
#	mv $@.tmp $@

# The `:;' works around a Bash 3.2 bug when the output is not writable.
$(srcdir)/package.m4: $(top_srcdir)/configure.ac $(srcdir)/Makefile.am
	:;{ \
	echo '# Signature of the current package.' && \
	echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])' && \
	echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])' && \
	echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])' && \
	echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])' && \
	echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])' && \
	echo 'm4_define([AT_TOP_SRCDIR],        [@top_srcdir@])' && \
	echo 'm4_define([AT_PACKAGE_HOST],      [@host@])'; \
	} >'$(srcdir)/package.m4'


