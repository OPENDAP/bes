#include "BESDataDDSResponse.h"

BESDataDDSResponse::~BESDataDDSResponse()
{
    if (_dds) {
        if (_dds->get_factory())
            delete _dds->get_factory();
        delete _dds;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the data dds object
 * created
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESDataDDSResponse::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDataDDSResponse::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    if( _dds )
    {
	strm << BESIndent::LMarg << "DDS:" << endl ;
	BESIndent::Indent() ;
	DapIndent::SetIndent( BESIndent::GetIndent() ) ;
	_dds->dump( strm ) ;
	DapIndent::Reset() ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "DDS: null" << endl ;
    }
    BESIndent::UnIndent() ;
}

