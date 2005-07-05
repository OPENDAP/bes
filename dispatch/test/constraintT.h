// constraintT.h

#ifndef I_constraintT_H
#define I_constraintT_H

#include "baseApp.h"

class constraintT : public baseApp {
public:
                                constraintT(void) : baseApp() {}
    virtual                     ~constraintT(void) {}
    virtual int			run(void);
};

#endif

