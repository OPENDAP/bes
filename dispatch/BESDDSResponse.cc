#include "BESDDSResponse.h"

BESDDSResponse::~BESDDSResponse()
{
    if( _dds )
	delete _dds ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the dds object
 * created
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESDDSResponse::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDDSResponse::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "dds: " << (void *)_dds << endl ;
    BESIndent::UnIndent() ;
}

