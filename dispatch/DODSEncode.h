// DODSEncode.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DODSEncode_h
#define I_DODSEncode_h 1

#include <string>

using std::string ;

class DODSEncode
{
private:
    static char sap_bit(unsigned char val, int pos) ;
    static void encode( const char * text, const char *key,
		        char *encoded_text ) ;
    static void my_encode( const char * text, const char *key,
                           char *encoded_text ) ;
    static void decode( const char * encoded_text, const char *key,
		        char *decoded_text ) ;
    static void my_decode( const char * encoded_text, const char *key,
                           char *decoded_text ) ;

public:
    // text MUST be buffer of 8 bytes, key MUST be buffer with 8 bytes,
    // encoded_text MUST be buffer with 64 bytes
    // not buffer overflow check is done!
    static string encode( const string &text, const string &key ) ;

    // text MUST be buffer of 64 bytes, key MUST be buffer with 8 bytes, 
    // decoded_text MUST be buffer with 8 bytes
    // no buffer overflow check is done!
    // encoded_text MUST have only 0s and 1s, otherwise behavior is not 
    // predicted.
    static string decode( const string &encoded_text, const string &key ) ;
} ;

#endif // I_DODSEncode_h

// $Log: DODSEncode.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
