
# Automake file for freeform-handler
#
# 9/14/2005 jhrg
#
# $Id: Makefile.am 12396 2005-10-13 03:28:41 -0600 (Thu, 13 Oct 2005) jimg $

AUTOMAKE_OPTIONS = foreign check-news dejagnu

ACLOCAL_AMFLAGS = -I conf

# SUBDIRS = 

# AM_CPPFLAGS = -DFREEFORM -I$(top_srcdir)/FFND

bin_PROGRAMS = dap_h5_handler

HDF5SRC = HDF5Array.cc HDF5Byte.cc HDF5Float64.cc HDF5Grid.cc HDF5UInt32.cc \
	HDF5Int32.cc HDF5Sequence.cc HDF5Str.cc HDF5Structure.cc HDF5Url.cc \
	HDF5UInt16.cc HDF5Int16.cc HDF5Float32.cc
HDF5HDR = HDF5Array.h HDF5Float64.h HDF5Int32.h HDF5Structure.h HDF5Url.h \
	HDF5Byte.h HDF5Grid.h HDF5Sequence.h HDF5UInt16.h HDF5Float32.h \
	HDF5Int16.h HDF5Str.h HDF5UInt32.h

SRCS = h5_handler.cc h5dds.cc H5Git.c
HDRS = common.h h5dds.h H5Git.h version.h

dap_h5_handler_SOURCES = $(HDF5SRC) $(HDF5HDR) $(SRCS) $(HDRS)
dap_h5_handler_LDADD = -L${hdf5_path}/lib -lhdf5 -lz

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
