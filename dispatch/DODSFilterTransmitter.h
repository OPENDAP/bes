// DODSFilterTransmitter.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef A_DODSFilterTransmitter_h
#define A_DODSFilterTransmitter_h 1

#include "DODSTransmitter.h"

class DODSFilter ;

class DODSFilterTransmitter : public DODSTransmitter
{
private:
    DODSFilter *	_df ;
public:
    			DODSFilterTransmitter( DODSFilter &df ) : _df( &df ) {}
    virtual		~DODSFilterTransmitter() {}

    virtual void	send_das( DAS &das, DODSDataHandlerInterface &dhi ) ;
    virtual void	send_dds( DDS &dds, DODSDataHandlerInterface &dhi ) ;
    virtual void	send_data( DDS &dds, DODSDataHandlerInterface &dhi ) ;
    virtual void	send_ddx( DDS &dds, DODSDataHandlerInterface &dhi ) ;
    virtual void	send_text( DODSInfo &info, DODSDataHandlerInterface &dhi) ;
    virtual void	send_html( DODSInfo &info, DODSDataHandlerInterface &dhi) ;
} ;

#endif // A_DODSFilterTransmitter_h
