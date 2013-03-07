// ***************************************************************
//  DCMAccess   version:  1.0   ¡¤  date: 02/08/2008
//  -------------------------------------------------------------
//  Implement for DCMAccess class 
//  -------------------------------------------------------------
//  Copyright (C) 2008 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include "StdAfx.h"
#include "DCMAccess.h"
#include "Utility.h"



CDCMAccess::CDCMAccess(void)
{
	m_pDcmFile = NULL;
	m_pDCMImage = NULL;
}

CDCMAccess::~CDCMAccess(void)
{
	if(m_pDCMImage != NULL)
	{
		delete m_pDCMImage;
		m_pDCMImage = NULL;
	}
}

// Read patient information from current opened DCM file
//************************************
// Method:    ReadPatientInfo
// FullName:  CDCMAccess::ReadPatientInfo
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CPatientInfo * out_pPatInfo
//************************************
int CDCMAccess::ReadPatientInfo(CPatientInfo* out_pPatInfo)
{
		
	return 0;
}

//************************************
// Method:    ReadImageDataSet
// FullName:  CDCMAccess::ReadImageDataSet
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CImageDataSet * out_pImgDataSet
//************************************
int CDCMAccess::ReadImageDataSet(CImageDataSet* out_pImgDataSet)
{
	//check validation of param
	ASSERT(out_pImgDataSet != NULL);
	ASSERT(m_pDCMImage != NULL);
	if ((out_pImgDataSet == NULL) || (m_pDCMImage == NULL))return SV_MEMORY_ERR;


	int retcode = SV_NORMAL;
	int nNumberOfFrame =  m_pDCMImage->getFrameCount();
	out_pImgDataSet->m_tSize.ndx =  m_pDCMImage->getWidth();		//width of image
	out_pImgDataSet->m_tSize.ndy =  m_pDCMImage->getHeight();		//height of image
	out_pImgDataSet->m_tSize.ndz =  nNumberOfFrame;					//get number of image (Z value of volume)

	UINT sizeOfFrame = m_pDCMImage->getOutputDataSize(SV_DCM_OUTPUTBITS);
	ULONG sizeOfDataSet = sizeOfFrame *  nNumberOfFrame;

	ASSERT(sizeOfDataSet == (out_pImgDataSet->m_tSize.ndx * out_pImgDataSet->m_tSize.ndy * out_pImgDataSet->m_tSize.ndz));
		
	if(sizeOfDataSet > SV_VOL_MAXSIZE) retcode =  SV_SIZE_OVERLOAD;

	if(retcode == SV_NORMAL)
	{
		out_pImgDataSet->PrepareBuffer(sizeOfDataSet);
		uchar* pBuff = out_pImgDataSet->GetDataBuff();

		for (INT i=0; i<nNumberOfFrame; i++)
		{

			BOOL sts = m_pDCMImage->getOutputData(pBuff, sizeOfFrame, SV_DCM_OUTPUTBITS, i);
			if (!sts)
			{
				retcode = SV_FILEIO_ERROR;
				break;
			}
			else
			{
				pBuff += sizeOfFrame;
			}
		}
	}
	return retcode;
}

//************************************
// Method:    Open
// FullName:  CDCMAccess::Open
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: WCHAR * in_szFilePath
//************************************
int CDCMAccess::Open( CString in_szFilePath )
{
	int retCode = SV_NORMAL; 
	m_szFilePath = in_szFilePath;
	m_pDcmFile = new DcmFileFormat();
	char* pSZ;
	if (CUtility::UnicodeToAnsi(m_szFilePath, &pSZ) != ERROR_SUCCESS)
	{
		retCode = SV_SYSTEM_ERR;
	}
	E_TransferSyntax xfer;
	if (retCode == SV_NORMAL)
	{
		OFCondition cond = m_pDcmFile->loadFile(pSZ,EXS_Unknown,
			EGL_withoutGL,DCM_MaxReadLength,ERM_autoDetect);
		if(cond.bad())
		{		
			retCode = SV_FILEIO_ERROR;
		}
		else
		{
			xfer = m_pDcmFile->getDataset()->getOriginalXfer();
		}		
	}

	//unsigned long opt_compatibilityMode = CIF_MayDetachPixelData | CIF_TakeOverExternalDataset;
	OFCmdUnsignedInt    opt_frame = 0;                    //default: start from first frame
	OFCmdUnsignedInt    opt_frameCount = 0;               //default: process all frame

	if(retCode == SV_NORMAL)
	{	
		//Create Image object
		m_pDCMImage = new DicomImage(m_pDcmFile, xfer, CIF_AcrNemaCompatibility, opt_frame, opt_frameCount);
		if (m_pDCMImage == NULL)
		{		
			retCode = SV_MEMORY_ERR;
		}
	}

	if(retCode == SV_NORMAL)
	{		
		
		if (m_pDCMImage->getStatus() != EIS_Normal)
		{
			retCode = SV_UNSUPPORT_FORMAT;			
		}
		// Adding VOI LUT process [1/31/2009 QUYPS]
		else
		{
			m_pDCMImage->setMinMaxWindow();
		}
	}

#ifdef _DEBUG
	BOOL res;
	if (retCode == SV_NORMAL)
	{		
		res = m_pDCMImage->writeBMP("D:/2.Projects/SliceView/~Temp/temp_dcm.bmp",24,20);		
	}
#endif

	delete[] pSZ;
	return retCode;
}

//************************************
// Method:    Close
// FullName:  CDCMAccess::Close
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: void
//************************************
int CDCMAccess::Close(void)
{
	
	delete m_pDCMImage;
	m_pDCMImage = NULL;
	delete m_pDcmFile;
	m_pDcmFile = NULL;
	m_szFilePath = "";
	return SV_NORMAL;
}
