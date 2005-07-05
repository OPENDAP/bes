// failInitT.h

#ifndef I_APPT_H
#define I_APPT_H

#include <baseApp.h>

class failInitT : public baseApp {
public:
                                failInitT(void) : baseApp() {}
    virtual                     ~failInitT(void) {}
    virtual int			run(void);
};

#endif

