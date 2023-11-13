/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2010-2013  The HDF Group                                    *
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
  \file xml_an.c
  \brief Write Annotation information.

  \author Hyo-Kyung Joe Lee (hyoklee@hdfgroup.org)
  \date October 11, 2013
  \note made error message consistent in write_file_attrs_an() and 
  write_pal_attrs_an().
  \note replaced free() with HDfree().

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 19, 2012
  \note added tag to calling parameters for write_pal_attrs_an and updated 
  function

  \author Hyo-Kyung Joe Lee (hyoklee@hdfgroup.org)
  \date August 12, 2010
  \note created.

*/


#include "hmap.h"

extern unsigned int ID_FA;      /* declared in hmap.c */
extern unsigned int ID_GA;      /* declared in xml_vg.c  */
extern unsigned int ID_AA;      /* declared in xml_sds.c  */
extern unsigned int ID_PA;      /* declared in xml_pal.c  */

/*!
  \fn write_file_attrs_an(FILE *ofptr, int32 an_id, intn indent)
  \brief Write file attributes from file annotations.

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung Joe Lee (hyoklee@hdfgroup.org)
  \date Oct 11, 2013
  \note made error message consistent.

*/
intn 
write_file_attrs_an(FILE *ofptr, int32 an_id,  intn indent)
{
    char* buf = NULL;                  /* buffer for annotation */

    int32 ann_id = 0;       /* annotation id */
    int32 ann_length = 0;   /* annotation length */
    int32 n_file_label = 0; /* number of file labels*/
    int32 n_file_desc  = 0; /* number of file descriptions*/
    int32 n_data_label = 0; /* number of data labels. */
    int32 n_data_desc  = 0; /* number of data descriptions. */
    int32 i = 0;
    int32 length = 0;
    int32 offset = 0;
    int32 status = 0;

    status = ANfileinfo(an_id, 
                        &n_file_label, &n_file_desc, 
                        &n_data_label, &n_data_desc);
    if(status == FAIL) {   
        return FAIL;
    }

    if(n_file_label > 0){
        for(i=0; i < n_file_label; i++){
            char* str_trimmed = NULL;

            ann_id = ANselect(an_id, i, AN_FILE_LABEL);
            if(ann_id == FAIL){
                return FAIL;
            }
            ann_length = ANannlen(ann_id);
            if(ann_length == FAIL){
                ANendaccess(ann_id);
                return FAIL;
            }

            buf = (char*) HDmalloc((ann_length+1)*sizeof(char));
            if(buf == NULL){
                ANendaccess(ann_id);
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }

            start_elm(ofptr, TAG_FATTR, indent);

            status = ANreadann(ann_id, buf, ann_length+1);
            if(status == FAIL){
                ANendaccess(ann_id);
                HDfree(buf);
                return FAIL;
            }

            status = ANgetdatainfo(ann_id, &offset, &length);
            if(status == FAIL){
                HDfree(buf);
                return FAIL;
            }

            status = ANendaccess(ann_id);
            if(status == FAIL){
                HDfree(buf);
                return FAIL;
            }
            /* Names are fixed for file annotation. */
            fprintf(ofptr, "name=\"%s\" ", "File Label");
            fprintf(ofptr, "origin=\"File Annotation: Label\" "); 
            fprintf(ofptr, "id=\"ID_FA%d\">", ++ID_FA);       
            /* Write non-numeric datum element. */
            write_map_datum_char(ofptr, DFNT_CHAR, indent+1);
            /* Write attributeData element. */
            write_attr_data(ofptr, offset, length, indent+1);
            /* Write stringValue element. */
            start_elm_0(ofptr, TAG_SVALUE, indent+1);
            /* According to HDF4 reference manual, user must add a NULL. */
            buf[ann_length] = '\0';            

            str_trimmed =  get_string_xml(buf, ann_length);
            if(str_trimmed != NULL) {
                fprintf(ofptr, "%s", str_trimmed);
                HDfree(str_trimmed);
            }
            end_elm_0(ofptr, TAG_SVALUE);            
            end_elm(ofptr, TAG_FATTR, indent);

            if(buf != NULL){
                HDfree(buf);
            }

        }
    }

    if(n_file_desc > 0){
        for(i=0; i < n_file_desc; i++){
            char* str_trimmed = NULL;
            /* Is it ncessary to store ann_id into visited list? */
            ann_id = ANselect(an_id, i, AN_FILE_DESC);
            if(ann_id == FAIL){
                return FAIL;
            }
            ann_length = ANannlen(ann_id);
            if(ann_length == FAIL){
                ANendaccess(ann_id);
                return FAIL;
            }

            buf = (char*) HDmalloc((ann_length+1)*sizeof(char));
            if(buf == NULL){
                ANendaccess(ann_id);
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }

            start_elm(ofptr, TAG_FATTR, indent);

            status = ANreadann(ann_id, buf, ann_length+1);
            if(status == FAIL){
                ANendaccess(ann_id);
                HDfree(buf);
                return FAIL;
            }

            status = ANgetdatainfo(ann_id, &offset, &length);
            if(status == FAIL){
                HDfree(buf);
                return FAIL;
            }

            status = ANendaccess(ann_id);
            if(status == FAIL){
                HDfree(buf);
                return FAIL;
            }
            /* Names are fixed for file annotation. */
            fprintf(ofptr, "name=\"%s\" ", "File Description");
            fprintf(ofptr, "origin=\"File Annotation: Description\" "); 
            fprintf(ofptr, "id=\"ID_FA%d\">", ++ID_FA);       
            /* Write non-numeric datum element. */
            write_map_datum_char(ofptr, DFNT_CHAR, indent+1);
            /* Write attributeData element. */
            write_attr_data(ofptr, offset, length, indent+1);
            /* Write stringValue element. */
            start_elm_0(ofptr, TAG_SVALUE, indent+1);
            /* According to HDF4 reference manual, user must add a NULL. */
            buf[ann_length] = '\0';

            str_trimmed = get_string_xml(buf, ann_length);
            if(str_trimmed != NULL) {
                fprintf(ofptr, "%s", str_trimmed);
                HDfree(str_trimmed);
            }
            end_elm_0(ofptr, TAG_SVALUE);            
            end_elm(ofptr, TAG_FATTR, indent);
            if(buf != NULL){
                HDfree(buf);
            }

        }
    }
    return status;

}   /* end of write_file_attrs_AN */

/*!
  \fn write_group_attrs_an(FILE *ofptr, int32 an_id, int32 tag,  int32 ref, 
  intn indent)
  \brief Write group annotation attributes.

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung Joe Lee (hyoklee@hdfgroup.org)
  \date August 2, 2016
  \note removed ISO C90 warning.


  \date March 14, 2010
  \note 
  corrected return type.

*/
intn 
write_group_attrs_an(FILE *ofptr, int32 an_id, int32 tag, int32 ref, 
                     intn indent)
{
    char* buf = NULL;
    char* str_trimmed = NULL;
    
    int n_ann = 0;
    
    int32 j = 0;        
    int32 length = 0;
    int32 offset = 0;
    int32 ann_id = 0;
    int32 ann_length = 0;
    
    int32* ann_list = NULL;

    intn status = SUCCEED;

    n_ann =  ANnumann(an_id, AN_DATA_LABEL, tag, ref);
    if(n_ann == FAIL){
        FAIL_ERROR("ANnumann() failed.");
    }

    if(n_ann > 0) {
        j = 0;
        ann_list = (int32 *)HDmalloc(n_ann*sizeof(int32));

        if(ann_list == NULL){
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }

        status = ANannlist(an_id, AN_DATA_LABEL, tag, ref, ann_list);
        if(status == FAIL){
            HDfree(ann_list);            
            FAIL_ERROR("ANannlist() failed.");
        }
        
        for(j=0; j < n_ann; j++){
            length = 0;
            offset = 0;
            ann_id = ann_list[j];
            ann_length = ANannlen(ann_id);
            buf = (char*) HDmalloc((ann_length+1)*sizeof(char));
            if(buf == NULL){
                HDfree(ann_list);            
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }

            str_trimmed = NULL;

            status = ANreadann(ann_id, buf, ann_length+1);

            if(status == FAIL){
                ANendaccess(ann_id);
                HDfree(ann_list);            
                HDfree(buf);
                FAIL_ERROR("ANreadann() failed.\n");
            }

            status = ANgetdatainfo(ann_id, &offset, &length);
            if(status == FAIL){
                ANendaccess(ann_id);
                HDfree(ann_list);            
                HDfree(buf);
                FAIL_ERROR("ANgetdatainfo() failed.\n");
            }

            buf[ann_length] = '\0';

            start_elm(ofptr, TAG_GATTR, indent); 
            
            fprintf(ofptr, "name=\"Object Label\" id=\"ID_GA%d\">", 
                    ++ID_GA);
            write_map_datum_char(ofptr, DFNT_CHAR, indent+1);
            /* Write attributeData element. */
            write_attr_data(ofptr, offset, length, indent+1);
            /* Write stringValue element. */
            start_elm_0(ofptr, TAG_SVALUE, indent+1);
            str_trimmed = get_string_xml(buf, ann_length);
            if(str_trimmed != NULL) {
                fprintf(ofptr, "%s", str_trimmed);
                HDfree(str_trimmed);
            }
            end_elm_0(ofptr, TAG_SVALUE);
            end_elm(ofptr, TAG_GATTR, indent);
            HDfree(buf);
            ANendaccess(ann_id);

        }
        HDfree(ann_list);
    }

    n_ann =  ANnumann(an_id, AN_DATA_DESC, tag, ref);
    if(n_ann == FAIL){
        FAIL_ERROR("ANnumann() failed.");
    }

    if(n_ann > 0) {
        j = 0;
        ann_list = (int32 *)HDmalloc(n_ann*sizeof(int32));

        if(ann_list == NULL){
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }

        status = ANannlist(an_id, AN_DATA_DESC, tag, ref, ann_list);
        if(status == FAIL){
            HDfree(ann_list);            
            FAIL_ERROR("ANannlist() failed.");
        }


        for(j=0; j < n_ann; j++){
            length = 0;
            offset = 0;
            ann_id = ann_list[j];
            ann_length = ANannlen(ann_id);
            buf = (char*) HDmalloc((ann_length+1)*sizeof(char));
            if(buf == NULL){
                HDfree(ann_list);            
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }

            str_trimmed = NULL;
            status = ANreadann(ann_id, buf, ann_length+1);

            if(status == FAIL){
                ANendaccess(ann_id);
                HDfree(ann_list);            
                HDfree(buf);
                FAIL_ERROR("ANreadann() failed.\n");
            }

            status = ANgetdatainfo(ann_id, &offset, &length);
            if(status == FAIL){
                ANendaccess(ann_id);
                HDfree(ann_list);            
                HDfree(buf);
                FAIL_ERROR("ANgetdatainfo() failed.\n");
            }

            buf[ann_length] = '\0';

            start_elm(ofptr, TAG_GATTR, indent); 
            fprintf(ofptr, "name=\"Object Description\" id=\"ID_GA%d\">", 
                    ++ID_GA);
            write_map_datum_char(ofptr, DFNT_CHAR, indent+1);
            /* Write attributeData element. */
            write_attr_data(ofptr, offset, length, indent+1);
            /* Write stringValue element. */
            start_elm_0(ofptr, TAG_SVALUE, indent+1);
            str_trimmed = get_string_xml(buf, ann_length);
            if(str_trimmed != NULL) {
                fprintf(ofptr, "%s", str_trimmed);
                HDfree(str_trimmed);
            }
            end_elm_0(ofptr, TAG_SVALUE);
            end_elm(ofptr, TAG_GATTR, indent);
            HDfree(buf);
            ANendaccess(ann_id);

        }
        HDfree(ann_list);
    }

    return status;   
    
}


/*!
  \fn write_array_attrs_an_1(FILE *ofptr, int32 an_id, int32 tag, int32 ref, 
  intn indent)
  \brief Write array annotation attributes.
*/
intn 
write_array_attrs_an_1(FILE *ofptr, int32 an_id, int32 tag, int32 ref, 
                       intn indent)
{

    int n_ann =  ANnumann(an_id, AN_DATA_LABEL, tag, ref);

    if(n_ann > 0) {
        int32 j = 0;
        int32* ann_list = (int32 *)HDmalloc(n_ann*sizeof(int32));
        ANannlist(an_id, AN_DATA_LABEL, tag, ref, ann_list);

        
        for(j=0; j < n_ann; j++){
            int32 length = 0;
            int32 offset = 0;
            int32 ann_id = ann_list[j];
            int32 ann_length = ANannlen(ann_id);
            char* buf = (char*) HDmalloc((ann_length+1)*sizeof(char));
            char* str_trimmed = NULL;
            intn status = ANreadann(ann_id, buf, ann_length+1);

            if(status == FAIL){
                ANendaccess(ann_id);
                HDfree(buf);
                return FAIL;
            }

            status = ANgetdatainfo(ann_id, &offset, &length);
            if(status == FAIL){
                HDfree(buf);
                return FAIL;
            }

            buf[ann_length] = '\0';

            start_elm(ofptr, TAG_AATTR, indent); 
            fprintf(ofptr, "name=\"Object Label\" id=\"ID_AA%d\">", 
                    ++ID_AA);
            write_map_datum_char(ofptr, DFNT_CHAR, indent+1);
            /* Write attributeData element. */
            write_attr_data(ofptr, offset, length, indent+1);
            /* Write stringValue element. */
            start_elm_0(ofptr, TAG_SVALUE, indent+1);
            str_trimmed = get_string_xml(buf, ann_length);
            if(str_trimmed != NULL) {
                fprintf(ofptr, "%s", str_trimmed);
                HDfree(str_trimmed);
            }
            end_elm_0(ofptr, TAG_SVALUE);
            end_elm(ofptr, TAG_AATTR, indent);
            HDfree(buf);
            ANendaccess(ann_id);

        }
        HDfree(ann_list);
    }

    n_ann =  ANnumann(an_id, AN_DATA_DESC, tag, ref);

    if(n_ann > 0) {
        int32 j = 0;
        int32* ann_list = (int32 *)HDmalloc(n_ann*sizeof(int32));
        ANannlist(an_id, AN_DATA_DESC, tag, ref, ann_list);


        for(j=0; j < n_ann; j++){
            int32 length = 0;
            int32 offset = 0;
            int32 ann_id = ann_list[j];
            int32 ann_length = ANannlen(ann_id);
            char* buf = (char*) HDmalloc((ann_length+1)*sizeof(char));
            char* str_trimmed = NULL;
            intn status = ANreadann(ann_id, buf, ann_length+1);

            if(status == FAIL){
                ANendaccess(ann_id);
                HDfree(buf);
                return FAIL;
            }

            status = ANgetdatainfo(ann_id, &offset, &length);
            if(status == FAIL){
                HDfree(buf);
                return FAIL;
            }

            buf[ann_length] = '\0';

            start_elm(ofptr, TAG_AATTR, indent); 
            fprintf(ofptr, "name=\"Object Description\" id=\"ID_AA%d\">", 
                    ++ID_AA);
            write_map_datum_char(ofptr, DFNT_CHAR, indent+1);
            /* Write attributeData element. */
            write_attr_data(ofptr, offset, length, indent+1);
            /* Write stringValue element. */
            start_elm_0(ofptr, TAG_SVALUE, indent+1);
            str_trimmed = get_string_xml(buf, ann_length);
            if(str_trimmed != NULL) {
                fprintf(ofptr, "%s", str_trimmed);
                HDfree(str_trimmed);
            }
            end_elm_0(ofptr, TAG_SVALUE);
            end_elm(ofptr, TAG_AATTR, indent);

            HDfree(buf);
            ANendaccess(ann_id);

        }
        HDfree(ann_list);
    }

    return SUCCEED;   
    
}

/*!
  \fn write_array_attrs_an(FILE *ofptr, int32 sds_id, int32 an_id, intn indent)
  \brief Write array annotation attributes.

  This function utilizes the SDgetanndatainfo() function defined in 
  mfhdf/libsrc/mfdatainfo.c of the HDF4 library.

  We support only DFTAG_SDG (Scientific Data Group).
  The DFTAG_SDG is defined in hdf/src/htags.h as 700.
  Other object annotations will be reported by the data descriptor checker
  at the end.

  \see write_array_attrs_an_1()
*/
intn 
write_array_attrs_an(FILE *ofptr, int32 sds_id, int32 an_id, intn indent)
{
    int32 tag, ref;
    intn i = SDgetanndatainfo(sds_id, AN_DATA_LABEL,  0, NULL, NULL);
    intn j = SDgetanndatainfo(sds_id, AN_DATA_DESC, 0, NULL, NULL);

    if(i > 0 || j > 0){
        ref = SDidtoref(sds_id);
        tag = DFTAG_SDG;
        return write_array_attrs_an_1(ofptr, an_id, tag, ref, indent);
    }
    return SUCCEED;
}

/*!
  \fn write_pal_attrs_an(FILE *ofptr, int32 an_id, uint16 tag, uint16 ref, intn indent)
  \brief Write pal annotation attributes.

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung Joe Lee (hyoklee@hdfgroup.org)
  \date August 2, 2016
  \note removed ISO C90 warning.

  \date Oct 11, 2013
  \note made error message consistent.

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 18, 2012
  \note added tag to calling parameters for new palette handling and updated accordingly

  \author Hyo-Kyung Joe Lee (hyoklee@hdfgroup.org)
  \date March 11, 2011
  \note added memory error handlers.
*/
intn 
write_pal_attrs_an(FILE *ofptr, int32 an_id, uint16 tag, uint16 ref, 
                   intn indent)
{

    ann_type checkType = AN_DATA_LABEL;

    char* buf = NULL;    
    char* str_trimmed = NULL;

    int pass = 0;
    
    int32 j = 0;
    int32 length = 0;
    int32 offset = 0;
    int32 ann_id = 0;
    int32 ann_length = 0;
    
    int32* ann_list = NULL;
    
    intn n_ann = 0;
    intn status = SUCCEED;
    
    for (pass = 0; pass < 2; pass++ ) {
        if ( pass == 0  ) {
            checkType = AN_DATA_LABEL;
        } else {
            checkType = AN_DATA_DESC;
        }
        n_ann = ANnumann(an_id, checkType, tag, ref);
        if(n_ann > 0) {
            j = 0;
            ann_list = (int32 *)HDmalloc(n_ann*sizeof(int32));
            if(ann_list == NULL) {
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }
            ANannlist(an_id, checkType, tag, ref, ann_list);

            for(j=0; j < n_ann; j++){
                length = 0;
                offset = 0;
                ann_id = ann_list[j];
                ann_length = ANannlen(ann_id);
                str_trimmed = NULL;
                buf = (char*) HDmalloc((ann_length+1)*sizeof(char));
                if(buf == NULL) {
                    FAIL_ERROR("HDmalloc() failed: Out of Memory");
                }                

                status = ANreadann(ann_id, buf, ann_length+1);
                if(status == FAIL){
                    ANendaccess(ann_id);
                    HDfree(buf);
                    FAIL_ERROR("ANreadann() failed.");
                }

                status = ANgetdatainfo(ann_id, &offset, &length);
                if(status == FAIL){
                    HDfree(buf);
                    FAIL_ERROR("ANgetdatainfo() failed.");
                }

                buf[ann_length] = '\0';

                start_elm(ofptr, TAG_PATTR, indent); 
                if(checkType == AN_DATA_DESC)
                    fprintf(ofptr, "name=\"Object Description\"");
                else{
                    fprintf(ofptr, "name=\"Object Label\"");
                }
                fprintf(ofptr, " id=\"ID_PA%d\">", ++ID_PA);
                write_map_datum_char(ofptr, DFNT_CHAR, indent+1);
                /* Write attributeData element. */
                write_attr_data(ofptr, offset, length, indent+1);
                /* Write stringValue element. */
                start_elm_0(ofptr, TAG_SVALUE, indent+1);
                str_trimmed = get_string_xml(buf, ann_length);
                if(str_trimmed != NULL) {
                    fprintf(ofptr, "%s", str_trimmed);
                    HDfree(str_trimmed);
                }
                end_elm_0(ofptr, TAG_SVALUE);
                end_elm(ofptr, TAG_PATTR, indent);

                HDfree(buf);
                ANendaccess(ann_id);

            }
            HDfree(ann_list);
        }


    }
    return SUCCEED;
}
