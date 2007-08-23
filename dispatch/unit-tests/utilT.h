// utilT.h

#ifndef I_utilT_H
#define I_utilT_H

#include "baseApp.h"

class utilT : public baseApp {
public:
                                utilT(void) : baseApp() {}
    virtual                     ~utilT(void) {}
    virtual int			run(void);
};

#endif

