%{
#define YYSTYPE char *
#define YYDEBUG 1
// #define VERBOSE // Comment this out for debugging.


#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <assert.h>

#include <vector>
#include <map>
#include <sstream>

using namespace std;

#include "he5dds.tab.hh"
#include "HE5Parser.h"
 
#define YYPARSE_PARAM he5parser


extern int yy_line_num;	// It's defined in he5dds.lex 


string full_path = "";
string grid_name = "";
string swath_name = "";
string data_field_name = "/Data Fields/";
string geo_field_name = "/Geolocation Fields/"; // for Swath only
string dimension_list = "";
string dimension_name = "";
 

#define TYPE_NAME_VALUE(x) type << " " << name << " " << (x)

void mem_list_report();
int  he5ddslex(void);
void he5ddserror(char *s);


%}


%token GROUP
%token END_GROUP
%token OBJECT
%token END_OBJECT

%token INT
%token FLOAT
%token STR
%token PROJECTION
%token HE5_GCTP_GEO
%token DATA_TYPE 
%token GRID_STRUCTURE
%token SWATH_STRUCTURE 
%token GRID_NAME  
%token SWATH_NAME 
%token DIMENSION_SIZE  
%token DIMENSION_NAME  
%token DIMENSION_LIST
%token DATA_FIELD_NAME
%token GEO_FIELD_NAME		// for swath
%token XDIM
%token YDIM
%%
attribute_list: /* empty */
          | attribute_list object
          | attribute_list attribute
	  | attribute_list group;
dataseq:
	| data
        | '(' 
	dataseq1 
	')' 
	;
dataseq1: data
	| dataseq1 ',' data
	;
data: /* empty */ 
      | INT
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

	   if(((HE5Parser*)(he5parser))->parser_state == 10){
	        if(dimension_list == ""){
	        	dimension_list = dimension_list.append($$);
                }
                else{
	        	dimension_list = dimension_list.append(",");
	        	dimension_list = dimension_list.append($$);
                }
           }


    	  }

;

attribute: attribute_grid_name
        | attribute_xdim
        | attribute_ydim
	| attribute_dimension_name
	| attribute_dimension_size
 	| attribute_dimension_list
	| attribute_data_field_name
	| attribute_geo_field_name
        | projection
	| DATA_TYPE 
	| STR '=' dataseq
	| attribute_swath_name
;
attribute_grid_name: GRID_NAME '=' STR
{
  // Remember the path.
  grid_name = $3;
  
  // Reset the full path
  full_path = "/HDFEOS/GRIDS/";
  ((HE5Parser*)(he5parser))->valid_projection = false;
  full_path.append(grid_name);
#ifdef VERBOSE  
  cout << "Grid Name is:" << grid_name << endl;
#endif
  
}
;

attribute_xdim: XDIM INT
{
  // Remember the X Dimension
#ifdef VERBOSE  
  cout << "XDim is:" << atoi($2) << endl;
  cout << "Full path is:" << full_path << endl;
#endif
  if(((HE5Parser*)(he5parser))->grid_structure_found)    
    ((HE5Parser*)(he5parser))->set_grid_dimension_size(full_path+"/XDim", atoi($2));
}
;

attribute_ydim: YDIM INT
{
  // Remember the Y Dimension
#ifdef VERBOSE  
  cout << "YDim is:" << atoi($2) << endl;
#endif
  // Reset the parser state
  ((HE5Parser*)(he5parser))->parser_state = 0;
  if(((HE5Parser*)(he5parser))->grid_structure_found)    
    ((HE5Parser*)(he5parser))->set_grid_dimension_size(full_path+"/YDim", atoi($2));
}
;


attribute_dimension_name: DIMENSION_NAME '=' STR 
{
#ifdef VERBOSE
  cout << "Full path: " << full_path;
#endif
  // Save the dimension name.  
  dimension_name = full_path + "/" + $3;
#ifdef VERBOSE
  cout << " Dimension name: " << dimension_name << endl;  
#endif
}
| DIMENSION_NAME '=' INT
{
#ifdef VERBOSE
  cout << "Full path: " << full_path;
#endif
  // Save the dimension name.  
  dimension_name = full_path + "/" + $3;
#ifdef VERBOSE
  cout << " Dimension name: " << dimension_name << endl;  
#endif
}
;
attribute_dimension_size: DIMENSION_SIZE '=' INT
{
  // Save the size info.
  if(((HE5Parser*)(he5parser))->grid_structure_found)  
    ((HE5Parser*)(he5parser))->set_grid_dimension_size(dimension_name, 
                                                       atoi($3));
  if(((HE5Parser*)(he5parser))->swath_structure_found)
    ((HE5Parser*)(he5parser))->set_swath_dimension_size(dimension_name, 
                                                        atoi($3));
}
;
attribute_dimension_list: DIMENSION_LIST 
{
   ((HE5Parser*)(he5parser))->parser_state = 10;
}
'=' dataseq
{
  ((HE5Parser*)(he5parser))->parser_state = 11;
  if(((HE5Parser*)(he5parser))->grid_structure_found){
    ((HE5Parser*)(he5parser))->set_grid_variable_dimensions(full_path, 
                                                            dimension_list);
    // Reset for next path
    data_field_name = "/Data Fields/";
    full_path = "/HDFEOS/GRIDS/";
    full_path.append(grid_name);
    dimension_list = "";
  }
  if(((HE5Parser*)(he5parser))->swath_structure_found){
    ((HE5Parser*)(he5parser))->set_swath_variable_dimensions(full_path,
                                                             dimension_list);
    // Reset for next path
    data_field_name = "/Data Fields/";
    geo_field_name = "/Geolocation Fields/";
    full_path = "/HDFEOS/SWATHS/";
    full_path.append(swath_name);
    dimension_list = "";
  }  
}
;
attribute_data_field_name: DATA_FIELD_NAME '=' STR
{
#ifdef VERBOSE
  cout << $3 << endl;
#endif
  // Construct the path.	
  if(((HE5Parser*)(he5parser))->valid_projection){
    data_field_name.append($3);
    full_path.append(data_field_name);
    ((HE5Parser*)(he5parser))->set_grid_variable_list(full_path);
#ifdef VERBOSE    
    cout << "set_grid_variable_list:" << full_path << endl;
#endif    
  }
  if(((HE5Parser*)(he5parser))->swath_structure_found){
    data_field_name.append($3);
    full_path.append(data_field_name);
    ((HE5Parser*)(he5parser))->set_swath_variable_list(full_path);
#ifdef VERBOSE    
    cout << "set_swath_variable_list:" << full_path << endl;
#endif        
  }
}
;

attribute_geo_field_name: GEO_FIELD_NAME '=' STR
{
  if(((HE5Parser*)(he5parser))->swath_structure_found){
    geo_field_name.append($3);
    full_path.append(geo_field_name);
    ((HE5Parser*)(he5parser))->set_swath_variable_list(full_path);
#ifdef VERBOSE    
    cout << "add_geo_path_swath:" << full_path << endl;
#endif        
  }
}
;


attribute_swath_name: SWATH_NAME '=' STR
{
  // Remember the path.
  swath_name = $3;
  ((HE5Parser*)(he5parser))->swath_structure_found = true;
  ((HE5Parser*)(he5parser))->set_swath(true);  
  // Reset the full path
  full_path = "/HDFEOS/SWATHS/";
  ((HE5Parser*)(he5parser))->valid_projection = false;
  full_path.append(swath_name);
#ifdef VERBOSE  
  cout << "Swath Name is:" << swath_name << endl;
#endif
  
}
;


group:GROUP '=' STR
      {
#ifdef VERBOSE	
	cout << "GROUP=" << $3 <<  endl;
#endif	
      }
      attribute_list
      END_GROUP '=' STR 
|
      GROUP '=' GRID_STRUCTURE
      {
	((HE5Parser*)(he5parser))->grid_structure_found = true;	
	((HE5Parser*)(he5parser))->swath_structure_found = false;	
#ifdef VERBOSE
	cout << $3 <<  endl; // $3 refers the STR
#endif	

      }
      attribute_list
      END_GROUP '=' GRID_STRUCTURE
|
      GROUP '=' SWATH_STRUCTURE
      {
	((HE5Parser*)(he5parser))->grid_structure_found = false;
      }
      attribute_list
      END_GROUP '=' SWATH_STRUCTURE

;

object:OBJECT '=' STR
      {
#ifdef VERBOSE	
	 cout <<  $3  <<  endl;
#endif	 
      }
       attribute_list
       END_OBJECT '=' STR;
		
projection: PROJECTION '=' HE5_GCTP_GEO
{
  // Set ((HE5Parser*)(he5parser))->valid_projection flag to "true".
  ((HE5Parser*)(he5parser))->valid_projection = true;
#ifdef VERBOSE  
  cout << "Got projection " << endl;
#endif  
}
|
PROJECTION '=' STR
{
#ifdef VERBOSE  
  cerr << "Got wrong projection " << endl;
#endif  
}
;



%%

// This function is required for linking, but DODS uses its own error
// reporting mechanism.

void
he5ddserror(char *s)
{
	cout << "he5dds.y ERROR: " << s << endl;
}

