
# Build libbes_dispatch and libdap_module, part of the BES code.

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I conf

pkgdata_templatesdir = $(pkgdatadir)/templates
pkgdata_templatesconfdir = $(pkgdatadir)/templates/conf

pkgdata_templates_DATA = Module.cc Module.h RequestHandler.cc	\
	RequestHandler.h ResponseHandler.cc ResponseHandler.h	\
	Command.cc Command.h XMLCommand.cc XMLCommand.h		\
	ResponseNames.h _commands.cc _module.cc			\
	Makefile.am.template configure.ac bes-data.sh.in	\
	module.spec.template COPYRIGHT COPYING NEWS README

pkgdata_templatesconf_DATA = conf/bes.m4 conf/libdap.m4 conf/pkg.m4

EXTRA_DIST = $(pkgdata_templates_DATA) $(pkgdata_templatesconf_DATA)
