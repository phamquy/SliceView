#include "StdAfx.h"
#include "LineControl.h"
#include "Common.h"

CLineControl::CLineControl(void)
{
	m_ptPos.x = m_ptPos.y = 0;
	m_rcFrame.top = m_rcFrame.left = m_rcFrame.right= m_rcFrame.bottom = 0;
	m_crColor = RGB(255,0,0);
	m_nLineWidth = 1;
}

CLineControl::~CLineControl(void)
{
}

COLORREF CLineControl::GetColor(void)
{
	return m_crColor;
}

POINT CLineControl::GetPos(void)
{
	return m_ptPos;
}

RECT CLineControl::GetFrame()
{
	return m_rcFrame;
}

BYTE CLineControl::GetLineWidth()
{
	return m_nLineWidth;
}

void CLineControl::SetColor(COLORREF in_color)
{
	m_crColor = in_color;
}

void CLineControl::SetPos(POINT in_ptPos)
{
	m_ptPos = in_ptPos;
}

void CLineControl::SetFrame(RECT in_rcFrm)
{
	m_rcFrame = in_rcFrm;
}

void CLineControl::SetLineWidth(BYTE in_width)
{
	m_nLineWidth = in_width;
}



//************************************
// Method:    Draw
// FullName:  CLineControl::Draw
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CDC * pDC
// Purpose:   
//************************************
int CLineControl::Draw(CDC* pDC)
{
	INT retcode = SV_NORMAL;
	CRect rcFrame(m_rcFrame);
	
	ASSERT((m_ptPos.x <= rcFrame.Width())&&(m_ptPos.y <= rcFrame.Height()));
	CPen pen(PS_SOLID,m_nLineWidth, m_crColor);
	CPen* oldPen = pDC->SelectObject(&pen);
	
	//Draw horizontal line
	pDC->MoveTo(rcFrame.left, m_ptPos.y + rcFrame.top);
	pDC->LineTo(rcFrame.left + max(m_ptPos.x - SV_CTRLLINE_SPC, 0), m_ptPos.y + rcFrame.top);
	pDC->MoveTo(rcFrame.left + min(m_ptPos.x + SV_CTRLLINE_SPC,rcFrame.Width()), m_ptPos.y + rcFrame.top);
	pDC->LineTo(rcFrame.right, m_ptPos.y + rcFrame.top);

	//Draw vertical line
	pDC->MoveTo(rcFrame.left + m_ptPos.x, rcFrame.top);
	pDC->LineTo(rcFrame.left + m_ptPos.x, rcFrame.top + max(m_ptPos.y - SV_CTRLLINE_SPC,0));
	pDC->MoveTo(rcFrame.left + m_ptPos.x, rcFrame.top + min( m_ptPos.y + SV_CTRLLINE_SPC, rcFrame.Height()));
	pDC->LineTo(rcFrame.left + m_ptPos.x, rcFrame.bottom);


	pDC->SelectObject(oldPen);
	return retcode;
}
