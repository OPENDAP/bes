#ifndef DODSClientUtilities_h_
#define DODSClientUtilities_h_ 1

/** This method helps clients that can not move
    strings aroung with blank spaces as the separator
    for the tokens in a DODS query, to use any other
    separator such as '&' or '/' and this method
    will replaced with blanks. 
    October 9, 2002.
    I tried to hint the compiler to make it inline
    and I tried to keep it short so it is fast.
    I am not sure it is the best algorithm but 
    I will do the job for the time being
    Jose Garcia (jgarcia@ucar.edu)
*/

inline void dodsutil_replace_with_blanks (char *data, int length, char separator)
{
  int j;
  for (j=0; j< length; j++)
    if(data[j]==separator)
      data[j]=' ';
}

inline void dodsutil_replace_character (char *data, int length, char source, char target)
{
  int j;
  for (j=0; j< length; j++)
    if(data[j]==source)
      data[j]=target;
}

#endif // DODSClientUtilities_h_
