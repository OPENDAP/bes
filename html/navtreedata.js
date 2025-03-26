/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "bes", "index.html", [
    [ "Introduction", "index.html#intro", null ],
    [ "Doxygen Conventions", "index.html#conventions", [
      [ "General", "index.html#general", null ],
      [ "Classes", "index.html#classes", null ],
      [ "Methods and Functions", "index.html#methods", null ]
    ] ],
    [ "vlsa-encoding", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html", [
      [ "VLSA dmr++ encoding efficiency", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md20", null ],
      [ "Two methods:", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md21", null ],
      [ "XML markup for each value", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md22", null ],
      [ "= base64 encoded and packed", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md23", null ],
      [ "The base64 encoding has a constant linear cost.", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md24", null ],
      [ "The encoded values are 134% of the raw string.", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md25", null ],
      [ "The XML markup version adds a constant size", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md26", null ],
      [ "increase of 15 characters per value (assuming", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md27", null ],
      [ "an average indent of 8 characters in the pretty", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md28", null ],
      [ "version we currently produce.", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md29", null ],
      [ "Worst case for the XML markup is 150% increase", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md30", null ],
      [ "for a single character string.", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md31", null ],
      [ "For strings 12 characters or longer the XML", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md32", null ],
      [ "more efficient base64 representation.", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md33", null ],
      [ "autotoc_md34", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md34", null ],
      [ "And you can read the XML with eyeballs.", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md35", null ],
      [ "- - - - - - - - - - - - - - - - - - - - - - -", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md36", null ],
      [ "autotoc_md37", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md37", null ],
      [ "Constant one time cost both versions:", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md38", null ],
      [ "Using XML Markup for each value.", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md39", null ],
      [ "The XML markup is a constant cost for each", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md40", null ],
      [ "string value.", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md41", null ],
      [ "15 / len = increase", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md42", null ],
      [ "Base64 encoding and packed into a single", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md43", null ],
      [ "XML element.", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md44", null ],
      [ "This is a linear cost", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md45", null ],
      [ "encoded - raw = diff diff/len", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md46", null ],
      [ "--------------------------------—", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md47", null ],
      [ "212 chars", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md48", null ],
      [ "--------------------------------—", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md49", null ],
      [ "433 chars", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md50", null ],
      [ "--------------------------------—", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md51", null ],
      [ "1170 chars", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md52", null ],
      [ "--------------------------------—", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md53", null ],
      [ "15873 chars", "da/d20/md_modules_2dmrpp__module_2vlsa-encoding.html#autotoc_md54", null ]
    ] ],
    [ "LICENSE", "d9/df1/md_pugixml_2LICENSE.html", null ],
    [ "README-VALGRIND", "d7/d85/md_README-VALGRIND.html", null ],
    [ "How to build the online documentation for the bes", "d3/d26/md_README_8gh-pages.html", [
      [ "How to make the special 'orphaned branch' gh-pages", "d3/d26/md_README_8gh-pages.html#autotoc_md68", null ]
    ] ],
    [ "README", "d0/d30/md_README.html", [
      [ "README for the OPeNDAP BES", "d0/d30/md_README.html#autotoc_md69", null ],
      [ "Version 3.21.1-0", "d0/d30/md_README.html#autotoc_md70", null ],
      [ "Version 3.21.0-46", "d0/d30/md_README.html#autotoc_md71", null ],
      [ "Version 3.20.13", "d0/d30/md_README.html#autotoc_md72", null ],
      [ "Version 3.20.12", "d0/d30/md_README.html#autotoc_md73", null ],
      [ "Version 3.20.11", "d0/d30/md_README.html#autotoc_md74", null ],
      [ "Introduction", "d0/d30/md_README.html#autotoc_md75", [
        [ "Contents", "d0/d30/md_README.html#autotoc_md76", null ]
      ] ],
      [ "What's here", "d0/d30/md_README.html#autotoc_md77", null ],
      [ "Configuration", "d0/d30/md_README.html#autotoc_md78", [
        [ "Basic Configuration", "d0/d30/md_README.html#autotoc_md79", null ],
        [ "Configuration for Hyrax", "d0/d30/md_README.html#autotoc_md80", [
          [ "Installing a custom handler/module", "d0/d30/md_README.html#autotoc_md81", [
            [ "Install the newly-built handler.", "d0/d30/md_README.html#autotoc_md82", null ]
          ] ]
        ] ],
        [ "Testing", "d0/d30/md_README.html#autotoc_md83", null ]
      ] ],
      [ "Various features the BES supports", "d0/d30/md_README.html#autotoc_md84", [
        [ "DAP2 versus DAP4", "d0/d30/md_README.html#autotoc_md85", null ],
        [ "Support for data stored on Amazon's S3 Web Object Store", "d0/d30/md_README.html#autotoc_md86", null ],
        [ "Support for dataset crawler/indexer systems", "d0/d30/md_README.html#autotoc_md87", null ],
        [ "Experimental support for STARE indexing", "d0/d30/md_README.html#autotoc_md88", null ],
        [ "Server function roi() improvements", "d0/d30/md_README.html#autotoc_md89", null ],
        [ "Site-specific configuration", "d0/d30/md_README.html#autotoc_md90", null ],
        [ "The Metadata Store (MDS)", "d0/d30/md_README.html#autotoc_md91", null ],
        [ "COVJSON Responses", "d0/d30/md_README.html#autotoc_md92", null ],
        [ "Improved catalog support", "d0/d30/md_README.html#autotoc_md93", null ],
        [ "For HDF4 data", "d0/d30/md_README.html#autotoc_md94", null ],
        [ "For HDF5 data", "d0/d30/md_README.html#autotoc_md95", null ]
      ] ],
      [ "COPYRIGHT INFORMATION", "d0/d30/md_README.html#autotoc_md96", [
        [ "PicoSHA2 license", "d0/d30/md_README.html#autotoc_md97", null ],
        [ "pugixml license", "d0/d30/md_README.html#autotoc_md98", null ],
        [ "rapidjson license", "d0/d30/md_README.html#autotoc_md99", null ]
      ] ]
    ] ],
    [ "Deprecated List", "da/d58/deprecated.html", null ],
    [ "Todo List", "dd/da0/todo.html", null ],
    [ "Topics", "topics.html", "topics" ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", null ],
        [ "Typedefs", "functions_type.html", null ],
        [ "Enumerations", "functions_enum.html", null ],
        [ "Related Symbols", "functions_rela.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ],
        [ "Variables", "globals_vars.html", null ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Enumerator", "globals_eval.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"annotated.html",
"d0/d90/classlibdap_1_1NDimensionalArray.html#ab5aa82d140f628ccca8014f0d9f4b568",
"d0/df7/classhttp_1_1url.html",
"d1/d79/RemoteResource_8h_source.html",
"d2/d19/classBESShowContextResponseHandler.html#ac377010cdc9f85ae702ae90d5d7d502d",
"d2/d85/classShowPathInfoCommand.html#a739549a03dcf93b071fdf5821e0a6c9f",
"d2/dfb/classagg__util_1_1RCObject.html#afb16e3ea983f5ab9ea2d6f06850a47b6",
"d3/d6a/HttpdCatalogModule_8h_source.html",
"d4/d2c/HDFCFStr_8cc.html",
"d4/db5/classHE2CF.html#a971a7c9570c3faba9c8dc58c8f423802",
"d5/d19/WWWFloat32_8cc_source.html",
"d5/d72/classagg__util_1_1AggMemberDatasetWithDimensionCacheBase.html#a6c6f7a4fc5ad21b73f6273064f689575",
"d6/d1d/HDF5UInt32_8h_source.html",
"d6/d8f/classPPTServer.html#a54383ffe309a5a7c49f66f3d68fd2535",
"d7/d13/classFONcTransform.html#a93904185a04ac0fbb1dbe5b25e8d8e03",
"d7/d7e/classagg__util_1_1GridJoinExistingAggregation.html#ab6c4436ada3c9e51ca74796c2f961ffd",
"d7/df7/classncml__module_1_1SimpleLocationParser.html#ae2bd384321956ef42f62572fa2999c3b",
"d8/d60/DmrppParserSax2_8h_source.html",
"d9/d19/structUTF32BE.html",
"d9/ddd/ServerHandler_8h_source.html",
"da/d46/dmrpp__module_2h5common_8cc.html",
"da/dc7/classGenericDocument.html#a54d96ce0902d2afe033faebfd2863bbc",
"db/d10/classdmrpp_1_1DmrppD4Sequence.html#aa532c99dff0c68c87f097e465d6aa8b2",
"db/d7f/classncml__module_1_1ScanElement.html#af80e7e3ef5047df2960bf03a1f1f52f2",
"dc/d16/classdmrpp_1_1DmrppByte.html#a24a964b6ce3ba4c0656b96dd4ed3d4fa",
"dc/d6e/classngap_1_1NgapOwnedContainer.html#af9c65df5d6033d048fd6c76147019f44",
"dd/d09/classHDF5CF_1_1EOS5File.html#af56cd089fdf2f7e81305b54bafe7fcf2",
"dd/d8b/classGenericPointer.html#ae1b46fbcab2c8557825f7be842acbfe7",
"dd/df9/classNCMLContainerStorage.html#aefcb1040fc818ead3b0e3a6f8c462b9e",
"de/d7d/classbes_1_1DmrppMetadataStore.html",
"df/d05/classDODS__EndTime__Factory.html#ad62eae80ba5a16c2d12462ded8dd6472",
"df/d87/NgapBuildDmrppContainer_8h_source.html",
"df/df1/classdmrpp_1_1DmrppUInt16.html#a7effa4ee168b68f7297b1088c74f4ce5"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';