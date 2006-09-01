// BESDapTransmit.cc

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

#include "BESDapTransmit.h"
#include "DODSFilter.h"
#include "BESContainer.h"
#include "BESDataNames.h"
#include "cgi_util.h"
#include "DAS.h"
#include "DDS.h"
#include "DataDDS.h"

void
BESDapTransmit::send_basic_das( DODSResponseObject *obj,
                                BESDataHandlerInterface &dhi )
{
    DAS *das = dynamic_cast<DAS *>(obj) ;
    dhi.first_container();

    DODSFilter df ;
    df.set_dataset_name( dhi.container->get_real_name() ) ;
    df.send_das( stdout, *das, "", false ) ;

    fflush( stdout ) ;
}

void
BESDapTransmit::send_http_das( DODSResponseObject *obj,
                               BESDataHandlerInterface &dhi )
{
    set_mime_text( stdout, dods_das ) ;
    BESDapTransmit::send_basic_das( obj, dhi ) ;
}

void
BESDapTransmit::send_basic_dds( DODSResponseObject *obj,
				BESDataHandlerInterface &dhi )
{
    DDS *dds = dynamic_cast<DDS *>(obj) ;
    dhi.first_container();

    DODSFilter df ;
    df.set_dataset_name( dhi.container->get_real_name() ) ;
    df.set_ce( dhi.data[POST_CONSTRAINT] ) ;
    df.send_dds( stdout, *dds, dhi.ce, true, "", false ) ;

    fflush( stdout ) ;
}

void
BESDapTransmit::send_http_dds( DODSResponseObject *obj,
			       BESDataHandlerInterface &dhi )
{
    set_mime_text( stdout, dods_dds ) ;
    BESDapTransmit::send_basic_dds( obj, dhi ) ;
}

void
BESDapTransmit::send_basic_data( DODSResponseObject *obj,
				 BESDataHandlerInterface &dhi )
{
    DataDDS *dds = dynamic_cast<DataDDS *>(obj) ;
    dhi.first_container() ;

    DODSFilter df ;
    df.set_dataset_name( dds->filename() ) ;
    df.set_ce(dhi.data[POST_CONSTRAINT]);
    df.send_data( *dds, dhi.ce, stdout, "", false ) ;

    fflush( stdout ) ;
}

void
BESDapTransmit::send_http_data( DODSResponseObject *obj,
				BESDataHandlerInterface &dhi )
{
    //set_mime_binary( stdout, dods_data ) ;
    BESDapTransmit::send_basic_data( obj, dhi ) ;
}

void
BESDapTransmit::send_basic_ddx( DODSResponseObject *obj,
				BESDataHandlerInterface &dhi )
{
    DDS *dds = dynamic_cast<DDS *>(obj) ;
    dhi.first_container() ;

    DODSFilter df ;
    df.set_dataset_name( dhi.container->get_real_name() ) ;
    df.set_ce( dhi.data[POST_CONSTRAINT] ) ;
    df.send_ddx( *dds, dhi.ce, stdout, false ) ;

    fflush( stdout ) ;
}

void
BESDapTransmit::send_http_ddx( DODSResponseObject *obj,
			       BESDataHandlerInterface &dhi )
{
    set_mime_text( stdout, dods_dds ) ;
    BESDapTransmit::send_basic_ddx( obj, dhi ) ;
}

