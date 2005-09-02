
#ifndef _config_dispatch_h
#define _config_dispatch_h

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
/* Shorthand for gcc's unused attribute feature */
#if defined(__GNUG__) || defined(__GNUC__)
#define not_used __attribute__ ((unused))
#else
#define not_used
#endif /* __GNUG__ || __GNUC__ */

#endif /* _config_dispatch_h */
