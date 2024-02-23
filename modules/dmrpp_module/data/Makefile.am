
AUTOMAKE_OPTIONS = foreign

lib_besdir=$(libdir)/bes

dist_bin_SCRIPTS = get_dmrpp ingest_filesystem ingest_s3bucket

# Build the get_dmrpp script so that the value substituted for
# bes_modules_dir is the path to the instll bes modules. Note that
# when get_dmrpp is used for tests, we use a separate configuration
# file that references local modules.
get_dmrpp: get_dmrpp.in $(top_builddir)/config.status Makefile
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
                -e "s%[@]sysconfdir[@]%${sysconfdir}%" \
                -e "s%[@]pkglibdir[@]%${pkglibdir}%" \
                -e "s%[@]datarootdir[@]%${datarootdir}%" \
                -e "s%[@]datadir[@]%${datadir}%" \
                -e "s%[@]bindir[@]%${bindir}%" \
                -e "s%[@]get_dmrpp_version[@]%${PACKAGE_VERSION}%" \
		        -e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" \
                -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > get_dmrpp

noinst_DATA = mds_for_tests

EXTRA_DIST = get_dmrpp.in README README.md dmrpp mds_for_tests s3 s3_data fill_value deflate_twice string_arrays h4_dmrpp

CLEANFILES = get_dmrpp mds_for_tests_ledger.txt mds_ledger.txt bes.log *~

# These are defined in the top-level Makefile.am
sample_datadir = $(datadir)/hyrax/data/dmrpp
sample_data_DATA = $(SAMPLE_DATA_FILES)

SAMPLE_DATA_FILES = dmrpp/README \
dmrpp/a2_local_twoD.h5.dmrpp \
dmrpp/a3_local_twoD.h5.dmrpp \
dmrpp/bstream_3980492644c726c40d98833e277e02da.bs \
dmrpp/bstream_52310773dfe8f711ca3ad4a95cbd65d8.bs \
dmrpp/bstream_63004b2080d84a81469dc0f113a3c843.bs \
dmrpp/bstream_a6fee7e3efc727e4b43ae9daf881cb58.bs \
dmrpp/chunked_fourD.h5 \
dmrpp/chunked_fourD.h5.dmrpp \
dmrpp/chunked_gzipped_fourD.h5 \
dmrpp/chunked_gzipped_fourD.h5.dmrpp \
dmrpp/chunked_gzipped_oneD.h5 \
dmrpp/chunked_gzipped_oneD.h5.dmrpp \
dmrpp/chunked_gzipped_threeD.h5 \
dmrpp/chunked_gzipped_threeD.h5.dmrpp \
dmrpp/chunked_gzipped_twoD.h5 \
dmrpp/chunked_gzipped_twoD.h5.dmrpp \
dmrpp/chunked_oneD.h5 \
dmrpp/chunked_oneD.h5.dmrpp \
dmrpp/chunked_oneD_uneven.h5 \
dmrpp/chunked_oneD_uneven.h5.dmrpp \
dmrpp/chunked_shuffled_fourD.h5 \
dmrpp/chunked_shuffled_fourD.h5.dmrpp \
dmrpp/chunked_shuffled_oneD.h5 \
dmrpp/chunked_shuffled_oneD.h5.dmrpp \
dmrpp/chunked_shuffled_threeD.h5 \
dmrpp/chunked_shuffled_threeD.h5.dmrpp \
dmrpp/chunked_shuffled_twoD.h5 \
dmrpp/chunked_shuffled_twoD.h5.dmrpp \
dmrpp/chunked_shufzip_fourD.h5 \
dmrpp/chunked_shufzip_fourD.h5.dmrpp \
dmrpp/chunked_shufzip_oneD.h5 \
dmrpp/chunked_shufzip_oneD.h5.dmrpp \
dmrpp/chunked_shufzip_threeD.h5 \
dmrpp/chunked_shufzip_threeD.h5.dmrpp \
dmrpp/chunked_shufzip_twoD.h5 \
dmrpp/chunked_shufzip_twoD.h5.dmrpp \
dmrpp/chunked_threeD.h5 \
dmrpp/chunked_threeD.h5.dmrpp \
dmrpp/chunked_threeD_asymmetric.h5 \
dmrpp/chunked_threeD_asymmetric.h5.dmrpp \
dmrpp/chunked_threeD_asymmetric_uneven.h5 \
dmrpp/chunked_threeD_asymmetric_uneven.h5.dmrpp \
dmrpp/chunked_twoD.h5 \
dmrpp/chunked_twoD.h5.dmrpp \
dmrpp/chunked_twoD_asymmetric.h5 \
dmrpp/chunked_twoD_asymmetric.h5.dmrpp \
dmrpp/chunked_twoD_uneven.h5 \
dmrpp/chunked_twoD_uneven.h5.dmrpp \
dmrpp/coads_climatology.bin \
dmrpp/coads_climatology.dmrpp \
dmrpp/d_int.h5 \
dmrpp/d_int.h5.dmrpp \
dmrpp/grid_1_2d.h5 \
dmrpp/grid_1_2d.h5.dmrpp \
dmrpp/grid_2_2d.h5 \
dmrpp/grid_2_2d.h5.dmrpp \
dmrpp/http_d_int.h5.dmrpp \
dmrpp/http_t_int_scalar.h5.dmrpp \
dmrpp/nc4_group_atomic.h5 \
dmrpp/nc4_group_atomic.h5.dmrpp \
dmrpp/t_float.h5 \
dmrpp/t_float.h5.dmrpp \
dmrpp/t_int_scalar.h5 \
dmrpp/t_int_scalar.h5.dmrpp \
dmrpp/trust_test.dmrpp
