/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2010-2013 The HDF Group                                     *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF.  The full HDF copyright notice, including       *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file and in the print documentation copyright notice.         *
 * COPYING can be found at the root of the source code distribution tree;    *
 * the copyright notice in printed HDF documentation can be found on the     *
 * back of the title page.  If you do not have access to either version,     *
 * you may request a copy from help@hdfgroup.org.                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*!

  \file  xml_ris.c 
  \brief Write Raster Image information to XML file.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Oct 11, 2013
  \note improved error message format.

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 19, 2012
  \note updated palette handling.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date March 17, 2011
  \note added Raster attribute id variable(ID_RA).

  \date March 10, 2011
  \note removed NPOINTS_PERLINE 20.

  \author Ruth Aydt (aydt@hdfgroup.org)
  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date February 22, 2011
  \note updated for the new schema.

  \author Peter Cao (xcao@hdfgroup.org)
  \date    August 23, 2007

*/


#include "hmap.h"

extern unsigned int ID_FA;   
unsigned int ID_R = 0;          /*!< Unique Raster Image ID.  */

/*!
  \fn write_file_attrs_gr(FILE *ofptr, int32 gr_id, int indent)
  \brief Write Raster Image file attributes.
  
  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date February 7, 2012
  \note filtered reserved XML character	from the attribute name.

  \date June 13, 2011
  \note fixed dropping MSB in nt_type.

  \date March 10, 2011
  \note renamed as write_file_attrs_gr()
  
*/
intn
write_file_attrs_gr(FILE *ofptr, int32 gr_id, int indent)
{
    VOIDP attr_buf = NULL;

    char  attr_name[H4_MAX_NC_NAME];
    char  *name_xml = NULL;

    int32 attr_buf_size = 0;
    int32 attr_count = 0;
    int32 attr_nt = 0;
    int32 attr_nt_no_endian = 0;
    int32 i = 0;
    int32 n_images = 0;
    int32 n_file_attrs = 0;
    int32 offset = -1;
    int32 length = 0;

    intn stat = FAIL;


    stat = GRfileinfo(gr_id, &n_images, &n_file_attrs);
    if(stat == FAIL){
        FAIL_ERROR("GRfileinfo() failed.");
    }

    for(i =0; i < n_file_attrs; i++){
        /* Get the attribute's name, number type, and number of values. */
        stat = GRattrinfo(gr_id, i, attr_name, &attr_nt, 
                          &attr_count);
        if( stat == FAIL ){
            FAIL_ERROR("GRattrinfo() failed.");
        }
        attr_buf_size = DFKNTsize(attr_nt) * attr_count;
        attr_nt_no_endian = (attr_nt & 0xff);

        if(attr_buf_size <= 0){
            continue;
        }

        if (attr_nt_no_endian == DFNT_CHAR || 
            attr_nt_no_endian == DFNT_UCHAR)
            attr_buf = (char *) HDmalloc(attr_buf_size+1);
        else
            attr_buf = (VOIDP) HDmalloc(attr_buf_size);

        stat =  GRgetattr(gr_id, i, attr_buf);
        if(stat == FAIL ) {
            fprintf(flog, "GRgetattr() failed for %s.\n", attr_name);
            HDfree (attr_buf);
            return stat;
        }
        start_elm(ofptr, TAG_FATTR, indent);
        name_xml = get_string_xml(attr_name, strlen(attr_name));        
        if(name_xml != NULL){
            fprintf(ofptr, 
                    "name=\"%s\" origin=\"File Attribute: Raster Images\"", 
                    name_xml);
            HDfree(name_xml);
        }
        else{
            fprintf(flog, "get_string_xml() returned NULL.\n");
            HDfree (attr_buf);
            return FAIL;
        }        
        fprintf(ofptr, 
                " id=\"ID_FA%d\">", 
                ++ID_FA);            
        stat = GRgetattdatainfo(gr_id, i, &offset, &length);
        if(stat == FAIL){
            fprintf(flog, "GRgetattatatinfo() failed for %s.\n", attr_name);
            HDfree (attr_buf);
            return stat;            
        }
        write_array_attribute(ofptr, attr_nt, attr_count, attr_buf, 
                              attr_buf_size, offset, length, indent+1);

        end_elm(ofptr, TAG_FATTR, indent);
        if(attr_buf != NULL)
            HDfree(attr_buf);
    }


    return SUCCEED;
}


/*!
  
  \fn write_map_lone_ris(FILE *ofptr, int32 file_id, int32 gr_id, 
  ref_count_t *ris_visited)

  \brief Write Raster Image information to map file.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date Oct 11, 2013
  \note added tag/ref information for error message.

  \date May 3, 2011
  \note increased the indentation.

  \date April 25, 2011
  \note filtered rasters that are mapped under groups.

  \date March 14, 2011
  \note 
  changed return type.
  removed num_errs.


*/
intn
write_map_lone_ris(FILE *ofptr, int32 file_id, int32 gr_id, 
                   ref_count_t *ris_visited)
{


    RIS_info_t         ris_info;

    int                i = 0;
    intn status = SUCCEED;

    HDmemset(&ris_info, 0, sizeof(ris_info));

    if (RISget_refs(gr_id, &ris_info) == FAIL) {
        fprintf(flog, "RISget_refs() failed:gr_id=%ld.\n", 
                (long) gr_id);
        RISfree_ris_info(&ris_info);
        return FAIL;
    }

    /* Write mapping information for RIs. */
    for (i=0; i < ris_info.nimages; i++) {
        RIS_mapping_info_t map_info; 
        HDmemset(&map_info, 0, sizeof(map_info));
        /* Filter out rasters that are visited already. */
        if (ris_info.refs[i] != ref_count(ris_visited, ris_info.refs[i])) {
            status = write_map_ris(ofptr, file_id, gr_id, ris_info.tags[i], 
                                   ris_info.refs[i], "/", &map_info, 2, 
                                   ris_visited);

            if(status == FAIL){
                fprintf(flog, "write_map_ris() failed");
                fprintf(flog, ":gr_id=%ld, tag=%ld, ref=%ld.\n", 
                        (long)gr_id,
                        (long)ris_info.tags[i],
                        (long)ris_info.refs[i]);
                RISfree_mapping_info(&map_info);
                RISfree_ris_info(&ris_info);
                return FAIL;
            }
            RISfree_mapping_info(&map_info);
        }
    }
    RISfree_ris_info(&ris_info);
    return status;

}

/*! 
  
  \fn write_map_ris(FILE *ofptr, int32 file_id, int32 gr_id, uint16 tag, 
  uint16 ref, const char *path, RIS_mapping_info_t *map_info,
  int indent, ref_count_t *ris_visited) 

  \brief Write Raster Image information.

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Oct 11, 2013
  \note added tag/ref information for error message.

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 19, 2012
  \note
  replaced map_info->pal.ref with map_info->pal_ref.
  replaced call to get_pr2id_list() with call to palRef_to_palMapID().

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date May 9, 2011
  \note	added error handling for write_ris_byte_stream().

  \date April 25, 2011
  \note allowed a raster to appear twice under different groups.

  \date March 18, 2011
  \note suppressed names for RIS8 case.

  \date March 17, 2011
  \note 
  swapped width and height to match the pink lines in RasterCode.docx.
  added error handler for invalid palette XML ID.
  added error handler for write_map_datum().
  added image name for verification values.
  added image attribute handling. 
  cleaned up error messages for GRendaccess().
  changed dimensionStorageOrder to "row,column" according to new schema.
  replaced fixed data type with write_map_datum().

  \date March 14, 2011
  \note 
  added unmappable case handling.
  changed return type.
  removed local num_errs variable.


*/
intn 
write_map_ris(FILE *ofptr, int32 file_id, int32 gr_id, uint16 tag, 
              uint16 ref, const char *path, RIS_mapping_info_t *map_info,
              int indent, ref_count_t *ris_visited) 
{
    char strInterlace[20];
    char* name_xml = NULL;
    int i = 0;
    int j = 0;
    int idx = 0;


    int32 height = 0;
    int32 ri_id = FAIL;
    int32 start[2];             /* used in GRreadimage(). */
    int32 width = 0;

    intn status = 0; 

    uint palMapID;

    uint8* image_values = NULL;

    /* Save the reference to avoid generating the same raster element
       in write_map_lone_ris().  */
    ref_count(ris_visited, ref);

    idx = GRreftoindex(gr_id, ref);
    ri_id = GRselect(gr_id, idx);
    if(ri_id == FAIL){
        FAIL_ERROR("GRselect() failed.");
     }

    status = RISmapping(file_id, ri_id, map_info);
    if(status == FAIL){
        fprintf(flog, "RISmapping() failed:ri_id=%ld, tag=%ld, ref=%ld.\n", 
                (long)ri_id, (long)tag, (long)ref);
        return FAIL;
    }


    /* Filter out unsupported images and throw an error. */
    if(map_info->is_mappable == 0){
        ++num_errs;
        fprintf(ferr, "UNMAPPABLE:Not an 8-bit image (with RLE compression).");
        fprintf(ferr, ":path=%s, name=%s\n", 
                path, map_info->name);  
        if(GRendaccess(ri_id) == FAIL){
            FAIL_ERROR("GRendaccess() failed.");
        }
        return SUCCEED;
    }

    if(HDstrlen(map_info->comp_info) > 0 && 
       strncmp(map_info->comp_info, "UNMAPPABLE:", 11) == 0){
        ++num_errs;
        fprintf(ferr, "%s:path=", map_info->comp_info);
        fprintf(ferr, "%s, name=%s\n", path, map_info->name);
        if(GRendaccess(ri_id) == FAIL){
            FAIL_ERROR("GRendaccess() failed.");
        }
        return SUCCEED;
    }

    /* Process supported ones. */
    if (map_info->interlace == DFIL_PIXEL)
        HDstrcpy(strInterlace, XML_INTERLACE_PIXEL);
    else if (map_info->interlace == DFIL_LINE)
        HDstrcpy(strInterlace, XML_INTERLACE_LINE);
    else if (map_info->interlace == DFIL_PLANE)
        HDstrcpy(strInterlace, XML_INTERLACE_PLANE);

    /* Swapped width and height to match the pink lines in section 3.4.3 of 
       RasterCode.docx document sent on 3/15/2011. 
       <hyokyung 2011.03.17. 15:36:19>  
    */
    width = map_info->dimsizes[0];
    height = map_info->dimsizes[1];

    start_elm(ofptr, TAG_RIS, indent);
    /* Write the basic Raster information. */
    name_xml = get_string_xml(map_info->name, strlen(map_info->name));
    if(name_xml == NULL){
        fprintf(flog, "get_string_xml() failed.\n");
        fprintf(flog, ":name=%s\n", map_info->name);
        return FAIL;
    }
    if(map_info->is_r8 == TRUE){
        /* Old raster images do not have names.
           Library inserts "Raster Image #%d" name. */
        name_xml[0]='\0';
    }
    fprintf(ofptr, "name=\"%s\" ", name_xml);
    fprintf(ofptr, "height=\"%d\" width=\"%d\" ", (int)height, 
            (int)width);
    fprintf(ofptr, "id=\"ID_R%d\"",
            ++ID_R);
    fprintf(ofptr, ">");
    /* Write attribute information. */
    if(map_info->num_attrs > 0){
        status = write_raster_attrs(ofptr, ri_id, map_info->num_attrs, 
                                    indent+1);
        if(status == FAIL){
            HDfree(name_xml);
            fprintf(flog, "write_raster_attrs() failed\n");
            fprintf(flog, ":ri_id=%ld, tag=%ld, ref=%ld, nattrs=%ld.\n", 
                    (long)ri_id, (long)tag, (long)ref, 
                    (long) map_info->num_attrs);
            return FAIL;
        }
    }
    /* Write the palette reference ID. */
    if(map_info->npals > 0){
        palMapID = palRef_to_palMapID(map_info->pal_ref);
        if (palMapID == 0){
            HDfree(name_xml);
            fprintf(flog, "palRef_to_palMapID() failed.");
            return FAIL;
        }
        else{
            start_elm(ofptr, TAG_PREF, indent+1);
            fprintf(ofptr, "ref=\"ID_P%u\"/>", palMapID);
        }
    }

    /* Write datatype information. */
    status = write_raster_datum(ofptr, map_info->nt, indent+1); 
    if(status == FAIL){
        HDfree(name_xml);
        fprintf(flog, "write_raster_datum() failed.");
        fprintf(flog, ":number_type=%ld\n", (long)map_info->nt);
        return FAIL;
    }

    start_elm(ofptr, TAG_RDATA, indent+1);
    fprintf(ofptr, "dimensionStorageOrder=\"row,column\"");
    if(HDstrlen(map_info->comp_info) > 0)
        fprintf(ofptr, " %s", map_info->comp_info);
    fprintf(ofptr, ">");

    /* Write offset/size information of data blocks. */
    status = write_ris_byte_stream(ofptr, map_info, ri_id, indent+2);
    if(status == FAIL){
        HDfree(name_xml);
        fprintf(flog, "write_ris_byte_stream() failed.");
        fprintf(flog, ":nblocks=%ld\n", (long)map_info->nblocks);
        return FAIL;
    }
    end_elm(ofptr, TAG_RDATA, indent+1);

    /* Write verification values. */
    if(map_info->nblocks > 0){
        start_elm(ofptr, "!--", indent+1);
        fprintf(ofptr, "value(s) for verification; [row,column]");

        /* Allocate image value buffer. */
        image_values = HDmalloc(sizeof(uint8) *
                                (int)map_info->dimsizes[0] *
                                (int)map_info->dimsizes[1] *
                                (int)map_info->ncomp
                                );
        if(image_values == NULL) {
            HDfree(name_xml);
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }

        start[0] = start[1] = 0;
        status = GRreadimage(ri_id, start, NULL, map_info->dimsizes, 
                             (VOIDP)image_values);

        if(status == FAIL){
            HDfree(name_xml);
            HDfree(image_values);
            FAIL_ERROR("GRreadimage() failed.\n");
        }

        /* Write sample values. Adapted from RIS8Palettes.c by Ruth Aydt. */
        i = 0; j = 0;
        indentation(ofptr, indent+2);
        fprintf(ofptr, "%s[%d,%d]=%u", 
                name_xml, i, j, image_values[ (i*width)+j ] );

        i = 0; j = width-1;
        indentation(ofptr, indent+2);
        fprintf(ofptr, "%s[%d,%d]=%u", 
                name_xml, i, j, image_values[ (i*width)+j ] );

        i = height-1; j = 0;
        indentation(ofptr, indent+2);
        fprintf(ofptr, "%s[%d,%d]=%u", 
                name_xml, i, j, image_values[ (i*width)+j ] );

        i = height-1; j = width-1;
        indentation(ofptr, indent+2);
        fprintf(ofptr, "%s[%d,%d]=%u", 
                name_xml, i, j, image_values[ (i*width)+j ] );

        i = height/2 + 1; j = width/3 + 1;
        if ( ( i < height ) && ( j < width ) ) {
            indentation(ofptr, indent+2);
            fprintf(ofptr, "%s[%d,%d]=%u", 
                    name_xml, i, j, image_values[ (i*width)+j ] );
        }


        HDfree(image_values);
        indentation(ofptr, indent+1);
        fprintf(ofptr, "-->");
    }
    HDfree(name_xml);
    end_elm(ofptr, TAG_RIS, indent);

    if (ri_id != FAIL){
        if(GRendaccess(ri_id) == FAIL){
            return FAIL;
        }
    }

    return SUCCEED;
}
/*!
  \fn write_raster_attrs(FILE* ofptr, int32 ri_id, int32 nattrs, intn indent) 

  \brief Write raster image attribute information.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date February 7, 2012
  \note filtered reserved XML character	from the attribute name.

  \date June 13, 2011
  \note fixed dropping MSB in nt_type.

  \date March 17, 2011
  \note created.

  \see write_array_attrs().
*/
unsigned int ID_RA = 0;  /*!< Unique Raster attribute ID.  */

intn 
write_raster_attrs(FILE* ofptr, int32 ri_id, int32 nattrs, intn indent)
{
    int32 attr_index = 0;
    int32 attr_count = 0;
    int32 attr_nt = 0;
    int32 attr_nt_no_endian = 0;
    int32 attr_buf_size = 0;

    char  attr_name[H4_MAX_NC_NAME];
    char *attr_nt_desc = NULL;
    char *name_xml = NULL;
    VOIDP attr_buf = NULL;

    intn status = SUCCEED;

    /* For each file attribute, print its info and values. */
    for (attr_index = 0; attr_index < nattrs; attr_index++)
        {
            int32 offset = -1;
            int32 length = 0;
            /* Get the attribute's name, number type, and number of values. */
            status = GRattrinfo(ri_id, attr_index, attr_name, &attr_nt, 
                                &attr_count);
            if( status == FAIL ){
                FAIL_ERROR("GRattrinfo() failed.");
            }
            /* get number type description of the attribute */
            attr_nt_desc = HDgetNTdesc(attr_nt);
            attr_buf_size = DFKNTsize(attr_nt) * attr_count;
            if (attr_nt_desc == NULL || attr_buf_size <= 0)
                continue;
            attr_nt_no_endian = (attr_nt & 0xff);
            if (attr_nt_no_endian == DFNT_CHAR || 
                attr_nt_no_endian  == DFNT_UCHAR)
                attr_buf = (char *) HDmalloc(attr_buf_size+1);
            else
                attr_buf = (VOIDP)HDmalloc(attr_buf_size);

            if(attr_buf == NULL){
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }
            status = GRgetattr(ri_id, attr_index, attr_buf);
            if( status == FAIL ) {
                HDfree(attr_nt_desc);
                HDfree (attr_buf);
                FAIL_ERROR("GRgetattr() failed.");
            }
            status = GRgetattdatainfo(ri_id, attr_index, &offset, &length);
            if(status == FAIL){
                HDfree(attr_nt_desc);
                HDfree(attr_buf);
                FAIL_ERROR("SDgetoldattdatainfo() failed.");
            }
            
            start_elm(ofptr, TAG_RATTR, indent);
            name_xml =  get_string_xml(attr_name, strlen(attr_name));
            if(name_xml != NULL){
                fprintf(ofptr, "name=\"%s\" id=\"ID_RA%d\">", name_xml, 
                        ++ID_RA);
                HDfree(name_xml);
            }
            else{
                fprintf(flog, "get_string_xml() returned NULL.\n");
                status = FAIL;
            }
            write_array_attribute(ofptr, attr_nt, attr_count, attr_buf, 
                                  attr_buf_size, offset, length, indent+1);
            end_elm(ofptr, TAG_RATTR, indent);
            HDfree(attr_buf);
            HDfree (attr_nt_desc);

        }
    return status;
}

/*!
  \fn void write_raster_datum(FILE *ofptr, int32 dtype, int indent)
  \brief Write datatype information for raster.

  Since Hgetntinfo() doesn't return (u)byte8 string for GR/DFR interface
  and the schema forces to use (u)byte8 for image data, this function 
  is created to pass the schema validation.

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date May 9, 2011
  \note added conditions for DFNT_(U)INT8 cases.

  \date March 21, 2011
  \note created.

  \see write_map_datum()
  \see write_map_datum_char()
*/

intn
write_raster_datum(FILE *ofptr, int32 dtype, int indent)
{
    intn result = SUCCEED;


    switch(dtype & 0xff)
        {
        case DFNT_CHAR:
            start_elm(ofptr, TAG_DATUM, indent); 
            fprintf(ofptr, "dataType=\"%s\"", XML_DATATYPE_BYTE8);
            fputs("/>", ofptr);
            break;
        case DFNT_UCHAR:
            start_elm(ofptr, TAG_DATUM, indent); 
            fprintf(ofptr, "dataType=\"%s\"", XML_DATATYPE_UBYTE8);
            fputs("/>", ofptr);
            break;
        case DFNT_INT8:
            start_elm(ofptr, TAG_DATUM, indent); 
            fprintf(ofptr, "dataType=\"%s\"", XML_DATATYPE_BYTE8);
            fputs("/>", ofptr);
            break;
        case DFNT_UINT8:
            start_elm(ofptr, TAG_DATUM, indent); 
            fprintf(ofptr, "dataType=\"%s\"", XML_DATATYPE_UBYTE8);
            fputs("/>", ofptr);
            break;
        default:
            result = write_map_datum(ofptr, dtype, indent);
            break;
        }
    return result;
}

/*!
  \fn write_ris_byte_stream(FILE* ofptr, RIS_mapping_info_t* map_info, 
  int32 ri_id, int indent)
  \brief Write raster image byte stream information.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date July 21, 2016
  \note added uuid function call.

  \date May 9, 2011
  \note 
  handled the case when map_info->nblocks = 0, which means the data is empty 
  with fill values.
  added ri_id argument.
  changed return type from void to intn.

  \date March 17, 2011
  \note 
  changed return type to void.
  corrected %d to %ld in fprintf to match type.
  
  \date September 29, 2010
  \note created.
  \see write_array_byte_stream()

*/
intn
write_ris_byte_stream(FILE* ofptr, RIS_mapping_info_t* map_info, int32 ri_id,
                      int indent)
{
    int32 j = 0;
    if(map_info->nblocks > 0){
        if(map_info->nblocks > 1){
            start_elm_0(ofptr, TAG_BSTMSET, indent);
            ++indent;
        }

        for (j=0; j < map_info->nblocks; j++) {
            start_elm(ofptr, TAG_BSTREAM, indent);
            if (uuid == 1) {
                write_uuid(ofptr, map_info->offsets[j], map_info->lengths[j]);
            }
            fprintf(ofptr, "offset=\"%ld\" nBytes=\"%ld\"/>", 
                    (long)map_info->offsets[j], (long)map_info->lengths[j]);


        } /* for */
        if(map_info->nblocks > 1){
            --indent;
            end_elm(ofptr, TAG_BSTMSET, indent);
        }
        return SUCCEED;
    } /* if data is not empty */

    if(map_info->nblocks == 0){ /* data is empty with fill value. */

        int32 start[2];         /* used in GRreadimage(). */
        int32 edge[2];          /* used in GRreadimage(). */
        intn status = 0;        /* return status of GRimage(). */
        uint8 fill_value = 0; /* used in GRreadimage(). */
        start[0] = start[1] = 0;
        edge[0] = edge[1] = 1;
        status = GRreadimage(ri_id, start, NULL, edge, 
                             (VOIDP)&fill_value);

        if(status == FAIL){
            FAIL_ERROR("GRreadimage() failed.\n");
        }

        start_elm(ofptr, TAG_FVALUE, indent);        
        fprintf(ofptr, "value=\"");
        fprintf(ofptr, "%u", fill_value);
        fprintf(ofptr, "\"/>");
    }
    return SUCCEED;
}
