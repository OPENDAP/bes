// DODSBasicTransmitter.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef A_DODSBasicTransmitter_h
#define A_DODSBasicTransmitter_h 1

#include "DODSTransmitter.h"

#define BASIC_TRANSMITTER "basic"

class DODSBasicTransmitter : public DODSTransmitter
{
public:
    			DODSBasicTransmitter() {}
    virtual		~DODSBasicTransmitter() {}

    virtual void	send_das( DAS &das, DODSDataHandlerInterface &dhi ) ;
    virtual void	send_dds( DDS &dds, DODSDataHandlerInterface &dhi ) ;
    virtual void	send_data( DDS &dds, DODSDataHandlerInterface &dhi ) ;
    virtual void	send_ddx( DDS &dds, DODSDataHandlerInterface &dhi ) ;
    virtual void	send_text( DODSInfo &info, DODSDataHandlerInterface &dhi) ;
    virtual void	send_html( DODSInfo &info, DODSDataHandlerInterface &dhi) ;
} ;

#endif // A_DODSBasicTransmitter_h
