// catInitT.h

#ifndef I_APPT_H
#define I_APPT_H

#include <baseApp.h>

class catInitT : public baseApp {
public:
                                catInitT(void) : baseApp() {}
    virtual                     ~catInitT(void) {}
    virtual int			run(void);
};

#endif

