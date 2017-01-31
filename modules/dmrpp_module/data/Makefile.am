
AUTOMAKE_OPTIONS = foreign check-news

AM_CPPFLAGS = $(BES_CPPFLAGS) -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch
AM_CXXFLAGS = 

noinst_PROGRAMS = endian_convert
endian_convert_SOURCES = endian_convert.cc endian_convert.h
endian_convert_CPPFLAGS = $(DAP_CFLAGS) $(AM_CPPFLAGS)
endian_convert_LDADD = $(DAP_SERVER_LIBS)

EXTRA_PROGRAMS = 

EXTRA_DIST = 

CLEANFILES = *~ dmrpp.conf

sample_datadir = $(datadir)/hyrax/data/dmrpp
sample_data_DATA = dmrpp/*.dmrpp dmrpp/*.bin dmrpp/*.h5 dmrpp/*.bs
