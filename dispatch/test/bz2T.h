// bz2T.h

#ifndef I_bz2T_H
#define I_bz2T_H

#include "baseApp.h"

class bz2T : public baseApp {
public:
                                bz2T(void) : baseApp() {}
    virtual                     ~bz2T(void) {}
    virtual int			run(void);
};

#endif

