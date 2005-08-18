// PPTClient.h

// 2005 Copyright University Corporation for Atmospheric Research

#ifndef PPTClient_h
#define PPTClient_hS 1

#include "PPTConnection.h"

class Socket ;

class PPTClient : public PPTConnection
{
private:
    bool			_connected ;
public:
    				PPTClient( const string &hostStr, int portVal );
    				PPTClient( const string &unix_socket );
    				~PPTClient() ;
    virtual void		initConnection() ;
    virtual void		closeConnection() ;
} ;

#endif // PPTClient_h

// $Log: PPTClient.h,v $
