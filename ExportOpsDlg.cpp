// ExportOpsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SliceView.h"
#include "ExportOpsDlg.h"


// CExportOpsDlg dialog

IMPLEMENT_DYNAMIC(CExportOpsDlg, CDialog)

CExportOpsDlg::CExportOpsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExportOpsDlg::IDD, pParent)
	, m_bRawFormat(TRUE)
{

}

CExportOpsDlg::~CExportOpsDlg()
{
}

void CExportOpsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RD_EXOP, m_bRawFormat);
}


BEGIN_MESSAGE_MAP(CExportOpsDlg, CDialog)
END_MESSAGE_MAP()


// CExportOpsDlg message handlers

BOOL CExportOpsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	m_bRawFormat = TRUE;
	
	UpdateData(FALSE);

	// TODO:  Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
