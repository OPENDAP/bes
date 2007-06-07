// zT.h

#ifndef I_zT_H
#define I_zT_H

#include "baseApp.h"

class zT : public baseApp {
public:
                                zT(void) : baseApp() {}
    virtual                     ~zT(void) {}
    virtual int			run(void);
};

#endif

