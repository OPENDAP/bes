// lockT.h

#ifndef I_lockT_H
#define I_lockT_H

#include "baseApp.h"

class lockT : public baseApp {
public:
                                lockT(void) : baseApp() {}
    virtual                     ~lockT(void) {}
    virtual int			run(void);
};

#endif

