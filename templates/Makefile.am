
# Build libbes_dispatch and libdap_module, part of the BES code.

AUTOMAKE_OPTIONS = foreign

pkgdata_templatesdir = $(pkgdatadir)/templates

pkgdata_templates_DATA = Module.cc Module.h RequestHandler.cc	\
	RequestHandler.h ResponseHandler.cc ResponseHandler.h	\
	Command.cc Command.h XMLCommand.cc XMLCommand.h		\
	ResponseNames.h _commands.cc _module.cc

EXTRA_DIST = $(pkgdata_templates_DATA)
