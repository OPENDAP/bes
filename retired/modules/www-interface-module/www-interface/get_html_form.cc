
// -*- mode: c++; c-basic-offset:4 -*-

// Copyright (c) 2006 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
// more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// This file holds the interface for the 'get data as ascii' function of the
// OPeNDAP/HAO data server. This function is called by the BES when it loads
// this as a module. The functions in the file ascii_val.cc also use this, so
// the same basic processing software can be used both by Hyrax and tie older
// Server3.

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <sstream>

#include <libdap/DataDDS.h>
#include <libdap/escaping.h>

#include "get_html_form.h"
#include "WWWOutput.h"
#include "WWWOutputFactory.h"

#include "WWWByte.h"
#include "WWWInt16.h"
#include "WWWUInt16.h"
#include "WWWInt32.h"
#include "WWWUInt32.h"
#include "WWWFloat32.h"
#include "WWWFloat64.h"
#include "WWWStr.h"
#include "WWWUrl.h"
#include "WWWArray.h"
#include "WWWStructure.h"
#include "WWWSequence.h"
#include "WWWGrid.h"

namespace dap_html_form {

#include "javascript.h"         // Try to hide this stuff...

// A better way to do this would have been to make WWWStructure, ..., inherit
// from both Structure and WWWOutput. But I didn't... jhrg 8/29/05
WWWOutput *wo = 0;

/** Given a BaseType variable, return a pointer to a new variable that has the
    same parent types (Byte, et c.) but is now one of the WWW specializations.

    @param bt A BaseType such as NCArray, HDFArray, ...
    @return A BaseType that uses the WWW specialization (e.g., WWWArray). */
BaseType *basetype_to_wwwtype(BaseType * bt)
{
    switch (bt->type()) {
    case dods_byte_c:
        return new WWWByte(dynamic_cast < Byte * >(bt));
    case dods_int16_c:
        return new WWWInt16(dynamic_cast < Int16 * >(bt));
    case dods_uint16_c:
        return new WWWUInt16(dynamic_cast < UInt16 * >(bt));
    case dods_int32_c:
        return new WWWInt32(dynamic_cast < Int32 * >(bt));
    case dods_uint32_c:
        return new WWWUInt32(dynamic_cast < UInt32 * >(bt));
    case dods_float32_c:
        return new WWWFloat32(dynamic_cast < Float32 * >(bt));
    case dods_float64_c:
        return new WWWFloat64(dynamic_cast < Float64 * >(bt));
    case dods_str_c:
        return new WWWStr(dynamic_cast < Str * >(bt));
    case dods_url_c:
        return new WWWUrl(dynamic_cast < Url * >(bt));
    case dods_array_c:
        return new WWWArray(dynamic_cast < Array * >(bt));
    case dods_structure_c:
        return new WWWStructure(dynamic_cast < Structure * >(bt));
    case dods_sequence_c:
        return new WWWSequence(dynamic_cast < Sequence * >(bt));
    case dods_grid_c:
        return new WWWGrid(dynamic_cast < Grid * >(bt));
    default:
        throw InternalErr(__FILE__, __LINE__, "Unknown type.");
    }
}

/** Given a DDS filled with variables that are one specialization of BaseType,
    build a second DDS which contains variables made using the WWW
    specialization.

    @param dds A DDS
    @return A DDS where each variable in \e dds is now a WWW* variable. */
DDS *dds_to_www_dds(DDS * dds)
{
#if 0
    // Using the factory class has no effect because it does not control
    // how a class like Structure or Grid builds child instances, so we have
    // to use the basetype_to_wwwtype() function.
    WWWOutputFactory wwwfactory;
    dds->set_factory(&wwwfactory);
#endif
    // Use the copy constructor to copy all the various private fields in DDS
    // including the attribute table for global attributes
    DDS *wwwdds = new DDS(*dds);

    // Because the DDS copy constructor copies the variables, we now, erase
    // them and...
    wwwdds->del_var(wwwdds->var_begin(), wwwdds->var_end());

    // Build copies of the variables using the WWW* types and manually add
    // their attribute tables.
    DDS::Vars_iter i = dds->var_begin();
    while (i != dds->var_end()) {
        BaseType *abt = basetype_to_wwwtype(*i);
        abt->set_attr_table((*i)->get_attr_table());
#if 0
        cerr << "dds attr: "; (*i)->get_attr_table().print(cerr); cerr << endl;
        cerr << "abt attr: "; abt->get_attr_table().print(cerr); cerr << endl;
#endif
        wwwdds->add_var(abt);
        // add_var makes a copy of the base type passed to it, so delete it
        // here
        delete abt;
        i++;
    }

#if 0
    // Should the following use WWWOutputFactory instead of the source DDS'
    // factory class?
    DDS *wwwdds = new DDS(dds->get_factory(), dds->get_dataset_name());

    wwwdds->set_attr_table(dds->get_attr_table);

    DDS::Vars_iter i = dds->var_begin();
    while (i != dds->var_end()) {
        BaseType *abt = basetype_to_wwwtype(*i);
        wwwdds->add_var(abt);
        // add_var makes a copy of the base type passed to it, so delete it
        // here
        delete abt;
        i++;
    }
#endif
    return wwwdds;
}

// This hack is needed because we have moved the #define of FILE_METHODS to
// the libdap header files until we can remove those methods from the library.
#undef FILE_METHODS

#ifdef FILE_METHODS
/** Using the stuff in WWWOutput and the hacked (specialized) print_val()
    methods in the WWW* classes, write out the HTML form interface.

    @todo Modify this code to expect a DDS whose BaseTypes are loaded with
    Attribute tables.

    @param dest Write HTML here.
    @param dds A DataDDS loaded with data; the BaseTypes in this must be
    WWW* instances,
    @param url The URL that should be initially displayed in the form. The
    form's javescript code will update this incrementally as the user builds
    a constraint.
    @param html_header Print a HTML header/footer for the page. True by default.
    @param admin_name "support@opendap.org" by default; use this as the
    address for support printed on the form.
    @param help_location "http://www.opendap.org/online_help_files/opendap_form_help.html"
    by default; otherwise, this is locataion where an HTML document that
    explains the page can be found.
*/
void write_html_form_interface(FILE * dest, DDS * dds,
                               const string & url, bool html_header,
                               const string & admin_name,
                               const string & help_location)
{
    wo = new WWWOutput(dest);

    if (html_header)
        wo->write_html_header();

    ostringstream oss;
    oss <<
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\"\n"
        << "\"http://www.w3.org/TR/REC-html40/loose.dtd\">\n" <<
        "<html><head><title>OPeNDAP Server Dataset Query Form</title>\n"
        << "<base href=\"" << help_location << "\">\n" <<
        "<script type=\"text/javascript\">\n"
        // Javascript code here
        << java_code << "\n"
        << "DODS_URL = new dods_url(\"" << url << "\");\n"
        << "</script>\n"

 	<< "<style id=\"antiClickjack\">body{display:none !important;}</style>\n" //cross frame fix.
 	<< "<script type=\"text/javascript\">\n"
 	<< "   if (self === top) {\n"
 	<< "       var antiClickjack = document.getElementById(\"antiClickjack\");\n"
 	<< "       antiClickjack.parentNode.removeChild(antiClickjack);\n"
 	<< "   } else {\n"
 	<< "       top.location = self.location;\n"
 	<< "   }\n"
 	<< "</script>\n"

        << "</head>\n"
        << "<body>\n"
        <<
        "<p><h2 align='center'>OPeNDAP Server Dataset Access Form</h2>\n"
        << "<hr>\n" << "<form action=\"\">\n" << "<table>\n";
    fprintf(dest, "%s", oss.str().c_str());

    wo->write_disposition(url);

    fprintf(dest, "<tr><td><td><hr>\n\n");

    wo->write_global_attributes(dds->get_attr_table());

    fprintf(dest, "<tr><td><td><hr>\n\n");

    wo->write_variable_entries(*dds);

    oss.str("");
    oss << "</table></form>\n\n" << "<hr>\n\n";
    oss << "<address>Send questions or comments to: <a href=\"mailto:"
        << admin_name << "\">" << admin_name << "</a></address>" << "<p>\n\
                    <a href=\"http://validator.w3.org/check?uri=referer\"><img\n\
                        src=\"http://www.w3.org/Icons/valid-html40\"\n\
                        alt=\"Valid HTML 4.0 Transitional\" height=\"31\" width=\"88\">\n\
                    </a></p>\n" << "</body></html>\n";

    fprintf(dest, "%s", oss.str().c_str());

}
#endif
/** Using the stuff in WWWOutput and the hacked (specialized) print_val()
    methods in the WWW* classes, write out the HTML form interface.

    @todo Modify this code to expect a DDS whose BaseTypes are loaded with
    Attribute tables.

    @param dest Write HTML here.
    @param dds A DataDDS loaded with data; the BaseTypes in this must be
    WWW* instances,
    @param url The URL that should be initially displayed in the form. The
    form's javascript code will update this incrementally as the user builds
    a constraint.
    @param html_header Print a HTML header/footer for the page. True by default.
    @param admin_name "support@opendap.org" by default; use this as the
    address for support printed on the form.
    @param help_location "http://www.opendap.org/online_help_files/opendap_form_help.html"
    by default; otherwise, this is locataion where an HTML document that
    explains the page can be found.
    */
void write_html_form_interface(ostream &strm, DDS * dds, const string & url, bool html_header,
		bool netcdf3_file_response, bool netcdf4_file_response, const string & admin_name,
		const string & help_location)
{
    wo = new WWWOutput(strm);

    if (html_header)
        wo->write_html_header();

    strm <<
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\"\n"
        << "\"http://www.w3.org/TR/REC-html40/loose.dtd\">\n" <<
        "<html><head><title>OPeNDAP Server Dataset Query Form</title>\n"
        << "<base href=\"" << help_location << "\">\n" <<
        "<script type=\"text/javascript\">\n"
        // Javascript code here
        << java_code << "\n"
        << "DODS_URL = new dods_url(\"" << url << "\");\n"
        << "</script>\n"

 	<< "<style id=\"antiClickjack\">body{display:none !important;}</style>\n" //cross frame fix.
 	<< "<script type=\"text/javascript\">\n"
 	<< "   if (self === top) {\n"
 	<< "       var antiClickjack = document.getElementById(\"antiClickjack\");\n"
 	<< "       antiClickjack.parentNode.removeChild(antiClickjack);\n"
 	<< "   } else {\n"
 	<< "       top.location = self.location;\n"
 	<< "   }\n"
 	<< "</script>\n"

        << "</head>\n"
        << "<body>\n"
        <<
        "<p><h2 align='center'>OPeNDAP Server Dataset Access Form</h2>\n"
        << "<hr>\n" << "<form action=\"\">\n" << "<table>\n";

    wo->write_disposition(url, netcdf3_file_response, netcdf4_file_response);

    strm << "<tr><td><td><hr>\n\n" ;

    wo->write_global_attributes(dds->get_attr_table());

    strm << "<tr><td><td><hr>\n\n" ;

    wo->write_variable_entries(*dds);

    strm << "</table></form>\n\n" << "<hr>\n\n";
    strm << "<address>Send questions or comments to: <a href=\"mailto:"
         << admin_name << "\">" << admin_name << "</a></address>" << "<p>\n\
                    <a href=\"http://validator.w3.org/check?uri=referer\"><img\n\
                        src=\"http://www.w3.org/Icons/valid-html40\"\n\
                        alt=\"Valid HTML 4.0 Transitional\" height=\"31\" width=\"88\">\n\
                    </a></p>\n" << "</body></html>\n";
}

const string allowable =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

// This function adds some text to the variable name so that conflicts with
// JavaScript's reserved words and other conflicts are avoided (or
// minimized...) 7/13/2001 jhrg
//
// I've modified this so that it no longer include the PID. Not sure why that
// was included... However, removing it will make using the HTML form in tests
// easier. jhrg 11/28/06
string name_for_js_code(const string & dods_name)
{
    return string("org_opendap_") +
        esc2underscore(id2www(dods_name, allowable));
}

/** Get the Fully Qualified Name for a variable.
    This function uses the BaseType name() and get_parent() methods to 'walk
    up' the hierarchy and build the FQN for a given variable.
    @return The FQN for a variable
    @param var The variable. */
string
get_fqn(BaseType *var)
{
    // I made this a local static object to avoid the repeated cost of creation
    // and also the issues with global static objects in dynamically-loaded
    // code.
    static const string dot = ".";

    // Testing !get_parent() means that the function returns the top most name.
    // If we tested !var instead it would return "" and the FQN would have a
    // leading dot. The test for !var is here as cheap insurance; it should
    // never happen.
    if (!var)
        return string("");
    else if (!var->get_parent())
        return var->name();
    else
        return get_fqn(var->get_parent()) + dot + var->name();
}

string fancy_typename(BaseType * v)
{
    switch (v->type()) {
    case dods_byte_c:
        return "Byte";
    case dods_int16_c:
        return "16 bit Integer";
    case dods_uint16_c:
        return "16 bit Unsigned integer";
    case dods_int32_c:
        return "32 bit Integer";
    case dods_uint32_c:
        return "32 bit Unsigned integer";
    case dods_float32_c:
        return "32 bit Real";
    case dods_float64_c:
        return "64 bit Real";
    case dods_str_c:
        return "string";
    case dods_url_c:
        return "URL";
    case dods_array_c:{
            ostringstream type;
            Array *a = (Array *) v;
            type << "Array of " << fancy_typename(a->var()) << "s ";
            for (Array::Dim_iter p = a->dim_begin(); p != a->dim_end();
                 ++p)
                type << "[" << a->dimension_name(p) << " = 0.." << a->
                    dimension_size(p, false) - 1 << "]";
            return type.str();
        }
    case dods_structure_c:
        return "Structure";
    case dods_sequence_c:
        return "Sequence";
    case dods_grid_c:{
        ostringstream type;
        Grid &g = dynamic_cast<Grid&>(*v);
        type << "Grid of " << fancy_typename(g.get_array());
#if 0
        << "s ";
        for (Grid::Map_iter p = g.map_begin(); p != g.map_end(); ++p) {
            Array &a = dynamic_cast<Array&>(**p);
            type << "[" << a.dimension_name(a.dim_begin()) << " = 0.." \
                 << a.dimension_size(a.dim_begin(), false) - 1 << "]";
        }
#endif
        return type.str();
    }
#if 0
        return "Grid";
#endif
        default:
        return "Unknown";
    }
}

/** This function is used by the Byte, ..., URL types to write their entries
    in the HTML Form. More complex classes do something else.
    @brief Output HTML entry for Scalars
    @param os Write to this output sink.
    @param name The name of the variable.
    @param type The name of the variable's data type. */
void
write_simple_variable(FILE * os, BaseType *var)
// const string & name, const string & type)
{
    ostringstream ss;
    write_simple_variable( ss, var); //name, type ) ;

    // Now write that string to os
    fprintf(os, "%s", ss.str().c_str());
}

/** This function is used by the Byte, ..., URL types to write their entries
    in the HTML Form. More complex classes do something else.
    @brief Output HTML entry for Scalars
    @param strm Write to this output stream.
    @param name The name of the variable.
    @param type The name of the variable's data type. */
void
write_simple_variable(ostream &strm, BaseType *var)
// const string & name, const string & type)
{
    const string fqn = get_fqn(var);

    strm << "<script type=\"text/javascript\">\n"
         << name_for_js_code(fqn) << " = new dods_var(\""
         << id2www_ce(fqn)
         << "\", \"" << name_for_js_code(fqn) << "\", 0);\n" <<
         "DODS_URL.add_dods_var(" << name_for_js_code(fqn) << ");\n"
         << "</script>\n";

    strm << "<b>"
         << "<input type=\"checkbox\" name=\"get_" << name_for_js_code(fqn)
         << "\"\n" << "onclick=\"" << name_for_js_code(fqn)
         << ".handle_projection_change(get_" << name_for_js_code(fqn)
         << ") \"  onfocus=\"describe_projection()\">\n" << "<font size=\"+1\">"
         << var->name() << "</font>" << ": "
         << fancy_typename(var) << "</b><br>\n\n";

    strm << var->name() << " <select name=\"" << name_for_js_code(fqn)
         << "_operator\"" << " onfocus=\"describe_operator()\""
         << " onchange=\"DODS_URL.update_url()\">\n"
         << "<option value=\"=\" selected>=\n"
         << "<option value=\"!=\">!=\n" << "<option value=\"<\"><\n"
         << "<option value=\"<=\"><=\n" << "<option value=\">\">>\n"
         << "<option value=\">=\">>=\n" << "<option value=\"-\">--\n"
         << "</select>\n";

    strm << "<input type=\"text\" name=\"" << name_for_js_code(fqn)
         << "_selection"
         << "\" size=12 onFocus=\"describe_selection()\" "
         << "onChange=\"DODS_URL.update_url()\">\n";

    strm << "<br>\n\n";
}
#if 0
void
write_simple_var_attributes(FILE * os, BaseType *var)
{
    ostringstream ss;
    write_simple_var_attributes(ss, var);

    // Now write that string to os
    fprintf(os, "%s", ss.str().c_str());
}

void
write_simple_var_attributes(ostream &os, int rows, int cols, BaseType *btp)
{
    AttrTable &attr = btp->get_attr_table();

    // Don't write anything if there are no attributes.
    if (attr.get_size() == 0) {
        return;
    }

    os << "<textarea name=\"" << btp->name()
	   << "_attr\" rows=\"" << rows
	   << "\" cols=\"" << cols << "\">\n" ;
    write_attributes(os, attr, "");
    os << "</textarea>\n\n" ;
}

void
write_attributes(ostream &os, AttrTable &attr, const string &prefix)
{
    for (AttrTable::Attr_iter a = attr.attr_begin(); a != attr.attr_end(); ++a) {
	if (attr.is_container(a))
	    write_attributes(os, attr.get_attr_table(a), (prefix == "") ? attr.get_name(a) : prefix + string(".") + attr.get_name(a));
	else {
	    if (prefix != "")
		os << prefix << "." << attr.get_name(a) << ": ";
	    else
		os << attr.get_name(a) << ": ";

	    int num_attr = attr.get_attr_num(a) - 1;
	    for (int i = 0; i < num_attr; ++i) {
		os << attr.get_attr(a, i) << ", ";
	    }
	    os << attr.get_attr(a, num_attr) << "\n";
	}
    }
}
#endif
} // namespace dap_html_form
