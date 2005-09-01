// DODSTransmitter.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef A_DODSTransmitter_h
#define A_DODSTransmitter_h 1

#include <string>

using std::string ;

#include "DODSDataHandlerInterface.h"

class DAS ;
class DDS ;
class DODSInfo ;

class DODSTransmitter
{
public:
    virtual void	send_das( DAS &das, DODSDataHandlerInterface &dhi ) = 0 ;
    virtual void	send_dds( DDS &dds, DODSDataHandlerInterface &dhi ) = 0 ;
    virtual void	send_data( DDS &dds, DODSDataHandlerInterface &dhi ) = 0 ;
    virtual void	send_ddx( DDS &dds, DODSDataHandlerInterface &dhi ) = 0 ;
    virtual void	send_text( DODSInfo &info, DODSDataHandlerInterface &dhi) = 0 ;
    virtual void	send_html( DODSInfo &info, DODSDataHandlerInterface &dhi) = 0 ;
} ;

#endif // A_DODSTransmitter_h
