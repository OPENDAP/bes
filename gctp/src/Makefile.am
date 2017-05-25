# GCTP source directory Makefile

# Boilerplate definitions
#include $(top_srcdir)/config/include.am

# Library to build
# Added 'noinst' to suppress installation of this library. jhrg 5/25/17
noinst_LIBRARIES = libGctp.a

## Normally DEFAULT_INCLUDES is supplied by Automake, but one of the
## directories included by default is $(top_builddir)/include, which
## contains several header files with the same names as files in the
## gctp/include directories.
## To work around this, override the DEFAULT_INCLUDES variable.
DEFAULT_INCLUDES = -I. -I$(srcdir)

# Include the libGCTP include directory
AM_CPPFLAGS=-fpic -I$(srcdir)/../include/

# Library source files
libGctp_a_SOURCES = gctp.c alberfor.c alberinv.c alconfor.c alconinv.c      \
          azimfor.c aziminv.c bceafor.c bceainv.c br_gctp.c ceafor.c         \
          ceainv.c cproj.c eqconfor.c eqconinv.c equifor.c equiinv.c         \
          for_init.c gnomfor.c gnominv.c goodfor.c goodinv.c gvnspfor.c      \
          gvnspinv.c hamfor.c haminv.c imolwfor.c imolwinv.c inv_init.c      \
          isinusfor.c isinusinv.c lamazfor.c lamazinv.c lamccfor.c           \
          lamccinv.c merfor.c merinv.c millfor.c millinv.c molwfor.c         \
          molwinv.c obleqfor.c obleqinv.c omerfor.c omerinv.c orthfor.c      \
          orthinv.c paksz.c polyfor.c polyinv.c psfor.c psinv.c report.c     \
          robfor.c robinv.c sinfor.c sininv.c sphdz.c somfor.c sominv.c      \
          sterfor.c sterinv.c stplnfor.c stplninv.c tmfor.c tminv.c untfz.c  \
          utmfor.c utminv.c vandgfor.c vandginv.c wivfor.c wivinv.c          \
          wviifor.c wviiinv.c

