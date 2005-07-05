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
  ///
  string _file;
  ///
  int _line;
  ///
  string _description;

public:
  ///
  PPTException(const char* file="UNDEFINED", const int & line=0, const string &description="UNDEFINED")
  {
    _file=file;
    _line=line;
    _description=description;
  }
  virtual ~PPTException(){}
  ///
  virtual string getErrorDescription()
  {
    return _description;
  }
  ///
  virtual string getErrorFile()
  {
    return _file;
  }
  ///
  virtual int getErrorLine()
  {
    return _line;
  }
};

#endif // PPTException_h_ 
