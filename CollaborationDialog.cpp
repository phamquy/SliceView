// CollaborationDialog.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "SliceView.h"
#include "CollaborationDialog.h"


// CCollaborationDialog 대화 상자입니다.

IMPLEMENT_DYNAMIC(CCollaborationDialog, CDialog)

CCollaborationDialog::CCollaborationDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CCollaborationDialog::IDD, pParent)
	, m_ServerIP(_T(""))
	, m_UserID(_T(""))
	, m_bAttend(FALSE)
{
	m_bAttend = true ;
	m_pParent = pParent ;
	m_nSelectedIndex = -1 ;
	m_nControlIndex = -1 ;
}

CCollaborationDialog::~CCollaborationDialog()
{
}

void CCollaborationDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_IPADDRESS, m_ServerIP);
	DDX_Text(pDX, IDC_EDIT_USERID, m_UserID);
	DDX_Control(pDX, IDC_LIST_CANDIDATE, m_CandidateList);
	DDX_Control(pDX, IDC_LIST_COLLABORATION, m_CollaborationList);
	DDX_Radio(pDX, IDC_RADIO_ATTEND1, m_bAttend);
}


BEGIN_MESSAGE_MAP(CCollaborationDialog, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CCollaborationDialog::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_START, &CCollaborationDialog::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_CONTROL, &CCollaborationDialog::OnBnClickedButtonControl)
	ON_BN_CLICKED(IDC_RADIO_ATTEND1, &CCollaborationDialog::OnBnClickedRadioAttend1)
	ON_BN_CLICKED(IDC_RADIO_LEAVE, &CCollaborationDialog::OnBnClickedRadioLeave)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_NOTIFY(NM_CLICK, IDC_LIST_CANDIDATE, &CCollaborationDialog::OnNMClickListCandidate)
	ON_NOTIFY(NM_CLICK, IDC_LIST_COLLABORATION, &CCollaborationDialog::OnNMClickListCollaboration)
END_MESSAGE_MAP()


// CCollaborationDialog Messages Handling

void CCollaborationDialog::OnBnClickedButtonConnect()
{
	UpdateData(TRUE) ;
	
	if(m_pParent != NULL)
	{
		m_pParent->PostMessage(WM_COLLABORATION_CONECT, NULL) ;
	}
}

void CCollaborationDialog::OnBnClickedButtonStart()
{
	if(m_pParent != NULL)
	{
		m_pParent->PostMessage(WM_COLLABORATION_START, NULL) ;
	}
}

void CCollaborationDialog::OnBnClickedButtonControl()
{
	if(m_pParent != NULL)
	{
		m_pParent->PostMessage(WM_COLLABORATION_CONTROL, NULL) ;
	}
}

void CCollaborationDialog::OnBnClickedRadioAttend1()
{
	if(m_pParent != NULL)
	{
		m_pParent->PostMessage(WM_COLLABORATION_ATTEND, NULL) ;
	}
}

void CCollaborationDialog::OnBnClickedRadioLeave()
{
	if(m_pParent != NULL)
	{
		m_pParent->PostMessage(WM_COLLABORATION_LEAVE, NULL) ;
	}
}

void CCollaborationDialog::OnClose()
{
	if(m_pParent != NULL)
	{
		m_pParent->PostMessage(WM_CCollaborationDialog_CLOSE, IDCANCEL) ;
	}

	CDialog::OnClose();
}

BOOL CCollaborationDialog::Create()
{
	BOOL bSuccess = CDialog::Create(CCollaborationDialog::IDD, m_pParent);
	
	//

	return bSuccess ;

}

int CCollaborationDialog::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here
	//UpdateData(TRUE) ;
	//m_CandidateList.InsertColumn(0, _T("FileInfo"), LVCFMT_LEFT, 126);

	return 0;
}


BOOL CCollaborationDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	UpdateData(TRUE) ;

	m_CandidateList.InsertColumn(0, _T("FileInfo"), LVCFMT_LEFT, 126);
	m_CollaborationList.InsertColumn(0, _T("Collaboration ID"), LVCFMT_LEFT, 260) ;
	m_CollaborationList.InsertColumn(1, _T("Owner ID"), LVCFMT_LEFT, 126) ;

	


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
void CCollaborationDialog::OnNMClickListCandidate(NMHDR *pNMHDR, LRESULT *pResult)
{
	//LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<NMITEMACTIVATE>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	DWORD dwPos = ::GetMessagePos();
	CPoint point ((int) LOWORD(dwPos), (int) HIWORD(dwPos));
	m_CandidateList.ScreenToClient(&point);
	m_nSelectedIndex = m_CandidateList.HitTest(point) ;


	*pResult = 0;
}

void CCollaborationDialog::OnNMClickListCollaboration(NMHDR *pNMHDR, LRESULT *pResult)
{
	//LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<NMITEMACTIVATE>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	DWORD dwPos = ::GetMessagePos();
	CPoint point ((int) LOWORD(dwPos), (int) HIWORD(dwPos));
	m_CollaborationList.ScreenToClient(&point);
	m_nControlIndex = m_CollaborationList.HitTest(point) ;

	*pResult = 0;
}
