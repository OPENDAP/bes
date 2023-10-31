/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2010-2016  The HDF Group.                                   *
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
  \file xml.h
  \brief Define XML tag elements.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Aug 2, 2016
  \note added write_uuid() header.

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 17, 2012
  \note updated for new palette handling
  changed parameters for write_pal_attrs_an() and write_map_pals().
  deleted declarations for free_pr2id_list(), get_pr2id_list(), set_pr2id_list().
  added declaration for palRef_to_palMapID().

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date June 30, 2011
  \note updated to increase readability.

  \date June 29, 2011
  \note 
  added TAG_FLOC, TAG_FLOC_TYPE, and TAG_FLOC_VAL.
  updated XML_INSTRUCTION to include note about offset/scale.

  \date May 24, 2011
  \note split XML_WARNING into three lines.

  \date May 23, 2011
  \note removed XML_TEND_LE and XML_TEND_BE.


  \date May 10, 2011
  \note write_ignored_tags() is added.


  \date May 9, 2011
  \note 
  added ri_id argument.	
  changed return type from void to intn.
  modified RLE part in XML_INSTRUCTION.

  \date May 3, 2011
  \note 
  added XML_WARN.

  \date May 2, 2011
  \note 
  added "version=0.9" in XML_HEAD_SCHEMA.

  \date April 29, 2011
  \note 
  changed XML_HEAD_SCHEMA.
  changed XML_INSTRUCTION.
  added XML_FILEINFO.
  added TAG_FINFO.
  added TAG_FCNT. 
  added TAG_FNAME.
  added TAG_FSIZE.
  added TAG_MD5.

  \date March 17, 2011
  \note 
  changed return type for ref_count_init().
  added raster attribute definition(TAG_RATTR).
  added write_raster_attrs().


  \date March 10, 2011
  \note removed PROFILE_EXT "map.xml" definition.

  \date October 14, 2010
  \note sorted functions by alphabetical order.

*/

#define NS "h4"                 /*!< XML name space */

/* Tag names of xml elements */
#define TAG_DOC     NS ":HDF4map"  /*!< Top level element. */
#define TAG_FINFO   NS ":HDF4FileInformation"  /*!< File info. element. */
#define TAG_FCNT    NS ":HDF4FileContents"  /*!< File content element. */
#define TAG_FNAME   NS ":fileName"  /*!< File name element. */
#define TAG_FLOC    NS ":fileLocation"  /*!< File location element. */
#define TAG_FLOC_T  TAG_FLOC "Type"  /*!< File location type element. */
#define TAG_FLOC_V  TAG_FLOC "Value"  /*!< File location value element. */
#define TAG_FSIZE   NS ":fileSize"  /*!< File size element. */
#define TAG_MD5     NS ":md5Checksum"  /*!< File MD5 checksum element. */
#define TAG_GRP     NS ":Group" /*!< Vgroup element. */
#define TAG_DSET    NS ":Array" /*!< SDS element. */
#define TAG_VS      NS ":Table" /*!< Vdata selement */
#define TAG_RIS     NS ":Raster"   /*!< Raster Image element */
#define TAG_PAL     NS ":Palette" /*!< Palette element  */
#define TAG_DSPACE  NS ":dataDimensionSizes" /*!< array dimension sizes  */
#define TAG_ASPACE  NS ":allocatedDimensionSizes" /*!< chunk dimension sizes */
#define TAG_FIELD   NS ":Column" /*!< Vdata field  */
#define TAG_ATTR    NS ":Attribute" /*!< Attributes  */
#define TAG_DATUM   NS ":datum" /*!< type and endianness  */
#define TAG_TDATA   NS ":tableData" /*!< Vdata table  */
#define TAG_BSTREAM NS ":byteStream" /*!< non-attribute data.  */
#define TAG_BSTMSET NS ":byteStreamSet" /*!< set of byte streams.  */
#define TAG_ATTDATA NS ":attributeData" /*!< attribute data */
#define TAG_FATTR   NS ":FileAttribute" /*!< file attribute. */
#define TAG_GATTR   NS ":GroupAttribute" /*!< group attribute. */
#define TAG_TATTR   NS ":TableAttribute" /*!< table attribute. */
#define TAG_CATTR   NS ":ColumnAttribute" /*!< column attribute. */
#define TAG_AATTR   NS ":ArrayAttribute" /*!< array attribute. */
#define TAG_DATTR   NS ":DimensionAttribute" /*!< dimension attribute. */
#define TAG_PATTR   NS ":PaletteAttribute" /*!< palette attribute. */
#define TAG_RATTR   NS ":RasterAttribute" /*!< array attribute. */
#define TAG_ARRDATA NS ":arrayData"            /*!< SDS data  */
#define TAG_CHUNK   NS ":chunks"               /*!< chunking  */
#define TAG_CSPACE  NS ":chunkDimensionSizes" /*!< chunking dimension sizes */
#define TAG_SVALUE  NS ":stringValue"   /*!< char attribute value */
#define TAG_NVALUE  NS ":numericValues" /*!< numeric attribute value */
#define TAG_DIM     NS ":Dimension"     /*!< dimension scale */
#define TAG_DREF    NS ":dimensionRef"  /*!< dimension scale reference */
#define TAG_DDATA   NS ":dimensionData"  /*!< dimension scale data  */
#define TAG_FVALUE  NS ":fillValues"  /*!< dimension scale data  */
#define TAG_PDATA   NS ":paletteData"  /*!< palette data  */
#define TAG_PREF    NS ":paletteRef"  /*!< palette reference */
#define TAG_RDATA   NS ":rasterData"  /*!< raster data  */

/* XML constants */
#define XML_INDENT "    "       /*!< Indentation. Use \\t if TAB is desired.  */
#define XML_HEAD "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<" TAG_DOC " " /*!< XML identifier. */

#define XML_HEAD_SCHEMA " version=\"1.0.1\" xsi:schemaLocation=\"http://www.hdfgroup.org/HDF4/XML/schema/HDF4map/1.0.1/HDF4map.xsd\" xmlns:h4=\"http://www.hdfgroup.org/HDF4/XML/schema/HDF4map/1.0.1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">" /*!< Update it whenever schema changes  */
#define XML_FILEINFO "\n<!--\n" \
"This XML file provides access to data stored in a companion HDF4 file without requiring HDF4 software.\n" \
"\n" \
"The HDF4FileInformation element provides information about the companion HDF4 file.\n" \
"\n" \
"-->" /*!< Instruction on file information */

#define XML_INSTRUCTION "\n<!--\n" \
"\n" \
"The HDF4FileContents element maps the contents of the companion HDF4 file.  \n" \
"Some contents are available directly from this element.  \n" \
"Other contents can be located in the HDF4 file and decoded using information in this element.\n" \
"\n" \
"Abbreviations:\n" \
"obj = HDF4 object\n" \
"elem = XML element\n" \
"attr = XML attribute\n" \
"Elem = Attribute, Group, Table, Array, Dimension, Raster or Palette elem\n" \
"Read bytes = Using information from byteStream elem, read the indicated number of bytes, starting at the indicated \n" \
"  zero-based offset in the HDF4 file. If there are multiple byteStream elems for a given Elem, they will be subelems of\n" \
"  a byteStreamSet elem; read and concatenate the bytes from the multiple byteStreams in the order they appear.\n" \
"Process = Using information from the datum elem, process the bytes read on a datum-by-datum basis, applying byteOrder\n" \
"  transformations if needed.\n" \
"Access = Interpret the processed bytes based on the dataType to obtain the raw data values. Calibrate raw data values \n" \
"  if Elem has calibration Attribute elems.\n" \
"\n" \
"HDF4FileContents:\n" \
"-uses elems to represent objs in the HDF4 file\n" \
"-uses nested elems and elem references to express relationships of objs to each other\n" \
"-uses byteStream elems to provide maps to bytes in the HDF4 file that hold raw data for objs\n" \
"-contains selected raw data values so the reader can verify binary data has been handled properly\n" \
"\n" \
"Representations:\n" \
"-Attribute elem represents Attribute or Annotation obj\n" \
" -contains user metadata\n" \
"-Group elem represents Vgroup obj\n" \
" -used to associate related objs\n" \
" -can form directed graphs\n" \
"-Table elem represents Vdata obj\n" \
" -organizes data into rows (records) and columns (fields)\n" \
" -datatype specified per column\n" \
" -number of entries per cell for a given column is constant for all rows and may be >1; all entries in a cell have \n" \
"  same datatype\n" \
"-Array elem represents SDS obj\n" \
" -organizes data into multi-dimensional array\n" \
" -all cells have same datatype\n" \
" -references Dimension elem for every dimension that has descriptive metadata\n" \
"-Dimension elem represents Dimension obj\n" \
" -contains descriptive metadata (name, attribute, scale value/coordinate variable) about a dimension for one or more \n" \
"  SDS objs\n" \
"-Raster elem represents RIS8 or GR obj\n" \
" -two-dimensional [row,column] array of component values representing pixels\n" \
" -row 0 corresponds to top of image; column 0 corresponds to left of image\n" \
" -one component value per pixel; all values have single-byte datatype\n" \
" -references Palette elem to correlate pixel values to colors\n" \
"-Palette elem represents Palette or Color Lookup Table obj\n" \
" -provides colors for images\n" \
" -each entry has 3 color components corresponding to Red, Green, Blue\n" \
"\n" \
"Bytes in HDF4 file (general):\n" \
"-Raw data are stored in binary format in the HDF4 file. The steps to convert bytes into meaningful raw data values \n" \
" depend on the elem and the HDF4 features used when obj was written\n" \
"-byteStream elems, together with other attrs and elems, provide the information required to convert the binary data\n" \
"-A datum elem defines the atomic unit of raw data for the obj it applies to\n" \
"-Each elem in this file that has raw data in the HDF4 file associated with it contains a datum subelem that describes \n" \
" the data type, size, and format\n" \
" -The number at the end of the dataType is the number of bits per datum; divide by 8 to get number of bytes to process \n" \
"  together\n" \
" -Integers are twos-complement and floating point is IEEE 754-1985\n" \
"\n" \
"Conversion Instructions:\n" \
"==Attribute\n" \
"Raw data stored as single stream of bytes in HDF4 file\n" \
"-Read bytes\n" \
" -If byteStreamSet elem for a given FileAttribute, multiple File Attribute objs were combined in the map file\n" \
"  Each byteStream elem can be processed separately to reproduce the contents of the individual objs if desired\n" \
"  (HDF4 limited the size of Attribute objs, so metadata was often split across multiple objs)\n" \
"-Process\n" \
"-Access\n" \
"Note: stringValue (dataType=char8) and numericValues (other dataTypes) let reader access user metadata without \n" \
"converting binary data; these have been pre-processed to make them more human-readable.  They can also be used to \n" \
"verify conversion\n" \
"\n" \
"==Table\n" \
"-If the Table does not have raw data content associated with it there will not be a tableData subelem\n" \
" Only follow these instructions if tableData subelem" \
"\n" \
"Raw data\n" \
" -stored as one or more streams of bytes in the HDF4 file or as single stream of bytes in another external file\n" \
" -stored by row or by column\n" \
"-Read bytes\n" \
" -If dataInExternalFile elem, get bytes from file in ExternalFile elem\n" \
"-Prepare to process\n" \
" -If >1 row or >1 column, storageOrder indicates if data stored by row or by column\n" \
" -If a column has >1 entry, the entries for a given cell are always stored together, regardless of the storageOrder\n" \
"-Process\n" \
" -Follow storage order to determine \"current\" column and its datum element in this step\n" \
"-Access\n" \
" -Follow storage order to determine dataType\n" \
"Note: comment block \"row(s) for verification\" can be used to verify conversion\n" \
"\n" \
"==Array\n" \
"-If the Array does not have raw data content associated with it there will not be an arrayData subelem\n" \
" Only follow these instructions if arrayData subelem\n" \
"\n" \
"Raw data\n" \
" -may be stored as a complete array (as a whole) or as multi-dimensional sub-arrays (as chunks), indicated by chunks \n" \
"  subelem\n" \
" -may be stored in a compressed format, indicated by compressionType attr\n" \
" -are stored as one or more streams of bytes in the HDF4 file or as single stream of bytes in another external file\n" \
" -are stored with either rightmost or leftmost array index varying fastest\n" \
"\n" \
"==Array stored as a whole\n" \
"-Read bytes\n" \
" -If Array contains all fill values, there is a fillValues element and no bytes are read; use value in that element\n" \
"  as raw data value for all cells in Array then skip to -Access step\n" \
" -If dataInExternalFile elem, get bytes from file in ExternalFile elem\n" \
"-If data was compressed, uncompress using indicated algorithm\n" \
"-Prepare to process\n" \
" -fastestVaryingDimensionIndex attr indicates storage order when nDimensions >= 1\n" \
"-Process\n" \
" -Follow storage order when processing\n" \
"-Access\n" \
"Note: comment block \"value(s) for verification\" can be used to verify conversion\n" \
"\n" \
"==Array stored as chunks\n" \
"With chunked storage, the data for each chunk is processed individually and then positioned in the array\n" \
"A chunk (sub-array) has the same number of dimensions as the complete array, and the dimension sizes for all chunks \n" \
"  are the same\n" \
"Special Notes:\n" \
"-When the dimension sizes for the complete array are not exact multiples of the dimension sizes for the chunks, \n" \
" extra \"ghost cells\" are stored. Data in ghost cells is not meaningful, but must be read and processed to get proper \n" \
" alignment of the actual array data values\n" \
"-When all data values in a given chunk are equal to the fill value, no bytes are written to the HDF4 file for that \n" \
" chunk. The data values for the chunk must all be set to the fill value.\n" \
"\n" \
"-Read bytes for a single chunk\n" \
" -If chunk contains all fill values, there is a fillValues elem instead of byteStream elem for the chunk and no bytes\n" \
"  are read; use value in fillValues elem as raw data value for all cells in chunk then skip to -Position data... step\n" \
"-If data was compressed, uncompress using indicated algorithm\n" \
"-Prepare to process \n" \
" -fastestVaryingDimensionIndex attr indicates storage order\n" \
" -since data stored chunk-by-chunk, storage order applies on a per-chunk basis\n" \
"  e.g., for 3x5 array dims, 2x3 chunk dims, and fastestVaryingDimensionIndex=1, data values will be assigned to chunk\n" \
"  indices [0,0], [0,1], [0,2], [1,0], [1,1], [1,2] as it is processed in the next step\n" \
"-Process\n" \
" -Follow storage order for chunk when processing\n" \
" -Interpret the processed bytes based on the storage order and dataType to obtain the raw data values for the cells \n" \
"  in the current chunk \n" \
"-Position data values for chunk within allocated array\n" \
" -If there are ghost cells, the elem allocatedDimensionSizes indicates the size of the allocated array. If this elem \n" \
"  is not present, dataDimensionSizes (the dimensions for the actual array) indicates the size of the allocated array\n" \
" -Use chunkPositionInArray offsets to position the data values for the current chunk within the allocated array\n" \
"  e.g., for 3x5 array, 2x4 chunks, 4x8 allocated array, and chunkPositionInArray of [2,4] for the data just read\n" \
"   allocated array[2,4] = chunk value[0,0] (only cell that isn't a ghost cell in this chunk)\n" \
"   allocated array[2,5] = chunk value[0,1]\n" \
"   allocated array[2,6] = chunk value[0,2]\n" \
"   ...\n" \
"   allocated array[3,7] = chunk value[1,3]\n" \
"-Repeat steps for each chunk in the array\n" \
"-Access\n" \
" -Ghost cells do not contain meaningful data values\n" \
"Note: comment block \"value(s) for verification\" can be used to verify conversion\n" \
"\n" \
"==Dimension\n" \
"A Dimension elem will be present if a dimension has one or more of these: (1) a name (2) an attribute (3) values\n" \
"If there are no values, the Dimension elem does not have raw data content associated with it and will not contain a\n" \
"dimensionData subelem. Only follow these instructions if dimensionData subelem\n" \
"\n" \
"Raw data stored as one or more streams of bytes in the HDF4 file\n" \
"-Read bytes\n" \
"-Process\n" \
"-Access\n" \
"Note: comment block \"value(s) for verification\" can be used to verify conversion\n" \
"\n" \
"==Raster\n" \
"-If the Raster does not have raw data content associated with it there will not be an rasterData subelem\n" \
" Only follow these instructions if rasterData subelem\n" \
"\n" \
"Raw data stored as single stream of bytes in HDF4 file\n" \
"-Read bytes\n" \
" -If Raster contains all fill values, there is a fillValues element and no bytes are read; use value in that element\n" \
"  as raw data value for all cells in Raster then skip to -Access step\n" \
"-If data was compressed, uncompress using algorithm indicated by compressionType attr\n" \
"-Prepare to process\n" \
" -dimensionStorageOrder attr indicates order of values in file; last dimension always varies the fastest\n" \
"-Process\n" \
" -Follow storage order when processing\n" \
"-Access\n" \
"Note: comment block \"value(s) for verification; [row,column]\" can be used to verify conversion\n" \
"\n" \
"==Palette\n" \
"Raw data stored as single stream of bytes in HDF4 file\n" \
"-Read bytes\n" \
"-Prepare to process\n" \
" -Storage order is entry[0][red]; entry[0][green]; entry[0][blue]; entry[1][red]...\n" \
"-Process\n" \
" -Follow storage order when processing\n" \
"-Access\n" \
"Note: comment block \"value(s) for verification; csv format\" can be used to verify conversion\n" \
"\n" \
"Calibration:\n" \
"Calibration Attribute elems (calibrated_nt, scale_factor, scale_factor_err, add_offset, add_offset_err) provide \n" \
" calibration information for raw data values of an Elem. To compute original data values apply this formula: \n" \
"  original_data_value = scale_factor * (raw_data_value - add_offset)\n" \
"scale_factor_err and offset_error give potential errors due to scaling and offset\n" \
"calibrated_nt is encoded datatype of original data values: 3=uchar8 4=char8 5=float32 6=float64 7=float128 \n" \
" 20=int8 21=uint8 22=int16 23=uint16 24=int32 25=uint32 26=int64 27=uint64 28=int128 30=uint128 42=char16 43=uchar16\n" \
"Note: Some files may use the calibration attributes in a manner different than the standard way described.  \n" \
"Consult relevant data product specifications for your particular HDF4 files.\n" \
"\n" \
"Compression algorithms:\n" \
"-deflate: also known as gzip or zlib; see IETF RFC1951\n" \
"-rle: byte-wise run length encoding\n" \
" Each run is preceded by a pseudo-count byte\n" \
" Low seven bits of the byte indicate the adjusted number of bytes (n)\n" \
" If high bit is 1, next byte should be replicated n+3 times\n" \
" If high bit is 0, next n+1 bytes should be included in whole\n" \
"\n" \
"-->" /*!< Instructions for reading map */
#define XML_WARN "Warning: Some objects or features in the HDF4 file were not mapped since they\n fell outside the capabilities of the map writer.  If recovery of these objects\n or features is important, the use of HDF4 libraries will be required." /*!< Warning message for unmappable objects  */
#define XML_FOOT "\n</" TAG_DOC ">\n" /*!< Last element. */
/* See Table 7B 24-Bit Raster Image Interlace format 
   in  http://hdfgroup.org/release4/HDF4_UG_html/UG_ris24s.html. */
#define XML_INTERLACE_PIXEL "by entry" /*!< components grouped by pixel  */
#define XML_INTERLACE_LINE "line" /*!<  components grouped by row*/
#define XML_INTERLACE_PLANE "plane" /*!< components grouped by plane */

#define XML_DATATYPE_CHAR8 "char8" /*!< Non-numeric datatype */
#define XML_DATATYPE_BYTE8 "byte8" /*!< Signed Byte  */
#define XML_DATATYPE_UBYTE8 "ubyte8" /*!< Unsigned Byte  */
#define XML_DATATYPE_INT8 "int8" /*!< 8-bit Integer */
#define XML_DATATYPE_UINT8 "uint8" /*!< Unsigned 8-bit Integer  */
#define XML_DATATYPE_INT16 "int16" /*!< 16-bit Integer  */
#define XML_DATATYPE_UINT16 "uint16" /*!< Unsigned 16-bit Integer   */
#define XML_DATATYPE_INT32 "int32" /*!< 32-bit Integer */
#define XML_DATATYPE_UINT32 "uint32" /*!< Unsigned 32-bit Integer */

#include "xml_sds.h"
#include "xml_vg.h"

#ifdef __cplusplus
extern "C" {
#endif


/* functions implemented in xml_an.c */
intn 
write_array_attrs_an(FILE *ofptr, int32 sds_id, int32 an_id, intn indent);

intn 
write_array_attrs_an_1(FILE *ofptr, int32 an_id, int32 tag, int32 ref, 
                       intn indent);
intn 
write_file_attrs_an(FILE *ofptr, int32 an_id,  intn indent);

intn 
write_group_attrs_an(FILE *ofptr, int32 an_id,  int32 tag, int32 ref,  
                     intn indent);
intn 
write_pal_attrs_an(FILE *ofptr, int32 an_id, uint16 tag, uint16 ref, intn indent);


/* functions implemented in xml_hmap.c */
intn
depth_first(int32 file_id, int32 vgid, int32 sd_id, int32 gr_id, int32 an_id,
            const char *path, FILE *ofptr, int indent,
            ref_count_t *sd_visited, ref_count_t *vs_visited,
            ref_count_t *vg_visited, ref_count_t *ris_visited);

void 
end_elm(FILE *ofptr, char *elm_name, int indent);

void 
end_elm_0(FILE *ofptr, char *elm_name);

void 
end_elm_c(FILE *ofptr, char *elm_name);

void
get_escape_xml(char *s);

char* 
get_string_xml(char *s, int count);

void 
indentation(FILE *ofptr, int indent);

int32 
ref_count(ref_count_t *count, int32 ref);

void 
ref_count_free(ref_count_t *count);

intn
ref_count_init(ref_count_t *count);

void 
start_elm(FILE *ofptr, char *elm_name, int indent);

void 
start_elm_0(FILE *ofptr, char *elm_name, int indent);

void 
start_elm_c(FILE *ofptr, char *elm_name, int indent);

void 
write_attr_data(FILE *ofptr, int32 offset, int32 length, int indent);

void 
write_attr_value(FILE *ofp, int32 nt, int32 cnt, VOIDP databuf, intn indent);

void 
write_byte_stream(FILE *ofptr, int32 offset, int32 length, intn indent);

void 
write_byte_streams(FILE *ofptr, int32 nblocks,  int32* offsets, int32* lengths,
                   intn indent);

int 
write_map_header(FILE *ofptr, char* filename, int file_size, char* md5_str, 
                 int no_full_path);

intn
write_map_datum(FILE *ofptr, int32 dtype, int indent);

void 
write_map_datum_char(FILE *ofptr, int32 dtype, int indent);

void 
write_missing_objects(FILE *ofptr, ref_count_t *obj_missed);

void 
write_ignored_tags(FILE *ofptr, int* list, int count);

void
write_uuid(FILE *ofptr, int32 offset, int32 length);

/* functions implemented in xml_pal.c */
intn
write_map_pals( FILE* ofptr, char* h4filename, int32 file_id, int32 gr_id, int32 an_id, 
                ref_count_t *palIP8_visited, ref_count_t *palLUT_visited);
uint
palRef_to_palMapID( uint16 ref );


/* functions implemented in xml_ris.c */
intn
write_file_attrs_gr(FILE *ofptr, int32 gr_id, int indent);

intn 
write_map_lone_ris(FILE *ofptr, int32 file_id,
                   int32 gr_id, ref_count_t *ris_visited);

intn
write_map_ris(FILE *ofptr, int32 file_id,
              int32 gr_id, uint16 tag, uint16 ref,
              const char *path,
              RIS_mapping_info_t *map_info, int indent,
              ref_count_t *ris_visited);

intn 
write_raster_attrs(FILE* ofptr, int32 ri_id, int32 nattrs, intn indent);

intn
write_raster_datum(FILE *ofptr, int32 dtype, int indent);

intn
write_ris_byte_stream(FILE* ofptr, RIS_mapping_info_t* map_info, int32 ri_id,
                      int indent);

/* functions implemented in xml_vdata.c */
intn 
write_VSattrs(FILE *ofptr, int32 id, int32 nattrs, intn indent);

intn 
write_column_attr(FILE* ofptr, int32 vid, int32 findex, intn indent);

intn
write_map_lone_vdata(FILE *ofptr, int32 file_id, ref_count_t *vs_visited);

intn
write_map_vdata(FILE *ofptr, int32 vdata_id, const char *path,
                VS_mapping_info_t *map_info, int indent, 
                ref_count_t *vs_visited);

intn 
write_verify_table_values(FILE *ofptr, int32 id, VS_mapping_info_t *map_info,
                          intn indent);

void 
write_verify_table_values_field(FILE *ofp, int32 nt, int32 cnt, VOIDP databuf, 
                                intn indent);

intn 
write_verify_table_values_row(FILE *ofptr, int32 id,
                              VS_mapping_info_t *map_info, int32 pos,
                              intn indent);


#ifdef __cplusplus
}

#endif
