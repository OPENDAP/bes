/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2010-2016  The HDF Group                                    *
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
  \file xml_vg.c
  \brief Write Vgroup information.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date September 17, 2010


*/

#include <uuid/uuid.h>          /* for UUID generation */    
#include "hmap.h"

unsigned int ID_GA = 0; /*!< Unique group attribute id  */

/*!

 \fn write_group(FILE *ofptr, char* name, const char* path, char* class, 
 intn indent)
 \brief Write h4:Group element.
 
 \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
 \date July 27, 2016
 \note genereated uuid for group created by ODL parser.

 \date March 14, 2011
 \note added FAIL case.
 
 */
intn 
write_group(FILE *ofptr, char* name, const char* path, char* class, 
            intn indent)
{
    uuid_t id;         /* for uuid */
    char uuid_str[37]; /* ex. "1b4e28ba-2fa1-11d2-883f-0016d3cca427" + "\0" */
    
    static unsigned int ID_G = 0; /*!< unique id number for h4:Group tag. */

    start_elm(ofptr, TAG_GRP, indent); 
    if(fprintf(ofptr, "name=\"%s\" path=\"%s\"", name, path) < 0)
        return FAIL;


    if(HDstrlen(class)> 0){
        if(fprintf(ofptr, " class=\"%s\"", class) < 0)
            return FAIL;
    }
    
    if(strcmp(path, "N/A") == 0){
        uuid_generate(id);
        uuid_unparse_lower(id, uuid_str);
        
        if(fprintf(ofptr, " id=\"ID_G%d\" uuid=\"%s\">", ++ID_G, uuid_str) < 0)
            return FAIL;        
    }
    else{
        if(fprintf(ofptr, " id=\"ID_G%d\">", ++ID_G) < 0)
            return FAIL;
    }

    return SUCCEED;
}

/*!

 \fn write_group_attrs(FILE *ofptr, int32 id, int32 nattrs, intn indent)
 \brief Write all Vgroup attributes.


 \return FAIL
 \return SUCCEED

 \author Hyo-Kyung Joe Lee (hyoklee@hdfgroup.org)

 \date June 13, 2011
 \note 
 fixed dropping MSB in nt_type.

 \date March 14, 2010
 \note 
 corrected return type.
 added more error handlings.
 
 \see write_VSattrs()


 */
intn write_group_attrs(FILE *ofptr, int32 id, int32 nattrs, intn indent)
{


    VOIDP attr_buf=NULL;

    char  attr_name[H4_MAX_NC_NAME],
         *attr_nt_desc = NULL;
    
    int32 attr_index = 0;
    int32 attr_count = 0;
    int32 attr_nt = 0;
    int32 attr_nt_no_endian = 0;
    int32 attr_buf_size = 0;
    int32 attr_size = 0;
    int32 length = 0;
    int32 offset = -1;

    intn  status = SUCCEED;          /* status from a called routine */


    /* for each file attribute, print its info and values */
    for (attr_index = 0; attr_index < nattrs; attr_index++) {

        int32 nfields = 0;
        uint16 ref = 0;

        status = Vattrinfo2(id, attr_index, attr_name, &attr_nt, &attr_count, 
                            &attr_size, &nfields, &ref);
        if(status == FAIL){
            FAIL_ERROR("Vattrinfo2() failed.");
        }       

        if(nfields > 1){
            /* Save the reference and process it with the Table map generation 
               routine. */
            fprintf(flog, 
                    "write_group_attrs() failed: ");
            fprintf(flog, "%s attribute Vdata has more than 1 field.\n",
                    attr_name);
            /* Maybe contiue or return SUCCEED? 
               <hyokyung 2011.03.14. 11:57:46> */
            return FAIL;       
        }

        status = Vgetattdatainfo(id, attr_index,
                                 &offset, &length);
        if(status == FAIL){
            FAIL_ERROR("Vgetattdatainfo() failed.");
        }
      
        /* Get number type description of the attribute. */

        attr_nt_desc = HDgetNTdesc(attr_nt);
        attr_buf_size = DFKNTsize(attr_nt) * attr_count;

        if (attr_nt_desc == NULL || attr_buf_size <= 0)
            continue;

        attr_nt_no_endian = (attr_nt & 0xff);

        if (attr_nt_no_endian == DFNT_CHAR || 
            attr_nt_no_endian == DFNT_UCHAR)
            attr_buf = (char *)HDmalloc(attr_buf_size+1);
        else
            attr_buf = (VOIDP)HDmalloc(attr_buf_size);

        if(attr_buf == NULL){
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }

        status = Vgetattr2(id, attr_index, attr_buf);
        if( status == FAIL ) {
            HDfree(attr_nt_desc);
            HDfree(attr_buf);
            FAIL_ERROR("Vgetattr2() failed.");
        }

        start_elm(ofptr, TAG_GATTR, indent); 
        fprintf(ofptr, "name=\"%s\" id=\"ID_GA%d\">", attr_name, ++ID_GA);
        /* The rest is identical to SDS attribute handling at the moment. */
        write_array_attribute(ofptr, attr_nt, attr_count, attr_buf, 
                              attr_buf_size, offset, length, indent+1);
        end_elm(ofptr, TAG_GATTR, indent);
        HDfree(attr_nt_desc);
        HDfree(attr_buf);
    }  /* for each file attribute */
    return status;
}   /* end of write_group_attrs */

/*!
  \fn write_vgroup_noroot(FILE *ofptr, int32 file_id, int32 sd_id, int32 gr_id,
  int32 an_id, int indent, ref_count_t *sd_visited, ref_count_t *vs_visited, 
  ref_count_t *vg_visited, ref_count_t *ris_visited)

  \brief Write any Vgroups that are not rooted.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date July 6, 2011
  \note 
  corrected the indentation level for Group element (1 to \a indent).

  \date March 15, 2010
  \note 
  cleaned up error messages.
  removed internal vgroup checking borrowed from h4toh5 tool.

  \return FAIL
  \return SUCCEED

 */
intn 
write_vgroup_noroot(FILE *ofptr, int32 file_id, int32 sd_id, int32 gr_id,
                    int32 an_id, int indent,
                    ref_count_t *sd_visited, 
                    ref_count_t *vs_visited, 
                    ref_count_t *vg_visited, 
                    ref_count_t *ris_visited)

{

    char* vgname = NULL;
    char* vgclass = NULL; /* Vgroup class name */

    int skip = 0;    

    int32 n = 0;
    int32 vg = FAIL;
    int32 vgid = -1;
    int32 ref = -1;
    
    intn status = SUCCEED;

    uint16 cname_len = 0;	/* length of vgroup's classname */
    uint16 name_len = 0;	/* length of vgroup's name */



    while ((vgid = Vgetid(file_id, vgid)) != -1){

        skip = 0;

        vg = Vattach(file_id, vgid, "r");

        if (vg == FAIL) {
            FAIL_ERROR("Vattach() failed.");
        }

        if(Vgetclassnamelen(vg, &cname_len) == FAIL) {
            if(Vdetach(vg) == FAIL){
                FAIL_ERROR("Vdetach() failed.");
            }
            FAIL_ERROR("Vgetclassnamelen() failed.");
        }
        else{
            vgclass = (char*) HDmalloc((int)(cname_len+1)*sizeof(char));
            if(vgclass == NULL){
                if(Vdetach(vg) == FAIL){
                    FAIL_ERROR("Vdetach() failed.");
                }
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }
        }


        if(Vgetclass(vg, vgclass) == FAIL) {
            HDfree(vgclass);
            if(Vdetach(vg) == FAIL){
                FAIL_ERROR("Vdetach() failed.");
            }
            FAIL_ERROR("Vgetclass() failed.");
            return FAIL;
        }

        /* Ignore reserved HDF groups - adapted from h4toh5vgroup.c.  
        if(vgclass != NULL) {
            if(strcmp(vgclass,_HDF_ATTRIBUTE) == 0) 
                skip = 1;

            if(strcmp(vgclass,_HDF_VARIABLE)  == 0)
                skip = 1;

            if(strcmp(vgclass,_HDF_DIMENSION) == 0)
                skip = 1;

            if(strcmp(vgclass,_HDF_UDIMENSION)== 0)
                skip = 1;

            if(strcmp(vgclass,_HDF_CDF) == 0)
                skip = 1;

            if(strcmp(vgclass,GR_NAME) == 0)
                skip = 1;

            if(strcmp(vgclass,RI_NAME) == 0)
                skip = 1;
        }
       */
        /* Skip internal Vgroup. */
        if(Visinternal(vgclass)){
            skip = 1;
        }

        ref = VQueryref(vg);
        if(ref == FAIL){
            HDfree(vgclass);
            if(Vdetach(vg) == FAIL){
                FAIL_ERROR("Vdetach() failed.");
            }
            FAIL_ERROR("VQueryref() failed.");
        }

        /* Make sure that this vgroup is not visisted. */
        if (ref == ref_count(vg_visited, ref)){
            skip = 1;
        }

        if (ref == ref_count(sd_visited, ref)){
            skip = 1;
        }


        if (ref == ref_count(vs_visited, ref)){
            skip = 1;
        }

        if (ref == ref_count(ris_visited, ref)){
            skip = 1;
        }

        if(skip == 0){
            char* class_xml = NULL;
            if(FAIL == Vgetnamelen(vg, &name_len)){
                HDfree(vgclass);
                if(Vdetach(vg) == FAIL){
                    FAIL_ERROR("Vdetach() failed.");
                }
                FAIL_ERROR("Vgetnamelen() failed.");
            };
            
            vgname = (char *) HDmalloc(sizeof(char *) * (name_len+1));
            if (vgname == NULL)
                {
                    HDfree(vgclass);
                    if(Vdetach(vg) == FAIL){
                        FAIL_ERROR("Vdetach() failed.");
                    }
                    FAIL_ERROR("HDmalloc() failed: Out of Memory");
                }
            if(Vinquire(vg, &n, vgname) == FAIL){
                HDfree(vgclass);
                if(Vdetach(vg) == FAIL){
                    FAIL_ERROR("Vdetach() failed.");
                }
                FAIL_ERROR("Vinquire() failed.");
            }
            
            /* Clean up class name. */
            class_xml =  get_string_xml(vgclass, strlen(vgclass));
            /* Write Group element.  */
            write_group(ofptr, vgname, "Not Applicable", class_xml, indent);
            /* Free memory. */
            HDfree(vgname);
            if(class_xml != NULL) {
                HDfree(class_xml);
            }
            status =
                depth_first(file_id, vg, sd_id, gr_id, an_id, 
                            "Not Applicable", ofptr, indent+1,
                            sd_visited, vs_visited, vg_visited, ris_visited);
            if(status == FAIL){
                fprintf(flog, "depth_first() failed.");
                fprintf(flog, ":path=Not Applicable\n");
                HDfree(vgclass);
                if(Vdetach(vg) == FAIL){
                    FAIL_ERROR("Vdetach() failed.");            
                };
                return FAIL;
            }
            end_elm(ofptr, TAG_GRP, indent);

        }
        HDfree(vgclass);
        if(Vdetach(vg) == FAIL){
            FAIL_ERROR("Vdetach() failed.");            
        };
    };                          /* while() */

    return status;

}
