// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "SliceView.h"
#include "MainFrm.h"
#include "XSliceDoc.h"
#include "XSliceView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	ON_WM_CREATE()
	ON_COMMAND(ID_COLLABORATION_COLLABORATIONDIALOG, &CMainFrame::OnCollaborationCollaborationdialog)
	ON_MESSAGE(WM_CCollaborationDialog_CLOSE, OnCloseCollaborationDialog)
	ON_MESSAGE(WM_COLLABORATION_CONECT, OnCollaborationConnect)
	ON_MESSAGE(WM_COLLABORATION_CONTROL, OnCollaborationControl)
	ON_MESSAGE(WM_COLLABORATION_ATTEND, OnCollaborationAttend)
	ON_MESSAGE(WM_COLLABORATION_LEAVE, OnCollaborationLeave)
	ON_MESSAGE(WM_COLLABORATION_CONNECTSUCCESS, OnCollaborationConnectSuccess)
	ON_MESSAGE(WM_COLLABORATION_START, OnCollaborationStart)
	ON_MESSAGE(WM_COLLABORATION_FILEDONE, OnCollaborationFileDone) 
	ON_MESSAGE(WM_COLLABORATION_DIRECTORY, OnCollaborationDirectory)
	ON_MESSAGE(WM_COLLABORATION_CONTROLRESULT, OnCollaborationCollaborationResult)
	ON_MESSAGE(WM_COLLABORATION_OPERATION_START, OnCollaborationOperationStart)
	ON_MESSAGE(WM_COLLABORATION_OPERATION_RUN, OnCollaborationOperationRun)
	ON_MESSAGE(WM_COLLABORATION_OPERATION_GETSTART, OnCollaborationOperationGetStart)
	ON_MESSAGE(WM_COLLABORATION_OPERATION_GETRUN, OnCollaborationOperationGetRun)


	
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	pCollaborationDlg = NULL ;
	pCollaboration =  new CCollaborationClient(this) ;
}

CMainFrame::~CMainFrame()
{
	if (pCollaboration != NULL)
	{

	}
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Delete these three lines if you don't want the toolbar to be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}


// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame message handlers




void CMainFrame::OnCollaborationCollaborationdialog()
{
	if(pCollaborationDlg == NULL)
		pCollaborationDlg = new CCollaborationDialog(this) ;

	if(pCollaborationDlg->GetSafeHwnd() == 0)
	{
		pCollaborationDlg->Create() ;
	}
}



// Custom Handling Messages

LRESULT CMainFrame::OnCloseCollaborationDialog(WPARAM wParam, LPARAM lParam)
{
	pCollaborationDlg->DestroyWindow() ;
	return 0 ;
}


LRESULT CMainFrame::OnCollaborationConnect(WPARAM wParam, LPARAM lParam)
{
	// 이미 연결이 되어 있는 경우에는 서버 연결을 종료 한다.
	if(pCollaboration->IsConnect())
	{
		AfxMessageBox(_T("서버 연결을 종료 합니다!") );
		pCollaboration->Leave() ;
		return 0;
	}

	// 서버에 연결을 신청 한다.
	//CString id, ipaddr ;
	char id[1024] ;
	char ipaddr[1024] ;

	memset(id, 0, sizeof(id) );
	memset(ipaddr, 0, sizeof(ipaddr)) ;



	WideCharToMultiByte( CP_ACP, 0, pCollaborationDlg->m_UserID, -1, id, pCollaborationDlg->m_UserID.GetLength(), NULL, NULL );
	WideCharToMultiByte( CP_ACP, 0, pCollaborationDlg->m_ServerIP, -1, ipaddr, pCollaborationDlg->m_ServerIP.GetLength(), NULL, NULL );
//	GetDlgItemText(IDC_EDIT_IPADDRESS, pCollaborationDlg->m_ServerIP) ;
//	GetDlgItemText(IDC_EDIT_USERID, pCollaborationDlg->m_UserID) ;



	pCollaboration->JoinServer( id, ipaddr) ;
	
	return 0 ;
}

LRESULT CMainFrame::OnCollaborationConnectSuccess(WPARAM wParam, LPARAM lParam)
{
	// 서버 연결이 성공한 경우, Collaboration Dialog의 List에 파일 정보를 설정한다.
	
	std::vector<std::string>::iterator fileItr ;
	fileItr = pCollaboration->GetFileInfoLists().begin() ;
	int i = 0 ;
	CandidateInfo *pCInfo = new CandidateInfo ;
	WCHAR temp[MAX_PATH] ;

	

	while( fileItr != pCollaboration->GetFileInfoLists().end())
	{
	
		LV_ITEM lvi;
		lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.iImage = 0;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.lParam = (LPARAM)pCInfo;
		
		pCollaborationDlg->m_CandidateList.InsertItem(&lvi);
		MultiByteToWideChar( CP_ACP, 0, fileItr->c_str(), -1,  temp, MAX_PATH) ;
		pCollaborationDlg->m_CandidateList.SetItemText(i, 0, temp);

		++fileItr ;
		i++;
	}

	delete pCInfo ;


	return 0 ;
}


LRESULT CMainFrame::OnCollaborationControl(WPARAM wParam, LPARAM lParam)
{
	if(!pCollaboration->IsConnect())
	{
		AfxMessageBox(L"서버에 연결에 안되었습니다!") ;
		return 0 ;
	}

	if(pCollaboration->GetControlInfoLists().empty())
	{
		AfxMessageBox(L"협업작업이 시작된게 없습니다!") ;
		return 0 ;
	}

	if(pCollaborationDlg->m_nControlIndex < 0 )
	{
		AfxMessageBox(L"제대로된 협업 작업을 선택해야 합니다!") ;
		return 0 ;
	}

	

	// 이 부분은 나중에 보강?
	pCollaboration->FloorControRequest(pCollaborationDlg->m_nControlIndex) ;



	return 0 ;
}

LRESULT CMainFrame::OnCollaborationAttend(WPARAM wParam, LPARAM lParam)
{
	return 0 ;

}

LRESULT CMainFrame::OnCollaborationLeave(WPARAM wParam, LPARAM lParam)
{
	return 0 ;
}

LRESULT CMainFrame::OnCollaborationStart(WPARAM wParam, LPARAM lParam)
{
	POSITION pos ;
	int nItem = 0 ;
	WCHAR temp1[1024] ;
	char temp2[1024] ;

	
	// 서버에 협업 작업 시작을 요청 한다. List Ctrl을 선택 해야함
	if(!pCollaboration->IsConnect())
	{
		AfxMessageBox(_T("서버 연결을 먼저 해야 합니다!")) ;
		return 0;
	}

	if(pCollaboration->IsStart())
	{
		AfxMessageBox(_T("이미 세션이 시작중입니다!")) ;
		return 0;
	}

	pos = pCollaborationDlg->m_CandidateList.GetFirstSelectedItemPosition();
	if(pos = NULL)
	{
		AfxMessageBox(_T("협업을 시작할 파일을 선택해야 합니다!")) ;
		return 0;
	}
	


	//nItem = pCollaborationDlg->m_CoInfoList.GetNextSelectedItem(pos);
	pCollaborationDlg->m_CandidateList.GetItemText(pCollaborationDlg->m_nSelectedIndex, 0, temp1, sizeof(temp1)) ;
	WideCharToMultiByte( CP_ACP, 0, temp1, -1, temp2, sizeof(temp1), NULL, NULL );
	pCollaboration->Start(temp2) ;

	//int index = pCollaborationDlg->m_CoInfoList.GetNextSelectedItem() ;
	//if(index <=0 )
	//{
	//	AfxMessageBox("협업을 시작할 파일을 선택해야 합니다!") ;
	//}

	return 0 ;
}

LRESULT CMainFrame::OnCollaborationFileDone(WPARAM wParam, LPARAM lParam)
{
	std::string strOsgFileName ;
	strOsgFileName = pCollaboration->ReadZIP(pCollaboration->GetUserName(), pCollaboration->GetFileName(), "", "") ;

	WCHAR tempfolder[MAX_PATH] ;
	WCHAR temppath[MAX_PATH] ;
	WCHAR temp[1024] ;


	MultiByteToWideChar( CP_ACP, 0, strOsgFileName.c_str(), -1,  tempfolder, MAX_PATH) ;
	GetCurrentDirectory(MAX_PATH, temppath) ;	
	wsprintf(temp, L"%s\\%s\\", temppath, tempfolder) ;	

	// 이제 폴더 경로를 바인딩 하여 파일 정보를 열어 준다.
	((CSliceViewApp*)AfxGetApp())->m_bDownload = true ;
	((CSliceViewApp*)AfxGetApp())->m_sFolderPath = temp ;


	//AfxGetApp()->O

	POSITION posTemplate = AfxGetApp()->GetFirstDocTemplatePosition();
	CDocTemplate* pDocTemplate;
	if (posTemplate != NULL)
	{
		//pDocTemplate->Get
		pDocTemplate = AfxGetApp()->GetNextDocTemplate(posTemplate);
		//pDocTemplate->OpenDocumentFile(NULL);
		pDocTemplate->OpenDocumentFile(NULL); 
	}
	
	POSITION pos = 	pDocTemplate->GetFirstDocPosition() ;
	CDocument *pDoc = pDocTemplate->GetNextDoc(pos) ;
	pos = pDoc->GetFirstViewPosition() ;
	CView *pView = pDoc->GetNextView(pos) ;

	// Collaboration ID
	// Control InfoLists를 업데이트 함
	
	
	pCollaboration->SetControlInfo(pView) ;
	((CXSliceView*)pView)->SetCollaboration(pCollaboration->GetUserNameW().c_str(), pCollaboration->GetControlInfoLists().size()-1, this) ;

	
	
	ControlInfoList *pCInfo = new ControlInfoList ;
	WCHAR tempID[MAX_PATH] ;
	//WCHAR tempUser[50] ;

	

	int i ;
	i = pCollaborationDlg->m_CollaborationList.GetItemCount() ;


	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;;
	lvi.iItem = i;
	lvi.iSubItem = 0;
	lvi.iImage = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	lvi.lParam = (LPARAM)pCInfo;
		
	pCollaborationDlg->m_CollaborationList.InsertItem(&lvi);
	MultiByteToWideChar( CP_ACP, 0, pCollaboration->GetFileInfo().szCollaborationID, -1,  tempID, MAX_PATH) ;
	pCollaborationDlg->m_CollaborationList.SetItemText(i, 0, tempID);

	//MultiByteToWideChar( CP_ACP, 0, pCollaboration->GetFileInfo().szFileName, -1,  tempUser, sizeof(tempUser)) ;
	pCollaborationDlg->m_CollaborationList.SetItemText(i, 1, L"None");


	//GetActiveView()
	



	return 0 ;
}

LRESULT CMainFrame::OnCollaborationDirectory(WPARAM wParam, LPARAM lParam)
{
		
	AfxMessageBox(_T("Downloading the collaboration file!") );
	pCollaboration->StartDownload() ;
	return 0 ;
}


LRESULT CMainFrame::OnCollaborationCollaborationResult(WPARAM wParam, LPARAM lParam)
{
	char opt ;
	int index ;


	index = (int)wParam ;
	opt = (char)lParam ;

	if(opt == CVSP_SUCCESS)
	{
		// 여기서 본인 자신인지 확인하여 View에 Control 가능을 설정
				
		
		((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->SetControl(true) ;


		WCHAR user[50] ;
		MultiByteToWideChar( CP_ACP, 0, pCollaboration->GetControlInfoLists()[index].szControlUser, -1,  user, 50) ;
		pCollaborationDlg->m_CollaborationList.SetItemText(index, 1, user);
	}
	else if(opt == CVSP_REQUEST)
	{
		WCHAR user[50] ;
		MultiByteToWideChar( CP_ACP, 0, pCollaboration->GetControlInfoLists()[index].szControlUser, -1,  user, 50) ;
		pCollaborationDlg->m_CollaborationList.SetItemText(index, 1, user);
	}
	else if(opt == CVSP_RELEASE_BYSERVER || opt == CVSP_RELEASE)
	{
		// 여기서 본인 자신인지 확인하여 View에 Control 가능을 설정
		((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->SetControl(false) ;

		pCollaborationDlg->m_CollaborationList.SetItemText(index, 1, L"None");

	}
	
	
	return 0 ;
}


LRESULT CMainFrame::OnCollaborationOperationStart(WPARAM wParam, LPARAM lParam)
{
	int index = (int)wParam ;
	long x, y ;

	x = ((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->m_CollborationPoint.x ;
	y = ((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->m_CollborationPoint.y ;

	pCollaboration->OperationStart(index, x, y) ;
		

	return 0 ;
}


LRESULT CMainFrame::OnCollaborationOperationRun(WPARAM wParam, LPARAM lParam)
{
	int index = (int)wParam ;
	long x, y ;
	double xRotate, yRotate ;
	
	if(((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->m_chOption == CVSP_ORTHOGONAL_RUN)
	{
		x = ((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->m_CollborationPoint.x ;
		y = ((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->m_CollborationPoint.y ;
		pCollaboration->OperationRun(index, x, y) ; // ORTHOGONAL MODE
	}
	else if(((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->m_chOption == CVSP_VOLUME_RUN)
	{
		xRotate = ((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->m_XRotate ;
		yRotate = ((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->m_YRotate ;
		pCollaboration->OperationRun(index, xRotate, yRotate) ; // VOLUME MODE

	}

	

	
	return 0 ;
}


LRESULT CMainFrame::OnCollaborationOperationGetStart(WPARAM wParam, LPARAM lParam)
{
	int index = (int)wParam ;
	CPoint point ;
	Operation op ;
	pCollaboration->GetOperation(&op) ;
	point.x = op.x ;
	point.y = op.y ;

	
	((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->OperationStart(point) ;


	//x = ((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->m_CollborationPoint.x ;
	//y = ((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->m_CollborationPoint.y ;

	///pCollaboration->OperationRun(index, x, y) ;

	
	return 0 ;
}



LRESULT CMainFrame::OnCollaborationOperationGetRun(WPARAM wParam, LPARAM lParam)
{
	int index = (int)wParam ;
	char chOption = (char)lParam ;

	CPoint point ;
	Operation op ;
	pCollaboration->GetOperation(&op) ;
	
	if(chOption == CVSP_ORTHOGONAL_RUN)
	{
		point.x = op.x ;
		point.y = op.y ;
		((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->OperationRun(point) ;
	}
	else if(chOption == CVSP_VOLUME_RUN)
	{
		((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->OperationRun(op.xRotate, op.yRotate) ;
	}
	
	
	
	
	//x = ((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->m_CollborationPoint.x ;
	//y = ((CXSliceView*)pCollaboration->GetControlInfoLists()[index].pWnd)->m_CollborationPoint.y ;

	//pCollaboration->OperationRun(index, x, y) ;

	
	return 0 ;
}