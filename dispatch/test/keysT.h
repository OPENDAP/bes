// keysT.h

#ifndef I_keysT_H
#define I_keysT_H

#include "baseApp.h"

class keysT : public baseApp {
public:
                                keysT(void) : baseApp() {}
    virtual                     ~keysT(void) {}
    virtual int			run(void);
};

#endif

