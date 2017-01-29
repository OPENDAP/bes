#
# 5/3/2006 jhrg
#
# $Id$

AUTOMAKE_OPTIONS = foreign check-news
ACLOCAL_AMFLAGS = -I conf

# SUBDIRS = . asciival www-interface

bin_PROGRAMS = ais_tool

ais_tool_SOURCES = ais_tool.cc AISDODSFilter.cc AISDODSFilter.h

modulesdir = $(dap_modulesdir)

cgidir = $(dap_cgidir)

nodist_cgi_SCRIPTS = nph-ais

# nodist_cgi_DATA = dap-server.rc


EXTRA_DIST = README INSTALL NEWS *.in *.spec 

stamp-nph-ais: nph-ais.in configure
	sed -e "s%[@]PERL[@]%${PERL}%" \
		-e "s%[@]prefix[@]%${prefix}%" \
		-e "s%[@]PACKAGE_VERSION[@]%${PACKAGE_VERSION}%" $< > nph-ais
	chmod +x nph-ais
	touch stamp-nph-ais

DISTCLEANFILES = nph-ais stamp-nph-ais

###########################################################################

# Build linux RPMs

srpm: dist
	rpmbuild -ts @PACKAGE@-@PACKAGE_VERSION@.tar.gz

rpm: dist
	rpmbuild -tb @PACKAGE@-@PACKAGE_VERSION@.tar.gz
