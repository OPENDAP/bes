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
    strm << BESIndent::LMarg << "data dds: " << (void *)_dds << endl ;
    BESIndent::UnIndent() ;
}

