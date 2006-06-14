// BESFilterTransmitter.cc

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

#include "BESFilterTransmitter.h"
#include "DODSFilter.h"
#include "ConstraintEvaluator.h"
#include "BESInfo.h"
#include "BESDataNames.h"

void
BESFilterTransmitter::send_das( DAS &das, BESDataHandlerInterface & )
{
    _df->send_das( stdout, das ) ;
}

void
BESFilterTransmitter::send_dds( DDS &dds, BESDataHandlerInterface &dhi )
{
    ConstraintEvaluator ce ;
    _df->set_ce( dhi.data[POST_CONSTRAINT] ) ;
    _df->send_dds( stdout, dds, ce, true ) ;
}

void
BESFilterTransmitter::send_data( DDS &dds, BESDataHandlerInterface &dhi )
{
    ConstraintEvaluator ce ;
    _df->set_ce( dhi.data[POST_CONSTRAINT] ) ;
    _df->send_data( dds, ce, stdout ) ;
}

void
BESFilterTransmitter::send_ddx( DDS &dds, BESDataHandlerInterface &dhi )
{
    ConstraintEvaluator ce ;
    _df->set_ce( dhi.data[POST_CONSTRAINT] ) ;
    _df->send_ddx( dds, ce, stdout ) ;
}

void
BESFilterTransmitter::send_text( BESInfo &info,
                                  BESDataHandlerInterface & )
{
    if( info.is_buffered() )
    {
	set_mime_text( stdout, info.type() ) ;
	info.print( stdout ) ;
    }
}

void
BESFilterTransmitter::send_html( BESInfo &info,
                                  BESDataHandlerInterface & )
{
    if( info.is_buffered() )
    {
	set_mime_html( stdout, info.type() ) ;
	info.print( stdout ) ;
    }
}

