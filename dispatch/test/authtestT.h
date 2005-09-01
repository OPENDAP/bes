// authtestT.h

#ifndef I_authtestT_H
#define I_authtestT_H

#include "baseApp.h"

class authtestT : public baseApp {
public:
                                authtestT(void) : baseApp() {}
    virtual                     ~authtestT(void) {}
    virtual int			run(void);
};

#endif

