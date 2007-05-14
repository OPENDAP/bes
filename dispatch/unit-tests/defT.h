// defT.h

#ifndef I_defT_H
#define I_defT_H

#include "baseApp.h"

class defT : public baseApp {
public:
                                defT(void) : baseApp() {}
    virtual                     ~defT(void) {}
    virtual int			run(void);
};

#endif

