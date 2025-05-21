AUTOMAKE_OPTIONS = foreign
HDF_FILES = *.hdf
HDF5_FILES = *.h5
PYTHON_FILES = *.py
BASELINE_FILES = *.baseline

EXTRA_DIST = *.hdf *.h5 *.py *.baseline

cp_build_script:
	if test $(srcdir) != .; then \
	cp -f `echo $(srcdir) | rev | cut -c 11- | rev`/gen_dmrpp_side_car $(builddir); \
	cp -f `echo $(srcdir) | rev | cut -c 11- | rev`/get_hdf_side_car $(builddir); \
	cp -f `echo $(srcdir) | rev | cut -c 11- | rev`/get_dmrpp_h5 $(builddir); \
	cp -f $(srcdir)/$(HDF_FILES) $(builddir); \
	cp -f $(srcdir)/$(HDF5_FILES) $(builddir); \
	cp -f $(srcdir)/$(PYTHON_FILES) $(builddir); \
	cp -f $(srcdir)/$(BASELINE_FILES) $(builddir); \
	cp -f $(abs_top_srcdir)/modules/dmrpp_module/build_dmrpp_h4/get_dmrpp_h4 $(builddir); \
	cp -f $(abs_top_builddir)/modules/hdf5_handler/.libs/libhdf5_module.so $(builddir); \
	cp -f $(abs_top_builddir)/modules/hdf4_handler/.libs/libhdf4_module.so $(builddir); \
	cp -f $(abs_top_builddir)/modules/fileout_netcdf/.libs/libfonc_module.so $(builddir); \
	cp -f $(abs_top_builddir)/xmlcommand/.libs/libdap_xml_module.so $(builddir); \
	cp -f $(abs_top_builddir)/dap/.libs/libdap_module.so $(builddir); \
	else \
	cp -f  ../$(srcdir)/gen_dmrpp_side_car $(builddir); \
	cp -f  ../$(srcdir)/get_hdf_side_car $(builddir); \
	cp -f  ../$(srcdir)/get_dmrpp_h5 $(builddir); \
	fi;

check-local: cp_build_script check-local-gen-dmrpp 

check-local-gen-dmrpp:
	(PATH=$(abs_top_builddir)/modules/dmrpp_module:$(abs_top_builddir)/modules/dmrpp_module/build_dmrpp_h4:$(abs_top_builddir)/standalone:$(abs_builddir):$$PATH; python3 -m unittest gen_dmrpp_side_car_test)

clean-local:
	rm -f $(builddir)/gen_dmrpp_side_car; \
	rm -f $(builddir)/get_hdf_side_car; \
	rm -f $(builddir)/get_dmrpp_h5; \
	if test $(srcdir) != .; then \
	rm -f $(builddir)/$(HDF_FILES); \
	rm -f $(builddir)/$(HDF5_FILES); \
	rm -f $(builddir)/$(PYTHON_FILES); \
	rm -f $(builddir)/$(BASELINE_FILES); \
	rm -f $(builddir)/get_dmrpp_h4; \
	rm -f $(builddir)/libhdf5_module.so; \
	rm -f $(builddir)/libhdf4_module.so; \
	rm -f $(builddir)/libfonc_module.so; \
	rm -f $(builddir)/libdap_xml_module.so; \
	rm -f $(builddir)/libdap_module.so; \
	fi;
