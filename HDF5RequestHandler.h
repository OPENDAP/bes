// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of hdf5_handler, a data handler for the OPeNDAP data
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

/// \file HDF5RequestHandler.h
/// \brief include the entry functions to execute the handlers
///
/// It includes build_das, build_dds and build_data etc.
/// \author James Gallagher <jgallagher@opendap.org>

#ifndef I_HDF5RequestHandler_H
#define I_HDF5RequestHandler_H 1

#include <string>
#include <map>

#include "BESRequestHandler.h"

namespace libdap {
    class DAS;
    class DDS;
    class DataDDS;
}

class HDF5RequestHandler:public BESRequestHandler {
private:
    static bool usecf;

    static bool use_experimental_cache;

    // This is part of an experiment to test the performance gains from memory
    // caching these objects. jhrg 2/18/16
    static map<std::string, libdap::DAS> das_cache;
    static map<std::string, libdap::DDS> dds_cache;
    static map<std::string, libdap::DataDDS> data_dds_cache;

    static bool hdf5_build_data_with_IDs(BESDataHandlerInterface &dhi);
    static bool hdf5_build_dmr_with_IDs(BESDataHandlerInterface &dhi);

  public:
    HDF5RequestHandler(const string & name);
    virtual ~HDF5RequestHandler() { }

    static bool hdf5_build_das(BESDataHandlerInterface & dhi);
    static bool hdf5_build_dds(BESDataHandlerInterface & dhi);
    static bool hdf5_build_data(BESDataHandlerInterface & dhi);

    static bool hdf5_build_dmr(BESDataHandlerInterface & dhi);

    static bool hdf5_build_help(BESDataHandlerInterface & dhi);
    static bool hdf5_build_version(BESDataHandlerInterface & dhi);
};

#endif
