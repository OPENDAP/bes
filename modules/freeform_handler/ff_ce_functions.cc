
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

// (c) COPYRIGHT URI/MIT 1998-1999
// Please read the full copyright statement in the file COPYRIGHT.
//
// Authors:
//      jhrg,jimg       James Gallagher (jgallagher@gso.uri.edu)

// This file contains various functions for use with/in constraint
// expressions.

#include <iostream>
#include <string>
#include <algorithm>

//#define DODS_DEBUG

#include <BaseType.h>
#include <Str.h>
#include <Structure.h>
#include <Sequence.h>
#include <DDS.h>
#include <ConstraintEvaluator.h>
#include <ServerFunctionsList.h>
#include <Error.h>
#include <util.h>
#include <debug.h>

#include "date_proc.h"
#include "DODS_Date.h"
#include "DODS_Date_Factory.h"
#include "DODS_StartDate_Factory.h"
#include "DODS_EndDate_Factory.h"
#include "DODS_Time.h"
#include "DODS_Time_Factory.h"
#include "DODS_StartTime_Factory.h"
#include "DODS_EndTime_Factory.h"
#include "DODS_Date_Time.h"
#include "DODS_Date_Time_Factory.h"
#include "DODS_StartDate_Time_Factory.h"
#include "DODS_EndDate_Time_Factory.h"

#include "ff_ce_functions.h"
#include "FFStr.h"

/** Read an instance of T using a Factory for objects of type T. The Factory
    class for T must read configuration information from the DAS.

    @return An instance of the class T. */

template < class T, class T_Factory >
inline static T get_instance(DDS & dds)
{
    return T_Factory(dds).get();
}

/** Compare an instance of T read from the dataset with the strings in one or
    two DODS Str variables. The Strs are passed into the function using
    BaseType pointers since that is how the constraint expression evaluator
    passes arguments to functions.

    @param argc The number of elements in argv[].
    @param argv[] An array of arguments.
    @param dds The DDS for the dataset.
    @return If one argument is given, return true if the value read from the
    dataset matches the argument value. If two arguments are given, return
    true if the value read from the dataset falls within (inclusive) of the
    two arguments given. */

template < class T, class T_Factory >
static bool comparison(int argc, BaseType * argv[], DDS & dds)
{
    if (argc < 1 || argc > 2)
        throw Error(malformed_expr,
                    "Wrong number of arguments to a constraint expression function.");

    DBG(cerr << "comparision: argc: " << argc << endl);

    T t1(argv[0]);
    T t2;
    if (argc == 2)
        t2.set(argv[1]);

    T current = get_instance < T, T_Factory > (dds);

    DBG(cerr << "t1: " << t1.get(iso8601) << endl);
    if (argc == 2) {
    	DBG(cerr << "t2 (1): " << t2.get(iso8601) << endl);
    }
    DBG(cerr << "current: " << current.get(iso8601) << endl);

    if (argc == 2)
        return ((t1 <= current) && (t2 >= current));
    else
        return (t1 == current);
}

/** Compare an instance of T read from the dataset with the strings in one or
    two DODS Str variables. The Strs are passed into the function using
    BaseType pointers since that is how the constraint expression evaluator
    passes arguments to functions.

    @param argc The number of elements in argv[].
    @param argv[] An array of arguments.
    @param dds The DDS for the dataset.
    @return If one argument is given, return true if the value read from the
    dataset matches the argument value. If two arguments are given, return
    true if the value read from the dataset falls within (inclusive) of the
    two arguments given. */

template < class T1, class T1_Factory, class T2, class T2_Factory >
    static bool range_comparison(int argc, BaseType * argv[], DDS & dds)
{
    if (argc != 2)
        throw Error(malformed_expr,
                    "Wrong number of arguments to a constraint expression function.");

    T1 t1(argv[0]);
    T2 t2(argv[1]);

    T1 current_start = get_instance < T1, T1_Factory > (dds);
    T2 current_end = get_instance < T2, T2_Factory > (dds);

    return (((current_start >= t1) && (current_start <= t2)) ||
            ((current_end >= t1) && (current_end <= t2)) ||
            ((current_start <= t1) && (current_end >= t2)));
}

/** Load a new Str variable into the DDS. If position is given, then insert
    the new Str variable into that Structure or Sequence. If position is
    given and is not a Structure or Sequence, throw an exception.

    @param name The name of the new variable.
    @param dds The DDS of the dataset.
    @param position Add the new variable to this Structure or Sequence. */

static void
new_string_variable(const string & name, DDS & dds, BaseType * position = 0)
{
    // Create the new variable

    Str *new_variable = new FFStr(name, "");
    new_variable->set_read_p(true);     // You must call this before ...
    new_variable->set_synthesized_p(true);      // this! Look at BaseType.cc.

    // Add it to the DDS in the right place

    if (position) {
        switch (position->type()) {
        case dods_structure_c:{
                Structure *sp = (Structure *) position;
                sp->add_var((BaseType *) new_variable);
                break;
            }

        case dods_sequence_c:{
                Sequence *sp = (Sequence *) position;
                sp->add_var((BaseType *) new_variable);
                break;
            }

        default:
            delete new_variable;
            throw Error(malformed_expr,
                        "You asked me to insert the synthesized variable in \n\
something that did not exist or was not a constructor \n\
type (e.g., a structure, sequence, ...).");
            break;
        }
    } 
    else {
        dds.add_var(new_variable);
    }

    // Mark the variable as part of the current projection.

    dds.mark(name, true);       // Don't just call set_send_p()!
    
    delete new_variable;
}

static void func_date(int argc, BaseType * argv[], DDS & dds, bool *result)
{
	DBG(cerr << "calling func_date" << endl);
    *result = comparison < DODS_Date, DODS_Date_Factory > (argc, argv, dds);
    DBG(cerr << "result: " << *result << endl << endl);
}

static void func_startdate(int argc, BaseType * argv[], DDS & dds, bool *result)
{
	DBG(cerr << "calling func_startdate" << endl);
    *result = comparison < DODS_Date, DODS_StartDate_Factory > (argc, argv,
                                                             dds);
}

static void func_enddate(int argc, BaseType * argv[], DDS & dds, bool *result)
{
	DBG(cerr << "calling func_enddate" << endl);
    *result = comparison < DODS_Date, DODS_EndDate_Factory > (argc, argv,
                                                           dds);
}

static void func_date_range(int argc, BaseType * argv[], DDS & dds, bool *result)
{
    *result = range_comparison < DODS_Date, DODS_StartDate_Factory, DODS_Date,
        DODS_EndDate_Factory > (argc, argv, dds);
}

static void func_time(int argc, BaseType * argv[], DDS & dds, bool *result)
{
    *result = comparison < DODS_Time, DODS_Time_Factory > (argc, argv, dds);
}

static void func_starttime(int argc, BaseType * argv[], DDS & dds, bool *result)
{
    *result = comparison < DODS_Time, DODS_StartTime_Factory > (argc, argv,
                                                             dds);
}

static void func_endtime(int argc, BaseType * argv[], DDS & dds, bool *result)
{
    *result = comparison < DODS_Time, DODS_EndTime_Factory > (argc, argv,
                                                           dds);
}

// This comparision function should be used for decimal dates. 5/29/99 jhrg

static void func_date_time(int argc, BaseType * argv[], DDS & dds, bool *result)
{
    *result = comparison < DODS_Date_Time, DODS_Date_Time_Factory > (argc,
                                                                  argv,
                                                                  dds);
}

static void func_startdate_time(int argc, BaseType * argv[], DDS & dds, bool *result)
{
    *result = comparison < DODS_Date_Time,
        DODS_StartDate_Time_Factory > (argc, argv, dds);
}

static void func_enddate_time(int argc, BaseType * argv[], DDS & dds, bool *result)
{
    *result = comparison < DODS_Date_Time, DODS_EndDate_Time_Factory > (argc,
                                                                     argv,
                                                                     dds);
}

// This function is added to the selection part of the CE when the matching
// `projection function' is run.

// The date and date_time functions should now recognize decimal format years
// and date-times. 5/30/99 jhrg

static void sel_dods_jdate(int argc, BaseType *[], DDS & dds, bool *result)
{
    if (argc != 0)
        throw Error(malformed_expr,
                    "Wrong number of arguments to internal selection function.\n\
Please report this error.");

    DODS_Date current =
        get_instance < DODS_Date, DODS_Date_Factory > (dds);

    // Stuff the yyyy/ddd string into DODS_JDate.
    Str *dods_jdate = (Str *) dds.var("DODS_JDate");
    // By calling DODS_Date::get with the token `yd' I'm explicitly asking
    // for the year/day (pseudo juilian) date format. 5/27/99 jhrg
    string s = current.get(yd).c_str();
    dods_jdate->val2buf(&s);

    *result = true;
}

// A projection function: This adds a new variable to the DDS and arranges
// for the matching selection function (above) to be called.

static void
proj_dods_jdate(int argc, BaseType * argv[], DDS & dds,
                ConstraintEvaluator & ce)
{
    if (argc < 0 || argc > 1)
        throw Error(malformed_expr,
                    "Wrong number of arguments to projection function.\n\
Expected zero or one arguments.");

    new_string_variable("DODS_JDate", dds, (argc == 1) ? argv[0] : 0);

    // Add the selection function to the CE

    ce.append_clause(sel_dods_jdate, 0);        // 0 == no BaseType args
}

// Same as the above function, but for ymd dates.

static void sel_dods_date(int argc, BaseType *[], DDS & dds, bool *result)
{
    if (argc != 0)
        throw Error(malformed_expr,
                    "Wrong number of arguments to internal selection function.\n\
Please report this error.");

    DODS_Date current =
        get_instance < DODS_Date, DODS_Date_Factory > (dds);

    // Stuff the yyyy/ddd string into DODS_Date.
    Str *dods_date = (Str *) dds.var("DODS_Date");
    // Calling the regular form of DODS_Date::get() returns the data in y/m/d
    // format. 5/27/99 jhrg
    string s = current.get().c_str();
    dods_date->val2buf(&s);

    *result = true;
}

static void
proj_dods_date(int argc, BaseType * argv[], DDS & dds,
               ConstraintEvaluator & ce)
{
    if (argc < 0 || argc > 1)
        throw Error(malformed_expr,
                    "Wrong number of arguments to projection function.\n\
Expected zero or one arguments.");

    new_string_variable("DODS_Date", dds, (argc == 1) ? argv[0] : 0);

    // Add the selection function to the CE

    ce.append_clause(sel_dods_date, 0); // 0 == no BaseType args
}

/************************ DODS_Time functions *************************/


static void sel_dods_time(int argc, BaseType *[], DDS & dds, bool *result)
{
    if (argc != 0)
        throw Error(malformed_expr,
                    "Wrong number of arguments to internal selection function.\n\
Please report this error.");

    DODS_Time current =
        get_instance < DODS_Time, DODS_Time_Factory > (dds);

    // Stuff the "hh:mm:ss" string into `DODS_Time'
    Str *dods_time = (Str *) dds.var("DODS_Time");
    string s = current.get().c_str();
    dods_time->val2buf(&s);

    *result = true;
}

static void
proj_dods_time(int argc, BaseType * argv[], DDS & dds,
               ConstraintEvaluator & ce)
{
    if (argc < 0 || argc > 1)
        throw Error(malformed_expr,
                    "Wrong number of arguments to projection function.\n\
Expected zero or one arguments.");

    // Create the new variable

    new_string_variable("DODS_Time", dds, (argc == 1) ? argv[0] : 0);

    // Add the selection function to the CE

    ce.append_clause(sel_dods_time, 0); // 0 == no BaseType args
}

/*************************** Date/Time functions *************************/

// This function is added to the selection part of the CE when the matching
// `projection function' is run.

static void sel_dods_date_time(int argc, BaseType *[], DDS & dds, bool *result)
{
    if (argc != 0)
        throw Error(malformed_expr,
                    "Wrong number of arguments to internal selection function.\n\
Please report this error.");

    DODS_Date_Time current
        = get_instance < DODS_Date_Time, DODS_Date_Time_Factory > (dds);

    Str *dods_date_time = (Str *) dds.var("DODS_Date_Time");
    string s = current.get().c_str();
    dods_date_time->val2buf(&s);

    *result = true;
}

// A projection function: This adds a new variable to the DDS and arranges
// for the matching selection function (above) to be called.

static void
proj_dods_date_time(int argc, BaseType * argv[], DDS & dds,
                    ConstraintEvaluator & ce)
{
    if (argc < 0 || argc > 1)
        throw Error(malformed_expr,
                    "Wrong number of arguments to projection function.\n\
Expected zero or one arguments.");

    // Create the new variable

    new_string_variable("DODS_Date_Time", dds, (argc == 1) ? argv[0] : 0);

    // Add the selection function to the CE

    ce.append_clause(sel_dods_date_time, 0);    // 0 == no BaseType args
}

/*************************** Decimal/Year functions *************************/

// This function is added to the selection part of the CE when the matching
// `projection function' is run.

static void sel_dods_decimal_year(int argc, BaseType *[], DDS & dds, bool *result)
{
    if (argc != 0)
        throw Error(malformed_expr,
                    "Wrong number of arguments to internal selection function.\n\
Please report this error.");

    DODS_Date_Time current
        = get_instance < DODS_Date_Time, DODS_Date_Time_Factory > (dds);

    // Stuff the yyyy/ddd string into DODS_JDate.
    Str *dods_decimal_year = (Str *) dds.var("DODS_Decimal_Year");
    string s = current.get(decimal);
    dods_decimal_year->val2buf(&s);

    *result = true;
}

// A projection function: This adds a new variable to the DDS and arranges
// for the matching selection function (above) to be called.

static void
proj_dods_decimal_year(int argc, BaseType * argv[], DDS & dds,
                       ConstraintEvaluator & ce)
{
    if (argc < 0 || argc > 1)
        throw Error(malformed_expr,
                    "Wrong number of arguments to projection function.\n\
Expected zero or one arguments.");

    // Create the new variable

    new_string_variable("DODS_Decimal_Year", dds,
                        (argc == 1) ? argv[0] : 0);

    // Add the selection function to the CE

    ce.append_clause(sel_dods_decimal_year, 0); // 0 == no BaseType args
}

/*************************** Decimal/Year functions *************************/

// This function is added to the selection part of the CE when the matching
// `projection function' is run.

static void sel_dods_startdecimal_year(int argc, BaseType *[], DDS & dds, bool *result)
{
    if (argc != 0)
        throw Error(malformed_expr,
                    "Wrong number of arguments to internal selection function.\n\
Please report this error.");

    DODS_Date_Time current
        =
        get_instance < DODS_Date_Time, DODS_StartDate_Time_Factory > (dds);

    // Stuff the yyyy/ddd string into DODS_JDate.
    Str *dods_decimal_year = (Str *) dds.var("DODS_StartDecimal_Year");
    string s = current.get(decimal);
    dods_decimal_year->val2buf(&s);

    *result = true;
}

// A projection function: This adds a new variable to the DDS and arranges
// for the matching selection function (above) to be called.

static void
proj_dods_startdecimal_year(int argc, BaseType * argv[], DDS & dds,
                            ConstraintEvaluator & ce)
{
    if (argc < 0 || argc > 1)
        throw Error(malformed_expr,
                    "Wrong number of arguments to projection function.\n\
Expected zero or one arguments.");

    // Create the new variable

    new_string_variable("DODS_StartDecimal_Year", dds,
                        (argc == 1) ? argv[0] : 0);

    // Add the selection function to the CE

    ce.append_clause(sel_dods_startdecimal_year, 0);    // 0 == no BaseType args
}

/*************************** Decimal/Year functions *************************/

// This function is added to the selection part of the CE when the matching
// `projection function' is run.

static void sel_dods_enddecimal_year(int argc, BaseType *[], DDS & dds, bool *result)
{
    if (argc != 0)
        throw Error(malformed_expr,
                    "Wrong number of arguments to internal selection function.\n\
Please report this error.");

    DODS_Date_Time current
        = get_instance < DODS_Date_Time, DODS_EndDate_Time_Factory > (dds);

    // Stuff the yyyy/ddd string into DODS_JDate.
    Str *dods_decimal_year = (Str *) dds.var("DODS_EndDecimal_Year");
    string s = current.get(decimal);
    dods_decimal_year->val2buf(&s);

    *result = true;
}

// A projection function: This adds a new variable to the DDS and arranges
// for the matching selection function (above) to be called.

static void
proj_dods_enddecimal_year(int argc, BaseType * argv[], DDS & dds,
                          ConstraintEvaluator & ce)
{
    if (argc < 0 || argc > 1)
        throw Error(malformed_expr,
                    "Wrong number of arguments to projection function.\n\
Expected zero or one arguments.");

    // Create the new variable

    new_string_variable("DODS_EndDecimal_Year", dds,
                        (argc == 1) ? argv[0] : 0);

    // Add the selection function to the CE

    ce.append_clause(sel_dods_enddecimal_year, 0);      // 0 == no BaseType args
}

/************************ DODS_StartDate functions *************************/

static void sel_dods_startdate(int argc, BaseType *[], DDS & dds, bool *result)
{
    if (argc != 0)
        throw Error(malformed_expr,
                    "Wrong number of arguments to internal selection function.\n\
Please report this error.");

    DODS_Date current =
        get_instance < DODS_Date, DODS_StartDate_Factory > (dds);

    // Stuff the yyyy/ddd string into DODS_StartDate.
    Str *dods_date = (Str *) dds.var("DODS_StartDate");
    // Calling the regular form of DODS_Date::get() returns the data in y/m/d
    // format. 5/27/99 jhrg
    string s = current.get().c_str();
    dods_date->val2buf(&s);

    *result = true;
}

static void
proj_dods_startdate(int argc, BaseType * argv[], DDS & dds,
                    ConstraintEvaluator & ce)
{
    if (argc < 0 || argc > 1)
        throw Error(malformed_expr,
                    "Wrong number of arguments to projection function.\n\
Expected zero or one arguments.");

    new_string_variable("DODS_StartDate", dds, (argc == 1) ? argv[0] : 0);

    // Add the selection function to the CE

    ce.append_clause(sel_dods_startdate, 0);    // 0 == no BaseType args
}

/************************ DODS_StartTime functions *************************/

static void sel_dods_starttime(int argc, BaseType *[], DDS & dds, bool *result)
{
    if (argc != 0)
        throw Error(malformed_expr,
                    "Wrong number of arguments to internal selection function.\n\
Please report this error.");

    DODS_Time current =
        get_instance < DODS_Time, DODS_StartTime_Factory > (dds);

    // Stuff the "hh:mm:ss" string into `DODS_Time'
    Str *dods_time = (Str *) dds.var("DODS_StartTime");
    string s = current.get().c_str();
    dods_time->val2buf(&s);

    *result = true;
}

static void
proj_dods_starttime(int argc, BaseType * argv[], DDS & dds,
                    ConstraintEvaluator & ce)
{
    if (argc < 0 || argc > 1)
        throw Error(malformed_expr,
                    "Wrong number of arguments to projection function.\n\
Expected zero or one arguments.");

    // Create the new variable

    new_string_variable("DODS_StartTime", dds, (argc == 1) ? argv[0] : 0);

    // Add the selection function to the CE

    ce.append_clause(sel_dods_starttime, 0);    // 0 == no BaseType args
}

/*************************** StartDate/Time functions *************************/

// This function is added to the selection part of the CE when the matching
// `projection function' is run.

static void sel_dods_startdate_time(int argc, BaseType *[], DDS & dds, bool *result)
{
    if (argc != 0)
        throw Error(malformed_expr,
                    "Wrong number of arguments to internal selection function.\n\
Please report this error.");

    DODS_Date_Time current
        =
        get_instance < DODS_Date_Time, DODS_StartDate_Time_Factory > (dds);

    Str *dods_date_time = (Str *) dds.var("DODS_StartDate_Time");
    string s = current.get().c_str();
    dods_date_time->val2buf(&s);

    *result = true;
}

// A projection function: This adds a new variable to the DDS and arranges
// for the matching selection function (above) to be called.

static void
proj_dods_startdate_time(int argc, BaseType * argv[], DDS & dds,
                         ConstraintEvaluator & ce)
{
    if (argc < 0 || argc > 1)
        throw Error(malformed_expr,
                    "Wrong number of arguments to projection function.\n\
Expected zero or one arguments.");

    // Create the new variable

    new_string_variable("DODS_StartDate_Time", dds,
                        (argc == 1) ? argv[0] : 0);

    // Add the selection function to the CE

    ce.append_clause(sel_dods_startdate_time, 0);       // 0 == no BaseType args
}

/************************ DODS_EndDate functions *************************/

static void sel_dods_enddate(int argc, BaseType *[], DDS & dds, bool *result)
{
    if (argc != 0)
        throw Error(malformed_expr,
                    "Wrong number of arguments to internal selection function.\n\
Please report this error.");

    DODS_Date current =
        get_instance < DODS_Date, DODS_EndDate_Factory > (dds);

    // Stuff the yyyy/ddd string into DODS_EndDate.
    Str *dods_date = (Str *) dds.var("DODS_EndDate");
    // Calling the regular form of DODS_Date::get() returns the data in y/m/d
    // format. 5/27/99 jhrg
    string s = current.get().c_str();
    dods_date->val2buf(&s);

    *result = true;
}

static void
proj_dods_enddate(int argc, BaseType * argv[], DDS & dds,
                  ConstraintEvaluator & ce)
{
    if (argc < 0 || argc > 1)
        throw Error(malformed_expr,
                    "Wrong number of arguments to projection function.\n\
Expected zero or one arguments.");

    new_string_variable("DODS_EndDate", dds, (argc == 1) ? argv[0] : 0);

    // Add the selection function to the CE

    ce.append_clause(sel_dods_enddate, 0);      // 0 == no BaseType args
}

/************************ DODS_EndTime functions *************************/

static void sel_dods_endtime(int argc, BaseType *[], DDS & dds, bool *result)
{
    if (argc != 0)
        throw Error(malformed_expr,
                    "Wrong number of arguments to internal selection function.\n\
Please report this error.");

    DODS_Time current =
        get_instance < DODS_Time, DODS_EndTime_Factory > (dds);

    // Stuff the "hh:mm:ss" string into `DODS_Time'
    Str *dods_time = (Str *) dds.var("DODS_EndTime");
    string s = current.get().c_str();
    dods_time->val2buf(&s);

    *result = true;
}

static void
proj_dods_endtime(int argc, BaseType * argv[], DDS & dds,
                  ConstraintEvaluator & ce)
{
    if (argc < 0 || argc > 1)
        throw Error(malformed_expr,
                    "Wrong number of arguments to projection function.\n\
Expected zero or one arguments.");

    // Create the new variable

    new_string_variable("DODS_EndTime", dds, (argc == 1) ? argv[0] : 0);

    // Add the selection function to the CE

    ce.append_clause(sel_dods_endtime, 0);      // 0 == no BaseType args
}

/*************************** EndDate/Time functions *************************/

// This function is added to the selection part of the CE when the matching
// `projection function' is run.

static void sel_dods_enddate_time(int argc, BaseType *[], DDS & dds, bool *result)
{
    if (argc != 0)
        throw Error(malformed_expr,
                    "Wrong number of arguments to internal selection function.\n\
Please report this error.");

    DODS_Date_Time current
        = get_instance < DODS_Date_Time, DODS_EndDate_Time_Factory > (dds);

    Str *dods_date_time = (Str *) dds.var("DODS_EndDate_Time");
    string s = current.get().c_str();
    dods_date_time->val2buf(&s);

    *result = true;
}

// A projection function: This adds a new variable to the DDS and arranges
// for the matching selection function (above) to be called.

static void
proj_dods_enddate_time(int argc, BaseType * argv[], DDS & dds,
                       ConstraintEvaluator & ce)
{
    if (argc < 0 || argc > 1)
        throw Error(malformed_expr,
                    "Wrong number of arguments to projection function.\n\
Expected zero or one arguments.");

    // Create the new variable

    new_string_variable("DODS_EndDate_Time", dds,
                        (argc == 1) ? argv[0] : 0);

    // Add the selection function to the CE

    ce.append_clause(sel_dods_enddate_time, 0); // 0 == no BaseType args
}

void ff_register_functions()
{
    libdap::ServerFunction *ff_dap_function;

    // The first set of functions all are 'Selection functions.' They are used
    // in the selection part of a CE to choose which instances of a sequence
    // will be sent. Each of these particular functions compare a given date,
    // or range of dates, to the date information in the sequence instance.
    // These function are very specific to sequences that contain date/time
    // information.

    ff_dap_function = new libdap::ServerFunction(
        // The name of the function as it will appear in a constraint expression
        "date",
        // The version of the function
        "1.0",
        // A brief description of the function
        string("Compares the current variable to the date parameters. If only one parameter is passed then ") +
        "this returns true if they're the same date. If two parameters are passed they are considered to be the beginning "+
        "and end of a date range and the function returns true if the current variable falls in the range.",
        // A usage/syntax statement
        "date(date_1[, date_2])",
        // A URL that points two a web page describing the function
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        // A URI that defines the role of the function
        "http://services.opendap.org/dap4/freeform-function/date",
        // A pointer to the helloWorld() function
        func_date
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "date_range",
        "1.0",
        string("Compares the current sequence instance with the passed date range. Return true if the instance is within the range. ")+
        "If only single date parameter is used as a start date to now.",
        "date_range(startDate, endDate)",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/date_range",
        func_date_range
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "start_date",
        "1.0",
        string("The current sequence instance is compared, as a start date, to the single passed parameter. Returns true if they're the same date. ")+
        "If two parameters are passed they are considered a date range. True is returned if the current variable falls into the range.",
        "start_date(date[, endDate])",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/start_date",
        func_startdate
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "end_date",
        "1.0",
        string("The current sequence instance is compared, as an end date, to the single passed parameter. Returns true if they're the same date. ") +
        "If two parameters are passed they are considered a date range. True is returned if the current variable falls into the range.",
        "end_date(date[, endDate])",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/end_date",
        func_enddate
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "time",
        "1.0",
        "Compares the current variable with the passed time range. Return true if the sequence instance is within the range.",
        "time( time[, endTime] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/time",
        func_time
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "start_time",
        "1.0",
        "Compares the current variable with the passed time range. Return true if the sequence instance is within the range.",
        "start_time( time[, endTime] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/start_time",
        func_starttime
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "end_time",
        "1.0",
        "Compares the current variable with the passed time range. Return true if the sequence instance is within the range.",
        "end_time( time[, endTime] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/end_time",
        func_endtime
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "date_time",
        "1.0",
        "Compares the current sequence instance with the passed date/time range. Return true if the instance is within the range.",
        "date_time( date/time [, end date/time] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/date_time",
        func_date_time
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "start_date_time",
        "1.0",
        "Compares the current sequence instance with the passed date/time range. Return true if the instance is within the range.",
        "start_date_time( date/time [, end date/time] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/start_date_time",
        func_startdate_time
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "end_date_time",
        "1.0",
        "Compares the current sequence instance with the passed date/time range. Return true if the instance is within the range.",
        "end_date_time(date/time [, end date/time] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/end_date_time",
        func_enddate_time
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    // These are 'Projection Functions' that add new variables to a dataset.
    // Their operation might be superceeded by NCML (except that NCML doesn't
    // do exactly what they do, but it could be extended...). The function
    // is used in the projection part of a URL.

    ff_dap_function = new libdap::ServerFunction(
        "DODS_Time",
        "1.0",
        "Adds a variable named DODS_Time that is an ISO 8601 time string to the dataset.",
        "DODS_Time( [Sequence or Structure] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/DODS_Time",
        proj_dods_time
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "DODS_StartTime",
        "1.0",
        "Adds a variable named DODS_StartTime that is an ISO 8601 time string to the dataset.",
        "DODS_StartTime( [Sequence or Structure] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/DODS_StartTime",
        proj_dods_starttime
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "DODS_EndTime",
        "1.0",
        "Adds a variable named DODS_EndTime that is an ISO 8601 time string to the dataset.",
        "DODS_EndTime( [Sequence or Structure] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/DODS_EndTime",
        proj_dods_endtime
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "DODS_JDate",
        "1.0",
        "Adds a variable named DODS_JDate that is the Julian day number to the dataset.",
        "DODS_JDate( [Sequence or Structure variable] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/DODS_JDATE",
        proj_dods_jdate
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "DODS_Date",
        "1.0",
        string("Adds a variable named DODS_Date that is an ISO 8601 date string to the dataset."),
        "DODS_Date( [Sequence or Structure] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/DODS_Date",
        proj_dods_date
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "DODS_StartDate",
        "1.0",
        string("Adds a variable named DODS_StartDate that is an ISO 8601 date string to the dataset."),
        "DODS_StartDate( [Sequence or Structure] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/DODS_StartDate",
        proj_dods_startdate
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "DODS_EndDate",
        "1.0",
        string("Adds a variable named DODS_EndDate that is an ISO 8601 date string to the dataset."),
        "DODS_EndDate( [Sequence or Structure] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/DODS_EndDate",
        proj_dods_enddate
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);
    ff_dap_function = new libdap::ServerFunction(
        "DODS_Date_Time",
        "1.0",
        "Adds a variable named DODS_Date_Time that is an ISO 8601 date/time string to the dataset.",
        "DODS_Date_Time( [Sequence or Structure] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/DODS_Date_Time",
        proj_dods_date_time
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "DODS_StartDate_Time",
        "1.0",
        "Adds a variable named DODS_StartDate_Time that is an ISO 8601 date/time string to the dataset.",
        "DODS_StartDate_Time( [Sequence or Structure] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/DODS_StartDate_Time",
        proj_dods_startdate_time
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "DODS_EndDate_Time",
        "1.0",
        "Adds a variable named DODS_EndDate_Time that is an ISO 8601 date/time string to the dataset.",
        "DODS_EndDate_Time( [Sequence or Structure] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/DODS_EndDate_Time",
        proj_dods_enddate_time
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "DODS_Decimal_Year",
        "1.0",
        "Adds a variable named DODS_Decimal_Year that is date as a decimal year/day value string to the dataset.",
        "DODS_Decimal_Year( [Sequence or Structure] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/DODS_Decimal_Year",
        proj_dods_decimal_year
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "DODS_StartDecimal_Year",
        "1.0",
        "Adds a variable named DODS_StartDecimal_Year that is date as a decimal year/day value string to the dataset.",
        "DODS_StartDecimal_Year( [Sequence or Structure] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/DODS_StartDecimal_Year",
        proj_dods_startdecimal_year
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);

    ff_dap_function = new libdap::ServerFunction(
        "DODS_EndDecimal_Year",
        "1.0",
        "Adds a variable named DODS_EndDecimal_Year that is date as a decimal year/day value string to the dataset.",
        "DODS_EndDecimal_Year( [Sequence or Structure] )",
        "http://docs.opendap.org/index.php/Server_Side_Processing_Functions#FreeForm_Functions",
        "http://services.opendap.org/dap4/freeform-function/DODS_EndDecimal_Year",
        proj_dods_enddecimal_year
    );
    libdap::ServerFunctionsList::TheList()->add_function(ff_dap_function);
}
