
# Automake file for w10n_handler
#
# 06/23/14 ndp
#

AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=w10n_handler
M_VER=1.1.8

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/xmlcommand $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = . unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libw10n_handler.la

libw10n_handler_la_SOURCES = $(W10N_SRC) $(W10N_HDR)
libw10n_handler_la_LDFLAGS = -avoid-version -module 
libw10n_handler_la_LIBADD = $(LIBADD)

W10N_SRC = w10n_utils.cc W10NModule.cc \
	W10nJsonTransform.cc W10nJsonTransmitter.cc W10nJsonRequestHandler.cc \
	W10nShowPathInfoResponseHandler.cc W10nShowPathInfoCommand.cc 

W10N_HDR = w10n_utils.h  W10NModule.h  \
	W10nJsonTransform.h W10nJsonTransmitter.h W10nJsonRequestHandler.h W10NNames.h \
	W10nShowPathInfoResponseHandler.h  W10nShowPathInfoCommand.h 

EXTRA_DIST = data w10n.conf.in

CLEANFILES = *~ w10n.conf bes.log

moduledir = $(sysconfdir)/bes/modules
module_DATA =  w10n.conf

w10n.conf: w10n.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > w10n.conf

# Not nearly as clean as it could be, but this removes .svn directories
# in subdirs.
dist-hook:
	rm -rf `find $(distdir) -name .svn`

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(FONC_SRC) $(FONC_HDR)

