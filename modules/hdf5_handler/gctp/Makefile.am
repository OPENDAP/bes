# GCTP library top-level Makefile

# Subdirectories
# Do not install the headers for this library. jhrg 5/25/17 SUBDIRS=include src
SUBDIRS=src

# Do not modify the env var; this is handled better with AM_CPPFLAGS in 
# the src/Makefile.am. jhrg 5/25/17 CFLAGS="CFLAGS -fPIC"

# Adding 'inlcude' to EXTRA_DIST is needed for the automatically built dist
# and for the distcheck target of automake. jhrg 5/26/17
EXTRA_DIST=include