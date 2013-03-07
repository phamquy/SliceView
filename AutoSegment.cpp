// AutoSegment.cpp : implementation file
//

#include "stdafx.h"
#include "SliceView.h"
#include "AutoSegment.h"
#include "ImgLib.h"
#include "Utility.h"
#include "SegmentExport.h"
// CAutoSegment dialog

IMPLEMENT_DYNAMIC(CAutoSegment, CDialog)
CAutoSegment::CAutoSegment(CWnd* pParent /*=NULL*/)
: CDialog(CAutoSegment::IDD, pParent)
{

}
CAutoSegment::CAutoSegment(CImageDataSet* in_pData, CWnd* pParent /*=NULL*/)
	: CDialog(CAutoSegment::IDD, pParent),
		m_pDataSet(in_pData), 
		m_bDefaultThres(FALSE)
		, m_nMode(0)
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
		FillMemory(m_pSourceByteImg,in_pData->GetSize().ndx * in_pData->GetSize().ndy,0);

		m_pResByteImg = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy];
		FillMemory(m_pResByteImg,in_pData->GetSize().ndx * in_pData->GetSize().ndy,0);

		m_pSourceImg = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy * 3];
		FillMemory(m_pSourceImg,in_pData->GetSize().ndx * in_pData->GetSize().ndy *3, 0);

		m_pResultImg = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy * 3];
		FillMemory(m_pResultImg,in_pData->GetSize().ndx * in_pData->GetSize().ndy * 3, 0);

		m_pCuttingMask = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy];
		FillMemory(m_pSourceByteImg,in_pData->GetSize().ndx * in_pData->GetSize().ndy,0);

		m_bInitDone = FALSE;
		m_ResRegion = CRect(0,0,0,0);
		m_OrgRegion = CRect(0,0,0,0);

		// Init look up table
		for (int i=0; i<SV_GRAYLEVELS; i++)
		{
			m_nLUT[i] = i;
		}
}

CAutoSegment::~CAutoSegment()
{
	delete[] m_pSourceByteImg;
	delete[] m_pResultImg;
	delete[] m_pSourceImg;
	delete[] m_pCuttingMask;
	delete[] m_pResByteImg;
}

void CAutoSegment::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_AU_SLD_BRIGHT, m_sldBright);
	DDX_Control(pDX, IDC_AU_SLD_CONTRAST, m_sldContrast);
	DDX_Control(pDX, IDC_AU_SLD_SLICE, m_sldSliceNo);
	DDX_Control(pDX, IDC_AU_SLD_THRES, m_sldThreshold);
	DDX_Control(pDX, IDC_AU_CHK_PREPROC, m_chkPreproc);
	DDX_Control(pDX, IDC_AU_CHK_DEF, m_chkDefaultThres);	
	DDX_Check(pDX, IDC_AU_CHK_DEF, m_bDefaultThres);
	DDX_Control(pDX, IDC_AU_CB_CMODE, m_cbbCutMethod);
	DDX_Radio(pDX, IDC_AU_RAD_SEG, m_nMode);
}


BEGIN_MESSAGE_MAP(CAutoSegment, CDialog)
	ON_BN_CLICKED(IDC_AU_CHK_DEF, &CAutoSegment::OnBnClickedAuChkDef)
	ON_BN_CLICKED(IDC_AU_CHK_PREPROC, &CAutoSegment::OnBnClickedAuChkPreproc)
	ON_BN_CLICKED(IDC_AU_CMD_SAVEALL, &CAutoSegment::OnBnClickedAuCmdSaveall)
	ON_BN_CLICKED(IDC_AU_CMD_SAVESINGLE, &CAutoSegment::OnBnClickedAuCmdSavesingle)
	ON_BN_CLICKED(IDC_AU_CMD_SAVEVOL, &CAutoSegment::OnBnClickedAuCmdSavevol)
	ON_BN_CLICKED(IDC_AU_RAD_CUT, &CAutoSegment::OnBnClickedRad)
	ON_BN_CLICKED(IDC_AU_RAD_SEG, &CAutoSegment::OnBnClickedRad)
	ON_WM_HSCROLL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_PAINT()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_AU_CB_CMODE, &CAutoSegment::OnCbnSelchangeAuCbCmode)
END_MESSAGE_MAP()



void CAutoSegment::DisplayImage( void )
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
		graph.DrawImage(pBmp,m_OrgRegion.left, m_OrgRegion.top, m_OrgRegion.Width(), m_OrgRegion.Height());		
		delete pBmp;
	}

	if(m_pResultImg)
	{			
		Bitmap* pBmp = Bitmap::FromBITMAPINFO(&bm,m_pResultImg);
		graph.DrawImage(pBmp,m_ResRegion.left, m_ResRegion.top, m_ResRegion.Width(), m_ResRegion.Height());		
		delete pBmp;
	}

	CString str;
	str.Format(_T("Slice No.%.3d"),m_sldSliceNo.GetPos());
	// Initialize arguments.
	Font myFont(_T("Tahoma"), 8,FontStyleBold);
	PointF origin(m_OrgRegion.left, m_OrgRegion.top);
	SolidBrush whiteBrush(Color(255, 255, 255));
	// Draw string.
	graph.DrawString(
		str,
		-1,
		&myFont,
		origin,
		&whiteBrush);	

	graph.SetClip(Rect(m_OrgRegion.TopLeft().x,m_OrgRegion.TopLeft().y, m_OrgRegion.Width(), m_OrgRegion.Height()));
	if(m_oCutter.GetCutStatus() != SV_CUT_NOTSTARTED)
		m_oCutter.DrawResult(&graph);
}



void CAutoSegment::UpdateSlices( void )
{
	ASSERT(m_pDataSet != NULL);	
	//ASSERT(m_pResultByteImg != NULL);
	ASSERT(m_pSourceByteImg != NULL);

	UCHAR* pVolume = m_pDataSet->GetDataBuff();
	CSize sliceSize(m_pDataSet->GetSize().ndx,m_pDataSet->GetSize().ndy);
	int nSlicePos = m_sldSliceNo.GetPos();
	UCHAR* pCurrentSlice = pVolume + sliceSize.cx * sliceSize.cy * nSlicePos;

	// preprocessing
	if(m_chkPreproc.GetCheck() == BST_CHECKED)
		CImgLib::GrayAdjust(pCurrentSlice, m_pSourceByteImg, m_nLUT, sliceSize.cx, sliceSize.cy);
	else
		CopyMemory(m_pSourceByteImg,pCurrentSlice, sliceSize.cx* sliceSize.cy);

	// Make source image
	for (int line=0; line < sliceSize.cy; line++)
		for (int pix=0; pix < sliceSize.cx; pix++)
			memset(m_pSourceImg + (sliceSize.cx*line + pix)*3, m_pSourceByteImg[(sliceSize.cy-line-1)*sliceSize.cx + pix], 3);			


	AutoSegment();

	for (int line=0; line < sliceSize.cy; line++)
		for (int pix=0; pix < sliceSize.cx; pix++)
			memset(m_pResultImg + (sliceSize.cx*line + pix)*3, m_pResByteImg[(sliceSize.cy-line-1)*sliceSize.cx + pix], 3);

}

//////////////////////////////////////////////////////////////////////////
#define SV_AU_SPACING					10
#define SV_AU_SMALL_SPACING				0
#define SV_AU_GRP_HEIGHT				200
#define SV_AU_GRP_WIDTH					200
#define SV_AU_TEXTHEIGHT				18
#define SV_AU_SMALLSLIDERHEIGHT			20
#define SV_AU_SMALLSLIDERWIDTH			160

#define SV_AU_LARGESLIDERHEIGHT			30
#define SV_AU_BUTTON_WIDTH				120
#define SV_AU_BUTTON_HEIGHT				25
#define SV_AU_DLGINITWIDTH				880
#define SV_AU_DLGINITHEIGHT				650

#define SV_AU_CHARWIDTH					10

#define SV_AU_MIN						0
#define SV_AU_MAX						255	
#define SV_AU_DEF						128

void CAutoSegment::UpdateLayout( int cx, int cy )
{
	// Compute layout for the window and the image displaying region
	CStatic*	ctrlStatic = NULL;
	CButton*	ctrlButton = NULL;
	CComboBox*	ctrlCombobox = NULL;
	CRect clientRect;
	CRect contrlRect;
	CRect infoRect;
	GetClientRect(&clientRect);

	// Compute the preprocessing group
	contrlRect.left = clientRect.left + SV_AU_SPACING;
	contrlRect.bottom = clientRect.bottom - SV_AU_SPACING;
	contrlRect.top = clientRect.bottom - SV_AU_GRP_HEIGHT;
	contrlRect.right = contrlRect.left + SV_AU_GRP_WIDTH;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_AU_GRAYGRP);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Preprocessing check box
	contrlRect.left += SV_AU_SPACING;
	contrlRect.top += SV_AU_SPACING * 2;
	contrlRect.right -= SV_AU_SPACING;
	contrlRect.bottom = contrlRect.top + SV_AU_TEXTHEIGHT;
	m_chkPreproc.MoveWindow(&contrlRect, FALSE);

	// Brightness label
	contrlRect.top = contrlRect.bottom + SV_AU_SPACING;
	contrlRect.bottom = contrlRect.top + SV_AU_TEXTHEIGHT;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_AU_LBL_BRIGHT);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);


	//Brightness slider
	contrlRect.top = contrlRect.bottom + SV_AU_SMALL_SPACING;
	contrlRect.bottom = contrlRect.top + SV_AU_SMALLSLIDERHEIGHT;
	contrlRect.right = contrlRect.left + SV_AU_CHARWIDTH;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_BR_100);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	contrlRect.left = contrlRect.right + SV_AU_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_AU_SMALLSLIDERWIDTH;
	m_sldBright.MoveWindow(&contrlRect, FALSE);

	contrlRect.left = contrlRect.right + SV_AU_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_AU_CHARWIDTH;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_BR_M100);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Contrast label
	contrlRect.top = contrlRect.bottom + SV_AU_SPACING;	
	contrlRect.bottom = contrlRect.top + SV_AU_TEXTHEIGHT;
	contrlRect.left = clientRect.left + SV_AU_SPACING *2;
	contrlRect.right = clientRect.left + SV_AU_SMALLSLIDERWIDTH;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_AU_LBL_CONTRAST);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Contrast slider
	contrlRect.top = contrlRect.bottom + SV_AU_SMALL_SPACING;
	contrlRect.bottom = contrlRect.top + SV_AU_SMALLSLIDERHEIGHT;
	contrlRect.right = contrlRect.left + SV_AU_CHARWIDTH;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_CS_100);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	contrlRect.left = contrlRect.right + SV_AU_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_AU_SMALLSLIDERWIDTH;
	m_sldContrast.MoveWindow(&contrlRect, FALSE);

	contrlRect.left = contrlRect.right + SV_AU_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_AU_CHARWIDTH;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_CS_M100);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Compute the region growing setting
	contrlRect.left = clientRect.left + SV_AU_SPACING*1.5 + SV_AU_GRP_WIDTH;
	contrlRect.bottom = clientRect.bottom - SV_AU_SPACING;
	contrlRect.top = clientRect.bottom - SV_AU_GRP_HEIGHT;
	contrlRect.right = contrlRect.left + SV_AU_GRP_WIDTH;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_AU_SETTINGGRP);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Use default check box
	contrlRect.left += SV_AU_SPACING;
	contrlRect.top += SV_AU_SPACING * 2;
	contrlRect.right -= SV_AU_SPACING;
	contrlRect.bottom = contrlRect.top + SV_AU_TEXTHEIGHT;
	m_chkDefaultThres.MoveWindow(&contrlRect, FALSE);

	// Threshold slider label
	contrlRect.top = contrlRect.bottom + SV_AU_SPACING;
	contrlRect.bottom = contrlRect.top + SV_AU_TEXTHEIGHT;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_AU_LBL_THRES);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Threshold  slider
	contrlRect.top = contrlRect.bottom + SV_AU_SMALL_SPACING;
	contrlRect.bottom = contrlRect.top + SV_AU_SMALLSLIDERHEIGHT;
	contrlRect.right = contrlRect.left + SV_AU_CHARWIDTH*2;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_TH_MIN);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	contrlRect.left = contrlRect.right + SV_AU_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_AU_SMALLSLIDERWIDTH - 2*SV_AU_CHARWIDTH;
	m_sldThreshold.MoveWindow(&contrlRect, FALSE);
	m_rThres = contrlRect;

	m_sldThreshold.GetThumbRect(&infoRect);
	infoRect += contrlRect.TopLeft();
	infoRect.top += SV_AU_SMALLSLIDERHEIGHT; 
	infoRect.bottom += SV_AU_SMALLSLIDERHEIGHT; 
	infoRect.right += SV_AU_CHARWIDTH *3;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_TH_CUR);
	ctrlStatic->MoveWindow(&infoRect, FALSE);	
	CString str;
	str.Format(_T("%d"), m_sldThreshold.GetPos());
	ctrlStatic->SetWindowText(str);

	contrlRect.left = contrlRect.right + SV_AU_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_AU_CHARWIDTH *2;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_TH_MAX);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);	


	// Compute the Operation mode
	contrlRect.left = clientRect.left + SV_AU_SPACING*2 + SV_AU_GRP_WIDTH *2;
	contrlRect.bottom = clientRect.bottom - SV_AU_SPACING;
	contrlRect.top = clientRect.bottom - SV_AU_GRP_HEIGHT;
	contrlRect.right = contrlRect.left + SV_AU_GRP_WIDTH;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_AU_OPPGRP);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Cutting mode radio
	contrlRect.left += SV_AU_SPACING;
	contrlRect.top += SV_AU_SPACING * 2;
	contrlRect.right -= SV_AU_SPACING;
	contrlRect.bottom = contrlRect.top + SV_AU_TEXTHEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_AU_RAD_CUT);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Segmentation mode radio
	contrlRect.top = contrlRect.bottom + SV_AU_SPACING;
	contrlRect.bottom = contrlRect.top + SV_AU_TEXTHEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_AU_RAD_SEG);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Cutting method combobox label
	contrlRect.top = contrlRect.bottom + SV_AU_SPACING;
	contrlRect.bottom = contrlRect.top + SV_AU_TEXTHEIGHT;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_AU_LBL_CUTMED);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Cutting method combobox
	contrlRect.top = contrlRect.bottom + SV_AU_SPACING/2;
	contrlRect.bottom = contrlRect.top + SV_AU_TEXTHEIGHT;
	m_cbbCutMethod.MoveWindow(&contrlRect, FALSE);

	
	// Compute the button set
	// Close button
	contrlRect.right = clientRect.right - SV_AU_SPACING;
	contrlRect.left = contrlRect.right - SV_AU_BUTTON_WIDTH;
	contrlRect.bottom = clientRect.bottom - SV_AU_SPACING;
	contrlRect.top = contrlRect.bottom - SV_AU_BUTTON_HEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDCANCEL );
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Save volume button
	contrlRect.bottom = contrlRect.top - SV_AU_SPACING*2;
	contrlRect.top = contrlRect.bottom - SV_AU_BUTTON_HEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_AU_CMD_SAVEVOL);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Save all slice button
	contrlRect.bottom = contrlRect.top - SV_AU_SPACING/2;
	contrlRect.top = contrlRect.bottom - SV_AU_BUTTON_HEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_AU_CMD_SAVEALL);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Save a slice button
	contrlRect.bottom = contrlRect.top - SV_AU_SPACING/2;
	contrlRect.top = contrlRect.bottom - SV_AU_BUTTON_HEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_AU_CMD_SAVESINGLE);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	//Compute the image output size
	int nHeight = clientRect.Height() - (5*SV_AU_SPACING + SV_AU_GRP_HEIGHT + SV_AU_LARGESLIDERHEIGHT);
	int nWidth = (clientRect.Width() - 3*SV_AU_SPACING) /2;
	int nSize = min(nHeight, nWidth);

	m_OrgRegion.left = clientRect.left + SV_AU_SPACING;
	m_OrgRegion.top = clientRect.top + SV_AU_SPACING;
	m_OrgRegion.right = m_OrgRegion.left + nSize;
	m_OrgRegion.bottom = m_OrgRegion.top + nSize;
	m_ResRegion = m_OrgRegion;
	m_ResRegion.OffsetRect(nSize + SV_AU_SPACING, 0);

	ctrlStatic = (CStatic*)GetDlgItem(IDC_AU_ORGFRAME);
	ctrlStatic->MoveWindow(&m_OrgRegion, FALSE);
	ctrlStatic = (CStatic*)GetDlgItem(IDC_AU_RESFRAME);
	ctrlStatic->MoveWindow(&m_ResRegion, FALSE);

	contrlRect.left = m_OrgRegion.left;
	contrlRect.right = m_ResRegion.right;
	contrlRect.top = m_OrgRegion.bottom + SV_AU_SPACING;
	contrlRect.bottom = contrlRect.top + SV_AU_LARGESLIDERHEIGHT;
	m_sldSliceNo.MoveWindow(&contrlRect, FALSE);
	m_OrgRegion.InflateRect(-3,-3);
	m_ResRegion.InflateRect(-3,-3);

	// Update frame for the cutting object
	m_oCutter.SetFrame(m_OrgRegion);
}

void CAutoSegment::UpdateLUT()
{
	CImgLib::GenerateLinearLUT(m_nLUT, m_sldBright.GetPos(), m_sldContrast.GetPos());
}

CPoint CAutoSegment::ScreenToImage( CPoint screenPoint, CRect in_Rect )
{
	CPoint imgPos(0,0);

	DOUBLE rate =1;
	rate = ((DOUBLE)(screenPoint.x - in_Rect.left))/in_Rect.Width();
	imgPos.x = rate * m_pDataSet->GetSize().ndx;
	rate = ((DOUBLE)(screenPoint.y - in_Rect.top))/in_Rect.Height();
	imgPos.y= rate * m_pDataSet->GetSize().ndy;

	return imgPos;
}

void CAutoSegment::GetRawSliceFromDataSet( int nPos )
{
	UCHAR* pVolume = m_pDataSet->GetDataBuff();
	CSize sliceSize(m_pDataSet->GetSize().ndx,m_pDataSet->GetSize().ndy);
	int nSlicePos = m_sldSliceNo.GetPos();
	UCHAR* pCurrentSlice = pVolume + sliceSize.cx * sliceSize.cy * nPos;
	CopyMemory(m_pSourceByteImg,pCurrentSlice, sliceSize.cx* sliceSize.cy);
}
// CAutoSegment message handlers

void CAutoSegment::OnBnClickedAuChkDef()
{
	// TODO: Add your control notification handler code here
	// TODO: Add your control notification handler code here	
	if(m_chkDefaultThres.GetCheck() == BST_CHECKED) 
		m_nThresholdVal = SV_AU_DEF;
	else 
		m_nThresholdVal = m_sldThreshold.GetPos();

	m_sldThreshold.EnableWindow(m_chkDefaultThres.GetCheck() == BST_UNCHECKED);
	UpdateSlices();
	DisplayImage();
}

void CAutoSegment::OnBnClickedAuChkPreproc()
{
	// TODO: Add your control notification handler code here
	// TODO: Add your control notification handler code here
	m_sldBright.EnableWindow(m_chkPreproc.GetCheck() == BST_CHECKED);
	m_sldContrast.EnableWindow(m_chkPreproc.GetCheck() == BST_CHECKED);

	UpdateSlices();
	DisplayImage();
}

void CAutoSegment::OnBnClickedAuCmdSaveall()
{
	// TODO: Add your control notification handler code here

	// User prompt to get the information: directory, file extension
	CSegmentExportDlg dlgSave;
	CString sDirPath = _T(".");
	CString sFileExt;
	CString strOutputFile;	
	strOutputFile.Format(_T("%s_%dx%d"),SV_EXP_DEFAULTNAME, m_pDataSet->GetSize().ndx, m_pDataSet->GetSize().ndx);

	CSize sliceSize(m_pDataSet->GetSize().ndx,m_pDataSet->GetSize().ndy);
	UCHAR* pBuffer = new UCHAR[sliceSize.cx * sliceSize.cy * 3];	
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
			//CutOut(m_pSourceByteImg, pByteBuffer, sliceSize.cx, sliceSize.cy);
			AutoSegment();

			//Save it
			CString numbers;
			numbers.Format(_T("_%.3d"), iSlice);
			CString FullPath = sDirPath + strOutputFile + numbers + _T(".")+ sFileExt;

			for (int i=0; i<sliceSize.cx * sliceSize.cy; i++)
			{			
				memset(pBuffer + i*3, m_pResByteImg[i], 3);
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
					oFile.Write(m_pResByteImg, nLength);		
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
	
	//Re update m_pSourceByteImg slice back to current slice
	UpdateSlices();


}

void CAutoSegment::OnBnClickedAuCmdSavesingle()
{
	// TODO: Add your control notification handler code here
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
				oFile.Write(m_pResByteImg, nLength);	
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

void CAutoSegment::OnBnClickedAuCmdSavevol()
{
	// TODO: Add your control notification handler code here
	CString strOutputFile;	
	strOutputFile.Format(_T("%s_%dx%dx%d"), SV_EXP_DEFAULTNAME, m_pDataSet->GetSize().ndx, m_pDataSet->GetSize().ndx,m_pDataSet->GetSize().ndz);
	CString strFilters = _T("Raw files (*.raw)| *.raw||");
	CFileDialog dlgSave(FALSE, _T(".raw"), strOutputFile, OFN_OVERWRITEPROMPT, strFilters);
	CFile oFile;	
	CFileException ex;

	if (dlgSave.DoModal() == IDOK)
	{
		UINT nLength = m_pDataSet->GetSize().ndx*m_pDataSet->GetSize().ndy;
		CString path = dlgSave.GetPathName();
		BOOL bflagOpen = oFile.Open(dlgSave.GetPathName(), CFile::modeWrite | CFile::modeCreate, &ex);
		if(bflagOpen)
		{
			for (int iSlice=m_pDataSet->GetSize().ndz-1 ; iSlice >=0 ; iSlice--)
			{
				//Get Raw Slice
				GetRawSliceFromDataSet(iSlice);

				//Cut it out
				CSize sliceSize(m_pDataSet->GetSize().ndx,m_pDataSet->GetSize().ndy);
				//CutOut(m_pSourceByteImg, pByteBuffer, sliceSize.cx, sliceSize.cy);
				AutoSegment();

				//Write to file
				oFile.Write(m_pResByteImg, nLength);
			}
			oFile.Flush();			
		}		
		oFile.Close();
	}
	
	//Re update m_pSourceByteImg slice back to current slice
	UpdateSlices();
}

void CAutoSegment::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default

	// TODO: Add your message handler code here and/or call default
	//UpdatePreproc();
	if ((m_chkDefaultThres.GetCheck() == BST_UNCHECKED) && (m_sldThreshold.GetPos() != m_nThresholdVal))
	{
		m_nThresholdVal = m_sldThreshold.GetPos();
		CString str;
		CRect thumb;
		m_sldThreshold.GetThumbRect(&thumb);
		thumb += m_rThres.TopLeft();
		thumb += CPoint(0, SV_AU_TEXTHEIGHT);
		thumb.right += SV_AU_CHARWIDTH*3;
		CStatic* pStatic = (CStatic*) GetDlgItem(IDC_TH_CUR);
		pStatic->MoveWindow(&thumb);
		str.Format(_T("%d"), m_nThresholdVal);
		pStatic->SetWindowText(str);		

	}
	UpdateLUT();
	UpdateSlices();
	DisplayImage();		
}

BOOL CAutoSegment::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	ASSERT(m_pDataSet != NULL);

	//Init for the slice No 
	m_sldSliceNo.SetRange(0, m_pDataSet->GetSize().ndz-1, TRUE);
	m_sldSliceNo.SetPos(m_pDataSet->GetSize().ndz/2);

	// Preprocessing group
	m_sldContrast.SetRange(SV_CONS_MIN, SV_CONS_MAX, TRUE);
	m_sldContrast.SetPos(SV_CONS_DEFAULT);
	m_sldBright.SetRange(SV_BRIG_MIN,SV_BRIG_MAX, TRUE);
	m_sldBright.SetPos(SV_BRIG_DEFAULT);
	m_chkPreproc.SetCheck(BST_UNCHECKED);
	m_sldBright.EnableWindow(m_chkPreproc.GetCheck()==BST_CHECKED);
	m_sldContrast.EnableWindow(m_chkPreproc.GetCheck()==BST_CHECKED);

	// Threshold setting group
	m_sldThreshold.SetRange(SV_AU_MIN, SV_AU_MAX, TRUE); 
	m_sldThreshold.SetPos(SV_AU_DEF);
	m_nThresholdVal = SV_AU_DEF;

	m_chkDefaultThres.SetCheck(BST_CHECKED);
	m_sldThreshold.EnableWindow(m_chkDefaultThres.GetCheck() == BST_UNCHECKED);

	CRect mYRec(50,50, 50+ SV_AU_DLGINITWIDTH, 50+SV_AU_DLGINITHEIGHT);
	CalcWindowRect(&mYRec);
	SetWindowPos(&CWnd::wndTop, mYRec.left, mYRec.top, mYRec.Width(), mYRec.Height(),SWP_SHOWWINDOW);
	m_bInitDone = TRUE;
	m_nMode = 0;
	m_cbbCutMethod.SetCurSel(1);
	m_oCutter.SetMode(1);
	m_cbbCutMethod.EnableWindow(FALSE);

	UpdateData(FALSE);
	UpdateLayout(SV_AU_DLGINITWIDTH, SV_AU_DLGINITHEIGHT);
	UpdateSlices();
	

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAutoSegment::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
// 	if (m_nMode == 1)
// 	{
		if ((m_oCutter.GetCutStatus() == SV_CUT_NOTSTARTED) && (m_OrgRegion.PtInRect(point)))
		{
			m_oCutter.SetCutStatus(SV_CUT_STARTED);
			m_oCutter.AddPoint(point);
		}
//	}
	
}

void CAutoSegment::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if(m_OrgRegion.PtInRect(point))
	{
		if(m_oCutter.GetCutStatus() == SV_CUT_CUTTING)
		{	
			m_oCutter.AddPoint(point);
			if (m_oCutter.GetMode() == SV_CUT_RECTANGLE)
			{
				m_oCutter.SetCutStatus(SV_CUT_FINISHED);
				m_oCutter.GenerateMask(m_pCuttingMask, m_pDataSet->GetSize().ndx, m_pDataSet->GetSize().ndy);
				UpdateSlices();
				DisplayImage();
			}
		}
	}	
	CDialog::OnLButtonUp(nFlags, point);
}

void CAutoSegment::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if(m_OrgRegion.PtInRect(point))
	{
		::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));	
		if ((m_oCutter.GetCutStatus() == SV_CUT_STARTED))
		{
			m_oCutter.SetCutStatus(SV_CUT_CUTTING);

		}
		if ((m_oCutter.GetCutStatus() == SV_CUT_CUTTING))
		{
			m_oCutter.AddCandidatePoint(point);
			DisplayImage();
		}
	}
	CDialog::OnMouseMove(nFlags, point);
}

void CAutoSegment::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	
	DisplayImage();
	
}

void CAutoSegment::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// If it is delete path action
	if(m_OrgRegion.PtInRect(point))
	{
		m_oCutter.ResetCut();
		FillMemory(m_pCuttingMask, m_pDataSet->GetSize().ndx *m_pDataSet->GetSize().ndy, 255);
		UpdateSlices();
		DisplayImage();
	}
	CDialog::OnRButtonDblClk(nFlags, point);
}

void CAutoSegment::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// TODO: Add your message handler code here and/or call default
	if (m_OrgRegion.PtInRect(point))
	{
		if(m_oCutter.GetCutStatus() == SV_CUT_CUTTING)
		{
			m_oCutter.SetCutStatus(SV_CUT_FINISHED);
			m_oCutter.GenerateMask(m_pCuttingMask, m_pDataSet->GetSize().ndx, m_pDataSet->GetSize().ndy);
			UpdateSlices();
			DisplayImage();
		}

	}
	CDialog::OnRButtonDown(nFlags, point);
}

void CAutoSegment::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if((m_bInitDone)&&(nType !=  SIZE_MINIMIZED)) 
	{
		UpdateLayout(cx,cy);
		Invalidate();
	}
	// TODO: Add your message handler code here
}

void CAutoSegment::OnCbnSelchangeAuCbCmode()
{
	// TODO: Add your control notification handler code here
	// TODO: Add your control notification handler code here
	m_oCutter.SetMode(m_cbbCutMethod.GetCurSel());
	TRACE1("%d\t", m_cbbCutMethod.GetCurSel());
}

void CAutoSegment::OnBnClickedRad()
{
// 	UpdateData(TRUE);
// 	if (m_nMode == m_nPreMode) return;
// 
// 	// From Cutting mode --> Segmentation 
// 	if (m_nMode == 1){}
// 	// From Segmentation --> Cutting mode
// 	else
// 	{
// 		// Generate the cutting mask
// 		m_oCutter.GenerateMask(m_pCuttingMask, m_pDataSet->GetSize().ndx, m_pDataSet->GetSize().ndy);
// 	}
// 
// 	m_nPreMode = m_nMode;
}

//////////////////////////////////////////////////////////////////////////
void CAutoSegment::AutoSegment()
{
	int width =  m_pDataSet->GetSize().ndx;
	int height =  m_pDataSet->GetSize().ndy;
	int memSize = width * height;
	UCHAR* pBufferImg1 = new UCHAR[memSize];
	UCHAR* pBufferImg2 = new UCHAR[memSize];

	// Binarize the image save to buffer 1
	CImgLib::Binary(m_pSourceByteImg, pBufferImg1, width, height, m_nThresholdVal);

	// Auto segmentation
	CRect region;
	if(m_oCutter.GetCutStatus() == SV_CUT_FINISHED)
		region = m_oCutter.GetCutRectangle();
	else
		region = m_OrgRegion;

	CPoint topleft = ScreenToImage(region.TopLeft(), m_OrgRegion);
	CPoint botright = ScreenToImage(region.BottomRight(), m_OrgRegion);
	
	CImgLib::BinBoundaryDetectEx(
		pBufferImg1, 
		pBufferImg2,
		m_pDataSet->GetSize().ndx, 
		m_pDataSet->GetSize().ndy, 
		topleft.x, 
		topleft.y, 
		botright.x - topleft.x, 
		botright.y - topleft.y );

	CImgLib::Mask(m_pSourceByteImg, m_pResByteImg, pBufferImg2, width, height, SV_MEANNING_MARK);
	if(m_oCutter.GetCutStatus() == SV_CUT_FINISHED)
		CImgLib::Mask(m_pResByteImg, m_pCuttingMask, memSize, SV_MEANNING_MARK);
	
	delete [] pBufferImg1;
	delete [] pBufferImg2;	

}