// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2002
// Please read the full copyright statement in the file COPYRIGHT.

#ifndef PPTException_h_
#define PPTException_h_ 1

#include <string>

using std::string ;

/**
 */

class PPTException
{
protected:
    string _msg ;
    string _file ;
    int _line ;
public:
    PPTException( const string &msg,
                  const string &file = "UNDEFINED",
                  const int & line = 0 )
	: _msg( msg ),
	  _file( file ),
	  _line( line ) {}
    virtual ~PPTException() {}
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

#endif // PPTException_h_ 

// $Log: PPTException.h,v $
