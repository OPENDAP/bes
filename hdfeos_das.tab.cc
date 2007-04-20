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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0

/* If NAME_PREFIX is specified substitute the variables and functions
   names.  */
#define yyparse hdfeos_dasparse
#define yylex   hdfeos_daslex
#define yyerror hdfeos_daserror
#define yylval  hdfeos_daslval
#define yychar  hdfeos_daschar
#define yydebug hdfeos_dasdebug
#define yynerrs hdfeos_dasnerrs


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
     END = 262,
     INT = 263,
     FLOAT = 264,
     STR = 265,
     COMMENT = 266,
     DATA_TYPE = 267
   };
#endif
#define GROUP 258
#define END_GROUP 259
#define OBJECT 260
#define END_OBJECT 261
#define END 262
#define INT 263
#define FLOAT 264
#define STR 265
#define COMMENT 266
#define DATA_TYPE 267




/* Copy the first part of user declarations.  */
#line 31 "hdfeos_das.y"


#define YYSTYPE char *
  
// static char rcsid[] not_used = {"$Id: hdfeos.y 13155 2006-02-09 20:41:24Z jimg $"};

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <assert.h>

#include <vector>
#include <sstream>

using namespace std;
#include "DAS.h"
#include "Error.h"
#include "debug.h"
#include "parser.h"
#include "hdfeos_das.tab.h"
#define YYDEBUG 1
#define TRACE_new

#ifdef TRACE_NEW
#include "trace_new.h"
#endif

// These macros are used to access the `arguments' passed to the parser. A
// pointer to an error object and a pointer to an integer status variable are
// passed in to the parser within a structure (which itself is passed as a
// pointer). Note that the ERROR macro explicitly casts OBJ to an ERROR *. 

#define ATTR_OBJ(arg) ((AttrTable *)((parser_arg *)(arg))->_object)
#define ERROR_OBJ(arg) ((parser_arg *)(arg))->_error
#define STATUS(arg) ((parser_arg *)(arg))->_status

#define YYPARSE_PARAM arg

extern int hdfeos_line_num;	/* defined in hdfeos.lex */

static string name;	/* holds name in attr_pair rule */
static string type;	/* holds type in attr_pair rule */
static string last_grid_swath;  /* holds HDF-EOS name for aliasing */
static int commentnum=0;   /* number of current comment */

static vector<AttrTable *> *attr_tab_stack;

// I use a vector of AttrTable pointers for a stack

#define TOP_OF_STACK (attr_tab_stack->back())
#define SECOND_IN_STACK ((*attr_tab_stack)[attr_tab_stack->size()-2])
#define PUSH(x) (attr_tab_stack->push_back(x))
#define POP (attr_tab_stack->pop_back())
#define STACK_LENGTH (attr_tab_stack->size())
#define STACK_EMPTY (attr_tab_stack->empty())

#define TYPE_NAME_VALUE(x) type << " " << name << " " << (x)

static char *NO_DAS_MSG =
"The attribute object returned from the dataset was null\n\
Check that the URL is correct.";

void mem_list_report();
int hdfeos_daslex(void);
void hdfeos_daserror(char *s);
static void process_group(parser_arg *arg, const string &s);



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 190 "hdfeos_das.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

# ifndef YYFREE
#  define YYFREE free
# endif
# ifndef YYMALLOC
#  define YYMALLOC malloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   define YYSTACK_ALLOC alloca
#  endif
# else
#  if defined (alloca) || defined (_ALLOCA_H)
#   define YYSTACK_ALLOC alloca
#  else
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   77

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  17
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  17
/* YYNRULES -- Number of rules. */
#define YYNRULES  34
/* YYNRULES -- Number of states. */
#define YYNSTATES  55

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   267

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      14,    15,     2,     2,    16,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    13,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     4,     7,    10,    11,    12,    22,    24,
      25,    26,    36,    37,    42,    44,    46,    47,    49,    52,
      54,    56,    58,    62,    64,    66,    68,    72,    74,    78,
      80,    84,    86,    88,    90
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      18,     0,    -1,    -1,    19,    20,    -1,    18,    20,    -1,
      -1,    -1,     3,    13,    10,    21,    26,    22,     4,    13,
      10,    -1,    12,    -1,    -1,    -1,     5,    13,    10,    23,
      26,    24,     6,    13,    10,    -1,    -1,    10,    25,    13,
      27,    -1,    11,    -1,     1,    -1,    -1,    20,    -1,    26,
      20,    -1,    29,    -1,    30,    -1,    33,    -1,    14,    28,
      15,    -1,    31,    -1,    33,    -1,     8,    -1,    29,    16,
       8,    -1,     9,    -1,    30,    16,     9,    -1,    32,    -1,
      31,    16,    32,    -1,     9,    -1,     8,    -1,    10,    -1,
      33,    16,    10,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   156,   156,   156,   161,   165,   169,   164,   175,   177,
     181,   176,   188,   187,   192,   209,   225,   226,   227,   230,
     231,   232,   233,   239,   240,   243,   267,   287,   304,   323,
     340,   359,   359,   362,   384
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "GROUP", "END_GROUP", "OBJECT",
  "END_OBJECT", "END", "INT", "FLOAT", "STR", "COMMENT", "DATA_TYPE",
  "'='", "'('", "')'", "','", "$accept", "attributes", "@1", "attribute",
  "@2", "@3", "@4", "@5", "@6", "attr_list", "data", "data2", "ints",
  "floats", "floatints", "float_or_int", "strs", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,    61,    40,    41,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    17,    19,    18,    18,    21,    22,    20,    20,    23,
      24,    20,    25,    20,    20,    20,    26,    26,    26,    27,
      27,    27,    27,    28,    28,    29,    29,    30,    30,    31,
      31,    32,    32,    33,    33
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     2,     2,     0,     0,     9,     1,     0,
       0,     9,     0,     4,     1,     1,     0,     1,     2,     1,
       1,     1,     3,     1,     1,     1,     3,     1,     3,     1,
       3,     1,     1,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       2,     0,     0,     1,    15,     0,     0,    12,    14,     8,
       4,     3,     0,     0,     0,     5,     9,     0,     0,     0,
      25,    27,    33,     0,    13,    19,    20,    21,    17,     0,
       0,    32,    31,     0,    23,    29,    24,     0,     0,     0,
      18,     0,     0,    22,     0,    26,    28,    34,     0,     0,
      30,     0,     0,     7,    11
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     1,     2,    28,    18,    41,    19,    42,    14,    29,
      24,    33,    25,    26,    34,    35,    27
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -10
static const yysigned_char yypact[] =
{
     -10,     2,    56,   -10,   -10,    -9,    -2,   -10,   -10,   -10,
     -10,   -10,    12,    17,    21,   -10,   -10,    10,     5,    20,
     -10,   -10,   -10,    30,   -10,    25,    37,    42,   -10,    32,
      44,   -10,   -10,    31,    46,   -10,    42,    40,    51,    53,
     -10,    60,    59,   -10,    43,   -10,   -10,   -10,    57,    58,
     -10,    62,    63,   -10,   -10
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -10,   -10,   -10,    -1,   -10,   -10,   -10,   -10,   -10,    50,
     -10,   -10,   -10,   -10,   -10,    33,    52
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -17
static const yysigned_char yytable[] =
{
      10,    11,     3,     4,    12,     5,     4,     6,     5,   -16,
       6,    13,     7,     8,     9,     7,     8,     9,    20,    21,
      22,     4,    15,     5,    23,     6,   -16,    16,    40,    40,
       7,     8,     9,     4,    17,     5,    -6,     6,    31,    32,
      22,    37,     7,     8,     9,     4,    43,     5,    45,     6,
     -10,    31,    32,    38,     7,     8,     9,     4,    39,     5,
      46,     6,    44,    47,    48,    49,     7,     8,     9,    30,
      51,    52,    53,    54,     0,    36,     0,    50
};

static const yysigned_char yycheck[] =
{
       1,     2,     0,     1,    13,     3,     1,     5,     3,     4,
       5,    13,    10,    11,    12,    10,    11,    12,     8,     9,
      10,     1,    10,     3,    14,     5,     6,    10,    29,    30,
      10,    11,    12,     1,    13,     3,     4,     5,     8,     9,
      10,    16,    10,    11,    12,     1,    15,     3,     8,     5,
       6,     8,     9,    16,    10,    11,    12,     1,    16,     3,
       9,     5,    16,    10,     4,     6,    10,    11,    12,    19,
      13,    13,    10,    10,    -1,    23,    -1,    44
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    18,    19,     0,     1,     3,     5,    10,    11,    12,
      20,    20,    13,    13,    25,    10,    10,    13,    21,    23,
       8,     9,    10,    14,    27,    29,    30,    33,    20,    26,
      26,     8,     9,    28,    31,    32,    33,    16,    16,    16,
      20,    22,    24,    15,    16,     8,     9,    10,     4,     6,
      32,    13,    13,    10,    10
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)		\
   ((Current).first_line   = (Rhs)[1].first_line,	\
    (Current).first_column = (Rhs)[1].first_column,	\
    (Current).last_line    = (Rhs)[N].last_line,	\
    (Current).last_column  = (Rhs)[N].last_column)
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if defined (YYMAXDEPTH) && YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 156 "hdfeos_das.y"
    {
		    if (!attr_tab_stack)
			attr_tab_stack = new vector<AttrTable *>;
		;}
    break;

  case 5:
#line 165 "hdfeos_das.y"
    {
		  process_group((parser_arg *)arg, yyvsp[0]);
		;}
    break;

  case 6:
#line 169 "hdfeos_das.y"
    {
		  /* pop top of stack; store in attr_tab */
		  DBG(cerr << " Popped attr_tab: " << TOP_OF_STACK << endl);
		  POP;
		;}
    break;

  case 9:
#line 177 "hdfeos_das.y"
    {
		    process_group((parser_arg *)arg, yyvsp[0]);
		;}
    break;

  case 10:
#line 181 "hdfeos_das.y"
    {
		  /* pop top of stack; store in attr_tab */
		  DBG(cerr << " Popped attr_tab: " << TOP_OF_STACK << endl);
		  POP;
		;}
    break;

  case 12:
#line 188 "hdfeos_das.y"
    { 
		    name = yyvsp[0]; 
		;}
    break;

  case 14:
#line 192 "hdfeos_das.y"
    {
		    ostringstream name, comment;
		    name << "comment" << commentnum++;
		    comment << "\"" << yyvsp[0] << "\"";
		    cerr << name.str() << ":" << comment.str() << endl;
		    AttrTable *a;
		    if (STACK_EMPTY)
		      a = ATTR_OBJ(arg);
		    else
		      a = TOP_OF_STACK;
		    if (!a->append_attr(name.str(), "String", comment.str())) {
		      ostringstream msg;
		      msg << "`" << name.str() << "' previously defined.";
		      parse_error((parser_arg *)arg, msg.str().c_str());
		      YYABORT;
		    }
		;}
    break;

  case 15:
#line 210 "hdfeos_das.y"
    {
		    AttrTable *a;
		    if (STACK_EMPTY)
			a = ATTR_OBJ(arg);
		    else
			a = TOP_OF_STACK;
		    a->append_attr(name.c_str(), "String", 
				   "\"Error processing EOS attributes\"");
		    parse_error((parser_arg *)arg, NO_DAS_MSG);
		    /* Don't abort; keep parsing to try and pick up more
		       attribtues. 3/30/2000 jhrg */
 		    /* YYABORT; */
		;}
    break;

  case 25:
#line 244 "hdfeos_das.y"
    {
		    /* NB: On the Sun (SunOS 4) strtol does not check for */
		    /* overflow. Thus it will never figure out that 4 */
		    /* billion is way to large to fit in a 32 bit signed */
		    /* integer. What's worse, long is 64  bits on Alpha and */
		    /* SGI/IRIX 6.1... jhrg 10/27/96 */
		    /* type = "Int32"; */
		    DBG(cerr << "Adding INT: " << TYPE_NAME_VALUE(yyvsp[0]) << endl);
		    DBG(cerr << " to AttrTable: " << TOP_OF_STACK << endl);
		    if (!(check_int32(yyvsp[0]) 
			  || check_uint32(yyvsp[0]))) {
			ostringstream msg;
			msg << "`" << yyvsp[0] << "' is not an Int32 value.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		    else if (!TOP_OF_STACK->append_attr(name, "Int32", yyvsp[0])) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		;}
    break;

  case 26:
#line 268 "hdfeos_das.y"
    {
		    type = "Int32";
		    DBG(cerr << "Adding INT: " << TYPE_NAME_VALUE(yyvsp[0]) << endl);
		    if (!(check_int32(yyvsp[0])
			  || check_uint32(yyvsp[-2]))) {
			ostringstream msg;
			msg << "`" << yyvsp[-2] << "' is not an Int32 value.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		    else if (!TOP_OF_STACK->append_attr(name, type, yyvsp[0])) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		;}
    break;

  case 27:
#line 288 "hdfeos_das.y"
    {
		    type = "Float64";
		    DBG(cerr << "Adding FLOAT: " << TYPE_NAME_VALUE(yyvsp[0]) << endl);
		    if (!check_float64(yyvsp[0])) {
			ostringstream msg;
			msg << "`" << yyvsp[0] << "' is not a Float64 value.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		    else if (!TOP_OF_STACK->append_attr(name, type, yyvsp[0])) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		;}
    break;

  case 28:
#line 305 "hdfeos_das.y"
    {
		    type = "Float64";
		    DBG(cerr << "Adding FLOAT: " << TYPE_NAME_VALUE(yyvsp[0]) << endl);
		    if (!check_float64(yyvsp[0])) {
			ostringstream msg;
			msg << "`" << yyvsp[-2] << "' is not a Float64 value.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		    else if (!TOP_OF_STACK->append_attr(name, type, yyvsp[0])) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		;}
    break;

  case 29:
#line 324 "hdfeos_das.y"
    {
		    type = "Float64";
		    DBG(cerr << "Adding FLOAT: " << TYPE_NAME_VALUE(yyvsp[0]) << endl);
		    if (!check_float64(yyvsp[0])) {
			ostringstream msg;
			msg << "`" << yyvsp[0] << "' is not a Float64 value.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		    else if (!TOP_OF_STACK->append_attr(name, type, yyvsp[0])) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		;}
    break;

  case 30:
#line 341 "hdfeos_das.y"
    {
		    type = "Float64";
		    DBG(cerr << "Adding FLOAT: " << TYPE_NAME_VALUE(yyvsp[0]) << endl);
		    if (!check_float64(yyvsp[0])) {
			ostringstream msg;
			msg << "`" << yyvsp[-2] << "' is not a Float64 value.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		    else if (!TOP_OF_STACK->append_attr(name, type, yyvsp[0])) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		;}
    break;

  case 33:
#line 363 "hdfeos_das.y"
    {
		    type = "String";
		    DBG(cerr << "Adding STR: " << TYPE_NAME_VALUE(yyvsp[0]) << endl);
		    if (!TOP_OF_STACK->append_attr(name, type, yyvsp[0])) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		    if (name=="GridName" || name=="SwathName" || name=="PointName") {
		      // Strip off quotes in new ID
		      string newname = yyvsp[0]+1;
		      newname.erase(newname.end()-1);
		      // and convert embedded spaces to _
		      string::size_type space = 0;
		      while((space = newname.find_first_of(' ', space)) != newname.npos) {
			newname[space] = '_';
		      }
 		      SECOND_IN_STACK->attr_alias(newname, last_grid_swath);
		    }
		;}
    break;

  case 34:
#line 385 "hdfeos_das.y"
    {
		    type = "String";
		    DBG(cerr << "Adding STR: " << TYPE_NAME_VALUE(yyvsp[0]) << endl);
		    if (!TOP_OF_STACK->append_attr(name, type, yyvsp[0])) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		;}
    break;


    }

/* Line 1000 of yacc.c.  */
#line 1367 "hdfeos_das.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {
		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
		 yydestruct (yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
	  yydestruct (yytoken, &yylval);
	  yychar = YYEMPTY;

	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

  yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 397 "hdfeos_das.y"


// This function is required for linking, but DODS uses its own error
// reporting mechanism.

void
hdfeos_daserror(char *s)
{
  cerr << s << endl;
}

// I wrote this because I thought at one point that it was the solution to 
// having some of the libdap methods change out from under the hdfeos code
// here. In the end, I added a new find methof to the AttrTable class that 
// doesn't require FQNs for th paths. (see AttrTable::recurrsive_find)
static string
build_fqn(AttrTable *at, string fqn)
{
    // The strange behavior at the top level is because the top level of an
    // AttrTable (i.e. the DAS) is anonymous. Another bad design... jhrg 2/8/06
    if (!at || !at->get_parent() || at->get_name().empty())
        return fqn;
    else
        return build_fqn(at->get_parent(), at->get_name() + string(".") + fqn);
}

static void 
process_group(parser_arg * arg, const string & id)
{
    AttrTable *at;
    DBG(cerr << "Processing ID: " << id << endl);
    /* If we are at the outer most level of attributes, make
       sure to use the AttrTable in the DAS. */
    if (STACK_EMPTY) {
        at = ATTR_OBJ(arg)->get_attr_table(id);
        if (!at)
            at = ATTR_OBJ(arg)->append_container(id);
    } else {
        at = TOP_OF_STACK->get_attr_table(id);
        if (!at)
            at = TOP_OF_STACK->append_container(id);
    }

    if (id.find("GRID_") == 0 || id.find("SWATH_") == 0 ||
        id.find("POINT_") == 0) {
        last_grid_swath = id;
    }

    PUSH(at);
    DBG(cerr << " Pushed attr_tab: " << at << endl);
}


