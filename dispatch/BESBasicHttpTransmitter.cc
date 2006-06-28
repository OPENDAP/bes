// BESBasicHttpTransmitter.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "BESBasicHttpTransmitter.h"
#include "DAS.h"
#include "DDS.h"
#include "BESInfo.h"
#include "cgi_util.h"

void
BESBasicHttpTransmitter::send_das( DAS &das, BESDataHandlerInterface &dhi )
{
    set_mime_text( stdout, dods_das ) ;
    BESBasicTransmitter::send_das( das, dhi ) ;
}

void
BESBasicHttpTransmitter::send_dds( DDS &dds, BESDataHandlerInterface &dhi )
{
    set_mime_text( stdout, dods_dds ) ;
    BESBasicTransmitter::send_dds( dds, dhi ) ;
}

void
BESBasicHttpTransmitter::send_data( DDS &dds, BESDataHandlerInterface &dhi )
{
    //set_mime_binary( stdout, dods_data ) ;
    BESBasicTransmitter::send_data( dds, dhi ) ;
}

void
BESBasicHttpTransmitter::send_ddx( DDS &dds, BESDataHandlerInterface &dhi )
{
    set_mime_text( stdout, dods_dds ) ;
    BESBasicTransmitter::send_ddx( dds, dhi ) ;
}

void
BESBasicHttpTransmitter::send_text( BESInfo &info,
                                     BESDataHandlerInterface &dhi )
{
    if( info.is_buffered() )
	set_mime_text( stdout, unknown_type ) ;
    BESBasicTransmitter::send_text( info, dhi ) ;
}

void
BESBasicHttpTransmitter::send_html( BESInfo &info,
                                     BESDataHandlerInterface &dhi )
{
    if( info.is_buffered() )
	set_mime_html( stdout, unknown_type ) ;
    BESBasicTransmitter::send_text( info, dhi ) ;
}

