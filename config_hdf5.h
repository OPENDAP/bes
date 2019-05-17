
// -*- C++ -*-

#ifndef _config_hdf5_h
#define _config_hdf5_h 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* GNU gcc/g++ provides a way to mark variables as unused */
#ifndef not_used

#if defined(__GNUG__) || defined(__GNUC__)
#define not_used __attribute__ ((unused))
#else
#define not_used
#endif

#endif

#endif                          /* _config_hdf5_h */
