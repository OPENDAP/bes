
# Automake file for fileout_json
#
# 06/23/14 ndp
#

AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=fileout_json
M_VER=1.0.9

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

SUBDIRS = . unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libcmr_module.la

libcmr_module_la_SOURCES = $(CMR_SRC) $(CMR_HDR)
libcmr_module_la_LDFLAGS = -avoid-version -module 
libcmr_module_la_LIBADD = $(LIBADD)

CMR_SRC = CMRCatalog.cc CmrInfoCmd.cc

CMR_HDR = CMRCatalog.h CmrInfoCmd.h

EXTRA_DIST = data cmr.conf.in

CLEANFILES = *~ cmr.conf

install-data-local: cmr.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/cmr.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/cmr.conf $(DESTDIR)$(sysconfdir)/bes/modules/cmr.conf.$$currdate; \
	$(INSTALL_DATA) cmr.conf $(DESTDIR)$(sysconfdir)/bes/modules/cmr.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/cmr.conf

cmr.conf: cmr.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > cmr.conf


C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(CMR_SRC) $(CMR_HDR)


