/*
 * FILENAME:        ndarray.c
 *
 * PURPOSE:     Routines for arbitrary n-dimensional (0 < n < 32768)
 *				array access, subsetting, and reorientation.
 *
 * DESCRIPTION: contents are:
 *
 *				ndarr_create()
 *				ndarr_create_brkn_desc()
 *				ndarr_create_from_str()
 *				ndarr_create_indices()
 *				ndarr_create_mapping()
 *				ndarr_convert_indices()
 *				ndarr_do_calculations()
 *				ndarr_free_indices()
 *				ndarr_free_descriptor()
 *				ndarr_free_mapping()
 *				ndarr_get_group()
 *				ndarr_get_mapped_offset()
 *				ndarr_get_next_group()
 *				ndarr_get_offset()
 *				ndarr_increment_indices()
 *				ndarr_increment_mapping()
 *				ndarr_reorient()
 *				ndarr_set()
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:	These functions are meant to have a fairly narrow interface
 *				with anything they are linked to.
 *
 * KEYWORDS: array
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

/*
 * NAME:        ndarr_create
 *
 * PURPOSE:     To allocate an array descriptor structure
 *
 * USAGE:		ARRAY_DESCRIPTOR_PTR ndarr_create(int numdim)
 *
 * RETURNS:     a pointer to the created ARRAY_DESCRIPTOR struct, or NULL
 *				on error.
 *
 * DESCRIPTION: Allocates an ARRAY_DESCRIPTOR structure for the appropriate
 *				number of dimensions given in numdim.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_create"
ARRAY_DESCRIPTOR_PTR ndarr_create(int numdim)
{
	ARRAY_DESCRIPTOR_PTR arrd;
	int i;

	assert(numdim);

	if(!(arrd = (ARRAY_DESCRIPTOR_PTR)memMalloc(sizeof(ARRAY_DESCRIPTOR), "arrd"))){
		err_push(ERR_NDARRAY, "Out of memory");
		return(NULL);
	}

	/* Initialize to default values */
	arrd->num_dim = numdim;
	arrd->element_size = 0;
	arrd->total_size = 0;
	arrd->total_elements = 0;
	arrd->dim_name = NULL;
	arrd->start_index = NULL;
	arrd->end_index = NULL;
	arrd->granularity = NULL;
	arrd->grouping = NULL;
	arrd->separation = NULL;
	arrd->index_dir = NULL;
	arrd->dim_size = NULL;
	arrd->coeffecient = NULL;
	arrd->type = NDARRT_CONTIGUOUS;
	arrd->extra_info = NULL;
	arrd->extra_index = NULL;
	arrd->num_groups = 0;
	arrd->group_size = 0;
	arrd->contig_size = 0;

	arrd->dim_name = NULL;
	arrd->start_index = NULL;
	arrd->end_index = NULL;
	arrd->granularity = NULL;
	arrd->grouping = NULL;
	arrd->separation = NULL;
	arrd->index_dir = NULL;
	arrd->dim_size = NULL;
	arrd->coeffecient = NULL;
	arrd->extra_info = NULL;
	arrd->extra_index = NULL;
#ifdef ND_FP 
	arrd->fp = NULL;
#endif

	/* Malloc all of the arrays... */
	if(!(arrd->dim_name = (char **)memMalloc((size_t)(sizeof(char *) * numdim), "arrd->dim_name"))){
		err_push(ERR_NDARRAY, "Out of memory");
		ndarr_free_descriptor(arrd);
		return(NULL);
	}
	if(!(arrd->start_index = (long *)memMalloc((size_t)(sizeof(long) * numdim), "arrd->start_index"))){
		err_push(ERR_NDARRAY, "Out of memory");
		memFree(arrd->dim_name, "arrd->dim_name");
		arrd->dim_name = NULL;
		ndarr_free_descriptor(arrd);
		return(NULL);
	}
	if(!(arrd->end_index = (long *)memMalloc((size_t)(sizeof(long) * numdim), "arrd->end_index"))){
		err_push(ERR_NDARRAY, "Out of memory");
		memFree(arrd->dim_name, "arrd->dim_name");
		arrd->dim_name = NULL;
		ndarr_free_descriptor(arrd);
		return(NULL);
	}
	if(!(arrd->granularity = (long *)memMalloc((size_t)(sizeof(long) * numdim), "arrd->granularity"))){
		err_push(ERR_NDARRAY, "Out of memory");
		memFree(arrd->dim_name, "arrd->dim_name");
		arrd->dim_name = NULL;
		ndarr_free_descriptor(arrd);
		return(NULL);
	}
	if(!(arrd->grouping = (long *)memMalloc((size_t)(sizeof(long) * numdim), "arrd->grouping"))){
		err_push(ERR_NDARRAY, "Out of memory");
		memFree(arrd->dim_name, "arrd->dim_name");
		arrd->dim_name = NULL;
		ndarr_free_descriptor(arrd);
		return(NULL);
	}
	if(!(arrd->separation = (long *)memMalloc((size_t)(sizeof(long) * numdim), "arrd->separation"))){
		err_push(ERR_NDARRAY, "Out of memory");
		memFree(arrd->dim_name, "arrd->dim_name");
		arrd->dim_name = NULL;
		ndarr_free_descriptor(arrd);
		return(NULL);
	}
	if(!(arrd->index_dir = (char *)memMalloc((size_t)(sizeof(char) * numdim), "arrd->index_dir"))){
		err_push(ERR_NDARRAY, "Out of memory");
		memFree(arrd->dim_name, "arrd->dim_name");
		arrd->dim_name = NULL;
		ndarr_free_descriptor(arrd);
		return(NULL);
	}
	if(!(arrd->dim_size = (long *)memMalloc((size_t)(sizeof(long) * numdim), "arrd->dim_size"))){
		err_push(ERR_NDARRAY, "Out of memory");
		memFree(arrd->dim_name, "arrd->dim_name");
		arrd->dim_name = NULL;
		ndarr_free_descriptor(arrd);
		return(NULL);
	}
	if(!(arrd->coeffecient = (long *)memMalloc((size_t)(sizeof(long) * numdim), "arrd->coeffecient"))){
		err_push(ERR_NDARRAY, "Out of memory");
		memFree(arrd->dim_name, "arrd->dim_name");
		arrd->dim_name = NULL;
		ndarr_free_descriptor(arrd);
		return(NULL);
	}
	
	/* Initialize the arrays */
	for(i = 0; i < numdim; i++){
		arrd->dim_name[i] = NULL;
		arrd->start_index[i] = 0;
		arrd->end_index[i] = 0;
		arrd->granularity[i] = 1;
		arrd->grouping[i] = 0;
		arrd->separation[i] = 0;
		arrd->index_dir[i] = 0;
		arrd->dim_size[i] = 0;
		arrd->coeffecient[i] = 0;
	}

	/* All is OK, return the array descriptor */
	return(arrd);
}

#define skip_lead_space(p) ((p)+=strspn(p," \t"))
/*
 * NAME:        ndarr_create_from_str
 *
 * PURPOSE:     To parse an array description string to form an array
 *					descriptor structure.
 *
 * USAGE:		ARRAY_DESCRIPTOR_PTR ndarr_create_from_str(char *arraystr)
 *
 * RETURNS:     a pointer to the created ARRAY_DESCRIPTOR struct, or NULL
 *				on error.
 *
 * DESCRIPTION:
 * ndarr_create_from_str basically parses a string which describes the
 * array and fills the information into an ARRAY_DESCRIPTOR structure.
 * This function "eats" the descriptor string passed to it-
 * it will modify the string in memory in that it will remove all names
 * inside quotes from the string.
 *
 * The format of the string is as follows:
 *
 * ["dimension name" start_index to end_index by granularity
 *       sb separation gb grouping]
 *		["name" start to end]...[]size
 *
 * The "by granularity" argument is completely optional; if left out,
 *  granularity defaults to 1.
 *
 * The "sb separation" argument is completely optional; if left out,
 *	 separation defaults to none.  separation is the number of bytes
 *	 between elements indexed in that dimension. "sb" can also be replaced
 *   with "separation".
 *
 * The "gb grouping" argument is completely optional; if left out,
 *   grouping defaults to none.  Grouping is the number of elements in
 *   that dimension which are contained in a single block of the array;
 *   blocks can be files or buffers.  "gb" can also be replaced with
 *   "grouping"
 *
 * For example,
 * ["longitude" -180 to 180 by 5]["latitude" 90 to -90]["band" 1 to 5] 2
 * would describe a 3-dimensional array, with the first dimension labeled
 * "longitude", with indices ranging from -180 to 180 in increments of 5,
 * second dimension "latitude" from 90 to -90, and third dimension "band",
 * from 1 to 5, with element size of 2 bytes.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:	This function "eats" it's argument- the string will not
 *				be the same after this function has been called.
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_create_from_str"
ARRAY_DESCRIPTOR_PTR ndarr_create_from_str(DATA_BIN_PTR dbin, char *arraystr)
{
	int numdim, i, j;
	char *position;
	char *pos;
	char *chpos;
	ARRAY_DESCRIPTOR_PTR arrd = NULL;
	double start;
	double end;
	long sep;
	double grp;
	double gran;

	char *endptr = NULL;
	char save_char = STR_END;

	char **dim_names = NULL;

	int error = 0;

	assert(arraystr);

	if (dbin)
		FF_VALIDATE(dbin);
    
    /* Determine the number of dimensions */
	j = 0;
	numdim = 0;
	for(i = strlen(arraystr); i >= 0; i--){
		if(arraystr[i] == '[')
			numdim++;
		if(arraystr[i] == ']')
			j++;
	}

	if(j != numdim){
		error = err_push(ERR_NDARRAY, "Mismatched '[]'");
		goto ndarr_create_from_str_exit;
	}

	pos = strrchr(arraystr, ']');
	if(!pos){
		error = err_push(ERR_NDARRAY, "No [] to hold array description");
		goto ndarr_create_from_str_exit;
	}
	pos++;

	/* Allocate the descriptor struct */
	arrd = ndarr_create(numdim);
	if(!arrd){
		error = err_push(ERR_NDARRAY, "Unable to allocate array descriptor");
		goto ndarr_create_from_str_exit;
	}

	dim_names = (char **)memMalloc(sizeof(char *) * numdim, "dim_names");
	if (!dim_names)
	{
		error = err_push(ERR_NDARRAY, "Unable to allocate array descriptor");
		goto ndarr_create_from_str_exit;
	}

	skip_lead_space(pos);
#if 0
	pos = skip_lead_space(pos);
#endif
	arrd->element_size = (long)strtol(pos, &endptr, 10);
	if (endptr && strlen(endptr))
	{
		error = err_push(ERR_NDARRAY, "Internal: badly formatted element size string");
		goto ndarr_create_from_str_exit;
	}

	j = numdim - 1;
	for(i = strlen(arraystr); i >= 0; i--){
		if(arraystr[i] == '[')
			dim_names[j--] = arraystr + i + 1;
		if(arraystr[i] == ']')
			arraystr[i] = '\0';
	}

	for(i = 0; i < numdim; i++){
		position = dim_names[i];
		/* Create a name for the dimension */
		pos = strchr(position, '\"');
		if(pos){
			chpos = strchr(++pos, '\"');
			if(!chpos){
				error = err_push(ERR_NDARRAY, "Mismatched \"\"");
				goto ndarr_create_from_str_exit;
			}
			chpos[0] = '\0';
			
			if(ndarr_set(arrd, NDARR_DIM_NUMBER, (int)i, NDARR_DIM_NAME, pos, NDARR_END_ARGS)){
				error = err_push(ERR_NDARRAY, "Cannot set dimension name");
				goto ndarr_create_from_str_exit;
			}
			for(j = 0; pos[j]; j++){
				pos[j] = ' ';
			}
			pos[-1] = ' '; /* Get rid of first quote */

			chpos[0] = ' ';
		}
		else{
			error = err_push(ERR_NDARRAY, "Dimension not named");
			goto ndarr_create_from_str_exit;
		}

		/* Get the starting and ending indices */
		skip_lead_space(position);
#if 0
		position = skip_lead_space(position);
#endif
		if (dbin && IS_KEYWORDED_PARAMETER(position))
		{
			endptr = position + 1;
			while (!isspace((int)*endptr) && *endptr != ']')
				++endptr;

			save_char = *endptr;
			*endptr = STR_END;

			error = nt_ask(dbin, NT_ANYWHERE, position + 1, FFV_DOUBLE, &start);
			if (error)
			{
				error = err_push(ERR_UNKNOWN_PARAMETER, position + 1);
				goto ndarr_create_from_str_exit;
			}

			*endptr = save_char;
		}
		else
		{
			start = strtod(position, &endptr);
#if 1
			if (endptr && strlen(endptr) && !isspace((int)*endptr))
			{
				error = err_push(ERR_NDARRAY, "Badly formatted starting index for %s (%s)", arrd->dim_name[i], position);
				goto ndarr_create_from_str_exit;
			}
#endif
		}

		for(j = strlen(position) - 1; j >= 0; j--)
			position[j] = (char)tolower(position[j]);
		pos = strstr(position, "to");
		if(!pos){
			error = err_push(ERR_NDARRAY, "Missing \"to\" in dimension");
			goto ndarr_create_from_str_exit;
		}
		pos += 2;
		skip_lead_space(pos);
#if 0
		pos = skip_lead_space(pos);
#endif
		if (dbin && IS_KEYWORDED_PARAMETER(pos))
		{
			endptr = pos + 1;
			while (!isspace((int)*endptr) && *endptr != ']')
				++endptr;

			save_char = *endptr;
			*endptr = STR_END;

			error = nt_ask(dbin, NT_ANYWHERE, pos + 1, FFV_DOUBLE, &end);
			if (error)
			{
				error = err_push(ERR_UNKNOWN_PARAMETER, pos + 1);
				goto ndarr_create_from_str_exit;
			}

			*endptr = save_char;
		}
		else
		{
			end = strtod(pos, &endptr);
#if 1
			if (endptr && strlen(endptr) && !isspace((int)*endptr))
			{
				error = err_push(ERR_NDARRAY, "Badly formatted ending index for %s (%s)", arrd->dim_name[i], pos);
				goto ndarr_create_from_str_exit;
			}
#endif
		}

		/* Determine dimension granularity */
		pos = strstr(position, "by");
		if(pos){
			pos += 2;
			skip_lead_space(pos);
#if 0
			pos = skip_lead_space(pos);
#endif
			if (dbin && IS_KEYWORDED_PARAMETER(pos))
			{
				endptr = pos + 1;
				while (!isspace((int)*endptr) && *endptr != ']')
					++endptr;

				save_char = *endptr;
				*endptr = STR_END;

				error = nt_ask(dbin, NT_ANYWHERE, pos + 1, FFV_DOUBLE, &gran);
				if (error)
				{
					error = err_push(ERR_UNKNOWN_PARAMETER, pos + 1);
					goto ndarr_create_from_str_exit;
				}

				*endptr = save_char;
			}
			else
			{
				gran = strtod(pos, &endptr);
#if 1
				if (endptr && strlen(endptr) && !isspace((int)*endptr))
				{
					error = err_push(ERR_NDARRAY, "Badly formatted granularity for %s (%s)", arrd->dim_name[i], pos);
					goto ndarr_create_from_str_exit;
				}
#endif
			}
		}
		else
			gran = 1;

		/* Determine dimension separation */
		pos = strstr(position, NDARR_SB_KEY1);
		if(pos)
			pos += 10;
		if(!pos)
		{
			pos = strstr(position, NDARR_SB_KEY0);
			if(pos)
				pos += 2;
		}
		if(pos){
			skip_lead_space(pos);
#if 0
			pos = skip_lead_space(pos);
#endif
			if (dbin && IS_KEYWORDED_PARAMETER(pos))
			{
				endptr = pos + 1;
				while (!isspace((int)*endptr) && *endptr != ']')
					++endptr;

				save_char = *endptr;
				*endptr = STR_END;

				error = nt_ask(dbin, NT_ANYWHERE, pos + 1, FFV_DOUBLE, &sep);
				if (error)
				{
					error = err_push(ERR_UNKNOWN_PARAMETER, pos + 1);
					goto ndarr_create_from_str_exit;
				}

				*endptr = save_char;
			}
			else
			{
				sep = strtol(pos, &endptr, 10);
				if (endptr && strlen(endptr) && !isspace((int)*endptr))
				{
					error = err_push(ERR_NDARRAY, "Badly formatted separation for %s (%s)", arrd->dim_name[i], pos);
					goto ndarr_create_from_str_exit;
				}
			}
		}
		else
			sep = 0;

		/* Determine dimension grouping */
		pos = strstr(position, NDARR_GB_KEY1);
		if(pos)
			pos += 8;
		if(!pos)
		{
			pos = strstr(position, NDARR_GB_KEY0);
			if(pos)
				pos += 2;
		}
		if(pos){
			skip_lead_space(pos);
#if 0
			pos = skip_lead_space(pos);
#endif
			if (dbin && IS_KEYWORDED_PARAMETER(pos))
			{
				endptr = pos + 1;
				while (!isspace((int)*endptr) && *endptr != ']')
					++endptr;

				save_char = *endptr;
				*endptr = STR_END;

				error = nt_ask(dbin, NT_ANYWHERE, pos + 1, FFV_DOUBLE, &grp);
				if (error)
				{
					error = err_push(ERR_UNKNOWN_PARAMETER, pos + 1);
					goto ndarr_create_from_str_exit;
				}

				*endptr = save_char;
			}
			else
			{
				grp = strtod(pos, &endptr);
#if 1
				if (endptr && strlen(endptr) && !isspace((int)*endptr))
				{
					error = err_push(ERR_NDARRAY, "Badly formatted grouping for %s (%s)", arrd->dim_name[i], pos);
					goto ndarr_create_from_str_exit;
				}
#endif
			}
		}
		else
			grp = 0;

		if (fabs(gran) < 1)
		{
			start /= fabs(gran);
			end /= fabs(gran);

			start = (long)ROUND(start);
			end = (long)ROUND(end);

			grp /= fabs(gran);

			grp = (long)ROUND(grp);

			gran = (long)ROUND(gran / fabs(gran)); /* preserve sign */
		}

		if (dbin)
		{
			char grid_cell_registration[32];
			char data_type[32];

			/* This code parallels that in make_inputs_sub_desc_str and make_outputs_super_desc_str */

			error = nt_ask(dbin, NT_INPUT, "data_type", FFV_TEXT, data_type);
			if (error && error != ERR_NT_KEYNOTDEF)
				error = err_push(ERR_PARAM_VALUE, "for data_type (%s)", data_type);
			else if (!error)
			{
				if (!os_strcmpi(data_type, "image") || !os_strcmpi(data_type, "raster"))
				{
					error = nt_ask(dbin, NT_INPUT, "grid_cell_registration", FFV_TEXT, grid_cell_registration);
					if (error && error != ERR_NT_KEYNOTDEF)
						error = err_push(ERR_PARAM_VALUE, "for grid_cell_registration (%s)", grid_cell_registration);
					else if (!error)
					{
						if (!os_strncmpi(grid_cell_registration, "center", 6))
						{
							if (start < end)
								end -= 1;
							else
								start -= 1;
						}
					}
					else if (error == ERR_NT_KEYNOTDEF)
						error = 0;
				}
			}
			else if (error == ERR_NT_KEYNOTDEF)
				error = 0;
		}

		if(ndarr_set(arrd, NDARR_DIM_NUMBER, (int)i,
				NDARR_DIM_START_INDEX, (long)start,
				NDARR_DIM_END_INDEX, (long)end,
				NDARR_DIM_GRANULARITY, (long)gran,
				NDARR_DIM_GROUPING, (long)grp, 
				NDARR_DIM_SEPARATION, sep,
				NDARR_END_ARGS)){
			error = err_push(ERR_NDARRAY, "Unable to set dimension attributes");
			goto ndarr_create_from_str_exit;
		}
	}
	
	if(ndarr_do_calculations(arrd)){
		error = err_push(ERR_NDARRAY, "Unable to calculate array attributes");
		goto ndarr_create_from_str_exit;
	}

ndarr_create_from_str_exit:

	memFree(dim_names, "dim_names");

	if (error && arrd)
	{
		ndarr_free_descriptor(arrd);
		arrd = NULL;
	}

	return(arrd);
}


/*
 * NAME:        ndarr_do_calculations
 *
 * PURPOSE:     To calculate the values in an ARRAY_DESCRIPTOR struct
 *
 * USAGE:		int ndarr_do_calculations(ARRAY_DESCRIPTOR_PTR arrd)
 *
 * RETURNS:     int, 0 if all is OK, <> 0 on error.
 *
 * DESCRIPTION: Calculates values inside an ARRAY_DESCRIPTOR structure.
 *				The values calculated include dimension sizes, coeffecients,
 *				group sizes, index directions, etc.
 *
 *				This function MUST BE CALLED before attempting to use the 
 *				functions:
 *					ndarr_create_mapping
 *					ndarr_create_indices
 *					ndarr_increment_indices
 *					ndarr_increment_mapping
 *					ndarr_convert_indices
 *					ndarr_get_offset
 *					ndarr_get_mapped_offset
 *					ndarr_reorient
 *					ndarr_free_descriptor
 *					ndarr_free_indices
 *					ndarr_free_mapping
 *					ndarr_get_group
 *					ndarr_get_next_group
 *					ndarr_create_brkn_desc
 *					ndarr_set w/ NDARR_FILE_GROUPING or NDARR_BUFFER_GROUPING args
 *
 *				(In short, always call this function after done setting the array
 *				dimension specifications with ndarr_set).
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:	After calling ndarr_do_calculations, no further calls to ndarr_set
 *				should be made EXCEPT with the arguments NDARR_FILE_GROUPING or
 *				NDARR_BUFFER_GROUPING.
 *
 *				Call this function before using most other ndarr_ functions (see
 *				description section above).
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_do_calculations"
int ndarr_do_calculations(ARRAY_DESCRIPTOR_PTR arrd)
{
	int numdim;
	int i, j;
	
	numdim = arrd->num_dim;
	
	for(i = 0; i < numdim; i++){
		if(!arrd->dim_name[i]){
			err_push(ERR_NDARRAY, "Dimension not named");
			return(1);
		}

		if(arrd->granularity[i] < 0)
			arrd->granularity[i] *= -1;
		if(arrd->granularity == 0){
			err_push(ERR_NDARRAY, "Cannot have granularity of 0");
			return(1);
		}

		if(arrd->separation[i] < 0){
			err_push(ERR_NDARRAY, "Cannot have negative separation");
			return(1);
		}

		if(arrd->grouping[i] < 0){
			err_push(ERR_NDARRAY, "Cannot have negative grouping");
			return(1);
		}

		if(arrd->grouping[i]){
			arrd->type = NDARRT_BROKEN;
			for(j = 0; j < i; j++){
				if(!arrd->grouping[j]){
                   	err_push(ERR_NDARRAY, "Grouping in dimension without lower grouping");
					return(1);
				}
            }
		}

		/* Determine dimension index direction */
		if(arrd->end_index[i] < arrd->start_index[i])
			arrd->index_dir[i] = -1;
		else
			arrd->index_dir[i] = 1;

		/* Calculate the size of this dimension */
		arrd->dim_size[i] = (arrd->end_index[i] - arrd->start_index[i]) *
				(long)arrd->index_dir[i];

		arrd->dim_size[i] = (arrd->dim_size[i] / arrd->granularity[i]) + 1;
		
		if(!arrd->dim_size){
			err_push(ERR_NDARRAY, "Dimension without size");
			return(1);
		}
	}

	/* Calculate the coeffecient necessary to calculate the offsets later */
	arrd->coeffecient[numdim - 1] = (long)arrd->element_size +
			arrd->separation[numdim - 1];
	for(i = numdim - 2; i >= 0; i--)
		if(arrd->grouping[i + 1])
			arrd->coeffecient[i] = arrd->coeffecient[i + 1] * arrd->grouping[i + 1] +
					arrd->separation[i];
		else
			arrd->coeffecient[i] = arrd->coeffecient[i + 1] * arrd->dim_size[i + 1] +
					arrd->separation[i];

	/* See that all the groupings are OK */
	for(i = 0; i < numdim; i++)
		if(arrd->grouping[i])
			if(arrd->dim_size[i] % arrd->grouping[i]){
				err_push(ERR_NDARRAY, "Illegal grouping- dimension size/grouping mismatch");
				return(1);
			}

	/* Calculate the total number of elements in the array */
	arrd->total_elements = 1;
	for(i = 0; i < numdim; i++)
		arrd->total_elements *= (unsigned long)arrd->dim_size[i];

	/* Calculate total size (in bytes) of the array */
	arrd->contig_size = (long)arrd->total_elements * arrd->element_size;

	arrd->num_groups = 1;

	/* Determine group size and number of groups */
	if(arrd->type == NDARRT_BROKEN){
		arrd->group_size = (long)(arrd->coeffecient[0] * arrd->grouping[0]);
		for(i = numdim - 1; i >= 0; i--){
			if(arrd->grouping[i]){
				arrd->num_groups *= arrd->dim_size[i] / arrd->grouping[i];
			}
		}
	}
	else{
		arrd->group_size = (long)(arrd->coeffecient[0] * arrd->dim_size[0]);
	}

	/* Calculate total size (in bytes) of the array */
	arrd->total_size = (long)(arrd->group_size * arrd->num_groups);

	/* All calculations went OK, return 0 */
	return(0);
}


/*
 * NAME:        ndarr_set
 *
 * PURPOSE:     To set the values in an ARRAY_DESCRIPTOR struct
 *
 * USAGE:		int ndarr_set(ARRAY_DESCRIPTOR_PTR arrd, ..., NDARR_END_ARGS);
 *
 * RETURNS:     int, 0 if all is OK, <> 0 on error.
 *
 * DESCRIPTION: Sets values inside an ARRAY_DESCRIPTOR structure.  The first
 *				argument to this function (after the ARRAY_DESCRIPTOR_PTR)
 *				should be NDARR_DIM_NUMBER, followed by the dimension number
 *				of the attribute(s) you wish to set.  (This is not the case if 
 *				setting element size, file or buffer grouping).
 *
 *				The arguments this function recognizes are:
 *    (attribute to set)    (arguments)
 *    NDARR_DIM_NUMBER      (int)current dimension number to set
 *    NDARR_DIM_NAME        (char *)name of the current dimension
 *    NDARR_DIM_START_INDEX (long)starting index of current dimension
 *    NDARR_DIM_END_INDEX   (long)ending index of current dimension
 *    NDARR_DIM_GRANULARITY (long)granularity of current dimension
 *    NDARR_DIM_GROUPING    (long)grouping of current dimension
 *    NDARR_DIM_SEPARATION  (long)separation of current dimension
 *    NDARR_ELEMENT_SIZE    (long)size of an individual array element, in bytes
 *    NDARR_FILE_GROUPING   NDARR_MAP_IN_BUFFER or NDARR_MAP_IN_FILE, followed by
 *                          (char **)array of filenames (for NDARR_MAP_IN_BUFFER) or
 *                          (char *)filename (for NDARR_MAP_IN_FILE).
 *    NDARR_BUFFER_GROUPING (void **)array of buffers
 *    NDARR_END_ARGS        NO ARGUMENT- ends the current session of setting
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:	After calling ndarr_do_calculations, no further calls to ndarr_set
 *				should be made EXCEPT with the arguments NDARR_FILE_GROUPING or
 *				NDARR_BUFFER_GROUPING.
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_set"
int ndarr_set(ARRAY_DESCRIPTOR_PTR arrd, ...)
{
	va_list args;
	int  attribute;
	int  dimnum = -1;
	char *name = NULL;
	char **fnarray = NULL;
	void **bfarray = NULL;

	assert(arrd);
	
	va_start(args, arrd);

	while ((attribute = va_arg(args, int)) != NDARR_END_ARGS ){
		switch(attribute){
			case NDARR_DIM_NUMBER:
				dimnum = va_arg(args, int);
				if((dimnum < 0) || (dimnum >= arrd->num_dim)){
					err_push(ERR_NDARRAY, "Illegal dimension number");
					return(1);
				}
				break;
				
			case NDARR_DIM_NAME:
				name = va_arg(args, char *);
				if(dimnum < 0){
					err_push(ERR_NDARRAY, "Dimension number unset");
					return(1);
				}
				if(!name){
					err_push(ERR_NDARRAY, "Illegal dimension name");
					return(1);
				}                                                   
				if(!(arrd->dim_name[dimnum] = (char *)memMalloc((size_t)(strlen(name)+5), "arrd->dim_name[dimnum]"))){
					err_push(ERR_NDARRAY, "Out of memory");
					return(1);
				}
				strcpy(arrd->dim_name[dimnum], name);				
				break;
				
			case NDARR_DIM_START_INDEX:
				if(dimnum < 0){
					err_push(ERR_NDARRAY, "Dimension number unset");
					return(1);
				}
				arrd->start_index[dimnum] = va_arg(args, long);
				break;
				
			case NDARR_DIM_END_INDEX:
				if(dimnum < 0){
					err_push(ERR_NDARRAY, "Dimension number unset");
					return(1);
				}
				arrd->end_index[dimnum] = va_arg(args, long);
				break;
				
			case NDARR_DIM_GRANULARITY:
				if(dimnum < 0){
					err_push(ERR_NDARRAY, "Dimension number unset");
					return(1);
				}
				arrd->granularity[dimnum] = va_arg(args, long);
				break;
				
			case NDARR_DIM_GROUPING:
				if(dimnum < 0){
					err_push(ERR_NDARRAY, "Dimension number unset");
					return(1);
				}
				arrd->grouping[dimnum] = va_arg(args, long);
				break;
				
			case NDARR_DIM_SEPARATION:
				if(dimnum < 0){
					err_push(ERR_NDARRAY, "Dimension number unset");
					return(1);
				}
				arrd->separation[dimnum] = va_arg(args, long);
				break;
				
			case NDARR_ELEMENT_SIZE:
				arrd->element_size = va_arg(args, long);
				if(arrd->element_size < 1){
					err_push(ERR_NDARRAY, "Illegal element size");
					return(1);
				}
				break;
				
			case NDARR_FILE_GROUPING:
				attribute = va_arg(args, int);
				switch(attribute){
					case NDARR_MAP_IN_BUFFER:
						fnarray = va_arg(args, char **);
						if(ndarr_create_brkn_desc(arrd, NDARR_MAP_IN_BUFFER, (void *)fnarray)){
							err_push(ERR_NDARRAY, "Unable to create mapping from buffer");
							return(1);
						}
						break;
						
					case NDARR_MAP_IN_FILE:
						name = va_arg(args, char *);
						if(ndarr_create_brkn_desc(arrd, NDARR_MAP_IN_FILE, (void *)name)){
							err_push(ERR_NDARRAY, "Unable to create mapping from file");
							return(1);
						}
						break;
						
					default:
						err_push(ERR_NDARRAY, "Unknown argument to NDARR_FILE_GROUPING");
						return(1);
				}
				break;
				
			case NDARR_BUFFER_GROUPING:
				bfarray = va_arg(args, void **);
				if(ndarr_create_brkn_desc(arrd, NDARR_BUFFER_GROUPING, (void *)bfarray)){
					err_push(ERR_NDARRAY, "Unable to create mapping from buffer");
					return(1);
				}
				
			default:
				err_push(ERR_NDARRAY, "Unknown argument");
				return(1);
		}
	}	
	
	va_end(args);

	/* All is OK */	
	return(0);
}

/*
 * NAME:        ndarr_free_descriptor
 *
 * PURPOSE:     To free an array descriptor structure.
 *
 * USAGE:		void ndarr_free_descriptor(ARRAY_DESCRIPTOR_PTR arrdesc)
 *
 * RETURNS:     void
 *
 * DESCRIPTION: Frees an array descriptor structure.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_free_descriptor"
void ndarr_free_descriptor(ARRAY_DESCRIPTOR_PTR arrdesc)
{
	int i;

	assert(arrdesc);

	if(arrdesc->dim_name)
	{
		for(i = 0; i < arrdesc->num_dim; i++)
		{
			if (arrdesc->dim_name[i])
				memFree(arrdesc->dim_name[i], "arrdesc->dim_name[i]");
		}

		memFree(arrdesc->dim_name, "arrdesc->dim_name");
	}
	if(arrdesc->start_index)
		memFree(arrdesc->start_index, "arrdesc->start_index");
	if(arrdesc->end_index)
		memFree(arrdesc->end_index, "arrdesc->end_index");
	if(arrdesc->granularity)
		memFree(arrdesc->granularity, "arrdesc->granularity");
	if(arrdesc->index_dir)
		memFree(arrdesc->index_dir, "arrdesc->index_dir");
	if(arrdesc->dim_size)
		memFree(arrdesc->dim_size, "arrdesc->dim_size");
	if(arrdesc->coeffecient)
		memFree(arrdesc->coeffecient, "arrdesc->coeffecient");
	if(arrdesc->extra_index)
		ndarr_free_indices((ARRAY_INDEX_PTR)arrdesc->extra_index);

		
	if(arrdesc->type == NDARRT_BROKEN)
		if(arrdesc->extra_info)
			ndarr_free_descriptor((ARRAY_DESCRIPTOR_PTR)arrdesc->extra_info);

	if(arrdesc->type == NDARRT_GROUPMAP_FILE)
		if(arrdesc->extra_info){
			char **fnarr;
		
			fnarr = (char **)arrdesc->extra_info;
			for(i = 0; i < arrdesc->total_elements; i++)
				memFree(fnarr[i], "fnarr[i]");
			memFree(fnarr, "fnarr");
		}

	if (arrdesc->grouping)
		memFree(arrdesc->grouping, "arrdesc->grouping");

	if (arrdesc->separation)
		memFree(arrdesc->separation, "arrdesc->separation");

#ifdef ND_FP 
	if (arrdesc->fp)
		fclose(arrdesc->fp);
#endif

	memFree(arrdesc, "arrdesc");
}

/*
 * NAME:    ndarr_create_indices
 *
 * PURPOSE: To create and initialize array indices
 *
 * USAGE:	ARRAY_INDEX_PTR ndarr_create_indices(ARRAY_DESCRIPTOR_PTR arrdesc)
 *
 * RETURNS:	ARRAY_INDEX_PTR to the newly created array indices
 *
 * DESCRIPTION:  Creates an ARRAY_INDEX struct, initializes the indices to 0,
 *					and sets the descriptor pointer to arrdesc.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_create_indices"
ARRAY_INDEX_PTR ndarr_create_indices(ARRAY_DESCRIPTOR_PTR arrdesc)
{
	int i;
	ARRAY_INDEX_PTR aindex;

	assert(arrdesc);

	if(!(aindex = (ARRAY_INDEX_PTR)memMalloc(sizeof(ARRAY_INDEX), "aindex"))){
		err_push(ERR_NDARRAY, "Out of memory");
		return(NULL);
	}
	if(!(aindex->index = (long *)memMalloc((size_t)(sizeof(long) * arrdesc->num_dim), "aindex->index"))){
		err_push(ERR_NDARRAY, "Out of memory");
		return(NULL);
	}
	aindex->descriptor = arrdesc;
	for(i = 0; i < aindex->descriptor->num_dim; i++){
		aindex->index[i] = 0;
	}

	return(aindex);
}

/*
 * NAME:	ndarr_get_offset
 *
 * PURPOSE: To determine the offset from the beginning of the array of
 *			the specified element.
 *
 * USAGE:	unsigned long ndarr_get_offset(ARRAY_INDEX_PTR aindex)
 *
 * RETURNS: unsigned long, the offset from the beginning of the array
 *			(in bytes) that the element pointed to by aindex resides.
 *
 * DESCRIPTION: Preforms calculations to determine the offset, in bytes,
 *				of the element specified by the array indices.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_get_offset"
unsigned long ndarr_get_offset(ARRAY_INDEX_PTR aindex)
{
	unsigned long byte_offset = 0;
	int i;

	assert(aindex);

	/* Calculate position in array */
	if(aindex->descriptor->type == NDARRT_BROKEN)
	{
		for(i = 0; i < aindex->descriptor->num_dim; i++)
		{
			if(aindex->descriptor->grouping[i])
				byte_offset += aindex->descriptor->coeffecient[i] * (aindex->index[i] % aindex->descriptor->grouping[i]);
			else
				byte_offset += aindex->descriptor->coeffecient[i] * aindex->index[i];
		}
	}
	else
	{
		for(i = 0; i < aindex->descriptor->num_dim; i++)
			byte_offset += aindex->descriptor->coeffecient[i] * aindex->index[i];
	}
	return(byte_offset);
}

/*
 * NAME:    ndarr_increment_indices
 *
 * PURPOSE:	to increment the indices given by 1 element.
 *
 * USAGE:	ARRAY_INDEX_PTR ndarr_increment_indices(ARRAY_INDEX_PTR aindex)
 *
 * RETURNS:	a pointer to the array indices, or NULL if the indices have
 *			wrapped back to the origion.
 *
 * DESCRIPTION: ndarr_increment_indices increments the index to the next
 *				 memory location. If the next indices are the origion again,
 *				 it returns NULL to signify that the end of the array has
 *				 been reached.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_increment_indices"
ARRAY_INDEX_PTR ndarr_increment_indices(ARRAY_INDEX_PTR aindex)
{
	int i;

	assert(aindex);

	/* The for-loop is so that if we have reached the maximum size of the
	 * particular dimension of the array, we reset it to 0 and increment
	 * the next-lower dimension.  Eventually we will wrap-back to 0,0,0...
	 * When that happens, return NULL */
	for(i = aindex->descriptor->num_dim - 1; i >= 0; i--){
		aindex->index[i] = (aindex->index[i] + 1) %
				aindex->descriptor->dim_size[i];
		if(aindex->index[i])
			return(aindex);
	}
	return(NULL); /* We have wrapped-back to the origion */
}

/*
 * NAME:    ndarr_increment_mapping
 *
 * PURPOSE:	to increment the indices given by the maximum dimension
 *				given in the ARRAY_MAPPING structure.
 *
 * USAGE:	ARRAY_INDEX_PTR ndarr_increment_mapping(ARRAY_MAPPING_PTR amap)
 *
 * RETURNS:	a pointer to the array indices, or NULL if the indices have
 *			wrapped back to the origion.
 *
 * DESCRIPTION: ndarr_increment_indices increments the amap->subaindex to the
 *				next dimension as given in the ARRAY_MAPPING->dimincrement.
 *				If the next indices are the origion again, it returns NULL
 *				to signify that the end of the array has been reached.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_increment_mapping"
ARRAY_INDEX_PTR ndarr_increment_mapping(ARRAY_MAPPING_PTR amap)
{
	int i;

	assert(amap);

	/* The for-loop is so that if we have reached the maximum size of the
	 * particular dimension of the array, we reset it to 0 and increment
	 * the next-lower dimension.  Eventually we will wrap-back to 0,0,0...
	 * When that happens, return NULL */
	for(i = amap->dimincrement; i >= 0; i--){
		amap->subaindex->index[i] = (amap->subaindex->index[i] + 1) %
				amap->subaindex->descriptor->dim_size[i];
		if(amap->subaindex->index[i])
			return(amap->subaindex);
	}
	return(NULL); /* We have wrapped-back to the origion */
}

/*
 * NAME:	ndarr_free_indices
 *
 * PURPOSE:	To free an ARRAY_INDEX structure
 *
 * USAGE:   void ndarr_free_indices(ARRAY_INDEX_PTR aindex)
 *
 * RETURNS: void
 *
 * DESCRIPTION: frees the indicated ARRAY_INDEX structure
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_free_indices"
void ndarr_free_indices(ARRAY_INDEX_PTR aindex)
{
	assert(aindex);

	if(aindex->index)
		memFree(aindex->index, "aindex->index");
	memFree(aindex, "aindex");
}

/*
 * NAME:	ndarr_copy_indices
 *
 * PURPOSE: To copy a set of indices.
 *
 * USAGE:   ARRAY_INDEX_PTR ndarr_copy_indices(ARRAY_INDEX_PTR source,
 *				ARRAY_INDEX_PTR dest)
 *
 * RETURNS: A pointer to the ARRAY_INDEX structure that was copied into,
 *				or NULL on error.
 *
 * DESCRIPTION: Copies the indices given in source to dest; if dest is NULL,
 *				then a destination index is created (and its pointer returned).
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_copy_indices"
ARRAY_INDEX_PTR ndarr_copy_indices(ARRAY_INDEX_PTR source, ARRAY_INDEX_PTR dest)
{
	int i;
	assert(source);

	if(!dest){
		dest = ndarr_create_indices(source->descriptor);
		if(!dest){
			err_push(ERR_NDARRAY, "Unable to create copy of indices");
			return(NULL);
		}
	}
	
	for(i = 0; i < source->descriptor->num_dim; i++){
		dest->index[i] = source->index[i];
	}
	return(dest);
}

/*
 * NAME:	ndarr_convert_indices
 *
 * PURPOSE: To convert indices between user-defined and "real" indices.
 *
 * USAGE:   ARRAY_INDEX_PTR ndarr_convert_indices(ARRAY_INDEX_PTR aindex,
 *				unsigned char direction)
 *			    where direction is either NDARR_REAL_TO_USER or
 *              NDARR_USER_TO_REAL
 *
 * RETURNS: A pointer to the passed ARRAY_INDEX structure, or NULL on error.
 *
 * DESCRIPTION: Converts the indices given between "real" and "user-defined",
 *				or vice-versa.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_convert_indices"
ARRAY_INDEX_PTR ndarr_convert_indices(ARRAY_INDEX_PTR aindex, unsigned char direction)
{
	int i;
	assert(aindex && direction);

	switch(direction){
		case NDARR_USER_TO_REAL:
			for(i = 0; i < aindex->descriptor->num_dim; i++){
				aindex->index[i] = aindex->index[i] *
						aindex->descriptor->index_dir[i] -
						aindex->descriptor->start_index[i] *
						aindex->descriptor->index_dir[i];
				if(aindex->index[i] % aindex->descriptor->granularity[i]){
					err_push(ERR_NDARRAY, "Illegal indices- granularity mismatch");
					return(NULL);
				}
				aindex->index[i] /= aindex->descriptor->granularity[i];
				if((aindex->index[i] < 0) ||
						(aindex->index[i] >= aindex->descriptor->dim_size[i])){
					err_push(ERR_NDARRAY, "Indices out of bounds");
					return(NULL);
				}
			}
			break;
		case NDARR_REAL_TO_USER:
			for(i = 0; i < aindex->descriptor->num_dim; i++)
				aindex->index[i] = aindex->index[i] *
						aindex->descriptor->granularity[i] *
						aindex->descriptor->index_dir[i] +
						aindex->descriptor->start_index[i];
			break;
		default:
			err_push(ERR_NDARRAY, "Unknown conversion type");
			return(NULL);
	}

	return(aindex);
}

/*
 * NAME:	ndarr_create_mapping
 *
 * PURPOSE: To create an ARRAY_MAPPING structure
 *
 * USAGE:	ARRAY_MAPPING_PTR ndarr_create_mapping(
 *			  ARRAY_DESCRIPTOR_PTR subarray, ARRAY_DESCRIPTOR_PTR superarray)
 *
 * RETURNS:	ARRAY_MAPPING_PTR to the created structure, or NULL on error.
 *
 * DESCRIPTION:	Creates an ARRAY_MAPPING structure, which describes exactly
 *				how elements in one array map to elements in another.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_create_mapping"
ARRAY_MAPPING_PTR ndarr_create_mapping(ARRAY_DESCRIPTOR_PTR subarray, ARRAY_DESCRIPTOR_PTR superarray)
{
	int i, j;
	long max, min, tmp;
	ARRAY_MAPPING_PTR amap;

	assert(subarray && superarray);

	if(subarray->num_dim != superarray->num_dim){
		err_push(ERR_NDARRAY, "Mapping onto array of different dimension");
		return(NULL);
	}

	if(subarray->element_size != superarray->element_size){
		err_push(ERR_NDARRAY, "Mapping onto array of differing element size");
		return(NULL);
	}

	if(!(amap = (ARRAY_MAPPING_PTR)memMalloc(sizeof(ARRAY_MAPPING), "amap"))){
		err_push(ERR_NDARRAY, "Out of memory");
		return(NULL);
	}

	amap->super_array = superarray;
	amap->sub_array = subarray;
	amap->dim_mapping = NULL;
	amap->gran_mapping = NULL;
	amap->gran_div_mapping = NULL;
	amap->cacheing = NULL; 
	amap->subaindex = NULL;
	amap->index_mapping = NULL;
	amap->index_dir = NULL;
	amap->subsep = 0;
	amap->fcreated = 0;

	amap->aindex = ndarr_create_indices(superarray);
	if(!amap->aindex){
		err_push(ERR_NDARRAY, "Error creating extra indices");
		ndarr_free_mapping(amap);
	}

	amap->subaindex = ndarr_create_indices(subarray);
	if(!amap->subaindex){
		err_push(ERR_NDARRAY, "Error creating extra indices");
		ndarr_free_mapping(amap);
	}

	if(!(amap->dim_mapping = (int *)memMalloc((size_t)(sizeof(int) * subarray->num_dim), "amap->dim_mapping"))){
		err_push(ERR_NDARRAY, "Out of memory");
		ndarr_free_mapping(amap);
		return(NULL);
	}

	if(!(amap->index_mapping = (long *)memMalloc((size_t)(sizeof(long) * subarray->num_dim), "amap->index_mapping"))){
		err_push(ERR_NDARRAY, "Out of memory");
		ndarr_free_mapping(amap);
		return(NULL);
	}

	if(!(amap->gran_mapping = (long *)memMalloc((size_t)(sizeof(long) * subarray->num_dim), "amap->gran_mapping"))){
		err_push(ERR_NDARRAY, "Out of memory");
		ndarr_free_mapping(amap);
		return(NULL);
	}

	if(!(amap->gran_div_mapping = (long *)memMalloc((size_t)(sizeof(long) * subarray->num_dim), "amap->gran_div_mapping"))){
		err_push(ERR_NDARRAY, "Out of memory");
		ndarr_free_mapping(amap);
		return(NULL);
	}

	if(!(amap->cacheing = (long *)memMalloc((size_t)(sizeof(long) * subarray->num_dim), "amap->cacheing"))){
		err_push(ERR_NDARRAY, "Out of memory");
		ndarr_free_mapping(amap);
		return(NULL);
	}

	if(!(amap->index_dir = (char *)memMalloc((size_t)(sizeof(char) * subarray->num_dim), "amap->index_dir"))){
		err_push(ERR_NDARRAY, "Out of memory");
		ndarr_free_mapping(amap);
		return(NULL);
	}

	/* Find the labels for matching all the dimensions */
	for(i = 0; i < subarray->num_dim; i++){
		for(j = 0; j < subarray->num_dim; j++)
			if(!strcmp(subarray->dim_name[i], superarray->dim_name[j]))
				break;
		if(j == subarray->num_dim){
			err_push(ERR_NDARRAY, "Matching dimension label not found");
			ndarr_free_mapping(amap);
			return(NULL);
		}
		amap->dim_mapping[i] = j;
	}

	for(i = 0; i < subarray->num_dim; i++){
		amap->subsep += subarray->separation[i];
		/* Check to make sure that the sub-array's indices are indeed
		 * a subset of the super-array's */
		j = amap->dim_mapping[i];
		if(((subarray->start_index[i] < superarray->start_index[j]) &&
				(subarray->start_index[i] < superarray->end_index[j])) ||
				((subarray->start_index[i] > superarray->start_index[j]) &&
				(subarray->start_index[i] > superarray->end_index[j])) ||
				((subarray->end_index[i] > superarray->start_index[j]) &&
				(subarray->end_index[i] > superarray->end_index[j])) ||
				((subarray->end_index[i] < superarray->start_index[j]) &&
				(subarray->end_index[i] < superarray->end_index[j]))){
			err_push(ERR_NDARRAY, "Subarray indices out of bounds");
			ndarr_free_mapping(amap);
			return(NULL);
		}

		/* Find an appropriate starting position for the mapping */
		amap->index_mapping[i] = subarray->start_index[i] -
				superarray->start_index[j];
		if(amap->index_mapping[i] < 0)
			amap->index_mapping[i] *= -1;
		amap->index_mapping[i] /= superarray->granularity[j]; /* kbf fix */
        
        if(subarray->granularity[i] > superarray->granularity[j]){
			if(subarray->granularity[i] % superarray->granularity[j]){
				err_push(ERR_NDARRAY, "Granularity mismatch");
				ndarr_free_mapping(amap);
				return(NULL);
			}

			amap->gran_mapping[i] = subarray->granularity[i] /
					superarray->granularity[j];
			amap->gran_div_mapping[i] = 1;
		}
		else if(subarray->granularity[i] < superarray->granularity[j]){
			if(superarray->granularity[j] % subarray->granularity[i]){
				err_push(ERR_NDARRAY, "Granularity mismatch");
				ndarr_free_mapping(amap);
				return(NULL);
			}

			amap->gran_div_mapping[i] = superarray->granularity[j] /
					subarray->granularity[i];
			amap->gran_mapping[i] = 1;
		}
		else{
			amap->gran_mapping[i] = 1;
			amap->gran_div_mapping[i] = 1;
		}

		assert(abs(subarray->index_dir[i] * superarray->index_dir[j]) < SCHAR_MAX);
		amap->index_dir[i] = (char)(subarray->index_dir[i] * superarray->index_dir[j]);
	}

	/* Find the lowest unreoriented dimension */
	for(i = subarray->num_dim - 1; i >= 0; i--){
		if((amap->index_dir[i] != 1) ||
				(amap->gran_div_mapping[i] != 1) ||
				(amap->gran_mapping[i] != 1) || (amap->dim_mapping[i] != i) ||
				(subarray->grouping[i]) || (superarray->grouping[i]) ||
				(subarray->separation[i]) || (superarray->separation[i]))
			break;
		if(i < (subarray->num_dim - 1)){
			j = i + 1;
			if(subarray->dim_size[j] != superarray->dim_size[j]){
				break;
			}
		}
	}
	
	amap->necessary = 1;
    
	if(i < 0){
		amap->dimincrement = -1;
		amap->increment_block = subarray->total_size;
	}
	else{
		amap->dimincrement = i;
		amap->increment_block = subarray->coeffecient[i] - subarray->separation[i];
		if(i == subarray->num_dim - 1)
			amap->increment_block = subarray->element_size; 
	}
	
	
	/* See if this mapping is really necessary */
	for(i = 0; i < subarray->num_dim; i++){
		if((amap->index_dir[i] != 1) || (amap->gran_div_mapping[i] != 1) ||
				(amap->gran_mapping[i] != 1) || (amap->dim_mapping[i] != i) ||
				(subarray->start_index[i] != superarray->start_index[i]) ||
				(subarray->end_index[i] != superarray->end_index[i]))
			break;
	}
	
	if(i == subarray->num_dim){
		amap->necessary = 0; /* No mapping is really necessary */
		/* NOTE: by 'necessary', we mean that no reorientation, thinning,
		 * flipping, subsetting or pixel replication is taking place.
		 * other goings-on (separation, grouping) are not considered. */
	}


	/* Now find the minimum and maximum offsets in the superarray that will
	 * be used by the subarray- we do this by finding corners */	
	max = 0;
	min = (long)amap->super_array->total_size;
	
	for(;;){
		/* Find the vertices of the array */
		for(i = subarray->num_dim - 1; i >= 0; i--){
			if(amap->subaindex->index[i])
				amap->subaindex->index[i] = 0;
			else{
				amap->subaindex->index[i] = amap->sub_array->dim_size[i] - 1; 
				if(amap->subaindex->index[i])
					break;
			}
		}
		if(i < 0)
			break; /* All done!!! */
		tmp = ndarr_get_mapped_offset(amap);
		if(tmp < min)
			min = tmp;
		if(tmp > max)
			max = tmp;
	}
	
	amap->superastart = min;
	amap->superaend = max;

	return(amap);
}

/*
 * NAME:    ndarr_free_mapping
 *
 * PURPOSE:	to free an ARRAY_MAPPING struct.
 *
 * USAGE:	void ndarr_free_mapping(ARRAY_MAPPING_PTR amap)
 *
 * RETURNS: void
 *
 * DESCRIPTION: Frees the indicated ARRAY_MAPPING struct.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_free_mapping"
void ndarr_free_mapping(ARRAY_MAPPING_PTR amap)
{
	assert(amap);

	if(amap->dim_mapping)
		memFree(amap->dim_mapping, "amap->dim_mapping");
	if(amap->index_mapping)
		memFree(amap->index_mapping, "amap->index_mapping");
	if(amap->gran_mapping)
		memFree(amap->gran_mapping, "amap->gran_mapping");
	if(amap->gran_div_mapping)
		memFree(amap->gran_div_mapping, "amap->gran_div_mapping");
	if(amap->index_dir)
		memFree(amap->index_dir, "amap->index_dir");
	if(amap->cacheing)
		memFree(amap->cacheing, "amap->cacheing");
	if(amap->aindex)
		ndarr_free_indices(amap->aindex);
	if(amap->subaindex)
		ndarr_free_indices(amap->subaindex);
	memFree(amap, "amap");
}

/*
 * NAME:	ndarr_get_mapped_offset
 *
 * PURPOSE:	To determine where in the superarray the element named in
 *			amap->suba_index resides (offset in bytes).
 *
 * USAGE:	unsigned long ndarr_get_mapped_offset(ARRAY_MAPPING_PTR amap)
 *
 * RETURNS: unsigned long, offset of element in the superarray
 *
 * DESCRIPTION: ndarr_get_mapped_offset uses an ARRAY_MAPPING struct to
 *				 determine where the indices supplied in amap->suba_index
 *				 would fall on the superarray.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_get_mapped_offset"
unsigned long ndarr_get_mapped_offset(ARRAY_MAPPING_PTR amap)
{
	int i;

	assert(amap);

	for(i = 0; i < amap->sub_array->num_dim; i++){
		amap->aindex->index[amap->dim_mapping[i]] =
				amap->index_mapping[i] +
				amap->index_dir[i] * amap->subaindex->index[i] *
				amap->gran_mapping[i] / amap->gran_div_mapping[i];
	}

	return(ndarr_get_offset(amap->aindex));
}

/*
 * NAME:	ndarr_reorient
 *
 * PURPOSE:	To reorient the superarray, stored in a file or a cache, into
 *			a subarray, stored in a file or a cache. Also preforms
 *			basic array I/O (no reorientation).
 *
 * USAGE:   long ndarr_reorient(ARRAY_MAPPING_PTR amap,
 *				NDARR_SOURCE sourceid,  void *source,  long source_size,
 *				NDARR_SOURCE destid,    void *dest,    long dest_size,
 *				int *array_complete)
 *
 * RETURNS: (unsigned long) bytes transferred, < 0 on error.
 *
 * DESCRIPTION: Uses the ARRAY_MAPPING struct to reorient a superarray into
 *		a subarray. (Source is the superarray, destination is
 *		the subarray).  Source and destination are determined
 *		by the NDARR_SOURCE bitfields.  Source and destination
 *		can be (independantly) file(s) or buffer(s).  The mode
 *		files are opened for, as well as any padding needed for
 *      array separation is also stored in the bitfields.
 *
 *		In general, the arguments sourceid and destid are used to
 *		specify how and where to read or write the array; source
 *		and dest are more specific locations; and source_size and
 *		dest_size are (for files) the length of a header or (for
 *		buffers) the size of the buffer.
 *
 *		If the NDARRS_FILE bit is set in sourceid or destid,
 *		the source or destination (respectively) is assumed
 *		to be a file.  If the NDARRS_BUFFER bit is set, the
 *		source or destination is assumed to be a buffer.
 *
 *		The NDARRS_APPEND, NDARRS_UPDATE, NDARRS_CREATE and
 *		NDARRS_PADDING bits are ignored for source, and are
 *		only used for destination if the NDARRS_FILE bit is
 *		set.
 *
 *		If the source array (superarray) or destination array
 *		(subarray) are broken (have grouping fields other than 0),
 *		and the descriptor extra_info pointer is NULL (no groupmap
 *		has been associated), a groupmap is created-
 *			if the NDARRS_FILE bit is set, the source or dest argument
 *				is taken as a char *- a filename.  This file is assumed
 *				to contain the list of group files.  For more information
 *				on the structure of this file, see the function header
 *				for ndarr_create_brkn_desc().  The source_size or dest_size
 *				arguments (normally taken to be the length of a header in the
 *				file case) are taken to be the length of the header on each
 *				of the group files.
 *			if the NDARRS_BUFFER bit is set, the source or dest argument
 *				is taken as a void **- an array of pointers to buffers.
 *				The source_size or dest_size arguments (normally taken to be the
 *				size of the buffer in the buffer case) are taken to be the
 *				size of each of the group buffers.
 *
 *		If the NDARRS_FILE bit is set for destid,
 *			NDARRS_APPEND signifies to open the output file(s) for
 *				binary append ("ab").  dest_size (normally the size
 *				of the header on the file) is ignored, as appending to
 *				the file will start after the header anyway.  The
 *				padding character specified by NDARRS_PADDING is used
 *				when separation is written to the file.
 *			NDARRS_UPDATE signifies to open the output file(s) for
 *				binary update ("r+b").  dest_size is taken to be
 *				the length of a file header to be seeked past.
 *				Padding is not used, any separation is simply seeked past.
 *			NDARRS_CREATE signifies to open the output file(s) for
 *				binary create ("wb").  dest_size is taken to be the
 *				length of a file header (written out using the padding
 *				character) to create on the front of the file.  The
 *				padding character specified by NDARRS_PADDING is used
 *				when separation is written to the file.
 *			NDARRS_PADDING specifies the padding character to use for
 *				separation in output.  If the output file was specified
 *				NDARRS_UPDATE, padding is not used (the separation is
 *				seeked past).  To set the padding character, use the
 *				NDARR_SET_PADDING macro defined in <ndarray.h>
 *
 *		Special conditions exist for each of the four input/output
 *		cases:
 *
 *		source    dest
 * 		FILE   to FILE  :  No restrictions on size exist.  The int stored
 *							at *(array_complete) will be set to 0 if the
 *							reorientation was unsuccessful, 1 if successful.
 *							The source and dest arguments are both interpreted
 *							as (char *) filenames; source_size and dest_size
 *							are taken as lengths of headers on the files
 *							(see above discussion of how file headers
 *							on output are handled for each of the file modes).
 * 		FILE   to BUFFER:  At most dest_size bytes will be transferred to
 *							the buffer.  If the entire array cannot fit
 *							into the buffer, subsequent calls to
 *							ndarr_reorient will fill the buffer with
 *							the remaining portions of the array.  When
 *							the entire array has been processed, the
 *							int stored at *(array_complete) will be set
 *							to 1.  If the destination array is broken
 *							across buffers, then each buffer must be
 *							at least (subarray->group_size) bytes in
 *							size (the entire array will then be read
 *							in in one call).  The source argument is
 *							interpreted as a (char *) filename; dest is taken
 *							as a (void *) buffer.  The source_size argument
 *							is used as the length of the header on the input
 *							file(s); dest_size is the size of the output
 *							buffer(s).
 * 		BUFFER to FILE  :  If any reorientation is to take place,
 *							the entire array must be contained in the
 *							buffer(s).  If no reorientation is taking place
 *							and the entire array is not contained in the
 *							buffer, subsequent calls to ndarr_reorient
 *							will continue to output the array to the file(s).
 *							When the entire array has been written out, the
 *							int stored at *(array_complete) will be set
 *							to 1.  If the source array is broken
 *							across buffers, then each buffer must be
 *							at least (superarray->group_size) bytes in
 *							size (the entire array will then be written
 *							in in one call).  The source argument is
 *							interpreted as a (void *) buffer; dest is taken as
 *							a (char *) filename.  The source_size argument is
 *							used as the size of the input buffer(s), while
 *							dest_size is the length of file header(s) on
 *							output (see above discussion of how file headers
 *							on output are handled for each of the file modes).
 * 		BUFFER to BUFFER:  If any reorientation is to take place, the
 *							entire array must be contained in the input and 
 *							output buffer(s).  If no reorientation is taking
 *							place, it is senseless to call this function, as
 *							the desired effect can be achieved with a call
 *							to memcpy().  The source and dest arguments are
 *							interpreted as (void *) buffers, and source_size
 *							and dest_size are taken to be the size of the input
 *							and output buffer(s), respectively.  The int stored
 *							at *(array_complete) will be set to 0 if the
 *							reorientation was unsuccessful, 1 if successful.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:   Special conditions exist for each of the four input/output
 *				combinations; see DESCRIPTION section above.
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_reorient"
long ndarr_reorient(ARRAY_MAPPING_PTR amap, 
		NDARR_SOURCE sourceid,  void *source,  long source_size,
		NDARR_SOURCE destid,    void *dest,    long dest_size,
		int *array_complete)
{
	long i;
	long bytesread = 0;
	FILE *infile = NULL;
	FILE *outfile = NULL;
	char *fmode = NULL;
	char *infilename = NULL;
	char *newinfn = NULL;
	char *outfilename = NULL;
	char *newoutfn = NULL;
	char *buffer = NULL;
	char *bufferpos = NULL;
	char *inbuffer = NULL;
	char *inbufferpos = NULL;
	char paddingch;
	char needtopad = 0;
	unsigned long offset = 0;
	long outoffset = 0;
	long startoffset = 0;
	long min_buff_size = 0;
	ARRAY_DESCRIPTOR_PTR sub_array = NULL;
	ARRAY_DESCRIPTOR_PTR super_array = NULL;
	ARRAY_DESCRIPTOR_PTR ingroupmap = NULL;
	ARRAY_DESCRIPTOR_PTR outgroupmap = NULL;


	assert(amap && source && dest);
	assert((amap->super_array->type == NDARRT_BROKEN) || (amap->super_array->type == NDARRT_CONTIGUOUS));
	assert((amap->sub_array->type == NDARRT_BROKEN) || (amap->sub_array->type == NDARRT_CONTIGUOUS));
	
	sub_array = amap->sub_array;
	super_array = amap->super_array;

	if(array_complete)
		*(array_complete) = 0;
	
	paddingch = (char)(destid & NDARRS_PADDING);

	/*********
	**********
	********** Check on the source, making sure everything is in order...
	**********
	*********/

	if(sourceid & NDARRS_FILE){ /* We are reading in from a file */
		infilename = (char *)source;
		if(super_array->type == NDARRT_BROKEN){ /* Broken across multiple files */
			/* See if the GROUPMAP_FILE array has been set up. */
			if(super_array->extra_info == NULL){
				/* Set up the GROUPMAP_FILE */
				if(ndarr_create_brkn_desc(super_array, NDARR_MAP_IN_FILE, (void *)infilename)){
					/* Error occured */
					err_push(ERR_NDARRAY, "Error attempting to create groupmap array");
					return(-1);
				}
			}
			else{
				ingroupmap = (ARRAY_DESCRIPTOR_PTR)super_array->extra_info;
				if(ingroupmap->type != NDARRT_GROUPMAP_FILE){
					err_push(ERR_NDARRAY, "Unknown external descriptive array");
					return(-1);
				}
			}
			infilename = NULL;
		}
		else{ /* The input array is contiguous */
#ifdef ND_FP 
			if (super_array->fp)
				infile = super_array->fp;
			else
#endif
			{
				if(!(infile = fopen(infilename, "rb")))
				{
					err_push(ERR_NDARRAY, "Unable to open input file");
					return(-1);
				}
			}
		}
	}
	else{
		if(sourceid & NDARRS_BUFFER){
			if(super_array->type == NDARRT_BROKEN){ /* Broken across multiple buffers */
				/* See if the GROUPMAP_BUFF array has been set up. */
				if(super_array->extra_info == NULL){
					if(ndarr_create_brkn_desc(sub_array, NDARR_BUFFER_GROUPING, (void *)dest)){
						err_push(ERR_NDARRAY, "Unable to create buffer groupmap");
						return(-1);
					}
				}
				else{
					ingroupmap = (ARRAY_DESCRIPTOR_PTR)super_array->extra_info;
					if(ingroupmap->type != NDARRT_GROUPMAP_BUFF){
						err_push(ERR_NDARRAY, "Unknown external descriptive array");
						return(-1);
					}
				}
			}
		}
		else{ /* Not a buffer or a file- where do we get it? */
			err_push(ERR_NDARRAY, "Unknown source type");
			return(-1);
		}
	}
	
	/*********
	**********
	********** Check on the destination, making sure everything is in order...
	**********
	*********/

	if(destid & NDARRS_FILE){ /* We are writing out to a file */
		/* Determine how we should open the file */
    	if(destid & NDARRS_APPEND){
    		fmode = "ab";
			needtopad = 1;
    	}
    	if(destid & NDARRS_UPDATE){
    		if(fmode){
    			err_push(ERR_NDARRAY, "Multiple file open modes defined");
				return(-1);
			}
    		fmode = "r+b";
    	}
    	if(destid & NDARRS_CREATE){
			if(fmode){
    			err_push(ERR_NDARRAY, "Multiple file open modes defined");
    			return(-1);
    		}
			if(amap->fcreated)
				fmode = "ab";
			else
				fmode = "wb";
    		needtopad = 1;
		}

		/* Determine filename and grouping */
		outfilename = (char *)dest;
		if(sub_array->type == NDARRT_BROKEN){ /* Broken across multiple files */
			/* See if the GROUPMAP_FILE array has been set up. */
			if(sub_array->extra_info == NULL){
				/* Set up the GROUPMAP_FILE */
				if(ndarr_create_brkn_desc(sub_array, NDARR_MAP_IN_FILE, (void *)outfilename)){
					/* Error occured */
					err_push(ERR_NDARRAY, "Error attempting to create groupmap array");
					return(-1);
				}
			}
			else{
				outgroupmap = (ARRAY_DESCRIPTOR_PTR)sub_array->extra_info;
				if(outgroupmap->type != NDARRT_GROUPMAP_FILE){
					err_push(ERR_NDARRAY, "Unknown external descriptive array");
					return(-1);
				}
			}
			outfilename = NULL;
		}
		else{ /* The output file is contiguous */
#ifdef ND_FP 
			if (sub_array->fp)
				outfile = sub_array->fp;
			else
#endif
			{
    			if(!(outfile = fopen(outfilename, fmode)))
				{
					err_push(ERR_NDARRAY, "Unable to open output file");
    				return(-1);
				}
			}
    	}

    	if((!amap->fcreated) && (destid & NDARRS_CREATE)){
    		/* Need to create the output file(s) */
    		amap->fcreated = 1;

    		if(outfilename){ /* The array output is contiguous */
    			/* Put out header if needed */
				if(dest_size)
					for(i = 0; i < dest_size; i++)
						fputc(paddingch, outfile);
    		}
    		else{ /* Output array is broken */
    			outfilename = (char *)ndarr_get_next_group(sub_array, NDARR_GINITIAL);
    			do{
					if(!(outfile = fopen(outfilename, fmode))){
						err_push(ERR_NDARRAY, "Unable to open output file");
						return(-1);
					}
					/* Put out header if needed */
					if(dest_size)
						for(i = 0; i < dest_size; i++)
							fputc(paddingch, outfile);
					fclose(outfile);
				} while((outfilename = (char *)ndarr_get_next_group(sub_array, NDARR_GNEXT)));
    			outfile = NULL;
    		}
    		
    		/* Reset file mode to append */
			fmode = "ab";
    	}
	}
	else{
		if(destid & NDARRS_BUFFER){
			if(sub_array->type == NDARRT_BROKEN){ /* Broken across multiple buffers */
				/* See if the GROUPMAP_BUFF array has been set up. */
				if(sub_array->extra_info == NULL){
					if(ndarr_create_brkn_desc(sub_array, NDARR_BUFFER_GROUPING, (void *)dest)){
						err_push(ERR_NDARRAY, "Unable to create buffer groupmap");
						return(-1);
					}
				}
				else{
					outgroupmap = (ARRAY_DESCRIPTOR_PTR)sub_array->extra_info;
					if(outgroupmap->type != NDARRT_GROUPMAP_BUFF){
						err_push(ERR_NDARRAY, "Unknown external descriptive array");
						return(-1);
					}
				}
			}
		}
		else{ /* Not a buffer or a file- where do we put it? */
			err_push(ERR_NDARRAY, "Unknown destination type");
			return(-1);
		}
	}
    
	/*********
	********** Source and destination seem to check out properly.
	********** Go ahead and try to reorient...
	**********
	*********/

	if((sourceid & NDARRS_FILE) && (destid & NDARRS_FILE)){
		/*********************************************
		**********************************************
		*******         FILE TO FILE           *******
		**********************************************
		*********************************************/

		if(!(buffer = (char *)memMalloc((size_t)(amap->increment_block + 50), "buffer"))){
			err_push(ERR_NDARRAY, "Out of memory");

			if(infile 
#ifdef ND_FP 
			   && !super_array->fp
#endif
			  )
				fclose(infile);

			if(outfile
#ifdef ND_FP 
			   && !sub_array->fp
#endif
			  )
				fclose(outfile);

			return(-1);
		}
	
		if((destid & NDARRS_UPDATE) && (sub_array->type != NDARRT_BROKEN)){
			/* Get around or put out a header in output */
			fseek(outfile, dest_size, SEEK_SET);
		}

		/* Create the sub-array */
		do{
			offset = ndarr_get_mapped_offset(amap) + source_size;

			if(super_array->type == NDARRT_BROKEN){
				newinfn = ndarr_get_group(amap->aindex);
				if(newinfn != infilename){
					if(infile)
						fclose(infile);
					infilename = newinfn;

					if(!(infile = fopen(infilename, "rb"))){
						err_push(ERR_NDARRAY, "Unable to open input file");
						err_push(ERR_NDARRAY, infilename);
						memFree(buffer, "buffer");
						if(outfile)
							fclose(outfile);
						return(-1);
					}
				}
			}

			if(fseek(infile, (long)offset, SEEK_SET)){
				err_push(ERR_NDARRAY, "Unable to seek in input file (file too small?)");
				err_push(ERR_NDARRAY, infilename);

				if(infile
#ifdef ND_FP 
				   && !super_array->fp
#endif
				  )
					fclose(infile);

				if(outfile
#ifdef ND_FP 
				   && !sub_array->fp
#endif
				  )
					fclose(outfile);

				memFree(buffer, "buffer");
				return(-1);
			}
			if(!(fread((void *)buffer, (size_t)amap->increment_block, (size_t)1, infile)))
			{
				if(outfile
#ifdef ND_FP 
				   && !sub_array->fp
#endif
				  )
					fclose(outfile);

				memFree(buffer, "buffer");

				if (feof(infile))
				{
					if(infile
#ifdef ND_FP 
					   && !super_array->fp
#endif
					  )
						fclose(infile);

					return(0);
				}

				if(infile
#ifdef ND_FP 
				   && !super_array->fp
#endif
				  )
					fclose(infile);

				err_push(ERR_NDARRAY, "Unable to read from input file (file too small?)");
				err_push(ERR_NDARRAY, infilename);

				return(-1);
			}

			if(sub_array->type == NDARRT_BROKEN){
				newoutfn = ndarr_get_group(amap->subaindex);
				if(newoutfn != outfilename){
					if(outfile)
						fclose(outfile);

					outfilename = newoutfn;
					if(!(outfile = fopen(outfilename, fmode))){
						err_push(ERR_NDARRAY, "Unable to open output file");
						err_push(ERR_NDARRAY, outfilename);
						fclose(infile);
						memFree(buffer, "buffer");
						return(-1);
					}

					if(destid & NDARRS_UPDATE){
						/* Get around or put out a header in output */
						outoffset = ndarr_get_offset(amap->subaindex) + dest_size;
						if(fseek(outfile, outoffset, SEEK_SET)){
							err_push(ERR_NDARRAY, "Unable to seek past separation in output file");
							err_push(ERR_NDARRAY, outfilename);
							fclose(infile);
							fclose(outfile);
							memFree(buffer, "buffer");
							return(-1);							
						}
					}
				}
			}
			
			if(!(fwrite(buffer, (size_t)amap->increment_block, 1, outfile))){
				err_push(ERR_NDARRAY, "Unable to write to output file");
				err_push(ERR_NDARRAY, outfilename);

#ifdef ND_FP 
				if (!super_array->fp)
#endif
					fclose(infile);

#ifdef ND_FP 
				if (!sub_array->fp)
#endif
					fclose(outfile);

				memFree(buffer, "buffer");
				return(-1);
			}
				
			/* See if we need to put out padding (or skip) for separation */
			if(amap->subsep)
			{
				NDARR_GET_MAP_SEPARATION(amap, outoffset);
				/* Need to pad a little for output */
				if(needtopad)
				{
					for(i = outoffset; i > 0; i--)
						fputc(paddingch, outfile);
				}
				else
				{
					if(fseek(outfile, outoffset, SEEK_CUR))
					{
						err_push(ERR_NDARRAY, "Unable to seek past separation in output file");
						err_push(ERR_NDARRAY, outfilename);

#ifdef ND_FP 
						if (!super_array->fp)
#endif
							fclose(infile);

#ifdef ND_FP 
						if (!sub_array->fp)
#endif
							fclose(outfile);
						
						memFree(buffer, "buffer");
						return(-1);
					}
				}
			}
		} while(ndarr_increment_mapping(amap));

		memFree(buffer, "buffer");

#ifdef ND_FP 
		if (!super_array->fp)
#endif
			fclose(infile);

#ifdef ND_FP 
		if (!sub_array->fp)
#endif
			fclose(outfile);

		if(array_complete)
			*(array_complete) = 1;

		return(sub_array->total_size);
	}

	if((sourceid & NDARRS_FILE) && (destid & NDARRS_BUFFER)){
		/*********************************************
		**********************************************
    	*******         FILE TO BUFF           *******
    	**********************************************
    	*********************************************/
    	
    	bytesread = 0;
    	
    	min_buff_size = amap->increment_block + amap->subsep;
    	
    	/* Make sure that all the restrictions for this case have been met */
    	if(dest_size < min_buff_size){
    		/* We can still process the array- we just need to modify the
    		 * amap->increment_block until it will fit */
    		i = (long)amap->dimincrement;
    		for(; amap->dimincrement < sub_array->num_dim; amap->dimincrement++){ 
				if(amap->dimincrement < 0){
					amap->dimincrement = -1;
					amap->increment_block = sub_array->total_size;
				}
				else{
					amap->increment_block = sub_array->coeffecient[amap->dimincrement] - sub_array->separation[amap->dimincrement];
					if(amap->dimincrement == sub_array->num_dim - 1)
						amap->increment_block = sub_array->element_size; 
				}
	    		min_buff_size = amap->increment_block + amap->subsep;
	    		if(dest_size >= min_buff_size){
	    			break;
	    		}
	    	}
	    	if(amap->dimincrement == sub_array->num_dim){ /* Just plain too small */
				if(i < 0){
					amap->dimincrement = -1;
					amap->increment_block = sub_array->total_size;
				}
				else{
					amap->dimincrement = (int)i;
					amap->increment_block = sub_array->coeffecient[i] - sub_array->separation[i];
					if(i == sub_array->num_dim - 1)
						amap->increment_block = sub_array->element_size; 
				}
				err_push(ERR_NDARRAY, "Buffer too small");

    			if(infile
#ifdef ND_FP 
				   && !super_array->fp
#endif
				  )
    				fclose(infile);

    			return(-1);
	    	}
		}
    	
    	if(sub_array->type == NDARRT_BROKEN){
    		if(sub_array->group_size > dest_size){
    			err_push(ERR_NDARRAY, "Broken buffers too small for groups");
				if(infile)
    				fclose(infile);
    			return(-1);
    		}
    	}
    	else{
    		buffer = (char *)dest;
    	}

		/* Create the sub-array */
		do{
			offset = ndarr_get_mapped_offset(amap) + source_size;
			
			if(super_array->type == NDARRT_BROKEN){
				newinfn = ndarr_get_group(amap->aindex);
				if(newinfn != infilename){
					if(infile)
						fclose(infile);
					infilename = newinfn;

					if(!(infile = fopen(infilename, "rb"))){
						err_push(ERR_NDARRAY, "Unable to open input file");
						err_push(ERR_NDARRAY, infilename);
						return(-1);
					}
				}
			}

			if(fseek(infile, (long)offset, SEEK_SET)){
				err_push(ERR_NDARRAY, "Unable to seek in input file (file too small?)");
				err_push(ERR_NDARRAY, infilename);

#ifdef ND_FP 
				if (!super_array->fp)
#endif
					fclose(infile);

				return(-1);
			}
			
			if(sub_array->type == NDARRT_BROKEN){
				buffer = (char *)ndarr_get_group(amap->subaindex);
				outoffset = ndarr_get_offset(amap->subaindex);
				buffer += outoffset;
			}
			
			if(!(fread((void *)buffer, (size_t)amap->increment_block, (size_t)1, infile)))
			{
				if(outfile
#ifdef ND_FP 
				   && !sub_array->fp
#endif
				  )
					fclose(outfile);

				if (feof(infile))
				{
					if (infile
#ifdef ND_FP 
					    && !super_array->fp
#endif
					   )
						fclose(infile);

					return(0);
				}

				if(infile
#ifdef ND_FP 
				   && !super_array->fp
#endif
				  )
					fclose(infile);

				err_push(ERR_NDARRAY, "Unable to read from input file (file too small?)");
				err_push(ERR_NDARRAY, infilename);

				return(-1);
			}
			
			buffer += amap->increment_block;
			bytesread += amap->increment_block;

			/* See if we need to put out padding (or skip) for separation */
			if(amap->subsep){
				NDARR_GET_MAP_SEPARATION(amap, outoffset);
				/* Need to pad a little for output */
				buffer += outoffset;
				bytesread += outoffset;
			}
		} while((ndarr_increment_mapping(amap)) && ((dest_size - bytesread) >= min_buff_size));
		
		if(array_complete){
			*(array_complete) = 1;
			for(i = 0; i < sub_array->num_dim; i++)
				if(amap->subaindex->index[i])
					*(array_complete) = 0;
		}

#ifdef ND_FP 
		if (!super_array->fp)
#endif
			fclose(infile);

		return(bytesread);
	}

	if((sourceid & NDARRS_BUFFER) && (destid & NDARRS_FILE)){
    	/*********************************************
    	**********************************************
    	*******         BUFF TO FILE           *******
    	**********************************************
		*********************************************/
		
		bytesread = 0;
		
		/* Make sure that all the restrictions for this case have been met */
    	if(super_array->type == NDARRT_BROKEN){
    		if(super_array->group_size > (long)source_size){
    			err_push(ERR_NDARRAY, "Broken buffers too small for groups");
				if(outfile)
    				fclose(outfile);
    			return(-1);
    		}
    	}
    	else{
    		if((amap->necessary) && ((long)source_size < super_array->total_size)){
    			err_push(ERR_API, "Input buffer (%lu bytes) must contain the entire array (%lu bytes)", (unsigned long)source_size, (unsigned long)super_array->total_size);

				if(outfile
#ifdef ND_FP 
				   && !sub_array->fp
#endif
				  )
    				fclose(outfile);

    			return(-1);
    		}
    		buffer = (char *)source;
    	}

		if((destid & NDARRS_UPDATE) && (sub_array->type != NDARRT_BROKEN)){
			/* Get around or put out a header in output */
			fseek(outfile, dest_size, SEEK_SET);
		}
		
		if(source_size < (long)super_array->group_size){
			/************************************************
			*****  Special case: input buffer does NOT  *****
			*****       contain the entire array        *****
			************************************************/
			
			/* If we are resuming writing to a file opened for update, we need
			 * to make sure to start off at the correct position */
			if((destid & NDARRS_UPDATE) && (sub_array->type != NDARRT_BROKEN)){
				/* Get around or put out a header in output */
				outoffset = ndarr_get_offset(amap->subaindex) + dest_size;
				if(fseek(outfile, outoffset, SEEK_SET)){
					err_push(ERR_NDARRAY, "Unable to seek past separation in output file");
					err_push(ERR_NDARRAY, outfilename);

#ifdef ND_FP 
					if (!sub_array->fp)
#endif
						fclose(outfile);

					memFree(buffer, "buffer");
					return(-1);							
				}
			}
            
            /* Subtract this starting offset from the offset into the buffer */
			startoffset = ndarr_get_mapped_offset(amap);
			
			/* Create the sub-array */
			do{
				offset = ndarr_get_mapped_offset(amap) - startoffset;
				
				/* Check to make sure we aren't going to run off the end of our
				 * input buffer */
				if((long)(offset + sub_array->element_size) > source_size)
					break;
				
				/* Don't have to worry about the input buffer being broken here;
				 * if the input buffer was broken, it must contain the entire
				 * array, and therefore we would not be in this special case */
				 
				bufferpos = buffer + offset;
	
				if(sub_array->type == NDARRT_BROKEN){
					newoutfn = ndarr_get_group(amap->subaindex);
					if(newoutfn != outfilename){
						if(outfile)
							fclose(outfile);
	
						outfilename = newoutfn;
						if(!(outfile = fopen(outfilename, fmode))){
							err_push(ERR_NDARRAY, "Unable to open output file");
							err_push(ERR_NDARRAY, outfilename);
							memFree(buffer, "buffer");
							return(-1);
						}
	
						if(destid & NDARRS_UPDATE){
							/* Get around or put out a header in output */
							outoffset = ndarr_get_offset(amap->subaindex) + dest_size;
							if(fseek(outfile, outoffset, SEEK_SET)){
								err_push(ERR_NDARRAY, "Unable to seek past separation in output file");
								err_push(ERR_NDARRAY, outfilename);
								fclose(outfile);
								memFree(buffer, "buffer");
								return(-1);							
							}
						}
					}
				}
				
				if(!(fwrite(bufferpos, (size_t)sub_array->element_size, 1, outfile))){
					err_push(ERR_NDARRAY, "Unable to write to output file");
					err_push(ERR_NDARRAY, outfilename);

#ifdef ND_FP 
					if (!sub_array->fp)
#endif
						fclose(outfile);

					memFree(buffer, "buffer");
					return(-1);
				}

				bytesread += sub_array->element_size;
									
				/* See if we need to put out padding (or skip) for separation */
				if(amap->subsep){
					NDARR_GET_SEPARATION(amap->subaindex, outoffset);
					bytesread += outoffset;
					/* Need to pad a little for output */
					if(needtopad)
						for(i = outoffset; i > 0; i--)
							fputc(paddingch, outfile);
					else
						if(fseek(outfile, outoffset, SEEK_CUR)){
							err_push(ERR_NDARRAY, "Unable to seek past separation in output file");
							err_push(ERR_NDARRAY, outfilename);

#ifdef ND_FP 
							if (!sub_array->fp)
#endif
								fclose(outfile);

							memFree(buffer, "buffer");
							return(-1);
						}
				}
			} while(ndarr_increment_indices(amap->subaindex));
	
#ifdef ND_FP 
			if (!sub_array->fp)
#endif
				fclose(outfile);
	
			if(array_complete){
				*(array_complete) = 1;
				for(i = 0; i < sub_array->num_dim; i++)
					if(amap->subaindex->index[i])
						*(array_complete) = 0;
			}
	
			return(bytesread);
		} /* End special case: input buffer does not contain the entire array */


		/********************************************
		*****  Special case: input buffer DOES  *****
		*****     contain the entire array      *****
		********************************************/

		/* Create the sub-array */
		do{
			offset = ndarr_get_mapped_offset(amap);

			if(super_array->type == NDARRT_BROKEN){
				buffer = ndarr_get_group(amap->aindex);
			}
			
			bufferpos = buffer + offset;

			if(sub_array->type == NDARRT_BROKEN){
				newoutfn = ndarr_get_group(amap->subaindex);
				if(newoutfn != outfilename){
					if(outfile)
						fclose(outfile);

					outfilename = newoutfn;
					if(!(outfile = fopen(outfilename, fmode))){
						err_push(ERR_NDARRAY, "Unable to open output file");
						err_push(ERR_NDARRAY, outfilename);
						memFree(buffer, "buffer");
						return(-1);
					}

					if(destid & NDARRS_UPDATE){
						/* Get around or put out a header in output */
						outoffset = ndarr_get_offset(amap->subaindex) + dest_size;
						if(fseek(outfile, outoffset, SEEK_SET)){
							err_push(ERR_NDARRAY, "Unable to seek past separation in output file");
							err_push(ERR_NDARRAY, outfilename);
							fclose(outfile);
							memFree(buffer, "buffer");
							return(-1);							
						}
					}
				}
			}
			
			if(!(fwrite(bufferpos, (size_t)amap->increment_block, 1, outfile))){
				err_push(ERR_NDARRAY, "Unable to write to output file");
				err_push(ERR_NDARRAY, outfilename);

#ifdef ND_FP 
				if (!sub_array->fp)
#endif
					fclose(outfile);

				memFree(buffer, "buffer");
				return(-1);
			}
			
			bytesread += amap->increment_block;
				
			/* See if we need to put out padding (or skip) for separation */
			if(amap->subsep){
				NDARR_GET_MAP_SEPARATION(amap, outoffset);
				bytesread += outoffset;
				/* Need to pad a little for output */
				if(needtopad)
					for(i = outoffset; i > 0; i--)
						fputc(paddingch, outfile);
				else
					if(fseek(outfile, outoffset, SEEK_CUR)){
						err_push(ERR_NDARRAY, "Unable to seek past separation in output file");
						err_push(ERR_NDARRAY, outfilename);

#ifdef ND_FP 
						if (!sub_array->fp)
#endif
							fclose(outfile);

						memFree(buffer, "buffer");
						return(-1);
					}
			}
		} while(ndarr_increment_mapping(amap));

#ifdef ND_FP 
		if (!sub_array->fp)
#endif
			fclose(outfile);

		if(array_complete)
			*(array_complete) = 1;

		return(sub_array->total_size);
	}
    
	if((sourceid & NDARRS_BUFFER) && (destid & NDARRS_BUFFER)){
    	/*********************************************
    	**********************************************
    	*******         BUFF TO BUFF           *******
    	**********************************************
    	*********************************************/

		/* Make sure that all the restrictions for this case have been met */
    	if(super_array->type == NDARRT_BROKEN){
    		if(super_array->group_size > (long)source_size){
    			err_push(ERR_NDARRAY, "Broken buffers too small for groups");
    			return(-1);
    		}
    	}
    	else{
    		if((long)source_size < super_array->total_size){
    			err_push(ERR_API, "Input buffer (%lu bytes) must contain the entire array (%lu bytes)", (unsigned long)source_size, (unsigned long)super_array->total_size);
    			return(-1);
    		}
    		inbuffer = (char *)source;
    	}

    	if(sub_array->type == NDARRT_BROKEN){
    		if(sub_array->group_size > (long)dest_size){
    			err_push(ERR_API, "Broken buffers too small for groups");
    			return(-1);
    		}
    	}
    	else{
    		if((long)dest_size < sub_array->total_size){
    			err_push(ERR_API, "Output buffer (%lu bytes) must contain the entire array (%lu bytes)", (unsigned long)dest_size, (unsigned long)sub_array->total_size);
    			return(-1);
    		}
    		buffer = (char *)dest;
    	}

		/* Create the sub-array */
		do{
			offset = ndarr_get_mapped_offset(amap);
			if(super_array->type == NDARRT_BROKEN)
				inbuffer = ndarr_get_group(amap->aindex);
			inbufferpos = inbuffer + offset;
			if(sub_array->type == NDARRT_BROKEN)
				buffer = (char *)ndarr_get_group(amap->subaindex);
				buffer += ndarr_get_offset(amap->subaindex);

			memcpy(buffer, inbufferpos, (size_t)amap->increment_block);
			buffer += amap->increment_block;

			/* See if we need to put out padding (or skip) for separation */
			if(amap->subsep){
				NDARR_GET_MAP_SEPARATION(amap, outoffset);
				buffer += outoffset;
			}
		} while(ndarr_increment_mapping(amap));

		if(array_complete)
			*(array_complete) = 1;

		return(sub_array->total_size);
    }

    return(-1);
}


/*
 * NAME:	ndarr_create_brkn_desc
 *
 * PURPOSE:	To create an array descriptor describing how the passed array descriptor
 *			is split across files.
 *
 * USAGE:   int ndarr_create_brkn_desc(ARRAY_DESCRIPTOR_PTR adesc, int map_type,
 *				 void *mapping)
 *
 *			where map_type is either
 *			NDARR_BUFFER_GROUPING (if the array is split across buffers),
 *			NDARR_MAP_IN_BUFFER   (if the array is split across files, and the
 *									filenames are stored in a buffer),
 *			NDARR_MAP_IN_FILE     (if the array is split across files, and the
 *									filenames are stored in another file).
 *
 * RETURNS: 0 if all is OK, >< 0 on error.
 *
 * DESCRIPTION: 
 *			A pointer to a filled, one-dimensional array of char * to file
 *			names or void * to buffers is stored in the extra_info pointer of
 *			the NDARRT_GROUPMAP_FILE or NDARRT_GROUPMAP_BUFF descriptor 
 *			(which is created by this function; the descriptor is stored
 *			in the extra_info pointer of the NDARRT_BROKEN ARRAY_DESCRIPTOR struct.)
 *
 *			If map_type == NDARR_BUFFER_GROUPING the void *mapping variable is
 *			    interpreted as a void ** (an array of pointers to buffers).
 *				Each of the buffers in the array must be allocated; a copy of the
 *				pointers will be stored. 
 *			If map_type == NDARR_MAP_IN_BUFFER the void *mapping variable is
 *				interpreted as a char ** (an array of filenames).
 *			If map_type == NDARR_MAP_IN_FILE the void *mapping variable is
 *				interpreted as a char * (a filename), where the file
 *				describes how the broken array is split across
 *				files.  This file has one filename per line.
 *				Filenames may have leading and trailing whitespace.
 *				The filenames must be less than 300 bytes in length.
 *
 *          The descriptor is generated from the array which is broken.
 *          All filenames and buffer pointers are assumed in 
 *			dimension[0] major order (in 2D, Row Major order, in 3d, Plane Major).
 *
 * SYSTEM DEPENDENT FUNCTIONS: Currently, paths are assumed native.
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_create_brkn_desc"
int ndarr_create_brkn_desc(ARRAY_DESCRIPTOR_PTR adesc, int map_type, void *mapping)
{
	FILE *infile;
	char *filename;
    char scratch[305];
	 char path[MAX_PATH];
    char **ptrarray;
    char **filearray;
    char *position;
    int i;
	int gdim;
    char *descstr;
	ARRAY_DESCRIPTOR_PTR groupmap;
    
    assert(adesc && mapping);
    
    if(adesc->type != NDARRT_BROKEN){
		return(0); /* It wasn't a broken array; leave it alone */
    }

	/* Determine how many dimensions we are dealing with */    
	for(gdim = 0; gdim < adesc->num_dim; gdim++)
    	if(!adesc->grouping[gdim])
    		break;

	if(!(descstr = (char *)memMalloc((size_t)(gdim * 30 + 5), "descstr"))){
		err_push(ERR_NDARRAY, "Out of memory");
		return(1);
	}

	/* Create the array descriptor string */
	position = descstr;
	for(i = 0; i < gdim; i++){
		sprintf(position, "[\"%d\" 0 to %d]", i, (int)(adesc->dim_size[i] / adesc->grouping[i]) - 1);
		position = position + strlen(position);
	}
	sprintf(position, " %d", (int)sizeof(char *));
	
	/* Create the array descriptor structure */
	groupmap = ndarr_create_from_str(NULL, descstr);
	memFree(descstr, "descstr");
	if(!groupmap){
		err_push(ERR_NDARRAY, "Creating grouping map");
		return(1);
	}

	/* Allocate the array */
	if(!(filearray = (char **)memMalloc((size_t)groupmap->contig_size, "filearray"))){
		err_push(ERR_NDARRAY, "Out of memory");
		ndarr_free_descriptor(groupmap);
		return(1);
	}
	
	switch(map_type){
		case NDARR_BUFFER_GROUPING:
			/*******************
			 * List of buffers *
			 *******************/
			groupmap->type = NDARRT_GROUPMAP_BUFF;

			ptrarray = (char **)mapping;
			
			/* Read in the buffers from the list */
			for(i = 0; i < groupmap->total_elements; i++){
				filearray[i] = ptrarray[i];
			}
			break;
			
		case NDARR_MAP_IN_BUFFER:
			/********************************************
			 * List of filenames are stored in a buffer *
			 ********************************************/
			groupmap->type = NDARRT_GROUPMAP_FILE;
			
			ptrarray = (char **)mapping;
			
			/* Read in the filenames from the list */
			for(i = 0; i < groupmap->total_elements; i++){
				position = ptrarray[i];
		
				/* Store the filename away in the array */
				if(!(filearray[i] = (char *)memMalloc((size_t)(strlen(position) + 3), "filearray[i]"))){
					err_push(ERR_NDARRAY, "Out of memory");
					ndarr_free_descriptor(groupmap);
					for(i--; i >=0; i--)
						memFree(filearray[i], "filearray[i]");
					memFree(filearray, "filearray");
					return(1);
				}
				strcpy(filearray[i], position);
			}
			break;
			
		case NDARR_MAP_IN_FILE:
			/******************************************
			 * List of filenames are stored in a file *
			 ******************************************/
			groupmap->type = NDARRT_GROUPMAP_FILE;
			
			filename = (char *)mapping;
			
			/* Open the file */
			if(!(infile = fopen(filename, "r"))){
				err_push(ERR_NDARRAY, "Out of memory");
				ndarr_free_descriptor(groupmap);
				memFree(filearray, "filearray");
				return(1);
			}
		
			/* Read in the filenames from the file */
			for(i = 0; i < groupmap->total_elements; i++){
				if(!(fgets(scratch, 300, infile))){
					err_push(ERR_NDARRAY, "Unexpected End Of File- Groupmap file");
					ndarr_free_descriptor(groupmap);
					for(i--; i >=0; i--)
						memFree(filearray[i], "filearray[i]");
					memFree(filearray, "filearray");

					fclose(infile);

					return(1);
				}
				
				/* Trim off trailing EOL, spaces... */
				position = scratch + strlen(scratch);
				while(position[0] < '!')
					position--;
				position[1] = '\0';
				
				/* Trim leading whitespace */
				position = scratch;
				while(position[0] < '!')
					position++;

				os_path_make_native(position, position);
				os_path_get_parts(position, path, NULL, NULL);
				if (!strlen(path))
				{
					os_path_get_parts(filename, path, NULL, NULL);
					os_path_put_parts(position, path, position, NULL);
				}
		
				/* Store the filename away in the array */
				if(!(filearray[i] = (char *)memMalloc((size_t)(strlen(position) + 3), "filearray[i]"))){
					err_push(ERR_NDARRAY, "Out of memory");
					ndarr_free_descriptor(groupmap);
					for(i--; i >=0; i--)
						memFree(filearray[i], "filearray[i]");
					memFree(filearray, "filearray");

					fclose(infile);

					return(1);
				}
				strcpy(filearray[i], position);
			}

			fclose(infile);

			break;
		default:
			err_push(ERR_NDARRAY, "Unknown mapping type");
			ndarr_free_descriptor(groupmap);
			return(1);
	}

	groupmap->extra_info = (void *)filearray;

	groupmap->extra_index = (void *)ndarr_create_indices(groupmap);
	if(!groupmap->extra_index){
		err_push(ERR_NDARRAY, "Error creating indices");
		ndarr_free_descriptor(groupmap);
		return(1);
	}

	/* Store away the correct pointers... */
	adesc->extra_info = (void *)groupmap;
	return(0); /* All done! */
}


/*
 * NAME:	ndarr_get_group
 *
 * PURPOSE:	To determine what group a given array element belongs to.
 *
 * USAGE:   void *ndarr_get_group(ARRAY_INDEX_PTR aindex)
 *
 * RETURNS: NULL on error, otherwise the pointer in the group mapping array to the
 *			appropriate buffer or filename.
 *
 * DESCRIPTION: Determines which group the element stored in the ARRAY_INDEX_PTR
 *				belongs to.  If the array is not broken, returns an error.
 *
 * SYSTEM DEPENDENT FUNCTIONS: none
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:
 *
 * KEYWORDS: array
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ndarr_get_group"
void *ndarr_get_group(ARRAY_INDEX_PTR aindex)
{
	ARRAY_DESCRIPTOR_PTR groupmap;
	ARRAY_INDEX_PTR gindex;
	char **extinf;
	int i;

	assert((aindex) && (aindex->descriptor->type == NDARRT_BROKEN) && (aindex->descriptor->extra_info));

	groupmap = (ARRAY_DESCRIPTOR_PTR)aindex->descriptor->extra_info;

	assert(groupmap->extra_info && groupmap->extra_index);

	gindex = (ARRAY_INDEX_PTR)groupmap->extra_index;
	extinf = (char **)groupmap->extra_info;

	for(i = 0; i < groupmap->num_dim; i++)
		gindex->index[i] = aindex->index[i] / aindex->descriptor->grouping[i];

	return((void *)extinf[(int)(ndarr_get_offset(gindex) / sizeof(char *))]);
}


/*
 * NAME:	ndarr_get_next_group
 *
 * PURPOSE:	To return the next group associated with the array.
 *
 * USAGE:   void *ndarr_get_next_group(ARRAY_DESCRIPTOR_PTR arrdesc, char mode)
 *				where mode is either:
 *					NDARR_GINITIAL (to reset and get the first group)
 *					NDARR_GNEXT (to get the next group)
 *
 * RETURNS: NULL if no more groups exist, otherwise the pointer in the group
 *			mapping array to the appropriate buffer or filename.
 *
 * DESCRIPTION: Call with NDARR_GINITIAL to start, then NDARR_GNEXT to continue.
 *			Calls to ndarr_get_group in between calls to ndarr_get_next_group will
 *			result in a nasty little bug (both functions use the groupmap's 
 *			"extra_index", ndarr_get_next_group uses it to store what group it is
 *			on currently, and ndarr_get_group resets it every time).
 *
 * SYSTEM DEPENDENT FUNCTIONS: none
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS:
 *
 * KEYWORDS: array
 *
 */
void *ndarr_get_next_group(ARRAY_DESCRIPTOR_PTR arrdesc, char mode)
{
	ARRAY_DESCRIPTOR_PTR groupmap;
	ARRAY_INDEX_PTR gindex;
	char **extinf;

	assert((arrdesc) && (arrdesc->type == NDARRT_BROKEN) && (arrdesc->extra_info));

	groupmap = (ARRAY_DESCRIPTOR_PTR)arrdesc->extra_info;

	assert(groupmap->extra_info && groupmap->extra_index);

	gindex = (ARRAY_INDEX_PTR)groupmap->extra_index;
	extinf = (char **)groupmap->extra_info;

	if(mode == NDARR_GINITIAL){
		NDARR_RESET_INDICES(gindex);
		return((void *)extinf[(int)(ndarr_get_offset(gindex) / sizeof(char *))]);
	}

	if(!(ndarr_increment_indices(gindex)))
		return(NULL); /* Done with our last group */

	return((void *)extinf[(int)(ndarr_get_offset(gindex) / sizeof(char *))]);
}
