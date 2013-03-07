// DlgRGSegment.cpp : implementation file
//

#include "stdafx.h"
#include "SliceView.h"
#include "DlgRGSegment.h"
#include "Common.h"
#include "ImgLib.h"
#include "Utility.h"
#include "SegmentExport.h"


#include <GdiPlus.h>
using namespace Gdiplus;



// Define the constant for the region growing
#define SV_RG_MIN			0
#define SV_RG_MAX			255
#define SV_RG_DEF			32




// CDlgRGSegment dialog
IMPLEMENT_DYNAMIC(CDlgRGSegment, CDialog)

CDlgRGSegment::CDlgRGSegment(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgRGSegment::IDD, pParent)
	, m_nMode(1)
{

}

CDlgRGSegment::CDlgRGSegment( CImageDataSet* in_pData, CWnd* pParent /*= NULL*/ )
: CDialog(CDlgRGSegment::IDD, pParent), 
	m_pDataSet(in_pData), 
	m_bDefaultThres(FALSE)
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

	m_ppResultSet = new UCHAR*[in_pData->GetSize().ndz];
	FillMemory(m_ppResultSet, in_pData->GetSize().ndz * sizeof(UCHAR*), NULL);

	m_pCuttingMask = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy];
	FillMemory(m_pCuttingMask,in_pData->GetSize().ndx * in_pData->GetSize().ndy,0);

	m_bInitDone = FALSE;
	m_ResRegion = CRect(0,0,0,0);
	m_OrgRegion = CRect(0,0,0,0);

	// Init look up table
	for (int i=0; i<SV_GRAYLEVELS; i++)
	{
		m_nLUT[i] = i;
	}
}


CDlgRGSegment::~CDlgRGSegment()
{
	delete[] m_pSourceByteImg;
	delete[] m_pResultImg;
	delete[] m_pSourceImg;
	delete[] m_pCuttingMask;
	delete[] m_pResByteImg;

	for (int i=0; i< m_pDataSet->GetSize().ndz; i++ )
	{
		if (m_ppResultSet[i] != NULL)
		{
			delete[] (UCHAR*)(m_ppResultSet[i]);
			m_ppResultSet[i] = NULL;
		}
	}
	delete[] m_ppResultSet;
}

void CDlgRGSegment::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RG_SLD_BRIGHT, m_sldBright);
	DDX_Control(pDX, IDC_RG_SLD_CONTRAST, m_sldContrast);
	DDX_Control(pDX, IDC_RG_SLD_SLICE, m_sldSliceNo);
	DDX_Control(pDX, IDC_RG_SLD_THRES, m_sldThreshold);
	DDX_Control(pDX, IDC_RG_CHK_PREPROC, m_chkPreproc);
	DDX_Control(pDX, IDC_RG_CHK_DEF, m_chkDefaultThres);	
	DDX_Check(pDX, IDC_RG_CHK_DEF, m_bDefaultThres);
	DDX_Radio(pDX, IDC_RAD_SEG, m_nMode);
	DDX_Control(pDX, IDC_RG_CB_CUTMODE, m_cbbCutMethod);
	
}


BEGIN_MESSAGE_MAP(CDlgRGSegment, CDialog)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_RG_CHK_DEF, &CDlgRGSegment::OnBnClickedRgChkDef)
	ON_BN_CLICKED(IDC_RG_CHK_PREPROC, &CDlgRGSegment::OnBnClickedRgChkPreproc)
	ON_BN_CLICKED(IDC_RG_CMD_SAVEALL, &CDlgRGSegment::OnBnClickedRgCmdSaveall)
	ON_BN_CLICKED(IDC_RG_CMD_SAVESINGLE, &CDlgRGSegment::OnBnClickedRgCmdSavesingle)
	ON_BN_CLICKED(IDC_RG_CMD_SAVEVOL, &CDlgRGSegment::OnBnClickedRgCmdSavevol)
	ON_WM_HSCROLL()
	ON_WM_SIZING()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	
	ON_WM_RBUTTONDBLCLK()
	ON_BN_CLICKED(IDC_RAD_CUT, &CDlgRGSegment::OnBnClickedRad)
	ON_BN_CLICKED(IDC_RAD_SEG, &CDlgRGSegment::OnBnClickedRad)
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_CBN_SELCHANGE(IDC_RG_CB_CUTMODE, &CDlgRGSegment::OnCbnSelchangeRgCbCutmode)
END_MESSAGE_MAP()


// CDlgRGSegment message handlers

void CDlgRGSegment::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
	//DrawCutCurve();
	DisplayImage();
	
	// Do not call CDialog::OnPaint() for painting messages
}

//////////////////////////////////////////////////////////////////////////
#define SV_RG_SPACING					10
#define SV_RG_SMALL_SPACING				0
#define SV_RG_GRP_HEIGHT				200
#define SV_RG_GRP_WIDTH					200
#define SV_RG_TEXTHEIGHT				18
#define SV_RG_SMALLSLIDERHEIGHT			20
#define SV_RG_SMALLSLIDERWIDTH			160

#define SV_RG_LARGESLIDERHEIGHT			30
#define SV_RG_BUTTON_WIDTH				120
#define SV_RG_BUTTON_HEIGHT				25
#define SV_RG_DLGINITWIDTH				880
#define SV_RG_DLGINITHEIGHT				650

#define SV_RG_CHARWIDTH					10

void CDlgRGSegment::OnSize(UINT nType, int cx, int cy)
{	
	CDialog::OnSize(nType, cx, cy);

	if((m_bInitDone)&&(nType !=  SIZE_MINIMIZED)) 
	{

		UpdateLayout(cx,cy);
		Invalidate();
	}
	// TODO: Add your message handler code here
}

void CDlgRGSegment::UpdateLayout( int cx, int cy )
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
	contrlRect.left = clientRect.left + SV_RG_SPACING;
	contrlRect.bottom = clientRect.bottom - SV_RG_SPACING;
	contrlRect.top = clientRect.bottom - SV_RG_GRP_HEIGHT;
	contrlRect.right = contrlRect.left + SV_RG_GRP_WIDTH;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_RG_GRAYGRP);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Preprocessing check box
	contrlRect.left += SV_RG_SPACING;
	contrlRect.top += SV_RG_SPACING * 2;
	contrlRect.right -= SV_RG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_RG_TEXTHEIGHT;
	m_chkPreproc.MoveWindow(&contrlRect, FALSE);

	// Brightness label
	contrlRect.top = contrlRect.bottom + SV_RG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_RG_TEXTHEIGHT;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_LBL_BRIGHT);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);
	

	//Brightness slider
	contrlRect.top = contrlRect.bottom + SV_RG_SMALL_SPACING;
	contrlRect.bottom = contrlRect.top + SV_RG_SMALLSLIDERHEIGHT;
	contrlRect.right = contrlRect.left + SV_RG_CHARWIDTH;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_BR_100);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);
	
	contrlRect.left = contrlRect.right + SV_RG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_RG_SMALLSLIDERWIDTH;
	m_sldBright.MoveWindow(&contrlRect, FALSE);
			
	contrlRect.left = contrlRect.right + SV_RG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_RG_CHARWIDTH;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_BR_M100);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Contrast label
	contrlRect.top = contrlRect.bottom + SV_RG_SPACING;	
	contrlRect.bottom = contrlRect.top + SV_RG_TEXTHEIGHT;
	contrlRect.left = clientRect.left + SV_RG_SPACING *2;
	contrlRect.right = clientRect.left + SV_RG_SMALLSLIDERWIDTH;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_LBL_CONTRAST);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Contrast slider
	contrlRect.top = contrlRect.bottom + SV_RG_SMALL_SPACING;
	contrlRect.bottom = contrlRect.top + SV_RG_SMALLSLIDERHEIGHT;
	contrlRect.right = contrlRect.left + SV_RG_CHARWIDTH;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_CS_100);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	contrlRect.left = contrlRect.right + SV_RG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_RG_SMALLSLIDERWIDTH;
	m_sldContrast.MoveWindow(&contrlRect, FALSE);

	contrlRect.left = contrlRect.right + SV_RG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_RG_CHARWIDTH;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_CS_M100);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Compute the region growing setting
	contrlRect.left = clientRect.left + SV_RG_SPACING*1.5 + SV_RG_GRP_WIDTH;
	contrlRect.bottom = clientRect.bottom - SV_RG_SPACING;
	contrlRect.top = clientRect.bottom - SV_RG_GRP_HEIGHT;
	contrlRect.right = contrlRect.left + SV_RG_GRP_WIDTH;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_RG_SETTINGGRP);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Use default check box
	contrlRect.left += SV_RG_SPACING;
	contrlRect.top += SV_RG_SPACING * 2;
	contrlRect.right -= SV_RG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_RG_TEXTHEIGHT;
	m_chkDefaultThres.MoveWindow(&contrlRect, FALSE);

	// Threshold slider label
	contrlRect.top = contrlRect.bottom + SV_RG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_RG_TEXTHEIGHT;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_LBL_THRES);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Threshold  slider
	contrlRect.top = contrlRect.bottom + SV_RG_SMALL_SPACING;
	contrlRect.bottom = contrlRect.top + SV_RG_SMALLSLIDERHEIGHT;
	contrlRect.right = contrlRect.left + SV_RG_CHARWIDTH*2;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_TH_MIN);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	contrlRect.left = contrlRect.right + SV_RG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_RG_SMALLSLIDERWIDTH - 2*SV_RG_CHARWIDTH;
	m_sldThreshold.MoveWindow(&contrlRect, FALSE);
	m_rThres = contrlRect;

	m_sldThreshold.GetThumbRect(&infoRect);
	infoRect += contrlRect.TopLeft();
	infoRect.top += SV_RG_SMALLSLIDERHEIGHT; 
	infoRect.bottom += SV_RG_SMALLSLIDERHEIGHT; 
	infoRect.right += SV_RG_CHARWIDTH *3;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_TH_CUR);
	ctrlStatic->MoveWindow(&infoRect, FALSE);	
	CString str;
	str.Format(_T("%d"), m_sldThreshold.GetPos());
	ctrlStatic->SetWindowText(str);

	contrlRect.left = contrlRect.right + SV_RG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_RG_CHARWIDTH *2;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_TH_MAX);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);	
	

	// Compute the Operation mode
	contrlRect.left = clientRect.left + SV_RG_SPACING*2 + SV_RG_GRP_WIDTH *2;
	contrlRect.bottom = clientRect.bottom - SV_RG_SPACING;
	contrlRect.top = clientRect.bottom - SV_RG_GRP_HEIGHT;
	contrlRect.right = contrlRect.left + SV_RG_GRP_WIDTH;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_RG_OPPGRP);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);
	
	// Cutting mode radio
	contrlRect.left += SV_RG_SPACING;
	contrlRect.top += SV_RG_SPACING * 2;
	contrlRect.right -= SV_RG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_RG_TEXTHEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_RAD_CUT);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Segmentation mode radio
	contrlRect.top = contrlRect.bottom + SV_RG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_RG_TEXTHEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_RAD_SEG);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Cutting method combobox label
	contrlRect.top = contrlRect.bottom + SV_RG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_RG_TEXTHEIGHT;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_RG_LBL_CUTMED);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Cutting method combobox
	contrlRect.top = contrlRect.bottom + SV_RG_SPACING/2;
	contrlRect.bottom = contrlRect.top + SV_RG_TEXTHEIGHT;
	ctrlCombobox = (CComboBox*)GetDlgItem(IDC_RG_CB_CUTMODE);
	ctrlCombobox->MoveWindow(&contrlRect, FALSE);


	// Compute the button set
	// Close button
	contrlRect.right = clientRect.right - SV_RG_SPACING;
	contrlRect.left = contrlRect.right - SV_RG_BUTTON_WIDTH;
	contrlRect.bottom = clientRect.bottom - SV_RG_SPACING;
	contrlRect.top = contrlRect.bottom - SV_RG_BUTTON_HEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDCANCEL);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Save volume button
	contrlRect.bottom = contrlRect.top - SV_RG_SPACING*2;
	contrlRect.top = contrlRect.bottom - SV_RG_BUTTON_HEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_RG_CMD_SAVEVOL);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Save all slice button
	contrlRect.bottom = contrlRect.top - SV_RG_SPACING/2;
	contrlRect.top = contrlRect.bottom - SV_RG_BUTTON_HEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_RG_CMD_SAVEALL);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Save a slice button
	contrlRect.bottom = contrlRect.top - SV_RG_SPACING/2;
	contrlRect.top = contrlRect.bottom - SV_RG_BUTTON_HEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_RG_CMD_SAVESINGLE);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	//Compute the image output size
	int nHeight = clientRect.Height() - (5*SV_RG_SPACING + SV_RG_GRP_HEIGHT + SV_RG_LARGESLIDERHEIGHT);
	int nWidth = (clientRect.Width() - 3*SV_RG_SPACING) /2;
	int nSize = min(nHeight, nWidth);

	m_OrgRegion.left = clientRect.left + SV_RG_SPACING;
	m_OrgRegion.top = clientRect.top + SV_RG_SPACING;
	m_OrgRegion.right = m_OrgRegion.left + nSize;
	m_OrgRegion.bottom = m_OrgRegion.top + nSize;
	m_ResRegion = m_OrgRegion;
	m_ResRegion.OffsetRect(nSize + SV_RG_SPACING, 0);
	
	ctrlStatic = (CStatic*)GetDlgItem(IDC_AU_ORGFRAME);
	ctrlStatic->MoveWindow(&m_OrgRegion, FALSE);
	ctrlStatic = (CStatic*)GetDlgItem(IDC_AU_RESFRAME);
	ctrlStatic->MoveWindow(&m_ResRegion, FALSE);

	contrlRect.left = m_OrgRegion.left;
	contrlRect.right = m_ResRegion.right;
	contrlRect.top = m_OrgRegion.bottom + SV_RG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_RG_LARGESLIDERHEIGHT;
	m_sldSliceNo.MoveWindow(&contrlRect, FALSE);
	m_OrgRegion.InflateRect(-3,-3);
	m_ResRegion.InflateRect(-3,-3);

	// Update frame for the cutting object
	m_oCutter.SetFrame(m_OrgRegion);
}

BOOL CDlgRGSegment::OnInitDialog()
{
	//this->Res
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
	m_sldThreshold.SetRange(SV_RG_MIN, SV_RG_MAX, TRUE); 
	m_sldThreshold.SetPos(SV_RG_DEF);
	m_nThresholdVal = SV_RG_DEF;

	m_chkDefaultThres.SetCheck(BST_CHECKED);
	m_sldThreshold.EnableWindow(m_chkDefaultThres.GetCheck() == BST_UNCHECKED);
	
	CRect mYRec(50,50, 50+ SV_RG_DLGINITWIDTH, 50+SV_RG_DLGINITHEIGHT);
	CalcWindowRect(&mYRec);
	SetWindowPos(&CWnd::wndTop, mYRec.left, mYRec.top, mYRec.Width(), mYRec.Height(),SWP_SHOWWINDOW);
	m_bInitDone = TRUE;

	m_cbbCutMethod.SetCurSel(0);
	m_nMode = 1;
	UpdateLayout(SV_RG_DLGINITWIDTH, SV_RG_DLGINITHEIGHT);
	UpdateSlices();
	return TRUE;  
}

void CDlgRGSegment::OnBnClickedRgChkDef()
{
	// TODO: Add your control notification handler code here	
	if(m_chkDefaultThres.GetCheck() == BST_CHECKED) 
		m_nThresholdVal = SV_RG_DEF;
	else 
		m_nThresholdVal = m_sldThreshold.GetPos();

	m_sldThreshold.EnableWindow(m_chkDefaultThres.GetCheck() == BST_UNCHECKED);
	UpdateSlices();
	DisplayImage();
}

void CDlgRGSegment::OnBnClickedRgChkPreproc()
{
	// TODO: Add your control notification handler code here
	m_sldBright.EnableWindow(m_chkPreproc.GetCheck() == BST_CHECKED);
	m_sldContrast.EnableWindow(m_chkPreproc.GetCheck() == BST_CHECKED);

	UpdateSlices();
	DisplayImage();
}

void CDlgRGSegment::OnBnClickedRgCmdSaveall()
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

			if (m_ppResultSet[iSlice] == NULL) continue;
		
			//Save it
			CString numbers;
			numbers.Format(_T("_%.3d"), iSlice);
			CString FullPath = sDirPath + strOutputFile + numbers + _T(".")+ sFileExt;
			UCHAR* pByteResult = m_ppResultSet[iSlice];
			for (int i=0; i<sliceSize.cx * sliceSize.cy; i++)
			{			
				memset(pBuffer + i*3, pByteResult[i], 3);
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
					oFile.Write(pByteResult, nLength);		
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
}

void CDlgRGSegment::OnBnClickedRgCmdSavesingle()
{
	// TODO: Add your control notification handler code here
	// TODO: Add your control notification handler code here
	
	int curPos = m_sldSliceNo.GetPos();
	if (m_ppResultSet[curPos] == NULL)
	{
		AfxMessageBox(_T("The slice is not be processed yet!"));
		return;
	}

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
				oFile.Write(m_ppResultSet[curPos], nLength);	
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

void CDlgRGSegment::OnBnClickedRgCmdSavevol()
{
	// TODO: Add your control notification handler code here
	CString strOutputFile;	
	strOutputFile.Format(_T("%s_%dx%dx%d"), SV_EXP_DEFAULTNAME, m_pDataSet->GetSize().ndx, m_pDataSet->GetSize().ndy,m_pDataSet->GetSize().ndz);
	CString strFilters = _T("Raw files (*.raw)| *.raw||");
	CFileDialog dlgSave(FALSE, _T(".raw"), strOutputFile, OFN_OVERWRITEPROMPT, strFilters);
	//UCHAR* pByteBuffer = new UCHAR[m_pDataSet->GetSize().ndx * m_pDataSet->GetSize().ndy];
	CFile oFile;	
	CFileException ex;
	int memsize = m_pDataSet->GetSize().ndx * m_pDataSet->GetSize().ndy;
	UCHAR* pDummySlice = new UCHAR[memsize];
	FillMemory(pDummySlice, memsize, 0);

	if (dlgSave.DoModal() == IDOK)
	{
		UINT nLength = m_pDataSet->GetSize().ndx*m_pDataSet->GetSize().ndy;
		CString path = dlgSave.GetPathName();
		BOOL bflagOpen = oFile.Open(dlgSave.GetPathName(), CFile::modeWrite | CFile::modeCreate, &ex);
		if(bflagOpen)
		{
			for (int iSlice=m_pDataSet->GetSize().ndz-1 ; iSlice >=0 ; iSlice--)
			{
				if (m_ppResultSet[iSlice] == NULL)
					oFile.Write(pDummySlice, memsize);
				else
					oFile.Write(m_ppResultSet[iSlice], memsize);				
			}
			oFile.Flush();			
		}		
		oFile.Close();
	}
	delete[] pDummySlice;
	
}


void CDlgRGSegment::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	//UpdatePreproc();
	if ((m_chkDefaultThres.GetCheck() == BST_UNCHECKED) && (m_sldThreshold.GetPos() != m_nThresholdVal))
	{
		m_nThresholdVal = m_sldThreshold.GetPos();
		CString str;
		CRect thumb;
		m_sldThreshold.GetThumbRect(&thumb);
		thumb += m_rThres.TopLeft();
		thumb += CPoint(0, SV_RG_TEXTHEIGHT);
		thumb.right += SV_RG_CHARWIDTH*3;
		CStatic* pStatic = (CStatic*) GetDlgItem(IDC_TH_CUR);
		pStatic->MoveWindow(&thumb);
		str.Format(_T("%d"), m_nThresholdVal);
		pStatic->SetWindowText(str);		

	}
	UpdateLUT();
	UpdateSlices();
	DisplayImage();	
}



void CDlgRGSegment::DisplayImage(void)
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



void CDlgRGSegment::UpdateSlices(void)
{
	ASSERT(m_pDataSet != NULL);	
	//ASSERT(m_pResultByteImg != NULL);
	ASSERT(m_pSourceByteImg != NULL);

	UCHAR* pVolume = m_pDataSet->GetDataBuff();
	CSize sliceSize(m_pDataSet->GetSize().ndx,m_pDataSet->GetSize().ndy);
	int nSlicePos = m_sldSliceNo.GetPos();
	UCHAR* pCurrentSlice = pVolume + sliceSize.cx * sliceSize.cy * nSlicePos;

	// Preprocessing if it is enabled
	if(m_chkPreproc.GetCheck() == BST_CHECKED)
		CImgLib::GrayAdjust(pCurrentSlice, m_pSourceByteImg, m_nLUT, sliceSize.cx, sliceSize.cy);
	else
		CopyMemory(m_pSourceByteImg,pCurrentSlice, sliceSize.cx* sliceSize.cy);
	
	// Make source image
	for (int line=0; line < sliceSize.cy; line++)
		for (int pix=0; pix < sliceSize.cx; pix++)
			memset(m_pSourceImg + (sliceSize.cx*line + pix)*3, m_pSourceByteImg[(sliceSize.cy-line-1)*sliceSize.cx + pix], 3);			
	
	// Make result image: Check current slice has been processed? if it has been processed
	if(m_ppResultSet[nSlicePos] != NULL)
	{
		// Make the display image
		UCHAR* pResult = m_ppResultSet[nSlicePos];
		for (int line=0; line < sliceSize.cy; line++)
			for (int pix=0; pix < sliceSize.cx; pix++)
				memset(m_pResultImg + (sliceSize.cx*line + pix)*3, pResult[(sliceSize.cy-line-1)*sliceSize.cx + pix], 3);
	}
	// if it hasn't -> set output image black
	else
	{
		FillMemory(m_pResultImg, sliceSize.cx * sliceSize.cy *3, 0);
	}
}

void CDlgRGSegment::GetRawSliceFromDataSet( int nPos )
{
	UCHAR* pVolume = m_pDataSet->GetDataBuff();
	CSize sliceSize(m_pDataSet->GetSize().ndx,m_pDataSet->GetSize().ndy);
	int nSlicePos = m_sldSliceNo.GetPos();
	UCHAR* pCurrentSlice = pVolume + sliceSize.cx * sliceSize.cy * nPos;

	CopyMemory(m_pSourceByteImg,pCurrentSlice, sliceSize.cx* sliceSize.cy);

	// Make source image
	for (int line=0; line < sliceSize.cy; line++)
		for (int pix=0; pix < sliceSize.cx; pix++)
			memset(m_pSourceImg + (sliceSize.cx*line + pix)*3, m_pSourceByteImg[(sliceSize.cy-line-1)*sliceSize.cx + pix], 3);					

}





void CDlgRGSegment::OnSizing(UINT fwSide, LPRECT pRect)
{
	CDialog::OnSizing(fwSide, pRect);
	// TODO: Add your message handler code here
	CRect rect = *pRect;
	if ((rect.Width() < SV_RG_DLGINITWIDTH) || (rect.Height() < SV_RG_DLGINITHEIGHT))
	{
		switch (fwSide)
		{
		case WMSZ_BOTTOM:
			rect.bottom = rect.top + SV_RG_DLGINITHEIGHT;
			break;
		case WMSZ_TOP:
			rect.top = rect.bottom - SV_RG_DLGINITHEIGHT;
			break;
		case WMSZ_LEFT:
			rect.left = rect.right - SV_RG_DLGINITWIDTH;
			break;
		case WMSZ_RIGHT:
			rect.right = rect.left + SV_RG_DLGINITWIDTH;
			break;
		case WMSZ_BOTTOMLEFT:
			rect.bottom = rect.top + SV_RG_DLGINITHEIGHT;
			rect.left = rect.right - SV_RG_DLGINITWIDTH;;
			break;
		case WMSZ_BOTTOMRIGHT:
			rect.bottom = rect.top + SV_RG_DLGINITHEIGHT;
			rect.right = rect.left + SV_RG_DLGINITWIDTH;
			break;
		case WMSZ_TOPLEFT:
			rect.top = rect.bottom - SV_RG_DLGINITHEIGHT;
			rect.left = rect.right - SV_RG_DLGINITWIDTH;	
		case WMSZ_TOPRIGHT:
			rect.top = rect.bottom - SV_RG_DLGINITHEIGHT;
			rect.right = rect.left + SV_RG_DLGINITWIDTH;
		default:
			break;
		}
		MoveWindow(&rect);
		UpdateLayout(rect.Width(), rect.Height());

	}
}

void CDlgRGSegment::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	//Check if the selected point is in image area
	if (m_nMode == 1) //Cutting mode
	{	
		if(m_OrgRegion.PtInRect(point))
			if (m_oCutter.GetCutStatus() == SV_CUT_NOTSTARTED)
		{
			m_oCutter.SetCutStatus(SV_CUT_STARTED);
			m_oCutter.AddPoint(point);
		}
	}
	else 
	{
		if(m_OrgRegion.PtInRect(point))
		{
			// Convert to image coordinate
			CPoint imgPos = ScreenToImage(point, m_OrgRegion);
			
			int width = m_pDataSet->GetSize().ndx;
			int height = m_pDataSet->GetSize().ndy;
			int sliceNo = m_sldSliceNo.GetPos();
			UCHAR* pResult = NULL;
			//Allocate memory for result image if has been not allocate
			if (m_ppResultSet[sliceNo] == NULL)
			{
				 m_ppResultSet[sliceNo] = new UCHAR[width*height];
				 
				 if (m_ppResultSet[sliceNo] != NULL)
				 {
					 FillMemory(m_ppResultSet[sliceNo], width*height, 0);
				 }
				 
			}
			pResult = m_ppResultSet[sliceNo];
			
			// Process the current slice 		
			if (pResult != NULL)
			{			
				CImgLib::RG2D(m_pSourceByteImg, pResult, width, height, imgPos, m_nThresholdVal);
				//Apply mask 
				CImgLib::Mask(pResult, m_pCuttingMask, width*height, 255);
			}		
			UpdateSlices();
			DisplayImage();
		}

	}
}


void CDlgRGSegment::OnMouseMove(UINT nFlags, CPoint point)
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


// Convert from screen coordinate to the image coordinate
CPoint CDlgRGSegment::ScreenToImage( CPoint screenPoint, CRect in_Rect )
{
	CPoint imgPos(0,0);

	DOUBLE rate =1;
	rate = ((DOUBLE)(screenPoint.x - in_Rect.left))/in_Rect.Width();
	imgPos.x = rate * m_pDataSet->GetSize().ndx;
	rate = ((DOUBLE)(screenPoint.y - in_Rect.top))/in_Rect.Height();
	imgPos.y= rate * m_pDataSet->GetSize().ndy;

	return imgPos;
}

void CDlgRGSegment::UpdateLUT()
{
	CImgLib::GenerateLinearLUT(m_nLUT, m_sldBright.GetPos(), m_sldContrast.GetPos());
}

void CDlgRGSegment::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	
	// If it is delete result action
	if(m_ResRegion.PtInRect(point))
	{
		int sliceNo = m_sldSliceNo.GetPos();
		//Allocate memory for result image if has been not allocate
		if (m_ppResultSet[sliceNo] != NULL)
		{
			delete[] m_ppResultSet[sliceNo];
			m_ppResultSet[sliceNo] = NULL;
			UpdateSlices();
			DisplayImage();
		}		
	}
	
	// If it is delete path action
	if(m_OrgRegion.PtInRect(point))
	{
		m_oCutter.ResetCut();		
		FillMemory(m_pCuttingMask, m_pDataSet->GetSize().ndx *m_pDataSet->GetSize().ndy ,255);
		UpdateSlices();
		DisplayImage();
	}
	CDialog::OnRButtonDblClk(nFlags, point);
}

void CDlgRGSegment::OnBnClickedRad()
{
 	UpdateData(TRUE);
	if (m_nMode == m_nPreMode) return;
	
	// From Segmentation --> Cutting mode
	if (m_nMode == 1){}
	// Segmentation --> cutting mode
	else
	{
		// Generate the cutting mask
		m_oCutter.GenerateMask(m_pCuttingMask, m_pDataSet->GetSize().ndx, m_pDataSet->GetSize().ndy);
	}
	m_nPreMode = m_nMode;
}

void CDlgRGSegment::OnLButtonUp(UINT nFlags, CPoint point)
{
 	//TODO: Add your message handler code here and/or call default
		if(m_OrgRegion.PtInRect(point))
		{
			if(m_oCutter.GetCutStatus() == SV_CUT_CUTTING)
			{	
				m_oCutter.AddPoint(point);
				if (m_oCutter.GetMode() == SV_CUT_RECTANGLE)
				{
					m_oCutter.SetCutStatus(SV_CUT_FINISHED);
				}
			}
		}	
	CDialog::OnLButtonUp(nFlags, point);
}

void CDlgRGSegment::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (m_OrgRegion.PtInRect(point))
	{
		if(m_oCutter.GetCutStatus() == SV_CUT_CUTTING)
		{
			m_oCutter.SetCutStatus(SV_CUT_FINISHED);
			DisplayImage();
		}
		
	}
	CDialog::OnRButtonDown(nFlags, point);
}



void CDlgRGSegment::OnCbnSelchangeRgCbCutmode()
{
	// TODO: Add your control notification handler code here
	m_oCutter.SetMode(m_cbbCutMethod.GetCurSel());
	TRACE1("%d\t", m_cbbCutMethod.GetCurSel());
}

