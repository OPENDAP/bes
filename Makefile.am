
# Automake file for hdf5_handler
#
# $Id: Makefile.am 12396 2005-10-13 03:28:41 -0600 (Thu, 13 Oct 2005) jimg $

AUTOMAKE_OPTIONS = foreign check-news
# dejagnu

ACLOCAL_AMFLAGS = -I conf

bin_PROGRAMS = dap_h5_handler

if OPENDAPSERVER
    lib_LTLIBRARIES = libhdf5_handler.la libhdf5_module.la
else
    lib_LTLIBRARIES = libhdf5_handler.la
endif

HDF5SRC = HDF5Array.cc HDF5Byte.cc HDF5Float64.cc HDF5Grid.cc HDF5UInt32.cc \
	HDF5Int32.cc HDF5Sequence.cc HDF5Str.cc HDF5Structure.cc HDF5Url.cc \
	HDF5UInt16.cc HDF5Int16.cc HDF5Float32.cc HDF5TypeFactory.cc
HDF5HDR = HDF5Array.h HDF5Float64.h HDF5Int32.h HDF5Structure.h HDF5Url.h \
	HDF5Byte.h HDF5Grid.h HDF5Sequence.h HDF5UInt16.h HDF5Float32.h \
	HDF5Int16.h HDF5Str.h HDF5UInt32.h HDF5TypeFactory.h

SRCS = h5dds.cc h5das.cc H5Git.c
HDRS = common.h h5dds.h h5das.h H5Git.h version.h

SERVER_SRC = HDF5RequestHandler.cc HDF5Module.cc

SERVER_HDR = HDF5RequestHandler.h HDF5Module.h

libhdf5_handler_la_SOURCES = $(HDF5SRC) $(SRCS) $(HDF5HDR)  $(HDRS)

libhdf5_module_la_SOURCES = $(HDF5SRC) $(SRCS) $(HDF5HDR)  $(HDRS) $(SERVER_SRC) $(SERVER_HDR)
libhdf5_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libhdf5_module_la_LDFLAGS = $(BES_LDFLAGS) -L${hdf5_path}/lib -lhdf5 -lz

dap_h5_handler_SOURCES = h5_handler.cc
dap_h5_handler_LDADD = libhdf5_handler.la -L${hdf5_path}/lib -lhdf5 -lz

# DEJATOOL = dap_h5_handler

# RUNTESTDEFAULTFLAGS = --tool $$tool --srcdir 

EXTRA_DIST = 
# COPYRIGHT COPYING 
# hdf5_handler.spec

CLEANFILES = *~ *.sum *.log

###########################################################################

# Build linux RPMs

srpm:
	rpmbuild -ts hdf5_handler-@PACKAGE_VERSION@.tar.gz

rpm:
	rpmbuild -tb hdf5_handler-@PACKAGE_VERSION@.tar.gz
