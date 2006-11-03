#include "BESDASResponse.h"

BESDASResponse::~BESDASResponse()
{
    if( _das )
	delete _das ;
}

