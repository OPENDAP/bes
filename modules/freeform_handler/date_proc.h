
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ff_handler a FreeForm API handler for the OPeNDAP
// DAP2 data server.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
// 
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.


// (c) COPYRIGHT URI/MIT 1998
// Please read the full copyright statement in the file COPYRIGHT.
//
// Authors:
//      jhrg,jimg       James Gallagher (jgallagher@gso.uri.edu)

int is_leap(int year);
double days_in_year(int year);
long julian_day(int year, int month, int day);
void gregorian_date(double jd, int *year, int *month, int *day, int *hours,
		    int *minutes, double *sec);
int month_day_to_days(int year, int month, int day);
void days_to_month_day(int year, int ddd, int *month, int *day);

int dayofweek(double j);
int days_in_month(int year, int month);

// $Log: date_proc.h,v $
// Revision 1.5  2001/09/28 23:19:43  jimg
// Merged with 3.2.3.
//
// Revision 1.4.2.2  2001/05/23 20:11:22  dan
// Modified to support year/month date representations,
// and to support ISO8601 output formats.
//
// Revision 1.4.2.1  2001/05/23 18:14:53  jimg
// Merged with changes on the release-3-1 branch. This apparently was not
// done corrrectly the first time around.
//
// Revision 1.4  2000/10/11 19:37:56  jimg
// Moved the CVS log entries to the end of files.
// Changed the definition of the read method to match the dap library.
// Added exception handling.
// Added exceptions to the read methods.
//
// Revision 1.3  2000/08/31 22:16:55  jimg
// Merged with 3.1.7
//
// Revision 1.2.24.1  2000/08/03 20:16:27  jimg
// The is_leap and days_in_year functions are now externally visible. This
// should be the only place where we calculate leap year stuff.
//
// Revision 1.2  1998/11/10 17:46:56  jimg
// Added this log.
//
