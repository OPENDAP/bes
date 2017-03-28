// GatewayModule.h

#ifndef I_GatewayModule_H
#define I_GatewayModule_H 1

#include "BESAbstractModule.h"

class GatewayModule: public BESAbstractModule {
public:
    GatewayModule()
    {
    }
    virtual ~GatewayModule()
    {
    }
    virtual void initialize(const string &modname);
    virtual void terminate(const string &modname);

    virtual void dump(ostream &strm) const;
};

#endif // I_GatewayModule_H
