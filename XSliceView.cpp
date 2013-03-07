// XSliceView.cpp : implementation of the CXSliceView class
//

#include "stdafx.h"
#include "SliceView.h"

#include "XSliceDoc.h"
#include "XSliceView.h"
#include "SliceDlgBar.h"

// TEST [2/11/2008 QUYPS]
#include "DCMAccess.h"
#include "RAWAccess.h"
#include "BufferDC.h"

#include "CollaborationControl.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CXSliceView

IMPLEMENT_DYNCREATE(CXSliceView, CView)

BEGIN_MESSAGE_MAP(CXSliceView, CView)
	//Custom message
	ON_MESSAGE(WM_ADJUST_IMAGE, &CXSliceView::OnAdjustColor)
	ON_MESSAGE(WM_BRIGHTNESS_CHANGE, &CXSliceView::OnBrightnessChange)
	ON_MESSAGE(WM_CONTRAST_CHANGE, &CXSliceView::OnContrastChange)
	
	// Standard printing commands
	ON_CBN_SELCHANGE(IDC_CMB_LAYOUT, &CXSliceView::OnCbnSelchangeCmbLayout)
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

// CXSliceView construction/destruction

CXSliceView::CXSliceView()
{
	// TODO: add construction code here
	m_bCollaboration  = false ;
	m_bControl = false ; 
	memset(m_szCollaborationID, 0, sizeof(m_szCollaborationID)) ;
	m_pwnd = NULL ;
	m_nCollaborationIndex = -1 ;
	m_XRotate = 0 ;
	m_YRotate = 0 ;
	m_chOption = CVSP_SUCCESS ;
}

CXSliceView::~CXSliceView()
{
}

BOOL CXSliceView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CXSliceView drawing

void CXSliceView::OnDraw(CDC* pDC)
{
/*
	CRect viewRct;
	GetClientRect(&viewRct);
	CRect rectdc;
	pDC->GetClipBox(&rectdc);
	//pDC->GetBoundsRect(&rectdc);
*/
	CXSliceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	
/*
	//////////////////////////////////////////////////////////////////////////
	CRect rcClient;		
	GetClientRect(rcClient);	// See Note 1


	CDC MemDC;
	CBitmap MemBitmap;

	MemDC.CreateCompatibleDC(pDC);
	MemBitmap.CreateCompatibleBitmap(pDC,rcClient.right,rcClient.bottom);

	CBitmap *pOldBitmap = MemDC.SelectObject(&MemBitmap);

	pDoc->Draw(&MemDC);

	pDC->BitBlt(0,0,rcClient.right,rcClient.bottom,&MemDC,0,0,SRCCOPY);	//See Note 3
	MemDC.SelectObject(pOldBitmap);

	//////////////////////////////////////////////////////////////////////////
*/
	pDoc->Draw(pDC);
}


// CXSliceView printing

BOOL CXSliceView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CXSliceView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CXSliceView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}


// CXSliceView diagnostics

#ifdef _DEBUG
void CXSliceView::AssertValid() const
{
	CView::AssertValid();
}

void CXSliceView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CXSliceDoc* CXSliceView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CXSliceDoc)));
	return (CXSliceDoc*)m_pDocument;
}
#endif //_DEBUG


// CXSliceView message handlers

int CXSliceView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here
	
	return 0;
}

//************************************
// Method:    OnCbnSelchangeCmbLayout
// FullName:  CXSliceView::OnCbnSelchangeCmbLayout
// Access:    public 
// Returns:   void
// Qualifier:
// Purpose:   
//************************************
void CXSliceView::OnCbnSelchangeCmbLayout()
{
	// TODO: Add your control notification handler code here
	Invalidate();
}

//************************************
// Method:    OnEraseBkgnd
// FullName:  CXSliceView::OnEraseBkgnd
// Access:    public 
// Returns:   BOOL
// Qualifier:
// Parameter: CDC * pDC
// Purpose:   
//************************************
BOOL CXSliceView::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	CBrush brNew(SV_VIEW_BGND);  //Creates a blue brush
	CBrush* pOldBrush = (CBrush*)pDC->SelectObject(&brNew);

	CRect rc;
	pDC->GetClipBox(rc); // Gets the co-ordinates of the client
	// area to repaint.
	pDC->PatBlt(0,0,rc.Width(),rc.Height(),PATCOPY);
	// Repaints client area with current brush.
	pDC->SelectObject(pOldBrush);

	return TRUE;    // Prevents the execution of return
}

//************************************
// Method:    OnLButtonDown
// FullName:  CXSliceView::OnLButtonDown
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: UINT nFlags
// Parameter: CPoint point
// Purpose:   
//************************************
void CXSliceView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	//SetCapture();
	

	if(m_bCollaboration && m_bControl)
	{
		m_CollborationPoint.x = point.x ;
		m_CollborationPoint.y = point.y ;
		m_pwnd->PostMessage(WM_COLLABORATION_OPERATION_START, m_nCollaborationIndex, 0) ;
	}
	updateOrthoSlices(point);

	prePoint = point;
	CView::OnLButtonDown(nFlags, point);
}

//************************************
// Method:    OnLButtonUp
// FullName:  CXSliceView::OnLButtonUp
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: UINT nFlags
// Parameter: CPoint point
// Purpose:   
//************************************
void CXSliceView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	ReleaseCapture();
	CView::OnLButtonUp(nFlags, point);
}

//************************************
// Method:    OnMouseMove
// FullName:  CXSliceView::OnMouseMove
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: UINT nFlags
// Parameter: CPoint point
// Purpose:   
//************************************
void CXSliceView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	INT retcode = SV_NORMAL;

	if (nFlags == MK_LBUTTON)
	{
		m_chOption = CVSP_ORTHOGONAL_RUN ;
		updateOrthoSlices(point);
		updateVolumeRotation(prePoint, point);
		prePoint = point;
		
		if(m_bCollaboration && m_bControl)
		{
			if(m_chOption == CVSP_ORTHOGONAL_RUN)
			{
				m_CollborationPoint.x = point.x ;
				m_CollborationPoint.y = point.y ;
				m_pwnd->PostMessage(WM_COLLABORATION_OPERATION_RUN, m_nCollaborationIndex, 0) ;
			}
			else if(m_chOption == CVSP_VOLUME_RUN)
			{
				CXSliceDoc* pDoc = GetDocument();
				ASSERT_VALID(pDoc);

				m_XRotate = pDoc->m_XRotate ;
				m_YRotate = pDoc->m_YRotate ;
				m_pwnd->PostMessage(WM_COLLABORATION_OPERATION_RUN, m_nCollaborationIndex, 0) ;
			}
			
		}

	}

	if(nFlags == MK_RBUTTON)
	{
		
	}

	CView::OnMouseMove(nFlags, point);
}

//************************************
// Method:    updateOrthoSlices
// FullName:  CXSliceView::updateOrthoSlices
// Access:    private 
// Returns:   int
// Qualifier:
// Parameter: CPoint in_ptMousePos
// Purpose:   
//************************************
int CXSliceView::updateOrthoSlices(CPoint in_ptMousePos)
{
	int retcode = SV_NORMAL;
	CXSliceDoc* pDoc = (CXSliceDoc*) GetDocument();

	//VOLUME MOUSE HANDLE [3/5/2009 QUYPS]
	//if(pDoc->GetActiveSlice(in_ptMousePos) != NULL)
	CViewSliceObj* pSlc = pDoc->GetActiveSlice(in_ptMousePos);
	if(( pSlc!= NULL) && (pSlc->m_nType != eVolume))
	{
		retcode = pDoc->UpdateSlices(in_ptMousePos);
		//if(retcode == SV_NORMAL) Invalidate();
		if(retcode == SV_NORMAL) 
		{
			CDC* pDC = GetDC();
			pDoc->Draw(pDC);
		}			
	}	
	return retcode;
}

//************************************
// Method:    OnMouseWheel
// FullName:  CXSliceView::OnMouseWheel
// Access:    public 
// Returns:   BOOL
// Qualifier:
// Parameter: UINT nFlags
// Parameter: short zDelta
// Parameter: CPoint pt
// Purpose:   
//************************************
BOOL CXSliceView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default
	//TRACE("\nDelta: %d and Pos: (%d,%d)", zDelta, pt.x, pt.y);
	INT nSliceChange = zDelta/WHEEL_DELTA;
	//convert from screen coordinate to view coordinate
	CRect rect;
	CPoint viewPos(pt);
	ScreenToClient(&viewPos);	
	updateOrthoSlices(nSliceChange, viewPos);
	return CView::OnMouseWheel(nFlags, zDelta, viewPos);
}

//************************************
// Method:    updateOrthoSlices
// FullName:  CXSliceView::updateOrthoSlices
// Access:    private 
// Returns:   int
// Qualifier:
// Parameter: int in_nDifferential
// Purpose:   
//************************************
int CXSliceView::updateOrthoSlices(int in_nDifferential,CPoint in_ptMousePos)
{
	int retcode = SV_NORMAL;
	CXSliceDoc* pDoc = (CXSliceDoc*) GetDocument();

	//VOLUME MOUSE HANDLE [3/5/2009 QUYPS]
	//if(pDoc->GetActiveSlice(in_ptMousePos) != NULL)
	CViewSliceObj* pSlc = pDoc->GetActiveSlice(in_ptMousePos);
	if(( pSlc!= NULL) && (pSlc->m_nType != eVolume))
	{
		retcode = pDoc->UpdateSlices(in_nDifferential, in_ptMousePos);
		//if(retcode == SV_NORMAL) Invalidate();
		if(retcode == SV_NORMAL) 
		{
			CDC* pDC = GetDC();
			pDoc->Draw(pDC);
		}	
	}

	return retcode;
}

//************************************
// Method:    updateGrayScale
// FullName:  CXSliceView::updateGrayScale
// Access:    private 
// Returns:   int
// Qualifier:
// Parameter: int in_nBright
// Parameter: int in_nContrast
// Purpose:   
//************************************
int CXSliceView::updateGrayScale(int in_nBright, int in_nContrast)
{
	int retcode = SV_NORMAL;
	CXSliceDoc* pDoc = (CXSliceDoc*) GetDocument();
	if (pDoc == NULL)
	{
		return SV_SYSTEM_ERR;
	}
	LUT lu;
	lu.nBirght = in_nBright;
	lu.nContrast = in_nContrast;
	retcode = pDoc->UpdateLuminance(lu);
	//if (retcode == SV_NORMAL) Invalidate();
	if(retcode == SV_NORMAL) 
	{
		CDC* pDC = GetDC();
		pDoc->Draw(pDC);
	}	
	return retcode;
}

LRESULT CXSliceView::OnBrightnessChange(WPARAM wParam, LPARAM lParam)
{
	TRACE0("\nCustom message done! OnBrightnessChange");
	return 0;
}

LRESULT CXSliceView::OnContrastChange(WPARAM wParam, LPARAM lParam)
{
	TRACE0("\nCustom message done! OnContrastChange");
	return 0;
}

LRESULT CXSliceView::OnAdjustColor(WPARAM wParam, LPARAM lParam)
{
	INT nBright = (INT) wParam;
	INT nContrast = (INT)lParam;
	TRACE2("\nOnAdjustColor(bright:%d *, cons:%d *)", nBright, nContrast);
	return updateGrayScale(nBright,nContrast);	
}

// VOLUME TEST [4/17/2008 QUYPS]
#define  ROTATE_STEP 1
void CXSliceView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	CXSliceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)	return;

	int retcode = SV_NORMAL;
	switch(nChar)
	{
	case VK_LEFT:
		pDoc->m_YRotate = -ROTATE_STEP;
		pDoc->m_RotateAxis = 1;
		//if (pDoc->m_YRotate > 360) pDoc->m_YRotate -= 360;
		break;

	case VK_RIGHT:
		pDoc->m_YRotate = +ROTATE_STEP;
		pDoc->m_RotateAxis = 1;
		//if (pDoc->m_YRotate < -360) pDoc->m_YRotate += 360;
		break;

	case VK_UP:
		pDoc->m_XRotate = -ROTATE_STEP;
		pDoc->m_RotateAxis = 0;
		//if (pDoc->m_XRotate > 360) pDoc->m_XRotate -= 360;
		break;

	case VK_DOWN:
		pDoc->m_XRotate = +ROTATE_STEP;
		pDoc->m_RotateAxis = 0;
		//if (pDoc->m_XRotate < -360) pDoc->m_XRotate += 360;
		break;

	default:

		break;
	}
	CDC* pDC = GetDC();
	retcode = pDoc->ReRenderVol(pDC);
	CView::OnKeyDown(nChar, nRepCnt, nFlags);	
}

//************************************
// Method:    updateVolumeRotation
// FullName:  CXSliceView::updateVolumeRotation
// Access:    private 
// Returns:   int
// Qualifier:
// Parameter: POINT in_prePoint
// Parameter: POINT in_point
//************************************
int CXSliceView::updateVolumeRotation( POINT in_prePoint, POINT in_point )
{
	// TODO: Add your message handler code here and/or call default
	CXSliceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)	return SV_SYSTEM_ERR;

	int retcode = SV_NORMAL;
	
	CViewSliceObj* pSlc = pDoc->GetActiveSlice(in_point);
	if(( pSlc!= NULL) && (pSlc->m_nType == eVolume))
	{
		CDC* pDC = GetDC();
		INT yRotate = in_point.x - in_prePoint.x;
		INT xRotate = in_point.y - in_prePoint.y;

		//Rotate the volume
		pDoc->m_XRotate = xRotate * ROTATE_STEP;
		pDoc->m_RotateAxis = 0;
		retcode = pDoc->ReRenderVol(pDC);

		if(retcode == SV_NORMAL)
		{
			pDoc->m_YRotate = yRotate * ROTATE_STEP;
			pDoc->m_RotateAxis = 1;
			retcode = pDoc->ReRenderVol(pDC);
		}

		m_chOption = CVSP_VOLUME_RUN ;
	}
	
	return retcode;	
}


void CXSliceView::SetCollaboration(const char *szCollaborationID, int nIndex, CWnd *pFrame) 
{
	m_bCollaboration = true ;
	sprintf(m_szCollaborationID, "%s", szCollaborationID) ;
	m_pwnd = pFrame ;
	m_nCollaborationIndex = nIndex ;
}


void CXSliceView::SetControl(bool bcontrol)
{
	m_bControl = bcontrol ;
}

void CXSliceView::OperationStart(CPoint point)
{

	updateOrthoSlices(point);
	prePoint = point;
}

void CXSliceView::OperationRun(CPoint point)
{
	updateOrthoSlices(point);
	updateVolumeRotation(prePoint, point);
	prePoint = point;
}

void CXSliceView::OperationRun(double xRotate, double yRRotate)
{
	CXSliceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	int retcode = SV_NORMAL;

	CDC* pDC = GetDC();
	//Rotate the volume
	pDoc->m_XRotate = xRotate  ;
	pDoc->m_RotateAxis = 0;
	retcode = pDoc->ReRenderVol(pDC);
	
	if(retcode == SV_NORMAL)
	{
		pDoc->m_YRotate = yRRotate ;
		pDoc->m_RotateAxis = 1;
		retcode = pDoc->ReRenderVol(pDC);
	}
}
