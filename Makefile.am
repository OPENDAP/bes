# Automake file for hdf5_handler
#
# $Id$

AUTOMAKE_OPTIONS = foreign check-news dejagnu

ACLOCAL_AMFLAGS = -I conf

bin_PROGRAMS = dap_h5_handler

lib_LTLIBRARIES = libhdf5_handler.la

if OPENDAPSERVER

lib_besdir=$(prefix)/lib/bes
lib_bes_LTLIBRARIES = libhdf5_module.la
dist_bin_SCRIPTS = configure-hdf5-data.sh

endif

HDF5SRC = HDF5Array.cc HDF5Byte.cc HDF5Float64.cc HDF5Grid.cc HDF5UInt32.cc \
	HDF5Int32.cc HDF5Sequence.cc HDF5Str.cc HDF5Structure.cc HDF5Url.cc \
	HDF5UInt16.cc HDF5Int16.cc HDF5Float32.cc HDF5TypeFactory.cc HDF5GridEOS.cc 
HDF5HDR = HDF5Array.h HDF5Float64.h HDF5Int32.h HDF5Structure.h HDF5Url.h \
	HDF5Byte.h HDF5Grid.h HDF5Sequence.h HDF5UInt16.h HDF5Float32.h \
	HDF5Int16.h HDF5Str.h HDF5UInt32.h HDF5TypeFactory.h HDF5GridEOS.h

# <hyokyung 2007.02.23. 14:19:46> <hyokyung 2007.06.14. 13:47:47>
SRCS = h5dds.cc h5das.cc H5Git.cc hdfeos.tab.cc lex.hdfeos.cc hdfeos_das.tab.cc lex.hdfeos_das.cc H5EOS.cc H5PathFinder.cc
# SRCS = h5dds.cc h5das.cc H5Git.c
HDRS = common.h h5dds.h h5das.h H5Git.h

SERVER_SRC = HDF5RequestHandler.cc HDF5Module.cc

SERVER_HDR = HDF5RequestHandler.h HDF5Module.h

libhdf5_handler_la_SOURCES = $(HDF5SRC) $(SRCS) $(HDF5HDR)  $(HDRS)

libhdf5_module_la_SOURCES = $(HDF5SRC) $(SRCS) $(HDF5HDR)  $(HDRS) $(SERVER_SRC) $(SERVER_HDR)
libhdf5_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libhdf5_module_la_LDFLAGS = -avoid-version -module 
libhdf5_module_la_LIBADD = $(BES_LIBS) -L${hdf5_path}/lib -lhdf5 -lz

dap_h5_handler_SOURCES = h5_handler.cc
dap_h5_handler_LDADD = libhdf5_handler.la -L${hdf5_path}/lib -lhdf5 -lz

DEJATOOL = dap_h5_handler

RUNTESTDEFAULTFLAGS = --tool $$tool --srcdir testsuite  # testsuite_internal 

EXTRA_DIST = data testsuite COPYING hdf5_handler.spec

CLEANFILES = *~ *.sum *.log

if OPENDAPSERVER

EXTRA_DIST += configure-hdf5-data.sh.in
CLEANFILES += configure-hdf5-data.sh
sample_datadir = $(datadir)/hyrax/data/h5
sample_data_DATA = data/tcompound.h5

do_subst = sed -e 's,[@]sysconfdir[@],$(sysconfdir),g' \
		   	   -e 's,[@]libdir[@],$(libdir),g' \
		   	   -e 's,[@]datadir[@],$(datadir),g'

configure-hdf5-data.sh: configure-hdf5-data.sh.in Makefile
	$(do_subst) < $(srcdir)/configure-hdf5-data.sh.in > configure-hdf5-data.sh
	chmod +x configure-hdf5-data.sh

# This makes sure that the handler modifies the bes  which confiure found
# even with distcheck is run.
install-data-hook: configure-hdf5-data.sh
	(bes_prefix=`bes-config --prefix` && \
	 ./configure-hdf5-data.sh $$bes_prefix/etc/bes/bes.conf $$bes_prefix/lib/bes)

endif
###########################################################################

# Build linux RPMs

srpm:
	rpmbuild -ts hdf5_handler-@PACKAGE_VERSION@.tar.gz

rpm:
	rpmbuild -tb hdf5_handler-@PACKAGE_VERSION@.tar.gz

###########################################################################

# Build the HDF-EOS parser

hdfeos.tab.cc hdfeos.tab.h: hdfeos.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -p hdfeos $<
	mv hdfeos.tab.c hdfeos.tab.cc

hdfeos_das.tab.cc hdfeos_das.tab.h: hdfeos_das.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -p hdfeos_das $<
	mv hdfeos_das.tab.c hdfeos_das.tab.cc

# Build the HDF-EOS scanner

lex.hdfeos.cc: hdfeos.lex hdfeos.tab.cc hdfeos.tab.h
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -Phdfeos $<
	mv lex.hdfeos.c lex.hdfeos.cc

lex.hdfeos_das.cc: hdfeos_das.lex hdfeos_das.tab.cc hdfeos_das.tab.h
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -Phdfeos_das $<
	mv lex.hdfeos_das.c lex.hdfeos_das.cc
