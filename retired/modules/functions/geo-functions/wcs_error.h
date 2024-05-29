/******************************************************************************
 * $Id: wcs_error.h 2011-07-19 16:24:00Z $
 *
 * Project:  The Open Geospatial Consortium (OGC) Web Coverage Service (WCS)
 * 			 for Earth Observation: Open Source Reference Implementation
 * Purpose:  WCS exception and error handler definition
 * Author:   Yuanzheng Shao, yshao3@gmu.edu
 *
 ******************************************************************************
 * Copyright (c) 2011, Liping Di <ldi@gmu.edu>, Yuanzheng Shao <yshao3@gmu.edu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

/************************************************************************
 WCS Exception Code

 See OGC Web Services Common Standard [OGC 06-121r9] for details
 Table 27 — Standard exception codes and meanings
	OperationNotSupported
	MissingParameterValue
	InvalidParameterValue
	VersionNegotiationFailed
	InvalidUpdateSequence
	OptionNotSupported
	NoApplicableCode

 See OGC® WCS 2.0 Interface Standard - Core [09-110r3] for details
 Table 18 - Exception codes for GetCoverage operation
	 NoSuchCoverage
	 InvalidAxisLabel
	 InvalidSubsetting

*************************************************************************/

#ifndef WCS_ERROR_H_
#define WCS_ERROR_H_

#include <cpl_error.h>
#include <string>

#define OGC_WCS_OperationNotSupported  		300
#define OGC_WCS_MissingParameterValue  		301
#define OGC_WCS_InvalidParameterValue  		302
#define OGC_WCS_VersionNegotiationFailed	303
#define OGC_WCS_InvalidUpdateSequence		304
#define OGC_WCS_OptionNotSupported			305
#define OGC_WCS_NoApplicableCode			306

#define OGC_WCS_NoSuchCoverage				307
#define OGC_WCS_InvalidAxisLabel			308
#define OGC_WCS_InvalidSubsetting			309

void CPL_DLL CPL_STDCALL WCS_Error(CPLErr, int, const char*, ...);
void CPL_DLL CPL_STDCALL SetWCS_ErrorLocator(const char* loc);
void CPL_DLL CPL_STDCALL WCS_ErrorHandler(CPLErr, int, const char*);
string CPL_DLL CPL_STDCALL GetWCS_ErrorMsg();

int	WCST_GetSoapMsgTrns();
void WCST_SetSoapMsgTrns(int);

#define WCS_Error  CPLError
#endif /* WCS_ERROR_H_ */
