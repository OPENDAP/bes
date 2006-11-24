// agglistT.h

#ifndef I_agglistT_H
#define I_agglistT_H

#include <string>

using std::string ;

#include "baseApp.h"

class BESAggregationServer ;

class agglistT : public baseApp {
public:
                                agglistT(void) : baseApp() {}
    virtual                     ~agglistT(void) {}
    virtual int			run(void);

    static BESAggregationServer *h1( string handler_name ) ;
    static BESAggregationServer *h2( string handler_name ) ;
    static BESAggregationServer *h3( string handler_name ) ;
    static BESAggregationServer *h4( string handler_name ) ;
};

#endif

