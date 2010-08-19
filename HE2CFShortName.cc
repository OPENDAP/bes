#include <cctype>
#include <sstream>
#include <iostream>
#include <InternalErr.h>

#include "HE2CFShortName.h"

using namespace std;
using namespace libdap;

// Default constructor
HE2CFShortName::HE2CFShortName()
{
    size = 15;
    cut_size = 0;
#ifdef SHORT_NAME
    set_short_name_on();
#else
    flag = false;
#endif

}
HE2CFShortName::~HE2CFShortName()
{
}

// Shorten the input string and then append it with a suffix string 
// and a unique number. The _flag indicates whenter it's shortened or not.
string HE2CFShortName::get_short_string(string s, bool* _flag)
{ 
    *_flag = false;
    s = get_valid_string(s);
    if (flag)
    {
	if(s.length() > cut_size)
	{
            s = s.substr(0, cut_size);
            s = get_uniq_string(s);
            *_flag = true;
	}
    }
    
    return s;
}

// Set values for the private members.
void HE2CFShortName::set_short_name(bool _flag, int _size, string _suffix)
{
    size = _size;
    // 3 means the size of unique number string: 0,1,...99,...999.
    cut_size = size - _suffix.length() - 3;
    if(cut_size < 0){
        ostringstream error;                
        error << "The short name size, " << _size
             << ", is smaller than suffix_size + 3, " << _suffix.length() + 3
             << ".";
        throw InternalErr(__FILE__, __LINE__,
                          error.str());        
    }
    
    flag = _flag;
    if(_suffix == ""){
        set_uniq_name("_", true);
    }
    else {
        set_uniq_name(_suffix, true);
    }
}




void HE2CFShortName::set_short_name_on()
{
    flag = true;
    set_uniq_name("_", true);
    cut_size = size - 4;
    
}

void HE2CFShortName::set_short_name_off()
{
    // reset to default values    
    flag = false;
    cut_size = 0;               
    size = 15;            
}
