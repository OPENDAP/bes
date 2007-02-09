
# Build the Back End Server.
#
# jhrg 9/2/05

AUTOMAKE_OPTIONS = foreign check-news
ACLOCAL_AMFLAGS = -I conf

SUBDIRS = dispatch command ppt cmdln server docs bin templates

bin_SCRIPTS = bes-config

EXTRA_DIST = templates apache doxy.conf bes.spec

#########################################################################
#
# docs

.PHONY: docs
docs:
	srcdir=$(srcdir) doxygen $(srcdir)/doxy.conf
