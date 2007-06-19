/* A Bison parser, made by GNU Bison 1.875c.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     GROUP = 258,
     END_GROUP = 259,
     OBJECT = 260,
     END_OBJECT = 261,
     INT = 262,
     FLOAT = 263,
     STR = 264,
     PROJECTION = 265,
     HE5_GCTP_GEO = 266,
     DATA_TYPE = 267,
     GRID_STRUCTURE = 268,
     GRID_NAME = 269,
     DIMENSION_SIZE = 270,
     DIMENSION_NAME = 271,
     DIMENSION_LIST = 272,
     DATA_FIELD_NAME = 273,
     XDIM = 274,
     YDIM = 275     
   };
#endif
#define GROUP 258
#define END_GROUP 259
#define OBJECT 260
#define END_OBJECT 261
#define INT 262
#define FLOAT 263
#define STR 264
#define PROJECTION 265
#define HE5_GCTP_GEO 266
#define DATA_TYPE 267
#define GRID_STRUCTURE 268
#define GRID_NAME 269
#define DIMENSION_SIZE 270
#define DIMENSION_NAME 271
#define DIMENSION_LIST 272
#define DATA_FIELD_NAME 273
#define XDIM 274
#define YDIM 275


#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE hdfeoslval;



