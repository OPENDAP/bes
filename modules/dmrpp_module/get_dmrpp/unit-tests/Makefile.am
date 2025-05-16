AUTOMAKE_OPTIONS = foreign

cp_build_script:
	if test $(srcdir) != .; then \
	cp -f `echo $(srcdir) | rev | cut -c 11- | rev`/gen_dmrpp_side_car $(builddir); \
	cp -f `echo $(srcdir) | rev | cut -c 11- | rev`/get_hdf_side_car $(builddir); \
	cp -f `echo $(srcdir) | rev | cut -c 11- | rev`/get_dmrpp_h5 $(builddir); \
	cp -fr $(srcdir)/g* $(builddir); \
	else \
	cp -f  ../$(srcdir)/gen_dmrpp_side_car $(builddir); \
	cp -f  ../$(srcdir)/get_hdf_side_car $(builddir); \
	cp -f  ../$(srcdir)/get_dmrpp_h5 $(builddir); \
	fi;

check-local: cp_build_script check-local-gen-dmrpp 

check-local-gen-dmrpp:
	python3 -m unittest gen_dmrpp_side_car_test
clean-local:
	rm -f $(builddir)/gen_dmrpp_side_car; \
	rm -f $(builddir)/get_hdf_side_car; \
	rm -f $(builddir)/get_dmrpp_h5; \
	if test $(srcdir) != .; then \
	rm -fr $(builddir)/g*; \
	fi;
