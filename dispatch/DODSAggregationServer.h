// DODSAggregationServer.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSAggregationServer_h_
#define DODSAggregationServer_h_ 1

#include "DODSDataHandlerInterface.h"

/** @brief Abstraction representing mechanism for aggregating data
 */
class DODSAggregationServer
{
private:
    string		_name ;
			DODSAggregationServer() {}
protected:
    			DODSAggregationServer( string name )
			    : _name( name ) {}
public:
    virtual		~DODSAggregationServer() {} ;
    /** @brief aggregate the response object
     *
     * @param dhi structure which contains the response object and the
     * aggregation command
     * @throws DODSAggregationException if problem aggregating the data
     * @see DODSAggregationException
     * @see _DODSDataHandlerInterface
     */
    virtual void	aggregate( DODSDataHandlerInterface &dhi ) = 0 ;

    virtual string	get_name() { return _name ; }
};

#endif // DODSAggregationServer_h_

// $Log: DODSAggregationServer.h,v $
// Revision 1.2  2005/03/26 00:33:33  pwest
// fixed aggregation server invoke problems. Other commands use the aggregation command but no aggregation is needed, so set aggregation to empty string when done
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
