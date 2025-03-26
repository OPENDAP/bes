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
    [ "README", "d0/d30/md_README.html", [
      [ "README for the OPeNDAP BES", "d0/d30/md_README.html#autotoc_md66", null ],
      [ "Version 3.21.1-0", "d0/d30/md_README.html#autotoc_md67", null ],
      [ "Version 3.21.0-46", "d0/d30/md_README.html#autotoc_md68", null ],
      [ "Version 3.20.13", "d0/d30/md_README.html#autotoc_md69", null ],
      [ "Version 3.20.12", "d0/d30/md_README.html#autotoc_md70", null ],
      [ "Version 3.20.11", "d0/d30/md_README.html#autotoc_md71", null ],
      [ "Introduction", "d0/d30/md_README.html#autotoc_md72", [
        [ "Contents", "d0/d30/md_README.html#autotoc_md73", null ]
      ] ],
      [ "What's here", "d0/d30/md_README.html#autotoc_md74", null ],
      [ "Configuration", "d0/d30/md_README.html#autotoc_md75", [
        [ "Basic Configuration", "d0/d30/md_README.html#autotoc_md76", null ],
        [ "Configuration for Hyrax", "d0/d30/md_README.html#autotoc_md77", [
          [ "Installing a custom handler/module", "d0/d30/md_README.html#autotoc_md78", [
            [ "Install the newly-built handler.", "d0/d30/md_README.html#autotoc_md79", null ]
          ] ]
        ] ],
        [ "Testing", "d0/d30/md_README.html#autotoc_md80", null ]
      ] ],
      [ "Various features the BES supports", "d0/d30/md_README.html#autotoc_md81", [
        [ "DAP2 versus DAP4", "d0/d30/md_README.html#autotoc_md82", null ],
        [ "Support for data stored on Amazon's S3 Web Object Store", "d0/d30/md_README.html#autotoc_md83", null ],
        [ "Support for dataset crawler/indexer systems", "d0/d30/md_README.html#autotoc_md84", null ],
        [ "Experimental support for STARE indexing", "d0/d30/md_README.html#autotoc_md85", null ],
        [ "Server function roi() improvements", "d0/d30/md_README.html#autotoc_md86", null ],
        [ "Site-specific configuration", "d0/d30/md_README.html#autotoc_md87", null ],
        [ "The Metadata Store (MDS)", "d0/d30/md_README.html#autotoc_md88", null ],
        [ "COVJSON Responses", "d0/d30/md_README.html#autotoc_md89", null ],
        [ "Improved catalog support", "d0/d30/md_README.html#autotoc_md90", null ],
        [ "For HDF4 data", "d0/d30/md_README.html#autotoc_md91", null ],
        [ "For HDF5 data", "d0/d30/md_README.html#autotoc_md92", null ]
      ] ],
      [ "COPYRIGHT INFORMATION", "d0/d30/md_README.html#autotoc_md93", [
        [ "PicoSHA2 license", "d0/d30/md_README.html#autotoc_md94", null ],
        [ "pugixml license", "d0/d30/md_README.html#autotoc_md95", null ],
        [ "rapidjson license", "d0/d30/md_README.html#autotoc_md96", null ]
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
"d0/df7/classhttp_1_1url.html#a0fb856b5bdaa8cee386d3263f388b2c2",
"d1/d7d/classBESXMLShowErrorCommand.html",
"d2/d19/classdmrpp_1_1DmrppInt64.html#a16142983bcd3bba4eb57370f1ba8d435",
"d2/d85/classShowPathInfoCommand.html#aba9b55f6d125a4dc210f030536d91ea0",
"d3/d02/structmemtrack__entry__struct__t.html",
"d3/d6a/classBESXDModule.html#aee92d12180d17fb923bb3af47105a57f",
"d4/d30/AggMemberDatasetUsingLocationRef_8h_source.html",
"d4/db7/BESXMLInterface_8cc_source.html",
"d5/d1f/DODS__Date__Time__Factory_8cc_source.html",
"d5/d72/classagg__util_1_1AggMemberDatasetWithDimensionCacheBase.html#a71f6dd7016edbc9dabadb6b83726e715",
"d6/d25/classReadTagRef.html",
"d6/d8f/classPPTServer.html#a711a9ffb25dc361baca260258d49cb2d",
"d7/d18/classfunctions_1_1StareBoxFunction.html",
"d7/d7e/classagg__util_1_1GridJoinExistingAggregation.html#ad1e5b4be4d08d14f518579477b938383",
"d7/dfc/classvector.html",
"d8/d64/classW10nJsonRequestHandler.html#a6043ce2bf6e4a0584caa9a13a3794957",
"d9/d31/HDF5Int8_8cc_source.html",
"d9/de2/classncml__module_1_1OtherXMLParser.html#a861d62cee38c269f33d15d86f68c13f5",
"da/d5c/classAsciiInt32.html#af48553a8fd3363f020c3c78a20830dbe",
"da/dc7/classGenericDocument.html#aee30721a49688ba0f865f5d581eb6be9",
"db/d10/classdmrpp_1_1DmrppD4Sequence.html#af5f6aa07929fe2ffb36b64b8a4317f66",
"db/d82/classFONcStructure.html#a865c780397607b8f48c1fb5096b967d7",
"dc/d16/classdmrpp_1_1DmrppByte.html#aa532c99dff0c68c87f097e465d6aa8b2",
"dc/d73/classBESXMLInterface.html#ad0d3fc680a2c2548ab81befd38071190",
"dd/d1b/classbes_1_1NullResponseHandler.html#a18da7f4dbfcc9cef736292e09883afca",
"dd/d98/classhttp_1_1RemoteResource.html#a2f70655c8f6e80cc99c8dc2361648890",
"de/d01/ndarray_8h_source.html",
"de/d7d/classbes_1_1DmrppMetadataStore.html#a86e416cb60cae7c0f67d1811322e7ab0",
"df/d20/XDByte_8cc_source.html",
"df/d98/NCMLModule_8h_source.html",
"df/df3/HDF5GMCFMissNonLLCVArray_8cc.html"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';