// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
// 
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// -*- C++ -*-

// (c) COPYRIGHT URI/MIT 1998
// Please first read the full copyright statement in the file COPYRIGH.  
//
// Authors:
//      jhrg,jimg       James Gallagher (jgallagher@gso.uri.edu)

#ifndef _HDFDODSFilter_h
#define _HDFDODSFilter_h

#include "DODSFilter.h"

#include <string>

using namespace libdap;

/**
   An overload of DODSFilter, this does special processing for the HDF
   server's filter program.

   @memo Special methods for the HDF server filter programs.
   @author jhrg 9/25/98 */

class HDFDODSFilter:DODSFilter {
  private:
    HDFDODSFilter() {
  } // Private default ctor. public:
  /** Create an instance of HDFDODSFilter using the command line
      arguments passed by the CGI (or other) program.

      @memo HDFDODSFilter constructor.
      */
     HDFDODSFilter(int argc, char *argv[]);

    virtual ~ HDFDODSFilter();

  /** Given that the hdf server caches files using pathanmes (a file with
      pathname /data/hdf/file.hdf.gz goes in the cache as data#hdf#file.hdf),
      return the file name part of the string. This actually only happens
      when a file is decompressed first, then moved to the cache. 

      @memo Get the dataset name. 
      @see get_data_name 
      @return A string object that contains the name of the dataset. */
    string cache_get_dataset_name();
};

#endif                          // _DODSFilter_h

