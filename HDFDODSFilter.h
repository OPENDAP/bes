
// -*- C++ -*-

// (c) COPYRIGHT URI/MIT 1998
// Please first read the full copyright statement in the file COPYRIGH.  
//
// Authors:
//	jhrg,jimg	James Gallagher (jgallagher@gso.uri.edu)

// $Log: HDFDODSFilter.h,v $
// Revision 1.3  1999/05/06 03:23:34  jimg
// Merged changes from no-gnu branch
//
// Revision 1.2.4.1  1999/05/06 00:27:21  jimg
// Jakes String --> string changes
//
// Revision 1.2  1998/11/11 17:59:13  jimg
// *** empty log message ***
//
// Revision 1.1  1998/09/25 23:02:15  jimg
// Created.
//

#ifndef _HDFDODSFilter_h
#define _HDFDODSFilter_h

#ifdef __GNUG__
#pragma "interface"
#endif

#include "DODSFilter.h"

#include <string>

/**
   An overload of DODSFilter, this does special processing for the HDF
   server's filter program.

   @memo Special methods for the HDF server filter programs.
   @author jhrg 9/25/98 */

class HDFDODSFilter : DODSFilter {
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
      @return A string object that contains the name of the dataset. */
    string cache_get_dataset_name();
};

#endif // _DODSFilter_h
