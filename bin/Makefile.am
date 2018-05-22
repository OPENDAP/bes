
# Build the Back End Server.
#
# jhrg 9/2/05

AUTOMAKE_OPTIONS = foreign check-news
ACLOCAL_AMFLAGS = -I conf

bin_SCRIPTS = besCreateModule localBesGetDap populateMDS

EXTRA_DIST = besCreateModule.in localBesGetDap populateMDS
DISTCLEANFILES = besCreateModule

besCreateModule: besCreateModule.in ../config.status
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
		-e "s%[@]sysconfdir[@]%${sysconfdir}%" \
		-e "s%[@]pkglibdir[@]%${pkglibdir}%" \
		-e "s%[@]bindir[@]%${bindir}%" $< > besCreateModule

