/*
 * h5_write_utf8_vlen_str.c
 *
 * Create an HDF5 file with a 1-D, 4-element, variable-length string dataset
 * whose character set is UTF-8. One element contains a Latin-1 character
 * (U+00ED, 'í'), stored as its UTF-8 encoding: 0xC3 0xAD.
 *
 * Uses only the core HDF5 C API (no HL APIs).
 *
 * Build:
 *   h5cc -o h5_write_utf8_vlen_str h5_write_utf8_vlen_str.c
 *
 * Usage:
 *   ./h5_write_utf8_vlen_str [output_file]     (default: utf8_vlen_str.h5)
 */

#include <stdio.h>
#include <stdlib.h>
#include "hdf5.h"

#define DSET_NAME "/site_name"
#define NELEMS    4

int
main(int argc, char *argv[])
{
    const char *fname = (argc > 1) ? argv[1] : "utf8_vlen_str.h5";

    /* Element 1 holds 'í' written as explicit UTF-8 bytes, so the result does
     * not depend on the encoding this source file is saved in. */
    const char *data[NELEMS] = {
        "Cuncumen",
        "Puchuncav\xC3\xAD", /* "Puchuncaví" */
        "Ventanas",
        "La Greda"
    };

    hsize_t dims[1] = {NELEMS};
    hid_t   file = H5I_INVALID_HID, space = H5I_INVALID_HID;
    hid_t   strtype = H5I_INVALID_HID, dset = H5I_INVALID_HID;
    int     ret = EXIT_FAILURE;

    if ((file = H5Fcreate(fname, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto done;

    if ((space = H5Screate_simple(1, dims, NULL)) < 0)
        goto done;

    /* Variable-length C string, null-terminated, character set UTF-8.
     * The same type is used as the file type and the memory type. */
    if ((strtype = H5Tcopy(H5T_C_S1)) < 0)
        goto done;
    if (H5Tset_size(strtype, H5T_VARIABLE) < 0)
        goto done;
    if (H5Tset_strpad(strtype, H5T_STR_NULLTERM) < 0)
        goto done;
    if (H5Tset_cset(strtype, H5T_CSET_UTF8) < 0)
        goto done;

    if ((dset = H5Dcreate2(file, DSET_NAME, strtype, space,
                           H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto done;

    /* For variable-length strings the buffer is an array of char pointers. */
    if (H5Dwrite(dset, strtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data) < 0)
        goto done;

    printf("Wrote %d UTF-8 variable-length strings to %s:%s\n", NELEMS, fname, DSET_NAME);
    ret = EXIT_SUCCESS;

done:
    if (dset >= 0)    H5Dclose(dset);
    if (strtype >= 0) H5Tclose(strtype);
    if (space >= 0)   H5Sclose(space);
    if (file >= 0)    H5Fclose(file);
    return ret;
}
