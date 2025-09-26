// This file is part of hdf5_handler - an HDF5 file handler for the OPeNDAP
// data server.
// Copyright (c) The HDF Group, Inc. and OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign IL 61820
// Authors: 
// Hyo-Kyung Lee <hyoklee@hdfgroup.org>
// Kent Yang <myang6@hdfgroup.org> 
// The code is mainly adopted from hdfeos.yy of the HDF4 handler. 

%code requires {

#define YYSTYPE char *
#define YYDEBUG 0
#define YYERROR_VERBOSE 0

// Uncomment the following line for debugging.
//#define VERBOSE 
//#define YYPARSE_PARAM he5parser

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <assert.h>
#include <vector>
#include <map>
#include <sstream>
//#include "he5dds.tab.hh"
#include "HE5Parser.h"
 
using namespace std;

// It's defined in he5dds.lex.
//extern int yy_line_num;	

} // code requires

%code {
// This is a flag to indicate if parser is reading geolocatoin or data
// variable.
// This should be changed to a enum type parser state variable later.
bool swath_is_geo_field = false;
string dimension_name = "";

void he5ddserror(HE5Parser *he5parser, char *s);
int  he5ddslex(void);

} //code

%require "2.4"

%parse-param {HE5Parser *he5parser}
%name-prefix "he5dds"
%defines
%debug
%verbose

%token GROUP
%token END_GROUP
%token OBJECT
%token END_OBJECT

%token GRID_NAME  
%token SWATH_NAME 
%token ZA_NAME


%token DEFAULT                  // is it used?
%token XDIM
%token YDIM
%token UPPERLEFTPT
%token LOWERRIGHTPT
%token PIXELREGISTRATION	
%token PROJECTION
%token GRIDORIGIN			
%token SPHERECODE
%token ZONECODE

%token DIMENSION_NAME  
%token DIMENSION_SIZE  
%token DATA_FIELD_NAME
%token GEO_FIELD_NAME		// for Swath only

%token DATA_TYPE 
%token DIMENSION_LIST
// UNCOMMENT OUT the line below to retrieve the maximum dimension list. ALSO NEED TO ADD MAX_DIMENSION_LIST at  he5dds.lex.
//%token MAX_DIMENSION_LIST 
%token COMPRESSION_TYPE

%token INT
%token FLOAT
%token STR



%%
attribute_list: // empty 
| attribute_list object
| attribute_list attribute
| attribute_list group;
dataseq:
| INT 
| '"' STR '"'
| '(' 
dataseq1
')' 
;

dataseq1: data
| dataseq1 ',' data
;

data: // empty 
| INT
{
#ifdef VERBOSE
    cout << "data: " << $1 
         << "parser_state: " 
         << ((HE5Parser*)(he5parser))->parser_state 
         << endl;	   
#endif	   
    // We find that some products have dimension names "1", "2", etc., so add this parsing. KY 2012-12-4
    HE5Parser* p = (HE5Parser*)he5parser;
    if(p->parser_state == 10){ // THis is parsing DimList. 
        string a;
        a = a.append($$);
        HE5Dim d;
        d.name = a;
        d.size = 0;             /* <hyokyung 2011.12. 8. 13:50:34> */

        if(p->structure_state == HE5Parser::GRID) {
            p->grid_list.back().data_var_list.back().dim_list.push_back(d);
        }


        if(p->structure_state == HE5Parser::SWATH) {
            if(swath_is_geo_field == true)
                p->swath_list.back().geo_var_list.back().dim_list.push_back(d);
            else
                p->swath_list.back().data_var_list.back().dim_list.push_back(d);
        }
               
        if(p->structure_state == HE5Parser::ZA) {
            p->za_list.back().data_var_list.back().dim_list.push_back(d);
        }
    }
// UNCOMMENT OUT the block below to retrieve the maximum dimension list. ALSO NEED TO ADD MAX_DIMENSION_LIST at  he5dds.lex.
/*
    else if(p->parser_state == 12){ // THis is parsing the MaxDimList. 
        string a;
        a = a.append($$);
        HE5Dim d;
        d.name = a;
        d.size = 0;             

        if(p->structure_state == HE5Parser::GRID) {
            p->grid_list.back().data_var_list.back().max_dim_list.push_back(d);
        }


        if(p->structure_state == HE5Parser::SWATH) {
            if(swath_is_geo_field == true)
                p->swath_list.back().geo_var_list.back().max_dim_list.push_back(d);
            else
                p->swath_list.back().data_var_list.back().max_dim_list.push_back(d);
        }

        if(p->structure_state == HE5Parser::ZA) {
            p->za_list.back().data_var_list.back().max_dim_list.push_back(d);
        }
    }
*/
}
| FLOAT
{
#ifdef VERBOSE
    cout << "data: " << $1 
         << "parser_state: " 
         << ((HE5Parser*)(he5parser))->parser_state 
         << endl;	   
#endif	   
    switch(((HE5Parser*)(he5parser))->parser_state){
    case 0:
        ((HE5Parser*)(he5parser))->parser_state = 1;
        ((HE5Parser*)(he5parser))->point_left = atof($1);
        break;
    case 1:
        ((HE5Parser*)(he5parser))->parser_state = 2;
        ((HE5Parser*)(he5parser))->point_upper = atof($1);
        break;
    case 2:
        ((HE5Parser*)(he5parser))->parser_state = 3;
        ((HE5Parser*)(he5parser))->point_right = atof($1);
        break;
    case 3:
        ((HE5Parser*)(he5parser))->parser_state = 4;
        ((HE5Parser*)(he5parser))->point_lower = atof($1);
        break;
    default:
        break;
    };
}

| STR
{	
    HE5Parser* p = (HE5Parser*)he5parser;
    if(p->parser_state == 10){
        string a;
        a = a.append($$);
        HE5Dim d;
        d.name = a;
        d.size = 0;             /* <hyokyung 2011.12. 8. 13:50:34> */

        if(p->structure_state == HE5Parser::GRID) {
            p->grid_list.back().data_var_list.back().dim_list.push_back(d);
        }


        if(p->structure_state == HE5Parser::SWATH) {
            if(swath_is_geo_field == true)
                p->swath_list.back().geo_var_list.back().dim_list.push_back(d);
            else
                p->swath_list.back().data_var_list.back().dim_list.push_back(d);
        }
               
        if(p->structure_state == HE5Parser::ZA) {
            p->za_list.back().data_var_list.back().dim_list.push_back(d);
        }
    }

// UNCOMMENT OUT the block below to retrieve the maximum dimension list. ALSO NEED TO ADD MAX_DIMENSION_LIST at  he5dds.lex.
/*
    else if(p->parser_state == 12){
        string a;
        a = a.append($$);
        HE5Dim d;
        d.name = a;
        d.size = 0;             

        if(p->structure_state == HE5Parser::GRID) {
            p->grid_list.back().data_var_list.back().max_dim_list.push_back(d);
        }


        if(p->structure_state == HE5Parser::SWATH) {
            if(swath_is_geo_field == true)
                p->swath_list.back().geo_var_list.back().max_dim_list.push_back(d);
            else
                p->swath_list.back().data_var_list.back().max_dim_list.push_back(d);
        }
               
        if(p->structure_state == HE5Parser::ZA) {
            p->za_list.back().data_var_list.back().max_dim_list.push_back(d);
        }
    }
*/

}

;

attribute: attribute_grid_name
| attribute_xdim
| attribute_ydim
| attribute_dimension_name
| attribute_dimension_size
| attribute_dimension_list

// UNCOMMENT OUT the line below to retrieve the maximum dimension list. ALSO NEED TO ADD MAX_DIMENSION_LIST at  he5dds.lex.
//| attribute_max_dimension_list
| attribute_data_field_name
| attribute_geo_field_name
| attribute_upperleft
| attribute_lowerright
| attribute_pixelregistration
| attribute_gridorigin
| attribute_spherecode
| attribute_zonecode
| projection
| DATA_TYPE 
           // <hyokyung 2011.09.13. 13:22:18> 
| STR '=' dataseq 
{
#ifdef VEROBSE
    cout << "Rule STR '=' dataseq:" << $2 << endl;    
#endif
}
           // <hyokyung 2011.09.29. 11:15:51> 
| COMPRESSION_TYPE '=' STR 
{
#ifdef VERBOSE  
    cout << "Rule COMPRESSION_TYPE '=' STR:" << $2 << endl;
#endif

}
| attribute_swath_name
| attribute_za_name
;

attribute_grid_name: GRID_NAME '=' STR
{
    HE5Parser* p = (HE5Parser*)he5parser;
    HE5Grid g;
    // Initialize the Grid structure.
    g.point_lower = -999.0;
    g.point_upper = -999.0;
    g.point_left = -999.0;
    g.point_right = -999.0;
    g.pixelregistration = HE5_HDFE_MISSING;
    g.gridorigin = HE5_HDFE_GD_MISSING;
    g.projection = HE5_GCTP_MISSING;
    g.zone = -1;
    g.sphere = -1;
    for (int i = 0; i <13;i++)
        g.param[i] = 0.0;

#ifdef VERBOSE  
    cout << "Grid Name is:" << $3 << endl;
#endif

    // Save the Grid name.
    g.name = $3;
    p->grid_list.push_back(g);  
    p->structure_state = HE5Parser::GRID;
}
;

attribute_swath_name:  SWATH_NAME '=' STR 
{
    HE5Parser* p = (HE5Parser*)he5parser;
    HE5Swath s;

#ifdef VERBOSE
    cout << "Swath Name is:[" << $3 << "]" << endl;
 #endif
    string str = string($3);
    // Save the Swath name. 
    s.name = str.substr(0, str.find("\""));
    // cout << "New name is " << s.name <<  endl;
    p->swath_list.push_back(s);  
    p->structure_state = HE5Parser::SWATH;	
}
| SWATH_NAME '=' STR '(' STR ')'
{
    HE5Parser* p = (HE5Parser*)he5parser;
    HE5Swath s;

#ifdef VERBOSE
    cout << "Swath Name has parentheses:[" << $3 << "]" << endl;
#endif

    // Save the Swath name. 
    s.name = string($3);
    p->swath_list.push_back(s);  
    p->structure_state = HE5Parser::SWATH;
}
;

attribute_za_name: ZA_NAME '=' STR
{
    HE5Parser* p = (HE5Parser*)he5parser;
    HE5Za z;

#ifdef VERBOSE
    cout << "Zonal Average Name is:" << $3 << endl;
#endif

    // Save the ZA name. 
    z.name = $3;
    p->za_list.push_back(z);  
    p->structure_state = HE5Parser::ZA;
}
;


attribute_dimension_name: DIMENSION_NAME '=' STR 
{
    // Save the dimension name. 
    dimension_name = $3;
#ifdef VERBOSE
    cout << " Dimension name: " << dimension_name << endl;  
#endif
}
| DIMENSION_NAME '=' INT
{
    // Save the dimension name. 
    dimension_name = $3;
#ifdef VERBOSE
    cout << " Dimension name: " << dimension_name << endl;  
#endif
}
;

attribute_dimension_size: DIMENSION_SIZE '=' INT
{
    HE5Parser* p = (HE5Parser*)he5parser;
    HE5Dim d;
    d.name = dimension_name;
    d.size = atoi($3);

    // Save the dimension name and its size. 
    if(p->structure_state == HE5Parser::GRID){  
        p->grid_list.back().dim_list.push_back(d);
    }
    if(p->structure_state == HE5Parser::SWATH){  
        p->swath_list.back().dim_list.push_back(d);  
    }
    if(p->structure_state == HE5Parser::ZA){  
        p->za_list.back().dim_list.push_back(d);  
    }

}
;

attribute_dimension_list: DIMENSION_LIST 
{
    ((HE5Parser*)(he5parser))->parser_state = 10;
}
'=' dataseq
{
    ((HE5Parser*)(he5parser))->parser_state = 11;
}
;
// UNCOMMENT OUT the lines below to retrieve the maximum dimension list. ALSO NEED TO ADD MAX_DIMENSION_LIST at  he5dds.lex.
/*
attribute_max_dimension_list: MAX_DIMENSION_LIST
{
    ((HE5Parser*)(he5parser))->parser_state = 12;
} 
'=' dataseq
{
    ((HE5Parser*)(he5parser))->parser_state = 13;
}
;
*/


attribute_data_field_name: DATA_FIELD_NAME '=' STR
{
#ifdef VERBOSE
    cout << $3 << endl;
#endif
    HE5Parser* p = (HE5Parser*)he5parser;
    HE5Var v;
    // Save the data field name. 
    v.name = $3;

    // Push the variable into list.
    switch(p->structure_state){

    case HE5Parser::GRID:
      p->grid_list.back().data_var_list.push_back(v);  
      break;

    case HE5Parser::SWATH:
      p->swath_list.back().data_var_list.push_back(v);  
      swath_is_geo_field = false;
      break;

    case HE5Parser::ZA:
      p->za_list.back().data_var_list.push_back(v);  
      break;

    default:
      p->err_msg = "Unexpected parser structure state.";
      YYERROR;
      break;
    }
    

}
| DATA_FIELD_NAME '=' PROJECTION
{
#ifdef VERBOSE
    cout << "attribute_data_field_name, projection: " << $3 << endl;
#endif
    HE5Parser* p = (HE5Parser*)he5parser;
    HE5Var v;
    // Save the data field name.
    v.name = $3;

    // Push the variable into list.
    switch(p->structure_state){

    case HE5Parser::GRID:
      p->grid_list.back().data_var_list.push_back(v);
      break;

    case HE5Parser::SWATH:
      p->swath_list.back().data_var_list.push_back(v);
      swath_is_geo_field = false;
      break;

    case HE5Parser::ZA:
      p->za_list.back().data_var_list.push_back(v);
      break;

    default:
      p->err_msg = "Unexpected parser structure state.";
      YYERROR;
      break;
    }
} 
;


attribute_geo_field_name: GEO_FIELD_NAME '=' STR
{
  
  HE5Parser* p = (HE5Parser*)he5parser;
  if(p->structure_state == HE5Parser::SWATH){
    HE5Var v;
    v.name = $3;
    p->swath_list.back().geo_var_list.push_back(v);  
    swath_is_geo_field = true;
  }
  else{
     p->err_msg = "Geo field variable is defined on non-Swath structure:" + string($3);
     YYERROR;

    
  }
}
;

// Some StructMetdata define XDim and YDim outside "GROUP=Dimension" block.
attribute_xdim: XDIM INT
{
  // Remember the X Dimension
#ifdef VERBOSE  
  cout << "XDim is:" << atoi($2) << endl;
#endif
    HE5Dim d;
    HE5Parser* p = (HE5Parser*)he5parser;

    d.name = "XDim";
    d.size = atoi($2);
    // Save the dimension name and its size. 
    if(p->structure_state == HE5Parser::GRID){  
        p->grid_list.back().dim_list.push_back(d);
    }

    // Throw an error instead. 
    // Although we haven't seen XDim and YDim defined outside the 
    // "GROUP=Dimension" block in Swath and ZA cases, we handle the following
    // cases for robustness. 
    if(p->structure_state == HE5Parser::SWATH){  
        p->swath_list.back().dim_list.push_back(d);  
    }

    if(p->structure_state == HE5Parser::ZA){  
        p->za_list.back().dim_list.push_back(d);  
    }

}
;

attribute_ydim: YDIM INT
{
  // Remember the Y Dimension
#ifdef VERBOSE  
  cout << "YDim is:" << atoi($2) << endl;
#endif
  HE5Dim d;
  HE5Parser* p = (HE5Parser*)he5parser;

  d.name = "YDim";
  d.size = atoi($2);
  // Save the dimension name and its size. 
  if(p->structure_state == HE5Parser::GRID){  
      p->grid_list.back().dim_list.push_back(d);
  }
  if(p->structure_state == HE5Parser::SWATH){  
      p->swath_list.back().dim_list.push_back(d);  
  }
  if(p->structure_state == HE5Parser::ZA){  
      p->za_list.back().dim_list.push_back(d);  
  }
}
;



group: GROUP '=' STR
{
#ifdef VERBOSE	
    cout << "GROUP=" << $3 <<  endl;
#endif	
}
attribute_list
END_GROUP '=' STR;



object:OBJECT '=' STR
{
#ifdef VERBOSE	
    cout <<  $3  <<  endl;
#endif	 
}
attribute_list
END_OBJECT '=' STR;
		
projection: PROJECTION '=' STR
{
    HE5Parser* p = (HE5Parser*)he5parser;
#ifdef VERBOSE  
    cout<< "Got projection " << $3 << endl;
#endif  
    HE5Grid *g = &p->grid_list.back();
    if(strncmp("HE5_GCTP_GEO", $3, 12)==0)
        g->projection = HE5_GCTP_GEO;
    else if(strncmp("HE5_GCTP_UTM", $3, 12)==0)
        g->projection = HE5_GCTP_UTM;
    else if(strncmp("HE5_GCTP_SPCS", $3, 13)==0)
        g->projection = HE5_GCTP_SPCS;
    else if(strncmp("HE5_GCTP_ALBERS", $3, 15)==0)
        g->projection = HE5_GCTP_ALBERS;
    else if(strncmp("HE5_GCTP_LAMCC", $3, 14)==0)
        g->projection = HE5_GCTP_LAMCC;
    else if(strncmp("HE5_GCTP_MERCAT", $3, 15)==0)
        g->projection = HE5_GCTP_MERCAT;
    else if(strncmp("HE5_GCTP_PS", $3, 11)==0)
        g->projection = HE5_GCTP_PS;
    else if(strncmp("HE5_GCTP_POLYC", $3, 14)==0)
        g->projection = HE5_GCTP_POLYC;
    else if(strncmp("HE5_GCTP_EQUIDC", $3, 15)==0)
        g->projection = HE5_GCTP_EQUIDC;
    else if(strncmp("HE5_GCTP_TM", $3, 11)==0)
        g->projection = HE5_GCTP_TM;
    else if(strncmp("HE5_GCTP_STEREO", $3, 15)==0)
        g->projection = HE5_GCTP_STEREO;
    else if(strncmp("HE5_GCTP_LAMAZ", $3, 14)==0)
        g->projection = HE5_GCTP_LAMAZ;
    else if(strncmp("HE5_GCTP_AZMEQD", $3, 15)==0)
        g->projection = HE5_GCTP_AZMEQD;
    else if(strncmp("HE5_GCTP_GNOMON", $3, 15)==0)
        g->projection = HE5_GCTP_GNOMON;
    else if(strncmp("HE5_GCTP_ORTHO", $3, 14)==0)
        g->projection = HE5_GCTP_ORTHO;
    else if(strncmp("HE5_GCTP_GVNSP", $3, 14)==0)
        g->projection = HE5_GCTP_GVNSP;
    else if(strncmp("HE5_GCTP_SNSOID", $3, 15)==0)
        g->projection = HE5_GCTP_SNSOID;
    else if(strncmp("HE5_GCTP_EQRECT", $3, 15)==0)
        g->projection = HE5_GCTP_EQRECT;
    else if(strncmp("HE5_GCTP_MILLER", $3, 15)==0)
        g->projection = HE5_GCTP_MILLER;
    else if(strncmp("HE5_GCTP_VGRINT", $3, 15)==0)
        g->projection = HE5_GCTP_VGRINT;
    else if(strncmp("HE5_GCTP_HOM", $3, 12)==0)
        g->projection = HE5_GCTP_HOM;
    else if(strncmp("HE5_GCTP_ROBIN", $3, 14)==0)
        g->projection = HE5_GCTP_ROBIN;
    else if(strncmp("HE5_GCTP_SOM", $3, 12)==0)
        g->projection = HE5_GCTP_SOM;
    else if(strncmp("HE5_GCTP_ALASKA", $3, 15)==0)
        g->projection = HE5_GCTP_ALASKA;
    else if(strncmp("HE5_GCTP_GOOD", $3, 13)==0)
        g->projection = HE5_GCTP_GOOD;
    else if(strncmp("HE5_GCTP_MOLL", $3, 13)==0)
        g->projection = HE5_GCTP_MOLL;
    else if(strncmp("HE5_GCTP_IMOLL", $3, 14)==0)
        g->projection = HE5_GCTP_IMOLL;
    else if(strncmp("HE5_GCTP_HAMMER", $3, 15)==0)
        g->projection = HE5_GCTP_HAMMER;
    else if(strncmp("HE5_GCTP_WAGIV", $3, 14)==0)
        g->projection = HE5_GCTP_WAGIV;
    else if(strncmp("HE5_GCTP_WAGVII", $3, 15)==0)
        g->projection = HE5_GCTP_WAGVII;
    else if(strncmp("HE5_GCTP_OBLEQA", $3, 15)==0)
        g->projection = HE5_GCTP_OBLEQA;
    else if(strncmp("HE5_GCTP_CEA", $3, 12)==0)
        g->projection = HE5_GCTP_CEA;
    else if(strncmp("HE5_GCTP_BCEA", $3, 13)==0)
        g->projection = HE5_GCTP_BCEA;
    else if(strncmp("HE5_GCTP_ISINUS", $3, 15)==0)
        g->projection = HE5_GCTP_ISINUS;
    else{
        p->err_msg = std::string("") + 
            "An unknown projection is detected in StructMetadata." +
            "The projection code is " + $3 + ".";
        g->projection = HE5_GCTP_UNKNOWN;
        //YYERROR;
    }
}
;

attribute_pixelregistration: PIXELREGISTRATION STR
{ 

    //   We use enum for pixel registration in HE5Grid.h, which starts from 0.
    //   Thus, the default value should be negative.
    EOS5GridPRType value = HE5_HDFE_UNKNOWN;
    HE5Parser* p = (HE5Parser*)he5parser;
    HE5Grid *g = &p->grid_list.back();

    if(strncmp("HE5_HDFE_CENTER", $2, 15)==0)
        value = HE5_HDFE_CENTER;
    else if(strncmp("HE5_HDFE_CORNER", $2, 15)==0)
        value = HE5_HDFE_CORNER;
    else
	{
            ((HE5Parser*)(he5parser))->err_msg = 
                "Wrong PixelRegistration Value";
            //YYERROR;
	}
    g->pixelregistration = value;
}
;

attribute_gridorigin: GRIDORIGIN STR
{ 
    //  We use enum for origin in HE5Grid.h, which starts from 0.
    //  Thus, the default value should be negative.
    EOS5GridOriginType value = HE5_HDFE_GD_UNKNOWN;
    HE5Parser* p = (HE5Parser*)he5parser;
    HE5Grid *g = &p->grid_list.back();
    if(strncmp("HE5_HDFE_GD_UL", $2, 14)==0)
        value = HE5_HDFE_GD_UL;
    else if(strncmp("HE5_HDFE_GD_UR", $2, 14)==0)
        value = HE5_HDFE_GD_UR;
    else if(strncmp("HE5_HDFE_GD_LL", $2, 14)==0)
        value = HE5_HDFE_GD_LL;
    else if(strncmp("HE5_HDFE_GD_LR", $2, 14)==0)
        value = HE5_HDFE_GD_LR;
    else
	{
            // Throw an error for unknown origin value.
            p->err_msg = 
                "Wrong Grid Origin Value:" + string($2);
            //YYERROR;
	}
    g->gridorigin = value;

}
;

attribute_spherecode: SPHERECODE INT
{
#ifdef VERBOSE
    cout<<"Sphere code is: "<<atoi($2)<<endl;
#endif

    HE5Parser* p = (HE5Parser*)he5parser;
    if(p->structure_state == HE5Parser::GRID) {
        HE5Grid *g = &p->grid_list.back();
        g->sphere = atoi($2);
    }

}
;

attribute_zonecode: ZONECODE INT
{
#ifdef VERBOSE
    cout<<"Zone code is: "<<atoi($2)<<endl;
#endif

    HE5Parser* p = (HE5Parser*)he5parser;
    HE5Grid *g = &p->grid_list.back();

    g->zone = atoi($2);

}
;


attribute_upperleft: UPPERLEFTPT '(' FLOAT ',' FLOAT ')'
{
#ifdef VERBOSE
    fprintf(stdout, "## %s %f %f\n", $1, atof($3), atof($5));
#endif

    HE5Parser* p = (HE5Parser*)he5parser;
    HE5Grid *g = &p->grid_list.back();

    g->point_left = atof($3);
    g->point_upper = atof($5);


}
| UPPERLEFTPT DEFAULT
{
#ifdef VERBOSE
    fprintf(stderr, "## %s %s\n", $1, $2);
#endif
}
;


attribute_lowerright: LOWERRIGHTPT '(' FLOAT ',' FLOAT ')'
{
#ifdef VERBOSE
    fprintf(stdout, "## %s %f %f\n", $1, atof($3), atof($5));
#endif

    HE5Parser* p = (HE5Parser*)he5parser;
    HE5Grid *g = &p->grid_list.back();

    g->point_right = atof($3);
    g->point_lower = atof($5);
}
| LOWERRIGHTPT DEFAULT
{
}
;

%%


//  This function is required for linking, but DODS uses its own error
// reporting mechanism.
void
he5ddserror(HE5Parser *, char *s)
{
     cerr<< "he5dds.y ERROR: " << s << endl;
}

