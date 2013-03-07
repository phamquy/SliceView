#pragma once
#include "afxcmn.h"
#include "./CollaborationControl.h"

#define WM_CCollaborationDialog_CLOSE	1002
#define WM_COLLABORATION_CONECT			1003
#define WM_COLLABORATION_CONTROL		1004
#define WM_COLLABORATION_ATTEND			1005
#define WM_COLLABORATION_LEAVE			1006
#define WM_COLLABORATION_START			1007


// CCollaborationDialog 대화 상자입니다.

class CCollaborationDialog : public CDialog
{
	DECLARE_DYNAMIC(CCollaborationDialog)

public:
	CCollaborationDialog(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CCollaborationDialog();
	BOOL Create() ;

// 대화 상자 데이터입니다.
	enum { IDD = IDD_COLLABORATION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	CWnd			*m_pParent ;
	CCollaborationClient *pCollaboration ;
	CString m_ServerIP;
	CString m_UserID;
	CListCtrl m_CandidateList;
	CListCtrl m_CollaborationList;
	int				m_nSelectedIndex ;
	int				m_nControlIndex ;
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonControl();
	afx_msg void OnBnClickedRadioAttend1();
	afx_msg void OnBnClickedRadioLeave();
	afx_msg void OnClose();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL OnInitDialog();

	afx_msg void OnNMClickListCandidate(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickListCollaboration(NMHDR *pNMHDR, LRESULT *pResult);
	BOOL m_bAttend;
};


typedef struct candidateInfo_t
{
	TCHAR					pFileName[MAX_PATH];
	//TCHAR					pTelePhone[20];
	//TCHAR					pCustomerAddr[50];
}CandidateInfo;


typedef struct controlInfoList_t
{
	TCHAR					pCollaborationID[MAX_PATH];
	TCHAR					pOwnerUser[50];
	//TCHAR					pCustomerAddr[50];
}ControlInfoList;


//typedef struct controlInfo_t
//{
//	CView*					pView ;
//	
//}ControlInfo_t ;