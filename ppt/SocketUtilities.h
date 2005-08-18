// SocketUtilities.h

// 2005 Copyright University Corporation for Atmospheric Research

#ifndef SocketUtilities_h
#define SocketUtilities_h 1

#include <string>

using std::string ;

class SocketUtilities
{
public:
    /**
      * Routine to convert a long int to the specified numeric base,
      * from 2 to 36. You must get sure the buffer val is big enough
      * to hold all the digits for val or this routine may be UNSAFE.
      * @param val the value to be converted.
      * @param buf A buffer where to place the conversion.  
      * @return Pointer to the buffer buf.
      */

    static char *ltoa( long val, char *buf, int base ) ;

    /** 
      * Create a uniq name which is used to create a 
      * unique name for a Unix socket in a client .
      * or for creating a temporary file.
      * @return uniq name
      */
    static string create_temp_name() ;
} ;

#endif // SocketUtilities_h

// $Log: SocketUtilities.h,v $
