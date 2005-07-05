// dog.h

#ifndef I_DOG_H
#define I_DOG_H

#include "Animal.h"

class dog : public Animal {
public:
				dog(char *name);
    virtual			~dog(void);
};

#endif

