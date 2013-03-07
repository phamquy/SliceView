// DlgDcmBrowse.cpp : implementation file
//

#include "stdafx.h"
#include "SliceView.h"
#include "DlgDcmBrowse.h"
#include "Common.h"
#include "Utility.h"

// CDlgDcmBrowse dialog

IMPLEMENT_DYNAMIC(CDlgDcmBrowse, CDialog)

CDlgDcmBrowse::CDlgDcmBrowse(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgDcmBrowse::IDD, pParent)
	, m_sFolderPath(_T(""))
{

}

CDlgDcmBrowse::~CDlgDcmBrowse()
{
}

void CDlgDcmBrowse::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILE_LIST, m_ctlFileList);
	DDX_Text(pDX, IDC_FOLDERPATH, m_sFolderPath);
}


BEGIN_MESSAGE_MAP(CDlgDcmBrowse, CDialog)
	ON_BN_CLICKED(IDC_CMD_BROWSE, &CDlgDcmBrowse::OnBnClickedCmdBrowse)
	ON_BN_CLICKED(IDC_CMD_SELECTALL, &CDlgDcmBrowse::OnBnClickedCmdSelectall)
	ON_BN_CLICKED(IDC_CMD_DESELECTALL, &CDlgDcmBrowse::OnBnClickedCmdDeselectall)
END_MESSAGE_MAP()


// Browse for folder routines [3/16/2008 QUYPS]
CString strTmpPath;

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	TCHAR szDir[MAX_PATH];
	
	switch(uMsg)
	{
	case BFFM_INITIALIZED:
		if (lpData)
		{
			wcscpy_s(szDir, strTmpPath.GetBuffer(strTmpPath.GetLength()));
			SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)szDir);
		}
		break;
	case BFFM_SELCHANGED: 
	{
		if (SHGetPathFromIDList((LPITEMIDLIST) lParam ,szDir)){
			SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)szDir);
		}
		break;
						  
	}
	default:
		break;
	}
	return 0;
}

BOOL GetFolder(CString* strSelectedFolder,
			   const char* lpszTitle,
			   const HWND hwndOwner, 
			   const char* strRootFolder, 
			   const char* strStartFolder)
{
	char pszDisplayName[MAX_PATH];
	LPITEMIDLIST lpID;
	BROWSEINFOA bi;

	bi.hwndOwner = hwndOwner;
	if (strRootFolder == NULL){
		bi.pidlRoot = NULL;
	}else{
		LPITEMIDLIST  pIdl = NULL;
		IShellFolder* pDesktopFolder;
		char          szPath[MAX_PATH];
		OLECHAR       olePath[MAX_PATH];
		ULONG         chEaten;
		ULONG         dwAttributes;

		strcpy_s(szPath, strRootFolder);
		if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder)))
		{
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szPath, -1, olePath, MAX_PATH);
			pDesktopFolder->ParseDisplayName(NULL, NULL, olePath, &chEaten, &pIdl, &dwAttributes);
			pDesktopFolder->Release();
		}
		bi.pidlRoot = pIdl;
	}
	bi.pszDisplayName = pszDisplayName;
	bi.lpszTitle = lpszTitle;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
	bi.lpfn = BrowseCallbackProc;
	if (strStartFolder == NULL){
		bi.lParam = FALSE;
	}else{
		strTmpPath.Format(L"%s", strStartFolder);
		bi.lParam = TRUE;
	}
	bi.iImage = NULL;
	lpID = SHBrowseForFolderA(&bi);
	if (lpID != NULL){
		BOOL b = SHGetPathFromIDListA(lpID, pszDisplayName);
		if (b == TRUE){
			TCHAR* pTemp;
			CUtility::AnsiToUnicode(pszDisplayName, &pTemp);
			strSelectedFolder->Format(L"%s",pTemp);
			delete[] pTemp;
			return TRUE;
		}
	}else{
		strSelectedFolder->Empty();
	}
	return FALSE;
}



// CDlgDcmBrowse message handlers
void CDlgDcmBrowse::OnBnClickedCmdBrowse()
{
	// TODO: Add your control notification handler code here
	CString strFolderPath;
	if (GetFolder(&strFolderPath, "Select DICOM files folder", this->m_hWnd, NULL, NULL)){
		if (!strFolderPath.IsEmpty()){
			m_sFolderPath = strFolderPath + _T("\\");
			UpdateData(FALSE);
		}
	}
	
	CFileFind finder; 
	CString sFilter = m_sFolderPath + _T("*.dcm");
	BOOL bWorking = finder.FindFile(sFilter);

	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		CString filename =  finder.GetFileName();
		INT idx = m_ctlFileList.AddString(filename);
		m_ctlFileList.SetSel(idx);
	} 	
}

void CDlgDcmBrowse::BrowseDownloads()
{
	CFileFind finder; 
	CString sFilter = m_sFolderPath + _T("*.dcm");
	BOOL bWorking = finder.FindFile(sFilter);

	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		CString filename =  finder.GetFileName();
		INT idx = m_ctlFileList.AddString(filename);
		m_ctlFileList.SetSel(idx);
	} 	

}



void CDlgDcmBrowse::OnBnClickedCmdSelectall()
{
	// TODO: Add your control notification handler code here
	for (int i=0; i< m_ctlFileList.GetCount(); i++)
	{
		m_ctlFileList.SetSel(i);
	}	
}

void CDlgDcmBrowse::OnBnClickedCmdDeselectall()
{
	// TODO: Add your control notification handler code here
	for (int i=0; i< m_ctlFileList.GetCount(); i++)
	{
		m_ctlFileList.SetSel(i, FALSE);
	}	
}

//************************************
// Method:    GetFileList
// FullName:  CDlgDcmBrowse::GetFileList
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: CStringList * out_pFileList
// Purpose:   
//************************************
int CDlgDcmBrowse::GetFileList(CStringList* out_pFileList)
{	
	int retcode = SV_NORMAL;

	for (INT i=0; i< m_oFileList.GetCount(); i++)
	{
		POSITION pos;
		pos = m_oFileList.FindIndex(i);
		if (pos == NULL)
		{
			retcode = SV_SYSTEM_ERR;
			break;
		}
		else
		{
			out_pFileList->AddTail(m_sFolderPath + m_oFileList.GetAt(pos));
		}		
	}
	return retcode;
}




void CDlgDcmBrowse::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class
	for (int i=0;i < m_ctlFileList.GetCount();i++)
	{	
		CString tempStr;
		INT isSelected = m_ctlFileList.GetSel(i);
		if (isSelected > 0)
		{	
			m_ctlFileList.GetText(i, tempStr);
			m_oFileList.AddTail(tempStr);					
		}		
	}

#ifdef _DEBUG
	POSITION pos;
	for (INT i=0; i<m_oFileList.GetCount(); i++)
	{
		pos = m_oFileList.FindIndex(i);
		TRACE(_T("\n%s"), m_oFileList.GetAt(pos));
	}
#endif

	CDialog::OnOK();
}


BOOL CDlgDcmBrowse::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	UpdateData(TRUE) ;

	if(m_sFolderPath != L"")
		BrowseDownloads() ;


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}