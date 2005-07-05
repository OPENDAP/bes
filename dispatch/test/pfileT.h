// pfileT.h

#ifndef I_pfileT_H
#define I_pfileT_H

#include "baseApp.h"

class pfileT : public baseApp {
public:
                                pfileT(void) : baseApp() {}
    virtual                     ~pfileT(void) {}
    virtual int			run(void);
};

#endif

