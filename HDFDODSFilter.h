
// -*- C++ -*-

// (c) COPYRIGHT URI/MIT 1998
// Please first read the full copyright statement in the file COPYRIGH.  
//
// Authors:
//	jhrg,jimg	James Gallagher (jgallagher@gso.uri.edu)

// $Log: HDFDODSFilter.h,v $
// Revision 1.1  1998/09/25 23:02:15  jimg
// Created.
//

#ifndef _HDFDODSFilter_h
#define _HDFDODSFilter_h

#ifdef __GNUG__
#pragma "interface"
#endif

#include <String.h>

/**
   An overload of DODSFilter, this does special processing for the HDF
   server's filter program.

   @memo Special methods for the HDF server filter programs.
   @author jhrg 9/25/98 */

class DODSFilter {
private:
    HDFDODSFilter() {}		// Private default ctor.

public:
  /** Create an instance of HDFDODSFilter using the command line
      arguments passed by the CGI (or other) program.

      @memo HDFDODSFilter constructor.
      */
    HDFDODSFilter(int argc, char *argv[]);

    virtual ~HDFDODSFilter();

  /** Given that the hdf server caches files using pathanmes (a file with
      pathname /data/hdf/file.hdf.gz goes in the cache as data#hdf#file.hdf),
      return the file name part of the string. This actually only happens
      when a file is decompressed first, then moved to the cache. 

      @memo Get the dataset name. 
      @see get_data_name 
      @return A String object that contains the name of the dataset. */
    String cache_get_dataset_name();
};

#endif // _DODSFilter_h
