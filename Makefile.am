
# Build the Back End Server.
#
# jhrg 9/2/05

AUTOMAKE_OPTIONS = foreign check-news
ACLOCAL_AMFLAGS = -I conf

SUBDIRS = dispatch command ppt cmdln server

bin_SCRIPTS = bes-config

EXTRA_DIST = templates apache

#########################################################################
#
# docs

.PHONY: docs
docs:
	doxygen doxy.conf
