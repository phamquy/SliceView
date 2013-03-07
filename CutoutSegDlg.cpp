// CutoutSegDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SliceView.h"
#include "CutoutSegDlg.h"
#include "Common.h"

#include <stack> 

#include "Utility.h"
#include "SegmentExport.h"
#include "ImgLib.h"


#include <GdiPlus.h>
using namespace Gdiplus;
// CCutoutSegDlg dialog

IMPLEMENT_DYNAMIC(CCutoutSegDlg, CDialog)

CCutoutSegDlg::CCutoutSegDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCutoutSegDlg::IDD, pParent)	
	, m_bEnablePreprocess(FALSE)
	, m_bUseDefaultThreshold(FALSE)	
	, m_pDataSet(NULL)
{

}

//************************************
// Method:    CCutoutSegDlg
// FullName:  CCutoutSegDlg::CCutoutSegDlg
// Access:    public 
// Returns:   
// Qualifier:
// Parameter: CImageDataSet * in_pData : volume data set
// Parameter: CWnd * pParent
//************************************
CCutoutSegDlg::CCutoutSegDlg(CImageDataSet* in_pData, CWnd* pParent /*= NULL*/ )
	: CDialog(CCutoutSegDlg::IDD, pParent)
	, m_bEnablePreprocess(FALSE)
	, m_bUseDefaultThreshold(TRUE)
	, m_pDataSet(in_pData)
{		

	FillMemory(&bmInfo, sizeof(BITMAPINFOHEADER), 0x0);
	bmInfo.biSize = sizeof(BITMAPINFOHEADER);
	bmInfo.biWidth = m_pDataSet->GetSize().ndx;
	bmInfo.biHeight = m_pDataSet->GetSize().ndy;
	bmInfo.biPlanes = 1;	
	bmInfo.biCompression = BI_RGB;
	bmInfo.biBitCount = 24;
	bmInfo.biSizeImage = bmInfo.biWidth * bmInfo.biHeight * 3;

	m_pSourceByteImg = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy];
	m_pSourceImg = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy * 3];
	m_pResultByteImg = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy];
	m_pResultImg = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy * 3];

#ifdef _DEBUG
	m_pDebugBinByte = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy];
	m_pDebugBinImage = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy *3];
#endif

}

CCutoutSegDlg::~CCutoutSegDlg()
{
	delete[] m_pSourceByteImg;
	delete[] m_pResultByteImg;
	delete[] m_pResultImg;
	delete[] m_pSourceImg;

#ifdef _DEBUG
	delete[] m_pDebugBinImage;
	delete[] m_pDebugBinByte;
#endif
}

void CCutoutSegDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);	
	DDX_Control(pDX, IDC_SEG_BIRGHT, m_ctlBrightSlider);
	DDX_Control(pDX, IDC_SEG_CONSTRAST, m_ctlContrastSlider);
	DDX_Control(pDX, IDC_SEG_THRESHOLD, m_ctlThresholdSlider);
	DDX_Control(pDX, IDC_SEG_SLICEPOS, m_ctlSlicePos);
	DDX_Check(pDX, IDC_SEG_PREPROC, m_bEnablePreprocess);
	DDX_Check(pDX, IDC_SEG_BINDEFAULT, m_bUseDefaultThreshold);
}


BEGIN_MESSAGE_MAP(CCutoutSegDlg, CDialog)	
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_SEG_PREPROC, &CCutoutSegDlg::OnBnClickedSegPreproc)
	ON_BN_CLICKED(IDC_SEG_BINDEFAULT, &CCutoutSegDlg::OnBnClickedSegBindefault)
	ON_BN_CLICKED(IDC_CMD_SAVESLICE, &CCutoutSegDlg::OnBnClickedCmdSaveslice)
	ON_BN_CLICKED(IDC_CMD_SAVEVOL, &CCutoutSegDlg::OnBnClickedCmdSavevol)
	ON_WM_PAINT()	
	
	ON_BN_CLICKED(IDC_CMD_SAVEALLSLICES, &CCutoutSegDlg::OnBnClickedCmdSaveallslices)
	ON_BN_CLICKED(IDC_CMD_RG3D, &CCutoutSegDlg::OnBnClickedCmdRg3d)
END_MESSAGE_MAP()


// CCutoutSegDlg message handlers

//************************************
// Method:    OnVScroll
// FullName:  CCutoutSegDlg::OnVScroll
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: UINT nSBCode
// Parameter: UINT nPos
// Parameter: CScrollBar * pScrollBar
//************************************
void CCutoutSegDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default

	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
	UpdateSourceSlice();
	DisplayImage();
}

//************************************
// Method:    OnHScroll
// FullName:  CCutoutSegDlg::OnHScroll
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: UINT nSBCode
// Parameter: UINT nPos
// Parameter: CScrollBar * pScrollBar
//************************************
void CCutoutSegDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
	UpdatePreproc();
	
	if (!m_bUseDefaultThreshold && (m_ctlThresholdSlider.GetPos() != m_nThresholdVal))
	{
		m_nThresholdVal = m_ctlThresholdSlider.GetPos();
		UpdateSourceSlice();
	}
	DisplayImage();
}

//************************************
// Method:    OnInitDialog
// FullName:  CCutoutSegDlg::OnInitDialog
// Access:    public 
// Returns:   BOOL
// Qualifier:
//************************************
BOOL CCutoutSegDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	ASSERT(m_pDataSet != NULL);
	
	//Init for the slider controls
	m_ctlThresholdSlider.SetRange(0, SV_GRAYLEVELS-1, TRUE); 
	m_ctlThresholdSlider.SetPos(SV_SEG_DEF_THRESHOLD);


	m_ctlContrastSlider.SetRange(SV_CONS_MIN, SV_CONS_MAX, TRUE);
	m_ctlContrastSlider.SetPos(SV_CONS_DEFAULT);
	m_CurLut.nContrast = SV_CONS_DEFAULT;

	m_ctlBrightSlider.SetRange(SV_BRIG_MIN,SV_BRIG_MAX, TRUE);
	m_ctlBrightSlider.SetPos(SV_BRIG_DEFAULT);
	m_CurLut.nBirght= SV_BRIG_DEFAULT;

	m_ctlSlicePos.SetRange(0, m_pDataSet->GetSize().ndz-1, TRUE);
	m_ctlSlicePos.SetPos(m_pDataSet->GetSize().ndz/2);

	//No preprocessing
	m_bEnablePreprocess = FALSE;
	m_ctlBrightSlider.EnableWindow(m_bEnablePreprocess);
	m_ctlContrastSlider.EnableWindow(m_bEnablePreprocess);

	//Use default threshold
	m_bUseDefaultThreshold = TRUE;
	m_nThresholdVal = SV_SEG_DEF_THRESHOLD;
	m_ctlThresholdSlider.EnableWindow(!m_bUseDefaultThreshold);
	

	// TEST [4/6/2009 QUYPS]
	((CButton* )GetDlgItem(IDC_CMD_RG3D))->ShowWindow(SW_HIDE);

	UpdateSourceSlice();
	UpdatePreproc();
	return TRUE;  
}

//************************************
// Method:    OnBnClickedSegPreproc
// FullName:  CCutoutSegDlg::OnBnClickedSegPreproc
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void CCutoutSegDlg::OnBnClickedSegPreproc()
{
	// TODO: Add your control notification handler code here
	m_bEnablePreprocess = !m_bEnablePreprocess;
	m_ctlBrightSlider.EnableWindow(m_bEnablePreprocess);
	m_ctlContrastSlider.EnableWindow(m_bEnablePreprocess);
}

//************************************
// Method:    OnBnClickedSegBindefault
// FullName:  CCutoutSegDlg::OnBnClickedSegBindefault
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void CCutoutSegDlg::OnBnClickedSegBindefault()
{
	// TODO: Add your control notification handler code here
	m_bUseDefaultThreshold = !m_bUseDefaultThreshold;
	if(m_bUseDefaultThreshold ) 
		m_nThresholdVal = SV_SEG_DEF_THRESHOLD;
	else 
		m_nThresholdVal = m_ctlThresholdSlider.GetPos();
	m_ctlThresholdSlider.EnableWindow(!m_bUseDefaultThreshold);
	UpdateSourceSlice();
	DisplayImage();
}

//************************************
// Method:    OnBnClickedCmdSaveslice
// FullName:  CCutoutSegDlg::OnBnClickedCmdSaveslice
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void CCutoutSegDlg::OnBnClickedCmdSaveslice()
{
	// TODO: Add your control notification handler code here
	BITMAPINFO bm;
	memset(&bm, 0, sizeof(BITMAPINFO));
	bm.bmiHeader = bmInfo;
	Bitmap* bmp = Bitmap::FromBITMAPINFO(&bm,m_pResultImg);
	
	CString strOutputFile;	
	strOutputFile.Format(_T("%s_%dx%d"), SV_EXP_DEFAULTNAME, m_pDataSet->GetSize().ndx, m_pDataSet->GetSize().ndx);
	CString strFilters = _T("Raw files (*.raw)|*.raw|Bitmap file (*.bmp)|*.bmp|JPEG file (*.jpg)|*.jpg||");
	CFileDialog dlgSave(FALSE, _T(".raw"), strOutputFile, OFN_OVERWRITEPROMPT, strFilters);

	CFile oFile;
	if (dlgSave.DoModal() == IDOK)
	{
		CLSID pngClsid;	
		CString ext = dlgSave.GetFileExt();
		if( ext == "raw")
		{			
			CFileException ex;
			BOOL bflagOpen = oFile.Open(dlgSave.GetPathName(), CFile::modeWrite | CFile::modeCreate, &ex);
			if(bflagOpen)
			{
				UINT nLength = m_pDataSet->GetSize().ndx*m_pDataSet->GetSize().ndy;
				oFile.Write(m_pResultByteImg, nLength);	
				oFile.Flush();
				oFile.Close();
			}	
		}
		else if(ext == "bmp")
		{
			CUtility::GetEncoderClsid(L"image/bmp", &pngClsid);
			bmp->Save(dlgSave.GetPathName(), &pngClsid, NULL);
		}
		else if(ext == "jpg")
		{
			CUtility::GetEncoderClsid(L"image/jpeg", &pngClsid);
			bmp->Save(dlgSave.GetPathName(), &pngClsid, NULL);			
		}
	}
}


//************************************
// Method:    OnBnClickedCmdSaveallslices
// FullName:  CCutoutSegDlg::OnBnClickedCmdSaveallslices
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void CCutoutSegDlg::OnBnClickedCmdSaveallslices()
{
	// User prompt to get the information: directory, file extension
	CSegmentExportDlg dlgSave;
	CString sDirPath = _T(".");
	CString sFileExt;
	CString strOutputFile;	
	strOutputFile.Format(_T("%s_%dx%d"),SV_EXP_DEFAULTNAME, m_pDataSet->GetSize().ndx, m_pDataSet->GetSize().ndx);

	CSize sliceSize(m_pDataSet->GetSize().ndx,m_pDataSet->GetSize().ndy);
	UCHAR* pBuffer = new UCHAR[sliceSize.cx * sliceSize.cy * 3];
	UCHAR* pByteBuffer = new UCHAR[sliceSize.cx * sliceSize.cy];
	if (dlgSave.DoModal() == IDOK)
	{
		sDirPath = dlgSave.m_sDirPath;
		sFileExt = dlgSave.m_sExt;
		// TODO: Add your control notification handler code here
		for (int iSlice=0; iSlice < m_pDataSet->GetSize().ndz; iSlice++)
		{
			//Get Raw Slice
			GetRawSliceFromDataSet(iSlice);

			//Cut it out
			CutOut(m_pSourceByteImg, pByteBuffer, sliceSize.cx, sliceSize.cy);
	

			//Save it
			CString numbers;
			numbers.Format(_T("_%.3d"), iSlice);
			CString FullPath = sDirPath + strOutputFile + numbers + _T(".")+ sFileExt;

			for (int i=0; i<sliceSize.cx * sliceSize.cy; i++)
			{			
				memset(pBuffer + i*3, pByteBuffer[i], 3);
			}

			BITMAPINFO bm;
			memset(&bm, 0, sizeof(BITMAPINFO));
			bm.bmiHeader = bmInfo;
			Bitmap* bmp = Bitmap::FromBITMAPINFO(&bm,pBuffer);

			CLSID pngClsid;	
			if( sFileExt == "raw")
			{			
				CFile oFile;
				CFileException ex;
				BOOL bflagOpen = oFile.Open(FullPath, CFile::modeWrite | CFile::modeCreate, &ex);
				if(bflagOpen)
				{
					UINT nLength = m_pDataSet->GetSize().ndx*m_pDataSet->GetSize().ndy;
					oFile.Write(pByteBuffer, nLength);		
					oFile.Flush();
					oFile.Close();
				}	
			}
			else if(sFileExt == "bmp")
			{
				CUtility::GetEncoderClsid(L"image/bmp", &pngClsid);
				bmp->Save(FullPath, &pngClsid, NULL);
			}
			else if(sFileExt == "jpg")
			{
				CUtility::GetEncoderClsid(L"image/jpeg", &pngClsid);
				bmp->Save(FullPath, &pngClsid, NULL);			
			}
		}
	}

	delete[] pBuffer;
	delete[] pByteBuffer;

	//Re update m_pSourceByteImg slice back to current slice
	UpdateSourceSlice();
	
}


//************************************
// Method:    OnBnClickedCmdSavevol
// FullName:  CCutoutSegDlg::OnBnClickedCmdSavevol
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void CCutoutSegDlg::OnBnClickedCmdSavevol()
{
	CString strOutputFile;	
	strOutputFile.Format(_T("%s_%dx%dx%d"), SV_EXP_DEFAULTNAME, m_pDataSet->GetSize().ndx, m_pDataSet->GetSize().ndx,m_pDataSet->GetSize().ndz);
	CString strFilters = _T("Raw files (*.raw)| *.raw||");
	CFileDialog dlgSave(FALSE, _T(".raw"), strOutputFile, OFN_OVERWRITEPROMPT, strFilters);
	UCHAR* pByteBuffer = new UCHAR[m_pDataSet->GetSize().ndx * m_pDataSet->GetSize().ndy];
	CFile oFile;	
	CFileException ex;

	if (dlgSave.DoModal() == IDOK)
	{
		UINT nLength = m_pDataSet->GetSize().ndx*m_pDataSet->GetSize().ndy;
		CString path = dlgSave.GetPathName();
		BOOL bflagOpen = oFile.Open(dlgSave.GetPathName(), CFile::modeWrite | CFile::modeCreate, &ex);
		if(bflagOpen)
		{
			//for (int iSlice=m_pDataSet->GetSize().ndz-1 ; iSlice >=0 ; iSlice--)
			for (int iSlice=0; iSlice < m_pDataSet->GetSize().ndz; iSlice++)
			{
				//Get Raw Slice
				GetRawSliceFromDataSet(iSlice);

				//Cut it out
				CSize sliceSize(m_pDataSet->GetSize().ndx,m_pDataSet->GetSize().ndy);
				CutOut(m_pSourceByteImg, pByteBuffer, sliceSize.cx, sliceSize.cy);

				//Write to file
				oFile.Write(pByteBuffer, nLength);
			}
			oFile.Flush();			
		}		
		oFile.Close();
	}
	delete[] pByteBuffer;
	//Re update m_pSourceByteImg slice back to current slice
	UpdateSourceSlice();
}


//************************************
// Method:    OnPaint
// FullName:  CCutoutSegDlg::OnPaint
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void CCutoutSegDlg::OnPaint()
{
	CPaintDC dc(this); 
	DisplayImage();
	
}

//************************************
// Method:    DisplayImage
// FullName:  CCutoutSegDlg::DisplayImage
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void CCutoutSegDlg::DisplayImage()
{
	CDC* pDC = GetDC();
	HDC hDC = pDC->GetSafeHdc();
	int errCode = 0;


	BITMAPINFO bm;
	memset(&bm, 0, sizeof(BITMAPINFO));
	bm.bmiHeader = bmInfo;
	Graphics graph(*pDC);

	if (m_pSourceImg != NULL)
	{		
		Bitmap* pBmp = Bitmap::FromBITMAPINFO(&bm,m_pSourceImg);
		graph.DrawImage(pBmp, 34, 44, 188, 188);		
		delete pBmp;
	}

	if(m_pResultImg)
	{			
		Bitmap* pBmp = Bitmap::FromBITMAPINFO(&bm,m_pResultImg);
		graph.DrawImage(pBmp, 254, 44, 188, 188);		
		delete pBmp;
	}

#ifdef _DEBUG
	if(m_pDebugBinImage)
	{			
		Bitmap* pBmp = Bitmap::FromBITMAPINFO(&bm,m_pDebugBinImage);
		graph.DrawImage(pBmp, 474, 44, 188, 188);
		delete pBmp;
	}
#endif	

	CString str;
	str.Format(_T("Slice No.%.3d"),m_ctlSlicePos.GetPos());
	// Initialize arguments.
	Font myFont(_T("Tahoma"), 8,FontStyleBold);
	PointF origin(34.0f, 44.0f);
	SolidBrush blackBrush(Color(255, 255, 255));
	
		// Draw string.
	graph.DrawString(
		str,
		-1,
		&myFont,
		origin,
		&blackBrush);	
}

//************************************
// Method:    UpdateSourceSlice
// FullName:  CCutoutSegDlg::UpdateSourceSlice
// Access:    private 
// Returns:   int
// Qualifier:
// Parameter: void
//************************************
int CCutoutSegDlg::UpdateSourceSlice(void)
{	
	ASSERT(m_pDataSet != NULL);	
	ASSERT(m_pResultByteImg != NULL);
	ASSERT(m_pSourceByteImg != NULL);

	
	UCHAR* pVolume = m_pDataSet->GetDataBuff();
	CSize sliceSize(m_pDataSet->GetSize().ndx,m_pDataSet->GetSize().ndy);
	int nSlicePos = m_ctlSlicePos.GetPos();
	UCHAR* pCurrentSlice = pVolume + sliceSize.cx * sliceSize.cy * nSlicePos;

	//Invert the line scan order
// 	for (int row = 0; row < sliceSize.cy;row ++)
// 	{
// 		CopyMemory(m_pSourceByteImg + (sliceSize.cy-row-1)*sliceSize.cx, pCurrentSlice + row * sliceSize.cx, sliceSize.cx);
// 	}

	CopyMemory(m_pSourceByteImg,pCurrentSlice, sliceSize.cx* sliceSize.cy);

	CutOut(m_pSourceByteImg, m_pResultByteImg, sliceSize.cx, sliceSize.cy);
	for (int line=0; line < sliceSize.cy; line++)
	{
		for(int pix=0; pix < sliceSize.cx; pix++)
		{
			memset(m_pSourceImg + (sliceSize.cx*line + pix)*3, m_pSourceByteImg[(sliceSize.cy-line-1)*sliceSize.cx + pix], 3);		
			memset(m_pResultImg + (sliceSize.cx*line + pix)*3, m_pResultByteImg[(sliceSize.cy-line-1)*sliceSize.cx + pix], 3);
		}
	}

#ifdef _DEBUG
	for (int line=0; line < sliceSize.cy; line++)
	{
		for(int pix=0; pix < sliceSize.cx; pix++)
		{
			memset(m_pDebugBinImage + (sliceSize.cx*line + pix)*3, m_pDebugBinByte[(sliceSize.cy-line-1)*sliceSize.cx + pix], 3);					
		}
	}
#endif
	return 0;
}


//************************************
// Method:    GetRawSliceFromDataSet
// FullName:  CCutoutSegDlg::GetRawSliceFromDataSet
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: int nPos
//************************************
void CCutoutSegDlg::GetRawSliceFromDataSet( int nPos )
{
	UCHAR* pVolume = m_pDataSet->GetDataBuff();
	CSize sliceSize(m_pDataSet->GetSize().ndx,m_pDataSet->GetSize().ndy);
	int nSlicePos = m_ctlSlicePos.GetPos();
	UCHAR* pCurrentSlice = pVolume + sliceSize.cx * sliceSize.cy * nPos;
	//Invert the line scan order
	for (int row = 0; row < sliceSize.cy;row ++)
	{
		CopyMemory(m_pSourceByteImg + (sliceSize.cy-row-1)*sliceSize.cx, pCurrentSlice + row * sliceSize.cx, sliceSize.cx);
	}
}

//************************************
// Method:    UpdatePreproc
// FullName:  CCutoutSegDlg::UpdatePreproc
// Access:    private 
// Returns:   int
// Qualifier:
// Parameter: void
//************************************
int CCutoutSegDlg::UpdatePreproc( void )
{
	int retcode = SV_NORMAL;

	if ((m_ctlBrightSlider.GetPos() != m_CurLut.nBirght)||(m_ctlContrastSlider.GetPos() != m_CurLut.nContrast))
	{
		m_CurLut.nBirght = m_ctlBrightSlider.GetPos();		
		m_CurLut.nContrast = m_ctlContrastSlider.GetPos();
		m_objLutProc.MakeFunctionTable(m_CurLut);		
	}

	return retcode;
}

//************************************
// Method:    CutOut
// FullName:  CCutoutSegDlg::CutOut
// Access:    private 
// Returns:   int
// Qualifier:
// Parameter: UCHAR * pSource
// Parameter: UCHAR * pResult
// Parameter: int width
// Parameter: int height
//************************************

int CCutoutSegDlg::CutOut( UCHAR* pSource, UCHAR* pResult, int width, int height )
{

	ASSERT(pSource != NULL);
	ASSERT(pResult != NULL);

	int memSize = width* height;
	UCHAR* pBufferImg1 = new UCHAR[memSize];
	UCHAR* pBufferImg2 = new UCHAR[memSize];
	
//***********************************************************************
// ORIGINAL ALGORITHM 

	// Binarize the image save to buffer 1
	CImgLib::Binary(pSource, pBufferImg1, width, height, m_nThresholdVal);

// #ifdef _DEBUG
// 	CopyMemory(m_pDebugBinByte, pBufferImg1, width*height);
// #endif

	// Detect the boundary save to buffer 2
	CImgLib::BinBoundaryDetect(pBufferImg1, pBufferImg2, width, height);

#ifdef _DEBUG
	CopyMemory(m_pDebugBinByte, pBufferImg2, width*height);
#endif

	CImgLib::Mask(pSource, pResult, pBufferImg2, width, height, SV_MEANNING_MARK);
/***********************************************************************/

/***********************************************************************
// REGION GROWING ALGORITHM
	
	// Binarize the image save to buffer 1
	CImgLib::Binary(pSource, pBufferImg1, width, height, m_nThresholdVal);
	#ifdef _DEBUG
	CopyMemory(m_pDebugBinByte, pBufferImg2, width*height);
	#endif
	CImgLib::BinRGBoundary(pBufferImg1, pBufferImg2, width, height);
	
	CImgLib::Mask(pSource, pResult,pBufferImg2, width, height);
/***********************************************************************/

/**********************************************************************
// MODIFIED ORIGINAL ALGORIHM
	// TEST [4/3/2009 QUYPS]
	// Re apply the algorithm but with different direction
	CImgLib::Binary(pSource, pBufferImg1, width, height, m_nThresholdVal);

#ifdef _DEBUG
	CopyMemory(m_pDebugBinByte, pBufferImg2, width*height);
#endif

	CImgLib::BinBoundaryDetect(pBufferImg1, pBufferImg2, width, height);

	// First time result to buffer 1
	CImgLib::Mask(pSource, pBufferImg1, pBufferImg2, width, height);
	
	// Vertically invert image and copy to buffer 2	and result
	//CImgLib::Invert(pBufferImg1, pBufferImg2, width, height, SV_INV_ROW);
	//CImgLib::Invert(pBufferImg1, pResult, width, height, SV_INV_ROW);

	// Diagonal invert
	CImgLib::Invert(pBufferImg1, pBufferImg2, width, height, SV_INV_DIAG);
	CImgLib::Invert(pBufferImg1, pResult, width, height, SV_INV_DIAG);


	// Binarize the image, save to buffer1
	CImgLib::Binary(pBufferImg2, pBufferImg1, width, height, m_nThresholdVal);
	// Detect the boundary save to buffer2
	CImgLib::BinBoundaryDetect(pBufferImg1, pBufferImg2, width, height);

	//Inverted result save to buffer 1	
	CImgLib::Mask(pResult, pBufferImg1, pBufferImg2, width, height);

	//Final image and copy to result
	//CImgLib::Invert(pBufferImg1, pResult, width, height, SV_INV_ROW);
	CImgLib::Invert(pBufferImg1, pResult, width, height, SV_INV_DIAG);


/*******************************************************************************/

	delete [] pBufferImg1;
	delete [] pBufferImg2;
	return 0;
}


void CCutoutSegDlg::OnBnClickedCmdRg3d()
{
#ifdef _DEBUG
	UCHAR* pVol = m_pDataSet->GetDataBuff();
	VOLSIZE volSize = m_pDataSet->GetSize();
	VOLPOS pos;
	pos.nX = 256;
	pos.nY = 256;
	pos.nZ = 25;
	UCHAR* pTemp = NULL;
	pTemp = (UCHAR*)malloc(volSize.ndx* volSize.ndy * volSize.ndz);
	if(pTemp)
	{
		CopyMemory(pTemp, pVol, volSize.ndx* volSize.ndy * volSize.ndz);
		CImgLib::Binary(pTemp, volSize.ndx* volSize.ndy * volSize.ndz, m_nThresholdVal);
		CImgLib::RG3D(pTemp, volSize, pos);	
		CImgLib::Mask(pVol,pTemp,volSize.ndx* volSize.ndy * volSize.ndz);
		// TODO: Add your control notification handler code here
	}

	free(pTemp);
#endif // _DEBUG
}
