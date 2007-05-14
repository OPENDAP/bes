// cat.h

#ifndef I_CAT_H
#define I_CAT_H

#include "Animal.h"

class cat : public Animal {
public:
				cat(char *name);
    virtual			~cat(void);
};

#endif

