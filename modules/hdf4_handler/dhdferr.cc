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
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

/////////////////////////////////////////////////////////////////////////////
// Copyright 1996, by the California Institute of Technology.
// ALL RIGHTS RESERVED. United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the
// Office of Technology Transfer at the California Institute of
// Technology. This software may be subject to U.S. export control
// laws and regulations. By accepting this software, the user
// agrees to comply with all applicable U.S. export laws and
// regulations. User has the responsibility to obtain export
// licenses, or other export authority as may be required before
// exporting such information to foreign countries or providing
// access to foreign persons.

// Author: Todd Karakashian, NASA/Jet Propulsion Laboratory
//         Todd.K.Karakashian@jpl.nasa.gov
//
/////////////////////////////////////////////////////////////////////////////
#include "config.h"

#include <sstream>
#include "config_hdf.h"
#include <mfhdf.h>
#include <BESLog.h>
#include "hcerr.h"
#include "dhdferr.h"

using std::ostringstream;
using std::endl;



dhdferr::dhdferr(const string & msg, const string & file, int line)
    :Error(msg)
{
    ostringstream strm;
    strm << get_error_message() << endl
        << "Location: \"" << file << "\", line " << line;
    ERROR_LOG(strm.str() );
}

dhdferr_hcerr::dhdferr_hcerr(const string & msg, const string & file, int line)
    :dhdferr(msg, file, line)
{
    ostringstream strm;
    strm << get_error_message() << endl
        << "Location: \"" << file << "\", line " << line;
    for (int i = 0; i < 5; ++i)
        strm << i << ") " << HEstring((hdf_err_code_t) HEvalue(i)) << endl;
    ERROR_LOG(strm.str());
}

