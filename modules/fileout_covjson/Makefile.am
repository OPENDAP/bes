
# Automake file for fileout_json
#
# 06/23/14 ndp
#

AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=fileout_covjson
M_VER=1.0.9

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

SUBDIRS = . unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libfocovjson_module.la

libfocovjson_module_la_SOURCES = $(FOCOVJSON_SRC) $(FOCOVJSON_HDR)
libfocovjson_module_la_LDFLAGS = -avoid-version -module
libfocovjson_module_la_LIBADD = $(LIBADD)

FOCOVJSON_SRC = FoCovJsonRequestHandler.cc FoCovJsonModule.cc FoDapCovJsonTransmitter.cc FoDapCovJsonTransform.cc  FoDapCovJsonValidation.cc\
	StreamString.cc focovjson_utils.cc

FOCOVJSON_HDR = FoCovJsonRequestHandler.h FoCovJsonModule.h FoDapCovJsonTransmitter.h FoDapCovJsonTransform.h FoDapCovJsonValidation.h\
	StreamString.h focovjson_utils.h

EXTRA_DIST = data focovjson.conf.in

CLEANFILES = *~ focovjson.conf

install-data-local: focovjson.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/focovjson.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/focovjson.conf $(DESTDIR)$(sysconfdir)/bes/modules/focovjson.conf.$$currdate; \
	$(INSTALL_DATA) focovjson.conf $(DESTDIR)$(sysconfdir)/bes/modules/focovjson.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/focovjson.conf

focovjson.conf: focovjson.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > focovjson.conf


C4_DIR=./cccc
.PHONY: cccc
cccc:
	cccc --outdir=$(C4_DIR) $(FONC_SRC) $(FONC_HDR)
