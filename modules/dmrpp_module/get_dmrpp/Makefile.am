AUTOMAKE_OPTIONS = foreign

SUBDIRS = unit-tests
dist_bin_SCRIPTS = gen_dmrpp_side_car get_hdf_side_car get_dmrpp_h5

check-local: cp_build_script 

cp_build_script:
	if test $(srcdir) != .; then \
	cp -f $(srcdir)/gen_dmrpp_side_car $(builddir); \
	cp -f $(srcdir)/get_hdf_side_car $(builddir); \
	cp -f $(srcdir)/get_dmrpp_h5 $(builddir); \
	fi;

clean-local:
	if test $(srcdir) != .; then \
	rm -f $(builddir)/gen_dmrpp_side_car; \
	rm -f $(builddir)/get_hdf_side_car; \
	rm -f $(builddir)/get_dmrpp_h5; \
	fi;
	
CLEANFILES =
