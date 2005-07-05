// pvolT.h

#ifndef I_pvolT_H
#define I_pvolT_H

#include "baseApp.h"

class pvolT : public baseApp {
public:
                                pvolT(void) : baseApp() {}
    virtual                     ~pvolT(void) {}
    virtual int			run(void);
};

#endif

