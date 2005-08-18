// ServerHandler.h

// 2005 Copyright University Corporation for Atmospheric Research

#ifndef ServerHandler_h
#define ServerHandler_h 1

class Connection ;

class ServerHandler
{
protected:
    				ServerHandler() {}
public:
    virtual			~ServerHandler() {}

    virtual void		handle( Connection *c ) = 0 ;
} ;

#endif // ServerHandler_h

// $Log: ServerHandler.h,v $
