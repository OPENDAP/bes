// containerT.h

#ifndef I_containerT_H
#define I_containerT_H

#include "baseApp.h"

class containerT : public baseApp {
public:
                                containerT(void) : baseApp() {}
    virtual                     ~containerT(void) {}
    virtual int			run(void);
};

#endif

