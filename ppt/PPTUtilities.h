// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2002
// Please read the full copyright statement in the file COPYRIGHT.


#ifndef PPTUtilities_h_
#define PPTUtilities_h_ 1

#include <stdlib.h>
#include <string>

using std::string ;

/**
 * Simple routine to convert a long to a char*.
 * Routine to convert a long int to the specified numeric base, from 2 to 36.
 * You must get sure the buffer val is big enough to hold all the digits  for val 
 * or this routine may be UNSAFE.
 * @param val the value to be converted.
 * @param buf A buffer where to place the conversion.  
 * @return Pointer to the buffer buf.
 */

char *ltoa(long val, char *buf, int base);

/**
 * write debugging messages.
 * Write a message to stdout and flush the stream
 * if the global variable debug_server is set to 1
 * otherwise ignore the message.
 * @param buf the message
 */

void write_out(const char *buf);

/**
 * write debugging messages by integer number.
 * Write a message to stdout and flush the stream
 * if the global variable debug_server is set to 1
 * otherwise ignore the message.
 * @param buf the message
 */
void write_out(int buf);

/** 
 * Create a name which is used to created a 
 * unique name for a Unix socket in a client .
 * or for creating a temporary file.
 */
string create_temp_name();


#endif // PPTUtilities_h_
