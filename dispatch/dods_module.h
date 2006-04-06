#ifndef dods_module_h
#define dods_module_h 1

class dods_module
{
public:
    static int initialize( int argc, char **argv ) ;
    static int terminate( void ) ;
} ;

#endif // dods_module_h

