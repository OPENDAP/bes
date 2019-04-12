
AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = $(DAP_CFLAGS) $(BES_CPPFLAGS) -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch
AM_CXXFLAGS = 

# noinst_PROGRAMS = endian_convert multiball keepalive2

# endian_convert_SOURCES = endian_convert.cc
# endian_convert_LDADD = $(DAP_SERVER_LIBS)

# multiball_SOURCES = multiball.cc 
# multiball_LDADD = $(DAP_SERVER_LIBS) $(CURL_LIBS)

# keepalive2_SOURCES = keepalive2.cc 
# keepalive2_LDADD = $(DAP_SERVER_LIBS) $(CURL_LIBS) $(PTHREAD_LIBS)

# noinst_SCRIPTS = keepalive_stats.sh mtest.sh shreddit.sh test_reuse.sh

dist_bin_SCRIPTS = get_dmrpp ingest_filesystem ingest_s3bucket

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
%.conf : %.conf.in
	@clean_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" \
		-e "s%[@]modulesdir[@]%${modulesdir}%" $< > $@

bes.hdf5.cf.conf: $(srcdir)/bes.hdf5.cf.conf.in $(top_srcdir)/configure.ac

# Build the get_dmrpp script so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
get_dmrpp: $(srcdir)/get_dmrpp.in $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	cat $(srcdir)/get_dmrpp.in | sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" \
		-e "s%[@]modulesdir[@]%${modulesdir}%" > get_dmrpp
	

check-local: bes.hdf5.cf.conf

clean-local:
	rm -f bes.hdf5.cf.conf

EXTRA_PROGRAMS = 

noinst_DATA = mds_for_tests

EXTRA_DIST = get_dmrpp.sh README dmrpp mds_for_tests s3 s3_data

CLEANFILES = *~

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
dmrpp/t_int_scalar.h5.dmrpp
