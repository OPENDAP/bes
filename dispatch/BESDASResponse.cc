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
    if( _das )
    {
	strm << BESIndent::LMarg << "DAS:" << endl ;
	BESIndent::Indent() ;
	DapIndent::SetIndent( BESIndent::GetIndent() ) ;
	_das->dump( strm ) ;
	DapIndent::Reset() ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "DAS: null" << endl ;
    }
    BESIndent::UnIndent() ;
}

