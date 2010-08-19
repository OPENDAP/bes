#include <cctype>
#include "HE2CFValidChar.h"

using namespace std;

HE2CFValidChar::HE2CFValidChar()
{
    prefix = 'A';
    valid = '_';
}

HE2CFValidChar::~HE2CFValidChar()
{
}


string HE2CFValidChar::get_valid_string(string s)
{
  // Test if the character belongs to letters, numbers or '_'

    string insertString(1,prefix);

    if (isdigit(s[0]))
	s.insert(0,insertString);
    
    for(int i=0; i < s.length(); i++)
	if(!(isalnum( s[i]) || s[i]=='_'))      
	    s[i]=valid;
    
    return s;
}



void HE2CFValidChar::set_valid_char(char _prefix, char _valid)
{ 
    prefix = _prefix;
    valid = _valid;
}    
    

