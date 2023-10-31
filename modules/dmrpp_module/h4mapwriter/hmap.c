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
  \file hmap.c
  \brief Create a mapping file in XML for a given HDF4 file.

  This file has the main function of the h4mapwriter.

  It also has help message, usage message, and gzip compression functions.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date July 19, 2016
  \note added options for calculating checksum and uuid for chunks.

  \author Ruth Aydt (aydt@hdgroup.org)
  \date July 19, 2012
  \note updates related to palette handling

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date June 30, 2011
  \note added flag for the use of external file to exit without map.

  \date June 29, 2011
  \note added limits.h for PATH_MAX.

  \date March 11, 2011
  \note made num_err global.

  \date October 8, 2010
  \note ported to Solaris.

  \date March 8, 2010
  \note added documentation in Doxygen style.

  \author Binh-Minh Ribler (bmribler@hdfgroup.org)
  \date March 15, 2009
  \note fixed several bugs. See ChangeLog.

  \author Peter Cao (xcao@hdfgroup.org)
  \date August 23, 2007

 */

/* Definitions for gzip compression. */
#define CHUNK 16384      /*!< gzip buffer size  */
#define ORIG_NAME 0x08   /*!< bit 3 set: original file name present */
#define OS_CODE 0x03     /*!< unix */
#define DEF_MEM_LEVEL 8  /*!< default memory level  */

#include "hmap.h"
#include "cdeflate.h"

#include <fts.h>                /* for fts_open() in gzip function */
#include <libgen.h>             /* for basename() in gzip function */
#include <getopt.h>             /* for long option names */
#include <openssl/md5.h>        /* for MD5 checksum */
#include <limits.h>             /* for PATH_MAX definition */

extern int trim;
extern int merge;
extern int uuid;
extern int parse_odl;
extern int optind;              /*!<  Option index */
extern char* optarg;            /*!<  Option argument value */
extern char* optfile;           /*!<  Input file name pointer */

FILE* flog = NULL; /*!< Pointer to error log file. By default, it's stderr. */
FILE* ferr = NULL; /*!< Pointer to error log file. By default, it's stderr. */
int num_errs = 0;  /*!< Number of errors for unmapped objects. */
int has_extf = 0;  /*!< Flag for the use of external file. */

unsigned int ID_FA = 0;  /*!< Unique file attribute ID */

/*!
 \fn void help(void)

 \brief  Print a long summary of usage and features.

 \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

 \date July 27, 2016
 \note added -o option description.

 \date July 20, 2016
 \note added -u option description.

 \date June 29, 2011
 \note updated -f option description.

 \date June 16, 2011
 \note added -f and -V options.

 \date May 23, 2011
 \note corrected the description for '-c' option.

 \date April 28, 2011
 \note added -n and -s option message for MD5 checksum.

 \date March 10, 2011
 \note changed message for -i option.

 */
void
help(void)
{
    (void) fflush(stdout);
    (void) fprintf(stdout, "NAME:\n\t");
    (void) fprintf(stdout, "h4mapwriter -- HDF4 mapping file writer\n\n");

    (void) fprintf(stdout,
                   "DESCRIPTION:\n\t");
    (void) fprintf(stdout,
                "h4mapwriter is a utility that creates an XML mapping file for an HDF4\n\t");
    (void) fprintf(stdout,
                   "file. A mapping file is an XML document that describes the file \n\t");
    (void) fprintf(stdout,
                   "structure, the metadatas, and offsets/lengths of raw data in an \n\tHDF4 file.\n\t");
    (void) fprintf(stdout,
                   "\n\tThe XML map file can be used to retrieve data from an HDF4 file without\n\t");
    (void) fprintf(stdout,
                   "the HDF4 library.\n\n");

    (void) fprintf(stdout, "SYNOPSIS:");
    (void) fprintf(stdout, "\n\th4mapwriter -h[elp], OR");
    (void) fprintf(stdout, 
                   "\n\th4mapwriter [-cefmntuV] [-i TAGS] [-s checksum] [-z[1-9]] [-l LOG] HDF4 [XML]");

    (void) fprintf(stdout, "\n\n\t-h[elp], --help:");
    (void) fprintf(stdout, "\n\t\tPrint this summary of usage, and exit.");
    (void) fprintf(stdout, "\n");

    (void) fprintf(stdout, "\n\n\t-c, --continue-with-unmapped-objects:");
    (void) fprintf(stdout, "\n\t\tContinue to generate an incomplete map by ignoring\n\t\tall unmapped objects.");
    (void) fprintf(stdout, "\n");

    (void) fprintf(stdout, "\n\n\t-e, --put-file-attributes-at-the-end:");
    (void) fprintf(stdout, "\n\t\tPut file attributes at the end.");
    (void) fprintf(stdout, "\n");

    (void) fprintf(stdout, "\n\n\t-f, --filename-only:");
    (void) fprintf(stdout, "\n\t\tInclude only the filename, not the complete directory\n\t\tpath, in the map.");
    (void) fprintf(stdout, "\n");

    (void) fprintf(stdout, "\n\n\t-i TAG-LIST, --ignore-tags=TAG-LIST:");
    (void) fprintf(stdout, "\n\t\tIgnore listed tags when checking all data are mapped.\n\t\tTAG-LIST must be in csv format.");
    (void) fprintf(stdout, "\n");

    (void) fprintf(stdout, "\n\n\t-l LOGFILE, --error-log-file=LOGFILE:");
    (void) fprintf(stdout, "\n\t\tRedirect all stderr messages to the LOGFILE.");
    (void) fprintf(stdout, "\n");

    (void) fprintf(stdout, "\n\n\t-m, --merge-sds-file-attributes:");
    (void) fprintf(stdout, "\n\t\tMerge SDS file attributes.");
    (void) fprintf(stdout, "\n");

    (void) fprintf(stdout, "\n\n\t-n, --no-checksum:");
    (void) fprintf(stdout, "\n\t\tDo not compute MD5 checksum and do not put any checksum\n\t\tinformation.");
    (void) fprintf(stdout, "\n");

    (void) fprintf(stdout, "\n\n\t-o, --odl-parser:");
    (void) fprintf(stdout, "\n\t\tParse ODL and put ODL contents as groups and attributes.");
    (void) fprintf(stdout, "\n");
    
    (void) fprintf(stdout, "\n\n\t-s CHECKSUM, --set-checksum=CHECKSUM:");
    (void) fprintf(stdout, "\n\t\tSet the checksum value to be the CHECKSUM. The writer will\n\t\tinsert the CHECKSUM instead of computing MD5.");
    (void) fprintf(stdout, "\n");

    (void) fprintf(stdout, "\n\n\t-t, --trim-nulls-from-end:");
    (void) fprintf(stdout, "\n\t\tTrim NULL characters from the end of the merged SDS file\n\t\tattributes. Use it with -m option to make it effective.");
    (void) fprintf(stdout, "\n");

    (void) fprintf(stdout, "\n\n\t-u, --uuid-and-checksum-for-bytestream:");
    (void) fprintf(stdout, "\n\t\tAdd uuid and md5 checksum information for bytestream.");
    (void) fprintf(stdout, "\n");
    
    (void) fprintf(stdout, "\n\n\t-V, --version:");
    (void) fprintf(stdout, "\n\t\tPrint version information.");
    (void) fprintf(stdout, "\n");

    (void) fprintf(stdout, "\n\n\t-z[1-9], --compress-output-file[=1-9]:");
    (void) fprintf(stdout, "\n\t\tCompress the XML file with gzip level 1-9.");
    (void) fprintf(stdout, "\n");

    (void) fprintf(stdout, "\n\n\tHDF4:");
    (void) fprintf(stdout, "\n\t\tName of the input HDF4 file");
    (void) fprintf(stdout, "\n");

    (void) fprintf(stdout, "\n\n\tXML:");
    (void) fprintf(stdout, "\n\t\tName of the output XML file. If it is not specified,\n\t\tthe MAP file will be displayed in UNIX stdout.\n\n");
    (void) fprintf(stdout, "\n");
    exit(1);
}

/*!
 \fn void usage(void)
 \brief  Print a summary of command line usage.

 \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

 \date July 27, 2016
 \note added -o option for odl parser.

 \date July 20, 2016
 \note added -u option.

 \date June 16, 2011
 \note added -f and -V options.

 \date April 28, 2011
 \note added -n and -s option for MD5 checksum.

 \date March 4, 2011
 \note updated with more options.

 */
void
usage(void)
{
    (void) fprintf(stderr, "usage:h4mapwriter -h[elp]\n");
    (void) fprintf(stderr, "      h4mapwriter [-cefmnotuV] [-i TAGS] [-s checksum]  [-z[1-9]] [-l LOG] HDF4 [XML]\n");
    exit(1);
}


/*!
 \fn int write_gzip_header(FILE* out, char *name, u_int32_t mtime, int bits)
 \brief  Write gzip file header information.

 \note This function is adapted from OpenBSD/usr.bin/compress source code.
 */
int 
write_gzip_header(FILE* out, char *name, u_int32_t mtime, int bits)
{
    static const u_char gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */
    u_char buf[10];
    int i=0;

    buf[0] = gz_magic[0];
    buf[1] = gz_magic[1];
    buf[2] = Z_DEFLATED;
    buf[3] = name ? ORIG_NAME : 0;
    buf[4] = mtime & 0xff;
    buf[5] = (mtime >> 8) & 0xff;
    buf[6] = (mtime >> 16) & 0xff;
    buf[7] = (mtime >> 24) & 0xff;
    buf[8] = bits == 1 ? 4 : bits == 9 ? 2 : 0;	/* xflags */
    buf[9] = OS_CODE;

    /* Write the gzip header information. */
    for(i=0; i < 10; i++){
        fprintf(out, "%c", buf[i]);
    }
    /* Write the original filename. */
    if(buf[3] != 0){
        fprintf(out, "%s", basename(name));
        fprintf(out, "%c", 0);
    }
    return (0);
}

/*!
 \fn int write_gzip(char* infile, char* outfile, int level)
 \brief  Compress the \a infile HDF4 map file using gzip \a level compression.

 The use of fts_open() makes the writer less portable since Solaris doesn't 
 have it.

 \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
 \date Oct 14, 2013
 \note added error handling for fwrite() functions.

 \note This function is adapted from OpenBSD/usr.bin/compress source code.
 */
int 
write_gzip(char* infile, char* outfile, int level)
{
    FILE* source = NULL;
    FILE* dest = NULL; 

    FTS *ftsp = NULL;

    FTSENT *entry = NULL;

    char *paths[2];

    int done = 0;
    int err = 0;
    int ret = 0;

    size_t nr = 0;
    size_t len = 0;

    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    u_int32_t z_crc = crc32(0L, Z_NULL, 0);
    u_int32_t mtime = 0;

    z_stream strm;

    paths[0] = infile;
    paths[1] = 0;

    /* Get mtime of the map file. */
    if ((ftsp = fts_open(paths, FTS_PHYSICAL|FTS_NOCHDIR, 0)) == NULL){
        fprintf(flog, "fts_open() failed on %s\n", infile);
        return FAIL;
    }
    if((entry = fts_read(ftsp)) == NULL){
        fprintf(flog, "fts_read() failed on %s\n", infile);
        return FAIL;
    }
    mtime = entry->fts_statp->st_mtime;
    source = fopen(infile, "r");
    if(source == NULL){
        fprintf(flog, "fopen() failed on %s\n", infile);        
        return FAIL;
    }

    dest = fopen(outfile, "w");
    if(dest == NULL){
        fprintf(flog, "fopen() failed on %s\n", outfile);        
        return FAIL;
    }
    write_gzip_header(dest, infile, mtime, level);

    /* Allocate deflate state. */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = Z_NULL;
    strm.next_out = Z_NULL;
    strm.avail_in = strm.avail_out = 0;

    ret = deflateInit2(&strm, level, Z_DEFLATED,
                       -MAX_WBITS, DEF_MEM_LEVEL, 0);
    if (ret != Z_OK)
        return FAIL;

    strm.next_out = out;
    strm.avail_out = CHUNK;

    /* Deflate the input. */
    while((nr = fread(in, 1, CHUNK, source)) > 0){
        strm.next_in =  in;
	strm.avail_in = nr;

	while (strm.avail_in != 0) {
            if (strm.avail_out == 0) {
                if (fwrite(out, 1, CHUNK, dest) != CHUNK)
                    break;
                strm.next_out = out;
                strm.avail_out = CHUNK;
            }
            if (deflate(&strm, Z_NO_FLUSH) != Z_OK)
                break;
	}
	z_crc = crc32(z_crc, in, nr);
    }

    /* Flush the rest. */
    for (;;) {
        len = CHUNK - strm.avail_out;

        if (len != 0) {
            if(fwrite(out, 1, len, dest) != len)
                return FAIL;
            strm.next_out = out;
            strm.avail_out = CHUNK;
        }
        if (done)
            break;
        if ((err = deflate(&strm, Z_FINISH)) != Z_OK &&
            err != Z_STREAM_END)
            return FAIL;

        /* deflate has finished flushing only when it hasn't
         * used up all the available space in the output buffer
         */
        done = (strm.avail_out != 0 || err == Z_STREAM_END);
    }

    /* Write CRC32. */
    if(fwrite(&z_crc, sizeof(z_crc), 1, dest) <= 0){
        fprintf(flog, "fwrite() failed on writing CRC\n");
        return FAIL;        
    };

    /* Write the total byte. */
    if(fwrite(&strm.total_in, sizeof(strm.total_in), 1, dest) <= 0){
        fprintf(flog, "fwrite() failed on writing gzip stream\n");
        return FAIL;                
    };

    /* Clean up. */
    (void)deflateEnd(&strm);
    fclose(dest);
    fclose(source);
    /* Remove the temporary map file. */
    unlink(infile);
    return SUCCEED;
}


/*! 
  \fn int main(int argc, char **argv)
  \brief Read HDF4 file and write a map file.

  The main function parses options and processes input HDF4 file accordingly.

  -# Parse options and set up file pointers for errors and outputs.
  -# Initialize variables and HDF4 interfaces.
  -# Generate map file or exit when HDF4 API calls or system calls fail.
  -# Report any errors related missing objects or append them at the end
  of map file.

  HDF4 failures start with HDF4 function name. 
  System error messages start with "Failed to <action>." 
  The rest are option errors.

  \return 0 if no errors 
  \return positive number otherwise

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Jul 27, 2016
  \note	added odl parser option.

  \date Jul 20, 2016
  \note	added uuid/md5 option for byte streams.

  \date Oct 11, 2013
  \note	added tag/ref information for error message.
  \note	improved error message format.

  \date Oct 15, 2012
  \note	updated version date.

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 19, 2012
  \note replaced pal_visited with palIP8_visited and palLUT_visited
  \note removed call to free_pr2id_list
  \note added call to free_pal_arrays
  \note moved variable declarations from inline to quiet compiler warnings

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date June 29, 2011
  \note	changed no-full-path to filename-only.
  replaced fixed 256 to PATH_MAX+1.
  \note added real_path() to supply absolute file name to write_map_header().
  \note added Hishdf() call to check HDF4 file.

  \date June 16, 2011
  \note	added -f and -V option.
  \note added '\n' for open file error message.

  \date May 23, 2011
  \note	printed the log message for -c option.

  \date May 10, 2011
  \note	added warning message at the end of the map when -i option is active.
  \note printed the log message for -i option.

  \date May 3, 2011
  \note	replaced detailed warning message to simple one.

  \date May 2, 2011
  \note	applied HDmemset() on MD5 string buufer.

  \date April 29, 2011
  \note	added -n and -s option message for MD5 checksum.

  \date April 25, 2011
  \note	
  placed palette processing before group processing to map raster under group 
  correctly.

  \date March 17, 2011
  \note 
  made gzip work if -c is active.
  added error handler for ref_count calls.
  relaxed ignore tag list range.
  changed error message for Hopen() call failure.
  added exit_errs to distinguish system/HDF4 call errors from mapping errors.

  \date March 15, 2011
  \note 
  changed return type and error message for write_map_lone_sds().
  cleaned up error handlers to make them consistent.
  forced to run ddChecker module always.

  \date March 14, 2011
  \note 
  added more error handlings.


  \date March 11, 2011
  \note 
  moved output file opening function toward the end.


  \date March 10, 2011
  \note 
  cleaned up error handling. 

*/
int
main(int argc, char **argv)
{

    FILE *ofptr = NULL;         /* output file pointer */
    FILE *tmpfp = NULL;         /* temp file pointer */

    MD5_CTX cs;                 /* for MD5 checksum */

    char c;                     /* for copying temp files into final ones */
    char *tok = NULL;           /* ignore option string tokenizer */
    static char* last;          /* for strtok_r().  */
    char *logfile = NULL;    
    char *ignore_list = NULL;    
    char *checksum_str = NULL;  /* for user supplied checksum */

    char file_name[PATH_MAX+1]; /* file name only for -f option. */
    char infile[PATH_MAX+1];
    char outfile[PATH_MAX+1];
    char buf[512];              /* for MD5 checksum computation */
    unsigned char out[MD5_DIGEST_LENGTH];


    char tmpname[] = "/tmp/h4map.XXXXXX"; /* temp file for output */
    char tmperrname[] = "/tmp/h4map.err.XXXXXX"; /* temp file for errors */
    char gzoutfile[PATH_MAX+1];        /* gzip output file name */

    int ch = 0;                 /* for get_option_long() */
    int checksum = 1;           /* checksum option */
    int compress = 0;           /* Compress at the end? */
    int ddChecker = 1;          /* Run ddChecker module? */
    int fd = -1;                /* file descriptor for temp. output file. */
    int fd_err = -1;            /* file descriptor for temp. error file. */
    int file_size = 0;          /* file size informatin. */
    int gzip_level = 6;         /* default gzip compression level */
    int ignore_cnt = 0;         /* number of tags to ignore */
    /* ignore tags list. See hdf/src/htags.h in hdf4 source distribution. 
       There are at most 128 tags in HDF4.  */
    int ignore_tags[128];            
    int j = 0;
    int n = 0;
    int no_full_path = 0;       /* no full path at the header */
    int exit_errs = 0;          /* errors related to system / HDF4 call.  */

    int option_index = 0;


    int32 file_id = -1;
    int32 end = 0;              /* Put file attributes at the end? */
    int32 *vg_refs = NULL;
    int32 nvg = 0;
    int32 num_datasets = 0;
    int32 nattrs = 0;
    int32 sd_id = FAIL;
    int32 gr_id = FAIL;
    int32 an_id = FAIL;

    intn status = FAIL;

    ref_count_t sd_visited, vs_visited, vg_visited, ris_visited, palIP8_visited, palLUT_visited;
    ref_count_t obj_missed;

    ssize_t bytes;              /* for MD5 checksum */

    struct stat st;             /* for file size */

    static struct option long_options[] = 
        {
            /* These options set a flag. */
            {"disable-check-for-missing-objects", no_argument, 0, 'c'},
            {"put-file-attributes-at-the-end", no_argument, 0, 'e'},
            {"help", no_argument, 0, 'h'},
            {"trim-nulls-from-end", no_argument, 0, 't'},
            {"merge-sds-file-attributes", no_argument, 0, 'm'},
            {"compress-output-file", optional_argument, 0, 'z'},
            {"error-log-file", required_argument, 0, 'l'},
            {"ignore-tags", required_argument, 0, 'i'},
            {"no-checksum", no_argument, 0, 'n'},
            {"set-checksum", required_argument, 0, 's'},
            {"filename-only", no_argument, 0, 'f'},
            {"uuid-md5", no_argument, 0, 'u'},
            {"odl-parser", no_argument, 0, 'o'},
            {"version", no_argument, 0, 'V'},
            {0,0,0,0}

        };

    char* path_ptr = NULL;
    int fd_cs = 0;
    FILE* filep = NULL;

    
    /* By default, error goes to stderr. */
    flog = stderr;

    while((ch = getopt_long(argc, argv, "cefhtumnoVl:i:s:z::", 
                            long_options, &option_index)) != -1)
        {
            switch(ch) {
            case 'c':
                ddChecker = 0;
                break;                
            case 'e':
                end = 1;
                break;
            case 'h':
                help();
                break;
            case 'l':
                logfile = optarg;
                break;
            case 'n':
                checksum = 0;
                break;
            case 't':
                trim = 1;
                break;
            case 'm':
                merge = 1;
                break;
            case 'z':
                compress = 1;
                if(optarg){
                    gzip_level = atoi(optarg);
                    if(gzip_level < 1 || gzip_level > 9){
                        fprintf(stderr, 
                                "Gzip level is out of range(1~9):%d\n", 
                                gzip_level);
                        exit(1);
                    }
                }
                break;
            case 'i':
                ignore_list = optarg;
                break;
            case 's':
                checksum = 0;
                checksum_str = optarg;
                break;
            case 'f':
                no_full_path = 1;
                break;
            case 'u':
                uuid = 1;
                break;
            case 'o':
                parse_odl = 1;
                break;                
            case 'V':
                (void) fprintf(stderr, "h4mapwriter version ");
                (void) fprintf(stderr, "%s", VERSION);
                (void) fprintf(stderr, " 20161201\n");
                exit(1);
                break;
            default:
                usage();
            }
        } /* while(getopt_long) */
    argc -= optind;
    argv += optind;    

    /* Validate the number of command line arguments. */
    if (argc < 1) {
        usage();
    }

    /* Validate compression option. */
    if (argc < 2 && compress == 1) {
        fprintf(stderr, "No output filename is specified with -z option.\n");
        exit(1);
    } 

    /* Get the list of tags to be ignored. */
    for(j=0; j < 128; j++){
        ignore_tags[j] = -1; /* Initialize. */
    }

    if(ignore_list != NULL){

        for(tok = strtok_r(ignore_list, ",", &last); tok != 0;
            tok = strtok_r(NULL, ",", &last)){
            int tag = atoi(tok);
        
            if(ignore_cnt > 127){
                fprintf(stderr, 
                        "Too many (> 128) tags:%d\n", ignore_cnt);
                exit(1);            
            }

            if(tag < 0) { /* TODO: Narrow it to valid tags. */
                fprintf(stderr, "Invalid tag number:%d\n", tag);
                exit(1);
            }
            else{
                ignore_tags[ignore_cnt] = tag;
                ++ignore_cnt;
            }

        }
    }

    /* Check for the validity of MD5 input. */
    if(checksum_str != NULL){
        if(strlen(checksum_str) != (MD5_DIGEST_LENGTH * 2)){
            fprintf(stderr, "Invalid MD5 checksum:length=%d\n", 
                    (int)strlen(checksum_str));
            exit(1);            
        }
    }

    /* Create a temporary output file. */
    if((fd = mkstemp(tmpname)) < 0){
        fprintf(stderr, "Failed to create temporary output file: %s.\n", 
                tmpname); 
        exit(1);
    }
    else {
        tmpfp = fdopen(fd, "w+");
        if(tmpfp == NULL){
            if(fd != -1){
                close(fd);
            }
            fprintf(stderr, "Failed to open temporary output file:fd=%d\n", 
                    fd);
            exit(1);
        }
                
    }

    /* Create a temporary error file. */
    if((fd_err = mkstemp(tmperrname)) < 0){
        fprintf(stderr, "Failed to create temporary error file: %s.\n", 
                tmperrname); 
        ++exit_errs;
        goto done;
    }
    else {
        ferr = fdopen(fd_err, "w+");
        if(ferr == NULL){
            if(fd_err != -1){
                close(fd_err);
            }
            fprintf(stderr, "Failed to open temporary error file:fd=%d\n", 
                    fd_err);
            ++exit_errs;
            goto done;
        }
                
    }

    /* Open log file. */
    if((logfile != NULL) && !(flog = fopen(logfile, "a")))
        {
            fprintf(stderr, "Failed to open log file: %s\n", logfile);
            ++exit_errs;
            goto done;
        }

    
    /* Initialize each list of already visited objects for SD, Vdata,
       Vgroup, and RIS interfaces. */
    if(ref_count_init(&sd_visited) == FAIL){
        ++exit_errs;
        fprintf(flog, "ref_count_init() failed.\n");
        goto done;
    }
    if(ref_count_init(&vs_visited) == FAIL){
        ++exit_errs;
        fprintf(flog, "ref_count_init() failed.\n");
        goto done;        
    };
    if(ref_count_init(&vg_visited) == FAIL){
        ++exit_errs;
        fprintf(flog, "ref_count_init() failed.\n");
        goto done;        
    };
    if(ref_count_init(&ris_visited) == FAIL){
        ++exit_errs;
        fprintf(flog, "ref_count_init() failed.\n");
        goto done;        
    };
    if(ref_count_init(&palIP8_visited) == FAIL){
        ++exit_errs;
        fprintf(flog, "ref_count_init() failed.\n");
        goto done;        
    };
    if(ref_count_init(&palLUT_visited) == FAIL){
        ++exit_errs;
        fprintf(flog, "ref_count_init() failed.\n");
        goto done;        
    };
    if(ref_count_init(&obj_missed) == FAIL){
        ++exit_errs;
        fprintf(flog, "ref_count_init() failed.\n");
        goto done;        
    };


    /* Get input file name from command line. */
    HDstrcpy(infile, argv[0]); 
    optfile = argv[0];
    
    /* Get absolute path name for the file. */
    path_ptr = realpath(argv[0], file_name);
    if(path_ptr == NULL){
        ++exit_errs;
        fprintf(flog, "%s file not found.\n", argv[0]);
        goto done;
    }

    /* Check if it is HDF file. */
    if(Hishdf(infile) == FALSE){
        ++exit_errs;
        fprintf(flog, "%s file is not an HDF4 file.\n", argv[0]);
        goto done;        
    }

    /* Open input file first. */
    file_id = Hopen(infile, DFACC_READ, DEF_NDDS);
    /* Although Hopen() is HDF4 call,  the error checking and message 
       is similar to system call error because it's opening input file. */
    if(file_id == FAIL){
        fprintf(flog, "Unable to open input file: %s\n", infile);
        ++exit_errs;
        goto done;
    }

    if(stat(infile, &st) != 0){
        fprintf(flog, "Unable to stat input file: %s\n", infile);
        ++exit_errs;
        goto done;
    }
    else{
        file_size = st.st_size;
    }


    if(checksum == 1){
        MD5_Init(&cs);
        fd_cs = open(infile, O_RDONLY, 0);
        bytes=read(fd_cs, buf, 512);
        while(bytes > 0)
            {
                MD5_Update(&cs, buf, bytes);
                bytes=read(fd_cs, buf, 512);
            }

        close(fd_cs);
        MD5_Final(out, &cs);
        
        /* Allocate memory for checksum_str. */
        checksum_str = (char*) HDmalloc((MD5_DIGEST_LENGTH * 2)
                                           * sizeof(char) + 1);
        if(checksum_str == NULL){
            fprintf(flog, "HDmalloc() failed: Out of Memory\n");
            ++exit_errs;
            goto done;
        }
        HDmemset(checksum_str, 0, 2*MD5_DIGEST_LENGTH + 1);

        for(n=0; n < MD5_DIGEST_LENGTH; n++){
            char tmp_str[3];
            HDmemset(tmp_str, 0, 3);
            snprintf(tmp_str, 3, "%02x", out[n]);
            tmp_str[2] = '\0';
            strncat(checksum_str, tmp_str, 2);
        }
        checksum_str[MD5_DIGEST_LENGTH * 2] = '\0';
    }

    /* Initialize V, SD, AN, and GR interfaces. */
    status = Vstart(file_id);
    CHECK_ERROR(status, FAIL, "Vstart() failed.");

    sd_id = SDstart(infile, DFACC_RDONLY);
    CHECK_ERROR(sd_id, FAIL, "SDstart() failed.");

    gr_id = GRstart(file_id);
    CHECK_ERROR(gr_id, FAIL, "GRstart() failed.");

    an_id = ANstart(file_id);
    CHECK_ERROR(an_id, FAIL, "ANstart() failed.");

    /* Put XML headers. */
    status = write_map_header(tmpfp, file_name, file_size, checksum_str,
                              no_full_path);

    if(status == FAIL){
        fprintf(flog, "write_map_header() failed.");    
        fprintf(flog, ":filename=%s\n", file_name);    
        ++exit_errs;
        goto done;
    }
    else{
        if(checksum == 1 && checksum_str != NULL){
            HDfree(checksum_str);
        }
    }

    /* Put map-reading instruction at the beginning. */
    fprintf(tmpfp, "%s",  XML_INSTRUCTION);

    /* Put file content element.  */
    start_elm_0(tmpfp, TAG_FCNT, 1);

    /* Put top-level file attributes. */
    if(end == 0) {
        /* Get SDS global file attributes. */
        status = SDfileinfo(sd_id, &num_datasets, &nattrs);
        CHECK_ERROR(status, FAIL, "SDfileinfo() failed.");
        /* Write file attributes using SDS APIs. */
        if (nattrs > 0 ) {
            status = write_file_attrs(tmpfp, sd_id, nattrs, 2);
            if(status == FAIL){
                fprintf(flog, "write_file_attrs() failed."); 
                fprintf(flog, ":sd_id=%ld, nattrs=%ld\n", 
                        (long)sd_id, (long)nattrs); 
                ++exit_errs;
                goto done;
            }
            status = write_mattrs(tmpfp, 2);
            if(status == FAIL){
                fprintf(flog, "write_mattrs() failed.");             
                ++exit_errs;
                goto done;
            }
        }

        /* Write file attributes using AN APIs. */
        status = write_file_attrs_an(tmpfp, an_id, 2);
        if(status == FAIL){
            fprintf(flog, "write_file_attrs_an() failed.");
            fprintf(flog, ":an_id=%ld\n", (long)an_id);
            ++exit_errs;
            goto done;
        }

        /* Write file attributes using GR APIs. */
        status = write_file_attrs_gr(tmpfp, gr_id, 2);
        if(status == FAIL){
            fprintf(flog, "write_file_attrs_gr() failed.");
            fprintf(flog, ":gr_id=%ld\n", (long)gr_id);
            ++exit_errs;
            goto done;
        }

    }

    /* Write palettes and collect palette's ref and ID information. */
    status = write_map_pals(tmpfp, infile, file_id, gr_id, an_id, &palIP8_visited, &palLUT_visited);
    if(status == FAIL){
        fprintf(flog, "write_map_pals() failed.");
        ++exit_errs;
        goto done;                
    }

    /* Get the top level vgroups. */
    nvg = Vlone(file_id, NULL, 0);
    CHECK_ERROR(nvg, FAIL, "Vlone failed. on first call");

    if (nvg > 0) {
        int i=0;
        char path[MAX_PATH_LEN]; /* path from the root */
        char name[H4_MAX_NC_NAME]; /* vgroup name */
        char cname[H4_MAX_NC_NAME]; /* vgroup class name */

        vg_refs = (int32 *)HDmalloc(nvg*sizeof(int32));

        if(vg_refs == NULL){
            fprintf(flog, "HDmalloc() failed: Out of Memory");
            ++exit_errs;
            goto done;
        }

        nvg = Vlone(file_id, vg_refs, nvg); 

        CHECK_ERROR(nvg, FAIL, "Vlone() failed.");

        for (i=0; i < nvg; i++) {

            int32 vgid = -1;
            int32 ref = -1;
            int32 tag = -1;

            char* name_xml = NULL;
            char* class_xml = NULL;

            HDmemset(path, 0, MAX_PATH_LEN);
            HDmemset(name, 0, H4_MAX_NC_NAME);
            HDmemset(cname, 0, H4_MAX_NC_NAME);

            /* If unable to attach to a vgroup, 
               just move to the next(?)-BMR */
            /* Elena said we should give info about it. */
            vgid = Vattach(file_id, vg_refs[i], "r");

            if (vgid == FAIL)
                {
                    fprintf(flog, 
                            "Vattach() failed: ref=%d\n",
                            (int)vg_refs[i]);
                    continue;
                }

            /* Remeber the ref number.  */
            ref = VQueryref(vgid);
            tag = VQuerytag(vgid);

            if(ref_count(&vg_visited, ref) == FAIL){
                ++exit_errs;
                fprintf(flog, "ref_count() failed.");
                fprintf(flog, ":ref=%ld\n", (long)ref);
                goto done;        
            };

            status = Vgetclass(vgid, cname);
            CHECK_ERROR(status, FAIL, "Vgetclass() failed.");

            if(Visinternal(cname)){
                continue;
            }

            HDmemset(name, 0, H4_MAX_NC_NAME);
            status = Vgetname(vgid, name);
            CHECK_ERROR(status, FAIL, "Vgetname() failed.");

            name_xml =  get_string_xml(name, strlen(name));
            class_xml = get_string_xml(cname, strlen(cname));
            status = write_group(tmpfp, name_xml,  "/", class_xml, 2);
            if(status == FAIL){
                fprintf(flog, "write_group() failed:name=%s, class=%s.\n",
                        name_xml, class_xml);
                ++exit_errs;
                goto done;
            }

            /* Write Vgroup annotations if they exist. */
            status = write_group_attrs_an(tmpfp, an_id, tag, ref, 3);
            if(status == FAIL){
                fprintf(flog, "write_group_attrs_an() failed:");
                fprintf(flog, "an_id=%ld, tag=%ld, ref=%ld.\n",
                        (long)an_id, (long)tag, (long)ref);
                ++exit_errs;
                goto done;
            }
            

            /* Write Vgroup attribute. */
            nattrs = (int32)Vnattrs2(vgid);
            CHECK_ERROR(nattrs, FAIL, "Vnattrs2() failed.");             
            if (nattrs > 0 ) {
                status = write_group_attrs(tmpfp, vgid, nattrs, 3);
                if(status == FAIL){
                    fprintf(flog, "write_group_attrs() failed:");
                    fprintf(flog, "vgid=%ld, nattrs=%ld.\n",
                            (long)vgid, (long)nattrs);
                    ++exit_errs;
                    goto done;
                }

            }

            path[0] = '/';
            HDstrcat(path, name_xml);
            
            status= depth_first(file_id, vgid, sd_id, gr_id, an_id, 
                                path, tmpfp, 3,
                                &sd_visited, &vs_visited, 
                                &vg_visited, &ris_visited);
            if(status == FAIL){
                fprintf(flog, "depth_first() failed:");
                fprintf(flog, "path=%s\n",path);
                ++exit_errs;
                goto done;                
            }

            end_elm(tmpfp, TAG_GRP, 2);

            if(name_xml != NULL) {
                HDfree(name_xml);
            }

            if(class_xml != NULL) {
                HDfree(class_xml);
            }

            status = Vdetach(vgid);
            CHECK_ERROR(status, FAIL, "Vdetach() failed.");

        }
    }

    
    /* The order of these calls are important. Call RIS  before vdata
       because RIS uses vdata to group its information. */
    status = write_map_lone_ris(tmpfp, file_id, gr_id, &ris_visited);
    if(status == FAIL){
        fprintf(flog, "write_map_lone_ris() failed");
        fprintf(flog, ":gr_id=%ld.\n", (long)gr_id);
        ++exit_errs;
        goto done;                
    }


    status = write_map_lone_sds(tmpfp, infile, sd_id, an_id,  &sd_visited);
    if(status == FAIL){
        fprintf(flog, "write_map_lone_sds() failed");
        fprintf(flog, ":sd_id=%ld.\n", (long)sd_id);
        ++exit_errs;
        goto done;                
    }

    status = write_map_lone_vdata(tmpfp, file_id, &vs_visited);
    if(status == FAIL){
        fprintf(flog, "write_map_lone_vdata() failed.\n");
        ++exit_errs;
        goto done;                
    }

    /* Write dimensions at the end since they involve reference IDs. */
    status = write_map_dimensions(tmpfp);
    if(status == FAIL){
        fprintf(flog, "write_map_dimensions() failed.\n");
        ++exit_errs;
        goto done;                
    }

    status = write_map_dimension_no_data(tmpfp);
    if(status == FAIL){
        fprintf(flog, "write_map_dimension_no_data() failed.\n");
        ++exit_errs;
        goto done;                
    }

    /* Write the non-rooted Vgroups and objects under the non-rooted vgroups.*/
    status=
        write_vgroup_noroot(tmpfp, file_id, sd_id, gr_id, an_id, 2, 
                            &sd_visited, &vs_visited, 
                            &vg_visited, &ris_visited); 
    if(status == FAIL){
        fprintf(flog, "write_vgroup_noroot() failed.\n");
        ++exit_errs;
        goto done;                
    }


    if(end == 1){
        /* Get SDS global file attributes. */
        SDfileinfo(sd_id, &num_datasets, &nattrs);
        CHECK_ERROR(status, FAIL, "SDfileinfo() failed.");
        if (nattrs > 0 ) {
            status = write_file_attrs(tmpfp, sd_id, nattrs, 2);
            if(status == FAIL){
                fprintf(flog, "write_file_attrs() failed"); 
                fprintf(flog, ":sd_id=%ld, nattrs=%ld\n.", 
                        (long)sd_id, (long)nattrs); 
                ++exit_errs;
                goto done;
            }
            status = write_mattrs(tmpfp, 2);
            if(status == FAIL){
                fprintf(flog, "write_mattrs() failed.");             
                ++exit_errs;
                goto done;
            }
        }

        /* Write file attributes using AN APIs. */
        status = write_file_attrs_an(tmpfp, an_id, 2);
        if(status == FAIL){
            fprintf(flog, "write_file_attrs_an() failed");
            fprintf(flog, ":an_id=%ld\n.", (long)an_id);
            ++exit_errs;
            goto done;
        }

        /* Write file attributes using GR APIs. */
        status = write_file_attrs_gr(tmpfp, gr_id, 2);
        if(status == FAIL){
            fprintf(flog, "write_file_attrs_gr() failed");
            fprintf(flog, ":gr_id=%ld.\n", (long)gr_id);
            ++exit_errs;
            goto done;
        }

    }

    /* Read all refs to find unmapped objects. */
    filep = fopen(infile, "r" );        
    if(filep == NULL){
        fprintf(stderr, "Failed to open: %s\n", infile);
        ++exit_errs;
        goto done;
    }
    else {
        if(read_all_refs(file_id, filep, &obj_missed, 
                         &sd_visited, &vs_visited,
                         &vg_visited, &ris_visited, 
                         &palIP8_visited, &palLUT_visited,
                         ignore_tags, ignore_cnt) > 0) {
            write_missing_objects(ferr, &obj_missed);
        }
        fclose(filep);
    }



    /* Insert the warning message for unmapped objects. */
    if(num_errs > 0 || ignore_cnt > 0){
        fprintf(tmpfp, "\n\n<!--\n\n");
        fputs(XML_WARN, tmpfp);
        fprintf(tmpfp, "\n\n-->\n");
        rewind(ferr);
        while((c = fgetc(ferr)) != EOF){
            fprintf(flog, "%c", c);               
        }
        if(ignore_cnt > 0){
            write_ignored_tags(flog, ignore_tags, ignore_cnt);
        }
    }
    end_elm(tmpfp, TAG_FCNT, 1);

    /* Put the file tag at the end. */
    fputs(XML_FOOT, tmpfp);


    if(fclose(tmpfp) !=0 ) {
        fprintf(stderr, "Failed to close temporary output file: %s\n",
                tmpname);
        ++exit_errs;
        goto done;
    }
    else {
        tmpfp = NULL;
    }

    /* By pass generating map if external file is used. */
    if(has_extf  == 1){
        ddChecker = 1;
    }

    /* Print the temporary output into either stdout or file. */
    if(num_errs == 0 || ddChecker == 0) {

       FILE* fp = fopen(tmpname, "r");        
       if(fp == NULL){
           fprintf(stderr, "Failed to open: %s\n", tmpname);
           goto done;
       }
       /* Get the output file name from user  or use stdout. */
       if (argc > 1)
           {
               HDstrcpy(outfile, argv[1]);
               /* Open output file for writing. */
               ofptr = fopen(outfile, "w");
               if (ofptr == NULL)
                   {
                       fprintf(stderr, "Failed to open output file: %s\n",
                               outfile);
                       goto done;
                   }

           }
       else
           {
               ofptr = stdout;
           }


       while((c = fgetc(fp)) != EOF){
           fprintf(ofptr, "%c", c);
       }
       if(ofptr != stdout)
           fclose(ofptr);
       fclose(fp);
       num_errs = 0;            /* To make gzip work if -c is active.   */
    }
    else {
        fprintf(flog, 
                "Unable to generate map: %s has %d error(s).\n", 
                infile, num_errs);
        if(has_extf == 0){
            fprintf(flog, 
                    "Please try -c or -i option to generate incomplete map.\n"); 
        }
        else{
            fprintf(flog, 
                    "Can't generate map for HDF4 file that uses an external file.\n"); 
        }
    }

    /* Compress if z option is active. 
       This must be done after fclose(ofptr) call above.*/
    if(compress == 1 && num_errs == 0){
        /* This will create the map file where data file is located.  */
        sprintf(gzoutfile, "%s.gz", outfile);
        if(write_gzip(outfile,  gzoutfile, gzip_level) != SUCCEED){
            fprintf(flog, "failed to compress using gzip level %d.\n",
                    gzip_level);
            ++exit_errs;
        }
    }


 done:
    
    if(exit_errs > 0){
        if(num_errs > 0){
            rewind(ferr);
            while((c = fgetc(ferr)) != EOF){
                fprintf(flog, "%c", c);               
            }
        }
    }

    /* Free the visited object reference lists. */
    ref_count_free(&sd_visited);
    ref_count_free(&vs_visited);
    ref_count_free(&vg_visited);
    ref_count_free(&ris_visited);
    ref_count_free(&palIP8_visited);
    ref_count_free(&palLUT_visited);
    ref_count_free(&obj_missed);

    if (flog != stderr && flog != NULL){
        fclose(flog);
    }

    if (ferr != NULL){
        fclose(ferr);
    }

    if(unlink(tmperrname) != 0){
        fprintf(stderr, "Failed to delete temporary error file: %s\n", 
                tmperrname);
        ++exit_errs;
    };

    
    if (tmpfp != NULL) {
        fclose(tmpfp);
    }
    /* Comment out the following statement to save temporary output. */
    /* It can help you to debug writer by looking at where it stopped. */
    if(unlink(tmpname) != 0){
        fprintf(stderr, "Failed to delete temporary output file: %s\n", 
                tmpname);
        ++exit_errs;
    };

    if (sd_id != FAIL) {
        SDend(sd_id);
    }
    
    if (gr_id != FAIL){
        GRend(gr_id);
    }

    if (an_id != FAIL){
        ANend(file_id);
    }

    if (file_id > 0) {
        Vend(file_id);
        Hclose(file_id);
    }

    if (vg_refs)
        HDfree(vg_refs);

    free_pal_arrays();
    free_did2sid_list();
    free_dn2id_list();
    free_cv_list();
    free_dn_list();
    free_mattr_list();

    /* Return 0 when there is no errors. */
    return(exit_errs + num_errs);
}
