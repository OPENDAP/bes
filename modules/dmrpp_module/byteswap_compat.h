//
// Created by James Gallagher on 7/1/20.
// Always include "config.h" before this header.
//

#ifndef HYRAX_GIT_BYTESWAP_COMPAT_H
#define HYRAX_GIT_BYTESWAP_COMPAT_H

#ifndef _config_h
#error "The config.h header must be included before this header can be used"
#endif

#ifdef HAVE_BYTESWAP_H

// If this OS has byteswap.h, use it.
#include <byteswap.h>

#else

// These are the definitions used by GNULib. jhrg 7/1/20

/* Given an unsigned 16-bit argument X, return the value corresponding to
   X with reversed byte order.  */
#define bswap_16(x) ((((x) & 0x00FF) << 8) | \
                     (((x) & 0xFF00) >> 8))

/* Given an unsigned 32-bit argument X, return the value corresponding to
   X with reversed byte order.  */
#define bswap_32(x) ((((x) & 0x000000FF) << 24) | \
                     (((x) & 0x0000FF00) << 8) | \
                     (((x) & 0x00FF0000) >> 8) | \
                     (((x) & 0xFF000000) >> 24))

/* Given an unsigned 64-bit argument X, return the value corresponding to
   X with reversed byte order.  */
#define bswap_64(x) ((((x) & 0x00000000000000FFULL) << 56) | \
                     (((x) & 0x000000000000FF00ULL) << 40) | \
                     (((x) & 0x0000000000FF0000ULL) << 24) | \
                     (((x) & 0x00000000FF000000ULL) << 8) | \
                     (((x) & 0x000000FF00000000ULL) >> 8) | \
                     (((x) & 0x0000FF0000000000ULL) >> 24) | \
                     (((x) & 0x00FF000000000000ULL) >> 40) | \
                     (((x) & 0xFF00000000000000ULL) >> 56))

#endif

#endif //HYRAX_GIT_BYTESWAP_COMPAT_H
