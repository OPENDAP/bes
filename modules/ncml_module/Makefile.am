
# Automake file for ncml-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news 

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=ncml_moddule
M_VER=1.5.7

AM_CPPFLAGS = $(ICU_CPPFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap \
-I$(top_srcdir)/xmlcommand $(DAP_CFLAGS)
LIBADD = $(ICU_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libncml_module.la

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = . tests

BES_SRCS:=
BES_HDRS:=

NCML_SRCS = \
		AggMemberDataset.cc \
		AggMemberDatasetDDSWrapper.cc \
		AggMemberDatasetSharedDDSWrapper.cc \
		AggMemberDatasetUsingLocationRef.cc \
		AggMemberDatasetWithDimensionCacheBase.cc \
		AggMemberDatasetDimensionCache.cc \
		AggregationElement.cc \
		AggregationException.cc \
		AggregationUtil.cc \
		ArrayAggregateOnOuterDimension.cc \
		ArrayAggregationBase.cc \
		ArrayJoinExistingAggregation.cc \
		AttributeElement.cc \
		DDSAccessInterface.cc \
		DDSLoader.cc \
		Dimension.cc \
		DimensionElement.cc \
		DirectoryUtil.cc \
		ExplicitElement.cc \
		GridAggregationBase.cc \
		GridAggregateOnOuterDimension.cc \
		GridJoinExistingAggregation.cc \
		MyBaseTypeFactory.cc \
		NCMLBaseArray.cc \
		NCMLElement.cc \
		NCMLModule.cc \
		NCMLParser.cc \
		NCMLRequestHandler.cc \
		NCMLResponseNames.cc \
		NCMLUtil.cc \
		NetcdfElement.cc \
		OtherXMLParser.cc \
		RCObject.cc \
		ReadMetadataElement.cc \
		RemoveElement.cc \
		RenamedArrayWrapper.cc \
		SaxParserWrapper.cc \
		SaxParser.cc \
		ScanElement.cc \
		ScopeStack.cc \
		Shape.cc \
		SimpleLocationParser.cc \
		SimpleTimeParser.cc \
		ValuesElement.cc \
		VariableAggElement.cc \
		VariableElement.cc \
		XMLHelpers.cc

# 		RCObjectInterface.cc


NCML_HDRS = \
		AggMemberDataset.h \
		AggMemberDatasetDDSWrapper.h \
		AggMemberDatasetSharedDDSWrapper.h \
		AggMemberDatasetUsingLocationRef.h \
		AggMemberDatasetWithDimensionCacheBase.h \
		AggMemberDatasetDimensionCache.h \
		AggregationElement.h \
		AggregationException.h \
		AggregationUtil.h \
		ArrayAggregateOnOuterDimension.h \
		ArrayAggregationBase.h \
		ArrayJoinExistingAggregation.h \
		AttributeElement.h \
		DDSAccessInterface.h \
		DDSLoader.h \
		Dimension.h \
		DimensionElement.h \
		DirectoryUtil.h \
		ExplicitElement.h \
		GridAggregationBase.h \
		GridAggregateOnOuterDimension.h \
		GridJoinExistingAggregation.h \
		MyBaseTypeFactory.h \
		NCMLArray.h \
		NCMLBaseArray.h \
		NCMLDebug.h \
		NCMLElement.h \
		NCMLModule.h \
		NCMLParser.h \
		NCMLRequestHandler.h \
		NCMLResponseNames.h \
		NCMLUtil.h \
		NetcdfElement.h \
		OtherXMLParser.h \
		RCObject.h \
		RCObjectInterface.h \
		ReadMetadataElement.h \
		RemoveElement.h \
		RenamedArrayWrapper.h \
		SaxParserWrapper.h \
		SaxParser.h \
		ScanElement.h \
		Shape.h \
		ScopeStack.h \
		SimpleLocationParser.h \
		SimpleTimeParser.h \
		ValuesElement.h \
		VariableAggElement.h \
		VariableElement.h \
		XMLHelpers.h

# ICU_CPPFLAGS and ICU_LIBS set from configure.ac check for ICU
libncml_module_la_SOURCES = $(NCML_SRCS) $(BES_SRCS) $(NCML_HDRS) $(BES_HDRS)
# libncml_module_la_CPPFLAGS = $(ICU_CPPFLAGS) $(BES_CPPFLAGS) $(DAP_CFLAGS) 
libncml_module_la_LDFLAGS = -avoid-version -module 
libncml_module_la_LIBADD = $(LIBADD)
#$(DAP_LIBS)

EXTRA_PROGRAMS = 

EXTRA_DIST = ncml.conf.in data 

CLEANFILES = *~ ncml.conf

# Sample data primaries for install
sample_datadir = 		$(datadir)/hyrax/data/ncml
sample_data_DATA = 		data/ncml/fnoc1_improved.ncml \
				data/ncml/fnoc1.nc \
				data/ncml/sample_virtual_dataset.ncml 

# Subdir of data for aggregation samples
sample_data_aggdir = 		$(datadir)/hyrax/data/ncml/agg
sample_data_agg_DATA = 		data/ncml/agg/joinNew_grid.ncml \
				data/ncml/agg/joinNew_scan.ncml \
				data/ncml/agg/joinNew_scan_dfm.ncml \
				data/ncml/agg/aggExisting.ncml

# Two subdirs of agg for the actual aggregation members
sample_data_agg_gridsdir = 	$(datadir)/hyrax/data/ncml/agg/grids
sample_data_agg_grids_DATA = 	data/ncml/agg/grids/f97182070958.hdf \
				data/ncml/agg/grids/f97182183448.hdf \
				data/ncml/agg/grids/f97183065853.hdf \
				data/ncml/agg/grids/f97183182355.hdf 

sample_data_agg_dateddir = 	$(datadir)/hyrax/data/ncml/agg/dated
sample_data_agg_dated_DATA = 	data/ncml/agg/dated/CG2006158_120000h_usfc.nc \
				data/ncml/agg/dated/CG2006158_130000h_usfc.nc \
				data/ncml/agg/dated/CG2006158_140000h_usfc.nc \
				data/ncml/agg/dated/CG2006158_150000h_usfc.nc 

sample_data_ncdir = 		$(datadir)/hyrax/data/nc
sample_data_nc_DATA =		data/nc/jan.nc data/nc/feb.nc

sample_data_dmrppdir = 		$(datadir)/hyrax/data/ncml/dmrpp
sample_data_dmrpp_DATA = 	data/ncml/dmrpp/chunked_twoD.h5


moduledir = $(sysconfdir)/bes/modules
module_DATA = ncml.conf

ncml.conf: ncml.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > ncml.conf

.PHONY: docs
docs:
	doxygen $(srcdir)/doxy.conf
	(cd docs && tar -czf html.tar.gz html)

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libncml_module_la_SOURCES)

