#include "BESDDSResponse.h"

BESDDSResponse::~BESDDSResponse()
{
    if( _dds )
	delete _dds ;
}

