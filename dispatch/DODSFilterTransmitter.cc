// DODSFilterTransmitter.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSFilterTransmitter.h"
#include "DODSFilter.h"
#include "DODSInfo.h"
#include "OPeNDAPDataNames.h"

void
DODSFilterTransmitter::send_das( DAS &das, DODSDataHandlerInterface &dhi )
{
    _df->send_das( stdout, das ) ;
}

void
DODSFilterTransmitter::send_dds( DDS &dds, DODSDataHandlerInterface &dhi )
{
    _df->set_ce( dhi.data[POST_CONSTRAINT] ) ;
    _df->send_dds( stdout, dds, true ) ;
}

void
DODSFilterTransmitter::send_data( DDS &dds, DODSDataHandlerInterface &dhi )
{
    _df->set_ce( dhi.data[POST_CONSTRAINT] ) ;
    _df->send_data( dds, stdout ) ;
}

void
DODSFilterTransmitter::send_ddx( DDS &dds, DODSDataHandlerInterface &dhi )
{
    _df->set_ce( dhi.data[POST_CONSTRAINT] ) ;
    _df->send_ddx( dds, stdout ) ;
}

void
DODSFilterTransmitter::send_text( DODSInfo &info,
                                  DODSDataHandlerInterface &dhi )
{
    if( info.is_buffered() )
    {
	set_mime_text( stdout, info.type() ) ;
	info.print( stdout ) ;
    }
}

void
DODSFilterTransmitter::send_html( DODSInfo &info,
                                  DODSDataHandlerInterface &dhi )
{
    if( info.is_buffered() )
    {
	set_mime_html( stdout, info.type() ) ;
	info.print( stdout ) ;
    }
}

