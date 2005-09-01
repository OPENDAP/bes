// DODSStatus.h

#ifndef DODSStatus_h_
#define DODSStatus_h_ 1

#include <time.h>
#include <string>

using std::string ;

class DODSStatus
{
    static int			_counter ;
    static string		boot_time ;
public:
				DODSStatus();
				DODSStatus(const DODSStatus &);
  				~DODSStatus();
  string			get_status() { return DODSStatus::boot_time ; }
} ;

#endif // DODSStatus_h_
