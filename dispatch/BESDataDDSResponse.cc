#include "BESDataDDSResponse.h"

BESDataDDSResponse::~BESDataDDSResponse()
{
    if( _dds )
	delete _dds ;
}

