/******************************************************************************
 * $Id: wcs_error.cpp 2011-07-19 16:24:00Z $
 *
 * Project:  The Open Geospatial Consortium (OGC) Web Coverage Service (WCS)
 * 			 for Earth Observation: Open Source Reference Implementation
 * Purpose:  WCS exception and error handler implementation, used to set
 * 			 the exception code and locator of WCS
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

#include "wcs_error.h"

static string WCS_ERROR_STRING="";
static string ERROR_LOCATOR="";
static string ExcptCodeText[] =
{
	"OperationNotSupported",
	"MissingParameterValue",
	"InvalidParameterValue",
	"VersionNegotiationFailed",
	"InvalidUpdateSequence",
	"OptionNotSupported",
	"NoApplicableCode",
	"NoSuchCoverage",
	"InvalidAxisLabel",
	"InvalidSubsetting"
};

static int WCS_IsSoapMessage_Transform = 0;

string CPL_STDCALL GetWCS_ErrorMsg()
{
	return WCS_ERROR_STRING;
}

void CPL_STDCALL SetWCS_ErrorLocator(const char* loc)
{
	ERROR_LOCATOR = loc;
}

void CPL_STDCALL WCS_ErrorHandler(CPLErr eErrClass,int err_no,const char *pszErrorMsg )
{
	WCS_ERROR_STRING.clear();
	string ExcCode;

	if(err_no >= 300 && err_no <= 309)
		ExcCode = ExcptCodeText[err_no - 300];
	else
		ExcCode = ExcptCodeText[6];

	if(WCS_IsSoapMessage_Transform)
	{
		WCS_ERROR_STRING  = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		WCS_ERROR_STRING += "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">\n";
		WCS_ERROR_STRING += "<soap:Body>\n";
		WCS_ERROR_STRING += "<ExceptionReport xmlns=\"http://www.opengis.net/ows\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n";
		WCS_ERROR_STRING += "xsi:schemaLocation=\"http://www.opengis.net/ows/owsCommon.xsd\" version=\"0.3.20\" language=\"en\">\n";
		WCS_ERROR_STRING += "	<Exception exceptionCode=\"";
		WCS_ERROR_STRING += ExcCode;
		WCS_ERROR_STRING += "\" locator=\"";
		WCS_ERROR_STRING += ERROR_LOCATOR;
		WCS_ERROR_STRING += "\">\n";
		WCS_ERROR_STRING += "		<ExceptionText>" + string(pszErrorMsg) + "</ExceptionText>\n";
		WCS_ERROR_STRING += "	</Exception>\n";
		WCS_ERROR_STRING += "</ExceptionReport>\n";
		WCS_ERROR_STRING += "/soap:Body>\n";
		WCS_ERROR_STRING += "/<soap:Envelope>\n";
	}
	else
	{
		WCS_ERROR_STRING  = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		WCS_ERROR_STRING += "<ExceptionReport xmlns=\"http://www.opengis.net/ows\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n";
		WCS_ERROR_STRING += "xsi:schemaLocation=\"http://www.opengis.net/ows/owsCommon.xsd\" version=\"0.3.20\" language=\"en\">\n";
		WCS_ERROR_STRING += "	<Exception exceptionCode=\"";
		WCS_ERROR_STRING += ExcCode;
		WCS_ERROR_STRING += "\" locator=\"";
		WCS_ERROR_STRING += ERROR_LOCATOR;
		WCS_ERROR_STRING += "\">\n";
		WCS_ERROR_STRING += "		<ExceptionText>" + string(pszErrorMsg) + "</ExceptionText>\n";
		WCS_ERROR_STRING += "	</Exception>\n";
		WCS_ERROR_STRING += "</ExceptionReport>\n";
	}

	return;
}

void WCST_SetSoapMsgTrns(int isSoap)
{
	WCS_IsSoapMessage_Transform = isSoap;
}

int	WCST_GetSoapMsgTrns()
{
	return WCS_IsSoapMessage_Transform;
}
