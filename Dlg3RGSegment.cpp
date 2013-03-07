// Dlg3RGSegment.cpp : implementation file
//

#include "stdafx.h"
#include "SliceView.h"
#include "Dlg3RGSegment.h"
#include "Common.h"
#include "Utility.h"
#include "SegmentExport.h"


#include <GdiPlus.h>
using namespace Gdiplus;

// Define the constant for the region growing
#define SV_3DRG_MIN			0
#define SV_3DRG_MAX			255
#define SV_3DRG_DEF			32


// CDlg3RGSegment dialog

IMPLEMENT_DYNAMIC(CDlg3RGSegment, CDialog)

CDlg3RGSegment::CDlg3RGSegment(CWnd* pParent /*=NULL*/)
	: CDialog(CDlg3RGSegment::IDD, pParent)	
	, m_bBoundDetect(FALSE)
{

}

CDlg3RGSegment::CDlg3RGSegment( CImageDataSet* in_pData, CWnd* pParent /*= NULL*/ )
	: CDialog(CDlg3RGSegment::IDD, pParent),
	m_pDataSet(in_pData), m_bDefaultThres(FALSE), m_nStartSlc(0), m_nEndSlc(0)

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


// 	m_pResByteImg = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy];
// 	FillMemory(m_pResByteImg,in_pData->GetSize().ndx * in_pData->GetSize().ndy,0);

	m_pSourceImg = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy * 3];
	FillMemory(m_pSourceImg,in_pData->GetSize().ndx * in_pData->GetSize().ndy *3, 0);

	m_pResultImg = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy * 3];
	FillMemory(m_pResultImg,in_pData->GetSize().ndx * in_pData->GetSize().ndy * 3, 0);

	m_ppResultSet = new UCHAR*[in_pData->GetSize().ndz];
	FillMemory(m_ppResultSet, in_pData->GetSize().ndz * sizeof(UCHAR*), NULL);

	m_pCuttingMask = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy];
	FillMemory(m_pCuttingMask,in_pData->GetSize().ndx * in_pData->GetSize().ndy,255);

	m_pBoundaryMask = new UCHAR[in_pData->GetSize().ndx * in_pData->GetSize().ndy];
	FillMemory(m_pBoundaryMask,in_pData->GetSize().ndx * in_pData->GetSize().ndy,0);

	m_bInitDone = FALSE;
	m_ResRegion = CRect(0,0,0,0);
	m_OrgRegion = CRect(0,0,0,0);

	FillMemory(&m_tSubVol, sizeof(m_tSubVol), 0);

	// Init look up table
	for (int i=0; i<SV_GRAYLEVELS; i++)
	{
		m_nLUT[i] = i;
	}
}



CDlg3RGSegment::~CDlg3RGSegment()
{
	delete[] m_pSourceByteImg;
	delete[] m_pResultImg;
	delete[] m_pSourceImg;
	delete[] m_pCuttingMask;
	//delete[] m_pResByteImg;
	delete[] m_pBoundaryMask;

	for (int i=0; i< m_pDataSet->GetSize().ndz; i++ )
	{
		if (m_ppResultSet[i] != NULL)
		{
			delete[] (UCHAR*)(m_ppResultSet[i]);
			m_ppResultSet[i] = NULL;
		}
	}
	delete[] m_ppResultSet;

	if (m_tSubVol.pData != NULL)
	{
		delete[] m_tSubVol.pData;
		m_tSubVol.pData = NULL;
	}
}

void CDlg3RGSegment::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);	
	DDX_Control(pDX, IDC_3DRG_SLD_BRIGHT, m_sldBright);
	DDX_Control(pDX, IDC_3DRG_SLD_CONTRAST, m_sldContrast);
	DDX_Control(pDX, IDC_3DRG_SLD_SLICE, m_sldSliceNo);
	DDX_Control(pDX, IDC_3DRG_SLD_THRES, m_sldThreshold);
	DDX_Control(pDX, IDC_3DRG_CHK_PREPROC, m_chkPreproc);
	DDX_Control(pDX, IDC_3DRG_CHK_DEF, m_chkDefaultThres);	
	DDX_Check(pDX, IDC_3DRG_CHK_DEF, m_bDefaultThres);
	DDX_Radio(pDX, IDC_3DRG_RAD_SEG, m_nMode);
	DDX_Control(pDX, IDC_3DRG_CB_CUTMODE, m_cbbCutMethod);
	DDX_Text(pDX, IDC_3DRG_STARSLICE, m_nStartSlc);
	DDX_Text(pDX, IDC_3DRG_ENDSLICE, m_nEndSlc);
	DDX_Check(pDX, IDC_3DRG_CHK_BOUND, m_bBoundDetect);
	DDX_Control(pDX, IDC_3DRG_SLD_BTHRES, m_sldBoundThres);
}


BEGIN_MESSAGE_MAP(CDlg3RGSegment, CDialog)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_HSCROLL()	
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_CBN_SELCHANGE(IDC_3DRG_CB_CUTMODE, &CDlg3RGSegment::OnCbnSelchangeRgCbCutmode)
	ON_BN_CLICKED(IDC_3DRG_CHK_DEF, &CDlg3RGSegment::OnBnClickedRgChkDef)
	ON_BN_CLICKED(IDC_3DRG_CHK_PREPROC, &CDlg3RGSegment::OnBnClickedRgChkPreproc)
	ON_BN_CLICKED(IDC_3DRG_CMD_SAVEALL, &CDlg3RGSegment::OnBnClickedRgCmdSaveall)
	ON_BN_CLICKED(IDC_3DRG_CMD_SAVESINGLE, &CDlg3RGSegment::OnBnClickedRgCmdSavesingle)
	ON_BN_CLICKED(IDC_3DRG_CMD_SAVEVOL, &CDlg3RGSegment::OnBnClickedRgCmdSavevol)
	ON_BN_CLICKED(IDC_3DRG_RAD_CUT, &CDlg3RGSegment::OnBnClickedRad)
	ON_BN_CLICKED(IDC_3DRG_RAD_SEG, &CDlg3RGSegment::OnBnClickedRad)
	ON_BN_CLICKED(IDC_3DRG_START, &CDlg3RGSegment::OnBnClicked3drgStart)
END_MESSAGE_MAP()

void CDlg3RGSegment::DisplayImage( void )
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
	graph.DrawString(str,-1, &myFont, origin, &whiteBrush);	
	graph.SetClip(Rect(m_OrgRegion.TopLeft().x,m_OrgRegion.TopLeft().y, m_OrgRegion.Width(), m_OrgRegion.Height()));
	if(m_oCutter.GetCutStatus() != SV_CUT_NOTSTARTED)
		m_oCutter.DrawResult(&graph);

	// Draw the picked point
	// Convert from the image coordinate to the window display window coordinate
	DrawSeedPoints();
}


void CDlg3RGSegment::UpdateSlices( void )
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

	// Make result image
	// IF the segmentation has been done --> result image = segmented image
	if (m_tSubVol.pData != NULL)
	{
		FillMemory(m_pResultImg, m_pDataSet->GetSize().ndx * m_pDataSet->GetSize().ndy * 3, 0);
		int nSubSlicePos = nSlicePos - m_tSubVol.pos3D.nZ;
 		if ((0 <= nSubSlicePos) && (nSubSlicePos < m_tSubVol.size3D.ndz))
 		{
			UCHAR* pCurSubImage = m_tSubVol.pData + m_tSubVol.size3D.ndx * m_tSubVol.size3D.ndy * nSubSlicePos;

			// If the boundary detect is enable then do the filter before output the image
			UpdateData();
			if (m_bBoundDetect)
			{				
				Cutout(m_pSourceByteImg, m_pBoundaryMask, sliceSize.cx, sliceSize.cy);
			}

// 			for (int line=0; line < sliceSize.cy; line++)
// 				for (int pix=0; pix < sliceSize.cx; pix++)
// 					memset(m_pResultImg + (sliceSize.cx*line + pix)*3, m_pBoundaryMask[(sliceSize.cy-line-1)*sliceSize.cx + pix], 3);			

			UCHAR* pCurOffset;
			// Place the sub image at the right position in result image
			for (int row=0; row<m_tSubVol.size3D.ndy; row++)
			for (int col=0; col<m_tSubVol.size3D.ndx; col++)
			{
				pCurOffset = m_pResultImg + ((sliceSize.cy - row - m_tSubVol.pos3D.nY - 1) * sliceSize.cx + (col + m_tSubVol.pos3D.nX ))*3;
				
				// If boundary detect is enabled
				if(m_bBoundDetect)
				{
					//check if pixel in the boundary mask 
					if(m_pBoundaryMask[(row + m_tSubVol.pos3D.nY) * sliceSize.cx + (col + m_tSubVol.pos3D.nX )])
						memset(pCurOffset, pCurSubImage[row * m_tSubVol.size3D.ndx + col], 3);
					else
						memset(pCurOffset, 0, 3);
				}
				else
					memset(pCurOffset, pCurSubImage[row * m_tSubVol.size3D.ndx + col], 3);
			}


		}
	}
	// ELSE 
	else
	{
		for (int line=0; line < sliceSize.cy; line++)
		for (int pix=0; pix < sliceSize.cx; pix++)
			memset(m_pResultImg + (sliceSize.cx*line + pix)*3, m_pCuttingMask[(sliceSize.cy-line-1)*sliceSize.cx + pix], 3);
	}
	
}




//////////////////////////////////////////////////////////////////////////
#define SV_3DRG_SPACING					10
#define SV_3DRG_SMALL_SPACING			0
#define SV_3DRG_GRP_HEIGHT				200
#define SV_3DRG_GRP_WIDTH				200
#define SV_3DRG_TEXTHEIGHT				18
#define SV_3DRG_SMALLSLIDERHEIGHT		20
#define SV_3DRG_SMALLSLIDERWIDTH		160

#define SV_3DRG_LARGESLIDERHEIGHT		30
#define SV_3DRG_BUTTON_WIDTH			120
#define SV_3DRG_BUTTON_HEIGHT			25
#define SV_3DRG_DLGINITWIDTH			880
#define SV_3DRG_DLGINITHEIGHT			650

#define SV_3DRG_CHARWIDTH				10
#define SV_3DRG_TEXTBOXWIDTH			35

/*
	This function update the window layout, the order of statement is significant.
//*/
void CDlg3RGSegment::UpdateLayout( int cx, int cy )
{
	// Compute layout for the window and the image displaying region
	CStatic*	ctrlStatic = NULL;
	CButton*	ctrlButton = NULL;
	CComboBox*	ctrlCombobox = NULL;
	CRect clientRect;
	CRect contrlRect;
	CRect infoRect;
	GetClientRect(&clientRect);

	//////////////////////////////////////////////////////////////////////////
	// Compute the preprocessing group
	contrlRect.left = clientRect.left + SV_3DRG_SPACING;
	contrlRect.bottom = clientRect.bottom - SV_3DRG_SPACING;
	contrlRect.top = clientRect.bottom - SV_3DRG_GRP_HEIGHT;
	contrlRect.right = contrlRect.left + SV_3DRG_GRP_WIDTH;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_3DRG_GRAYGRP);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Preprocessing check box
	contrlRect.left += SV_3DRG_SPACING;
	contrlRect.top += SV_3DRG_SPACING * 2;
	contrlRect.right -= SV_3DRG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_3DRG_TEXTHEIGHT;
	m_chkPreproc.MoveWindow(&contrlRect, FALSE);

	// Brightness label
	contrlRect.top = contrlRect.bottom + SV_3DRG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_3DRG_TEXTHEIGHT;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_LBL_BRIGHT);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);


	//Brightness slider
	contrlRect.top = contrlRect.bottom + SV_3DRG_SMALL_SPACING;
	contrlRect.bottom = contrlRect.top + SV_3DRG_SMALLSLIDERHEIGHT;
	contrlRect.right = contrlRect.left + SV_3DRG_CHARWIDTH;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_BR_100);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	contrlRect.left = contrlRect.right + SV_3DRG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_3DRG_SMALLSLIDERWIDTH;
	m_sldBright.MoveWindow(&contrlRect, FALSE);

	contrlRect.left = contrlRect.right + SV_3DRG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_3DRG_CHARWIDTH;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_BR_M100);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Contrast label
	contrlRect.top = contrlRect.bottom + SV_3DRG_SPACING;	
	contrlRect.bottom = contrlRect.top + SV_3DRG_TEXTHEIGHT;
	contrlRect.left = clientRect.left + SV_3DRG_SPACING *2;
	contrlRect.right = clientRect.left + SV_3DRG_SMALLSLIDERWIDTH;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_LBL_CONTRAST);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Contrast slider
	contrlRect.top = contrlRect.bottom + SV_3DRG_SMALL_SPACING;
	contrlRect.bottom = contrlRect.top + SV_3DRG_SMALLSLIDERHEIGHT;
	contrlRect.right = contrlRect.left + SV_3DRG_CHARWIDTH;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_CS_100);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	contrlRect.left = contrlRect.right + SV_3DRG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_3DRG_SMALLSLIDERWIDTH;
	m_sldContrast.MoveWindow(&contrlRect, FALSE);

	contrlRect.left = contrlRect.right + SV_3DRG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_3DRG_CHARWIDTH;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_CS_M100);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	//////////////////////////////////////////////////////////////////////////
	// Compute the region growing setting
	contrlRect.left = clientRect.left + SV_3DRG_SPACING*1.5 + SV_3DRG_GRP_WIDTH;
	contrlRect.bottom = clientRect.bottom - SV_3DRG_SPACING;
	contrlRect.top = clientRect.bottom - SV_3DRG_GRP_HEIGHT;
	contrlRect.right = contrlRect.left + SV_3DRG_GRP_WIDTH;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_3DRG_SETTINGGRP);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Use default check box
	contrlRect.left += SV_3DRG_SPACING;
	contrlRect.top += SV_3DRG_SPACING * 2;
	contrlRect.right -= SV_3DRG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_3DRG_TEXTHEIGHT;
	m_chkDefaultThres.MoveWindow(&contrlRect, FALSE);

	// Threshold slider label
	contrlRect.top = contrlRect.bottom + SV_3DRG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_3DRG_TEXTHEIGHT;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_LBL_THRES);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Threshold  slider
	contrlRect.top = contrlRect.bottom + SV_3DRG_SMALL_SPACING;
	contrlRect.bottom = contrlRect.top + SV_3DRG_SMALLSLIDERHEIGHT;
	contrlRect.right = contrlRect.left + SV_3DRG_CHARWIDTH*2;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_TH_MIN);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Slider threshold
	contrlRect.left = contrlRect.right + SV_3DRG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_3DRG_SMALLSLIDERWIDTH - 2*SV_3DRG_CHARWIDTH;
	m_sldThreshold.MoveWindow(&contrlRect, FALSE);
	m_rThres = contrlRect;

	// Label current threshold value
	m_sldThreshold.GetThumbRect(&infoRect);
	infoRect += contrlRect.TopLeft();
	infoRect.top += SV_3DRG_SMALLSLIDERHEIGHT; 
	infoRect.bottom += SV_3DRG_SMALLSLIDERHEIGHT; 
	infoRect.right += SV_3DRG_CHARWIDTH *3;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_TH_CUR);
	ctrlStatic->MoveWindow(&infoRect, FALSE);	
	CString str;
	str.Format(_T("%d"), m_sldThreshold.GetPos());
	ctrlStatic->SetWindowText(str);

	// Label max threshold
	contrlRect.left = contrlRect.right + SV_3DRG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_3DRG_CHARWIDTH *2;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_TH_MAX);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);	

	// Label start slice
	contrlRect = m_rThres;
	contrlRect.left -= (SV_3DRG_CHARWIDTH*2 + SV_3DRG_SMALL_SPACING);
	contrlRect.top += (1.5*SV_3DRG_SMALLSLIDERHEIGHT + SV_3DRG_SPACING);
	contrlRect.right = contrlRect.left + SV_3DRG_CHARWIDTH*3;
	contrlRect.bottom = contrlRect.top + SV_3DRG_TEXTHEIGHT;
	ctrlStatic = (CStatic*) GetDlgItem(IDC_3DRG_LBLSTART);
	ctrlStatic->MoveWindow(contrlRect);

	// Textbox start slice
	contrlRect.left = contrlRect.right + SV_3DRG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_3DRG_TEXTBOXWIDTH;
	CEdit* ctrlEdit = (CEdit*) GetDlgItem(IDC_3DRG_STARSLICE);
	ctrlEdit->MoveWindow(contrlRect);

	// Label end slice
	contrlRect.left = contrlRect.right + SV_3DRG_SPACING;
	contrlRect.right = contrlRect.left + SV_3DRG_CHARWIDTH * 3;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_3DRG_LBLEND);
	ctrlStatic->MoveWindow(contrlRect);

	// textbox end slice
	contrlRect.left = contrlRect.right + SV_3DRG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_3DRG_TEXTBOXWIDTH;
	ctrlEdit = (CEdit*) GetDlgItem(IDC_3DRG_ENDSLICE);
	ctrlEdit->MoveWindow(contrlRect);
	

	// Boundary threshold label
	contrlRect.left = m_rThres.left - (SV_3DRG_CHARWIDTH*2 + SV_3DRG_SMALL_SPACING);
	contrlRect.top = contrlRect.bottom + SV_3DRG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_3DRG_TEXTHEIGHT;
	contrlRect.right = contrlRect.left + SV_3DRG_CHARWIDTH*9;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_3DRG_BTHRES);
	ctrlStatic->MoveWindow(contrlRect);

	//
	// Threshold  slider
	contrlRect.top = contrlRect.bottom + SV_3DRG_SMALL_SPACING;
	contrlRect.bottom = contrlRect.top + SV_3DRG_SMALLSLIDERHEIGHT;
	contrlRect.right = contrlRect.left + SV_3DRG_CHARWIDTH*2;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_TH_MINBTHRES);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);
	
	// Slider boundary threshold
	contrlRect.left = contrlRect.right + SV_3DRG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_3DRG_SMALLSLIDERWIDTH - 2*SV_3DRG_CHARWIDTH;
 	m_sldBoundThres.MoveWindow(&contrlRect, FALSE);
 	m_rBthres = contrlRect;


	// Label current boundary threshold value
	m_sldBoundThres.GetThumbRect(&infoRect);
	infoRect += contrlRect.TopLeft();
	infoRect.top += SV_3DRG_SMALLSLIDERHEIGHT; 
	infoRect.bottom += SV_3DRG_SMALLSLIDERHEIGHT; 
	infoRect.right += SV_3DRG_CHARWIDTH *3;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_LBL_CURBTHRE);
	ctrlStatic->MoveWindow(&infoRect, FALSE);	
	str.Format(_T("%d"), m_sldBoundThres.GetPos());
	ctrlStatic->SetWindowText(str);
	
	
	// Label max threshold
	contrlRect.left = contrlRect.right + SV_3DRG_SMALL_SPACING;
	contrlRect.right = contrlRect.left + SV_3DRG_CHARWIDTH *2;
	ctrlStatic =(CStatic*)GetDlgItem(IDC_TH_MAXBTHRES);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);	


	//////////////////////////////////////////////////////////////////////////
	// Compute the Operation mode
	contrlRect.left = clientRect.left + SV_3DRG_SPACING*2 + SV_3DRG_GRP_WIDTH *2;
	contrlRect.bottom = clientRect.bottom - SV_3DRG_SPACING;
	contrlRect.top = clientRect.bottom - SV_3DRG_GRP_HEIGHT;
	contrlRect.right = contrlRect.left + SV_3DRG_GRP_WIDTH;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_3DRG_OPPGRP);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Cutting mode radio
	contrlRect.left += SV_3DRG_SPACING;
	contrlRect.top += SV_3DRG_SPACING * 2;
	contrlRect.right -= SV_3DRG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_3DRG_TEXTHEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_3DRG_RAD_CUT);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Segmentation mode radio
	contrlRect.top = contrlRect.bottom + SV_3DRG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_3DRG_TEXTHEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_3DRG_RAD_SEG);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Cutting method combobox label
	contrlRect.top = contrlRect.bottom + SV_3DRG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_3DRG_TEXTHEIGHT;
	ctrlStatic = (CStatic*)GetDlgItem(IDC_3DRG_LBL_CUTMED);
	ctrlStatic->MoveWindow(&contrlRect, FALSE);

	// Cutting method combobox
	contrlRect.top = contrlRect.bottom + SV_3DRG_SPACING/2;
	contrlRect.bottom = contrlRect.top + SV_3DRG_TEXTHEIGHT;
	ctrlCombobox = (CComboBox*)GetDlgItem(IDC_3DRG_CB_CUTMODE);
	ctrlCombobox->MoveWindow(&contrlRect, FALSE);

	// Boundary detection check box
	contrlRect.top = contrlRect.bottom + SV_3DRG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_3DRG_TEXTHEIGHT;
	ctrlButton = (CButton*) GetDlgItem(IDC_3DRG_CHK_BOUND);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Segment start button
	contrlRect.top = contrlRect.bottom + SV_3DRG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_3DRG_BUTTON_HEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_3DRG_START);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	//////////////////////////////////////////////////////////////////////////
	// Compute the button set
	// Close button
	contrlRect.right = clientRect.right - SV_3DRG_SPACING;
	contrlRect.left = contrlRect.right - SV_3DRG_BUTTON_WIDTH;
	contrlRect.bottom = clientRect.bottom - SV_3DRG_SPACING;
	contrlRect.top = contrlRect.bottom - SV_3DRG_BUTTON_HEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDCANCEL);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Save volume button
	contrlRect.bottom = contrlRect.top - SV_3DRG_SPACING*2;
	contrlRect.top = contrlRect.bottom - SV_3DRG_BUTTON_HEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_3DRG_CMD_SAVEVOL);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Save all slice button
	contrlRect.bottom = contrlRect.top - SV_3DRG_SPACING/2;
	contrlRect.top = contrlRect.bottom - SV_3DRG_BUTTON_HEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_3DRG_CMD_SAVEALL);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	// Save a slice button
	contrlRect.bottom = contrlRect.top - SV_3DRG_SPACING/2;
	contrlRect.top = contrlRect.bottom - SV_3DRG_BUTTON_HEIGHT;
	ctrlButton = (CButton*)GetDlgItem(IDC_3DRG_CMD_SAVESINGLE);
	ctrlButton->MoveWindow(&contrlRect, FALSE);

	//Compute the image output size
	int nHeight = clientRect.Height() - (5*SV_3DRG_SPACING + SV_3DRG_GRP_HEIGHT + SV_3DRG_LARGESLIDERHEIGHT);
	int nWidth = (clientRect.Width() - 3*SV_3DRG_SPACING) /2;
	int nSize = min(nHeight, nWidth);

	m_OrgRegion.left = clientRect.left + SV_3DRG_SPACING;
	m_OrgRegion.top = clientRect.top + SV_3DRG_SPACING;
	m_OrgRegion.right = m_OrgRegion.left + nSize;
	m_OrgRegion.bottom = m_OrgRegion.top + nSize;
	m_ResRegion = m_OrgRegion;
	m_ResRegion.OffsetRect(nSize + SV_3DRG_SPACING, 0);

	ctrlStatic = (CStatic*)GetDlgItem(IDC_3DRG_ORGFRAME);
	ctrlStatic->MoveWindow(&m_OrgRegion, FALSE);
	ctrlStatic = (CStatic*)GetDlgItem(IDC_3DRG_RESFRAME);
	ctrlStatic->MoveWindow(&m_ResRegion, FALSE);

	contrlRect.left = m_OrgRegion.left;
	contrlRect.right = m_ResRegion.right;
	contrlRect.top = m_OrgRegion.bottom + SV_3DRG_SPACING;
	contrlRect.bottom = contrlRect.top + SV_3DRG_LARGESLIDERHEIGHT;
	m_sldSliceNo.MoveWindow(&contrlRect, FALSE);
	m_OrgRegion.InflateRect(-3,-3);
	m_ResRegion.InflateRect(-3,-3);

	// Update frame for the cutting object
	m_oCutter.SetFrame(m_OrgRegion);
}

void CDlg3RGSegment::UpdateLUT()
{
	CImgLib::GenerateLinearLUT(m_nLUT, m_sldBright.GetPos(), m_sldContrast.GetPos());
}


CPoint CDlg3RGSegment::ImageToScreen( CPoint imagePoint, CRect in_Rect )
{
	CPoint screenPos(0,0);
	int imgW = m_pDataSet->GetSize().ndx;
	int imgH = m_pDataSet->GetSize().ndy;

	DOUBLE rate = 1;
	rate = ((DOUBLE)(imagePoint.x))/imgW;
	screenPos.x = rate * m_OrgRegion.Width() + m_OrgRegion.left + 0.5;
	rate = ((DOUBLE)(imagePoint.y))/imgH ;
	screenPos.y= rate * m_OrgRegion.Height() + m_OrgRegion.top + 0.5;

	return screenPos;
}

CPoint CDlg3RGSegment::ScreenToImage( CPoint screenPoint, CRect in_Rect )
{
	CPoint imgPos(0,0);

	DOUBLE rate =1;
	rate = ((DOUBLE)(screenPoint.x - in_Rect.left))/in_Rect.Width();
	imgPos.x = rate * m_pDataSet->GetSize().ndx  + 0.5;
	rate = ((DOUBLE)(screenPoint.y - in_Rect.top))/in_Rect.Height();
	imgPos.y= rate * m_pDataSet->GetSize().ndy + 0.5;

	return imgPos;
}

void CDlg3RGSegment::GetRawSliceFromDataSet( int nPos )
{

}
// CDlg3RGSegment message handlers

void CDlg3RGSegment::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if((m_bInitDone)&&(nType !=  SIZE_MINIMIZED)) 
	{

		UpdateLayout(cx,cy);
		Invalidate();
	}
	// TODO: Add your message handler code here
}

BOOL CDlg3RGSegment::OnInitDialog()
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
	m_sldThreshold.SetRange(SV_3DRG_MIN, SV_3DRG_MAX, TRUE); 
	m_sldThreshold.SetPos(SV_3DRG_DEF);
	m_nThresholdVal = SV_3DRG_DEF;

	m_chkDefaultThres.SetCheck(BST_CHECKED);
	m_sldThreshold.EnableWindow(m_chkDefaultThres.GetCheck() == BST_UNCHECKED);

	
	m_sldBoundThres.SetRange(SV_3DRG_MIN, SV_3DRG_MAX, TRUE);
	m_sldBoundThres.SetPos(SV_3DRG_DEF);


	// Init the dialog size and position
	CRect mYRec(50,50, 50+ SV_3DRG_DLGINITWIDTH, 50+SV_3DRG_DLGINITHEIGHT);
	CalcWindowRect(&mYRec);
	SetWindowPos(&CWnd::wndTop, mYRec.left, mYRec.top, mYRec.Width(), mYRec.Height(),SWP_SHOWWINDOW);
	m_bInitDone = TRUE;

	// Init control variables
	m_bBoundDetect = FALSE;
	m_cbbCutMethod.SetCurSel(0);
	m_nMode = 1;
	m_nStartSlc = 0;
	m_nEndSlc = m_pDataSet->GetSize().ndz;
	GetDlgItem(IDC_3DRG_START)->EnableWindow(FALSE);
	UpdateData(FALSE);

	UpdateLayout(SV_3DRG_DLGINITWIDTH, SV_3DRG_DLGINITHEIGHT);
	UpdateSlices();
	return TRUE;  
}

void CDlg3RGSegment::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	
	DisplayImage();
}

void CDlg3RGSegment::OnBnClickedRgChkDef()
{
	// TODO: Add your control notification handler code here	
	if(m_chkDefaultThres.GetCheck() == BST_CHECKED) 
		m_nThresholdVal = SV_3DRG_DEF;
	else 
		m_nThresholdVal = m_sldThreshold.GetPos();

	m_sldThreshold.EnableWindow(m_chkDefaultThres.GetCheck() == BST_UNCHECKED);
	UpdateSlices();
	DisplayImage();
}

void CDlg3RGSegment::OnBnClickedRgChkPreproc()
{
	// TODO: Add your control notification handler code here
	m_sldBright.EnableWindow(m_chkPreproc.GetCheck() == BST_CHECKED);
	m_sldContrast.EnableWindow(m_chkPreproc.GetCheck() == BST_CHECKED);

	UpdateSlices();
	DisplayImage();
}

void CDlg3RGSegment::OnBnClickedRgCmdSaveall()
{

}

void CDlg3RGSegment::OnBnClickedRgCmdSavesingle()
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
// 			CFileException ex;
// 			BOOL bflagOpen = oFile.Open(dlgSave.GetPathName(), CFile::modeWrite | CFile::modeCreate, &ex);
// 			if(bflagOpen)
// 			{
// 				UINT nLength = m_pDataSet->GetSize().ndx*m_pDataSet->GetSize().ndy;
// 				oFile.Write(m_pResByteImg, nLength);	
// 				oFile.Flush();
// 				oFile.Close();
// 			}	
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

void CDlg3RGSegment::OnBnClickedRgCmdSavevol()
{
	CString strOutputFile;	
	strOutputFile.Format(_T("%s_%dx%dx%d"), SV_EXP_DEFAULTNAME, m_tSubVol.size3D.ndx, m_tSubVol.size3D.ndy, m_tSubVol.size3D.ndz);
	CString strFilters = _T("Raw files (*.raw)|*.raw||");
	CFileDialog dlgSave(FALSE, _T(".raw"), strOutputFile, OFN_OVERWRITEPROMPT, strFilters);

	CFile oFile;
	if (dlgSave.DoModal() == IDOK)
	{
		CFileException ex;
		BOOL bflagOpen = oFile.Open(dlgSave.GetPathName(), CFile::modeWrite | CFile::modeCreate, &ex);
		if(bflagOpen)
		{
			UCHAR* pBuffer1 = NULL;
			UCHAR* pBuffer2 = NULL;
			UCHAR* pCurOrgSlice = NULL;
			UCHAR* pCurSubSlice = NULL;
			UCHAR temp;

			pBuffer1 = new UCHAR[m_tSubVol.size3D.ndx * m_tSubVol.size3D.ndy];
			pBuffer2 = new UCHAR[m_tSubVol.size3D.ndx * m_tSubVol.size3D.ndy];
			INT nOrgFrameSize = m_pDataSet->GetSize().ndx* m_pDataSet->GetSize().ndy;
			INT nSubFrameSize = m_tSubVol.size3D.ndx * m_tSubVol.size3D.ndy;
			
			// for each slice in sub-volume
			for (int i=0; i<m_tSubVol.size3D.ndz; i++)
			{
				// Obtain the corresponding slice of the original volume
				pCurOrgSlice = m_pDataSet->GetDataBuff() + (i + m_tSubVol.pos3D.nZ) * nOrgFrameSize;
				
				// Obtain the original sub-slice
				for (int j=0; j< m_tSubVol.size3D.ndy; j++)
				{
					UINT orgY = j + m_tSubVol.pos3D.nY;
					UINT orgX = m_tSubVol.pos3D.nX;
					CopyMemory(pBuffer1 + j * m_tSubVol.size3D.ndx , pCurOrgSlice + orgY * m_pDataSet->GetSize().ndx + orgX, m_tSubVol.size3D.ndx);
				}

				// Make a mask to the buffer
				Cutout(pBuffer1, pBuffer2, m_tSubVol.size3D.ndx, m_tSubVol.size3D.ndy);


				// Do the masking
				pCurSubSlice = m_tSubVol.pData + i * nSubFrameSize;
				CImgLib::Mask(pCurSubSlice, pBuffer1, pBuffer2, m_tSubVol.size3D.ndx, m_tSubVol.size3D.ndy, 255);

				// Write slice to the file
				oFile.Write(pBuffer1, nSubFrameSize);
			}

			oFile.Flush();
			oFile.Close();
		}	
	}
}

void CDlg3RGSegment::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	CString str;
	CRect thumb;
	CStatic* pStatic;
	//UpdatePreproc();
	if ((m_chkDefaultThres.GetCheck() == BST_UNCHECKED) && (m_sldThreshold.GetPos() != m_nThresholdVal))
	{
		m_nThresholdVal = m_sldThreshold.GetPos();
		m_sldThreshold.GetThumbRect(&thumb);
		thumb += m_rThres.TopLeft();
		thumb += CPoint(0, SV_3DRG_TEXTHEIGHT);
		thumb.right += SV_3DRG_CHARWIDTH*3;
		pStatic = (CStatic*) GetDlgItem(IDC_TH_CUR);
		pStatic->MoveWindow(&thumb);
		str.Format(_T("%d"), m_nThresholdVal);
		pStatic->SetWindowText(str);		
	}

	m_sldBoundThres.GetThumbRect(&thumb);
	thumb += m_rBthres.TopLeft();
	thumb += CPoint(0, SV_3DRG_TEXTHEIGHT);
	thumb.right += SV_3DRG_CHARWIDTH*3;
	pStatic = (CStatic*) GetDlgItem(IDC_LBL_CURBTHRE);
	pStatic->MoveWindow(&thumb);
	str.Format(_T("%d"), m_sldBoundThres.GetPos());
	pStatic->SetWindowText(str);	

	UpdateLUT();
	UpdateSlices();
	DisplayImage();	
}


void CDlg3RGSegment::OnLButtonDown( UINT nFlags, CPoint point )
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
				//m_oCutter.AddPoint(ScreenToImage(point, m_OrgRegion));
			}
	}
	// Picking seed points
	else 
	{
		if(m_OrgRegion.PtInRect(point))
		{
			CPoint imgPos = ScreenToImage(point, m_OrgRegion);
			ASSERT((imgPos.x >= 0) && (imgPos.x < m_pDataSet->GetSize().ndx));
			ASSERT((imgPos.y >= 0) && (imgPos.y < m_pDataSet->GetSize().ndy));

			//Check if the picked point is in the cutting area
			if (m_pCuttingMask[imgPos.y * m_pDataSet->GetSize().ndx + imgPos.x] == 255) //
			{
				// Put the point in to the seed points list
				CVolPos cvPos(imgPos.x, imgPos.y, m_sldSliceNo.GetPos());
				m_seedPoints.push_back(cvPos);			
				DisplayImage();
			}
		}
	}
}

void CDlg3RGSegment::OnMouseMove( UINT nFlags, CPoint point )
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

void CDlg3RGSegment::OnRButtonDblClk( UINT nFlags, CPoint point )
{
	// If it is delete result action
	if(m_ResRegion.PtInRect(point))
	{
		// do nothing...not be considered yet
	}

	// If it is delete path action
	if(m_OrgRegion.PtInRect(point))
	{
		m_oCutter.ResetCut();		
		FillMemory(m_pCuttingMask, m_pDataSet->GetSize().ndx *m_pDataSet->GetSize().ndy ,255);
		m_seedPoints.clear();
		UpdateSlices();
		DisplayImage();
	}
	CDialog::OnRButtonDblClk(nFlags, point);
}

void CDlg3RGSegment::OnBnClickedRad()
{
	UpdateData(TRUE);
	if (m_nMode == m_nPreMode) return;

	// From Segmentation --> Cutting mode
	if (m_nMode == 1)
	{
	// Segmentation --> cutting mode
		GetDlgItem(IDC_3DRG_START)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_3DRG_START)->EnableWindow(TRUE);
		// Generate the cutting mask
		if (m_oCutter.GetNumOfPoint() > 0)
		{
			m_oCutter.GenerateMask(m_pCuttingMask, m_pDataSet->GetSize().ndx, m_pDataSet->GetSize().ndy);
		}		
	}
	m_nPreMode = m_nMode;
}

void CDlg3RGSegment::OnLButtonUp( UINT nFlags, CPoint point )
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

void CDlg3RGSegment::OnRButtonDown( UINT nFlags, CPoint point )
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

void CDlg3RGSegment::OnCbnSelchangeRgCbCutmode()
{
	// TODO: Add your control notification handler code here
	m_oCutter.SetMode(m_cbbCutMethod.GetCurSel());
	TRACE1("%d\t", m_cbbCutMethod.GetCurSel());
}




void CDlg3RGSegment::OnBnClicked3drgStart()
{
	// TODO: Add your control notification handler code here
/*
#ifdef _DEBUG
	for (int i =0; i< m_seedPoints.size(); i++)
	{
		CVolPos& pos = m_seedPoints.at(i);
		TRACE3("\nSeed position: [%d, %d, %d]", pos.nX, pos.nY, pos.nZ);
		UCHAR* pVolume = m_pDataSet->GetDataBuff();
		TRACE1("\Seed value: [%d]", pVolume[pos.nZ * m_pDataSet->GetSize().ndx * m_pDataSet->GetSize().ndy + pos.nY * m_pDataSet->GetSize().ndx + pos.nX]);
	}
#endif // _DEBUG
//*/

	//Compute to allocate memory for the sub-volume
	CRect subRect = CImgLib::GetRegion2DBB(m_pCuttingMask, m_pDataSet->GetSize().ndx, m_pDataSet->GetSize().ndy, 255);
		
	// ===== Create sub volume ==== 
	// make sub-volume information 
	m_tSubVol.pos3D.nX = subRect.left;
	m_tSubVol.pos3D.nY = subRect.top;
	m_tSubVol.pos3D.nZ = m_nStartSlc;
	m_tSubVol.size3D.ndx = subRect.Width();
	m_tSubVol.size3D.ndy = subRect.Height();
	m_tSubVol.size3D.ndz = m_nEndSlc - m_nStartSlc;

/*
#ifdef _DEBUG
	TRACE("\nSub-volume information:");
	TRACE3("\n\tPosition: [%d, %d, %d]", m_tSubVol.pos3D.nX, m_tSubVol.pos3D.nY, m_tSubVol.pos3D.nZ);
	TRACE3("\n\tVolume Size: [%d, %d, %d]", m_tSubVol.size3D.ndx, m_tSubVol.size3D.ndy, m_tSubVol.size3D.ndz);
#endif // _DEBUG
//*/

	// Allocate memory for sub-volume
	if (m_tSubVol.pData != NULL)
	{
		delete[] m_tSubVol.pData;
		m_tSubVol.pData = NULL;
	}
	m_tSubVol.pData = new UCHAR[m_tSubVol.size3D.ndx * m_tSubVol.size3D.ndy * m_tSubVol.size3D.ndz];
	FillMemory(m_tSubVol.pData,m_tSubVol.size3D.ndx * m_tSubVol.size3D.ndy * m_tSubVol.size3D.ndz, 0);
			
	// Run region growing algorithm with sub volume
	int nMode = 0;
	if (!m_seedPoints.empty())
	{
		CImgLib::RG3DEx(m_pDataSet->GetDataBuff(),	m_pDataSet->GetSize(),	m_pCuttingMask,					
			m_tSubVol.pData, m_tSubVol.pos3D, m_tSubVol.size3D,	
			m_seedPoints, m_sldThreshold.GetPos() ,nMode);		
	}

	
}


void CDlg3RGSegment::DrawSeedPoints( void )
{
	CDC* pDC = GetDC();
	HDC hDC = pDC->GetSafeHdc();
	Graphics graph(*pDC);
	Pen pen(Color(0, 255, 0), 2);
	Pen darkPen(Color(0, 128, 0), 2);

	for (int i=0; i < m_seedPoints.size(); i++ )
	{
		CVolPos& pos = m_seedPoints.at(i);
		CPoint imgPoint(pos.nX, pos.nY);
		CPoint screenPoint = ImageToScreen(imgPoint, m_OrgRegion);

		// Draw this seed point
/*
#ifdef _DEBUG
		TRACE3("\n[%d, %d , %d]", pos.nX, pos.nY, pos.nZ);			
#endif // _DEBUG
*/
		if (pos.nZ == m_sldSliceNo.GetPos())
		{
			graph.DrawLine(&pen, screenPoint.x-5, screenPoint.y, screenPoint.x+5, screenPoint.y);
			graph.DrawLine(&pen, screenPoint.x, screenPoint.y-5, screenPoint.x, screenPoint.y+5);
		}
		else
		{
			graph.DrawLine(&darkPen, screenPoint.x-5, screenPoint.y, screenPoint.x+5, screenPoint.y);
			graph.DrawLine(&darkPen, screenPoint.x, screenPoint.y-5, screenPoint.x, screenPoint.y+5);
		}
	}
}

void CDlg3RGSegment::Cutout( UCHAR* pSource, UCHAR* pResult, int width, int height )
{
	ASSERT(pSource != NULL);
	ASSERT(pResult != NULL);

	int memSize = width* height;
	UCHAR* pBufferImg1 = new UCHAR[memSize];

	//***********************************************************************
	// ORIGINAL ALGORITHM 

	// Binarize the image save to buffer 1
	CImgLib::Binary(pSource, pBufferImg1, width, height, m_sldBoundThres.GetPos());

	// Detect the boundary save to buffer 2
	CImgLib::BinBoundaryDetect(pBufferImg1, pResult, width, height);

	delete [] pBufferImg1;
}
