Attributes {
	DODS_Global {
		# Server supports the following interfaces:
		#	date	
		# 	time
		#	date/time

		String DODS_FileType "FileServer";
		String DODS_Interfaces "date", "time", "date_time";
		String DODS_Title "Univ. R.I. 5.5KM AVHRR Browse Archive";
		String DODS_LatRange "60.5750","9.4250";
		String DODS_LonRange "-96.2220","-33.7780";
		String Sel_Vars "Sea_Temp";
		String Dod_Vars "dsp_band_1";
		#
		String DODS_StartDate "1979/4/11";
		String DODS_EndDate "2002/12/31";
	}
	DODS_Date {
		# This Catalog of the AVHRR Archive was created
		# converting the Archive's native "(base 1900)year/year-day"
		# into a prototype DODS-Date representation.
	
		# The DODS_Date prototype is as follows:
		#
		#       yyyy/mm/dd
		#
		# This prototype is purely experimental.

		String year_variable URI_Avhrr.year;
		String month_variable URI_Avhrr.month;
		String day_variable URI_Avhrr.day;
	}

	DODS_Time {
		# The DODS_Time prototype is as follows:
		#
		#       hh:mm:ss
		#
		# This prototype is purely experimental.

		String hours_variable URI_Avhrr.hours;
		String minutes_variable URI_Avhrr.minutes;
		String seconds_variable URI_Avhrr.seconds;
		String gmt_time true;
	}
	DODS_Date_Time {
		# The DODS_Date_Time prototype is as follows:
		#
		#       yyyy/mm/dd:hh:mm:ss
		#
		# This prototype is purely experimental.

		String year_variable URI.Avhrr.year;
		String month_variable URI_Avhrr.month;
		String day_variable URI_Avhrr.day;
		String hours_variable URI_Avhrr.hours;
		String minutes_variable URI_Avhrr.minutes;
		String seconds_variable URI_Avhrr.seconds;
		String gmt_time true;
	}
}	



