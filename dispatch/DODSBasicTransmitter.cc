// DODSBasicTransmitter.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSBasicTransmitter.h"
#include "DAS.h"
#include "DDS.h"
#include "DODSInfo.h"
#include "OPeNDAPDataNames.h"

void
DODSBasicTransmitter::send_das( DAS &das, DODSDataHandlerInterface &dhi )
{
    das.print( stdout ) ;
    fflush( stdout ) ;
}

void
DODSBasicTransmitter::send_dds( DDS &dds, DODSDataHandlerInterface &dhi )
{
    dds.parse_constraint( dhi.data[POST_CONSTRAINT] ) ;
    dds.print_constrained( stdout ) ;

#if 0
    if( dhi.post_constraint != "" )
    {
	printf("The post constraint is: %s\n", dhi.post_constraint.c_str());
	dds.parse_constraint( dhi.post_constraint ) ;
	dds.print_constrained( stdout ) ;
    } else {
	dds.print( stdout ) ;
    }
#endif
    fflush( stdout ) ;
}

void
DODSBasicTransmitter::send_data( DDS &dds, DODSDataHandlerInterface &dhi )
{
    dhi.first_container();
    //printf("Data name: %s\n", dhi.container->get_real_name().c_str());
    //printf("Post CE: %s\n", dhi.post_constraint.c_str());

    dds.send( dhi.container->get_real_name(), dhi.data[POST_CONSTRAINT], stdout, false ) ;

    fflush( stdout ) ;
}

void
DODSBasicTransmitter::send_ddx( DDS &dds, DODSDataHandlerInterface &dhi )
{
    string constraint = dhi.data[POST_CONSTRAINT] ;
    if( constraint != "" )
    {
	dds.parse_constraint( constraint, stdout, true ) ;
    }
    // FIX: print_xml assumes http
    // FIX: need to figure out blob
    dds.print_xml( stdout, !constraint.empty(), ".blob" ) ;

    // FIX: Do I need to flush?
    fflush( stdout ) ;
}

void
DODSBasicTransmitter::send_text( DODSInfo &info,
                                 DODSDataHandlerInterface &dhi )
{
    info.print( stdout ) ;
}

void
DODSBasicTransmitter::send_html( DODSInfo &info,
                                 DODSDataHandlerInterface &dhi )
{
    info.print( stdout ) ;
}

