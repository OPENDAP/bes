#include <cctype>
#include <sstream>
#include <iostream>
#include <InternalErr.h>
#include "HE2CFUniqName.h"

using namespace std;
using namespace libdap;

// Default constructor
HE2CFUniqName::HE2CFUniqName()
{
    limit = false;
    suffix = "U";
    counter = -1;
}

HE2CFUniqName::~HE2CFUniqName()
{
}
    
// Append input string name with a number to distinguish from others
string HE2CFUniqName::get_uniq_string(string s)
{

    string eosString=s;

    string appendstr;
    stringstream out;

    ++counter;
    if(counter > 999 && limit){
        throw InternalErr(__FILE__, __LINE__,
                      "Uniq id counter reached 1,000.");        
    }

    if(counter < 10 && limit){
        out << suffix << "00" << counter;
    }
    else if(counter < 100 && limit){
        out << suffix << "0" << counter;
    }
    else{
        out << suffix << counter;        
    }

    appendstr=out.str();

    s=s+appendstr;
    
    return s;
}

void
HE2CFUniqName::set_counter()
{
    counter = 0;
}

void HE2CFUniqName::set_uniq_name(string _suffix, bool _limit)
{
    limit = _limit;
    counter = 0;                // reset the counter.
    if (_suffix.length() > 3)
    {
        throw InternalErr(__FILE__, __LINE__,
                      "Length of the suffix should be < 4.");        
    }
    else{
        suffix = _suffix;
    }
    
}



