// PPTProtocol.h

// 2005 Copyright University Corporation for Atmospheric Research

#ifndef PPTProtocol_h_
#define PPTProtocol_h_ 1

#include <string>

using std::string ;

class PPTProtocol
{
public:
    // Generic socket message when the mapper fails to find the proper
    // protocol string
    static string PPT_PROTOCOL_UNDEFINED ;
    static string PPT_COMPLETE_DATA_TRANSMITION ;
    static string PPT_EXIT_NOW ;

    // From client to server
    static string PPTCLIENT_TESTING_CONNECTION ;

    // From server to client
    static string PPTSERVER_CONNECTION_OK ;
} ;

#endif // PPTProtocol_h_

// $Log: PPTProtocol.h,v $
