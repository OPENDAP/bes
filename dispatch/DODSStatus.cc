// DODSStatus.cc

#include "DODSStatus.h" 

string DODSStatus::boot_time ;
int DODSStatus::_counter ;

DODSStatus::DODSStatus()
{
    if( _counter++ == 0 ) 
    {   
	const time_t sctime = time( NULL ) ;
	const struct tm *sttime = localtime( &sctime ) ;
	char zone_name[10] ;
	strftime( zone_name, sizeof( zone_name ), "%Z", sttime ) ;
	boot_time = string( zone_name ) + " " + string( asctime( sttime ) ) ;
    }
}

DODSStatus::DODSStatus( const DODSStatus & )
{
    _counter++ ;
} 

DODSStatus::~DODSStatus()
{
    _counter-- ;
}

static DODSStatus _static_status ;

