// pvolT.h

#ifndef I_pvolT_H
#define I_pvolT_H

#include "baseApp.h"

class pvolT : public baseApp {
public:
                                pvolT(void) : baseApp() {}
    virtual                     ~pvolT(void) {}
    virtual int			initialize(int argC, char **argV);
    virtual int			run(void);
    virtual int			terminate(int sig = 0);
};

#endif

