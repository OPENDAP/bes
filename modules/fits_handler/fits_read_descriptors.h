// fits_read_descriptors.h

// This file is part of fits_handler, a data handler for the OPeNDAP data
// server. 

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef fits_read_descriptors_h_
#define fits_read_descriptors_h_

#include <string>

namespace  libdap {
    class DDS;
}

namespace fits_handler {
    bool fits_read_descriptors( libdap::DDS &dds, const std::string &filename, std::string &error ) ;

    int process_hdu_image( fitsfile *fptr, libdap::DDS &dds, const std::string &hdu, const std::string &str ) ;

    int process_hdu_ascii_table( fitsfile *fptr, libdap::DDS &dds, const std::string &hdu, const std::string &str ) ;
#if 0
    int process_hdu_binary_table( fitsfile *fptr, libdap::DDS &dds ) ;
#endif
    void process_status( int status, std::string &error ) ;
#if 0
    char *ltoa( long val, char *buf, int base ) ;
#endif
}

#endif // fits_read_descriptors_h_

