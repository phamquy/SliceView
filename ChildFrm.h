// ChildFrm.h : interface of the CXSliceFrame class
//


#pragma once
#include "afxext.h"
#include "SliceDlgBar.h"

class CXSliceFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CXSliceFrame)
public:
	CXSliceFrame();

// Attributes
public:

// Operations
public:

// Overrides
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
	virtual ~CXSliceFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	DISPLAYTINFO GetDisplayInfo();
protected:
	// Child toolbar
	CSliceDlgBar m_wndBar;
};
