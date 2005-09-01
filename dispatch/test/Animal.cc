// Animal.cc

#include "Animal.h"

Animal::Animal( char *name )
    : _name( name )
{
}

Animal::~Animal( )
{
}

string
Animal::get_name( )
{
    return _name ;
}

