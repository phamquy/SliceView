// XSliceDoc.cpp : implementation of the CXSliceDoc class
//

#include "stdafx.h"
#include "SliceView.h"
#include "XSliceDoc.h"
#include "RAWAccess.h"
#include "SVFileIO.h"
#include "XSliceView.h"
#include "ChildFrm.h"
#include "LutProc.h"
#include "Utility.h"

#include "DlgDcmBrowse.h"
#include "DcmMerger.h"

#include "ExportOpsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CXSliceDoc

IMPLEMENT_DYNCREATE(CXSliceDoc, CDocument)

BEGIN_MESSAGE_MAP(CXSliceDoc, CDocument)
END_MESSAGE_MAP()


// CXSliceDoc construction/destruction

CXSliceDoc::CXSliceDoc() 
{
	// TODO: add one-time construction code here
}

CXSliceDoc::~CXSliceDoc()
{
}





// CXSliceDoc serialization

void CXSliceDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CXSliceDoc diagnostics

#ifdef _DEBUG
void CXSliceDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CXSliceDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CXSliceDoc commands

//************************************
// Method:    Draw
// FullName:  CXSliceDoc::Draw
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CDC * pDC
// Purpose:   
//************************************
int CXSliceDoc::Draw(CDC* pDC)
{
	int retcode = SV_NORMAL;
	//Create view layout
	retcode = UpdateViewLayout();
	//Update frame for each slice object
	if(retcode == SV_NORMAL)
		retcode = UpdateSlicesFrame();

	if(retcode == SV_NORMAL)
		retcode = UpdateLineControl();

	if(retcode == SV_NORMAL) 
	{

		// Render Volume result [2/23/2009 QUYPS]
		m_objVolumeImg.Draw(pDC);

		m_objSlcAxial.Draw(pDC);
		m_objSlcCoronal.Draw(pDC);
		m_objSlcSagittal.Draw(pDC);
	}
	return retcode;
}


//************************************
// Method:    UpdateLuminance
// FullName:  CXSliceDoc::UpdateLuminance
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: LUT in_lu
// Purpose:	  Update brightness and contrast value, when 
//************************************
int CXSliceDoc::UpdateLuminance( LUT in_lu, BOOL in_flgForced )
{
	int retcode = SV_NORMAL;

	if ((in_flgForced)||(in_lu.nBirght != m_CurLut.nBirght)||(in_lu.nContrast != m_CurLut.nContrast))
	{
		m_CurLut = in_lu;		
		m_objLutProc.MakeFunctionTable(in_lu);		
	}

	retcode = m_objLutProc.ApplyProc(&m_objSlcAxial);
	if (retcode == SV_NORMAL)
	{
		retcode = m_objLutProc.ApplyProc(&m_objSlcCoronal);
	}

	if(retcode == SV_NORMAL)
	{
		retcode = m_objLutProc.ApplyProc(&m_objSlcSagittal);
	}
	return retcode;
}

//************************************
// Method:    UpdateViewLayout
// FullName:  CXSliceDoc::UpdateViewLayout
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: LAYOUT in_Layout
// Purpose:   
//************************************
int CXSliceDoc::UpdateViewLayout()
{
	POSITION pos = GetFirstViewPosition();
	CXSliceView* pView = (CXSliceView*)GetNextView(pos);
	CRect rcViewFrame;
	pView->GetClientRect(&rcViewFrame);
	CXSliceFrame* pFrame  = (CXSliceFrame*)pView->GetParentFrame();
	DISPLAYTINFO dInfo = pFrame->GetDisplayInfo();
	return m_objLayout.MakeLayout(rcViewFrame,m_objImgDataSet.m_tSize, &dInfo);	
}

//************************************
// Method:    UpdateSlices
// FullName:  CXSliceDoc::UpdateSlices
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: void
// Purpose:   
//************************************
int CXSliceDoc::UpdateSlices( VOLPOS in_3DPosition )
{
	INT retcode = SV_NORMAL;
	
	if(m_objSlcAxial.GetIndex() != in_3DPosition.nZ)
		retcode = m_objImgDataSet.GetSlice(&m_objSlcAxial, eAxial, in_3DPosition.nZ);
	
	if (retcode == SV_NORMAL)
	{
		if(m_objSlcCoronal.GetIndex() != in_3DPosition.nY)
			retcode = m_objImgDataSet.GetSlice(&m_objSlcCoronal, eCoronal, in_3DPosition.nY);
	}
	
	if (retcode == SV_NORMAL)
	{
		if(m_objSlcSagittal.GetIndex() != in_3DPosition.nX)
			retcode = m_objImgDataSet.GetSlice(&m_objSlcSagittal, eSagittal, in_3DPosition.nX);
	}


	//update current position
	if (retcode == SV_NORMAL)
	{
		retcode = UpdateLuminance(m_CurLut);
		m_Cur3Dpos = in_3DPosition;
	}
	return retcode;
}

//************************************
// Method:    UpdateSlices
// FullName:  CXSliceDoc::UpdateSlices
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: int in_nChange
// Parameter: CPoint in_MousePos
// Purpose:   
//************************************
int CXSliceDoc::UpdateSlices(int in_nChange, CPoint in_MousePos)
{
	int retcode = SV_NORMAL;
	CViewSliceObj* pActiveSlice = GetActiveSlice(in_MousePos);
	INT nCurIdx = pActiveSlice->GetIndex();
	INT nNewIdx = nCurIdx +in_nChange;
	if (nNewIdx < 0) nNewIdx = 0;
	VOLSIZE volSize = m_objImgDataSet.GetSize();
	VOLPOS  volPos = m_Cur3Dpos;

	if(pActiveSlice != NULL)
	{
		switch(pActiveSlice->m_nType)
		{
		case eAxial:
			if (nNewIdx >= volSize.ndz) nNewIdx= volSize.ndz - 1;
			volPos.nZ = nNewIdx;
			break;
		case eCoronal:
			if (nNewIdx >= volSize.ndy) nNewIdx= volSize.ndy - 1;
			volPos.nY = nNewIdx;
			break;
		case eSagittal:
			if (nNewIdx >= volSize.ndx) nNewIdx= volSize.ndx - 1;
			volPos.nX = nNewIdx;
		    break;
		default:
			retcode = SV_INVALID_PARAM;
		    break;
		}
	}

	if (retcode == SV_NORMAL)
	{
		retcode = UpdateSlices(volPos);
	}

	return retcode;
}

//************************************
// Method:    UpdateSlices
// FullName:  CXSliceDoc::UpdateSlices
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CPoint in_pt2Dpos
// Purpose:   
//************************************
int CXSliceDoc::UpdateSlices(CPoint in_pt2Dpos)
{
	INT retcode = SV_NORMAL;
	
	CViewSliceObj* pActiveSlice = GetActiveSlice(in_pt2Dpos);

	if(pActiveSlice != NULL)
	{
		VOLPOS volpos;
		CPoint localFramePos(0,0);
		CPoint ImgCoordPoint = pActiveSlice->ViewPos2ImagePos(in_pt2Dpos);
		switch(pActiveSlice->m_nType)
		{
		case eAxial:		//XY
			volpos.nX = ImgCoordPoint.x;
			volpos.nY = ImgCoordPoint.y;
			volpos.nZ = pActiveSlice->GetIndex();
			break;

		case eCoronal:		//XZ
			volpos.nX = ImgCoordPoint.x;
			volpos.nY = pActiveSlice->GetIndex();
			volpos.nZ = ImgCoordPoint.y;
			break;

		case eSagittal:		//YZ
			volpos.nX = pActiveSlice->GetIndex();
			volpos.nY = ImgCoordPoint.x;
			volpos.nZ = ImgCoordPoint.y;
			break;

		default:
			retcode = SV_INVALID_PARAM;
			break;
		}

		if (retcode == SV_NORMAL)
		{
			retcode = UpdateSlices(volpos);
		}
	}
	return retcode;
}

//************************************
// Method:    OnOpenDocument
// FullName:  CXSliceDoc::OnOpenDocument
// Access:    public 
// Returns:   BOOL
// Qualifier:
// Parameter: LPCTSTR lpszPathName
// Purpose:   
//************************************
BOOL CXSliceDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	BOOL flagRet = TRUE;
	int retcode = SV_NORMAL;

	//create File IO
	CSVFileIO oFile;
	retcode = oFile.Open(lpszPathName);
	if (retcode != SV_NORMAL)
	{
		CUtility::ReportError(retcode);
		return FALSE;
	}
	
	//////////////////////////////////////////////////////////////////////////
	// VOLUME TEST [4/17/2008 QUYPS]
	//read dataset
	retcode = oFile.ReadImageDataSet(&m_objImgDataSet);
	if(!retcode) retcode = m_objVolumeImg.PrepareMem(CSize(512, 512));
	//Prepare slice to store image
	m_objVolumeImg.m_nType = eVolume;
	//make header
	BITMAPINFOHEADER bmpInfo;
	memset(&bmpInfo, 0, sizeof(BITMAPINFOHEADER));
	bmpInfo.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.biWidth = 512;
	bmpInfo.biHeight = 512;
	bmpInfo.biPlanes = 1;	
	bmpInfo.biCompression = BI_RGB;
	bmpInfo.biBitCount = 24;	
	m_objVolumeImg.SetBitmapInfo(bmpInfo);
	m_objVolumeImg.SetIndex(0);
	if(!retcode)
	{
//		m_objVolumeImg.TurnOnLine(FALSE);
		retcode = m_objRenderer.InitRender(&m_objImgDataSet, FALSE, FALSE, TRUE);
		if(!retcode) retcode = m_objRenderer.Render(&m_objVolumeImg, bmpInfo.biWidth, bmpInfo.biHeight, m_XRotate,m_YRotate,m_ZRotate);
	}
	//////////////////////////////////////////////////////////////////////////

	//create slice objects
	if(retcode == SV_NORMAL)
	{
		//Set default 3dconsole position 
		m_Cur3Dpos.nX = m_objImgDataSet.m_tSize.ndx / 2;
		m_Cur3Dpos.nY = m_objImgDataSet.m_tSize.ndy / 2;
		m_Cur3Dpos.nZ = m_objImgDataSet.m_tSize.ndz / 2;
		retcode = m_objImgDataSet.GetSlice(&m_objSlcAxial, eAxial, m_Cur3Dpos.nZ);
	}
	if(retcode == SV_NORMAL)
		retcode = m_objImgDataSet.GetSlice(&m_objSlcCoronal, eCoronal, m_Cur3Dpos.nY);
	if(retcode == SV_NORMAL)
		retcode = m_objImgDataSet.GetSlice(&m_objSlcSagittal, eSagittal, m_Cur3Dpos.nX);

	//Update luminance
	if(retcode == SV_NORMAL)
	{
		LUT	lu;
		lu.nBirght = lu.nContrast = 0;
		retcode = UpdateLuminance(lu, TRUE);
	}	

	oFile.Close();
	if (retcode != SV_NORMAL)
	{
		CUtility::ReportError(retcode);
		flagRet = FALSE;
	}
	return flagRet;
}



//************************************
// Method:    OnNewDocument
// FullName:  CXSliceDoc::OnNewDocument
// Access:    public 
// Returns:   BOOL
// Qualifier:
// Purpose:   
//************************************
BOOL CXSliceDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	int retcode = SV_NORMAL;
	BOOL retflag = TRUE;

	CDcmMerger 	oDcmImporter;
	// TODO: add reinitialization code here

	

	CDlgDcmBrowse imPortDlg;

	// Collaboration Support Mode
	if( ((CSliceViewApp*)AfxGetApp())->m_bDownload )
	{
		imPortDlg.m_sFolderPath = ((CSliceViewApp*)AfxGetApp())->m_sFolderPath ;
		
	}


	CStringList sFilelist;
	if (imPortDlg.DoModal() == IDOK)
	{
		retcode = imPortDlg.GetFileList(&sFilelist);
	}

#ifdef _DEBUG //---[3/18/2008 QUYPS]---
	POSITION pos = NULL;
	for (INT i=0; i<sFilelist.GetCount(); i++)
	{
		pos = sFilelist.FindIndex(i);
		if (pos) TRACE(_T("\n%s"), sFilelist.GetAt(pos));		
	}
#endif //------------------------------

	
	if (retcode == SV_NORMAL)
		//read dataset
		retcode = oDcmImporter.Open(&sFilelist);
	
	if (retcode == SV_NORMAL)
		retcode = oDcmImporter.ReadImageDataSet(&m_objImgDataSet);
		
	//////////////////////////////////////////////////////////////////////////
	if(retcode == SV_NORMAL) 
		retcode = m_objVolumeImg.PrepareMem(CSize(m_objImgDataSet.m_tSize.ndx, m_objImgDataSet.m_tSize.ndy));
	//Prepare slice to store image
	m_objVolumeImg.m_nType = eVolume;
	//make header
	BITMAPINFOHEADER bmpInfo;
	memset(&bmpInfo, 0, sizeof(BITMAPINFOHEADER));
	bmpInfo.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.biWidth = m_objImgDataSet.m_tSize.ndx;
	bmpInfo.biHeight = m_objImgDataSet.m_tSize.ndy;
	bmpInfo.biPlanes = 1;	
	bmpInfo.biCompression = BI_RGB;
	bmpInfo.biBitCount = 24;
	bmpInfo.biSizeImage = bmpInfo.biWidth * bmpInfo.biHeight * 3;
	m_objVolumeImg.SetBitmapInfo(bmpInfo);
	m_objVolumeImg.SetIndex(0);
	if(retcode == SV_NORMAL)
	{
//		m_objVolumeImg.TurnOnLine(FALSE);		
		retcode = m_objRenderer.InitRender(&m_objImgDataSet, FALSE, FALSE, TRUE);
		if(retcode == SV_NORMAL) retcode = m_objRenderer.Render(&m_objVolumeImg, bmpInfo.biWidth, bmpInfo.biHeight, m_XRotate,m_YRotate,m_ZRotate);
	}
	//////////////////////////////////////////////////////////////////////////

	//create slice objects
	if(retcode == SV_NORMAL)
	{
		//Set default 3dconsole position 
		m_Cur3Dpos.nX = m_objImgDataSet.m_tSize.ndx / 2;
		m_Cur3Dpos.nY = m_objImgDataSet.m_tSize.ndy / 2;
		m_Cur3Dpos.nZ = m_objImgDataSet.m_tSize.ndz / 2;
		retcode = m_objImgDataSet.GetSlice(&m_objSlcAxial, eAxial, m_Cur3Dpos.nZ);
	}
	if(retcode == SV_NORMAL)
		retcode = m_objImgDataSet.GetSlice(&m_objSlcCoronal, eCoronal, m_Cur3Dpos.nY);
	if(retcode == SV_NORMAL)
		retcode = m_objImgDataSet.GetSlice(&m_objSlcSagittal, eSagittal, m_Cur3Dpos.nX);

	//Update luminance
	if(retcode == SV_NORMAL)
	{
		LUT	lu;
		lu.nBirght = lu.nContrast = 0;
		retcode = UpdateLuminance(lu, TRUE);
	}	
	
	oDcmImporter.Close();
	if (retcode != SV_NORMAL) retflag = FALSE;
	
	return retflag;
}

//************************************
// Method:    UpdateSlicesFrame
// FullName:  CXSliceDoc::UpdateSlicesFrame
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: void
// Purpose:   Make or update frame for each slice object
//************************************
int CXSliceDoc::UpdateSlicesFrame(void)
{
	int retcode = SV_NORMAL;
	//Each layout type has a different way to assign slices object
	//to a view rectangle on the the view
	switch(m_objLayout.GetLayoutType())
	{

	case SV_LO_CUBE:
		// VOlume render [2/23/2009 QUYPS]
		retcode = m_objVolumeImg.UpdateFrame(m_objLayout.GetViewRect(eFrame1));
		if(!retcode) retcode = m_objSlcAxial.UpdateFrame(m_objLayout.GetViewRect(eFrame2));
		if(!retcode) retcode = m_objSlcCoronal.UpdateFrame(m_objLayout.GetViewRect(eFrame4));
		if(!retcode) retcode = m_objSlcSagittal.UpdateFrame(m_objLayout.GetViewRect(eFrame3));
		break;

	case SV_LO_HORZ:
		retcode = m_objSlcAxial.UpdateFrame(m_objLayout.GetViewRect(eFrame1));
		if(!retcode) retcode = m_objSlcCoronal.UpdateFrame(m_objLayout.GetViewRect(eFrame2));
		if(!retcode) retcode = m_objSlcSagittal.UpdateFrame(m_objLayout.GetViewRect(eFrame3));
		// VOlume render [2/23/2009 QUYPS]
		if(!retcode) retcode = m_objVolumeImg.UpdateFrame(m_objLayout.GetViewRect(eFrame4));
		break;
	case SV_LO_VERT:
		retcode = m_objSlcAxial.UpdateFrame(m_objLayout.GetViewRect(eFrame3));
		if(!retcode) retcode = m_objSlcCoronal.UpdateFrame(m_objLayout.GetViewRect(eFrame2));
		if(!retcode) retcode = m_objSlcSagittal.UpdateFrame(m_objLayout.GetViewRect(eFrame1));
		// VOlume render [2/23/2009 QUYPS]
		if(!retcode) retcode = m_objSlcAxial.UpdateFrame(m_objLayout.GetViewRect(eFrame4));
	    break;
	default:
		retcode = SV_INVALID_PARAM;
	    break;
	}
	return retcode;
}
//************************************
// Method:    UpdateLineControl
// FullName:  CXSliceDoc::UpdateLineControl
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: VOLPOS in_3DPOS
// Purpose:   
//************************************
int CXSliceDoc::UpdateLineControl()
{	
	m_objSlcAxial.UpdateLineCtrl(m_Cur3Dpos);
	m_objSlcCoronal.UpdateLineCtrl(m_Cur3Dpos);
	m_objSlcSagittal.UpdateLineCtrl(m_Cur3Dpos);
	return 0;
}

//************************************
// Method:    GetActiveSlice
// FullName:  CXSliceDoc::GetActiveSlice
// Access:    public 
// Returns:   CSliceObj*
// Qualifier:
// Parameter: CPoint in_MousePos
// Purpose:   
//************************************
CViewSliceObj* CXSliceDoc::GetActiveSlice(CPoint in_MousePos)
{
	CViewSliceObj* pSlc = NULL;
	if (m_objSlcAxial.IsOnMouse(in_MousePos))
	{
		pSlc = &m_objSlcAxial;
	}
	else if(m_objSlcCoronal.IsOnMouse(in_MousePos))
	{
		pSlc = &m_objSlcCoronal;
	}
	else if (m_objSlcSagittal.IsOnMouse(in_MousePos))
	{
		pSlc = &m_objSlcSagittal;
	}
	// VOLUME MOUSE HANDLE [3/5/2009 QUYPS]
	// Add mouse handle volume function [3/5/2009 QUYPS]
	else if (m_objVolumeImg.IsOnMouse(in_MousePos))
	{
		pSlc = &m_objVolumeImg;
	}
	return pSlc;
}


//////////////////////////////////////////////////////////////////////////
// ADDED [2/2/2009 QUYPS]
//EXPORT DATASET
/*
if the user option is raw
Write out the file name

if the user option is TIFF or other multi-frame format
Iterator over the picture frame
Create the image for each each frame 
Add to the TIFF format		
*/
//************************************
// Method:    OnExportDocument
// FullName:  CXSliceDoc::OnExportDocument
// Access:    public 
// Returns:   BOOL
// Qualifier:
// Parameter: void
//************************************
BOOL CXSliceDoc::OnExportDocument(void)
{
	BOOL retflag = TRUE;
	int retcode = SV_NORMAL;


//////////////////////////////////////////////////////////////////////////
// TEMPORARY CODE [1/23/2009 QUYPS]
	CString strOutputFile;
	CString strOutputPath;
	strOutputFile.Format(_T("%s_%dx%dx%d"), SV_EXP_DEFAULTNAME, m_objImgDataSet.m_tSize.ndx, m_objImgDataSet.m_tSize.ndy, m_objImgDataSet.m_tSize.ndz);
	TCHAR szFilters[]= _T("Raw volume (*.raw)|*.raw|")
					   _T("Multiframe Tiff (*.tif;*.tiff)|*.tif;*.tiff||");					   
	CFileDialog dlgSave(FALSE, _T(".raw"), strOutputFile, OFN_OVERWRITEPROMPT, szFilters);
	
	if(dlgSave.DoModal() == IDOK)
	{
		strOutputPath = dlgSave.GetPathName();
		CSVFileIO oFile;
		retcode = oFile.Open(strOutputPath, eWrite);
		if (retcode != SV_NORMAL)
		{
			CUtility::ReportError(retcode);
			retflag = FALSE;
		}
		else
		{
			retcode = oFile.ExportImageDataSet(&m_objImgDataSet);
			retflag = (retcode == SV_NORMAL)?TRUE:FALSE;
		}

		retcode = oFile.Close();
		retflag = (retcode == SV_NORMAL)?TRUE:FALSE;
	}

/*
// TEST
	HANDLE hFile = CreateFile( strOutputFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		AfxMessageBox(_T("Couldn't create the file!"));
	}
	else
	{
		// Attach a CFile object to the handle we have.
		CFile myFile(hFile);

		UINT nLength = m_objImgDataSet.GetSize().ndx*m_objImgDataSet.GetSize().ndy*m_objImgDataSet.GetSize().ndz;

		// write string
		myFile.Write(m_objImgDataSet.GetDataBuff(), nLength);

		myFile.Close();
	}
*/
	
	return retflag;
}

//////////////////////////////////////////////////////////////////////////
// VOLUME TEST [4/17/2008 QUYPS]
int CXSliceDoc::ReRenderVol( CDC* pDC )
{
	int retcode = SV_NORMAL;

	CSize  outSize = m_objVolumeImg.GetSize();
	switch(m_RotateAxis)
	{
	case 0:
		retcode = m_objRenderer.Render(&m_objVolumeImg, outSize.cx, outSize.cy, m_XRotate,0,0);
		break;

	case 1:
		retcode = m_objRenderer.Render(&m_objVolumeImg, outSize.cx, outSize.cy, 0,m_YRotate,0);
		break;

	default:
		break;
	}

	if (retcode == SV_NORMAL)
	{
		m_objVolumeImg.Draw(pDC);
	}
	return retcode;

}
