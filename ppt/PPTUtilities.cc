#include <unistd.h>
#include <iostream>

using std::cout ;

#include "PPTUtilities.h"

string create_temp_name()
{
    char tempbuf1[50];
    ltoa(getpid(),tempbuf1,10);
    string s=tempbuf1 +(string)"_";
    char tempbuf0[50];
    unsigned int t=time(NULL)-1000000000;
    ltoa(t,tempbuf0,10);
    s+=tempbuf0;
    return s;
}

/************************************************* Credits ***********************************************
 *
 *  ltoa(long, char*, int) -- routine to convert a long int to the specified numeric base, from 2 to 36.
 *  Written by Thad Smith III, Boulder, CO. USA  9/06/91 and contributed to the Public Domain.
 *
 *  ltostr(long, char*, size_t, unsigned) -- An improved, safer, ltoa()
 *  public domain by Jerry Coffin
 * 
 *  unsigned int hstr_i(char *cptr) -- Make an ascii hexadecimal string into an integer.
 *  Originally published as part of the MicroFirm Function Library
 *  Copyright 1986, S.E. Margison
 *  Copyright 1989, Robert B.Stout
 *  The user is granted a free limited license to use this source file
 *  to create royalty-free programs, subject to the terms of the
 *  license restrictions specified in the LICENSE.MFL file.
 *
 *********************************************************************************************************/


char *ltoa(
      long val,                                 /* value to be converted */
      char *buf,                                /* output string         */
      int base)                                 /* conversion base       */
{
      ldiv_t r;                                 /* result of val / base  */

      if (base > 36 || base < 2)          /* no conversion if wrong base */
      {
            *buf = '\0';
            return buf;
      }
      if (val < 0)
            *buf++ = '-';
      r = ldiv (labs(val), base);

      /* output digits of val/base first */

      if (r.quot > 0)
            buf = ltoa ( r.quot, buf, base);
      /* output last digit */

      *buf++ = "0123456789abcdefghijklmnopqrstuvwxyz"[(int)r.rem];
      *buf   = '\0';
      return buf;
}

int debug_server=0;

// -*- C -*-

// (c) COPYRIGHT UCAR/HAO 1993-2002
// Please read the full copyright statement in the file COPYRIGHT.

void write_out(const char *buf)
{
  extern int debug_server;
  if (debug_server)
    {
      cout<<buf;
      cout.flush();
    }
  
}

void write_out(int buf)
{
  extern int debug_server;
  if (debug_server)
    {
      cout<<buf;
      cout.flush();
    }
  
}


/************************************************* LICENSE.MFL ***********************************************
 * Portions of SNIPPETS code are Copyright 1987-1996 by Robert B. Stout dba
 * MicroFirm. The user is granted a free license to use these source files in
 * any way except for commercial publication other than as part of your own
 * program. This means you are explicitly granted the right to:
 * 
 * 1.  Use these files to create applications for any use, private or commercial,
 *     without any license fee.
 * 
 * 2.  Copy or otherwise publish copies of the source files so long as the
 *     original copyright notice is not removed and that such publication is 
 *     free of any charges other than the costs of duplication and distribution.
 * 
 * 3.  Distribute modified versions of the source files so long as the original
 *     copyright notice is not removed, and that the modified nature is clearly
 *     noted in the source file, and that such distribution is free of any
 *     charges other than the costs of duplication and distribution.
 * 
 * 4.  Distribute object files and/or libraries compiled from the supplied
 *     source files, provided that such publication is free of any charges other 
 *     than the costs of duplication and distribution.
 * 
 * Rights reserved to the copyright holder include:
 * 
 * 1.  The right to publish these works commercially including, but not limited
 *     to, books, magazines, and commercial software libraries.
 * 
 * 2.  The commercial rights, as cited above, to all derivative source and/or
 *     object modules. Note that this explicitly grants to the user all rights,
 *     commercial or otherwise, to any and all executable programs created using
 *     MicroFirm copyrighted source or object files contained in SNIPPETS.
 * 
 * Users are specifically prohibited from:
 * 
 * 1.  Removing the copyright notice and claiming the source files or any
 *     derivative works as their own.
 * 
 * 2.  Publishing the source files, the compiled object files, or libraries of
 *     the compiled object files commercially.
 * 
 * In other words, you are free to use these files to make programs, not to make
 * money! The programs you make with them, you may sell or give away as you
 * like, but you many not sell either the source files or object files, alone
 * or in a library, even if you've modified the original source, without a
 * license from the copyright holder.
 * 
 *********************************************************************************************************/
