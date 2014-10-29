
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of hdf4_handler, a data handler for the OPeNDAP data
// server. 

// Copyright (c) 2002,2003 OPeNDAP, Inc.
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
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// CDFRequestHandler.h

#ifndef I_HDF4RequestHandler_H
#define I_HDF4RequestHandler_H 1

#include <string>

using std::string;

#include "BESRequestHandler.h"

class HDF4RequestHandler:public BESRequestHandler {
  private:
    static string _cachedir;
  public:
    HDF4RequestHandler(const string & name);
    virtual ~ HDF4RequestHandler(void);

    static bool hdf4_build_das(BESDataHandlerInterface & dhi);
    static bool hdf4_build_dds(BESDataHandlerInterface & dhi);
    static bool hdf4_build_data(BESDataHandlerInterface & dhi);
    static bool hdf4_build_data_with_IDs(BESDataHandlerInterface & dhi);
#ifdef USE_DAP4
    static bool hdf4_build_dmr(BESDataHandlerInterface & dhi);
    static bool hdf4_build_dmr_with_IDs(BESDataHandlerInterface & dhi);
#endif
    static bool hdf4_build_help(BESDataHandlerInterface & dhi);
    static bool hdf4_build_version(BESDataHandlerInterface & dhi);
};


#if 0
void close_fileid(const int sdfd, const int fileid,const int gridfd, const int swathfd); 
void close_hdf4_fileid(const int sdfd,const int fileid);
#endif

#endif
