// DODSMemoryException.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSMemoryException.h"

void
DODSMemoryException::set_amount_of_memory_required( const unsigned long &a )
{
    _mem_required = a ;
}

unsigned int
DODSMemoryException::get_amount_of_memory_required()
{
    return _mem_required ;
}

// $Log: DODSMemoryException.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
