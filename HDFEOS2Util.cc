/////////////////////////////////////////////////////////////////////////////
// Helper functions for handling dimension maps and clear memories
//
//  Authors:   MuQun Yang <myang6@hdfgroup.org> Eunsoo Seo
// Copyright (c) 2010 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include "HDFEOS2Util.h"

void
HDFEOS2Util::ClearMem (int32 * offset32, int32 * count32, int32 * step32,
					   int *offset, int *count, int *step)
{
	delete[]offset32;
	delete[]count32;
	delete[]step32;
	delete[]offset;
	delete[]count;
	delete[]step;
}
