// OPENDAP_CLASSModule.h

#ifndef I_OPENDAP_CLASSModule_H
#define I_OPENDAP_CLASSModule_H 1

#include "BESAbstractModule.h"

class OPENDAP_CLASSModule : public BESAbstractModule
{
public:
    				OPENDAP_CLASSModule() {}
    virtual		    	~OPENDAP_CLASSModule() {}
    virtual void		initialize() ;
    virtual void		terminate() ;
} ;

#endif // I_OPENDAP_CLASSModule_H

