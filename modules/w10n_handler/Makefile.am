
# Automake file for w10n_handler
#
# 06/23/14 ndp
#

AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=w10n_handler
M_VER=1.0.8

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/xmlcommand $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

SUBDIRS = . unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libw10n_handler.la

libw10n_handler_la_SOURCES = $(W10N_SRC) $(W10N_HDR)
libw10n_handler_la_LDFLAGS = -avoid-version -module 
libw10n_handler_la_LIBADD = $(LIBADD)

W10N_SRC = w10n_utils.cc W10NModule.cc \
	W10nJsonTransform.cc W10nJsonTransmitter.cc W10nJsonRequestHandler.cc
#	ShowPathInfoResponseHandler.cc ShowPathInfoCommand.cc 

W10N_HDR = w10n_utils.h  W10NModule.h  \
	W10nJsonTransform.h W10nJsonTransmitter.h W10nJsonRequestHandler.h W10NNames.h
#	ShowPathInfoResponseHandler.h  ShowPathInfoCommand.h 

EXTRA_DIST = data w10n.conf.in

CLEANFILES = *~ w10n.conf bes.log

install-data-local: w10n.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/w10n.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/w10n.conf $(DESTDIR)$(sysconfdir)/bes/modules/w10n.conf.$$currdate; \
	$(INSTALL_DATA) w10n.conf $(DESTDIR)$(sysconfdir)/bes/modules/w10n.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/w10n.conf

w10n.conf: w10n.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > w10n.conf

# Not nearly as clean as it could be, but this removes .svn directories
# in subdirs.
dist-hook:
	rm -rf `find $(distdir) -name .svn`


C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(FONC_SRC) $(FONC_HDR)

