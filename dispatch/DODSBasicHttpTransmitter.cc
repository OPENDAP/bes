// DODSBasicHttpTransmitter.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSBasicHttpTransmitter.h"
#include "DAS.h"
#include "DDS.h"
#include "DODSInfo.h"

void
DODSBasicHttpTransmitter::send_das( DAS &das, DODSDataHandlerInterface &dhi )
{
    set_mime_text( stdout, dods_das ) ;
    DODSBasicTransmitter::send_das( das, dhi ) ;
}

void
DODSBasicHttpTransmitter::send_dds( DDS &dds, DODSDataHandlerInterface &dhi )
{
    set_mime_text( stdout, dods_dds ) ;
    DODSBasicTransmitter::send_dds( dds, dhi ) ;
}

void
DODSBasicHttpTransmitter::send_data( DDS &dds, DODSDataHandlerInterface &dhi )
{
    set_mime_binary( stdout, dods_data ) ;
    DODSBasicTransmitter::send_data( dds, dhi ) ;
}

void
DODSBasicHttpTransmitter::send_ddx( DDS &dds, DODSDataHandlerInterface &dhi )
{
    set_mime_text( stdout, dods_dds ) ;
    DODSBasicTransmitter::send_ddx( dds, dhi ) ;
}

void
DODSBasicHttpTransmitter::send_text( DODSInfo &info,
                                     DODSDataHandlerInterface &dhi )
{
    if( info.is_buffered() )
	set_mime_text( stdout, info.type() ) ;
    DODSBasicTransmitter::send_text( info, dhi ) ;
}

void
DODSBasicHttpTransmitter::send_html( DODSInfo &info,
                                     DODSDataHandlerInterface &dhi )
{
    if( info.is_buffered() )
	set_mime_html( stdout, info.type() ) ;
    DODSBasicTransmitter::send_text( info, dhi ) ;
}

