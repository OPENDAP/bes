// utilT.h

#ifndef I_utilT_H
#define I_utilT_H

#include <list>
#include <string>

using std::list ;
using std::string ;

#include "baseApp.h"

class utilT : public baseApp {
private:
    void			display_values( const list<string> &values ) ;
public:
                                utilT(void) : baseApp() {}
    virtual                     ~utilT(void) {}
    virtual int			run(void);
};

#endif

