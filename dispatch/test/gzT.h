// gzT.h

#ifndef I_gzT_H
#define I_gzT_H

#include "baseApp.h"

class gzT : public baseApp {
public:
                                gzT(void) : baseApp() {}
    virtual                     ~gzT(void) {}
    virtual int			run(void);
};

#endif

