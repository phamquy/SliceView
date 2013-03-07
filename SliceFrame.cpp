#include "StdAfx.h"
#include "SliceFrame.h"

CSliceFrame::CSliceFrame(void)
{
}

CSliceFrame::~CSliceFrame(void)
{
}

//************************************
// FullName:  CSliceFrame::Draw
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CDC * pDC
// Purpose:   
//************************************
int CSliceFrame::Draw(CDC* pDC)
{
	int retcode = SV_NORMAL;
		
	CRect rcbound(m_SlcFrm.rcBound);
	CRect rcImage(m_SlcFrm.rcImage);
	rcImage.InflateRect(SV_SLC_BORDER,0);
	rcbound.InflateRect(SV_SLC_BORDER,SV_SLC_BORDER);
	
	CPen curPen(PS_SOLID, SV_SLC_BORDER, SV_SLC_BORDER_CLR);
	CBrush curBr(m_SlcFrm.clrBgrnd);

	CPen* oldPen = pDC->SelectObject(&curPen);
	CBrush* pOldBr = pDC->SelectObject(&curBr);

	pDC->MoveTo(rcbound.TopLeft());
	pDC->LineTo(rcbound.right, rcbound.top);
	pDC->LineTo(rcbound.BottomRight());
	pDC->LineTo(rcbound.left, rcbound.bottom);
	pDC->LineTo(rcbound.TopLeft());
	//pDC->Rectangle(&(rcImage));
	
	pDC->SelectObject(pOldBr);
	pDC->SelectObject(oldPen);
	return retcode;
}

//************************************
// Method:    Update
// FullName:  CSliceFrame::Update
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: SLCFRAME in_SlcFrame
// Purpose:   
//************************************
int CSliceFrame::Update(SLCFRAME in_SlcFrame)
{
	int retcode = SV_NORMAL;
	m_SlcFrm = in_SlcFrame;
	return retcode;
}


//************************************
// Method:    IsOnImage
// FullName:  CSliceFrame::IsOnImage
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: CPoint in_ptPos
// Purpose:   
//************************************
BOOL CSliceFrame::IsOnImage(CPoint in_ptPos)
{
	CRect rcImageArea(m_SlcFrm.rcImage);
	BOOL isOn = TRUE;

	if( (in_ptPos.x < rcImageArea.left) || (in_ptPos.x > rcImageArea.right))
	{
		isOn = FALSE;
	}

	if( (in_ptPos.y < rcImageArea.top) || (in_ptPos.y > rcImageArea.bottom))
	{
		isOn = FALSE;
	}

	return isOn;
}
