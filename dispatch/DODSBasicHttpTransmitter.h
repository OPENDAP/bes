// DODSBasicHttpTransmitter.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef A_DODSBasicHttpTransmitter_h
#define A_DODSBasicHttpTransmitter_h 1

#include "DODSBasicTransmitter.h"

#define HTTP_TRANSMITTER "HTTP"

class DODSBasicHttpTransmitter : public DODSBasicTransmitter
{
public:
    			DODSBasicHttpTransmitter() {}
    virtual		~DODSBasicHttpTransmitter() {}

    virtual void	send_das( DAS &das, DODSDataHandlerInterface &dhi ) ;
    virtual void	send_dds( DDS &dds, DODSDataHandlerInterface &dhi ) ;
    virtual void	send_data( DDS &dds, DODSDataHandlerInterface &dhi ) ;
    virtual void	send_ddx( DDS &dds, DODSDataHandlerInterface &dhi ) ;
    virtual void	send_text( DODSInfo &info, DODSDataHandlerInterface &dhi) ;
    virtual void	send_html( DODSInfo &info, DODSDataHandlerInterface &dhi) ;
} ;

#endif // A_DODSBasicHttpTransmitter_h
