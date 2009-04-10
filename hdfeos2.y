%{

#define YYSTYPE char *
#define YYDEBUG 1
// #define VERBOSE
  
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <assert.h>

#include <vector>
#include <map>
#include <sstream>

using namespace std;

// #include "hdfeos2.tab.hh"
#include "HDFEOS.h"
 
#define YYPARSE_PARAM arg

extern int yy_line_num;	// defined in hdfeos.lex 

static int parser_state = 0; // parser state 
static bool valid_projection = false;
static bool grid_structure_found = false; 
string full_path = "/HDFEOS/GRIDS/";
string grid_name = "";
string data_field_name = "/Data Fields/";
string dimension_list = "";
string dimension_name = "";
 

#define TYPE_NAME_VALUE(x) type << " " << name << " " << (x)

void mem_list_report();
int  hdfeos2lex(void);
void hdfeos2error(char *s);


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
%token GRID_NAME  
%token DIMENSION_SIZE  
%token DIMENSION_NAME  
%token DIMENSION_LIST
%token DATA_FIELD_NAME
%token XDIM
%token YDIM
%token GRID_ORIGIN
%token HDFE_GD_UL
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
	   cout << "data: " << $1 << "parser_state: " << parser_state << endl;	   
#endif	   
	    switch(parser_state){
	     case 0:
		parser_state = 1;
	        ((HDFEOS*)(arg))->point_left = atof($1);
        	break;
             case 1:
	        parser_state = 2;
	        ((HDFEOS*)(arg))->point_upper = atof($1);
                break;
             case 2:
	        parser_state = 3;
	        ((HDFEOS*)(arg))->point_right = atof($1);
                break;
             case 3:
	        parser_state = 4;
	        ((HDFEOS*)(arg))->point_lower = atof($1);
                break;
	     default:
                break;
            };
    	  }

      | STR
       	 {	

	   if(parser_state == 10){
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
        | projection
        | origin
	| DATA_TYPE 
	| STR '=' dataseq
;
attribute_grid_name: GRID_NAME '=' STR
{
  // Remember the path.
  grid_name = $3;
  
  // Reset the full path
  full_path = "/HDFEOS/GRIDS/";
  valid_projection = false;
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
  if(grid_structure_found)    
    ((HDFEOS*)(arg))->add_dimension_map(full_path+"/XDim", atoi($2));
}
;

attribute_ydim: YDIM INT
{
  // Remember the Y Dimension
#ifdef VERBOSE  
  cout << "YDim is:" << atoi($2) << endl;
#endif
  // Reset the parser state
  parser_state = 0;
  if(grid_structure_found)    
    ((HDFEOS*)(arg))->add_dimension_map(full_path+"/YDim", atoi($2));
}
;


attribute_dimension_name: DIMENSION_NAME '=' STR
{
  // cout << "Full path: " << full_path;
  // Save the dimension name.  
  dimension_name = full_path + "/" + $3;
  // cout << " Dimension name: " << dimension_name << endl;  
}
;
attribute_dimension_size: DIMENSION_SIZE '=' INT
{
  // Save the size info.
  if(grid_structure_found)  
    ((HDFEOS*)(arg))->add_dimension_map(dimension_name, atoi($3));
}
;
attribute_dimension_list: DIMENSION_LIST 
{
   parser_state = 10;
}
'=' dataseq
{
  parser_state = 11;
  if(grid_structure_found)
    ((HDFEOS*)(arg))->add_dimension_list(full_path, dimension_list);
  
  // Reset for next path
  data_field_name = "/Data Fields/";
  full_path = "/HDFEOS/GRIDS/";
  full_path.append(grid_name);
  dimension_list = "";
}
;
attribute_data_field_name: DATA_FIELD_NAME '=' STR
{
  // cout << $3 << endl;
  // Construct the path.	
  if(valid_projection){
    data_field_name.append($3);
    full_path.append(data_field_name);
    ((HDFEOS*)(arg))->add_data_path(full_path);
#ifdef VERBOSE    
    cout << "add_data_path:" << full_path << endl;
#endif    
  }
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
	grid_structure_found = true;	
#ifdef VERBOSE
	cout << $3 <<  endl; // $3 refers the STR
#endif	

      }
      attribute_list
      END_GROUP '=' GRID_STRUCTURE
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
  // Set valid_projection flag to "true".
  valid_projection = true;
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


origin: GRID_ORIGIN '=' HDFE_GD_UL
{
   ((HDFEOS*)(arg))->borigin_upper = true;
  // cerr << "Got origin" << endl;  
}
;

%%

// This function is required for linking, but DODS uses its own error
// reporting mechanism.

void
hdfeos2error(char *s)
{
	cout << "ERROR: " << s << endl;
}

