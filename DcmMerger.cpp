#include "StdAfx.h"
#include "DcmMerger.h"
#include "Utility.h"

CDcmMerger::CDcmMerger(void)
{
}

CDcmMerger::~CDcmMerger(void)
{


}

//************************************
// Method:    ReadImageDataSet
// FullName:  CDcmMerger::ReadImageDataSet
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CImageDataSet * out_pImgDataSet
// Purpose:   
//************************************
int CDcmMerger::ReadImageDataSet( CImageDataSet* out_pImgDataSet )
{
	//check validation of param
	ASSERT(out_pImgDataSet != NULL);

	if (out_pImgDataSet == NULL)return SV_MEMORY_ERR;
	if ((m_ImageList.GetCount() <= 0) || (m_FileList.GetCount() <= 0)) 
		return SV_FILEIO_ERROR;
	
	int retcode = SV_NORMAL;
	
	//Get size of frame
	DicomImage* pDcmImage = NULL;  
	POSITION pos = NULL;
	
	INT nWidth = 0;
	INT nHeight = 0;
	INT nNumberOfFrame = 0;
	ULONG nSizeOfDataSet = 0;
	UINT nSizeOfFrame = 0;

	

	pos = m_ImageList.FindIndex(0);
	pDcmImage = m_ImageList.GetAt(pos);
	nWidth = pDcmImage->getWidth();
	nHeight = pDcmImage->getHeight();

	//Count number of frame and check size of all frame.
	for (INT i=0; i< m_ImageList.GetCount(); i++)
	{
		pos = NULL;
		pos = m_ImageList.FindIndex(i);
		if(pos != NULL) 
		{
			pDcmImage = m_ImageList.GetAt(pos);
			if ((nWidth != pDcmImage->getWidth())||
				(nHeight != pDcmImage->getHeight()))
			{
				retcode = SV_UNSUPPORT_FORMAT;
			}
			else
			{
				nNumberOfFrame += pDcmImage->getFrameCount();
			}
		}
		else
		{
			retcode = SV_SYSTEM_ERR;
		}

		if (retcode != SV_NORMAL) break;
	}

	//if all frame have the same size -> allocate memory for the dataset
	if (retcode == SV_NORMAL)
	{
		out_pImgDataSet->m_tSize.ndx = nWidth;
		out_pImgDataSet->m_tSize.ndy = nHeight;
		out_pImgDataSet->m_tSize.ndz = nNumberOfFrame;

		nSizeOfDataSet = out_pImgDataSet->m_tSize.ndx * out_pImgDataSet->m_tSize.ndy * out_pImgDataSet->m_tSize.ndz;
		nSizeOfFrame = pDcmImage->getOutputDataSize(SV_DCM_OUTPUTBITS);
		ASSERT((nSizeOfFrame *  nNumberOfFrame) == nSizeOfDataSet);	

		retcode = out_pImgDataSet->PrepareBuffer(nSizeOfDataSet);
	}

	//Copy data from DICOM file to the dataset
	uchar* pBuff = out_pImgDataSet->GetDataBuff();
	POSITION pos1 = NULL;
	for (INT i=0; i< m_ImageList.GetCount(); i++)
	{
		pos1 = NULL;
		pos1 = m_ImageList.FindIndex(i);
		if(pos1 != NULL) 
		{
			pDcmImage = m_ImageList.GetAt(pos1);
			for (INT j=0; j < pDcmImage->getFrameCount(); j++)			
			{
				BOOL sts = pDcmImage->getOutputData(pBuff, nSizeOfFrame,SV_DCM_OUTPUTBITS,j);
				if (!sts)
					retcode = SV_FILEIO_ERROR;
				else
					pBuff += nSizeOfFrame;

				if (retcode != SV_NORMAL) break;				
			}			
		}
		else retcode = SV_SYSTEM_ERR;		

		if (retcode != SV_NORMAL) break;
	}

	return retcode;
}

//************************************
// Method:    ReadPatientInfo
// FullName:  CDcmMerger::ReadPatientInfo
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CPatientInfo * out_pPatInfo
// Purpose:   
//************************************
int CDcmMerger::ReadPatientInfo( CPatientInfo* out_pPatInfo )
{
	int retcode = SV_NORMAL;
	
	return retcode;
}



//************************************
// Method:    Open
// FullName:  CDcmMerger::Open
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CStringList * in_pFileList
// Purpose:   
//************************************
int CDcmMerger::Open(CStringList* in_pFileList)
{
	int retcode = SV_NORMAL;

	for (INT i = 0; i< in_pFileList->GetCount(); i++)
	{
		POSITION pos = NULL;
		pos = in_pFileList->FindIndex(i);
		if(pos != NULL)
		{
			CString sFileName = in_pFileList->GetAt(pos);
			char* pzTempName;

			//Convert to ANSI code
			if(CUtility::UnicodeToAnsi(sFileName,&pzTempName) != ERROR_SUCCESS) 
				retcode = SV_SYSTEM_ERR;

			//Create DICOM file access object
			DcmFileFormat* pDcmFile = NULL;
			DicomImage* pDcmImage = NULL;

			//*********Create file object 
			pDcmFile = new DcmFileFormat();					
			if (pDcmFile == NULL) retcode = SV_MEMORY_ERR;
			//Load file
			E_TransferSyntax xfer;
			if(retcode == SV_NORMAL)
			{
				OFCondition cond = pDcmFile->loadFile(pzTempName, 
													  EXS_Unknown, 
													  EGL_withoutGL, 
													  DCM_MaxReadLength,
													  ERM_autoDetect);
				if (cond.bad()) retcode = SV_FILEIO_ERROR;
				else xfer = pDcmFile->getDataset()->getOriginalXfer();				
			}

			OFCmdUnsignedInt    opt_frame = 0;                    //default: start from first frame
			OFCmdUnsignedInt    opt_frameCount = 0;               //default: process all frame

			if(retcode == SV_NORMAL)
			{
				//*********Create image object
				pDcmImage = new DicomImage(pDcmFile, xfer, CIF_AcrNemaCompatibility, opt_frame, opt_frameCount);

				if (pDcmImage == NULL) 
					retcode = SV_MEMORY_ERR;
				else if (pDcmImage->getStatus() != EIS_Normal)
					retcode = SV_UNSUPPORT_FORMAT;			

			}

			if(retcode == SV_NORMAL)
			{
				// VOI LUT processing [1/31/2009 QUYPS]
				pDcmImage->setMinMaxWindow();

				//Add to list
				m_FileList.AddTail(pDcmFile);
				m_ImageList.AddTail(pDcmImage);
			}
			else
			{
				delete pDcmFile;
				delete pDcmImage;
			}
			//Release memory
			delete[] pzTempName;			
		}
		else
		{
			retcode = SV_SYSTEM_ERR;
		}

		//if error occur, escape from loop
		if (retcode != SV_NORMAL) break;		
	}
	return retcode;
}


int CDcmMerger::Close( void )
{
	//release all object pointed by pointers in lists
	for (INT i=0; i<m_ImageList.GetCount(); i++)
	{
		POSITION pos = NULL;
		pos = m_ImageList.FindIndex(i);
		DicomImage* pImage = m_ImageList.GetAt(pos);
		delete pImage;
	}
	m_ImageList.RemoveAll();

	for (INT j=0; j<m_FileList.GetCount(); j++)
	{
		POSITION pos = NULL;
		pos = m_FileList.FindIndex(j);
		DcmFileFormat* pFile = m_FileList.GetAt(pos);
		delete pFile;
	}	
	m_FileList.RemoveAll();

	return SV_NORMAL;
}