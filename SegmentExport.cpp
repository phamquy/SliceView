// SegmentExport.cpp : implementation file
//

#include "stdafx.h"
#include "SliceView.h"
#include "SegmentExport.h"


// CSegmentExport dialog

IMPLEMENT_DYNAMIC(CSegmentExportDlg, CDialog)

CSegmentExportDlg::CSegmentExportDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSegmentExportDlg::IDD, pParent)
	, m_sDirPath(_T(""))
	, m_sExt(_T("raw"))
	, m_nType(0)
{

}

CSegmentExportDlg::~CSegmentExportDlg()
{
}

void CSegmentExportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SEGEX_PATH, m_sDirPath);
	DDX_CBIndex(pDX, IDC_SEGEX_FILETYPE, m_nType);
}


BEGIN_MESSAGE_MAP(CSegmentExportDlg, CDialog)
	ON_BN_CLICKED(IDC_SEGEX_BROW, &CSegmentExportDlg::OnBnClickedSegexBrow)
	ON_CBN_SELCHANGE(IDC_SEGEX_FILETYPE, &CSegmentExportDlg::OnCbnSelchangeSegexFiletype)
END_MESSAGE_MAP()


// CSegmentExport message handlers

extern BOOL GetFolder(CString* strSelectedFolder,
					  const char* lpszTitle,
					  const HWND hwndOwner, 
					  const char* strRootFolder, 
					  const char* strStartFolder);

void CSegmentExportDlg::OnBnClickedSegexBrow()
{
	// TODO: Add your control notification handler code here
	CString strFolderPath;
	if (GetFolder(&strFolderPath, "Select DICOM files folder", this->m_hWnd, NULL, NULL)){
		if (!strFolderPath.IsEmpty()){
			strFolderPath = strFolderPath + _T("\\");
			UpdateData(FALSE);
		}
		else
		{
			strFolderPath = _T(".\\");
		}
		m_sDirPath = strFolderPath;
		UpdateData(FALSE);
	}
}

void CSegmentExportDlg::OnCbnSelchangeSegexFiletype()
{
	// TODO: Add your control notification handler code here
	UpdateData();
	switch (m_nType)
	{
	case 0:
		m_sExt = _T("jpg");
		break;
	case 1:
		m_sExt = _T("bmp");
		break;
	case 2:	
		m_sExt = _T("raw");
		break;
	default:
		break;
	}
}

void CSegmentExportDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class
	OnCbnSelchangeSegexFiletype();
	CDialog::OnOK();
}
