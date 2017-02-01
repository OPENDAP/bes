/* FILENAME:  ff_utils.c
 *
 * CONTAINS:  newform()
 *            checkvar()
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

#include <freeform.h>

#ifdef TIMER
#include <time.h>
#endif 

typedef struct
{
	char str[MAX_NAME_LENGTH];
	long count;
} CHAR_ARRAY_TYPE;

typedef struct
{
	double bin;
	long count;
} BIN_ARRAY_TYPE;

#include <avltree.h>

#define FINT_EQ(a,b) (fabs((a)-(b))<0.5)

static long newform_total_bytes_to_process(DATA_BIN_PTR dbin)
{
	int error = 0;
	long bytes_to_process = 0;

	PROCESS_INFO_LIST pinfo_list = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	FF_VALIDATE(dbin);

	error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_OUTPUT, &pinfo_list);
	if (error)
		return -1;

	pinfo_list = dll_first(pinfo_list);
	pinfo      = FF_PI(pinfo_list);
	while (pinfo)
	{
		bytes_to_process += PINFO_SUPER_ARRAY_BYTES(pinfo);

		pinfo_list = dll_next(pinfo_list);
		pinfo      = FF_PI(pinfo_list);
	}

	ff_destroy_process_info_list(pinfo_list);

	return(bytes_to_process);
}

int do_log(FF_BUFSIZE_PTR log_bufsize, char *format, ...)
{
	va_list va_args;
	int bytes_written;

	va_start(va_args, format);
  
	if (log_bufsize)
	{
		if (log_bufsize->bytes_used + LOGGING_QUANTA > log_bufsize->total_bytes)
		{
			int error = 0;

			error = ff_resize_bufsize(log_bufsize->total_bytes + LOGGING_QUANTA, &log_bufsize);
			if (error)
			{
				err_push(ERR_MEM_LACK, "");
				return(0);
			}
		}
  
		vsprintf(log_bufsize->buffer + log_bufsize->bytes_used, format, va_args);
		bytes_written = strlen(log_bufsize->buffer + log_bufsize->bytes_used);
		log_bufsize->bytes_used += bytes_written;
			
		assert(log_bufsize->bytes_used < log_bufsize->total_bytes);
	}
	else
		bytes_written = vfprintf(stderr, format, va_args);

	return(bytes_written);
}

/*****************************************************************************
 * NAME: wfprintf()
 *
 * PURPOSE:  FF_MAIN-sensitive wrapper around fprintf()
 *
 * USAGE:  same as fprintf()
 *
 * RETURNS:  same as fprintf()
 *
 * DESCRIPTION:  If FF_MAIN is defined, calls vfprintf().
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * GLOBALS:
 *
 * COMMENTS:
 *
 * KEYWORDS:
 *
 * ERRORS:  vsprintf returns char * on non-System V Unix environments.
 ****************************************************************************/

int wfprintf(FILE *stream, const char *format, ...)
{
	va_list va_args;

	va_start(va_args, format);
	
	return(stream ? vfprintf(stream, format, va_args) : 0);
}

static int warn_if_eqv_vlist_mismatch
	(
	 NAME_TABLE_PTR table1,
	 NAME_TABLE_PTR table2
	)
{
	int error = 0;

	VARIABLE_LIST vlist = NULL;

	VARIABLE_PTR var1 = NULL;
	VARIABLE_PTR var2 = NULL;

	FF_VALIDATE(table1);
	FF_VALIDATE(table2);

	vlist = FFV_FIRST_VARIABLE(table1->format);
	var1 = FF_VARIABLE(vlist);
	while (var1)
	{
		FF_VALIDATE(var1);

		var2 = ff_find_variable(var1->name, table2->format);
		if (!var2)
			error = err_push(ERR_WARNING_ONLY + ERR_EQV_CONTEXT, "%s definition in %s equivalence section", var1->name, IS_INPUT(table1->format) ? "input" : "output");
		else if (FF_VAR_LENGTH(var1) != FF_VAR_LENGTH(var2))
			error = err_push(ERR_WARNING_ONLY + ERR_EQV_CONTEXT, "%s definition differs between equivalence sections", var1->name);
		else if (memcmp(table1->data->buffer + var1->start_pos - 1, table2->data->buffer + var2->start_pos - 1, FF_VAR_LENGTH(var1)))
			error = err_push(ERR_WARNING_ONLY + ERR_EQV_CONTEXT, "%s definition differs between equivalence sections", var1->name);
		else if ((IS_TRANSLATOR(var1) && !IS_TRANSLATOR(var2)) || (!IS_TRANSLATOR(var1) && IS_TRANSLATOR(var2)))
			error = err_push(ERR_WARNING_ONLY + ERR_EQV_CONTEXT, "%s definition differs between equivalence sections", var1->name);
		else if (IS_TRANSLATOR(var1) && IS_TRANSLATOR(var2))
		{
			if (!nt_comp_translator_sll(var1, var2))
				error = err_push(ERR_WARNING_ONLY + ERR_EQV_CONTEXT, "%s translators differ between equivalence sections", var1->name);
		}

		vlist = dll_next(vlist);
		var1 = FF_VARIABLE(vlist);
	}

	return error;
}

/*****************************************************************************
 * NAME:  warn_if_eqv_mismatch
 *
 * PURPOSE:  Find if constants and name equivalences have different definitions
 * in the input and output name tables.
 *
 * USAGE:  error = warn_if_eqv_mismatch(dbin);
 *
 * RETURNS:  Zero if no differences in definitions, otherwise an error code
 *
 * DESCRIPTION:  If either an input or an output name table exists, but not
 * both, this is an error.  If both exist, and any constant has different
 * definitions, this is also an error.  If any name equivalence has
 * different definitions, this too is an error.  Lastly, if any name
 * equivalence has translators with different values, this is an error.
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * GLOBALS:
 *
 * COMMENTS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/

static int warn_if_eqv_mismatch(DATA_BIN_PTR dbin)
{
	int error = 0;

	NAME_TABLE_PTR intable = NULL;
	NAME_TABLE_PTR outtable = NULL;

	FF_VALIDATE(dbin);

	intable = fd_find_format_data(dbin->table_list, FFF_GROUP, FFF_TABLE | FFF_INPUT);
	outtable = fd_find_format_data(dbin->table_list, FFF_GROUP, FFF_TABLE | FFF_OUTPUT);

	if (intable)
		FF_VALIDATE(intable);

	if (outtable)
		FF_VALIDATE(outtable);

	if (!intable && !outtable)
		return 0;
	else if ((intable && !outtable) || (!intable && outtable))
		return err_push(ERR_WARNING_ONLY + ERR_EQV_CONTEXT, "%sput equivalence section", intable ? "in" : "out");

	error = warn_if_eqv_vlist_mismatch(intable, outtable);
	if (!error)
		error = warn_if_eqv_vlist_mismatch(outtable, intable);

	return error;
}

/*****************************************************************************
 * NAME:  write_output_format_file
 *
 * PURPOSE:  Write a format file for the output data file if safe
 *
 * USAGE:  error = write_output_format_file(dbin, std_args);
 *
 * RETURNS:  Zero on success, an error code on failure
 *
 * DESCRIPTION:  Determine if writing a default format file (fname.fmt) for the
 * output data file would overwrite an original format file (there may be two,
 * one given with -if and the other given with -of).  If not, write the format
 * file, unless it already exists (e.g., from a previous newform run).
 *
 * If an output format file cannot be written because it would overwrite an
 * original format file, then check to see if there are definitions in input
 * and output equivalence sections which disagree, and give an appropriate
 * warning.  For example, we're creating binary output data and the original
 * input format file has an output_eqv defining data_byte_order, but data_byte_order
 * is not defined in an input_eqv.  The original input format file cannot describe
 * the new data file, because the new data file's format file needs to define
 * the data_byte_order in an input_eqv (not the output_eqv).
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * GLOBALS:
 *
 * COMMENTS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/

static int write_output_format_file
	(
	 DATA_BIN_PTR dbin,
	 FF_STD_ARGS_PTR std_args
	)
{
	int error = 0;
	BOOLEAN write_it = TRUE;

	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	char *input_format_file = NULL;

	char infmtpath[MAX_PATH] = {""};
	char infmtname[MAX_PATH] = {""};

	char outdatapath[MAX_PATH] = {""};
	char outdataname[MAX_PATH] = {""};

	char inoutfmtpath[MAX_PATH] = {""};
	char inoutfmtname[MAX_PATH] = {""};

	char fmt_filename[MAX_PATH];

	FF_VALIDATE(dbin);
	FF_VALIDATE(std_args);

	os_path_get_parts(std_args->output_format_file, inoutfmtpath, inoutfmtname, NULL);

	if (!db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT | FFF_DATA, &plist))
	{
		pinfo = FF_PI(dll_first(plist));

		FF_VALIDATE(pinfo);

		if (PINFO_IS_FILE(pinfo))
		{
			input_format_file = PINFO_ORIGIN(pinfo);
			os_path_get_parts(input_format_file, infmtpath, infmtname, NULL);
		}

		ff_destroy_process_info_list(plist);
	}

	if (!db_ask(dbin, DBASK_PROCESS_INFO, FFF_OUTPUT | FFF_DATA, &plist))
	{
		pinfo = FF_PI(dll_first(plist));

		FF_VALIDATE(pinfo);

		if (PINFO_IS_FILE(pinfo))
			os_path_get_parts(PINFO_FNAME(pinfo), outdatapath, outdataname, NULL);

		ff_destroy_process_info_list(plist);
	}

	if (strlen(outdataname))
	{
		if (!strcmp(outdatapath, infmtpath) && !strcmp(outdataname, infmtname))
		{
			err_push(ERR_WARNING_ONLY + ERR_WONT_OVERWRITE_FILE, "New format file would overwrite input format file (%s)", input_format_file);
			write_it = FALSE;
		}
		else if (!strcmp(outdatapath, inoutfmtpath) && !strcmp(outdataname, inoutfmtname))
		{
			err_push(ERR_WARNING_ONLY + ERR_WONT_OVERWRITE_FILE, "New format file would overwrite output format file (%s)", std_args->output_format_file);
			write_it = FALSE;
		}
	}
	else
		write_it = FALSE;

	os_path_put_parts(fmt_filename, outdatapath, outdataname, "fmt");

	if (write_it)
	{
		if (os_file_exist(fmt_filename))
			err_push(ERR_WARNING_ONLY + ERR_WONT_OVERWRITE_FILE, "Output format file (%s) already exists!", fmt_filename);
		else
			error = db_do(dbin, DBDO_WRITE_OUTPUT_FMT_FILE, fmt_filename);
	}
	else if (strlen(outdataname))
	{
		error = warn_if_eqv_mismatch(dbin);
		if (error)
			error = err_push(ERR_WARNING_ONLY + ERR_POSSIBLE, "Using %s with %s", fmt_filename, std_args->output_file);
	}

	return error;
}

static int get_geo_ref
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t io,
	 int *numof,
	 char **dim_names_vector[],
	 FF_ARRAY_DIM_INFO_HANDLE *dim_info,
	 int fudge
	)
{
	int i = 0;
	int num_names = 0;
	char **names_vector = NULL;

	int error = 0;

	FF_VALIDATE(dbin);

	*numof = 0;
	*dim_info = NULL;

	/* What to do if multiple arrays? */

	error = db_ask(dbin, DBASK_VAR_NAMES, (FFF_IO & io) | FFF_DATA, &num_names, &names_vector);
	if (error)
		return error;

	i = 0;
	while (i < num_names)
	{
		if (strstr(names_vector[i], "EOL"))
			i++;
		else
			break;
	}

	if (i < num_names)
	{
		error = db_ask(dbin, DBASK_ARRAY_DIM_NAMES, names_vector[i], numof, dim_names_vector);
		if (!error)
		{
			int j = 0;

			*dim_info = (FF_ARRAY_DIM_INFO_HANDLE)memCalloc(*numof, sizeof(FF_ARRAY_DIM_INFO_PTR), "dim_info");
			if (!*dim_info)
			{
				memFree(names_vector, "");
				return error = err_push(ERR_MEM_LACK, "");
			}

			for (j = 0; j < *numof; j++)
			{
				error = db_ask(dbin, DBASK_ARRAY_DIM_INFO, names_vector[i], (*dim_names_vector)[j], &(*dim_info)[j]);
				if (!error)
				{
					if (fudge)
					{
						if ((*dim_info)[j]->start_index < (*dim_info)[j]->end_index)
							(*dim_info)[j]->end_index += 1;
						else
							(*dim_info)[j]->start_index += 1;
					}

#ifdef _DEBUG
					printf("\nArray %s, dimension %s:\n", names_vector[i], (*dim_names_vector)[j]);
					printf("Start index is %ld\n", (*dim_info)[j]->start_index);
					printf("End index is %ld\n", (*dim_info)[j]->end_index);
					printf("Granularity is %ld\n", (*dim_info)[j]->granularity);
					printf("Separation is %ld\n", (*dim_info)[j]->separation);
					printf("Grouping is %ld\n", (*dim_info)[j]->grouping);
					printf("Number of elements in array is %ld\n\n", (*dim_info)[j]->num_array_elements);
#endif
				}
			}
		}
	}

	memFree(names_vector, "");

	return error;
}

#define NT_OUT_HD FFF_OUTPUT | FFF_HEADER | FFF_FILE
#define NT_IN_HD FFF_INPUT | FFF_HEADER | FFF_FILE

static int update_image_info(DATA_BIN_PTR dbin)
{
	int error = 0;

	int i = 0;

	double upper_map_y = 0;
	double lower_map_y = 0;
	double right_map_x = 0;
	double left_map_x = 0;

	double grid_sizex = 0;
	double grid_sizey = 0;

	long number_of_rows = 0;
	long number_of_columns = 0;

	int innumof = 0;
	int outnumof = 0;

	int fudge = 0;

	char **in_dim_names = NULL;
	char **out_dim_names = NULL;

	FF_ARRAY_DIM_INFO_HANDLE in_diminfo = NULL;
	FF_ARRAY_DIM_INFO_HANDLE out_diminfo = NULL;

	char grid_cell_registration[MAX_PV_LENGTH] = {""};

	FF_VALIDATE(dbin);

	error = nt_ask(dbin, NT_INPUT, "grid_cell_registration", FFV_TEXT, grid_cell_registration);
	if (error && error != ERR_NT_KEYNOTDEF)
		error = err_push(ERR_PARAM_VALUE, "for grid_cell_registration (%s)", grid_cell_registration);
	else if (!error)
	{
		if (!os_strncmpi(grid_cell_registration, "center", 6))
			fudge = 1;
	}
	else if (error == ERR_NT_KEYNOTDEF)
		error = 0;

	if (error)
		return error;

	nt_ask(dbin, NT_IN_HD, "upper_map_y", FFV_DOUBLE, &upper_map_y);
	nt_ask(dbin, NT_IN_HD, "lower_map_y", FFV_DOUBLE, &lower_map_y);
	nt_ask(dbin, NT_IN_HD, "right_map_x", FFV_DOUBLE, &right_map_x);
	nt_ask(dbin, NT_IN_HD, "left_map_x", FFV_DOUBLE, &left_map_x);

	nt_ask(dbin, NT_IN_HD, "number_of_rows", FFV_LONG, &number_of_rows);
	nt_ask(dbin, NT_IN_HD, "number_of_columns", FFV_LONG, &number_of_columns);

	error = get_geo_ref(dbin, FFF_INPUT, &innumof, &in_dim_names, &in_diminfo, fudge);
	if (error)
		return error;

	error = get_geo_ref(dbin, FFF_OUTPUT, &outnumof, &out_dim_names, &out_diminfo, fudge);
	if (error)
		return error;

	if (!innumof || !outnumof)
		goto update_image_info_exit;
	
	if (number_of_rows)
	{
		error = nt_ask(dbin, NT_IN_HD, "grid_size(y)", FFV_DOUBLE, &grid_sizey);
		if (error && error != ERR_NT_KEYNOTDEF)
			return err_push(ERR_GENERAL, "Problem with definition of grid_size(y)");
		else if (error == ERR_NT_KEYNOTDEF)
		{
			error = nt_ask(dbin, NT_IN_HD, "grid_size", FFV_DOUBLE, &grid_sizey);
			if (error && error != ERR_NT_KEYNOTDEF)
				return err_push(ERR_GENERAL, "Problem with definition of grid_size");
			else if (error == ERR_NT_KEYNOTDEF)
				grid_sizey = 0;
		}
		else
			grid_sizey = 0;

		if (grid_sizey == 0)
			grid_sizey = (upper_map_y - lower_map_y) / number_of_rows;

		if (grid_sizey)
		{
			if (in_diminfo[0]->start_index > in_diminfo[0]->end_index)
			{
				upper_map_y -= labs(in_diminfo[0]->start_index - out_diminfo[0]->start_index) * grid_sizey;
				lower_map_y += labs(out_diminfo[0]->end_index - in_diminfo[0]->end_index) * grid_sizey;
			}
			else
			{
				upper_map_y -= labs(in_diminfo[0]->end_index - out_diminfo[0]->end_index) * grid_sizey;
				lower_map_y += labs(out_diminfo[0]->start_index - in_diminfo[0]->start_index) * grid_sizey;
			}
		}
	}

	if (number_of_columns)
	{
		error = nt_ask(dbin, NT_IN_HD, "grid_size(x)", FFV_DOUBLE, &grid_sizex);
		if (error && error != ERR_NT_KEYNOTDEF)
			return err_push(ERR_GENERAL, "Problem with definition of grid_size(x)");
		else if (error == ERR_NT_KEYNOTDEF)
		{
			error = nt_ask(dbin, NT_IN_HD, "grid_size", FFV_DOUBLE, &grid_sizex);
			if (error && error != ERR_NT_KEYNOTDEF)
				return err_push(ERR_GENERAL, "Problem with definition of grid_size");
			else if (error == ERR_NT_KEYNOTDEF)
				grid_sizex = 0;
		}
		else
			grid_sizex = 0;

		if (grid_sizex == 0)
			grid_sizex = (right_map_x - left_map_x) / number_of_columns;

		if (grid_sizex)
		{
			if (in_diminfo[1]->start_index < in_diminfo[1]->end_index)
			{
				left_map_x += labs(out_diminfo[1]->start_index - in_diminfo[1]->start_index) * grid_sizex;
				right_map_x -= labs(in_diminfo[1]->end_index - out_diminfo[1]->end_index) * grid_sizex;
			}
			else
			{
				left_map_x += labs(out_diminfo[1]->end_index - in_diminfo[1]->end_index) * grid_sizex;
				right_map_x -= labs(in_diminfo[1]->start_index - out_diminfo[1]->start_index) * grid_sizex;
			}
		}
	}

	if (nt_askexist(dbin, FFF_OUTPUT | NT_TABLE, "file_title"))
	{
		char file_title[MAX_PV_LENGTH];

		nt_ask(dbin, FFF_OUTPUT | NT_TABLE, "file_title", FFV_TEXT, file_title);

		error = nt_put(dbin, NT_OUT_HD, "file_title", FFV_TEXT, file_title);
	}
	else if (nt_askexist(dbin, FFF_FILE | FFF_HEADER, "file_title"))
	{
		char file_title[MAX_PV_LENGTH];

		strcpy(file_title, "Subset of ");
		nt_ask(dbin, NT_IN_HD, "file_title", FFV_TEXT, file_title + strlen(file_title));

		error = nt_put(dbin, NT_OUT_HD, "file_title", FFV_TEXT, file_title);
	}

	number_of_rows = labs(out_diminfo[0]->end_index - out_diminfo[0]->start_index) + 1 - fudge;
	if (!error && nt_askexist(dbin, NT_IN_HD, "number_of_rows"))
		error = nt_put(dbin, NT_OUT_HD, "number_of_rows", FFV_LONG, &number_of_rows);

	number_of_columns = labs(out_diminfo[1]->end_index - out_diminfo[1]->start_index) + 1 - fudge;
	if (!error && nt_askexist(dbin, NT_IN_HD, "number_of_columns"))
		error = nt_put(dbin, NT_OUT_HD, "number_of_columns", FFV_LONG, &number_of_columns);

	if (!error && nt_askexist(dbin, NT_IN_HD, "upper_map_y"))
		error = nt_put(dbin, NT_OUT_HD, "upper_map_y", FFV_DOUBLE, &upper_map_y);

	if (!error && nt_askexist(dbin, NT_IN_HD, "lower_map_y"))
		error = nt_put(dbin, NT_OUT_HD, "lower_map_y", FFV_DOUBLE, &lower_map_y);

	if (!error && nt_askexist(dbin, NT_IN_HD, "left_map_x"))
		error = nt_put(dbin, NT_OUT_HD, "left_map_x", FFV_DOUBLE, &left_map_x);

	if (!error && nt_askexist(dbin, NT_IN_HD, "right_map_x"))
		error = nt_put(dbin, NT_OUT_HD, "right_map_x", FFV_DOUBLE, &right_map_x);

	if (!error && nt_askexist(dbin, NT_IN_HD, "data_representation"))
	{
		char data_representation[MAX_PV_LENGTH];
		FORMAT_DATA_PTR fd = NULL;
		VARIABLE_PTR var = NULL;

		fd = fd_get_data(dbin, FFF_OUTPUT);
		FF_VALIDATE(fd);

		var = FF_VARIABLE(FFV_FIRST_VARIABLE(fd->format)); /* can we assume the first variable? */ 
		FF_VALIDATE(var);

		strcpy(data_representation, ff_lookup_string(variable_types, FFV_DATA_TYPE(var)));

		error = nt_put(dbin, NT_OUT_HD, "data_representation", FFV_TEXT, data_representation);
	}

	if (!error && nt_askexist(dbin, NT_IN_HD, "file_type"))
	{
		char file_type[MAX_PV_LENGTH];
		FORMAT_DATA_PTR fd = NULL;

		fd = fd_get_data(dbin, FFF_OUTPUT);
		FF_VALIDATE(fd);

		if (IS_BINARY(fd->format))
			strcpy(file_type, "binary");
		else
			strcpy(file_type, "ascii");

		error = nt_put(dbin, NT_OUT_HD, "file_type", FFV_TEXT, file_type);
	}

	if (!error && nt_askexist(dbin, FFF_INPUT | NT_TABLE, "data_type"))
	{
		char data_type[MAX_PV_LENGTH];

		error = nt_ask(dbin, FFF_INPUT | NT_TABLE, "data_type", FFV_TEXT, data_type);
		if (!error)
			error = nt_put(dbin, FFF_OUTPUT | NT_TABLE, "data_type", FFV_TEXT, data_type);
	}

	if (!error && nt_askexist(dbin, FFF_INPUT | NT_TABLE, "grid_cell_registration"))
	{
		char grid_cell_registration[MAX_PV_LENGTH];

		error = nt_ask(dbin, FFF_INPUT | NT_TABLE, "grid_cell_registration", FFV_TEXT, grid_cell_registration);
		if (!error)
			error = nt_put(dbin, FFF_OUTPUT | NT_TABLE, "grid_cell_registration", FFV_TEXT, grid_cell_registration);
	}

update_image_info_exit:

	for (i = 0; i < innumof; i++)
		memFree(in_diminfo[i], "in_diminfo[i]");
	memFree(in_diminfo, "in_diminfo");

	for (i = 0; i < outnumof; i++)
		memFree(out_diminfo[i], "out_diminfo[i]");
	memFree(out_diminfo, "out_diminfo");

	memFree(in_dim_names, "in_dim_names");
	memFree(out_dim_names, "out_dim_names");

	return error;
}

static int fix_header(DATA_BIN_PTR dbin)
{
	int error = 0;
	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	FF_VALIDATE(dbin);

	error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT | FFF_HEADER, &plist);
	if (!error)
	{
		plist = dll_first(plist);
		pinfo = FF_PI(plist);
		while (!error && pinfo)
		{
			FF_VALIDATE(pinfo);

			if (IS_VARIED(PINFO_FORMAT(pinfo)) && PINFO_MATE(pinfo) && IS_VARIED(PINFO_MATE_FORMAT(pinfo)))
			{
				VARIABLE_PTR var = NULL;
				VARIABLE_LIST vlist = NULL;

				FF_BUFSIZE_PTR bufsize = NULL;

				FF_VALIDATE(PINFO_FORMAT(pinfo));

				bufsize = ff_create_bufsize(FORMAT_LENGTH(PINFO_FORMAT(pinfo)) + 1);
				if (!bufsize)
					return ERR_MEM_LACK;

				vlist = FFV_FIRST_VARIABLE(PINFO_FORMAT(pinfo));
				var = FF_VARIABLE(vlist);
				while (var)
				{
					FF_VALIDATE(var);

					if (!IS_TEXT(var) && !IS_CONSTANT(var) && !IS_INITIAL(var))
					{
						if (!nt_ask(dbin, NT_TABLE | FFF_FORMAT_TYPE(PINFO_FORMAT(pinfo)), var->name, FFV_TYPE(var), bufsize->buffer))
						{
							char *geovu_name = nt_find_geovu_name(dbin, FFF_INPUT, var->name, NULL);

							error = nt_put(dbin, FFF_FORMAT_TYPE(PINFO_MATE_FORMAT(pinfo)), geovu_name ? geovu_name : var->name, FFV_TYPE(var), bufsize->buffer);
							if (error)
								break;
						}
					}

					vlist = dll_next(vlist);
					var = FF_VARIABLE(vlist);
				}

				ff_destroy_bufsize(bufsize);
			}

			plist = dll_next(plist);
			pinfo = FF_PI(plist);
		}

		ff_destroy_process_info_list(plist);
	}

	return error;
}

static int set_new_mms
	(
	 DATA_BIN_PTR dbin,
	 VARIABLE_PTR var,
	 int band,
	 char *mm_spec
	)
{
	BOOLEAN found = FALSE;
	int error = 0;
	char name_buffer[MAX_NAME_LENGTH];

	FF_VALIDATE(dbin);
	FF_VALIDATE(var);

	sprintf(name_buffer, "%s_%simum", var->name, mm_spec);
	if (nt_askexist(dbin, FFF_FILE | FFF_HEADER, name_buffer))
		found = TRUE;

	if (!found)
	{
		sprintf(name_buffer, "%s_%s", var->name, mm_spec);
		if (nt_askexist(dbin, FFF_FILE | FFF_HEADER, name_buffer))
			found = TRUE;
	}

	if (!found)
	{
		sprintf(name_buffer, "band_%d_%s", band + 1, mm_spec);
		if (nt_askexist(dbin, FFF_FILE | FFF_HEADER, name_buffer))
			found = TRUE;
	}

	if (!found)
	{
		sprintf(name_buffer, "%simum_value", mm_spec);
		if (nt_askexist(dbin, FFF_FILE | FFF_HEADER, name_buffer))
			found = TRUE;
	}

	if (found)
	{
		double d = 0;

		error = btype_to_btype(strcmp(mm_spec, "min") ? var->misc.mm->maximum : var->misc.mm->minimum, FFV_DATA_TYPE(var), &d, FFV_DOUBLE);
		if (!error)
		{
			if (IS_INTEGER(var) && var->precision)
				d /= pow(10, var->precision);

			error = nt_put(dbin, NT_OUT_HD, name_buffer, FFV_DOUBLE, &d);
		}
	}

	return error;
}

static int set_var_minmaxs(DATA_BIN_PTR dbin)
{
	int error = 0;

	int band = 0;

	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	FF_VALIDATE(dbin);

	error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_OUTPUT | FFF_DATA, &plist);
	if (!error)
	{
		plist = dll_first(plist);
		pinfo = FF_PI(plist);
		while (!error && pinfo)
		{
			VARIABLE_LIST vlist = NULL;
			VARIABLE_PTR var = NULL;

			FF_VALIDATE(PINFO_FORMAT(pinfo));

			band = 0;
			vlist = FFV_FIRST_VARIABLE(PINFO_FORMAT(pinfo));
			var = FF_VARIABLE(vlist);
			while (var)
			{
				FF_VALIDATE(var);

				if (IS_TEXT(var) || IS_INTEGER(var) || IS_REAL(var))
				{
					error = set_new_mms(dbin, var, band, "min");
					if (error)
						break;

					error = set_new_mms(dbin, var, band, "max");
					if (error)
						break;
				}

				++band;

				vlist = dll_next(vlist);
				var = FF_VARIABLE(vlist);
			}

			plist = dll_next(plist);
			pinfo = FF_PI(plist);
		}

		ff_destroy_process_info_list(plist);
	}
	else if (error == ERR_GENERAL)
		error = 0;

	return(error);
}

static void change_param_name
	(
	 DATA_BIN_PTR dbin,
	 VARIABLE_PTR var,
	 short distance,
	 FORMAT_PTR format
	)
{
	char *geovu_name = NULL;
	char inuser_name[MAX_PV_LENGTH];
	char outuser_name[MAX_PV_LENGTH];
	char *outuvalue_ptr = NULL;

	FF_VALIDATE(dbin);
	FF_VALIDATE(var);
	FF_VALIDATE(format);

	strncpy(inuser_name, var->name, sizeof(inuser_name) - 1);
	inuser_name[sizeof(inuser_name) - 1] = STR_END;

	assert(!strcmp(inuser_name + strlen(inuser_name) - strlen("_\bname"), "_\bname"));

	int str_end = strlen(inuser_name) - strlen("_\bname");
	if (str_end < 0 || str_end-1 > MAX_PV_LENGTH)
		return;
	inuser_name[str_end] = STR_END;

	os_str_trim_whitespace(inuser_name, inuser_name);

	geovu_name = nt_find_geovu_name(dbin, FFF_INPUT, inuser_name, NULL);
	if (!geovu_name)
		geovu_name = inuser_name;

	outuvalue_ptr = nt_find_user_name(dbin, FFF_OUTPUT, geovu_name, NULL);

	if (!outuvalue_ptr)
	{
		if (geovu_name)
			outuvalue_ptr = geovu_name;
		else
			outuvalue_ptr = inuser_name;
	}

	if (distance)
		snprintf(outuser_name, MAX_PV_LENGTH, "%-*.*s", distance, distance, outuvalue_ptr);
	else
	{
		strncpy(outuser_name, outuvalue_ptr, MAX_PV_LENGTH-1);
		outuser_name[MAX_PV_LENGTH-1] = '\0';

		if (outuser_name[strlen(outuser_name) - 1] != ' ')
			strcat(outuser_name, " ");
	}

	new_name_string__(outuser_name, &var->name);

	update_format_var(FFV_CONSTANT, strlen(var->name), var, format);
}

static void update_variable_precision
	(
	 DATA_BIN_PTR dbin,
	 VARIABLE_PTR var
	)
{
	FORMAT_DATA_PTR od = NULL;

	VARIABLE_PTR od_var = NULL;

	char od_varname[MAX_PV_LENGTH];

	FF_VALIDATE(dbin);

	od = fd_get_data(dbin, FFF_OUTPUT);
	if (!od)
		return;

	FF_VALIDATE(od);

	strncpy(od_varname, var->name, sizeof(od_varname) - 1);
	od_varname[sizeof(od_varname) - 1] = STR_END;

	if (!strcmp(od_varname + strlen(od_varname) - 4, "_min") || !strcmp(od_varname + strlen(od_varname) - 4, "_max"))
	{
		int str_end = strlen(od_varname) -4;
		if (str_end < 0 || str_end-1 > MAX_PV_LENGTH)
			return;
		od_varname[strlen(od_varname) - 4] = STR_END;

		od_var = ff_find_variable(od_varname, od->format);
		if (od_var)
		{
			FF_VALIDATE(od_var);

			var->precision = od_var->precision;
		}
	}
}

static int change_param_value
	(
	 DATA_BIN_PTR dbin,
	 FORMAT_PTR miduser_format,
	 char *header_text,
	 VARIABLE_PTR var,
	 FORMAT_PTR format
	)
{
	int error = 0;

	NAME_TABLE_PTR intable = NULL;
	NAME_TABLE_PTR outtable = NULL;
	char *geovu_name = NULL;

	char *outuser_name = NULL;

	char *miduser_value = NULL;
	FF_TYPES_t miduvalue_type = 0;

	char *outuser_value = NULL;
	FF_TYPES_t outuvalue_type = 0;

	char *geovu_value = NULL;
	FF_TYPES_t gvalue_type = 0;

	double d = 0;
	void *miduvalue_ptr = &d;

	VARIABLE_PTR miduser_var = NULL;

	miduser_var = ff_find_variable(var->name, miduser_format);
	FF_VALIDATE(miduser_var);

	geovu_name = nt_find_geovu_name(dbin, FFF_INPUT, var->name, &intable);
	if (!geovu_name)
		geovu_name = var->name;

	outuser_name = nt_find_user_name(dbin, FFF_OUTPUT, geovu_name, &outtable);
	if (!outuser_name)
		outuser_name = geovu_name;

	miduser_value = (char *)memMalloc(max(FORMAT_LENGTH(miduser_format), MAX_PV_LENGTH), "miduser_value");
	if (!miduser_value)
		return err_push(ERR_MEM_LACK, "");

	outuser_value = (char *)memMalloc(max(max((intable ? FORMAT_LENGTH(intable->format) : 0), (outtable ? FORMAT_LENGTH(outtable->format) : 0)), FORMAT_LENGTH(miduser_format)), "outuser_value");
	if (!outuser_value)
		return err_push(ERR_MEM_LACK, "");

	geovu_value = (char *)memMalloc(max(intable ? FORMAT_LENGTH(intable->format) : 0, FORMAT_LENGTH(miduser_format)), "geovu_value");
	if (!geovu_value)
		return err_push(ERR_MEM_LACK, "");

	miduvalue_type = FFV_DATA_TYPE(var);
	memcpy(miduser_value, header_text + miduser_var->start_pos - 1, FF_VAR_LENGTH(var));

	if (!IS_BINARY(format) || IS_TEXT(miduser_var))
		miduser_value[FF_VAR_LENGTH(miduser_var)] = STR_END;

	if (IS_BINARY(format) || IS_TEXT(miduser_var))
		miduvalue_ptr = miduser_value;
	else
	{
		error = ff_string_to_binary(miduser_value, FFV_DATA_TYPE(var), miduvalue_ptr);
		if (error)
			return error;
	}

	/* miduvalue_ptr now points to either a binary number or a text string */

	if (nt_get_geovu_value(intable, geovu_name, miduvalue_ptr, miduvalue_type, geovu_value, &gvalue_type) == FALSE)
	{
		memcpy(geovu_value, miduvalue_ptr, IS_TEXT_TYPE(miduvalue_type) ? strlen(miduvalue_ptr) + 1 : ffv_type_size(miduvalue_type));

		gvalue_type = miduvalue_type;
	}

	if (nt_get_user_value(outtable, geovu_name, geovu_value, gvalue_type, outuser_value, &outuvalue_type) == FALSE)
	{
		memcpy(outuser_value, geovu_value, IS_TEXT_TYPE(gvalue_type) ? strlen(geovu_value) + 1 : ffv_type_size(gvalue_type));

		outuvalue_type = gvalue_type;
	}

	/* outuser_value is either a binary number or a text string */

	if (!IS_BINARY(format) && !IS_TEXT_TYPE(outuvalue_type))
	{
		error = btype_to_btype(outuser_value, outuvalue_type, &d, FFV_DOUBLE);
		if (!error)
		{
			update_variable_precision(dbin, var);

			error = ff_binary_to_string(&d, FFV_DOUBLE, var->precision, outuser_value);
			if (error)
				return error;

			outuvalue_type = FFV_TEXT;
		}
	}

	new_name_string__(outuser_value, &var->name);

	update_format_var(FFV_CONSTANT, IS_TEXT_TYPE(outuvalue_type) ? strlen(outuser_value) : ffv_type_size(outuvalue_type), var, format);

	memFree(miduser_value, "miduser_value");
	memFree(outuser_value, "outuser_value");
	memFree(geovu_value, "geovu_value");

	return error;
}

static FF_NDX_t calculate_distance
	(
	 DATA_BIN_PTR dbin,
	 FORMAT_PTR format
	)
{
	FF_NDX_t distance = 0;
	VARIABLE_LIST vlist = NULL;
	VARIABLE_PTR var = NULL;

	FF_VALIDATE(dbin);
	FF_VALIDATE(format);

	vlist = FFV_FIRST_VARIABLE(format);
	var = FF_VARIABLE(vlist);
	while (var)
	{
		FF_VALIDATE(var);

		if (IS_PARAM_NAME_VAR(var))
		{
			char inuser_name[MAX_PV_LENGTH];

			char *geovu_name = NULL;
			char *user_name = NULL;

			strncpy(inuser_name, var->name, sizeof(inuser_name) - 1);
			inuser_name[sizeof(inuser_name) - 1] = STR_END;

			assert(!strcmp(inuser_name + strlen(inuser_name) - strlen("_\bname"), "_\bname"));

			inuser_name[strlen(inuser_name) - strlen("_\bname")] = STR_END;

			os_str_trim_whitespace(inuser_name, inuser_name);

			geovu_name = nt_find_geovu_name(dbin, FFF_INPUT, inuser_name, NULL);
			if (!geovu_name)
				geovu_name = inuser_name;

			user_name = nt_find_user_name(dbin, FFF_OUTPUT, geovu_name, NULL);

			if (user_name && !strcmp(user_name, inuser_name))
				distance = max(distance, FF_VAR_LENGTH(var));
			else
			{
				if (!user_name && geovu_name)
					user_name = geovu_name;
				else if (!user_name)
					user_name = inuser_name;

				distance = max(distance, strlen(user_name) + 1);
			}
		}

		vlist = dll_next(vlist);
		var = FF_VARIABLE(vlist);
	}

	return distance;
}

/*****************************************************************************
 * NAME: make_output_header
 *
 * PURPOSE:  Convert header according to name equivalences and constants
 *
 * USAGE:  error = make_output_header(dbin);
 *
 * RETURNS:  Zero on success, an error code on failure
 *
 * DESCRIPTION:  Convert the input parameter value header file to the output
 * parameter value header file.  This allows for input and output name equivalences
 * and translators, and changes in delimiters.  For example:
 *
 * An input header that looks like:
 *
 * data type   : integer
 *
 * with input equivalence section:
 *
 * input_eqv
 * $data_representation data%type
 * text integer text int16
 *
 * and output equivalence section:
 *
 * output_eqv
 * $data_representation fmt
 * text int16 text BI16
 *
 * will result in an output header that looks like:
 *
 * fmt : BI16
 *
 * For DELIM_ITEM and DELIM_VALUE variables in the output header change their names
 * according to the output_eqv constant values for "delimiter_item" and "delimiter_value"
 * or default to native EOL and '=', respectively.  For PARAM_NAME variables change
 * their names according to input_eqv and output_eqv name equivalences:  alias the native
 * variable name according to the input_eqv, and if an alias exists, alias the FreeForm
 * (GeoVu) keyword to the native name given in the output_eqv, it that exists.
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * GLOBALS:
 *
 * COMMENTS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/

static int make_output_header(DATA_BIN_PTR dbin)
{
	FILE *fp = NULL;

	FORMAT_PTR miduser_format = NULL;

	char delim_item[MAX_PV_LENGTH];
	char delim_value[MAX_PV_LENGTH];
	short distance = 0;

	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	VARIABLE_LIST vlist = NULL;
	VARIABLE_PTR var = NULL;

	int error = 0;

	FF_VALIDATE(dbin);

	error = get_output_delims(dbin, delim_item, &distance, delim_value);
	if (error)
		return error;

	error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_OUTPUT | FFF_FILE | FFF_HEADER, &plist);
	if (error)
		return error;

	pinfo = FF_PI(dll_first(plist));

	FF_VALIDATE(pinfo);

	miduser_format = ff_copy_format(PINFO_FORMAT(pinfo));
	if (!miduser_format)
		return ERR_MEM_LACK;

	if (!distance)
		distance = (short)calculate_distance(dbin, miduser_format);

	vlist = FFV_FIRST_VARIABLE(PINFO_FORMAT(pinfo));
	var = FF_VARIABLE(vlist);
	while (var)
	{
		FF_VALIDATE(var);

		if (IS_DELIM_ITEM_VAR(var))
		{
			new_name_string__(delim_item, &var->name);

			update_format_var(FFV_CONSTANT, strlen(var->name), var, PINFO_FORMAT(pinfo));
		}
		else if (IS_DELIM_VALUE_VAR(var))
		{
			new_name_string__(delim_value, &var->name);

			update_format_var(FFV_CONSTANT, strlen(var->name), var, PINFO_FORMAT(pinfo));
		}
		else if (IS_PARAM_NAME_VAR(var))
			change_param_name(dbin, var, distance, PINFO_FORMAT(pinfo));
		else if (IS_PARAM_VALUE_VAR(var))
		{
			error = change_param_value(dbin, miduser_format, PINFO_BUFFER(pinfo), var, PINFO_FORMAT(pinfo));
			if (error)
				break;
		}

		vlist = dll_next(vlist);
		var = FF_VARIABLE(vlist);
	}

	ff_destroy_format(miduser_format);
	miduser_format = NULL;

	error = db_set(dbin, DBSET_INIT_CONDUITS, FFF_OUTPUT | FFF_FILE | FFF_HEADER, 0);
	if (error)
		return error;

	ff_destroy_format_data_mapping(PINFO_FORMAT_MAP(pinfo));
	PINFO_FORMAT_MAP(pinfo) = NULL;

	error = ff_create_format_data_mapping(PINFO_MATE_FD(pinfo), PINFO_FD(pinfo), &PINFO_FORMAT_MAP(pinfo));
	if (error)
		return error;

	PINFO_MATE_NEW_RECORD(pinfo) = TRUE;
	PINFO_NEW_RECORD(pinfo) = FALSE;

	if (!error)
		error = db_do(dbin, DBDO_CONVERT_FORMATS, FFF_FILE | FFF_HEADER);

#ifdef ND_FP 
#error "Need to truncate output header file -- can't do with open file handles"
#endif

	fp = fopen(PINFO_FNAME(pinfo), "w");
	if (!fp)
		err_push(ERR_ASSERT_FAILURE, "Cannot truncate file (%s)", PINFO_FNAME(pinfo));
	else
		fclose(fp);

	if (!error)
		error = db_do(dbin, DBDO_WRITE_FORMATS, FFF_OUTPUT | FFF_FILE | FFF_HEADER);

	ff_destroy_process_info_list(plist);

	return error;
}

#if 0
{
	int error = 0;

	VARIABLE_LIST vlist = NULL;
	VARIABLE_PTR var = NULL;

	FORMAT_DATA_PTR in_fd = fd_get_header(dbin, FFF_INPUT | FFF_FILE);
	FORMAT_DATA_PTR out_fd = fd_get_header(dbin, FFF_OUTPUT | FFF_FILE);

	if (!in_fd || !out_fd)
		return 0;

	FF_VALIDATE(in_fd);
	FF_VALIDATE(out_fd);

	vlist = FFV_FIRST_VARIABLE(out_fd->format);
	var = FF_VARIABLE(vlist);
	while (var)
	{
		FF_VALIDATE(var);

		if (IS_PARAM_VALUE_VAR(var))
		{
			NAME_TABLE_PTR table = NULL;
			char *user_name = nt_find_user_name(dbin, FFF_INPUT, var->name, &table);

			VARIABLE_PTR in_var = ff_find_variable(user_name ? user_name : var->name, in_fd->format);

			char *user_value = NULL;
			char *geovu_value = NULL;

			FF_TYPES_t gvalue_type = 0;

			user_value = (char *)memMalloc(FF_VAR_LENGTH(in_var) + 1, "user_value");
			if (!user_value)
				return err_push(ERR_MEM_LACK, "");

			geovu_value = (char *)memMalloc(table->data->bytes_used, "geovu_value");
			if (!geovu_value)
				return err_push(ERR_MEM_LACK, "");

			memcpy(user_value, in_fd->data->buffer + in_var->start_pos - 1, FF_VAR_LENGTH(in_var));
			if (IS_TEXT(in_var))
				user_value[FF_VAR_LENGTH(in_var)] = STR_END;

			if (nt_get_geovu_value(table, var->name, user_value, FFV_DATA_TYPE(var), geovu_value, &gvalue_type) == FALSE)
			{
				memcpy(geovu_value, in_fd->data->buffer + in_var->start_pos - 1, FF_VAR_LENGTH(in_var));
				if (IS_TEXT(in_var))
					geovu_value[FF_VAR_LENGTH(in_var)] = STR_END;

				gvalue_type = FFV_DATA_TYPE(var);
			}

			memcpy(out_fd->data->buffer + var->start_pos - 1, geovu_value, FFV_DATA_TYPE_TYPE(gvalue_type) ? strlen(geovu_value) : ffv_type_size(gvalue_type));

			memFree(user_value, "user_value");
			memFree(geovu_value, "geovu_value");
		}

		vlist = dll_next(vlist);
		var = FF_VARIABLE(vlist);
	}

	return error;
}
#endif

static int update_output_file_header(DATA_BIN_PTR dbin)
{
	int error = 0;

	char data_type[32];

	FORMAT_DATA_PTR fhd = NULL;
	FORMAT_DATA_PTR rhd = NULL;

	FF_VALIDATE(dbin);

	fhd = fd_get_header(dbin, FFF_OUTPUT | FFF_FILE);
	rhd = fd_get_header(dbin, FFF_OUTPUT | FFF_REC);

	if (!fhd || IS_EMBEDDED(fhd->format) || (rhd && IS_SEPARATE(rhd->format)))
		return 0;

	error = fix_header(dbin);
	if (error)
		return error;

	error = set_var_minmaxs(dbin);
	if (error)
		return error;

	error = nt_ask(dbin, NT_INPUT, "data_type", FFV_TEXT, data_type);
	if (error && error != ERR_NT_KEYNOTDEF)
		error = err_push(ERR_PARAM_VALUE, "for data_type (%s)", data_type);
	else if (!error)
	{
		if (!os_strcmpi(data_type, "image") || !os_strcmpi(data_type, "raster"))
			error = update_image_info(dbin);
	}
	else if (error == ERR_NT_KEYNOTDEF)
		error = 0;

	error = make_output_header(dbin);
	if (error)
		return (error);

	return error;
}

static void print_maxmins
	(
	 DATA_BIN_PTR dbin,
	 FF_BUFSIZE_PTR newform_log,
	 FILE *to_user
	)
{
	int error = 0;

	double minimum = 0;
	double maximum = 0;

	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	FF_VALIDATE(dbin);
	if (newform_log)
		FF_VALIDATE(newform_log);

	error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_OUTPUT | FFF_DATA, &plist);
	if (!error)
	{
		plist = dll_first(plist);
		pinfo = FF_PI(plist);
		while (!error && pinfo)
		{
			VARIABLE_LIST vlist = NULL;
			VARIABLE_PTR var = NULL;

			FF_VALIDATE(PINFO_FORMAT(pinfo));

			vlist = FFV_FIRST_VARIABLE(PINFO_FORMAT(pinfo));
			var = FF_VARIABLE(vlist);
			while (var)
			{
				FF_VALIDATE(var);

				if (!IS_EOL(var))
				{
					minimum = mm_getmn(var);
					maximum = mm_getmx(var);
					
					if (IS_INTEGER(var))
					{
						minimum *= pow(10.0, -var->precision);
						maximum *= pow(10.0, -var->precision);
					}

					if (newform_log)
					{
						if (IS_TEXT(var) || IS_CONSTANT(var) || IS_INITIAL(var))
						{
							do_log(newform_log, "%s: minimum: %s\n", var->name, (char *)((long)(minimum)));
							do_log(newform_log, "%s: maximum: %s\n", var->name, (char *)((long)(maximum)));
						}
						else
						{
							int col1_len = 0;

							if (var->misc.mm->max_flag)
							{
								double flag = 0;

								btype_to_btype(var->misc.mm->max_flag, FFV_DATA_TYPE(var), &flag, FFV_DOUBLE);

								if (IS_INTEGER(var))
									flag *= pow(10, -var->precision);

								col1_len = (int)(flag ? log10(fabs(flag)) : 0) + 1 + (flag < 0 ? 1 : 0);
						
								do_log(newform_log, "Ignoring %s data flag value: %*.*f\n", var->name, (int)col1_len, (int)(var->precision), flag);
							}

							col1_len = (int)(minimum ? log10(fabs(minimum)) : 0) + 1 + (minimum < 0 ? 1 : 0);
							col1_len = max(col1_len, (int)(maximum ? log10(fabs(maximum)) : 0) + 1 + (maximum < 0 ? 1 : 0));
							
							if (var->precision)
								col1_len += var->precision + 1;

							do_log(newform_log, "%s: minimum: %*.*f\n",
									 var->name, (int)col1_len, (int)(var->precision), minimum);
							do_log(newform_log, "%s: maximum: %*.*f\n",
									 var->name, (int)col1_len, (int)(var->precision), maximum);
						}
					}
					else if (to_user)
					{
						if (IS_TEXT(var) || IS_CONSTANT(var) || IS_INITIAL(var))
						{
							wfprintf(to_user, "%s: minimum: %s\n", var->name, (char *)((long)(minimum)));
							wfprintf(to_user, "%s: maximum: %s\n", var->name, (char *)((long)(maximum)));
						}
						else
						{
							int col1_len = 0;

							if (var->misc.mm->max_flag)
							{
								double flag = 0;

								btype_to_btype(var->misc.mm->max_flag, FFV_DATA_TYPE(var), &flag, FFV_DOUBLE);

								if (IS_INTEGER(var))
									flag *= pow(10, -var->precision);

								col1_len = (int)(flag ? log10(fabs(flag)) : 0) + 1 + (flag < 0 ? 1 : 0);
						
								do_log(newform_log, "Ignoring %s data flag value: %*.*f\n", var->name, (int)col1_len, (int)(var->precision), flag);
							}

							col1_len = (int)(minimum ? log10(fabs(minimum)) : 0) + 1 + (minimum < 0 ? 1 : 0);
							col1_len = max(col1_len, (int)(maximum ? log10(fabs(maximum)) : 0) + 1 + (maximum < 0 ? 1 : 0));
							
							if (var->precision)
								col1_len += var->precision + 1;
						
							wfprintf(to_user, "%s: minimum: %*.*f\n",
									 var->name, (int)col1_len, (int)(var->precision), minimum);
							wfprintf(to_user, "%s: maximum: %*.*f\n",
									 var->name, (int)col1_len, (int)(var->precision), maximum);
						}
					}
				}

				vlist = dll_next(vlist);
				var = FF_VARIABLE(vlist);
			}

			plist = dll_next(plist);
			pinfo = FF_PI(plist);
		}

		ff_destroy_process_info_list(plist);
	}
}

static void remove_orphan_input_data(DATA_BIN_PTR dbin)
{
	FF_ARRAY_CONDUIT_LIST array_conduit_list = NULL;
	FF_ARRAY_CONDUIT_PTR array_conduit = NULL;

	FF_VALIDATE(dbin);

	array_conduit_list = dll_first(dbin->array_conduit_list);
	array_conduit = FF_AC(array_conduit_list);
	while (array_conduit)
	{
		FF_VALIDATE(array_conduit);

 		array_conduit_list = dll_next(array_conduit_list);

		if (array_conduit->input && IS_DATA(array_conduit->input->fd->format) && IS_ARRAY(array_conduit->input->fd->format) && !array_conduit->output)
			dll_delete(dll_previous(array_conduit_list));

		array_conduit = FF_AC(array_conduit_list);
	}
}

 
#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "Newform"

/*****************************************************************************
 * NAME:  newform()
 *
 * PURPOSE:  To change the format of a data set.
 *
 * USAGE:  error = newform(FFF_STD_ARGS_PTR pstd_args);
 *
 * RETURNS:  Zero on success, otherwise an error code defined in err.h or errno.h
 *
 * DESCRIPTION:  newform takes an allocated and filled std_args structure
 * containing at a minimum the data file name to process.  Other fields, if
 * left initialized (zero for integer variables and NULL for char pointers)
 * will result in the following:  Default searches for format files,
 * local_buffer (the data bin scratch buffer) allocated with a size of
 * DEFAULT_BUFFER_SIZE, and no variable thinning on output, no queries, and
 * no initial or trailing records to skip.
 *
 * In any event, local_buffer should never be allocated by the calling routine.
 * Also, while the user has ability to change the local_buffer size, this
 * should probably never actually be done, as newform() changes local_buffer_size
 * to LOCAL_BUFFER_SIZE (32K) but only if local_buffer_size has been set to
 * DEFAULT_BUFFER_SIZE (8K).
 *
 * AUTHOR:  Ted Habermann, NGDC, (303)497-6472, haber@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * GLOBALS:
 *
 * COMMENTS:  If GeoVu is calling NEWFORM (as determined by FF_MAIN not being defined
 * but TEST_SUITE is not) then the menu information associated with the current
 * DATA SOURCE (as contained in the data bin string data base) gets grafted
 * into the "input" data bin.  I'm assuming that the only way a newform()-
 * generated data bin can get the menu information is by such a graft.  So,
 * before I call db_free(input), I always set input->strdb to NULL, so that
 * db_free() cannot possibly damage the menu information associated with the
 * GeoVu-generated data bin.
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/

int newform
	(
	 FF_STD_ARGS_PTR std_args,
	 FF_BUFSIZE_PTR newform_log,
	 FILE *to_user
	)
{
	char *greeting = 
{
"\nWelcome to Newform release "FFND_LIB_VER" -- an NGDC FreeForm ND application\n\n"
};

	char *command_line_usage = {
"Several Newform command line elements have default extensions:\n\
data_file: .bin = binary  .dat = ASCII  .dab = flat ASCII\n\
input/output_format_file: .fmt = format description file\n\
Newform [-d data_file] [-f format_file] [-if input_format_file]\n\
        [-of output_format_file] [-ft \"format title\"]\n\
        [-ift \"input format title\"] [-oft \"output format title\"]\n\
        [-b cache_size] Sets the input data cache size in bytes\n\
        [-c count] No. records to process at head(+)/tail(-) of file\n\
        [-q query] Output records matching criteria in query file\n\
        [-v var] Process variables listed in var file, default = all\n\
        [-o output_file] default = output to screen\n\
        [-el error_log_file] error messages also go to error_log_file\n\
        [-ep] passive error messages - non-interactive\n\
        [-ol log] File to contain log of processing information\n\
        [-mm] Output min/max values\n\n\
See the FreeForm User's Guide for detailed information.\n"
};

	int error = 0;
	int percent_done = 0;

	long bytes_remaining = 0;
	long total_bytes = 0L;

	BOOLEAN done_processing = FALSE;

	DATA_BIN_PTR dbin = NULL;
	FF_BUFSIZE_PTR bufsize = NULL;

#ifdef TIMER
	long elapsed_time = 0;
	time_t finish_time = 0;

	time_t start_time;
	(void)time(&start_time);
#endif

	if (newform_log)
		do_log(newform_log, "%s", greeting);
	else
		wfprintf(to_user, "%s", greeting);

	/* Check number of command args */
	if (std_args->input_file == NULL && std_args->input_bufsize == NULL && !std_args->user.is_stdin_redirected)
	{
		wfprintf(to_user, "%s", command_line_usage);

		return 0;
	}

	error = db_init(std_args, &dbin, NULL);
	if (error && error < ERR_WARNING_ONLY)
	{
		db_destroy(dbin);

		return error;
	}
	else if (error)
		error = 0;

	remove_orphan_input_data(dbin);

	if (std_args->user.is_stdin_redirected)
	{
		error = db_set(dbin, DBSET_SETUP_STDIN, std_args);
		if (error)
		{
			db_destroy(dbin);
			return error;
		}
	}

	if (std_args->user.is_stdout_redirected)
	{
#if FF_OS == FF_OS_DOS || FF_OS == FF_OS_MACOS
		setmode(fileno(stdout), O_BINARY);
#endif
		error = db_do(dbin, DBDO_CHECK_STDOUT);
		if (error)
		{
			db_destroy(dbin);
			return error;
		}
	}

	if (std_args->query_file)
	{
		if (newform_log)
			do_log(newform_log, "Using query file: %s\n", std_args->query_file);
		else
			wfprintf(to_user, "Using query file: %s\n", std_args->query_file);
	}

	/* Display some information about the data, ignore errors */
	db_ask(dbin, DBASK_FORMAT_SUMMARY, 0, &bufsize);
	if (newform_log)
		do_log(newform_log, "%s\n", bufsize->buffer);
	else
		wfprintf(to_user, "%s\n", bufsize->buffer);

	ff_destroy_bufsize(bufsize);

	while (!error && !done_processing)
	{
		error = db_do(dbin, DBDO_PROCESS_FORMATS, FFF_INPUT);
		if (error == EOF)
		{
			error = 0;
			done_processing = TRUE;
		}

		if (!error)
			error = db_do(dbin, DBDO_WRITE_FORMATS, FFF_OUTPUT);

		/* Calculate percentage left to process and display */
		total_bytes = newform_total_bytes_to_process(dbin);

		bytes_remaining = db_ask(dbin, DBASK_BYTES_TO_PROCESS, FFF_OUTPUT);

		percent_done = (int)((1 - ((float)bytes_remaining / total_bytes)) * 100);

		if (!newform_log)
		{
			wfprintf(to_user,"\r%3d%% processed", percent_done);

#ifdef TIMER
			(void)time(&finish_time);
			elapsed_time = (long)difftime(finish_time, start_time);
			wfprintf(to_user, "     Elapsed time - %02d:%02d:%02d",
			        (int)(elapsed_time / (3600)),
			        (int)((elapsed_time / 60) % 60),
			        (int)(elapsed_time % 60)
			);
#endif /* TIMER */
		}

		if (!error && std_args->user.is_stdin_redirected)
			error = db_do(dbin, DBDO_READ_STDIN, std_args, &done_processing);

		if (!error && !done_processing && std_args->user.is_stdin_redirected)
		{
			FF_BUFSIZE_PTR bufsize = NULL;

			/* Display some information about the data, ignore errors */
			db_ask(dbin, DBASK_FORMAT_SUMMARY, 0, &bufsize);

			if (newform_log)
				do_log(newform_log, "%s\n", bufsize->buffer);
			else
				wfprintf(to_user, "%s\n", bufsize->buffer);

			ff_destroy_bufsize(bufsize);
		}
	}
	
	/* End Processing */
	
	if (!error || error > ERR_WARNING_ONLY)
	{
		error = update_output_file_header(dbin);
		if (error == ERR_GENERAL)
			error = 0;
	}

	if (!error || error > ERR_WARNING_ONLY)
		error = write_output_format_file(dbin, std_args);

	if (std_args->cv_maxmin_only)
		print_maxmins(dbin, newform_log, to_user);

	bytes_remaining = db_ask(dbin, DBASK_BYTES_TO_PROCESS, FFF_OUTPUT);
	if (bytes_remaining)
		error = err_push(ERR_PROCESS_DATA + ERR_WARNING_ONLY, "%ld BYTES OF DATA NOT WRITTEN.", bytes_remaining);

	db_destroy(dbin);

#ifdef TIMER
	(void)time(&finish_time);
	elapsed_time = (long)difftime(finish_time, start_time);
	if (newform_log)
	{
		do_log(newform_log, "%3d%% processed     Total elapsed time - %02d:%02d:%02d\n",
		       percent_done,
		       (int)(elapsed_time / (3600)),
		       (int)((elapsed_time / 60) % 60),
		       (int)(elapsed_time % 60)
		      );
	}
	else
	{
		wfprintf(to_user, "\r%3d%% processed     Total elapsed time - %02d:%02d:%02d\n",
		         percent_done,
		         (int)(elapsed_time / (3600)),
		         (int)((elapsed_time / 60) % 60),
		         (int)(elapsed_time % 60)
		        );
	}
#endif /* TIMER */
	
	return(error);
} /* newform */

static void hackmerge_input_data_formats(FF_BUFSIZE_PTR bufsize)
{
	char *first = NULL;
	char *last = NULL;

	char *tag[3];

	int tag_no = 0;

	FF_VALIDATE(bufsize);

	tag[0] = ff_lookup_string(format_types, FFF_BINARY | FFF_INPUT | FFF_DATA);
	tag[1] = ff_lookup_string(format_types, FFF_ASCII | FFF_INPUT | FFF_DATA);
	tag[2] = ff_lookup_string(format_types, FFF_FLAT | FFF_INPUT | FFF_DATA);

	first = strstr(bufsize->buffer, tag[tag_no]);
	if (!first)
		first = strstr(bufsize->buffer, tag[++tag_no]);
	if (!first)
		first = strstr(bufsize->buffer, tag[++tag_no]);

	last = os_strrstr(bufsize->buffer, tag[tag_no]);
	while ((char HUGE *)last > (char HUGE *)first)
	{
		*last = '/';
		last = os_strrstr(bufsize->buffer, tag[tag_no]);
	}
}

static void hackmerge_output_data_formats(FF_BUFSIZE_PTR bufsize)
{
	char *first = NULL;
	char *last = NULL;

	FF_VALIDATE(bufsize);

	first = strstr(bufsize->buffer, "binary_output_data");
	last = os_strrstr(bufsize->buffer, "binary_output_data");
	while ((char HUGE *)last > (char HUGE *)first)
	{
		*last = '/';
		last = os_strrstr(bufsize->buffer, "binary_output_data");
	}
}

static void remove_eqn_vars(FORMAT_PTR format)
{
	VARIABLE_LIST vlist = NULL;
	VARIABLE_PTR var = NULL;

	FF_VALIDATE(format);

	vlist = FFV_FIRST_VARIABLE(format);
	var = FF_VARIABLE(vlist);
	while (var)
	{
		FF_VALIDATE(var);

		if (IS_EQN(var))
		{
			char *cp = NULL;

			cp = strstr(var->name, "_eqn");
			assert(cp && strlen(cp) == 4);
			
			*cp = STR_END;

			var->type &= ~FFV_EQN;

			vlist = dll_next(vlist);
			var = FF_VARIABLE(vlist);
		}

		vlist = dll_next(vlist);
		var = FF_VARIABLE(vlist);
	}
}

static void remove_eqn_companion_vars(FORMAT_PTR format)
{
	VARIABLE_LIST vlist = NULL;
	VARIABLE_PTR var = NULL;

	FF_VALIDATE(format);

	vlist = FFV_FIRST_VARIABLE(format);
	var = FF_VARIABLE(vlist);
	while (var)
	{
		FF_VALIDATE(var);

		if (IS_EQN(var))
		{
			char *cp = NULL;

#ifndef NDEBUG
			VARIABLE_PTR var2 = FF_VARIABLE(dll_next(vlist));
#endif

			FF_VALIDATE(var2);

			cp = strstr(var->name, "_eqn");
			assert(cp && strlen(cp) == 4);
			*cp = STR_END;

			assert(!strcmp(var2->name, var->name));

			*cp = '_';

			dll_delete_node(dll_next(vlist));
			--format->num_vars;

			vlist = dll_next(vlist);
			var = FF_VARIABLE(vlist);
		}

		vlist = dll_next(vlist);
		var = FF_VARIABLE(vlist);
	}
}

static BOOLEAN input_eqn_vars(DATA_BIN_PTR dbin)
{
	BOOLEAN eqn_vars_found = FALSE;

	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	FF_VALIDATE(dbin);

	db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT, &plist);

	plist = dll_first(plist);
	pinfo = FF_PI(plist);
	while (pinfo)
	{
		unsigned int old_num_vars = 0;

		FF_VALIDATE(pinfo);

		old_num_vars = PINFO_FORMAT(pinfo)->num_vars;

		remove_eqn_companion_vars(PINFO_FORMAT(pinfo));

		if (old_num_vars != PINFO_FORMAT(pinfo)->num_vars)
			eqn_vars_found = TRUE;

		plist = dll_next(plist);
		pinfo = FF_PI(plist);
	}

	ff_destroy_process_info_list(plist);

	return eqn_vars_found;
}

static int transmogrify
	(
	 FF_STD_ARGS_PTR std_args,
	 FF_BUFSIZE_PTR checkvar_log,
	 DATA_BIN_PTR dbin,
	 char **tmpfiles
	)
{
	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	FF_BUFSIZE_PTR bufsize = NULL;
	FORMAT_PTR binary_format = NULL;

	char *save_var_file = NULL;
	BOOLEAN save_cv_maxmin_only = FALSE;

	enum {NO_REASON = 0, BECAUSE_REC_HEADERS = 1, BECAUSE_QUERY = 2, BECAUSE_CONVERT = 4, BECAUSE_EQN = 8} do_newform = NO_REASON;

	int error = 0;

	NAME_TABLE_PTR table = NULL;

	if (checkvar_log)
		FF_VALIDATE(checkvar_log);

	bufsize = ff_create_bufsize(LOGGING_QUANTA);
	if (!bufsize)
		return ERR_MEM_LACK;

	error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT | FFF_REC | FFF_HEADER, &plist);
	if (error && error != ERR_GENERAL)
		goto transmogrify_exit;
	else if (!error)
	{
		do_newform = BECAUSE_REC_HEADERS;

		ff_destroy_process_info_list(plist);
		plist = NULL;
	}

	if (std_args->query_file)
		do_newform += BECAUSE_QUERY;

	if (std_args->cv_subset)
	{
		error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_OUTPUT | FFF_DATA, &plist);
		if (!error)
		{
			VARIABLE_LIST vlist = NULL;
			VARIABLE_PTR var = NULL;

			pinfo = FF_PI(dll_first(plist));

			FF_VALIDATE(pinfo);

			vlist = FFV_FIRST_VARIABLE(PINFO_FORMAT_MAP(pinfo)->middle->format);
			var = FF_VARIABLE(vlist);
			while (var)
			{
				FF_VALIDATE(var);

				if (IS_CONVERT(var))
				{
					do_newform += BECAUSE_CONVERT;

					break;
				}

				vlist = dll_next(vlist);
				var = FF_VARIABLE(vlist);
			}

			ff_destroy_process_info_list(plist);
			plist = NULL;
		}
	}

	if (input_eqn_vars(dbin))
		do_newform += BECAUSE_EQN;

	if (do_newform)
	{
		db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT, &plist);
		plist = dll_first(plist);
		pinfo = FF_PI(plist);
		while (pinfo)
		{
			error = db_ask(dbin, DBASK_FORMAT_DESCRIPTION, PINFO_FIRST_ARRAY_OFFSET(pinfo), PINFO_FORMAT(pinfo), bufsize);
			if (error)
				goto transmogrify_exit;

			if (!IS_REC_HEADER(PINFO_FORMAT(pinfo)))
			{
				/* Not necessarily a binary format... */
				binary_format = ff_copy_format((do_newform & (BECAUSE_CONVERT | BECAUSE_EQN)) && std_args->cv_subset && IS_DATA(PINFO_FORMAT(pinfo)) ? PINFO_MATE_FORMAT(pinfo) : PINFO_FORMAT(pinfo));

				binary_format->type &= ~FFF_INPUT;
				binary_format->type |= FFF_OUTPUT;

				if ((do_newform & BECAUSE_EQN) && !std_args->cv_subset)
					remove_eqn_vars(binary_format);

				error = db_ask(dbin, DBASK_FORMAT_DESCRIPTION, 0, binary_format, bufsize);
				if (error)
					goto transmogrify_exit;

				ff_destroy_format(binary_format);
				binary_format = NULL;
			}

			plist = dll_next(plist);
			pinfo = FF_PI(plist);
		}

		ff_destroy_process_info_list(plist);
		plist = NULL;

		table = fd_find_format_data(dbin->table_list, FFF_GROUP, FFF_TABLE | FFF_INPUT);
		if (table)
		{
			FF_VALIDATE(table);

			strcpy(bufsize->buffer + bufsize->bytes_used, "input_eqv\n");
			bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

			error = nt_show(table, bufsize);
			if (error)
				return error;

			strcpy(bufsize->buffer + bufsize->bytes_used, "\n");
			bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);
		}

		static char tmpnam_replacement[] = {"/tmp/freeformXXXXXX"};
		if (mkstemp(tmpnam_replacement) > -1)
		    std_args->output_file = tmpnam_replacement;
		else
		    std_args->output_file = 0;
#if 0
		// I got tired of gcc's warnings... my fix is not
		// thread-safe. 05/14/03 jhrg
		std_args->output_file = tmpnam(NULL);
#endif
		if (!std_args->output_file)
		{
			error = err_push(ERR_CREATE_FILE, "Unable to create a temporary file");
			goto transmogrify_exit;
		}

		std_args->input_format_file = NULL;
		std_args->output_format_file = NULL;
		std_args->input_format_title = NULL;
		std_args->output_format_title = NULL;

		std_args->input_format_buffer = bufsize->buffer;
		std_args->output_format_buffer = bufsize->buffer;

		save_var_file = std_args->var_file;
		std_args->var_file = NULL;

		save_cv_maxmin_only = std_args->cv_maxmin_only;
		std_args->cv_maxmin_only = FALSE;

		do_log(checkvar_log, "Using Newform to create a temporary file ");
		if (do_newform & BECAUSE_REC_HEADERS)
			do_log(checkvar_log, "without record headers");

		if (do_newform & BECAUSE_QUERY)
			do_log(checkvar_log, "%sto match query", (do_newform & BECAUSE_REC_HEADERS) ? "and " : "");

		if (do_newform & BECAUSE_CONVERT)
			do_log(checkvar_log, "%sto process conversion variables", (do_newform & (BECAUSE_REC_HEADERS | BECAUSE_QUERY)) ? "and " : "");

		if (do_newform & BECAUSE_EQN)
			do_log(checkvar_log, "%sto process equation variables",  (do_newform & (BECAUSE_REC_HEADERS | BECAUSE_QUERY |BECAUSE_CONVERT)) ? "and " : "");

		do_log(checkvar_log, "...\n");

		error = newform(std_args, checkvar_log, stderr);
		if (error && error < ERR_WARNING_ONLY)
		{
			err_push(ERR_GENERAL, "Problems converting data with newform");
			goto transmogrify_exit;
		}

		do_log(checkvar_log, "\nNewform processing complete.  Returning you now to Checkvar...\n");

		std_args->var_file = save_var_file;
		std_args->query_file = NULL;
		std_args->cv_maxmin_only = save_cv_maxmin_only;

		bufsize->buffer[0] = STR_END;
		bufsize->bytes_used = 0;

		os_path_get_parts(std_args->output_file, *tmpfiles, *(tmpfiles + 1), NULL);
		os_path_put_parts(*tmpfiles, *tmpfiles, *(tmpfiles + 1), "fmt");
		++tmpfiles;

		if (fd_get_header(dbin, FFF_SEPARATE))
		{
			os_path_get_parts(std_args->output_file, *tmpfiles, *(tmpfiles + 1), NULL);
			os_path_put_parts(*tmpfiles, *tmpfiles, *(tmpfiles + 1), "hdr");
			++tmpfiles;
		}

		strcpy(*tmpfiles, std_args->input_file = std_args->output_file);
		tmpfiles++;

		std_args->output_file = NULL;
	} /* if do_newform */

	*tmpfiles = NULL;

	/* Ensure a binary output format */

	error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT | FFF_DATA, &plist);
	if (error)
	{
		err_push(ERR_GENERAL, "Cannot proceed without an input data format");
		goto transmogrify_exit;
	}

	plist = dll_first(plist);
	pinfo = FF_PI(plist);
	while (pinfo)
	{
		FF_VALIDATE(pinfo);

		binary_format = NULL;
		/* Ensure a binary output data format */

		if (std_args->cv_subset && PINFO_MATE(pinfo))
			binary_format = PINFO_MATE_FORMAT(pinfo);
		else
			binary_format = PINFO_FORMAT(pinfo);

		remove_eqn_vars(binary_format);

		if (IS_BINARY(binary_format))
		{
			FF_VALIDATE(binary_format);

			binary_format = ff_copy_format(binary_format);
		}
		else
		{
			binary_format = ff_afm2bfm(binary_format, binary_format->name);
			if (!binary_format)
				return err_push(ERR_GENERAL, "Could not convert to binary");
		}

		binary_format->type &= ~FFF_INPUT;
		binary_format->type |= FFF_OUTPUT;

		error = db_ask(dbin, DBASK_FORMAT_DESCRIPTION, 0, binary_format, bufsize);
		if (error)
			goto transmogrify_exit;

		ff_destroy_format(binary_format);
		binary_format = NULL;

		plist = dll_next(plist);
		pinfo = FF_PI(plist);
	}

	ff_destroy_process_info_list(plist);
	plist = NULL;

	hackmerge_output_data_formats(bufsize);

	db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT, &plist);

	plist = dll_first(plist);
	pinfo = FF_PI(plist);
	while (pinfo)
	{
		if (!IS_REC_HEADER(PINFO_FORMAT(pinfo)))
		{
			/* Not necessarily a binary format... */
			binary_format = ff_copy_format((do_newform & BECAUSE_CONVERT) && IS_DATA(PINFO_FORMAT(pinfo)) ? PINFO_MATE_FORMAT(pinfo) : PINFO_FORMAT(pinfo));

			binary_format->type &= ~FFF_OUTPUT;
			binary_format->type |= FFF_INPUT;

			error = db_ask(dbin, DBASK_FORMAT_DESCRIPTION, 0, binary_format, bufsize);
			if (error)
				goto transmogrify_exit;

			ff_destroy_format(binary_format);
			binary_format = NULL;
		}

		plist = dll_next(plist);
		pinfo = FF_PI(plist);
	}

	ff_destroy_process_info_list(plist);
	plist = NULL;

	hackmerge_input_data_formats(bufsize);

	std_args->input_format_file = NULL;
	std_args->output_format_file = NULL;
	std_args->input_format_title = NULL;
	std_args->output_format_title = NULL;

	std_args->input_format_buffer = bufsize->buffer;
	std_args->output_format_buffer = bufsize->buffer;

	ff_destroy_array_conduit_list(dbin->array_conduit_list);
	dbin->array_conduit_list = NULL;

	table = fd_find_format_data(dbin->table_list, FFF_GROUP, FFF_TABLE | FFF_INPUT);
	if (table)
	{
		FF_VALIDATE(table);

		strcpy(bufsize->buffer + bufsize->bytes_used, "input_eqv\n");
		bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

		error = nt_show(table, bufsize);
		if (error)
			return error;

		strcpy(bufsize->buffer + bufsize->bytes_used, "\n");
		bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);
	}

	if (dbin->table_list)
	{
		fd_destroy_format_data_list(dbin->table_list);
		dbin->table_list = NULL;
	}

	error = db_init(std_args, &dbin, NULL);
	if (error)
		goto transmogrify_exit;

	bufsize->bytes_used = 0;
	bufsize->buffer[0] = STR_END;

	/* Make BIP-type interleaved arrays to emulate single-variable data views */
	db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT | FFF_DATA, &plist);

	plist = dll_first(plist);
	pinfo = FF_PI(plist);

	if (!IS_ARRAY(PINFO_FORMAT(pinfo)))
	{
		long save_records_to_read = 0;

		ff_destroy_process_info_list(plist);
		plist = NULL;

		error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT | FFF_FILE | FFF_HEADER, &plist);
		if (!error)
		{
			plist = dll_first(plist);
			pinfo = FF_PI(plist);
			while (pinfo)
			{
				error = db_ask(dbin, DBASK_FORMAT_DESCRIPTION, PINFO_FIRST_ARRAY_OFFSET(pinfo), PINFO_FORMAT(pinfo), bufsize);
				if (error)
					goto transmogrify_exit;

				plist = dll_next(plist);
				pinfo = FF_PI(plist);
			}
		}

		error = db_ask(dbin, DBASK_TAB_TO_ARRAY_FORMAT_DESCRIPTION, bufsize);
		if (error)
			goto transmogrify_exit;

		std_args->input_format_file = NULL;
		std_args->output_format_file = NULL;
		std_args->input_format_title = NULL;
		std_args->output_format_title = NULL;

		std_args->input_format_buffer = bufsize->buffer;
		std_args->output_format_buffer = bufsize->buffer;

		ff_destroy_array_conduit_list(dbin->array_conduit_list);
		dbin->array_conduit_list = NULL;

		std_args->var_file = NULL;

		save_records_to_read = std_args->records_to_read;
		std_args->records_to_read = 0;

		table = fd_find_format_data(dbin->table_list, FFF_GROUP, FFF_TABLE | FFF_INPUT);
		if (table)
		{
			FF_VALIDATE(table);

			strcpy(bufsize->buffer + bufsize->bytes_used, "input_eqv\n");
			bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

			error = nt_show(table, bufsize);
			if (error)
				return error;

			strcpy(bufsize->buffer + bufsize->bytes_used, "\n");
			bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);
		}

		if (dbin->table_list)
		{
			fd_destroy_format_data_list(dbin->table_list);
			dbin->table_list = NULL;
		}

		error = db_init(std_args, &dbin, NULL);
		std_args->records_to_read = save_records_to_read;
		if (error)
			goto transmogrify_exit;
	}

transmogrify_exit:

	if (binary_format)
		ff_destroy_format(binary_format);

	if (bufsize)
		ff_destroy_bufsize(bufsize);

	if (plist)
		ff_destroy_process_info_list(plist);

	return error;
}

static int get_name_number
	(
	 char *name,
	 int total,
	 char **names
	)
{
	int i = 0;

	i = 0;
	while (i < total && strcmp(name, os_strrstr(names[i], "::") + 2))
		i++;

	if (i == total)
		err_push(ERR_ASSERT_FAILURE, "in get_name_nubmer");

	return i;
}

static int setup_lists
	(
	 DATA_BIN_PTR dbin,
	 FF_BUFSIZE_PTR checkvar_log,
	 int maxmin_only,
	 char *list_file_dir,
	 PROCESS_INFO_LIST_HANDLE hplist,
	 FF_DATA_FLAG_LIST_HANDLE hdata_flag_list
	)
{
	int error = 0;
	PROCESS_INFO_PTR pinfo = NULL;
	FF_DATA_FLAG_PTR data_flag = NULL;

	VARIABLE_PTR var = NULL;

	char file_name[MAX_NAME_LENGTH + 16];

	int num_names = 0;
	int name_number = 0;
	char **var_names = 0;
	double **var_flags = NULL;

	FF_VALIDATE(dbin);

	if (checkvar_log)
		FF_VALIDATE(checkvar_log);

	error = db_ask(dbin, DBASK_VAR_NAMES, FFF_INPUT | FFF_DATA, &num_names, &var_names);
	if (error)
		return err_push(ERR_GENERAL, "Cannot get variable's names");

	error = db_ask(dbin, DBASK_VAR_FLAGS, FFV_DOUBLE, num_names, var_names, &var_flags);
	if (error)
		return err_push(ERR_GENERAL, "Cannot get variable's data flags");

	/* LOOP THE LIST OF VIEWS */
	error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_OUTPUT | FFF_DATA, hplist);
	if (error)
		return err_push(ERR_GENERAL, "Cannot get list of variables");

	*hdata_flag_list = dll_init();
	if (!*hdata_flag_list)
		return err_push(ERR_MEM_LACK, "");

	*hplist = dll_first(*hplist);
	pinfo = FF_PI(*hplist);
	while (pinfo)
	{
		var = FF_VARIABLE(FFV_FIRST_VARIABLE(PINFO_FORMAT(pinfo)));
		FF_VALIDATE(var);

		/* CREATE AN OUTPUT FILE FOR EACH VARIABLE */
		(void)strcpy(file_name, var->name);
		
		if (!maxmin_only)
		{
#if FF_OS == FF_OS_DOS
			PROCESS_INFO_LIST collider_list = NULL;
			VARIABLE_PTR collider = NULL;

			int occurrence = 0;
			short num_char = 0;

			/* Check for duplicate variable file names */
			/* Repeat occurrences are checked against the format list -- no matter if
		   	  a variable file exists or not. The decision to go this way was made to
		   	  guarantee no duplicate file names (although the occurrence counter will
		   	  not always match up with repeated variables in a variable file).
              
		   	  ie. 
			  If you did checkvar on the same file with two different variable files,
			  you may end up with duplicate files -- if the occurrence counter was
			  based solely on the variable file
			*/
			num_char = (short)min(strlen(var->name), 8);
			occurrence = 0;
		
			collider_list = dll_previous(*hplist);
			while (FF_PI(collider_list))
			{
				collider = FF_VARIABLE(FFV_FIRST_VARIABLE(PINFO_FORMAT(FF_PI(collider_list))));

				if (num_char == (short)min(strlen(collider->name),8) )
				{
					if (strncmp(var->name, collider->name, num_char) == 0)
						++occurrence;
				}
				collider_list = dll_previous(collider_list);
			}

			/* Surely there won't be more than 99 variables with the 
		      first 8 characters the same  :- */
			if (occurrence > 9)
				sprintf((file_name + num_char - 2), "%d", occurrence);
			else if (occurrence)
				sprintf((file_name + num_char - 1), "%d", occurrence);

			/* Truncate the file name at num_char characters */
			*(file_name + num_char) = '\0';
		
#endif
			if (IS_TEXT(var) || IS_CONSTANT(var) || IS_INITIAL(var))
				(void)strcat(file_name, ".cst");
			else
				(void)strcat(file_name, ".lst");

		} /* end if (!maxmin_only) */
		
		/* Prepend file_name with the output summary file directory */
		if (list_file_dir)
			os_path_put_parts(file_name, list_file_dir, file_name, NULL);

		memFree(pinfo->name, "pinfo->name");
		pinfo->name = memStrdup(file_name, "file_name");
		if (!pinfo->name)
			return err_push(ERR_MEM_LACK, "");

		data_flag = ff_create_data_flag();
		if (!data_flag)
			return ERR_MEM_LACK;

		if (!dll_add(*hdata_flag_list))
			return ERR_MEM_LACK;

		dll_assign(data_flag, DLL_DF, dll_last(*hdata_flag_list));

		data_flag->value_exists = 0;
		data_flag->value = 0;
		data_flag->var = var;
		
		name_number = get_name_number(var->name, num_names, var_names);
		/* Set the missing data flag for this variable */
		if (var_flags[name_number])
		{
			double lower_bound = 0;
			double upper_bound = 0;

			assert(!strcmp(var->name, os_strrstr(var_names[name_number], "::") + 2));

			switch (FFV_DATA_TYPE(var))
			{
				double temp;

				case FFV_TEXT:
					/* What to do? */
				break;

				case FFV_INT8:
				case FFV_UINT8:
				case FFV_INT16:
				case FFV_UINT16:
				case FFV_INT32:
				case FFV_UINT32:
				case FFV_INT64:
				case FFV_UINT64:
					temp = *var_flags[name_number] * pow(10, var->precision);

					btype_to_btype(&temp, FFV_DOUBLE, &lower_bound, FFV_DATA_TYPE(var));
					btype_to_btype(&temp, FFV_DOUBLE, &upper_bound, FFV_DATA_TYPE(var));
				break;
				
				case FFV_FLOAT32:
					btype_to_btype(var_flags[name_number], FFV_DOUBLE, &temp, FFV_FLOAT);

					if (*var_flags[name_number] < 0)
					{
						*(float *)&lower_bound = *(float *)&temp * (1 + FLT_EPSILON);
						*(float *)&upper_bound = *(float *)&temp * (1 - FLT_EPSILON);							
					}
					else
					{
						*(float *)&lower_bound = *(float *)&temp * (1 - FLT_EPSILON);
						*(float *)&upper_bound = *(float *)&temp * (1 + FLT_EPSILON);							
					}
				break;
				
				case FFV_FLOAT64:
				case FFV_ENOTE:
					if (*var_flags[name_number] < 0)
					{
						lower_bound = *var_flags[name_number] * (1 + DBL_EPSILON);
						upper_bound = *var_flags[name_number] * (1 - DBL_EPSILON);
					}
					else
					{
						lower_bound = *var_flags[name_number] * (1 - DBL_EPSILON);
						upper_bound = *var_flags[name_number] * (1 + DBL_EPSILON);
					}
				break;
			}

			mm_set(data_flag->var, MM_MISSING_DATA_FLAGS, &upper_bound, &lower_bound);

			data_flag->value_exists = 1;
			data_flag->value = *var_flags[name_number];
			
			if (name_number == 0)
				do_log(checkvar_log, "\n");

			if (IS_REAL(var) || var->precision)
			{
				if (IS_ENOTE(var))
				{
					do_log(checkvar_log, "Ignoring data with flag value %0.*G for %s\n",
					       DBL_DIG ? DBL_DIG : var->precision,
					       data_flag->value,
					       var->name);
				}
				else
				{
					do_log(checkvar_log, "Ignoring data with flag value %0.*f for %s\n",
					       var->precision,
					       data_flag->value,
					       var->name);
				}

				/* This bears some looking at... */

				if (IS_REAL(var))
				{
					/* scale float up to an integer, according to precision, for floating
					point comparisons.  Complementary scaling-up for actual data values
					occurs below...
					*/
					data_flag->value *= pow(10, var->precision);
					data_flag->value += (data_flag->value < 0 ? -0.5 : 0.5);
					data_flag->value -= (data_flag->value < 0 ? -fabs(fmod(data_flag->value, 1)) : fabs(fmod(data_flag->value, 1)));
				}
				else
				{
					/*
					Missing data flag must be entered as a floating point number
					if it applies to an implied precision integer.
					*/
					data_flag->value *= pow(10, var->precision);
				}
			} /* if real, or non-zero precision (e.g., integers with implied precision) */
			else if (IS_INTEGER(var))
			{
				do_log(checkvar_log, "Ignoring data with flag value %ld for %s\n",
				       (long)data_flag->value, var->name);
			}
			else
			{
				data_flag->value = 0;
				data_flag->value_exists = 0;
			}
		} /* if ignore missing data */

		*hplist = dll_next(*hplist);
		pinfo = FF_PI(*hplist);
	}	/* End setting up view list */
	
	memFree(var_flags, "var_flags");
	memFree(var_names, "var_names");

	return error;
}

static int iccmp(CHAR_ARRAY_TYPE *n1,CHAR_ARRAY_TYPE *n2)    /* Comparison function for insert to use */
{
	return(strcmp(n1->str, n2->str));
}

static int icmp(BIN_ARRAY_TYPE *n1,BIN_ARRAY_TYPE *n2)	/* Comparison function for insert to use */
{
	/*  ORIGINAL FUNCTION return(n1->bin - n2->bin);     */

	if (n1->bin < n2->bin)
		return(-1);
	else if (n1->bin > n2->bin)
		return(1);

	return(0);
}

static int collapse
	(
	 HEADER *root,
	 long sorterfactor, 
	 int *firstnode, 
	 double *thisbin,
	 long *cnt,
	 HEADER **newroot,
	 long *bins
	)
{
	if (root)
	{
		int error = 0;
		BIN_ARRAY_TYPE *trojan;

		error = collapse(root->left,sorterfactor, firstnode, thisbin, cnt, newroot, bins);
		if (error)
			return(error);

		trojan = (BIN_ARRAY_TYPE *)(root + 1);

		if (*firstnode)
		{
			*firstnode = 0;
			*cnt = 0;

			if (trojan->bin < 0 && !FINT_EQ(fmod(trojan->bin, sorterfactor), 0))
				*thisbin = trojan->bin - (sorterfactor - fabs(fmod(trojan->bin, sorterfactor)));
			else
 				*thisbin = trojan->bin - fmod(trojan->bin, sorterfactor);
		}

		if (trojan->bin < 0 && trojan->bin >= *thisbin && trojan->bin < *thisbin + sorterfactor)
			*cnt += trojan->count;	  	
		else if (trojan->bin >= 0 && trojan->bin - fmod(trojan->bin, sorterfactor) <= *thisbin)
			*cnt += trojan->count;	  	
		else
		{
			HEADER *p;
	
			if ((p = (void *)talloc(sizeof(BIN_ARRAY_TYPE))) == NULL)
				return(ERR_MEM_LACK);
			else
			{
				BIN_ARRAY_TYPE *bat_ptr;

				bat_ptr = (BIN_ARRAY_TYPE *)( (char *)p + sizeof(HEADER) );
				bat_ptr->count = *cnt;
				bat_ptr->bin = *thisbin;

				if (insert(newroot,(HEADER *)bat_ptr,(int(*)(void *, void*))icmp))
					return(err_push(ERR_PROCESS_DATA, "Collision in collapsing tree: bin %ld count %ld", bat_ptr->bin,bat_ptr->count));
				else 
					++(*bins);
			}

			if (trojan->bin < 0 && !FINT_EQ(fmod(trojan->bin, sorterfactor), 0))
				*thisbin = trojan->bin - (sorterfactor - fabs(fmod(trojan->bin, sorterfactor)));
			else
 				*thisbin = trojan->bin - fmod(trojan->bin, sorterfactor);
			  	
			*cnt = trojan->count;
	  } /* else, if (locroot->bin >= 0) && ... */

		error = collapse(root->right,sorterfactor, firstnode, thisbin, cnt, newroot, bins);
		if (error)
			return(error);
	} /* if (root) */

	return(0);
}

/* Traverses the trees inorder and prints the histogram
		out to the file that is externally declared  	 */
static void tcwrite(HEADER *root, FILE *histo_file)
{
	CHAR_ARRAY_TYPE *trojan;

	if (root)
	{
		tcwrite(root->left, histo_file);
		trojan = (CHAR_ARRAY_TYPE *)(root + 1);
		fprintf(histo_file,"%s %ld\n", trojan->str, trojan->count);
		tcwrite(root->right, histo_file);
	}
}

/*****************************************************************************
 * NAME:  tpass1()
 *
 * PURPOSE:  Perform the first pass -- see output_number_histogram()
 *
 * USAGE:  tpass1(root, &col1_len, &col1_prec, &col2_len);
 *
 * RETURNS:  void
 *
 * DESCRIPTION:  Makes the first pass through the bins to determine the
 * maximum field widths.  Since the bins are nodes in a binary tree, this is
 * a recursive function.
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * GLOBALS:  factor (Module)
 *
 * COMMENTS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/

static void tpass1
	(
	 HEADER *root,
	 const int user_precision,
	 int *max_col1_len,
	 int *max_col2_len,
	 double factor
	)
{
	BIN_ARRAY_TYPE *trojan;

	int col1_len = 0;
	int exp = 0;
	
	if (!root)
		return;
	
	tpass1(root->left, user_precision, max_col1_len, max_col2_len, factor);
	
	trojan = (BIN_ARRAY_TYPE *)(root + 1);
	
	if (fabs(trojan->bin) > factor)
		exp = (int)((trojan->bin ? log10(fabs(trojan->bin)) : 0) / factor) + 1;
	else
		exp = (int)((trojan->bin ? log10(fabs(trojan->bin)) : 0) / factor) - 1;

	if (exp < -4 || exp >= DBL_DIG)
		col1_len = DBL_DIG + 7 + (trojan->bin < 0 ? 1 : 0); /* m.dddddddddddddddE+xxx */
	else
		col1_len = (exp > 0 ? exp : 0) + user_precision + (user_precision ? 1 : 0) + (trojan->bin < 0 ? 1 : 0);

	*max_col1_len = max(*max_col1_len, col1_len);

	*max_col2_len = max(*max_col2_len, (int)log10(trojan->count) + 1);

	tpass1(root->right, user_precision, max_col1_len, max_col2_len, factor);
}

/*****************************************************************************
 * NAME:  tpass2()
 *
 * PURPOSE:  Performs the second pass -- see output_number_histogram()
 *
 * USAGE:  tpass2(root, col1_len, col1_prec, col2_len);
 *
 * RETURNS:  void
 *
 * DESCRIPTION:  col1_len, col1_prec, and col2_len contain the maximum field
 * width for column one, the maximum number of decimal places for column one,
 * and the maximum field width for column two, respectively.  Using this
 * information, this function will print two fixed field columns to the
 * summary (aka histogram) file, taking care that decimal points in the first
 * column are aligned, and that trailing zero's in the decimal portion are
 * added.  This is a recursive function.
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * GLOBALS:  histo_file (Module), factor (Module)
 *
 * COMMENTS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/
/*
 * HISTORY:
 *	Rich Fozzard	12/22/95		-rf01
 *		(HEADER *) cast for CW compiler
*/

static void tpass2
	(
	 HEADER *root,
	 int col1_len,
	 const int user_precision,
	 int col2_len,
	 double factor,
	 FILE *histo_file
	)
{
	int exp = 0;
	BIN_ARRAY_TYPE *trojan;

	if (!root)
		return;

	tpass2(root->left, col1_len, user_precision, col2_len, factor, histo_file);

	trojan = (BIN_ARRAY_TYPE *)(root + 1);

	if (fabs(trojan->bin) > factor)
		exp = (int)((trojan->bin ? log10(fabs(trojan->bin)) : 0) / factor) + 1;
	else
		exp = (int)((trojan->bin ? log10(fabs(trojan->bin)) : 0) / factor) - 1;

	if (exp < -4 || exp >= DBL_DIG)
	{
		fprintf(histo_file, "%*.*G\t%*ld\n", (int)col1_len, (int)DBL_DIG,
				  trojan->bin / factor,
				  (int)col2_len, trojan->count
				 );
	}
	else
	{
		fprintf(histo_file, "%*.*f\t%*ld\n", (int)col1_len, (int)user_precision, trojan->bin / factor,
				  (int)col2_len, trojan->count
				 );
	}

	tpass2(root->right, col1_len, user_precision, col2_len, factor, histo_file);
}

/*****************************************************************************
 * NAME:  output_number_histogram()
 *
 * PURPOSE:  Writes the .lst file with fixed field formatting
 *
 * USAGE:  output_number_histogram(root);
 *
 * RETURNS:  void
 *
 * DESCRIPTION:  Makes two passes through the bins:  first to determine
 * the maximum field widths, and the second to write the histogram file with
 * fixed fields that are sized according to the first pass.
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * GLOBALS:
 *
 * COMMENTS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/

static void output_number_histogram
	(
	 HEADER *root,
	 const int precision, 
	 FILE *histo_file
	)
{
	int col1_len = 0,
	    col2_len = 0;

	double factor = 0;
	
	factor = (double)pow(10.0, precision);
	assert(fabs(factor) > DBL_EPSILON);

	tpass1(root, precision, &col1_len, &col2_len, factor);
	tpass2(root, col1_len, precision, col2_len, factor, histo_file);
}

static BOOLEAN set_max_min
	(
	 VARIABLE_PTR var,
	 void *data_ptr,
	 FF_DATA_FLAG_PTR data_flag,
	 double *double_var,
	 char str[MAX_NAME_LENGTH]
	)
{
	BOOLEAN found_data_flag = FALSE;

	FF_VALIDATE(var);
	FF_VALIDATE(data_flag);

	/* Retrieve a value for the variable and compare it to MAXMIN */
	if (IS_TEXT(var) || IS_CONSTANT(var) || IS_INITIAL(var))
	{
		int count = 0;

		count = (int)(var->end_pos - var->start_pos + 1);
 		count = min(count, (MAX_NAME_LENGTH - 1));
		str[count] = '\0';

		memcpy(str, data_ptr, count);
		mm_set(data_flag->var, MM_MAX_MIN, str, &found_data_flag);
		--data_flag->var->misc.mm->cur_record;
	}
	else
	{
		/* The maxmin calculation is done without changing the precision
		of the variable, the adjustment gets done prior to printing it out. We also
		assume at this point that the data is binary. If it is ASCII, it will be
		converted to binary using the same scheme as in maxmin.c The copy
		to double_var is done to insure alignment. */

		memcpy((void*)double_var, data_ptr, FF_VAR_LENGTH(var));

		/* check to see if this is missing data */
		if (data_flag->value_exists)
		{
			btype_to_btype(data_ptr, var->type, (void *)(&(data_flag->temp_dvar)), FFV_DOUBLE);

			if (var->precision)
			{
				if (IS_REAL(var))
				{
					data_flag->temp_dvar *= pow(10, var->precision);
					data_flag->temp_dvar += (data_flag->temp_dvar < 0 ? -0.5 : 0.5);
					data_flag->temp_dvar -= (data_flag->temp_dvar < 0 ? -fabs(fmod(data_flag->temp_dvar, 1)) : fabs(fmod(data_flag->temp_dvar, 1)));
				}
				else 
				{
					assert(IS_INTEGER(var));
					/* nothing to change, data_flag->value was already
						"upscaled" to a whole number*/
				}

			}
	if (data_flag->temp_dvar == data_flag->value)
	{
		assert(IS_REAL(var) || IS_INTEGER(var));

		assert(FALSE == FALSE);
	}
		}
		
		mm_set(data_flag->var, MM_MAX_MIN, (char *)double_var, &found_data_flag);
		--data_flag->var->misc.mm->cur_record;

		if (data_flag->value_exists && data_flag->temp_dvar == data_flag->value)
		{
			if (found_data_flag == FALSE)
				err_push(ERR_ASSERT_FAILURE, "Expecting data flag to have been found by mm_set");

			return TRUE;
		}
		else if (data_flag->value_exists)
		{
			if (found_data_flag == TRUE)
				err_push(ERR_ASSERT_FAILURE, "Expecting data flag to have been found by set_max_min");
		}
	}

	return FALSE;
}

static int collapse_tree
	(
	 long *bins,
	 long *sorterfactor,
	 int *firstnode,
	 HEADER **root,
	 double *thisbin,
	 long *cnt,
	 HEADER **newroot
	)
{
	int error = 0;
	void *p = NULL;

#ifdef TREESIZ	
	do_log(checkvar_log,"%ld bins in the tree before forced collapse.\n",bins);
#endif

	*bins = 0;
	*sorterfactor *= 2;
	*firstnode = 1;
	error = collapse(*root,*sorterfactor, firstnode, thisbin, cnt, newroot, bins);
	if (error)
		return(error);

	if ((p = (void *)talloc(sizeof(BIN_ARRAY_TYPE))) == NULL)
		return(ERR_MEM_LACK);
	else
	{	
		BIN_ARRAY_TYPE *bat_ptr = NULL;

		bat_ptr = (BIN_ARRAY_TYPE *)((char *)p + sizeof(HEADER));
		bat_ptr->count = *cnt;
		bat_ptr->bin = *thisbin;

		if (insert(newroot,(HEADER *)bat_ptr,(int(*)(void *, void*))icmp))	/* cast for CW compiler -rf01 */
			return(err_push(ERR_PROCESS_DATA, "Collision in collapsing tree: bin %ld count %ld",bat_ptr->bin,bat_ptr->count));
		else 
			++(*bins);
	}

	freeall(root);
	*root = *newroot;
	*newroot = NULL;

	return error;
}

static int print_summaries
	(
	 FF_BUFSIZE_PTR checkvar_log,
	 FF_DATA_FLAG_PTR data_flag,
	 PROCESS_INFO_PTR pinfo,
	 long std_args_records_to_read,
	 VARIABLE_PTR var,
	 long num_records,
	 int maxmin_only,
	 int precision,
	 long *bins,
	 HEADER **root,
	 FILE *histo_file,
	 long *sorterfactor
	)
{
	int error = 0;
	double minimum = 0;
	double maximum = 0;

	long min_rec_occurrence = 0;
	long max_rec_occurrence = 0;

	double adjustment = 0;

	if (checkvar_log)
		FF_VALIDATE(checkvar_log);

	FF_VALIDATE(data_flag);
	FF_VALIDATE(pinfo);
	FF_VALIDATE(var);

	/* PRINT OUT THE MINIMUM AND MAXIMUM VALUE */
	minimum = mm_getmn(data_flag->var);
	maximum = mm_getmx(data_flag->var);
	min_rec_occurrence = (data_flag->var->misc.mm)->min_record;
	max_rec_occurrence = (data_flag->var->misc.mm)->max_record;
	
	/* record occurrences may be relative to the tail */
	if (std_args_records_to_read < 0)
	{
		min_rec_occurrence += (PINFO_MATE_SUPER_ARRAY_ELS(pinfo) + std_args_records_to_read);
		max_rec_occurrence += (PINFO_MATE_SUPER_ARRAY_ELS(pinfo) + std_args_records_to_read);
	}
	
	if (IS_INTEGER(var))
	{
		adjustment = -var->precision;
		minimum *= pow(10.0, adjustment);
		maximum *= pow(10.0, adjustment);
	}
	
	do_log(checkvar_log, "\n\n");

	if (IS_TEXT(var) || IS_CONSTANT(var) || IS_INITIAL(var))
	{
		do_log(checkvar_log, "%s: %ld values read\n", var->name, num_records - 1);
		do_log(checkvar_log, "minimum: %s found at record %lu\n", (char *)((long)(minimum)), min_rec_occurrence);
		do_log(checkvar_log, "maximum: %s found at record %lu\n", (char *)((long)(maximum)), max_rec_occurrence);
		do_log(checkvar_log, "Summary file: %s\n", maxmin_only ? "No Summary" : pinfo->name);
	}
	else
	{
		int col1_len = 0;
		int col2_len = 0;

		/* determine maximum field widths for min/max report */
		if (IS_ENOTE(var))
		{
			col1_len = DBL_DIG + 7 + ((minimum < 0 || maximum < 0) ? 1 : 0);
		}
		else
		{
			col1_len = (int)(minimum ? log10(fabs(minimum)) : 0) + 1 + (minimum < 0 ? 1 : 0);
			col1_len = max(col1_len, (int)(maximum ? log10(fabs(maximum)) : 0) + 1 + (maximum < 0 ? 1 : 0));
			
			if (var->precision)
				col1_len += var->precision + 1;
		}
		
		col2_len = (int)log10(min_rec_occurrence) + 1;
		col2_len = max(col2_len, (int)log10(max_rec_occurrence) + 1);
		
		if (!maxmin_only)
			do_log(checkvar_log, "Histogram data precision: %d, Number of sorting bins: %ld\n",
				    (int)precision, (long)*bins);

		do_log(checkvar_log, "%s: %ld values read", var->name, num_records - 1);
		
		if (std_args_records_to_read > 0)
			do_log(checkvar_log, " from head of file\n");
		else if (std_args_records_to_read < 0)
			do_log(checkvar_log, " from tail of file\n");
		else
			do_log(checkvar_log, "\n");

		if (IS_ENOTE(var))
		{
			do_log(checkvar_log, "minimum: %*.*g found at record %*ld\n",
				    (int)col1_len, DBL_DIG ? DBL_DIG : (int)(var->precision), minimum, (int)col2_len, min_rec_occurrence);
			do_log(checkvar_log, "maximum: %*.*g found at record %*ld\n",
				    (int)col1_len, DBL_DIG ? DBL_DIG : (int)(var->precision), maximum, (int)col2_len, max_rec_occurrence);
		}
		else
		{
			do_log(checkvar_log, "minimum: %*.*f found at record %*ld\n",
				    (int)col1_len, (int)(var->precision), minimum, (int)col2_len, min_rec_occurrence);
			do_log(checkvar_log, "maximum: %*.*f found at record %*ld\n",
				    (int)col1_len, (int)(var->precision), maximum, (int)col2_len, max_rec_occurrence);
		}

		do_log(checkvar_log, "Summary file: %s\n", maxmin_only ? "No Summary" : pinfo->name);
	}
	
	if (!maxmin_only)
	{
	/* PRINT OUT AND FREE THE HISTOGRAM TREES */
		if (IS_TEXT(var) || IS_CONSTANT(var) || IS_INITIAL(var))
			tcwrite(*root, histo_file);
		else
			output_number_histogram(*root, (unsigned char)precision, histo_file);

		if (fclose(histo_file) != 0)
			return(ERR_WRITE_FILE); /* replace this with a better error code */

#ifdef HEAPCHK
		/*  Print heap size before free     */
		heapPrt("before free");	 
#endif

		freeall(root);
		*root = NULL;
		*bins = 0;
#ifdef HEAPCHK
		/*  Print heap size after free    */
		heapPrt("after free");	 
#endif
		*sorterfactor = 1;	
	}	/* end of if (!maxmin_only) */

	return error;
}

static int fill_histo
	(
	 VARIABLE_PTR var,
	 char str[MAX_NAME_LENGTH],
	 HEADER **root,
	 int precision,
	 double *double_var,
	 long sorterfactor,
	 long *bins
	)
{
	int error = 0;
	void *p = NULL;
	void *p2 = NULL;

	FF_VALIDATE(var);

	if (IS_TEXT(var) || IS_CONSTANT(var) || IS_INITIAL(var))
	{
		if ((p = (void *)talloc(sizeof(CHAR_ARRAY_TYPE))) == NULL)
			return err_push(ERR_MEM_LACK, "");
		else
		{
			CHAR_ARRAY_TYPE *cat_ptr = NULL;

			cat_ptr = (CHAR_ARRAY_TYPE *)( (char *)p + sizeof(HEADER) );
			cat_ptr->count = 1;
			if (var->precision)
			{
				/* Added '-1' because I'm not sure if var->precision is the size or
				 * the size-1. jhrg */
				strncpy(cat_ptr->str, str, var->precision);
				cat_ptr->str[var->precision-1] = STR_END;
			}
			else
				strcpy(cat_ptr->str, str);

			if ((p2 = insert(root,(HEADER *)cat_ptr,(int(*)(void *, void*))iccmp)) != NULL)	/* (HEADER *) cast for CW compiler -rf01 */
			{
				((CHAR_ARRAY_TYPE *)p2)->count++;
				(void)memFree(p, "p");
			}
		} /* else, p successfully talloc'd */
	} /* if (IS_TEXT(var)) */
	else
	{	/* numeric data */
		if ((p = (void *)talloc(sizeof(BIN_ARRAY_TYPE))) == NULL)
			return(ERR_MEM_LACK);
		else
		{
			BIN_ARRAY_TYPE *bat_ptr = NULL;
			double sorter = 0;
			double adjustment = 0;

			memcpy((void *)&adjustment, (void *)double_var, sizeof(double));
			btype_to_btype(&adjustment, FFV_TYPE(var), double_var, FFV_DOUBLE);

			if (0 && IS_ENOTE(var))
			{
				adjustment = -(int)(*double_var ? log10(fabs(*double_var)) : 0);
				if (adjustment > 0)
					++adjustment;
				adjustment += var->precision - 1;
			}
			else
				adjustment = precision;

			/* If implied precision and user wants a certain number of decimal places,
			   then multiply the value by ten raised to the power of the difference in
				requested and actual decimal places.
			*/
			if (IS_INTEGER(var))
				adjustment -= var->precision;

			*double_var *= pow(10.0, adjustment);
			*double_var = floor(*double_var) + (*double_var < 0 ? -FLT_EPSILON : FLT_EPSILON);
			btype_to_btype((void *)double_var, FFV_DOUBLE, (void *)&sorter, FFV_DOUBLE);

			bat_ptr = (BIN_ARRAY_TYPE *)( (char *)p + sizeof(HEADER) );
			bat_ptr->count = 1;

			/* Round sorter to the next smaller multiple of sorterfactor (sorter may
			   already be so rounded).
			*/

			if (sorter < 0 && !FINT_EQ(fmod(sorter, sorterfactor), 0))
				bat_ptr->bin = sorter - (sorterfactor - fabs(fmod(sorter, sorterfactor)));
			else
				bat_ptr->bin = sorter - fmod(sorter, sorterfactor);

			/* BUS error can occur above; bat_ptr->bin is not necessarily double-aligned.
			   Need to do better than trojan structure; unions would work but would waste
			   a lot of memory.  Maybe two different HEADERs.
			*/

			if ((p2 = insert(root,(HEADER *)bat_ptr,(int(*)(void *, void*))icmp)) != NULL)	/* (HEADER *) cast for CW compiler -rf01 */
			{
				((BIN_ARRAY_TYPE *)p2)->count++;
				(void)memFree((BIN_ARRAY_TYPE *)p, "p");
			}
			else
				++(*bins);
		} /* else, p successfully talloc'd */
	} /* else, if (IS_TEXT(var)) -- var is not char */

	return error;
}

static int checkvar_error_cb(int event)
{
	if (event == DBSET_OUTPUT_FORMATS)
		return FALSE;
	else
		return TRUE;
}

/*
 * NAME:	checkvar
 *		
 * PURPOSE:	To create histograms of data values in a file
 *
 * USAGE:	
 *
 * RETURNS:	
 *
 * DESCRIPTION:	This program produces output which can be used to make
 * histograms of variables in binary or ascii data files 
 * through the FREEFORM format specification system.
 * It can have up to four command line arguments:
 *   1) The file name.
 *   2) The precision.
 *   3) The maximum number of bins. This	controls the
 *      resolution of the histogram.
 *   4) The format file name.
 * The format file name is assumed to be the same as the
 * data file name  with an extension of .afm or .bfm and if
 * this is the case, this argument may be omitted.
 *
 * SYSTEM DEPENDENT FUNCTIONS:	
 *
 * AUTHOR:	 T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 * Modified (or rewritten shall we say) by Merrill Bennett, July, 1991
 * Major revision to list of views: Mark Van Gorp, mvg@ngdc.noaa.gov, 497-6221
 * ASCII data handling, merging with maxmin.c: kbf@ngdc.noaa.gov, February, 1994
 * COMMENTS:	
 *
 * KEYWORDS:	
 *
 */

/* Function prototypes */

#define DEFAULT_MAXBINS 100
#define MAXIMUM_MAXBINS 10000
#define SCRATCH_SIZE 16384
#define NUM_RECORDS_UPDATE 1000

int checkvar
	(
	 FF_STD_ARGS_PTR std_args,
	 FF_BUFSIZE_PTR checkvar_log,
	 FILE *to_user
	)
{ 
	char *greeting = 
{
#ifdef FF_ALPHA
"\nWelcome to Checkvar alpha "FFND_LIB_VER" "__DATE__" -- an NGDC FreeForm ND application\n\n"
#elif defined(FF_BETA)
"\nWelcome to Checkvar beta "FFND_LIB_VER" "__DATE__" -- an NGDC FreeForm ND application\n\n"
#else
"\nWelcome to Checkvar release "FFND_LIB_VER" -- an NGDC FreeForm ND application\n\n"
#endif
};

	char *command_line_usage = {
"Several Checkvar command line elements have default extensions:\n\
data_file: .bin = binary  .dat = ASCII  .dab = flat ASCII\n\
input/output_format_file: .fmt = format description file\n\
checkvar [-d data_file] [-f format_file][-ift \"input format title\"]\n\
         [-b cache_size] Sets the input data cache size in bytes\n\
         [-c count] No. records to process at head(+)/tail(-) of file\n\
         [-q query] Process records matching criteria in query file\n\
         [-v var] Process variables listed in var file, default = all\n\
         [-p precision] No. of decimal places in variable summaries\n\
         [-m maxbins] approximate maximum number of bins desired\n\
                      default = 100, min = 6, max = 10,000\n\
         [-mm] Output min/max values; no variable summaries created\n\
         [-s] Subset or convert using output formats (if they exist)\n\
         [-od directory] Directory to contain variable summary files\n\
         [-el error_log_file] error messages also go to error_log_file\n\
         [-ep] passive error messages - non-interactive\n\
         [-ol log] File to contain log of processing information\n\n\
See the FreeForm User's Guide for detailed information.\n"
};

	int firstnode = 1;
	double thisbin = 0;
	FILE *histo_file = NULL;
	long cnt = 0L;
	long bins = 0L;

	int precision = 0;
	int maxbins = DEFAULT_MAXBINS;

	char str[MAX_NAME_LENGTH];

	double double_var = 0;
	
	int perc_done = 0;
	int nevercollapse = 0;
	int error = 0;
	int maxmin_only = 0;    
		
	long num_lines = 0;
	
	long num_records = 1;
	long sorterfactor = 1L;
	
	HEADER *newroot = NULL;
	HEADER *root = NULL;

	VARIABLE_PTR var = NULL;
	 	
	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	FF_DATA_FLAG_LIST data_flag_list = NULL;
	FF_DATA_FLAG_PTR data_flag = NULL;

	long total_bytes = 0;
	long total_done_bytes = 0;

	FF_BUFSIZE_PTR bufsize = NULL;
	DATA_BIN_PTR dbin = NULL;

	char *save_var_file = NULL;
	char tmpfile_array[3][MAX_PATH];
	char *tmpfile_ptr_array[4];
	char **tmpfiles = tmpfile_ptr_array;

#ifdef TIMER
	long elapsed_time = 0;
	time_t finish_time = 0, current_start_time = 0;
	time_t start_time;
	(void)time(&start_time);
#endif

	tmpfile_ptr_array[0] = *tmpfile_array;
	tmpfile_ptr_array[1] = *tmpfile_array + MAX_PATH;
	tmpfile_ptr_array[2] = *tmpfile_array + 2 * MAX_PATH;

	if (checkvar_log)
		do_log(checkvar_log, "%s", greeting);
	else
		wfprintf(to_user, "%s", greeting);

	/* Check number of command args */
	if (std_args->input_file == NULL && std_args->input_bufsize == NULL && !std_args->user.is_stdin_redirected)
	{
		fprintf(stderr, "%s", command_line_usage);

		return 0;
	}

	save_var_file = std_args->var_file;
	std_args->var_file = NULL;

	error = db_init(std_args, &dbin, checkvar_error_cb);
	if (error && error < ERR_WARNING_ONLY)
		return error;
	else if (error)
		error = 0;

	if (std_args->query_file)
		do_log(checkvar_log, "Using query file: %s\n", std_args->query_file);

	/* Display some information about the data, ignore errors */
	db_ask(dbin, DBASK_FORMAT_SUMMARY, std_args->cv_subset ? 0 : FFF_INPUT, &bufsize);
	do_log(checkvar_log, "%s\n", bufsize->buffer);

	ff_destroy_bufsize(bufsize);

	std_args->var_file = save_var_file;

	error = transmogrify(std_args, checkvar_log, dbin, tmpfiles);
	if (error)
	{
		db_destroy(dbin);
		return error;
	}

	std_args->var_file = save_var_file;

	if (std_args->user.set_cv_precision)
		precision = std_args->cv_precision;

	if (std_args->cv_maxbins)
		maxbins = std_args->cv_maxbins;

	if (maxbins < 1)
		nevercollapse = 1;
	
	if (std_args->cv_maxmin_only)
		maxmin_only = 1;

	do_log(checkvar_log, "Input file : %s", std_args->input_file);

	if (std_args->user.set_cv_precision)
		do_log(checkvar_log, "\nRequested precision = %d, Approximate number of sorting bins = %d\n", precision, maxbins);
	else
		do_log(checkvar_log, "\nNo requested precision, Approximate number of sorting bins = %d\n", maxbins);

	if (std_args->var_file)
		do_log(checkvar_log, "\nUsing variable file: %s", std_args->var_file);
	
	if (maxmin_only)
		do_log(checkvar_log, "\nCalculating minimums and maximums only");
	
	if (std_args->query_file)
		do_log(checkvar_log, "\nUsing query file: %s", std_args->query_file);
	
	/*	If stdin is redirected, the precision and maxbins stated on
	the command line will be ignored unless they are left out
	of the list in the redirected file.
	The format of the entries in the redirected file are as follows:
			
	var_name precision maxbins
		
	for each variable to be histogrammed. */

#ifdef HEAPCHK
	/* Print the heap size at the beginning    */
	heapPrt("Before mallocs of data ");
#endif

	if (std_args->var_file || maxmin_only || std_args->query_file)
		do_log(checkvar_log, "\n");

	error = setup_lists(dbin, checkvar_log, maxmin_only, std_args->cv_list_file_dir, &plist, &data_flag_list);
	if (error)
		return error;

	/* TRAVERSE THE LIST OF VIEWS AND CREATE HISTOGRAM FILES FOR EACH VIEW */
	data_flag_list = dll_first(data_flag_list);
	plist = dll_first(plist);
	pinfo = FF_PI(plist);
	while (pinfo)
	{
		do_log(checkvar_log, "\n");

		data_flag = FF_DF(data_flag_list);

		var = FF_VARIABLE(FFV_FIRST_VARIABLE(PINFO_FORMAT(pinfo)));
		FF_VALIDATE(var);
	
		if (!maxmin_only && (histo_file = fopen(pinfo->name, "wt")) == NULL)
			return err_push(ERR_CREATE_FILE, pinfo->name);

		if (std_args->user.set_cv_precision)
			precision = (int)min((int)std_args->cv_precision, (int)var->precision);
		else
			precision = var->precision;

#ifdef TIMER
		(void)time(&current_start_time);
#endif
		total_bytes = PINFO_MATE_ARRAY_BYTES(pinfo);

		num_records = 1;
		do
		{
			void *data_ptr = NULL;
			unsigned long buffer_size = 0;

			error = db_do(dbin, DBDO_PROCESS_FORMATS, FFF_INPUT);
			if (error && error != EOF)
				return err_push(ERR_GENERAL, "");

			error = ff_lock(pinfo, &data_ptr, &buffer_size);
			if (error)
				return err_push(ERR_GENERAL, "Couldn't get the buffer for %s", PINFO_NAME(pinfo));

			assert(PINFO_NEW_RECORD(pinfo));
			assert(buffer_size % PINFO_RECL(pinfo) == 0);
			
			for (num_lines = buffer_size / PINFO_RECL(pinfo); num_lines > 0; --num_lines, ++num_records, data_ptr = (char *)data_ptr + PINFO_RECL(pinfo))
			{
				if (set_max_min(var, data_ptr, data_flag, &double_var, str))
					continue;

				if (!maxmin_only)
				{
					/* COLLAPSE TREE IF RUNNING OUT OF MEMORY */
					if (!(IS_TEXT(var) || IS_CONSTANT(var) || IS_INITIAL(var)))
					{
						if (bins > MAXIMUM_MAXBINS)
						{
							/* ^^^ this is a kludgy way of deciding we're out of memory */
#ifdef TREESIZ	
							do_log(checkvar_log,"%ld bins in the tree before collapse.\n",bins);
#endif

							error = collapse_tree(&bins, &sorterfactor, &firstnode, &root, &thisbin, &cnt, &newroot);
							if (error)
								return error;
						}
					} /* if (!IS_TEXT(var)) */
		
					/* COLLAPSE TREE IF BIGGER THAN REQUESTED */ 
					if ((bins > (int)(3 * maxbins / 2)) && (!nevercollapse))
					{
						if (!(IS_TEXT(var) || IS_CONSTANT(var) || IS_INITIAL(var)))
						{
#ifdef TREESIZ	
							do_log(checkvar_log,"%ld bins in the tree before collapse.\n",bins);
#endif

							error = collapse_tree(&bins, &sorterfactor, &firstnode, &root, &thisbin, &cnt, &newroot);
							if (error)
								return error;
						}
					} /* if tree too big */
				   
					error = fill_histo(var, str, &root, precision, &double_var, sorterfactor, &bins);
				} /* end of if (!maxmin_only) */
			} /* End of Loop over a single cache */

			error = ff_unlock(pinfo, &data_ptr);
			if (error)
				return err_push(ERR_GENERAL, "Couldn't release the buffer for %s", PINFO_NAME(pinfo));

			PINFO_BYTES_LEFT(pinfo) -= buffer_size;

			/* Calculate percentage left to process and display */
			if (1 || !checkvar_log)
			{
				perc_done = (int)((1 - ((float)PINFO_MATE_BYTES_LEFT(pinfo) / total_bytes)) * 100);
				wfprintf(stderr,"\r%3d%% processed", perc_done);
		  
#ifdef TIMER
				(void)time(&finish_time);
				elapsed_time = (long)difftime(finish_time, current_start_time);
				wfprintf(stderr, "     Elapsed time - %02d:%02d:%02d",
				        (int)(elapsed_time / (3600)),
				        (int)((elapsed_time / 60) % 60),
				        (int)(elapsed_time % 60)
				        );
#endif
			}
		} while (!error && PINFO_BYTES_LEFT(pinfo));

		print_summaries(checkvar_log, data_flag, pinfo, std_args->records_to_read, var, num_records, maxmin_only, precision, &bins, &root, histo_file, &sorterfactor);

		data_flag_list = dll_next(data_flag_list);
		plist = dll_next(plist);
		pinfo = FF_PI(plist);
	} /* while (dll_data(view_list)) */

	ff_destroy_process_info_list(data_flag_list);

	total_bytes = 0;
	plist = dll_first(plist);
	pinfo = FF_PI(plist);
	while (pinfo)
	{
		total_bytes += PINFO_MATE_ARRAY_BYTES(pinfo);

		plist = dll_next(plist);
		pinfo = FF_PI(plist);
	}

	total_done_bytes = 0;
	plist = dll_first(plist);
	pinfo = FF_PI(plist);
	while (pinfo)
	{
		total_done_bytes += PINFO_MATE_BYTES_LEFT(pinfo);

		plist = dll_next(plist);
		pinfo = FF_PI(plist);
	}

	ff_destroy_process_info_list(plist);

	perc_done = (int)((1 - ((float)total_done_bytes / total_bytes)) * 100);

#ifndef DEBUG
	while (*tmpfiles && os_file_exist(*tmpfiles))
		remove(*(tmpfiles++));
#endif

	db_destroy(dbin);
	
#ifdef TIMER
	(void)time(&finish_time);
	elapsed_time = (long)difftime(finish_time, start_time);
	wfprintf(to_user,
	       "\n%3d%% processed     Total elapsed time - %02d:%02d:%02d\n",
	       perc_done,
	       (int)(elapsed_time / (3600)),
	  	    (int)((elapsed_time / 60) % 60),
		    (int)(elapsed_time % 60)
	      );
	if (checkvar_log)
	{
		do_log(checkvar_log,
		       "\n%3d%% processed     Total elapsed time - %02d:%02d:%02d\n",
		       perc_done,
		       (int)(elapsed_time / (3600)),
		  	    (int)((elapsed_time / 60) % 60),
			    (int)(elapsed_time % 60)
		      );
	}
#endif

	return(0);

}	/* ******************** end of main ****************** */

