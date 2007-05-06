// cacheT.h

#ifndef I_cacheT_H
#define I_cacheT_H

#include "baseApp.h"

#include <string>
#include <map>

using std::string ;
using std::map ;

class cacheT : public baseApp {
private:
    void			check_cache( const string &cache_dir,
					     map<string,string> &contents ) ;
public:
                                cacheT(void) : baseApp() {}
    virtual                     ~cacheT(void) {}
    virtual int			run(void);
};

#endif

