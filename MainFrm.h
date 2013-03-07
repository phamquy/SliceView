// MainFrm.h : interface of the CMainFrame class
//
#include "CollaborationDialog.h"

#pragma once

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// Attributes
public:
	CCollaborationDialog *pCollaborationDlg ;
	CCollaborationClient *pCollaboration ;

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCollaborationCollaborationdialog();
	afx_msg LRESULT OnCloseCollaborationDialog(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCollaborationConnect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCollaborationControl(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCollaborationAttend(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCollaborationLeave(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCollaborationConnectSuccess(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCollaborationStart(WPARAM wParam, LPARAM lParam) ;
	afx_msg LRESULT OnCollaborationFileDone(WPARAM wParam, LPARAM lParam) ;
	afx_msg LRESULT OnCollaborationDirectory(WPARAM wParam, LPARAM lParam) ;
	afx_msg LRESULT OnCollaborationCollaborationResult(WPARAM wParam, LPARAM lParam) ;
	afx_msg LRESULT OnCollaborationOperationStart(WPARAM wParam, LPARAM lParam) ;
	afx_msg LRESULT OnCollaborationOperationRun(WPARAM wParam, LPARAM lParam) ;
	afx_msg LRESULT OnCollaborationOperationGetStart(WPARAM wParam, LPARAM lParam) ;
	afx_msg LRESULT OnCollaborationOperationGetRun(WPARAM wParam, LPARAM lParam) ;
	

	
};


