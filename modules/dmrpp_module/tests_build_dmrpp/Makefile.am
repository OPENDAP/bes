
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)

CLEANFILES =

# Parallel make clean fail. jhrg 4/13/22 $(TESTSUITE_DMRPP)

EXTRA_DIST = get_dmrpp_baselines $(srcdir)/package.m4 $(TESTSUITE_DMRPP) $(TESTSUITE_DMRPP).at \
atlocal.in build_dmrpp_macros.m4 test_bes.conf.in

DISTCLEANFILES = atconfig test_bes.conf a.out chunked_oneD.h5 grid_1_2d.h5 grid_1_2d.h5.missing \
grid_2_2d_size.h5 grid_2_2d_size.h5.missing grid_2_2d_sin.h5 grid_2_2d_sin.h5.missing

noinst_DATA = $(abs_builddir)/grid_1_2d.h5 $(abs_builddir)/grid_2_2d_size.h5 $(abs_builddir)/grid_2_2d_sin.h5

############## Autotest follows #####################

AUTOM4TE = autom4te
AUTOTEST = $(AUTOM4TE) --language=autotest

TESTSUITE_DMRPP = $(srcdir)/testsuite

check-local: atconfig atlocal  $(TESTSUITE_DMRPP) $(builddir)/test_bes.conf
	if ./chkForAWS.sh; then export tsflags="$(TESTSUITEFLAGS) -j1 --s3tests=yes"; else export tsflags="$(TESTSUITEFLAGS) -j1"; fi && \
	$(SHELL) '$(TESTSUITE_DMRPP)' $${tsflags} && echo "bindir: $(bindir)"

clean-local:
	test ! -f '$(TESTSUITE_DMRPP)' || $(SHELL) '$(TESTSUITE_DMRPP)' --clean

%: %.at 
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at

$(TESTSUITE_DMRPP): $(TESTSUITE_DMRPP).at $(srcdir)/package.m4 \
$(srcdir)/build_dmrpp_macros.m4 $(top_srcdir)/modules/common/handler_tests_macros.m4

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

%.conf : %.conf.in
	@clean_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" \
		-e "s%[@]modulesdir[@]%${modulesdir}%" $< > $@

$(abs_builddir)/grid_1_2d.h5:
	@echo "# abs_top_srcdir: $(abs_top_srcdir)"
	@echo "# top_srcdir: $(top_srcdir)"
	@echo "# srcdir: $(srcdir)"
	@echo "# abs_top_builddir: $(abs_top_builddir)"
	@echo "# abs_builddir: $(abs_builddir)"
	@echo "# builddir: $(builddir)"
	@echo "# Copying grid_1_2d.h5 from: '$(top_srcdir)/modules/dmrpp_module/data/dmrpp/grid_1_2d.h5' to '$(abs_builddir)/grid_1_2d.h5'"
	cp $(top_srcdir)/modules/dmrpp_module/data/dmrpp/grid_1_2d.h5 $(abs_builddir)/grid_1_2d.h5
	chmod -R ug+rw $(abs_builddir)/grid_1_2d.h5

$(abs_builddir)/grid_2_2d_size.h5:
	@echo "# abs_top_srcdir: $(abs_top_srcdir)"
	@echo "# top_srcdir: $(top_srcdir)"
	@echo "# srcdir: $(srcdir)"
	@echo "# abs_top_builddir: $(abs_top_builddir)"
	@echo "# abs_builddir: $(abs_builddir)"
	@echo "# builddir: $(builddir)"
	@echo "# Copying grid_2_2d_size.h5 from: '$(top_srcdir)/modules/dmrpp_module/data/dmrpp/grid_2_2d_size.h5' to '$(abs_builddir)/grid_2_2d_size.h5'"
	cp $(top_srcdir)/modules/dmrpp_module/data/dmrpp/grid_2_2d_size.h5 $(abs_builddir)/grid_2_2d_size.h5
	chmod -R ug+rw $(abs_builddir)/grid_2_2d_size.h5

$(abs_builddir)/grid_2_2d_sin.h5:
	@echo "# abs_top_srcdir: $(abs_top_srcdir)"
	@echo "# top_srcdir: $(top_srcdir)"
	@echo "# srcdir: $(srcdir)"
	@echo "# abs_top_builddir: $(abs_top_builddir)"
	@echo "# abs_builddir: $(abs_builddir)"
	@echo "# builddir: $(builddir)"
	@echo "# Copying grid_2_2d_sin.h5 from: '$(top_srcdir)/modules/dmrpp_module/data/dmrpp/grid_2_2d_sin.h5' to '$(abs_builddir)/grid_2_2d_sin.h5'"
	cp $(top_srcdir)/modules/dmrpp_module/data/dmrpp/grid_2_2d_sin.h5 $(abs_builddir)/grid_2_2d_sin.h5
	chmod -R ug+rw $(abs_builddir)/grid_2_2d_sin.h5

