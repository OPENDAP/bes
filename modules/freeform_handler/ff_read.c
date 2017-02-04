/*
 (c) COPYRIGHT URI/MIT 1997-98
 Please read the full copyright statement in the file COPYRIGHT.
 Authors: reza (Reza Nekovei)
 */

#include "config_ff.h"

#include <freeform.h>

#include <debug.h>

/** Read from a file/database using the FreeForm API. Data values are read
 using an input file descriptor and written using an output format
 description.
 @param dataset The name of the file/database to read from
 @param if_file The input format descriptor
 @param o_format The output format description
 @param o_buffer Value-result parameter for the data
 @param bsize Size of the buffer in bytes */
long read_ff(const char *dataset, const char *if_file, const char *o_format,
        char *o_buffer, unsigned long bsize)
{
    FF_BUFSIZE_PTR newform_log = NULL;
    FF_BUFSIZE_PTR bufsz = NULL;
    FF_STD_ARGS_PTR std_args = NULL;
    long bytes_read = 0;

    std_args = ff_create_std_args();
    if (!std_args) {
        goto main_exit;
    }

    /* set the std_arg structure values - cast away const for dataset, if_file,
     * and o_format.*/
    std_args->error_prompt = FALSE;
    std_args->user.is_stdin_redirected = 0;
    std_args->input_file = (char*)(dataset);
    std_args->input_format_file = (char*)(if_file);
    std_args->output_file = NULL;
    std_args->output_format_buffer = (char*)(o_format);
    std_args->log_file = "/dev/null";
    /* Define DBG (as per dap/debug.h) to get a log file from FreeForm. 9/8/98
     jhrg */
    std_args->log_file = "/tmp/ffdods.log";

    bufsz = (FF_BUFSIZE_PTR) memMalloc(sizeof(FF_BUFSIZE), "bufsz");
    if (!bufsz) {
        goto main_exit;
    }

    bufsz->usage = 1;
    bufsz->buffer = o_buffer;
    bufsz->total_bytes = (FF_BSS_t) bsize;
    bufsz->bytes_used = (FF_BSS_t) 0;

    std_args->output_bufsize = bufsz;

    newform_log = ff_create_bufsize(SCRATCH_QUANTA);
    if (!newform_log) {
        goto main_exit;
    }

    newform(std_args, newform_log, stderr);

    ff_destroy_bufsize(newform_log);

    main_exit:

    err_disp(std_args);

    if (std_args)
        ff_destroy_std_args(std_args);

    bytes_read = bufsz ? bufsz->bytes_used : 0;

    if (bufsz)
        memFree(bufsz, "bufsz");

    return bytes_read;
}

#ifdef TEST

/* This 'TEST_LOGGING' code was part of the above code. Moved here to
 * reduce clutter. jhrg 8/10/12
 */
#ifdef TEST_LOGGING
    char log_file_write_mode[4];
    /* Is user asking for both error logging and a log file? */
    if (std_args->error_log && newform_log) {
        if (strcmp(std_args->error_log, std_args->log_file))
        strcpy(log_file_write_mode, "w");
        else
        strcpy(log_file_write_mode, "a");

    }
    else if (newform_log)
    strcpy(log_file_write_mode, "w");

    FILE *fp = NULL;

    fp = fopen(std_args->log_file, log_file_write_mode);
    if (fp) {
        size_t bytes_written = fwrite(newform_log->buffer, 1,
                (size_t)newform_log->bytes_used, fp);

        if (bytes_written != (size_t)newform_log->bytes_used)
        error = err_push(ERR_WRITE_FILE, "Wrote %d bytes of %d to %s",
                (int)bytes_written,
                (int)newform_log->bytes_used, std_args->log_file);

        fclose(fp);
    }
    else
    error = err_push(ERR_CREATE_FILE, std_args->log_file);

    if (std_args->user.is_stdin_redirected)
    ff_destroy_bufsize(std_args->input_bufsize);
#endif /* TEST_LOGGING */

    #define OUTPUT_FMT_STR "binary_output_data \"output\"\n\
year 1 4 int32 0\n\
day 5 8 int32 0\n\
time 9 9 text 0\n\
DODS_URL 10 135 text 0"

int main(int argc, char *argv[])
{
    char *datafile;
    char *in_format;
    char *out_format;
    char *data;
    unsigned long data_size;
    unsigned long bytes_in;

    /* name and directory for data file */
    datafile = malloc(strlen(argv[1]) + 1);
    strcpy(datafile, argv[1]);

    /* name and directory for the input format file */
    in_format = malloc(strlen(argv[2]) + 1);
    strcpy(in_format, argv[2]);

    /* output format written in the buffer */
    out_format = malloc(strlen(OUTPUT_FMT_STR) + 1);
    strcpy(out_format, OUTPUT_FMT_STR);

    /* size of data expecting to read (can be oversized) */
    data_size = atoi(argv[3]);
    data = malloc(data_size);

    printf("Converting to the following format:\n%s\n",out_format);
    bytes_in = read_ff(datafile, in_format, out_format, data, data_size);

    printf("Bytes read: %ld\n", bytes_in);
    if (bytes_in> 0)
    fwrite(data, 1, bytes_in, stdout);

    exit(0);
}

#endif
