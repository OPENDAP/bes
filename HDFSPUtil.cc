#include "HDFSPUtil.h"

void
HDFSPUtil::ClearMem (int32 * offset32, int32 * count32, int32 * step32,
					 int *offset, int *count, int *step)
{
	delete[]offset32;
	delete[]count32;
	delete[]step32;
	delete[]offset;
	delete[]count;
	delete[]step;
}

void
HDFSPUtil::ClearMem2 (int32 * offset32, int32 * count32, int32 * step32)
{
	delete[]offset32;
	delete[]count32;
	delete[]step32;
}

void
HDFSPUtil::ClearMem3 (int *offset, int *count, int *step)
{
	delete[]offset;
	delete[]count;
	delete[]step;
}
