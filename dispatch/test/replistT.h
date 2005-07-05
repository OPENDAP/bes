// replistT.h

#ifndef I_replistT_H
#define I_replistT_H

#include "baseApp.h"

class replistT : public baseApp {
public:
                                replistT(void) : baseApp() {}
    virtual                     ~replistT(void) {}
    virtual int			run(void);
};

#endif

