// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2002
// Please read the full copyright statement in the file COPYRIGHT.

#ifndef SocketException_h_
#define SocketException_h_ 1

#include <string>

using std::string ;

/**
 */

class SocketException
{
protected:
    string _msg ;
    string _file ;
    int _line ;
public:
    SocketException( const string &msg,
                  const string &file = "UNDEFINED",
                  const int & line = 0 )
	: _msg( msg ),
	  _file( file ),
	  _line( line ) {}
    virtual ~SocketException() {}
    virtual string getMessage()
    {
	return _msg;
    }

    virtual string getErrorFile()
    {
	return _file;
    }

    virtual int getErrorLine()
    {
	return _line;
    }
};

#endif // SocketException_h_ 

// $Log: SocketException.h,v $
