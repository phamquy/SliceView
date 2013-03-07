#include "StdAfx.h"
#include "ViewLayout.h"

CViewLayout::CViewLayout(void)
{
}

CViewLayout::~CViewLayout(void)
{
}

//************************************
// Method:    GetViewRect
// FullName:  CViewLayout::GetViewRect
// Access:    public 
// Returns:   RECT
// Qualifier:
// Parameter: int in_nIndex: index of frame
// Purpose:   
//************************************
RECT CViewLayout::GetViewRect(int in_nIndex)
{
	RECT retRC;
	memset(&retRC,0,sizeof(RECT));
	switch(in_nIndex)
	{
	case eFrame1:
		retRC = m_Layout.rcFrame1;
		break;
	case eFrame2:
		retRC = m_Layout.rcFrame2;
		break;
	case eFrame3:
		retRC = m_Layout.rcFrame3;
	    break;
	case eFrame4:
		retRC = m_Layout.rcFrame4;
	    break;
	default:
	    break;
	}
	return retRC;
}


//************************************
// Method:    GetLayoutType
// FullName:  CViewLayout::GetLayoutType
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: void
// Purpose:   
//************************************
int CViewLayout::GetLayoutType(void)
{
	return m_Layout.ntype;
}


//************************************
// Method:    MakeLayout
// FullName:  CViewLayout::MakeLayout
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: RECT in_rcBoudary
// Parameter: VOLSIZE in_voSize
// Parameter: DISPLAYTINFO * pDinfo
// Purpose:   
//************************************
int CViewLayout::MakeLayout(RECT in_rcBoudary, VOLSIZE in_voSize,DISPLAYTINFO* pDinfo)
{
	int retcode = SV_NORMAL;

	switch(pDinfo->nlayout)
	{
	case SV_LO_CUBE:
		retcode = makeCubeLayout(in_rcBoudary, in_voSize, pDinfo->nMode);
		break;
	case SV_LO_HORZ:
		retcode = makeHorzLayout(in_rcBoudary, in_voSize, pDinfo->nMode);
		break;
	case SV_LO_VERT:
		retcode = makeVertLayout(in_rcBoudary, in_voSize, pDinfo->nMode);
		break;
	default:
		retcode = SV_INVALID_PARAM;
		break;
	}

	return retcode;
	
}

//************************************
// Method:    makeCubeLayout
// FullName:  CViewLayout::makeCubeLayout
// Access:    protected 
// Returns:   int
// Qualifier:
// Parameter: RECT rcBound
// Parameter: VOLSIZE voSize
// Parameter: INT nMode
// Purpose:   
//************************************
int CViewLayout::makeCubeLayout(RECT rcBoundary, VOLSIZE voSize, INT nMode)
{
	int retcode = SV_NORMAL;
	m_Layout.ntype = SV_LO_CUBE;
	m_Layout.rcbound = rcBoundary;
	CRect boundary(rcBoundary);
	INT nCol1_width = 0;
	INT nRow1_height = 0;

	INT nCol2_width = 0;
	INT nRow2_height = 0;

	switch(nMode)
	{
	case eEqual:
		nCol1_width = nCol2_width = (boundary.Width() - 3*SV_LO_BORDER) / 2;
		nRow2_height = nRow1_height = (boundary.Height() - 3*SV_LO_BORDER) / 2;
		break;

	case eAuto:
		// TEMP [2/18/2008 QUYPS]
		nCol1_width = nCol2_width = (boundary.Width() - 3*SV_LO_BORDER) / 2;
		nRow2_height = nRow1_height = (boundary.Height() - 3*SV_LO_BORDER) / 2;
	    break;

	default:
		retcode = SV_INVALID_PARAM;
	    break;
	}

	m_Layout.rcFrame1.top    = m_Layout.rcFrame2.top    = rcBoundary.top           + SV_LO_BORDER;
	m_Layout.rcFrame1.bottom = m_Layout.rcFrame2.bottom = m_Layout.rcFrame1.top    + nRow1_height;
	m_Layout.rcFrame3.top    = m_Layout.rcFrame4.top    = m_Layout.rcFrame1.bottom + SV_LO_BORDER;
	m_Layout.rcFrame3.bottom = m_Layout.rcFrame4.bottom = m_Layout.rcFrame3.top    + nRow2_height;


	m_Layout.rcFrame1.left	= m_Layout.rcFrame3.left	= rcBoundary.left			+ SV_LO_BORDER;
	m_Layout.rcFrame1.right = m_Layout.rcFrame3.right	= m_Layout.rcFrame1.left	+ nCol1_width;
	m_Layout.rcFrame2.left	= m_Layout.rcFrame4.left	= m_Layout.rcFrame1.right	+ SV_LO_BORDER;
	m_Layout.rcFrame2.right = m_Layout.rcFrame4.right	= m_Layout.rcFrame2.left	+ nCol2_width;
	
	return retcode;
}

//************************************
// Method:    makeHorzLayout
// FullName:  CViewLayout::makeHorzLayout
// Access:    protected 
// Returns:   int
// Qualifier:
// Parameter: RECT rcBound
// Parameter: VOLSIZE voSize
// Parameter: INT nMode
// Purpose:   
//************************************
int CViewLayout::makeHorzLayout(RECT rcBound, VOLSIZE voSize, INT nMode)
{
	int retcode = SV_NORMAL;
	m_Layout.ntype = SV_LO_VERT;
	m_Layout.rcbound = rcBound;
	CRect boundary(rcBound);
	INT nRow_height = 0;
	INT nCol1_width = 0;
	INT nCol2_width = 0;
	INT nCol3_width = 0;

	switch(nMode)
	{
	case eEqual:
		nRow_height = boundary.Height() - 2*SV_LO_BORDER;
		nCol3_width = nCol2_width = nCol1_width = (boundary.Width() - 4*SV_LO_BORDER)/3;
		break;

	case eAuto:
		// TEMP [2/18/2008 QUYPS]
		nRow_height = boundary.Height() - 2*SV_LO_BORDER;
		nCol3_width = nCol2_width = nCol1_width = (boundary.Width() - 4*SV_LO_BORDER)/3;
		break;

	default:
		retcode = SV_INVALID_PARAM;
		break;
	}

	m_Layout.rcFrame1.top = m_Layout.rcFrame2.top = m_Layout.rcFrame3.top = boundary.top + SV_LO_BORDER;
	m_Layout.rcFrame1.bottom = m_Layout.rcFrame2.bottom = m_Layout.rcFrame3.bottom = m_Layout.rcFrame1.top + nRow_height;

	//col1
	m_Layout.rcFrame1.left = boundary.left + SV_LO_BORDER;
	m_Layout.rcFrame1.right = m_Layout.rcFrame1.left + nCol1_width;

	//col2
	m_Layout.rcFrame2.left = m_Layout.rcFrame1.right + SV_LO_BORDER;
	m_Layout.rcFrame2.right = m_Layout.rcFrame2.left + nCol2_width;

	//col3
	m_Layout.rcFrame3.left = m_Layout.rcFrame2.right + SV_LO_BORDER;
	m_Layout.rcFrame3.right = m_Layout.rcFrame3.left + nCol3_width;

	m_Layout.rcFrame4 = CRect(0,0,0,0); 
	return retcode;
}
//************************************
// Method:    makeVertLayout
// FullName:  CViewLayout::makeVertLayout
// Access:    protected 
// Returns:   int
// Qualifier:
// Parameter: RECT rcBound
// Parameter: VOLSIZE voSize
// Parameter: INT nMode
// Purpose:   
//************************************
int CViewLayout::makeVertLayout(RECT rcBound, VOLSIZE voSize, INT nMode)
{
	int retcode = SV_NORMAL;
	m_Layout.ntype = SV_LO_VERT;
	m_Layout.rcbound = rcBound;
	CRect boundary(rcBound);
	INT nCol_width = 0;
	INT nRow1_height = 0;
	INT nRow2_height = 0;
	INT nRow3_height = 0;

	switch(nMode)
	{
	case eEqual:
		nCol_width = boundary.Width() - 2*SV_LO_BORDER;
		nRow1_height = nRow2_height = nRow3_height = (boundary.Height() - 4*SV_LO_BORDER) / 3;
		break;

	case eAuto:
		// TEMP [2/18/2008 QUYPS]
		nCol_width = boundary.Width() - 2*SV_LO_BORDER;
		nRow1_height = nRow2_height = nRow3_height = (boundary.Height() - 4*SV_LO_BORDER) / 3;

		break;

	default:
		retcode = SV_INVALID_PARAM;
		break;
	}
	
	m_Layout.rcFrame1.left	= m_Layout.rcFrame2.left	= m_Layout.rcFrame3.left	= boundary.left + SV_LO_BORDER;
	m_Layout.rcFrame1.right = m_Layout.rcFrame2.right	= m_Layout.rcFrame3.right	= m_Layout.rcFrame1.left + nCol_width;
	
	//row1
	m_Layout.rcFrame1.top = boundary.top + SV_LO_BORDER;
	m_Layout.rcFrame1.bottom = m_Layout.rcFrame1.top + nRow1_height;
	//row2
	m_Layout.rcFrame2.top = m_Layout.rcFrame1.bottom + SV_LO_BORDER;
	m_Layout.rcFrame2.bottom = m_Layout.rcFrame2.top + nRow2_height;
	//row3
	m_Layout.rcFrame3.top = m_Layout.rcFrame2.bottom + SV_LO_BORDER;
	m_Layout.rcFrame3.bottom = m_Layout.rcFrame3.top + nRow3_height;

	m_Layout.rcFrame4 = CRect(0,0,0,0); 
	return retcode;
}

//************************************
// Method:    Draw
// FullName:  CViewLayout::Draw
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CDC * pDC
// Purpose:   
//************************************
int CViewLayout::Draw(CDC* pDC)
{
	CBrush br(RGB(0,0,0));

// 	pDC->FillRect(&(m_Layout.rcFrame1),&br);
// 	pDC->FillRect(&(m_Layout.rcFrame2),&br);
// 	pDC->FillRect(&(m_Layout.rcFrame3),&br);
// 	pDC->FillRect(&(m_Layout.rcFrame4),&br);

	pDC->Rectangle(&(m_Layout.rcFrame1));
	pDC->Rectangle(&(m_Layout.rcFrame2));
	pDC->Rectangle(&(m_Layout.rcFrame3));
	pDC->Rectangle(&(m_Layout.rcFrame4));
	return 0;
}
