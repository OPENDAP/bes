// Animal.h

#ifndef A_ANIMAL_H
#define A_ANIMAL_H

#include <string>

using std::string ;

class Animal {
private:
    string			_name ;
public:
				Animal( char *name ) ;
    virtual			~Animal(void) ;
    virtual string		get_name() ;
};

#endif

