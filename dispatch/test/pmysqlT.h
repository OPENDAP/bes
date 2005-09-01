// pmysqlT.h

#ifndef I_pmysqlT_H
#define I_pmysqlT_H

#include "baseApp.h"

class pmysqlT : public baseApp {
public:
                                pmysqlT(void) : baseApp() {}
    virtual                     ~pmysqlT(void) {}
    virtual int			run(void);
};

#endif

