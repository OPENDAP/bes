// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2002
// Please read the full copyright statement in the file COPYRIGHT.

#include "PPTBuffer.h"

PPTBuffer::PPTBuffer (int s)
{
    data= new unsigned char[s];
    size=s;
    valid=0;
}

PPTBuffer::~PPTBuffer ()
{
    if (data)
	{
	    delete [] data;
	    data=0;
	}
    size=0;
    valid=0;
}
