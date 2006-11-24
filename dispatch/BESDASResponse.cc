#include "BESDASResponse.h"

BESDASResponse::~BESDASResponse()
{
    if( _das )
	delete _das ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the das object
 * created
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESDASResponse::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDASResponse::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "das: " << (void *)_das << endl ;
    BESIndent::UnIndent() ;
}

