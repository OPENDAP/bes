
# Build bescmdln, part of the BES code.

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = -I$(top_srcdir)/xmlcommand -I$(top_srcdir)/cmdln -I$(top_srcdir)/dispatch $(XML2_CFLAGS)

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

## FIXME Kludge replace aws_libdir with something real. jhrg 1/6/26

# Help the link step find libs at build time too (not just run time)
AM_LDFLAGS += -L$(aws_libdir)

# aws_libdir is set in configure.ac. jhrg 10/16/25
# aws_libdir64 is also set in configure.ac sbl 11/14/25

if DARWIN
AM_LDFLAGS += -Wl,-rpath,$(aws_libdir) -Wl,-rpath,@loader_path
else
AM_LDFLAGS += -Wl,-rpath,$(aws_libdir) -Wl,-rpath,'$$ORIGIN'
    ifeq ($(strip $(aws_libdir64)),)
        $(info "aws_libdir64 not set, skipping")
    else
        AM_LDFLAGS += -L$(aws_libdir64)
        AM_LDFLAGS += -Wl,-rpath,$(aws_libdir64)
    endif
endif

## END FIXME jhrg 1/6/26

EXTRA_DIST = response-split.py

# This installs the script
noinst_SCRIPTS = response-split.py

# This byte-compiles the script and installs the result in lib/python-3.9.
# Since this code cannot function as a library, I'm going to leave it out.
# python_PYTHON = response-split.py

bin_PROGRAMS = besstandalone

besstandalone_SOURCES = besstandalone.cc StandAloneApp.cc StandAloneClient.cc \
    StandAloneApp.h StandAloneClient.h

# This depends on building in bes/cmdln first. It's not a good way to write
# teh Makefile.am, but I cannot get distcheck to work otherwise. jhrg 9/8/23
besstandalone_LDADD = $(top_builddir)/cmdln/CmdTranslation.o $(top_builddir)/dispatch/libbes_dispatch.la \
    $(top_builddir)/xmlcommand/libbes_xml_command.la $(READLINE_LIBS) $(XML2_LIBS)
