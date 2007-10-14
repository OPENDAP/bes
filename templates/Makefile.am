
# Build libbes_dispatch and libdap_module, part of the BES code.

AUTOMAKE_OPTIONS = foreign

pkgdata_templatesdir = $(pkgdatadir)/templates
pkgdata_templates_DATA = Makefile.am.template Module.cc Module.h \
	RequestHandler.cc RequestHandler.h \
	ResponseHandler.cc ResponseHandler.h \
	Command.cc Command.h ResponseNames.h \
	_commands.cc _module.cc \
	bes.conf configure.ac

pkgdata_templatesconfdir = $(pkgdatadir)/templates/conf
pkgdata_templatesconf_DATA = conf/acinclude.m4 conf/bes.m4 conf/config.guess \
	conf/config.sub conf/depcomp conf/install-sh conf/libdap.m4 \
	conf/ltmain.sh conf/missing conf/mkinstalldirs

EXTRA_DIST = Makefile.am.template Module.cc Module.h RequestHandler.cc \
RequestHandler.h ResponseHandler.cc ResponseHandler.h ResponseNames.h \
Command.cc Command.h \
_commands.cc _module.cc bes.conf configure.ac conf/acinclude.m4 conf/bes.m4 \
conf/config.guess conf/config.sub conf/depcomp conf/install-sh conf/libdap.m4 \
conf/ltmain.sh conf/missing conf/mkinstalldirs
