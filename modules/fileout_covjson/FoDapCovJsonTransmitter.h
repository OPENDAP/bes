// -*- mode: c++; c-basic-offset:4 -*-
//
// FoDapCovJsonTransmitter.h
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

#ifndef A_FoDapCovJsonTransmitter_h
#define A_FoDapCovJsonTransmitter_h 1

#include <BESTransmitter.h>

class BESResponseObject;
class BESDataHandlerInterface;

/** @brief BESTransmitter class named "json" that transmits an OPeNDAP
 * data object as a JSON file
 *
 * The FoW10JsonTransmitter transforms an OPeNDAP DataDDS object into a
 * JSON file and streams the new (temporary) JSON file back to the
 * client.
 *
 * @see BESTransmitter
 */
class FoDapCovJsonTransmitter: public BESTransmitter {
private:
    static string temp_dir;

public:
    FoDapCovJsonTransmitter();
    virtual ~FoDapCovJsonTransmitter() { }

    static void send_data(BESResponseObject *obj, BESDataHandlerInterface &dhi);
    static void send_metadata(BESResponseObject *obj, BESDataHandlerInterface &dhi);
};

#endif // A_FoDapCovJsonTransmitter_h

