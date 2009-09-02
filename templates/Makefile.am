
# Build libbes_dispatch and libdap_module, part of the BES code.

AUTOMAKE_OPTIONS = foreign

pkgdata_templatesdir = $(pkgdatadir)/templates
pkgdata_templates_DATA = Makefile.am.template Module.cc Module.h	\
	RequestHandler.cc RequestHandler.h				\
	ResponseHandler.cc ResponseHandler.h				\
	Command.cc Command.h XMLCommand.cc XMLCommand.h			\
	ResponseNames.h							\
	_commands.cc _module.cc						\
	configure.ac bes-data.sh.in					\
	module.spec.template COPYRIGHT COPYING NEWS README

pkgdata_templatesconfdir = $(pkgdatadir)/templates/conf
pkgdata_templatesconf_DATA = conf/acinclude.m4 conf/bes.m4		\
	conf/libdap.m4 conf/mkinstalldirs conf/pkg.m4

EXTRA_DIST = Makefile.am.template Module.cc Module.h RequestHandler.cc	\
	RequestHandler.h ResponseHandler.cc ResponseHandler.h		\
	ResponseNames.h Command.cc Command.h XMLCommand.cc XMLCommand.h \
	_commands.cc _module.cc						\
	bes-data.sh.in configure.ac conf/acinclude.m4			\
	conf/bes.m4 conf/libdap.m4 conf/mkinstalldirs conf/pkg.m4	\
	module.spec.template COPYRIGHT COPYING NEWS README
