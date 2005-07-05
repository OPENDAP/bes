// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2002
// Please read the full copyright statement in the file COPYRIGHT.

#ifndef PPTBuffer_h_
#define PPTBuffer_h_ 1

/**
 */

class PPTBuffer
{
  int size;
  // Disable copy constructor
  PPTBuffer(PPTBuffer &);
  // Disable asignment operator
  PPTBuffer & operator =(const PPTBuffer &) ;

public:
  //
  unsigned char *data;
  //
  int valid;
  //
  PPTBuffer (int s);
  //
  ~PPTBuffer ();
  //
  int get_size(){return size;}
};

#endif // PPTBuffer_h_
