// DODSBasicTransmitter.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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

#include <DAS.h>
#include <DDS.h>
#include <DODSFilter.h>

#include "DODSBasicTransmitter.h"
#include "DODSInfo.h"
#include "OPeNDAPDataNames.h"

void
DODSBasicTransmitter::send_das( DAS &das, DODSDataHandlerInterface & )
{
    das.print( stdout ) ;
    fflush( stdout ) ;
}

void
DODSBasicTransmitter::send_dds( DDS &dds, DODSDataHandlerInterface &dhi )
{
    dds.parse_constraint( dhi.data[POST_CONSTRAINT] ) ;
    dds.print_constrained( stdout ) ;

#if 0
    if( dhi.post_constraint != "" )
    {
	printf("The post constraint is: %s\n", dhi.post_constraint.c_str());
	dds.parse_constraint( dhi.post_constraint ) ;
	dds.print_constrained( stdout ) ;
    } else {
	dds.print( stdout ) ;
    }
#endif
    fflush( stdout ) ;
}

void
DODSBasicTransmitter::send_data( DDS &dds, DODSDataHandlerInterface &dhi )
{
    dhi.first_container();
    //printf("Data name: %s\n", dhi.container->get_real_name().c_str());
    //printf("Post CE: %s\n", dhi.post_constraint.c_str());

    DODSFilter df;
    df.set_dataset_name(dhi.container->get_real_name());
    df.set_ce(dhi.data[POST_CONSTRAINT]);
    
    df.send_data(dds, stdout, "", false);
#if 0
    dds.send( dhi.container->get_real_name(), dhi.data[POST_CONSTRAINT], stdout, false ) ;
#endif
    fflush( stdout ) ;
}

void
DODSBasicTransmitter::send_ddx( DDS &dds, DODSDataHandlerInterface &dhi )
{
    string constraint = dhi.data[POST_CONSTRAINT] ;
    if( constraint != "" )
    {
#if 0
        // This method was deprecated and has been removed. jhrg 2/1/06
	dds.parse_constraint( constraint, stdout, true ) ;
#endif
        dds.parse_constraint( constraint ) ;
    }
    // FIX: print_xml assumes http
    // FIX: need to figure out blob
    // Comment: Using DODSFilter may solve these issues. jhrg 9/16/05
    dds.print_xml( stdout, !constraint.empty(), ".blob" ) ;

    // FIX: Do I need to flush?
    fflush( stdout ) ;
}

void
DODSBasicTransmitter::send_text( DODSInfo &info,
                                 DODSDataHandlerInterface & )
{
    info.print( stdout ) ;
}

void
DODSBasicTransmitter::send_html( DODSInfo &info,
                                 DODSDataHandlerInterface & )
{
    info.print( stdout ) ;
}

