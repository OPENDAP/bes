// JPEG2000Transmitter.h

// This file is part of BES GDAL File Out Module

// Copyright (c) 2012 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

#ifndef A_JPEG2000Transmitter_h
#define A_JPEG2000Transmitter_h 1

#include <BESTransmitter.h>

class BESContainer;

/** @brief BESTransmitter class named "geotiff" that transmits an OPeNDAP
 * data object as a geotiff file
 *
 * The JPEG2000Transmitter transforms an OPeNDAP DataDDS object into a
 * geotiff file and streams the new (temporary) geotiff file back to the
 * client.
 *
 * @see BESTransmitter
 */
class JPEG2000Transmitter: public BESTransmitter {
private:
    static void return_temp_stream(const string &filename, ostream &strm);
    static string temp_dir;


public:
    JPEG2000Transmitter();
    virtual ~JPEG2000Transmitter()
    {
    }

    static void send_data_as_jp2(BESResponseObject *obj, BESDataHandlerInterface &dhi);

    static string default_gcs;
};

#endif // A_JPEG2000Transmitter_h
