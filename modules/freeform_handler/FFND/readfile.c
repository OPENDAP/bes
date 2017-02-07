/*
 * NAME:	readfile.c
 *
 * PURPOSE:	to read a binary file
 *
 * USAGE:	C main program
 *
 * RETURNS:
 *
 * DESCRIPTION:
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * AUTHOR:	T. Habermann, NGDC, (303) 497 - 6472, haber@mail.ngdc.noaa.gov
 * Modified by TAM
 * Modified by MAO
 *
 * COMMENTS:	This code will not work on the Mac, until reading input is
 * better dealt with.
 *
 * KEYWORDS:
 *
 * CAVEAT:
 * No claims are made as to the suitability of the accompanying
 * source code for any purpose.  Although this source code has been
 * used by the NOAA, no warranty, expressed or implied, is made by
 * NOAA or the United States Government as to the accuracy and
 * functioning of this source code, nor shall the fact of distribution
 * constitute any such endorsement, and no responsibility is assumed
 * by NOAA in connection therewith.  The source code contained
 * within was developed by an agency of the U.S. Government.
 * NOAA's National Geophysical Data Center has no objection to the
 * use of this source code for any purpose since it is not subject to
 * copyright protection in the U.S.  If this source code is incorporated
 * into other software, a statement identifying this source code may be
 * required under 17 U.S.C. 403 to appear with any copyright notice.
 */

#define WANT_NCSA_TYPES
#define DEFINE_DATA
#include "freeform.h"

/* Next three character array variables duplicate same in proclist.c */

const char *fft_cnv_flags_width[FFNT_ENOTE + 1] =
{
"%*d",  /* int8 */
"%*u",  /* uint8 */
"%*hd", /* int16 */
"%*hu", /* uint16 */
#ifdef LONGS_ARE_32
"%*ld", /* int32 */
"%*lu", /* uint32 */
"?",    /* int64 */
"?",    /* uint64 */
#elif defined(LONGS_ARE_64)
"%*d",  /* int32 */
"%*u",  /* uint32 */
"%*ld", /* int64 */
"%*lu", /* uint64 */
#endif
"%*f",  /* float32 */
"%*f",  /* float64 */
"%*E"   /* E-notation */
};

const char *fft_cnv_flags_prec[FFNT_ENOTE + 1] =
{
"%.*d",  /* int8 */
"%.*u",  /* uint8 */
"%.*hd", /* int16 */
"%.*hu", /* uint16 */
#ifdef LONGS_ARE_32
"%.*ld", /* int32 */
"%.*lu", /* uint32 */
"?",     /* int64 */
"?",     /* uint64 */
#elif defined(LONGS_ARE_64)
"%.*d",  /* int32 */
"%.*u",  /* uint32 */
"%.*ld", /* int64 */
"%.*lu", /* uint64 */
#endif
"%.*f",  /* float32 */
"%.*f",  /* float64 */
"%.*E"   /* E-notation */
};

const char *fft_cnv_flags_width_prec[FFNT_ENOTE + 1] =
{
"%*.*d",  /* int8 */
"%*.*u",  /* uint8 */
"%*.*hd", /* int16 */
"%*.*hu", /* uint16 */
#ifdef LONGS_ARE_32
"%*.*ld", /* int32 */
"%*.*lu", /* uint32 */
"?",      /* int64 */
"?",      /* uint64 */
#elif defined(LONGS_ARE_64)
"%*.*d",  /* int32 */
"%*.*u",  /* uint32 */
"%*.*ld", /* int64 */
"%*.*lu", /* uint64 */
#endif
"%*.*f",  /* float32 */
"%*.*f",  /* float64 */
"%*.*E",  /* E-notation */
};

void show_opt(void);
char get_option(void);
void put_option(char c);
int check_error(int, int);

#define TEXT_CODE    't'

#define FLOAT32_CODE 'f'
#define FLOAT64_CODE 'd'

#define UINT_PREFIX  'u'

#define INT8_CODE    'c'
#define INT16_CODE   's'
#define INT32_CODE   'i'

#ifdef LONGS_ARE_64
#define INT64_CODE   'l'
#endif

#define UINT8_WIDTH   3
#define UINT16_WIDTH  5
#define UINT32_WIDTH 10
#define UINT64_WIDTH 20

#define INT8_WIDTH  UINT8_WIDTH  + 1
#define INT16_WIDTH UINT16_WIDTH + 1
#define INT32_WIDTH UINT32_WIDTH + 1
#define INT64_WIDTH UINT64_WIDTH + 1

#define FLOAT32_WIDTH 13
#define FLOAT32_PREC   6
#define FLOAT64_WIDTH 22
#define FLOAT64_PREC  15

char *input = NULL;

#if FF_OS == FF_OS_DOS
#define EOL_MARKER (int)'\r'
#endif
#if FF_OS == FF_OS_UNIX
#define EOL_MARKER (int)'\n'
#endif

#ifdef LITTLE_ENDIAN
#undef LITTLE_ENDIAN
#endif
#define LITTLE_ENDIAN 1

/* casts below are to ensure logical, rather than arithmetic, bit shifts */
#ifdef FLIP_4_BYTES
#undef FLIP_4_BYTES
#endif
#define FLIP_4_BYTES(a)	(	((*(uint32 *)(a) & 0x000000FFu) << 24) | \
							((*(uint32 *)(a) & 0x0000FF00u) <<  8) | \
							((*(uint32 *)(a) & 0x00FF0000u) >>  8) | \
							((*(uint32 *)(a) & 0xFF000000u) >> 24) )

#ifdef FLIP_2_BYTES
#undef FLIP_2_BYTES
#endif
#define FLIP_2_BYTES(a)	( ((*(uint16 *)(a) & 0xFF00u) >> 8) | \
                          ((*(uint16 *)(a) & 0x00FFu) << 8) )

/*  system dependent MACROS */

#if FF_OS == FF_OS_DOS

#include <conio.h>
#define os_getch() 	(getch())

#endif

#if FF_OS == FF_OS_UNIX

#define os_getch()	(getc(stdin))

#endif

void flip_2_bytes(void *s)
{
	*(int16 *)s = FLIP_2_BYTES(s);
}

void flip_4_bytes(void *l)
{
	*(int32 *)l = FLIP_4_BYTES(l);
}

void flip_8_bytes(void *d)
{
	int32 t;

	*(int32 *)d = FLIP_4_BYTES((int32 *)d);
	*((int32 *)d + 1) = FLIP_4_BYTES((int32 *)d + 1);

	t = *(int32 *)d;
	*(int32 *)d = *((int32 *)d + 1);
	*((int32 *)d + 1) = t;
}

/* little endian byte order: least significant byte = low address byte
   big endian byte order:  most significant byte = low address byte

   two byte int with a value of one (1)...
   big endian:     0x 00 01  (0x 00 00 00 01)
   little endian:  0x 01 00  (0x 01 00 00 00)
                      ^^         ^^
                    low address byte
*/
BOOLEAN endian(void)
{
	int i = LITTLE_ENDIAN;

	return(*(char *)&i);
}

int main(int argc, char **argv)
{
	double double_var;

	char *input_buffer = NULL;

#if 0
	unsigned char input_tty = 1;
#endif

	int data_file;
	int option;
	int last_option;
	int last_u_option;
	int flip_bytes = 0;

/*	long offset; */
/*	long record_number; */
	long file_position;
	long data_file_length;
	long ops_file_length = 0;
	long new_position;
/*	long long_var; */

	fprintf(stderr, "%s",
#ifdef FF_ALPHA
"\nWelcome to readfile alpha 1.1 "__DATE__" -- an NGDC binary file reader\n"
#elif defined(FF_BETA)
"\nWelcome to readfile beta 1.1 "__DATE__" -- an NGDC binary file reader\n"
#else
"\nWelcome to readfile release 1.1 -- an NGDC binary file reader\n"
#endif
	       );

	if (argc == 1)
	{
		fprintf(stderr, "%s", "\nUSAGE: readfile binary_file\n");
		exit(1);
	}

	show_opt();

#if FF_OS == FF_OS_DOS
	data_file = open(argv[1], O_RDONLY | O_BINARY);
#endif

#if FF_OS == FF_OS_UNIX
	data_file = open(argv[1], O_RDONLY);
#endif

	if(data_file == -1)
	{
		fprintf(stderr,"Could Not Open Input File %s\n", argv[1]);
		exit(0);
	}

	{
		FILE *fp = fopen(argv[1], "r");
		data_file_length = -1;

		if (fp)
		{
			if (!fseek(fp, 0, SEEK_END))
				data_file_length = ftell(fp);

			fclose(fp);
		}
	}

	/* Determine if the input is coming from a file, if so read the file in */
	if (!isatty(fileno(stdin)))
	{
		ops_file_length = lseek(fileno(stdin), 0L, SEEK_END);
		input_buffer = (char *)malloc((size_t)ops_file_length + 1);
		if (!input_buffer)
		{
			fprintf(stderr, "Insufficient memory -- file is too large");
			exit(1);
		}

		lseek(fileno(stdin), 0L, SEEK_SET);
		read(fileno(stdin), input_buffer, (size_t)ops_file_length);
		*(input_buffer + ops_file_length) = '\0';
		input = input_buffer;
	}

	last_u_option = ' ';
	last_option = ' ';

	while ((option = get_option()) != 'q')
	{
		if (option == UINT_PREFIX)
			last_u_option = ' ';
		if (option == EOL_MARKER)
			option = last_option;

		switch(option)
		{
			case TEXT_CODE:

				if (check_error(read(data_file, (char *)&double_var, sizeof(char)), sizeof(char)))
					break;

				fprintf(stdout, "text: \'%c\', dec: %3d, hex: %2x, oct: %3o\n",
				        *(char *)&double_var,
				        *(unsigned char *)&double_var,
				        *(unsigned char *)&double_var,
				        *(unsigned char *)&double_var);

			break;

			case INT8_CODE:

				if (check_error(read(data_file, (char *)&double_var, SIZE_INT8), SIZE_INT8))
					break;

				fprintf(stdout, "int8: ");
				fprintf(stdout, fft_cnv_flags_width[FFNT_INT8], INT8_WIDTH,
				        *((int8 *)&double_var));
				fprintf(stdout, "\n");
			break;

			case INT16_CODE:

				if (check_error(read(data_file, (char *)&double_var, SIZE_INT16), SIZE_INT16))
					break;

				if (flip_bytes)
				{
					flip_2_bytes(&double_var);
					fprintf(stdout,"int16, byte swapped: ");
					fprintf(stdout, fft_cnv_flags_width[FFNT_INT16], INT16_WIDTH,
					        *((int16 *)&double_var));
					flip_2_bytes(&double_var);
					fprintf(stdout,", (\"%s-endian\": ",
					        endian() == LITTLE_ENDIAN ? "little" : "big");
					fprintf(stdout, fft_cnv_flags_width[FFNT_INT16], INT16_WIDTH,
					        *((int16 *)&double_var));
					fprintf(stdout, ")\n");
				}
				else
				{
					fprintf(stdout, "int16: ");
					fprintf(stdout, fft_cnv_flags_width[FFNT_INT16], INT16_WIDTH,
					        *((int16 *)&double_var));
					fprintf(stdout, "\n");
				}

			break;

			case INT32_CODE:

				if (check_error(read(data_file, (char *)&double_var, SIZE_INT32), SIZE_INT32))
					break;

				if (flip_bytes)
				{
					flip_4_bytes(&double_var);
					fprintf(stdout,"int32, byte swapped: ");
					fprintf(stdout, fft_cnv_flags_width[FFNT_INT32], INT32_WIDTH,
					        *((int32 *)&double_var));
					flip_4_bytes(&double_var);
					fprintf(stdout,", (\"%s-endian\": ",
					        endian() == LITTLE_ENDIAN ? "little" : "big");
					fprintf(stdout, fft_cnv_flags_width[FFNT_INT32], INT32_WIDTH,
					        *((int32 *)&double_var));
					fprintf(stdout, ")\n");
				}
				else
				{
					fprintf(stdout, "int32: ");
					fprintf(stdout, fft_cnv_flags_width[FFNT_INT32], INT32_WIDTH,
					        *((int32 *)&double_var));
					fprintf(stdout, "\n");
				}

			break;

#ifdef LONGS_ARE_64

			case INT64_CODE:

				if (check_error(read(data_file, (char *)&double_var, SIZE_INT64), SIZE_INT64))
					break;

				if (flip_bytes)
				{
					flip_8_bytes(&double_var);
					fprintf(stdout,"int64, byte swapped: ");
					fprintf(stdout, fft_cnv_flags_width[FFNT_INT64], INT64_WIDTH,
					        *((int64 *)&double_var));
					flip_8_bytes(&double_var);
					fprintf(stdout,", (\"%s-endian\": ",
					        endian() == LITTLE_ENDIAN ? "little" : "big");
					fprintf(stdout, fft_cnv_flags_width[FFNT_INT64], INT64_WIDTH,
					        *((int64 *)&double_var));
					fprintf(stdout, ")\n");
				}
				else
				{
					fprintf(stdout, "int64: ");
					fprintf(stdout, fft_cnv_flags_width[FFNT_INT64], INT64_WIDTH,
					        *((int64 *)&double_var));
					fprintf(stdout, "\n");
				}

			break;

#endif

			case FLOAT32_CODE:

				if (check_error(read(data_file,(char *)&double_var, SIZE_FLOAT32), SIZE_FLOAT32))
					break;

				if (flip_bytes)
				{
					flip_4_bytes(&double_var);
					fprintf(stdout, "float32, byte swapped: ");
					fprintf(stdout, fft_cnv_flags_width_prec[FFNT_FLOAT32], FLOAT32_WIDTH,
					        FLOAT32_PREC, *((float32 *)&double_var));
					flip_4_bytes(&double_var);
					fprintf(stdout,", (\"%s-endian\": ",
					        endian() == LITTLE_ENDIAN ? "little" : "big");
					fprintf(stdout, fft_cnv_flags_width_prec[FFNT_FLOAT32], FLOAT32_WIDTH,
					        FLOAT32_PREC, *((float32 *)&double_var));
					fprintf(stdout, ")\n");
				}
				else
				{
					fprintf(stdout, "float32: ");
					fprintf(stdout, fft_cnv_flags_width_prec[FFNT_FLOAT32], FLOAT32_WIDTH,
					        FLOAT32_PREC, *((float32 *)&double_var));
					fprintf(stdout, "\n");
				}

			break;

			case FLOAT64_CODE:

				if (check_error(read(data_file,(char *)&double_var, SIZE_FLOAT64), SIZE_FLOAT64))
					break;

				if (flip_bytes)
				{
					flip_8_bytes(&double_var);
					fprintf(stdout, "float64, byte swapped: ");
					fprintf(stdout, fft_cnv_flags_width_prec[FFNT_FLOAT64], FLOAT64_WIDTH,
					        FLOAT64_PREC, *((float64 *)&double_var));
					flip_8_bytes(&double_var);
					fprintf(stdout,", (\"%s-endian\": ",
					        endian() == LITTLE_ENDIAN ? "little" : "big");
					fprintf(stdout, fft_cnv_flags_width_prec[FFNT_FLOAT64], FLOAT64_WIDTH,
					        FLOAT64_PREC, *((float64 *)&double_var));
					fprintf(stdout, ")\n");
				}
				else
				{
					fprintf(stdout, "float64: ");
					fprintf(stdout, fft_cnv_flags_width_prec[FFNT_FLOAT64], FLOAT64_WIDTH,
					        FLOAT64_PREC, *((float64 *)&double_var));
					fprintf(stdout, "\n");
				}

			break;

			case UINT_PREFIX: /* unsigned ... */

				/* If last option is not yet defined, prompt the user */
				if (last_u_option == ' ')
				{
					last_u_option = get_option();
				}

				switch (last_u_option)
				{
					case INT8_CODE: /* unsigned char */

						if (check_error(read(data_file, (char *)&double_var, SIZE_UINT8), SIZE_UINT8))
							break;

						fprintf(stdout, "uint8: ");
						fprintf(stdout, fft_cnv_flags_width[FFNT_UINT8], UINT8_WIDTH,
						        *((uint8 *)&double_var));
						fprintf(stdout, "\n");

					break;

					case INT16_CODE:

						if (check_error(read(data_file, (char *)&double_var, SIZE_UINT16), SIZE_UINT16))
							break;

						if (flip_bytes)
						{
							flip_2_bytes(&double_var);
							fprintf(stdout,"uint16, byte swapped: ");
							fprintf(stdout, fft_cnv_flags_width[FFNT_UINT16], UINT16_WIDTH,
							        *((uint16 *)&double_var));
							flip_2_bytes(&double_var);
							fprintf(stdout,", (\"%s-endian\": ",
							        endian() == LITTLE_ENDIAN ? "little" : "big");
							fprintf(stdout, fft_cnv_flags_width[FFNT_UINT16], UINT16_WIDTH,
							        *((uint16 *)&double_var));
							fprintf(stdout, ")\n");
						}
						else
						{
							fprintf(stdout, "uint16: ");
							fprintf(stdout, fft_cnv_flags_width[FFNT_UINT16], UINT16_WIDTH,
							        *((uint16 *)&double_var));
							fprintf(stdout, "\n");
						}

					break;

					case INT32_CODE:

						if (check_error(read(data_file, (char *)&double_var, SIZE_UINT32), SIZE_UINT32))
							break;

						if (flip_bytes)
						{
							flip_4_bytes(&double_var);
							fprintf(stdout,"uint32, byte swapped: ");
							fprintf(stdout, fft_cnv_flags_width[FFNT_UINT32], UINT32_WIDTH,
							        *((uint32 *)&double_var));
							flip_4_bytes(&double_var);
							fprintf(stdout,", (\"%s-endian\": ",
							        endian() == LITTLE_ENDIAN ? "little" : "big");
							fprintf(stdout, fft_cnv_flags_width[FFNT_UINT32], UINT32_WIDTH,
							        *((uint32 *)&double_var));
							fprintf(stdout, ")\n");
						}
						else
						{
							fprintf(stdout, "uint32: ");
							fprintf(stdout, fft_cnv_flags_width[FFNT_UINT32], UINT32_WIDTH,
							        *((uint32 *)&double_var));
							fprintf(stdout, "\n");
						}

					break;

#ifdef LONGS_ARE_64

					case INT64_CODE:

						if (check_error(read(data_file, (char *)&double_var, SIZE_UINT64), SIZE_UINT64))
							break;

						if (flip_bytes)
						{
							flip_8_bytes(&double_var);
							fprintf(stdout,"uint64, byte swapped: ");
							fprintf(stdout, fft_cnv_flags_width[FFNT_UINT64], UINT64_WIDTH,
							        *((uint64 *)&double_var));
							flip_8_bytes(&double_var);
							fprintf(stdout,", (\"%s-endian\": ",
							        endian() == LITTLE_ENDIAN ? "little" : "big");
							fprintf(stdout, fft_cnv_flags_width[FFNT_UINT64], UINT64_WIDTH,
							        *((uint64 *)&double_var));
							fprintf(stdout, ")\n");
						}
						else
						{
							fprintf(stdout, "uint64: ");
							fprintf(stdout, fft_cnv_flags_width[FFNT_UINT64], UINT64_WIDTH,
							        *((uint64 *)&double_var));
							fprintf(stdout, "\n");
						}

					break;

#endif

					default:
						fprintf(stdout,"Type 'h' or '?' to see options menu.\n");
						last_u_option = ' ';
					break;
				}/* end switch on unsigned types */

			break;

			case 'b': /* toggle between little-endian and big-endian */

				flip_bytes = (flip_bytes) ? 0 : 1;
				if (flip_bytes)
				{
					fprintf(stderr, "Displaying numbers with byte swapping\n");
				}
				else
				{
					/* Say what the native byte order IS */
					fprintf(stderr, "Displaying numbers using your machine's native byte order\n");
				}

			break;

			case 'p': /* position file, don't show file size */

				if (input)
				{
					input = strtok(input, " ");

					/* Check for errors */
					if (!input)
					{
						fprintf(stderr, "Missing blank space after number.");
						exit(1);
					}
					if ((input - input_buffer) >= ops_file_length)
					{
						fprintf(stderr, "Reading past end of input file.");
						exit(1);
					}

					sscanf(input, "%ld", &new_position);
					input += strlen(input) + 1;
				}
				else
				{
					fprintf(stdout,"Input New File Position in Bytes ");
					scanf("%ld", &new_position);

#if FF_OS == FF_OS_UNIX
					os_getch();
#endif

				}

				if (new_position > data_file_length)
				{
					fprintf(stderr,"ERROR: Position not changed.\n");
					fprintf(stderr,"       New position cannot be beyond %ld, the size of %s.\n", data_file_length, argv[1]);
					break;
				}

				file_position = lseek(data_file, new_position, SEEK_SET);
				fprintf(stdout,"New File Position = %ld\n", file_position);
				new_position = 0;

			break;

			case 'P':  /* show file position and size */

				file_position = lseek(data_file, 0L, SEEK_CUR);
				fprintf(stdout,"File Position: %ld\tFile Length: %ld\n", file_position, data_file_length);

			break;

			case 'h':
			case '?':
				show_opt();
			break;

			default:
				fprintf(stdout,"Type 'h' or '?' to see options menu.\n");
		} /* end switch on all types */
		last_option = option;
		if (last_option != 'u')
			last_u_option = ' ';

	}/* end while loop */

	return 0;
}/* end readfile */

void show_opt(void)
{
	fprintf(stderr,"\nOptions:\n");
	fprintf(stderr,"   %c:  text\t\tas ASCII, decimal, hexadecimal, and octal\n", TEXT_CODE);
	fprintf(stderr,"   %c:  int8\t\t1 byte signed integer\n", INT8_CODE);
	fprintf(stderr,"   %c:  int16\t\t2 byte signed integer\n", INT16_CODE);
	fprintf(stderr,"   %c:  int32\t\t4 byte signed integer\n", INT32_CODE);
#ifdef LONGS_ARE_64
	fprintf(stderr,"   %c:  int64\t\t8 byte signed integer\n", INT64_CODE);
#endif
	fprintf(stderr,"   %c:  float32\t\t4 byte single-precision floating point\n", FLOAT32_CODE);
	fprintf(stderr,"   %c:  float64\t\t8 byte double-precision floating point\n", FLOAT64_CODE);
	fprintf(stderr,"  %c%c:  uint8\t\t1 byte unsigned integer\n", UINT_PREFIX, INT8_CODE);
	fprintf(stderr,"  %c%c:  uint16\t\t2 byte unsigned integer\n", UINT_PREFIX, INT16_CODE);
	fprintf(stderr,"  %c%c:  uint32\t\t4 byte unsigned integer\n", UINT_PREFIX, INT32_CODE);
#ifdef LONGS_ARE_64
	fprintf(stderr,"  %c%c:  uint64\t\t8 byte unsigned integer\n", UINT_PREFIX, INT64_CODE);
#endif

	fprintf(stderr,"\n");
	fprintf(stderr,"   b:  Toggle between \"%s-endian\" and your machine's native byte order\n"
	        , endian() == LITTLE_ENDIAN ? "big" : "little");
	fprintf(stderr,"   p:  Set new file position\n");
	fprintf(stderr,"   P:  Show present file position and length\n");
	fprintf(stderr,"   h:  Display this help screen\n");
	fprintf(stderr,"   q:  Quit\n\n");
	fprintf(stderr,"%s option codes to view binary encoded values.\n"
#if FF_OS == FF_OS_DOS
          , "Type"
#endif
#if FF_OS == FF_OS_UNIX
          , "Enter"
#endif
	       );
	fprintf(stderr,"Tip:  %s return repeats the last option.\n"
#if FF_OS == FF_OS_DOS
          , "Pressing"
#endif
#if FF_OS == FF_OS_UNIX
          , "Entering a blank"
#endif
         );

	return;
}

/* This function allows input to be read from a file or the keyboard. On
the SUN, an extra getch() must be done to capture the return - otherwise
all options are executed twice.
*/
char get_option(void)
{
	int c;
	static int save_c;

	if (input)
		return(*(input++));

	c = os_getch();

#if FF_OS == FF_OS_UNIX

	/* If c is return, return it
	   If not, then the next character is either a return or another character
	   If it is a return, get it and ignore it
	   If it is a character, put it back so that it can be gotten */

	if (c != EOL_MARKER)
	{
		save_c = c;
		if ((c = os_getch()) != EOL_MARKER)
			ungetc(c, stdin);

		c = save_c;
	}
#endif

	return((char)c);
}

void put_option(char c)
{

	if (input)
		input--;
	else
		ungetc(c, stdin);

	return;
}

/* This function checks the return from read. It decides if there has either been
an error during the read, or if the end of the file has been reached. It prompts
the user to take appropriate action
*/
int check_error(int error, int byte_size)
{
	if (error != byte_size)
	{
		if (error)
		{
			fprintf(stderr, "Error reading file, Try resetting the file (p) or quit (q)");
		}
		else
		{
			if (!input)
				fprintf(stderr,"END of FILE: Enter 'q' to quit or 'p' to set a new position.\n");
			else
				fprintf(stderr,"END of FILE REACHED.\n");
		}
		return(1);
	}
	else
		return(0);
}
