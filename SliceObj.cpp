#include "StdAfx.h"
#include "SliceObj.h"
#include "Utility.h"
#include <GdiPlus.h>
using namespace Gdiplus;

CViewSliceObj::CViewSliceObj(void)
: m_pPatInfo(NULL), m_nType(eUndef), m_pOrgData(NULL), m_pProcessedData(NULL)
, m_bReDrawFrame(FALSE)
{
	memset(&m_bmpInfo,0, sizeof(BITMAPINFOHEADER));
	m_nType = eUndef;
	m_nSlcIndex = 0;
	m_bReDrawFrame = TRUE;
}

//************************************
// Method:    ~CSliceObj
// FullName:  CSliceObj::~CSliceObj
// Access:    public 
// Returns:   
// Qualifier:
// Parameter: void
// Purpose:   
//************************************
CViewSliceObj::~CViewSliceObj(void)
{
	if (m_pOrgData != NULL)
	{
		delete []m_pOrgData;
		m_pOrgData = NULL;
	}

	if (m_pProcessedData != NULL)
	{
		delete []m_pProcessedData;
		m_pProcessedData = NULL;
	}
}

//************************************
// Method:    Draw
// FullName:  CSliceObj::Draw
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CDC * pDC
// Purpose:   
//************************************
int CViewSliceObj::Draw(CDC* pDC)
{	
	if(m_bReDrawFrame) 
	{
		m_objSlcFrame.Draw(pDC);
		m_bReDrawFrame = FALSE;
	}
	HDC hDC = pDC->GetSafeHdc();
	CRect imgRegion(m_objSlcFrame.GetImageRegion());
// 	int errCode = ::StretchDIBits(hDC,
// 						imgRegion.left,imgRegion.top,imgRegion.Width(),imgRegion.Height(),
// 						0,0,m_bmpInfo.biWidth,m_bmpInfo.biHeight,
// 						m_pProcessedData,
// 						(PBITMAPINFO)&m_bmpInfo,
// 						DIB_RGB_COLORS,
// 						SRCCOPY);
	
	BITMAPINFO bm;
	memset(&bm, 0, sizeof(BITMAPINFO));
	bm.bmiHeader = m_bmpInfo;
	Bitmap* pBmp = Bitmap::FromBITMAPINFO(&bm,m_pProcessedData);
	Graphics graph(*pDC);

	graph.DrawImage(pBmp, imgRegion.left, imgRegion.top, imgRegion.Width(), imgRegion.Height());
	delete pBmp;
	
	m_objCtrlLine.Draw(pDC);

	//OUTPUT TEXT INFOR
	COLORREF prebkColor = pDC->SetBkColor(COLORREF(0x000000));	
	COLORREF preColor = pDC->SetTextColor(COLORREF(0xFFFFFF));
	CString str = CUtility::GetSliceName(m_nType);
	// Text out the slice name
	pDC->TextOut(m_objSlcFrame.GetBoundary().left,m_objSlcFrame.GetBoundary().top, str);

	if (m_nType != eVolume)
	{
		// Text out the slice index
		str.Format(_T("Slice No.: %d   "), m_nSlcIndex);	
		pDC->TextOut(m_objSlcFrame.GetBoundary().left, m_objSlcFrame.GetBoundary().top + 16, str);
	}
	pDC->SetTextColor(preColor);
	pDC->SetBkColor(prebkColor);

	return 0;
}

//************************************
// Method:    UpdateFrame
// FullName:  CSliceObj::UpdateFrame
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: RECT in_rcBoundary
// Purpose:   
//************************************
int CViewSliceObj::UpdateFrame( RECT in_rcBoundary )
{
	INT retcode = SV_NORMAL;
	CRect boundary(in_rcBoundary);
	CSize imgSize(m_bmpInfo.biWidth,m_bmpInfo.biHeight);
	CRect imgRect(0,0,0,0);
	CSize szImage;

	SLCFRAME slcFrm;
	slcFrm.clrBound = SV_SLC_BORDER_CLR;
	slcFrm.clrBgrnd = SV_SLC_FRMBACK;

	boundary.InflateRect(-SV_SLC_BORDER,-SV_SLC_BORDER);
	slcFrm.rcBound = boundary;

	CRect rcImgBound(slcFrm.rcBound);

	if(rcImgBound.IsRectEmpty()) retcode = SV_INVALID_PARAM;

	if((imgSize.cx*imgSize.cy) == 0) retcode = SV_INVALID_PARAM;
	
	if (retcode == SV_NORMAL)
	{
		DOUBLE dRatio = 1;
		if ((rcImgBound.Height()*imgSize.cx) >= (rcImgBound.Width()*imgSize.cy ))//width ratio
		{
			dRatio = (DOUBLE)rcImgBound.Width()/(DOUBLE)imgSize.cx;
		}
		else	//height ratio
		{
			dRatio = (DOUBLE)rcImgBound.Height()/(DOUBLE)imgSize.cy;
		}
		
		szImage.cx = (dRatio * (DOUBLE)imgSize.cx);
		szImage.cy = (dRatio * (DOUBLE)imgSize.cy);

		if((szImage.cx * szImage.cy) == 0) retcode = SV_INVALID_PARAM;
	}

	if (retcode == SV_NORMAL)
	{
		imgRect.left = rcImgBound.left + (rcImgBound.Width() - szImage.cx)/2;
		imgRect.right = imgRect.left + szImage.cx;
		imgRect.top = rcImgBound.top + (rcImgBound.Height()-szImage.cy)/2;
		imgRect.bottom = imgRect.top + szImage.cy;
		slcFrm.rcImage = imgRect;
		retcode = m_objSlcFrame.Update(slcFrm);
		m_bReDrawFrame = TRUE;
	}
	
	return retcode; 
}

//************************************
// Method:    UpdateResultSlice
// FullName:  CSliceObj::UpdateResultSlice
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CSliceProc * in_pProc
// Purpose:   
//************************************
int CViewSliceObj::UpdateResultSlice(CSliceProc* in_pProc)
{
	return 0;
}

//************************************
// Method:    PrepareMem
// FullName:  CSliceObj::PrepareMem
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CSize in_size
// Purpose:   
//************************************
// RISK: what if the m_pOrgData is not null, but point to wrong size [2/2/2009 QUYPS]
int CViewSliceObj::PrepareMem(CSize in_size)
{
	int retcode = SV_NORMAL;
	
// Fix the bug of bitmap shearing [4/30/2009 QUYPS]
/*
	Prob: the row size must be multiple of four since the window DWORD is 32 bits
	Equation to calculate the size of memory: http://en.wikipedia.org/wiki/BMP_file_format

*/	

	INT nRowSize;
	nRowSize = (24 * in_size.cx + sizeof(DWORD) - 1)/sizeof(DWORD);
	nRowSize *= 4;
	UINT nImgSize_bytes = nRowSize * in_size.cy;
	//CUtility::DeleteMem(m_pOrgData);
	if(m_pOrgData == NULL)
		m_pOrgData = new UCHAR[ nImgSize_bytes ];

	if(m_pOrgData == NULL)	retcode = SV_MEMORY_ERR;
	else
	{
		memset(m_pOrgData,0,nImgSize_bytes);
		if(m_pProcessedData == NULL)
			m_pProcessedData = new UCHAR[ nImgSize_bytes ];
		if (m_pProcessedData == NULL)
		{
			retcode = SV_MEMORY_ERR;
			CUtility::DeleteMem(m_pOrgData);
		}
		else
		{
			//memset(m_pProcessedData,0,in_size.cx * in_size.cy * 3);
		}
	}
	return retcode;
}
//************************************
// Method:    UpdateLineCtrl
// FullName:  CSliceObj::UpdateLineCtrl
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: VOLPOS in_3dPos
// Purpose:   
//************************************
int CViewSliceObj::UpdateLineCtrl( VOLPOS in_3dPos )
{
	POINT ptRealpos;
	ptRealpos.x = 0;
	ptRealpos.y = 0;
	switch(m_nType)
	{
	case eAxial:	//XY plane
		ptRealpos.x = in_3dPos.nX;
		ptRealpos.y = in_3dPos.nY;
		break;

	case eCoronal:	//XZ
		ptRealpos.x = in_3dPos.nX;
		ptRealpos.y = in_3dPos.nZ;
		break;
	case eSagittal:	//YZ
		ptRealpos.x = in_3dPos.nY;
		ptRealpos.y = in_3dPos.nZ;
	    break;
	default:
	    break;
	}
	
	CPoint ptDisplayPos = ImagePos2FramePos(ptRealpos);
//	CRect displayRegion = m_objSlcFrame.GetImageRegion();
// 	DOUBLE ratio = (DOUBLE)displayRegion.Width()/(DOUBLE)m_bmpInfo.biWidth;
// 	ptDisplayPos.x = ratio * ptRealpos.x;
// 	ptDisplayPos.y = ratio * ptRealpos.y;

	m_objCtrlLine.SetPos(ptDisplayPos);
	m_objCtrlLine.SetFrame(m_objSlcFrame.GetImageRegion());

	return 0;
}

//************************************
// Method:    IsOnMouse
// FullName:  CSliceObj::IsOnMouse
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: CPoint in_ptMousePos
// Purpose:   TRUE if input point in on the image area
//			  FALSE if not
//************************************
BOOL CViewSliceObj::IsOnMouse(CPoint in_ptMousePos)
{
	return m_objSlcFrame.IsOnImage(in_ptMousePos);
}

//************************************
// Method:    ImagePos2FramePos
// FullName:  CSliceObj::ImagePos2FramePos
// Access:    public  
// Returns:   CPoint
// Qualifier:
// Parameter: CPoint in_imgpoint
// Purpose:   
//************************************
CPoint CViewSliceObj::ImagePos2FramePos(CPoint in_imgpoint)
{
	CPoint ptDisplayPos(0,0);

	CRect displayRegion = m_objSlcFrame.GetImageRegion();
 	DOUBLE ratio = (DOUBLE)displayRegion.Width()/(DOUBLE)m_bmpInfo.biWidth;
 	ptDisplayPos.x = ratio * in_imgpoint.x;
	ptDisplayPos.y = ratio * in_imgpoint.y;

	if(ptDisplayPos.x > displayRegion.Width()) 
		ptDisplayPos.x =  displayRegion.Width();
	else if (ptDisplayPos.x < 0) 
		ptDisplayPos.x =  0;

	if(ptDisplayPos.y > displayRegion.Height()) 
		ptDisplayPos.y =  displayRegion.Height();
	else if (ptDisplayPos.x < 0) 
		ptDisplayPos.y =  0;

	return ptDisplayPos;
}

//************************************
// Method:    FramePos2ImagePos
// FullName:  CSliceObj::FramePos2ImagePos
// Access:    public 
// Returns:   CPoint
// Qualifier:
// Parameter: CPoint in_FrmPoint: current console position in image are on screen
// Purpose:   to translate from screen coordinate to real-image coordinate
//************************************
CPoint CViewSliceObj::FramePos2ImagePos(CPoint in_FrmPoint)
{

	CPoint ptRealPos(0,0);

	CRect displayRegion = m_objSlcFrame.GetImageRegion();
	DOUBLE ratio = (DOUBLE)m_bmpInfo.biWidth/(DOUBLE)displayRegion.Width();

	ptRealPos.x = ratio * in_FrmPoint.x;
	ptRealPos.y = ratio * in_FrmPoint.y;

	if(ptRealPos.x >= m_bmpInfo.biWidth) 
		ptRealPos.x =  m_bmpInfo.biWidth-1;
	else if (ptRealPos.x < 0) 
		ptRealPos.x =  0;

	if(ptRealPos.y >= m_bmpInfo.biHeight) 
		ptRealPos.y =  m_bmpInfo.biHeight-1;
	else if (ptRealPos.x < 0) 
		ptRealPos.y =  0;

	return ptRealPos;
}

//************************************
// Method:    ViewPos2ImagePos
// FullName:  CSliceObj::ViewPos2ImagePos
// Access:    public 
// Returns:   CPoint
// Qualifier:
// Parameter: CPoint in_viewpos
// Purpose:   
//************************************
CPoint CViewSliceObj::ViewPos2ImagePos(CPoint in_viewpos)
{
	CPoint rePos(0,0);
	CPoint localFrmPos(0,0);
	localFrmPos.x = in_viewpos.x - m_objSlcFrame.GetImageRegion().left;
	localFrmPos.y = in_viewpos.y - m_objSlcFrame.GetImageRegion().top;
	rePos = FramePos2ImagePos(localFrmPos);
	
	return rePos;
}

//************************************
// Method:    GetSize
// FullName:  CSliceObj::GetSize
// Access:    public 
// Returns:   CSize
// Qualifier:
// Purpose:   
//************************************
CSize CViewSliceObj::GetSize()
{
	CSize size;
	size.cx = m_bmpInfo.biWidth;
	size.cy = m_bmpInfo.biHeight;
	return size;
}