/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2010-2016 The HDF Group                                     *
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
  \file xml_hmap.c
  \brief Have general routines for writing map information to xml file.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date October 11, 2013
  \note made error message consistent in ref_count().

  \date June 29, 2011
  \note added libgen.h for basename() and dirname().


  \date May 10, 2011
  \note write_ignored_tags() is added.

  \date October 14, 2010
  \note write_byte_streams() is added.

  \author Peter Cao (xcao@hdfgroup.org)
  \date August 23, 2007
  \note created.
*/

#include "hmap.h"
#include <libgen.h>             /* for basename() and dirname(). */
#include <uuid/uuid.h>          /* for UUID generation */
#include <openssl/md5.h>        /* for MD5 checksum */

int uuid = 0;                   /*!< Add UUID and MD5 for byte streams.  */
char* optfile = NULL;

/*!
  \fn depth_first(int32 file_id, int32 vgid, int32 sd_id, int32 gr_id, 
  int32 an_id,
  const char *path, FILE *ofptr, int indent, 
  ref_count_t *sd_visited, 
  ref_count_t *vs_visited, 
  ref_count_t *vg_visited, 
  ref_count_t *ris_visited)

  \brief Traverse HDF4 objects in depth-first manner.

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung Joe Lee (hyoklee@hdfgroup.org)
  \date March 15, 2010
  \note 
  added FAIL case for write_map_vdata.

  \date March 14, 2010
  \note 
  corrected return type.
  redirected cycle error to ferr.
  moved cycle error below name processing.
*/
intn 
depth_first(int32 file_id, int32 vgid, int32 sd_id, int32 gr_id, int32 an_id,
            const char *path, FILE *ofptr, int indent, 
            ref_count_t *sd_visited, 
            ref_count_t *vs_visited, 
            ref_count_t *vg_visited, 
            ref_count_t *ris_visited)
{

    SD_mapping_info_t sd_map_info;
    VS_mapping_info_t vs_map_info;
    RIS_mapping_info_t ris_map_info;

    char class[VGNAMELENMAX];
    char name[H4_MAX_NC_NAME];
    char new_path[MAX_PATH_LEN];

    int i = 0;
    int nelems = Vntagrefs(vgid);

    int32 *tags = NULL;
    int32 *refs = NULL;
    int32 index = 0;
    int32 num_attrs = 0;
    int32 ref = -1;
    int32 sdsid = 0;
    int32 tag = -1;
    int32 vdata_id = 0;
    int32 vgroup_id = 0;

    intn status = SUCCEED;

    if (nelems <=0 )
        return SUCCEED;

    if (nelems == FAIL){
        FAIL_ERROR("Vntagrefs() failed.");
    }

    HDmemset(new_path, 0, MAX_PATH_LEN);
    HDmemset(name, 0, H4_MAX_NC_NAME);
    HDmemset(class, 0, VGNAMELENMAX);


    tags = (int32 *)HDmalloc(nelems*sizeof(int32));
    if(tags == NULL){
        FAIL_ERROR("HDmalloc() failed: Out of Memory");
    }
    refs = (int32 *)HDmalloc(nelems*sizeof(int32));
    if(refs == NULL){
        FAIL_ERROR("HDmalloc() failed: Out of Memory");
    }

    status = Vgettagrefs(vgid, tags, refs, nelems);
    if(status == FAIL){
        FAIL_ERROR("Vgettagrefs() failed.");
    }

    for (i=0; i < nelems; i++) {
        char* name_xml = NULL;
        char* class_xml = NULL;

        tag = tags[i];
        ref = refs[i];

        switch (tag) {
        case DFTAG_RI8:
        case DFTAG_RIG:
            HDmemset(&ris_map_info, 0, sizeof(ris_map_info));
            write_map_ris(ofptr, file_id, gr_id, tag, ref, path, 
                          &ris_map_info, indent, ris_visited);
            RISfree_mapping_info(&ris_map_info);
            break;

        case DFTAG_SD:
        case DFTAG_SDG:
        case DFTAG_NDG:
            HDmemset(&sd_map_info, 0, sizeof(sd_map_info));

            index =  SDreftoindex(sd_id, ref);
            if(index == FAIL){
                FAIL_ERROR("SDreftoindex() failed.");
            }

            sdsid = SDselect(sd_id, index);
            if(sdsid == FAIL){
                FAIL_ERROR("SDselect() failed.");
            }
            
            status = 
                write_map_sds(ofptr, sdsid, an_id, path, &sd_map_info, indent, 
                              sd_visited);
            if(status == FAIL){
                fprintf(flog, "write_map_sds() failed.");
                fprintf(flog, ":path=%s, id=%ld\n", path, (long)sdsid);
                return FAIL;
            }
            SDfree_mapping_info(&sd_map_info);
            if(SDendaccess(sdsid) == FAIL){
                FAIL_ERROR("SDendaccess() failed.");
            }
            break;

        case DFTAG_VH:
        case DFTAG_VS:
            HDmemset(&vs_map_info, 0, sizeof(vs_map_info));
            vdata_id = VSattach(file_id, ref, "r");
            if(vdata_id == FAIL){
                FAIL_ERROR("VSattach() failed.");
            }
            status = write_map_vdata(ofptr, vdata_id, path, &vs_map_info, 
                                     indent, vs_visited);
            if(status == FAIL){
                fprintf(flog, "write_map_vdata() failed.");
                fprintf(flog, ":path=%s, name=%s\n", path, vs_map_info.name);
                VSfree_mapping_info(&vs_map_info);
                return FAIL;
            }

            if(VSdetach(vdata_id) == FAIL){
                FAIL_ERROR("VSdetach() failed.");
            }
            VSfree_mapping_info(&vs_map_info);
            break;

        case DFTAG_VG:
            vgroup_id = Vattach(file_id, ref, "r");

            if(Vgetname(vgroup_id, name) == FAIL) {
                FAIL_ERROR("Vgetname() failed.");
            };
            
            if(Vgetclass(vgroup_id, class) == FAIL) {
                FAIL_ERROR("Vgetclass() failed.");
            };

            /* Clean string. */
            name_xml = get_string_xml(name, strlen(name));
            class_xml = get_string_xml(class, strlen(class));

            /* Give an UMAPPABLE message if cycle among groups is detected. */
            if (ref == ref_count(vg_visited, ref)){
		fprintf(ferr, "UNMAPPABLE:");
                fprintf(ferr, "Cycle is detected in group hierarchy.");
		fprintf(ferr, ":path=%s, name=%s\n", path, name);
                ++num_errs;

                if(name_xml != NULL) {
                    HDfree(name_xml);
                }

                if(class_xml != NULL) {
                    HDfree(class_xml);
                }
                if(Vdetach(vgroup_id) == FAIL){
                    FAIL_ERROR("Vdetach() failed.");
                }
		continue;
            }

            status = write_group(ofptr, name_xml, path, class_xml, indent);
            if(status == FAIL){
                fprintf(flog, "write_group() failed:");
                fprintf(flog, "path=%s, name=%s\n", path, name_xml);
                return FAIL;
            }

            /* Write attributes. */
            num_attrs = (int32)Vnattrs2(vgroup_id);
            if(num_attrs == FAIL){
                FAIL_ERROR("Vnattrs2() failed.");
            }
            if(num_attrs > 0) {
                status = write_group_attrs(ofptr, vgroup_id, num_attrs, 
                                           indent+1);
                if(status == FAIL){

                    fprintf(flog, "write_group_attrs() failed:");
                    fprintf(flog, "path=%s, name=%s\n", path, name_xml);
                    if(name_xml != NULL) {
                        HDfree(name_xml);
                    }

                    if(class_xml != NULL) {
                        HDfree(class_xml);
                    }
                    if(Vdetach(vgroup_id) == FAIL){
                        FAIL_ERROR("Vdetach() failed.");
                    }
                    return FAIL;                    
                }
            }
            HDstrcpy(new_path, path);

            /* For non-rooted groups, skip appending path.  */
            if(strncmp(path, "Not Applicable", 14) != 0){
                HDstrcat(new_path, "/");
                HDstrcat(new_path, name_xml);
            }


           status =  depth_first(file_id, vgroup_id, sd_id, gr_id, an_id, 
                                 new_path, ofptr, indent+1, 
                                 sd_visited, vs_visited, 
                                 vg_visited, ris_visited);
           if(status == FAIL){
                fprintf(flog, "depth_first() failed:");
                fprintf(flog, "new_path=%s\n", new_path);
               return FAIL;
           }
            
            end_elm(ofptr, TAG_GRP, indent); 

            if(name_xml != NULL) {
                HDfree(name_xml);
            }

            if(class_xml != NULL) {
                HDfree(class_xml);
            }

            if(Vdetach(vgroup_id) == FAIL){
                FAIL_ERROR("Vdetach() failed.");
            }

            break;
        default:
            break;
        } /* switch */
    }

    /* Clean up resources. */
    if(tags)
        HDfree(tags);
    if(refs)
        HDfree(refs);
    return status;
}

/*!

\fn end_elm(FILE *ofptr, char *elm_name, int indent)
\brief  End an XML element with indentation.

*/
void
end_elm(FILE *ofptr, char *elm_name, int indent)
{
    indentation(ofptr, indent);
    fprintf(ofptr, "</%s>", elm_name);
}

/*!

\fn void end_elm_0(FILE *ofptr, char *elm_name)
\brief End an XML element without indentation.
  
*/
void
end_elm_0(FILE *ofptr, char *elm_name)
{
    fprintf(ofptr, "</%s>", elm_name);
}

/*!

\fn void end_elm_c(FILE *ofptr, char *elm_name)
\brief End an XML element with comment.
  
*/
void
end_elm_c(FILE *ofptr, char *elm_name)
{
    fprintf(ofptr, "</%s> -->", elm_name);
}

/*!
  \fn get_escape_xml(char *s)
  \brief Replace XML reserved characters and escape them.

  The \a s must be a null terminated string and has enough buffer size
  to hold all replaced characters. 
  This function is primarily used to clean object names since it escapes
  quote character.


  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Aug 20, 2010
  \note modified from ap_escape_html() in  http-2.2.4/server/util.c.

  \see get_string_xml()
*/
void
get_escape_xml(char *s)
{
    int i = 0;
    int j = 0;
    char *x = NULL;

    /* first, count the number of extra characters */
    for (i = 0, j = 0; s[i] != '\0'; i++)
        if (s[i] == '<' || s[i] == '>')
            j += 3;
        else if (s[i] == '&')
            j += 4;
        else if (s[i] == '"')
            j += 5;

    if (j == 0)
        return;               /* It's already clean. */

    x = HDmalloc((i + j + 1)* sizeof(char));
    for (i = 0, j = 0; s[i] != '\0'; i++, j++)
        if (s[i] == '<') {
            memcpy(&x[j], "&lt;", 4);
            j += 3;
        }
        else if (s[i] == '>') {
            memcpy(&x[j], "&gt;", 4);
            j += 3;
        }
        else if (s[i] == '&') {
            memcpy(&x[j], "&amp;", 5);
            j += 4;
        }
        else if (s[i] == '"') {
            memcpy(&x[j], "&quot;", 6);
            j += 5;
        }
        else
            x[j] = s[i];

    x[j] = '\0';
    strncpy(s, x, i+j+1);
    HDfree(x);
}

/*!
  \fn get_string_xml(char *s, int count)
  \brief Replace XML reserved characters and escape them.

  This generates a clean string that can be used inside \ref TAG_SVALUE element.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Apr 1, 2011
  \note replaced isspace() with the specific valid XML character set.

  \date Mar 18, 2011
  \note cleaned up error message.

  \date Sep 13, 2010
  \note modified from ap_escape_html() in  http-2.2.4/server/util.c.

  \see get_trimmed_xml()

*/
char* 
get_string_xml(char *s, int count)
{
    int i = 0;
    int j = 0;
    char* x = NULL;


    /* First, count the number of extra characters */
    for (i = 0, j = 0; i < count; i++){
        if(!isascii(s[i]))
            j += 3;
        else if (s[i] == '<' || s[i] == '>')
            j += 3;
        else if (s[i] == '&')
            j += 4;
        else if (s[i] == '\0'){
            j += 1;
        }
        else if(!isprint(s[i]) && 
                (s[i] != '\n') &&
                (s[i] != '\t') &&
                (s[i] != '\r')){
            j += 3;
        }
#ifdef ESCAPE_QUOTE
        else if (s[i] == '"')
            j += 5;
#endif

    }

    x = HDmalloc((i + j + 1)* sizeof(char));

    if(x == NULL){
        fprintf(flog, "HDmalloc() failed: Out of Memory\n");
        return NULL;
    }

    for (i = 0, j = 0; i < count; i++, j++)
        if(!isascii(s[i])) {
            char hex[5];
            hex[4] = '\0';
            sprintf(hex, "\\%03o", (uchar8)s[i]);
            memcpy(&x[j], hex, 4);
            j += 3;
        }
        else if (s[i] == '<') {
            memcpy(&x[j], "&lt;", 4);
            j += 3;
        }
        else if (s[i] == '>') {
            memcpy(&x[j], "&gt;", 4);
            j += 3;
        }
        else if (s[i] == '&') {
            memcpy(&x[j], "&amp;", 5);
            j += 4;
        }
        else if (s[i] == '\0'){
            memcpy(&x[j], "\\0", 2);
            j += 1;
        }
        else if(!isprint(s[i]) &&
                (s[i] != '\n') &&
                (s[i] != '\t') &&
                (s[i] != '\r')){
            char hex[5];
            hex[4] = '\0';
            sprintf(hex, "\\%03o", (uchar8)s[i]);
            memcpy(&x[j], hex, 4);
            j += 3;
        }
#ifdef ESCAPE_QUOTE
        else if (s[i] == '"') {
            memcpy(&x[j], "&quot;", 6);
            j += 5;
        }
#endif
        else{
            x[j] = s[i];
        }

    x[j] = '\0';
    return x;

}

/*!

\fn indentation(FILE *ofptr, int indent)
\brief Indent at the beginning of line.

*/
void
indentation(FILE *ofptr, int indent)
{
    int i = 0;

    fputs("\n", ofptr);
    for (i=0; i < indent; i++)
        fputs(XML_INDENT, ofptr);
}


/*!

  \fn ref_count(ref_count_t *count, int32 ref)
  \brief  Insert \a ref into the visited list.

  It first searches if \a ref is already exists in refs array of the \a count.
  If \a ref is not found, add the \a ref into the visited list.

  \return FAIL if \a count is NULL or memory allocation fails.
  \return 0 if \a ref is added.
  \return \a ref if it already exists.

  \author Hyo-Kyung Joe Lee (hyoklee@hdfgroup.org)
  \date October 11, 2013
  \note made error message consistent.

  \date March 17, 2010
  \note changed -1 to FAIL.


*/
int32 
ref_count(ref_count_t *count, int32 ref)
{
    int i;

    if (count == NULL)
        return FAIL;

    /* Search if the ref is visited. */
    for (i=0; i < count->size; i++) {
        if (count->refs[i] == ref)
            return ref;
    }

    /* Double the counter size if it reaches the max size. */
    if (count->size == count->max_size) {
        count->max_size *= 2;
        count->refs = (int32 *) HDrealloc((VOIDP) count->refs, 
                                          count->max_size * sizeof(int32));
        if(count->refs == NULL){
            FAIL_ERROR("HDrealloc() failed: Out of Memory");
        }

    }

    /* Add the newly visited ref. */
    count->refs[count->size] = ref;
    count->size++;
    return 0;
}

/*!

\fn ref_count_free(ref_count_t *count)
\brief Deallocate the list of object references.

*/
void 
ref_count_free(ref_count_t *count) {
    if (count == NULL)
        return;

    if (count->refs) {
        HDfree(count->refs);
        count->refs = NULL;
    }
}

/*!

  \fn ref_count_init(ref_count_t *count)
  \brief Initialize the structure that stores a list of object references.

  \return FAIL if \a count is NULL or memory allocation fails.
  \return SUCCEED if memory allocation succeeds.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date March 17, 2010
  \note 
  added error handler for memory allocation.
  changed return type form void to intn.

*/
intn
ref_count_init(ref_count_t *count) 
{
    if (count == NULL)
        return FAIL;

    count->size = 0;
    count->max_size = 100;
    count->refs = (int32 *)HDmalloc(count->max_size*sizeof(int32));
    if(count->refs == NULL){
        FAIL_ERROR("HDmalloc() failed: Out of Memory");
    }
    return SUCCEED;
}



/*!

\fn start_elm(FILE *ofptr, char *elm_name, int indent)
\brief Start an XML element.

*/
void
start_elm(FILE *ofptr, char *elm_name, int indent)
{
    indentation(ofptr, indent);
    fprintf(ofptr, "<%s ", elm_name);
}


/*!

\fn void start_elm_0(FILE *ofptr, char *elm_name, int indent)
\brief Start an XML element without any attributes.
  
*/
void
start_elm_0(FILE *ofptr, char *elm_name, int indent)
{
    indentation(ofptr, indent);
    fprintf(ofptr, "<%s>", elm_name);
}

/*!

\fn start_elm_c(FILE *ofptr, char *elm_name, int indent)
\brief Start an XML element with comment.

*/
void
start_elm_c(FILE *ofptr, char *elm_name, int indent)
{
    indentation(ofptr, indent);
    fprintf(ofptr, "<!-- <%s>", elm_name);
}

/*!
  \fn write_uuid(FILE *ofptr, int32 offset, int32 length)
  \brief Write uuid and md5 checksum for \a length binary stream at \a offset
         to \a ofptr.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date July 20, 2016
  \note created.

*/

void
write_uuid(FILE *ofptr, int32 offset, int32 length)
{
    uuid_t id;         /* for uuid */
    char uuid_str[37]; /* ex. "1b4e28ba-2fa1-11d2-883f-0016d3cca427" + "\0" */
    MD5_CTX cs;        /* for MD5 checksum */
    int fd_cs = -1;
    int count = -1;
    int n=0;
    char tmp_str[3];
    char* buf = NULL;
    char *checksum_str = NULL;  /* for user supplied checksum */    
    unsigned char out[MD5_DIGEST_LENGTH];
    
    uuid_generate(id);
    uuid_unparse_lower(id, uuid_str);
    
    MD5_Init(&cs);
    fd_cs = open(optfile, O_RDONLY, 0);
    if (fd_cs < 0){
        perror("open");
        return;        
    }
    else {
        buf = (char *)HDmalloc(length);        
    }
    count = pread(fd_cs, buf, length, offset);
    if (count < 0) {
        perror("pread");
        return;
    }
    else {
        /* Allocate memory for checksum_str. */
        checksum_str = (char*) HDmalloc((MD5_DIGEST_LENGTH * 2)
                                           * sizeof(char) + 1);
        if(checksum_str == NULL){
            fprintf(flog, "HDmalloc() failed: Out of Memory\n");
            return;
        }
        HDmemset(checksum_str, 0, 2*MD5_DIGEST_LENGTH + 1);
        
        MD5_Init(&cs);
        MD5_Update(&cs, buf, count);
        MD5_Final(out, &cs);

        for(n=0; n < MD5_DIGEST_LENGTH; n++){
            HDmemset(tmp_str, 0, 3);
            snprintf(tmp_str, 3, "%02x", out[n]);
            tmp_str[2] = '\0';
            strncat(checksum_str, tmp_str, 2);
        }
        checksum_str[MD5_DIGEST_LENGTH * 2] = '\0';
        
        close(fd_cs);                
    }

    
    fprintf(ofptr, " uuid=\"%s\" md5=\"%s\" ", uuid_str, checksum_str);
    if (checksum_str != NULL){
        HDfree(checksum_str);
    }
    if (buf != NULL){
        HDfree(buf);
    }
}


/*!

\fn write_attr_data(FILE *ofptr, int32 offset, int32 length, int indent)

\brief Write attribute data.

\author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
\date Jul 21, 2016
\note added uuid function call.

\date Sep 24, 2010
\note added.

*/
void write_attr_data(FILE *ofptr, int32 offset, int32 length, int indent)
{
    start_elm_0(ofptr, TAG_ATTDATA, indent);
    start_elm(ofptr, TAG_BSTREAM, indent+1);
    if(offset == -1){
        fprintf(ofptr, "offset=\"N/A\" nBytes=\"%ld\"/>", (long)length); 
    }
    else {
        if(uuid == 1) {
            write_uuid(ofptr, offset, length);
        }
        
        fprintf(ofptr, "offset=\"%ld\" nBytes=\"%ld\"/>", 
                (long)offset, (long)length);
    }
    end_elm(ofptr, TAG_ATTDATA, indent);

}



/*!

\fn write_attr_value(FILE *ofp, int32 nt, int32 cnt, VOIDP databuf, 
intn indent)
\brief Write attribute values.

\author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
\date Aug 15, 2011
\note changed output format for uint8, uint16, and uint32 data type.


*/
void 
write_attr_value(FILE *ofp, int32 nt, int32 cnt, VOIDP databuf, intn indent)
{
    intn i = 0;

    if( NULL == databuf || NULL == ofp)
        return;

    switch (nt & 0xff )
        {
        case DFNT_CHAR:
        case DFNT_UCHAR: 
            if(cnt > 0){
                char* dst = get_string_xml((char*)databuf, cnt);
                fprintf(ofp, "%s", dst);
                if(dst != NULL)
                    HDfree(dst);
            }
            break;
        case DFNT_UINT8:
            for (i=0; i<cnt; i++) {
                if(i != cnt-1){
                    fprintf(ofp, "%u ", ((uint8 *)databuf)[i]);
                }
                else{
                    fprintf(ofp, "%u", ((uint8 *)databuf)[i]);
                }
            }
            break;
        case DFNT_INT8:
            for (i=0; i<cnt; i++) {
                if(i != cnt-1){
                    fprintf(ofp, "%d ", ((int8 *)databuf)[i]);
                }
                else{
                    fprintf(ofp, "%d", ((int8 *)databuf)[i]);
                }
            }
            break;
        case DFNT_UINT16:
            for (i=0; i<cnt; i++) {
                if(i != cnt-1){
                    fprintf(ofp, "%u ", ((uint16 *)databuf)[i]);
                }
                else{
                    fprintf(ofp, "%u", ((uint16 *)databuf)[i]);
                }
            }
            break;
        case DFNT_INT16:
            for (i=0; i < cnt; i++) {
                if(i != cnt-1){
                    fprintf(ofp, "%d ", ((int16 *)databuf)[i]);
                }
                else{
                    fprintf(ofp, "%d", ((int16 *)databuf)[i]);

                }
            }
            break;
        case DFNT_UINT32:
            for (i=0; i<cnt; i++) {
                if(i != cnt-1){
                    fprintf(ofp, "%lu ", (unsigned long)((uint32 *)databuf)[i]);
                }
                else {
                    fprintf(ofp, "%lu",  (unsigned long)((uint32 *)databuf)[i]);
                }
            }
            break;
        case DFNT_INT32:
            for (i=0; i<cnt; i++) {
                if(i != cnt-1){
                    fprintf(ofp, "%d ", (int)((int32 *)databuf)[i]);
                }
                else{
                    fprintf(ofp, "%d", (int)((int32 *)databuf)[i]);
                }
            }
            break;
        case DFNT_FLOAT32:
            for (i=0; i<cnt; i++) {
                if(i != cnt-1){
                    fprintf(ofp, "%f ", ((float32 *)databuf)[i]);
                }
                else{
                    fprintf(ofp, "%f", ((float32 *)databuf)[i]);
                }
            }
            break;
        case DFNT_FLOAT64:
            for (i=0; i<cnt; i++) {
                if( i!= cnt-1){
                    fprintf(ofp, "%f ", ((float64 *)databuf)[i]);
                }
                else {
                    fprintf(ofp, "%f", ((float64 *)databuf)[i]);
                }
            }
            break;
        default:
            break;
        }
}


/*!
  \fn write_byte_stream(FILE *ofptr, int32 offset, int32 length, 
  intn indent)
  \brief Write offset and length information.

  If offset is -1, it writes N/A for offset and 0 for length.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date July 21, 2016
  \note added uuid function call.

  \date October 14, 2010
  \note created.


*/
void 
write_byte_stream(FILE *ofptr, int32 offset, int32 length, intn indent)
{
    start_elm(ofptr, TAG_BSTREAM, indent);
    
    if(offset == -1){
        fprintf(ofptr, "offset=\"N/A\" nBytes=\"0\"/>");
    }
    else {
        if (uuid == 1) {
            write_uuid(ofptr, offset, length);
        }
        fprintf(ofptr, "offset=\"%ld\" nBytes=\"%ld\"/>", 
                (long) offset, (long) length);
    }

}

/*!
  \fn write_byte_streams(FILE *ofptr, int32 nblocks, int32* offsets, 
  int32* lengths, intn indent)
  \brief Write a merged offset and length information.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date October 14, 2010


*/
void 
write_byte_streams(FILE *ofptr, int32 nblocks,  int32* offsets, int32* lengths,
                   intn indent)
{
    int j = 0;
    int32 start_offset = -1;
    int32 cum_length = 0;
    int written = 1;
    int wnblocks = 0;           /* number of blocks to be written. */

    /* Loop twice to determine whether we have to use byteStreamSet or not. */

    for (j=0; j < nblocks; j++) {
        if(written == 1){
            /* Reset everything. */
            start_offset = offsets[j]; 
            cum_length = 0;
        }

        if( (j + 1) < nblocks){ /* Not a last element  */

            if(offsets[j+1] == 
               (start_offset + cum_length + lengths[j])){

                /* If next offset is the sum of the previous one, 
                   skip writing an element and increment cum length. */
                cum_length += lengths[j];

                written = 0;
            }
            else{
                if(written == 0) { /* Write the accumulated one */
                    /* Add the current one.. */
                    cum_length += lengths[j];
                    ++wnblocks;
                    written = 1;
                }
                else{           /* Write the current one */
                    cum_length = lengths[j];
                    ++wnblocks;

                }
            }
        }
        else {                  /* Last element */
            if(written == 1){
                ++wnblocks;
            }
            else{
                /* Add the final length. */
                cum_length += lengths[j];
                /* Write element.  */
                ++wnblocks;
            }
            
        }

    }


    if(wnblocks > 1){
        start_elm_0(ofptr, TAG_BSTMSET, indent);
        ++indent;
    }

    written = 1;                /* Reset from the previous loop. */
    start_offset = -1;
    cum_length = 0;

    /* Now, loop for actual writing.  */
    for (j=0; j < nblocks; j++) {

        if(written == 1){
            /* Reset everything. */
            start_offset = offsets[j]; 
            cum_length = 0;
        }

        if( (j + 1) < nblocks){ /* Not a last element  */

            if(offsets[j+1] == 
               (start_offset + cum_length + lengths[j])){

                /* If next offset is the sum of the previous one, 
                   skip writing an element and increment cum length. */
                cum_length += lengths[j];

                written = 0;
            }
            else{
                if(written == 0) { /* Write the accumulated one */
                    /* Add the current one.. */
                    cum_length += lengths[j];
                    write_byte_stream(ofptr,  start_offset, 
                                      cum_length,
                                      indent);
                    written = 1;
                }
                else{           /* Write the current one */
                    cum_length = lengths[j];
                    write_byte_stream(ofptr,  start_offset, 
                                      cum_length,
                                      indent);

                }
            }
        }
        else {                  /* Last element */
            if(written == 1){
                write_byte_stream(ofptr, offsets[j],
                                  lengths[j], indent);
            }
            else{
                /* Add the final length. */
                cum_length += lengths[j];
                /* Write element.  */
                write_byte_stream(ofptr,  start_offset, cum_length,
                                  indent);
            }
            
        }

    }

    if(wnblocks > 1) {
        --indent;
        end_elm(ofptr, TAG_BSTMSET, indent);
    }
 

}


/*!
  \fn write_map_header(FILE *ofptr, char* filename, int file_size, 
  char* md5_str, int no_full_path)
  \brief  Insert \a filename, \a file_size, and \a md5_str inside  the 
  \ref TAG_FINFO element. If \a no_full_path is 0, insert absolute path name
  from \a filename. Otherwise, insert only base name from \a filename.

  \return FAIL if system error occurs.
  \return SUCEED

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date June 29, 2011
  \note
  added no_full_path argument to insert absolute full path.
  added fileLocation element.

  \date May 1, 2011
  \note
  removed quotes from filename.

  \date April 29, 2011
  \note 
  added XML_FILEINFO.
  added file size and md5 string arguments. 

  \date March 10, 2011
  \note added error checking for fputs() and fprintf().

  \date July 27, 2010
  \note This function replaced "fputs(XML_HEAD, ofptr);" in hmap.c

*/
int write_map_header(FILE *ofptr, char* filename, int file_size, char* md5_str,
                     int no_full_path)
{

    char base_name[PATH_MAX+1]; /* file name only from absolute name */
    char path_name[PATH_MAX+1]; /* path name only from absolute name */

    HDstrcpy(base_name, basename(filename)); 
    if(no_full_path == 0){
        HDstrcpy(path_name, dirname(filename)); 
    }


    if(fputs(XML_HEAD, ofptr) == EOF){
        return FAIL;
    }
    

    if(fputs(XML_HEAD_SCHEMA, ofptr) == EOF){
        return FAIL;
    }

    /* Write file information comment. */
    if(fprintf(ofptr, "%s",  XML_FILEINFO) < 0){
        return FAIL;
    };

    /* Write file content element.  */
    start_elm_0(ofptr, TAG_FINFO, 1);

    /* Write the filename. */
    start_elm_0(ofptr, TAG_FNAME, 2);
    if(fprintf(ofptr, "%s", base_name) < 0){
        return FAIL;
    }
    end_elm_0(ofptr, TAG_FNAME);

    /* Write the location.  */
    if(no_full_path == 0){
        start_elm_0(ofptr, TAG_FLOC, 2);
        start_elm_0(ofptr, TAG_FLOC_T, 3);
        if(fprintf(ofptr, "filepath") < 0){
            return FAIL;
        }
        end_elm_0(ofptr, TAG_FLOC_T);

        start_elm_0(ofptr, TAG_FLOC_V, 3);
        if(fprintf(ofptr, "%s", path_name) < 0){
            return FAIL;
        }
        end_elm_0(ofptr, TAG_FLOC_V);
        end_elm(ofptr, TAG_FLOC, 2);
    }
    
    /* Write the file size. */
    start_elm_0(ofptr, TAG_FSIZE, 2);
    if(fprintf(ofptr, "%d", file_size) < 0){
        return FAIL;
    }
    end_elm_0(ofptr, TAG_FSIZE);

    /* Write the MD5 info. */
    if(md5_str != NULL){
        start_elm_0(ofptr, TAG_MD5, 2);
        if(fprintf(ofptr, "%s", md5_str) < 0){
            return FAIL;
        }
        end_elm_0(ofptr, TAG_MD5);
    }

    end_elm(ofptr, TAG_FINFO, 1);
    return SUCCEED;
}

/*!
  \fn void write_map_datum(FILE *ofptr, int32 dtype, int indent)
  \brief Write datatype information.

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date Feb 17, 2012
  \note changed uchar8 to char8 because schema doesn't have uchar8 type.

  \date June 30, 2011
  \note added error handler for littleEndian.

  \date March 17, 2011
  \note changed the return type.

  \date July 27, 2010
  \note This is dervied from write_map_datatype().

*/

intn
write_map_datum(FILE *ofptr, int32 dtype, int indent)
{


    hdf_ntinfo_t info;          /* defined in hdf.h near line 142. */
    intn result = FAIL;

    start_elm(ofptr, TAG_DATUM, indent); 

    result = Hgetntinfo(dtype, &info);
    if(result == FAIL){
        FAIL_ERROR("Hgetntinfo() failed.");
    }
    else{
        /* Map UCHAR8 to char8 because schema doesn't have uchar8. */
        if(0 == strncmp(info.type_name, "uchar8", 5)){
            fprintf(ofptr, "dataType=\"char8\""); 
        }
        else{
            fprintf(ofptr, "dataType=\"%s\"", 
                    info.type_name);
        }
        /* byteOrder is meaningless if size is one byte. */
        if(strrchr(info.type_name, '8') == NULL){
            fprintf(ofptr, " byteOrder=\"%s\"", 
                    info.byte_order);            
        }

        if(strncmp(info.type_name, "float",5) == 0){
            fprintf(ofptr, " floatingPointFormat=\"IEEE\"");
        }
        /* Throw an error if littleEndian is detected. */
        /* ddChecker can't catch littleEndian on attributes. */
        if(strncmp(info.byte_order, "littleEndian",12) == 0){
                fprintf(ferr, 
                        "DATATYPE:little-endian");
                fprintf(ferr,":type=%s\n", info.type_name);
                ++num_errs;
        }

    }
    fputs("/>", ofptr);
    return SUCCEED;
}

/*!
  \fn write_map_datum_char(FILE *ofptr, int32 dtype, int indent)
  \brief Write non-numeric datatype.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Aug 3, 2010
  \see write_map_datum

*/
void write_map_datum_char(FILE *ofptr, int32 dtype, int indent)
{
    start_elm(ofptr, TAG_DATUM, indent); 
    switch(dtype & 0xff)
        {
        case DFNT_CHAR:
            fprintf(ofptr, "dataType=\"%s\"", XML_DATATYPE_CHAR8);
            break;
        case DFNT_UCHAR:
            fprintf(ofptr, "dataType=\"%s\"", XML_DATATYPE_CHAR8);
            break;
        default:
            break;
        }
    fputs("/>", ofptr);
}

/*!
  \fn write_missing_objects(FILE *ofptr, ref_count_t *obj_missed)
  \brief Write missing objects in \a obj_missed to \a ofptr.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date June 30, 2011
  \note removed the count of objects since tag/ref indicates a unique object.
  The reference alone is not sufficient.

  \date Oct 29, 2010
  \note created.

*/
void write_missing_objects(FILE *ofptr, ref_count_t *obj_missed)
{
    int i = 0;
    if(obj_missed != NULL){
        fprintf(ofptr, 
                "\nObject(s) with the following reference number(s):\n\n");
        for(i = 0; i < obj_missed->size; i++){
            fprintf(ofptr, "    [%d] ref=%ld\n", i+1, 
                    (long) obj_missed->refs[i]);
        }
        fprintf(ofptr, "\ncould not be mapped properly.\n");
    }

}


/*!
  \fn write_ignored_tags(FILE *ofptr, int* list, int count)
  \brief Write \a count ignored tags in \a list to \a ofptr.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date May 10, 2011
  \note created.

*/
void write_ignored_tags(FILE *ofptr, int* list, int count)
{
    int i = 0;
    if(count > 0 && list != NULL){
        fprintf(ofptr, 
                "\n%d object(s) with the following tag(s):\n\n",
                count);
        for(i = 0; i < list[i]; i++){
            fprintf(ofptr, "    [%d] tag=%d\n", i+1, 
                    list[i]);
        }
        fprintf(ofptr, "\ncould not be mapped properly.\n");
    }
}

