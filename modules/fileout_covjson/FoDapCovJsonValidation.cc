// -*- mode: c++; c-basic-offset:4 -*-
//
// FoDapCovJsonValidation.cc
//
// This file is part of BES CovJSON File Out Module
//
// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Corey Hemphill <hemphilc@oregonstate.edu>
// Author: River Hendriksen <hendriri@oregonstate.edu>
// Author: Riley Rimer <rrimer@oregonstate.edu>
//
// Adapted from the File Out JSON module implemented by Nathan Potter
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.

#include "config.h"

#include <cassert>

#include <sstream>
#include <iostream>
#include <fstream>
#include <stddef.h>
#include <string>
#include <typeinfo>

using std::ostringstream;
using std::istringstream;

#include <DDS.h>
#include <Structure.h>
#include <Constructor.h>
#include <Array.h>
#include <Grid.h>
#include <Sequence.h>
#include <Float64.h>
#include <Str.h>
#include <Url.h>

#include <BESDebug.h>
#include <BESInternalError.h>

#include <DapFunctionUtils.h>

#include "FoDapCovJsonValidation.h"
#include "focovjson_utils.h"

#define FoDapCovJsonValidation_debug_key "focovjson"

/** @brief dumps information about this transformation object for debugging
 * purposes
 *
 * Displays the pointer value of this instance plus instance data,
 * including all of the FoJson objects converted from DAP objects that are
 * to be sent to the netcdf file.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FoDapCovJsonValidation::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FoDapCovJsonValidate::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    if (_dds != 0) {
        _dds->print(strm);
    }
    BESIndent::UnIndent();
}

FoDapCovJsonValidation::FoDapCovJsonValidation(libdap::DDS *dds) : _dds(dds)
{
    if (!_dds) throw BESInternalError("File out COVJSON, null DDS passed to constructor", __FILE__, __LINE__);
}
/*
* This method is for validating a dds to see if it is possible to convert the dataset into the coverageJson format
*/
void FoDapCovJsonValidation::validateDataset()
{
    validateDataset(_dds);
}

void FoDapCovJsonValidation::validateDataset(libdap::DDS *dds)
{
    ofstream tempOut;
    string tempFileName = "/home/ubuntu/hyrax/dds.log";

    // Open/truncate for a new log file
    tempOut.open(tempFileName.c_str(), ios::trunc);
    // Append to the existing log file
    //tempOut.open(tempFileName.c_str(), ios::app);

    if(tempOut.fail()) {
       cout << "Could not open " << tempFileName << endl;
       exit(EXIT_FAILURE);
    }

    dds->print(tempOut);

    tempOut.close();
}
