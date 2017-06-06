# -*- Perl -*-
#
# This file contains attribute information used by the DODS_Time
# interface. 12/22/98 jhrg

Attributes {
    DODS_Time {
	    String hours_variable GSO_AVHRR.hh;
        String minutes_variable GSO_AVHRR.mm;
        String seconds_variable GSO_AVHRR.ss;
        String gmt_time true;
    }
    DODS_Date {
        String year_variable GSO_AVHRR.year;
        Int32 year_base 1900;
        String year_day_variable GSO_AVHRR.day_num;
    }
}	
