// ChildFrm.cpp : implementation of the CXSliceFrame class
//
#include "stdafx.h"
#include "SliceView.h"
#include "ChildFrm.h"
#include "Common.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CXSliceFrame

IMPLEMENT_DYNCREATE(CXSliceFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CXSliceFrame, CMDIChildWnd)
	ON_WM_CREATE()
END_MESSAGE_MAP()


// CXSliceFrame construction/destruction

CXSliceFrame::CXSliceFrame()
{
	// TODO: add member initialization code here
}

CXSliceFrame::~CXSliceFrame()
{
}


BOOL CXSliceFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying the CREATESTRUCT cs
	if( !CMDIChildWnd::PreCreateWindow(cs) )
		return FALSE;

	return TRUE;
}


// CXSliceFrame diagnostics

#ifdef _DEBUG
void CXSliceFrame::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CXSliceFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG


// CXSliceFrame message handlers

int CXSliceFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here
	//Create Auto check box

	if (!m_wndBar.Create(this, IDD_DIALOGBAR,
		CBRS_TOP | CBRS_GRIPPER |CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_HIDE_INPLACE,
		ID_VIEW_SLICEBAR))
	{
		TRACE0("Failed to create dialog bar m_wndDlgBar\n");
		return -1;		// fail to create
	}
// 	m_wndBar.EnableDocking(CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM|CBRS_FLOAT_MULTI);
// 	EnableDocking(CBRS_ALIGN_ANY);
// 	DockControlBar(&m_wndBar);

	return 0;
}

//************************************
// Method:    GetDisplayInfo
// FullName:  CXSliceFrame::GetDisplayInfo
// Access:    public 
// Returns:   DISPLAYTINFO
// Qualifier:
// Purpose:   
//************************************
DISPLAYTINFO CXSliceFrame::GetDisplayInfo()
{
	DISPLAYTINFO dInfo; 
	dInfo.nlayout = m_wndBar.GetLayoutType();
	dInfo.nMode = m_wndBar.GetLayoutMode();
	dInfo.nContrast = m_wndBar.GetContrast();
	dInfo.nBright = m_wndBar.GetBright();
	dInfo.bLines = m_wndBar.IsDisplayLines();
	dInfo.bInfo = m_wndBar.IsDisplayInfo();
	return dInfo;
}