#include "StdAfx.h"
#include "TiffFileExporter.h"
#include "Utility.h"
#include "gdiplus.h"
using namespace Gdiplus;


CTiffFileExporter::CTiffFileExporter(void)
{
}

CTiffFileExporter::~CTiffFileExporter(void)
{
}


//************************************
// Method:    Open
// FullName:  CTiffFileExporter::Open
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CString in_szFilePath
//************************************
int CTiffFileExporter::Open( CString in_szFilePath )
{
	int retcode = SV_NORMAL;
	m_szFilePath = in_szFilePath;
	return retcode;
}

//************************************
// Method:    ExportImageDataSet
// FullName:  CTiffFileExporter::ExportImageDataSet
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CImageDataSet * in_pImgDataSet
//************************************
int CTiffFileExporter::ExportImageDataSet( CImageDataSet* in_pImgDataSet )
{
	int retcode = SV_NORMAL;

	//Get the first slice (First XY plane)
	CViewSliceObj objSlice;
	in_pImgDataSet->GetSlice(&objSlice, eAxial, 0);
	BITMAPINFO bmpInfo;
	memset(&bmpInfo, 0, sizeof(BITMAPINFO));

	bmpInfo.bmiHeader = objSlice.GetInfoHeader();
	UCHAR* pBmpData = objSlice.GetOrgBuffer();

	Bitmap* outputTiff;
	outputTiff = new Bitmap(&bmpInfo, (void*)pBmpData);
	if(outputTiff == NULL)	retcode = SV_MEMORY_ERR;

	//Save the first slice to the file
	EncoderParameters encoderParameters;
	ULONG             parameterValue;
	Status            stat;

	// Get the CLSID of the TIFF encoder.
	CLSID encoderClsid;
	if(retcode == SV_NORMAL) 
	{
		CUtility::GetEncoderClsid(L"image/tiff", &encoderClsid);
	
		// Initialize the one EncoderParameter object.
		encoderParameters.Count = 1;
		encoderParameters.Parameter[0].Guid = EncoderSaveFlag;
		encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
		encoderParameters.Parameter[0].NumberOfValues = 1;
		encoderParameters.Parameter[0].Value = &parameterValue;

		parameterValue = EncoderValueMultiFrame;
		stat = outputTiff->Save(m_szFilePath,  &encoderClsid, &encoderParameters);
		if (stat != Ok) retcode = SV_SYSTEM_ERR;
	}

	//Loop to create the subsequent pages
	if(retcode == SV_NORMAL)	
	{
		//loop and save the subsequent slice		
		INT nNumberOfSlice = in_pImgDataSet->GetSize().ndz;
		parameterValue = EncoderValueFrameDimensionPage;
		for (int i=1; i<nNumberOfSlice; i++)
		{
			in_pImgDataSet->GetSlice(&objSlice, eAxial, i);
			bmpInfo.bmiHeader = objSlice.GetInfoHeader();
			pBmpData = objSlice.GetOrgBuffer();
			Bitmap* page = new Bitmap(&bmpInfo, (void*)pBmpData);
			if (page == NULL) 
			{
				retcode = SV_SYSTEM_ERR;				
			}
			else
			{
				stat = outputTiff->SaveAdd(page, &encoderParameters);
				if (stat != Ok) retcode = SV_SYSTEM_ERR;
				delete page;
			}
			if (retcode != SV_NORMAL) break;
		}		
	}

	parameterValue = EncoderValueFlush;
	stat = outputTiff->SaveAdd(&encoderParameters);
	if(stat != Ok) retcode = SV_SYSTEM_ERR;

	delete outputTiff;
	return retcode;
}

//************************************
// Method:    Close
// FullName:  CTiffFileExporter::Close
// Access:    public 
// Returns:   int
// Qualifier:
//************************************
int CTiffFileExporter::Close()
{
	return SV_NORMAL;
}