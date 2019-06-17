// BESDASResponseHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <memory>

#include <DAS.h>

#include "BESDASResponseHandler.h"
#include "BESDASResponse.h"
#include "BESRequestHandlerList.h"
#include "BESDapNames.h"
#include "BESTransmitter.h"

#include "GlobalMetadataStore.h"

using namespace libdap;
using namespace bes;
using namespace std;

BESDASResponseHandler::BESDASResponseHandler( const string &name )
    : BESResponseHandler( name )
{
}

BESDASResponseHandler::~BESDASResponseHandler( )
{
}

/**
 * @brief executes the command `<get type="das" definition=...>`
 *
 * For each container in the specified definition go to the request
 * handler for that container and have it add to the OPeNDAP DAS response
 * object. The DAS response object is built within this method and passed
 * to the request handler list. This command will read the DAS from the
 * MDS if that has been configured and the response is found there.
 *
 * DAS responses are added to the MDS when a request for either the DMR or
 * DDS is made.
 *
 * @param dhi structure that holds request and response information
 *
 * @see BESDataHandlerInterface
 * @see BESDASResponse
 * @see BESRequestHandlerList
 */
void
BESDASResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    dhi.action_name = DAS_RESPONSE_STR;

    GlobalMetadataStore *mds = GlobalMetadataStore::get_instance();

    GlobalMetadataStore::MDSReadLock lock;

    dhi.first_container();
    if (mds) lock = mds->is_das_available(dhi.container);

    if (mds && lock()) {
        // send the response
        mds->write_das_response(dhi.container->get_relative_name(), dhi.get_output_stream());
        // suppress transmitting a ResponseObject in transmit()
        d_response_object = 0;
    }
    else {
        DAS *das = new DAS();

        d_response_object = new BESDASResponse( das ) ;

        BESRequestHandlerList::TheList()->execute_each(dhi);

#if ANNOTATION_SYSTEM
            // Support for the experimental Dataset Annotation system. jhrg 12/19/18
            if (!d_annotation_service_url.empty()) {
                // resp_dds is a convenience object
                BESDASResponse *resp_das = static_cast<BESDASResponse *>(d_response_object);

                // Add the Annotation Service URL attribute in the DODS_EXTRA container.
                AttrTable *dods_extra = resp_das->get_das()->get_table(DODS_EXTRA_ATTR_TABLE);
                if (dods_extra)
                    dods_extra->append_attr(DODS_EXTRA_ANNOTATION_ATTR, "String", d_annotation_service_url);
                else {
                    auto_ptr<AttrTable> new_dods_extra(new AttrTable);
                    new_dods_extra->append_attr(DODS_EXTRA_ANNOTATION_ATTR, "String", d_annotation_service_url);
                    resp_das->get_das()->add_table(DODS_EXTRA_ATTR_TABLE, new_dods_extra.release());
                }
            }
#endif
        // The DDS and DMR ResponseHandler code stores those responses when the
        // MDS is configured (*mds is not null) and can make all of the DDS, DAS
        // and DMR from either the DDS or DMR alone. But the DAS is different -
        // the MDS code cannot generate the DDS and DMR responses from the DAS.
        // We could re-work the software to build a DDS even when the request is
        // for a DAS, but that would involve a number of modifications to the code
        // for an optimization of dubious value. If a client asked for a DAS, it
        // will likely ask for a DDS very soon... jhrg 3/18/18
    }
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it using the send_das method
 * on the transmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESDASResponse
 * @see BESTransmitter
 * @see BESDataHandlerInterface
 */
void
BESDASResponseHandler::transmit( BESTransmitter *transmitter,
                              BESDataHandlerInterface &dhi )
{
    if( d_response_object )
    {
	transmitter->send_response( DAS_SERVICE, d_response_object, dhi ) ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESDASResponseHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDASResponseHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESResponseHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESResponseHandler *
BESDASResponseHandler::DASResponseBuilder( const string &name )
{
    return new BESDASResponseHandler( name ) ;
}

