AUTOMAKE_OPTIONS = foreign

cp_build_script:
	if test $(srcdir) != .; then \
	cp -fr $(srcdir)/* $(builddir); \
	fi;

check-local: cp_build_script check-local-gen-dmrpp 

check-local-gen-dmrpp:
	python3 -m unittest gen_dmrpp_side_car_test
clean-local:
	if test $(srcdir) != .; then \
	rm -fr $(builddir)/*; \
	fi;
